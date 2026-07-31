#include "PCH.h"

#include "LowerWeapon.h"
#include "Settings.h"

namespace ALW
{
	namespace
	{
		constexpr auto PLUGIN_FILE = "AutoLoweredWeapons.esp"sv;
		constexpr auto VANILLA_FILE = "Fallout4.esm"sv;

		constexpr std::uint32_t GUN_DOWN_FP = 0x09B20E;
		constexpr std::uint32_t GUN_UP_FP = 0x09B20F;
		constexpr std::uint32_t LOWER_WEAPON_PERK = 0x000802;

		constexpr std::size_t IS_IN_COMBAT_VFUNC = 0xFE;
		constexpr std::size_t PROCESS_EVENT_VFUNC = 0x01;
		constexpr std::size_t PERFORM_INPUT_VFUNC = 0x00;

		constexpr auto SPRINT_STOP = "PASprintStop"sv;

		constexpr std::array TRACKED_TAGS{
			"weaponDraw"sv,
			"sightedStateExit"sv,
			"WeaponFire"sv,
			"weaponSwing"sv,
			"ReloadComplete"sv,
			SPRINT_STOP,
			"initiateStart"sv
		};
	}

	namespace
	{
		std::uint32_t g_lastHandled = static_cast<std::uint32_t>(-1);

		void ScanForToggle(const RE::InputEvent* a_queueHead)
		{
			const auto key = Settings::GetSingleton().ToggleKey();
			if (key < 0) {
				return;
			}

			const bool debug = Settings::GetSingleton().DebugLog();

			for (auto event = a_queueHead; event; event = event->next) {
				const auto button = event->As<RE::ButtonEvent>();
				if (!button || !button->QJustPressed()) {
					continue;
				}

				if (debug) {
					logger::info("button device {} idCode {} time {}",
						std::to_underlying(button->device.get()), button->idCode, button->timeCode);
				}

				if (button->device.get() != RE::INPUT_DEVICE::kKeyboard || button->idCode != key) {
					continue;
				}

				if (button->timeCode == g_lastHandled) {
					continue;
				}

				g_lastHandled = button->timeCode;

				if (debug) {
					logger::info("toggle key pressed");
				}

				LowerWeapon::GetSingleton().Toggle();
			}
		}
	}

	LowerWeapon& LowerWeapon::GetSingleton()
	{
		static LowerWeapon singleton;
		return singleton;
	}

	void LowerWeapon::MenuInput(RE::MenuControls* a_this, const RE::InputEvent* a_queueHead)
	{
		ScanForToggle(a_queueHead);
		_originalMenuInput(a_this, a_queueHead);
	}

	void LowerWeapon::PlayerInput(RE::PlayerControls* a_this, const RE::InputEvent* a_queueHead)
	{
		ScanForToggle(a_queueHead);
		_originalPlayerInput(a_this, a_queueHead);
	}

	void LowerWeapon::InstallInput()
	{
		if (_inputInstalled || Settings::GetSingleton().ToggleKey() < 0) {
			return;
		}

		REL::Relocation<std::uintptr_t> menuVtable{ RE::VTABLE::MenuControls[0] };
		_originalMenuInput = menuVtable.write_vfunc(PERFORM_INPUT_VFUNC, MenuInput);

		REL::Relocation<std::uintptr_t> playerVtable{ RE::VTABLE::PlayerControls[0] };
		_originalPlayerInput = playerVtable.write_vfunc(PERFORM_INPUT_VFUNC, PlayerInput);

		_inputInstalled = true;

		logger::info("toggle key {} registered", Settings::GetSingleton().ToggleKey());
	}

	void LowerWeapon::Install()
	{
		REL::Relocation<std::uintptr_t> graphVtable{ RE::VTABLE::BSAnimationGraphManager[0] };
		_originalProcessEvent = graphVtable.write_vfunc(PROCESS_EVENT_VFUNC, ProcessEvent);

		REL::Relocation<std::uintptr_t> playerVtable{ RE::VTABLE::PlayerCharacter[0] };
		_originalIsInCombat = playerVtable.write_vfunc(IS_IN_COMBAT_VFUNC, IsInCombat);

		_worker = std::jthread([this](std::stop_token a_token) { Wait(a_token); });

		logger::info("hooks installed");
	}

	void LowerWeapon::OnDataReady()
	{
		for (std::size_t i = 0; i < TRACKED_TAGS.size(); ++i) {
			_tracked[i] = RE::BSFixedString(TRACKED_TAGS[i]);
		}

		_sprintStop = RE::BSFixedString(SPRINT_STOP);

		const auto handler = RE::TESDataHandler::GetSingleton();
		if (!handler) {
			logger::error("no data handler");
			return;
		}

		_gunDown = handler->LookupForm<RE::TESIdleForm>(GUN_DOWN_FP, VANILLA_FILE);
		if (!_gunDown) {
			logger::error("failed to find GunDownFP");
		}

		_gunUp = handler->LookupForm<RE::TESIdleForm>(GUN_UP_FP, VANILLA_FILE);
		if (!_gunUp) {
			logger::error("failed to find GunUpFP");
		}

		_perk = handler->LookupForm<RE::BGSPerk>(LOWER_WEAPON_PERK, PLUGIN_FILE);
		if (!_perk) {
			logger::warn("{} is not loaded, the walk animation fix is unavailable", PLUGIN_FILE);
		}

		InstallInput();
	}

	void LowerWeapon::OnPlayerReady()
	{
		InstallInput();
		Cancel();

		_lowered = false;
		_manualHold = false;

		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		_inCombat.store(_originalIsInCombat(player), std::memory_order_relaxed);

		if (_perk) {
			const auto base = player->GetNPC();
			if (base && !base->GetPerkIndex(_perk)) {
				player->AddPerk(_perk, 0);
				logger::info("added {}", _perk->GetFormEditorID());
			}
		}
	}

	bool LowerWeapon::IsPlayerGraph(
		const RE::BSAnimationGraphManager* a_manager,
		const RE::BSAnimationGraphEvent& a_event)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return false;
		}

		const auto holder = reinterpret_cast<std::uint64_t>(static_cast<RE::TESObjectREFR*>(player));
		if (a_event.holderID == holder) {
			return true;
		}

		if (a_event.holderID == player->GetHandle().native_handle()) {
			return true;
		}

		if (!player->Get3D()) {
			return false;
		}

		RE::BSTSmartPointer<RE::BSAnimationGraphManager> manager;
		if (!player->GetAnimationGraphManagerImpl(manager) || !manager) {
			return false;
		}

		return manager.get() == a_manager;
	}

	bool LowerWeapon::IsTracked(const RE::BSFixedString& a_tag) const
	{
		return std::ranges::find(_tracked, a_tag) != _tracked.end();
	}

	RE::BSEventNotifyControl LowerWeapon::ProcessEvent(
		RE::BSAnimationGraphManager* a_this,
		const RE::BSAnimationGraphEvent& a_event,
		RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source)
	{
		auto& self = GetSingleton();

		if (self._gunDown && self.IsTracked(a_event.tag)) {
			const bool mine = IsPlayerGraph(a_this, a_event);

			if (Settings::GetSingleton().DebugLog()) {
				logger::info("anim event '{}' holder {:X} graph {} -> player: {}",
					a_event.tag.c_str(), a_event.holderID, static_cast<const void*>(a_this), mine);
			}

			if (mine) {
				self._lowered = false;
				self.Arm();

				if (a_event.tag == self._sprintStop && Settings::GetSingleton().LowerAfterSprint()) {
					F4SE::GetTaskInterface()->AddTask([&self]() { self.Lower(); });
				}
			}
		}

		return _originalProcessEvent(a_this, a_event, a_source);
	}

	bool LowerWeapon::IsInCombat(const RE::Actor* a_this)
	{
		const bool result = _originalIsInCombat(a_this);

		auto& self = GetSingleton();

		if (self._inCombat.exchange(result, std::memory_order_relaxed) != result) {
			if (result) {
				self.Cancel();
			} else {
				self.Arm();
			}
		}

		return result;
	}

	void LowerWeapon::Arm()
	{
		if (!Settings::GetSingleton().AutoLower() || _manualHold) {
			return;
		}

		const auto delay = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
			std::chrono::duration<float>(Settings::GetSingleton().LowerDelay()));

		{
			std::scoped_lock lock(_lock);
			_deadline = std::chrono::steady_clock::now() + delay;
			_armed = true;
			++_generation;
		}

		_signal.notify_all();
	}

	void LowerWeapon::Cancel()
	{
		{
			std::scoped_lock lock(_lock);
			_armed = false;
			++_generation;
		}

		_signal.notify_all();
	}

	void LowerWeapon::Wait(std::stop_token a_token)
	{
		while (!a_token.stop_requested()) {
			bool expired = false;

			{
				std::unique_lock lock(_lock);

				if (!_signal.wait(lock, a_token, [this]() { return _armed; })) {
					return;
				}

				const auto generation = _generation;
				const auto deadline = _deadline;

				if (!_signal.wait_until(lock, a_token, deadline, [this, generation]() { return _generation != generation; })) {
					if (a_token.stop_requested()) {
						return;
					}

					_armed = false;
					expired = true;
				}
			}

			if (expired) {
				F4SE::GetTaskInterface()->AddTask([this]() { Lower(); });
			}
		}
	}

	bool LowerWeapon::CanLower(RE::PlayerCharacter* a_player) const
	{
		if (!a_player->GetWeaponMagicDrawn()) {
			return false;
		}

		switch (a_player->gunState) {
		case RE::GUN_STATE::kRelaxed:
		case RE::GUN_STATE::kSighted:
		case RE::GUN_STATE::kFireSighted:
		case RE::GUN_STATE::kBlocked:
			return false;
		default:
			break;
		}

		if (a_player->IsInCombat()) {
			return false;
		}

		if (Settings::GetSingleton().RequireLightOff() && a_player->IsPipboyLightOn()) {
			return false;
		}

		return true;
	}

	bool LowerWeapon::PlayIdle(RE::PlayerCharacter* a_player, RE::TESIdleForm* a_idle, bool a_testConditions) const
	{
		if (!a_idle || !a_player->currentProcess || !a_player->Get3D()) {
			return false;
		}

		a_player->currentProcess->SetupSpecialIdle(
			*a_player, RE::DEFAULT_OBJECT::kActionIdle, a_idle, a_testConditions, nullptr);

		return true;
	}

	void LowerWeapon::Lower()
	{
		if (!Settings::GetSingleton().AutoLower() || _manualHold) {
			return;
		}

		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !CanLower(player)) {
			return;
		}

		if (PlayIdle(player, _gunDown, true)) {
			_lowered = true;
		}
	}

	void LowerWeapon::Raise()
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !player->GetWeaponMagicDrawn()) {
			return;
		}

		if (PlayIdle(player, _gunUp, false)) {
			_lowered = false;
		}
	}

	void LowerWeapon::Toggle()
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		if (!player || !player->GetWeaponMagicDrawn()) {
			return;
		}

		Cancel();

		if (_lowered) {
			_manualHold = true;
			Raise();
		} else {
			_manualHold = false;
			if (PlayIdle(player, _gunDown, true)) {
				_lowered = true;
			}
		}
	}
}
