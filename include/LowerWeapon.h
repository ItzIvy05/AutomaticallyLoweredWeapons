#pragma once

namespace ALW
{
	class LowerWeapon
	{
	public:
		[[nodiscard]] static LowerWeapon& GetSingleton();

		void Install();
		void InstallInput();
		void OnDataReady();
		void OnPlayerReady();

		void Toggle();

	private:
		static RE::BSEventNotifyControl ProcessEvent(
			RE::BSAnimationGraphManager* a_this,
			const RE::BSAnimationGraphEvent& a_event,
			RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source);

		static bool IsInCombat(const RE::Actor* a_this);

		static void MenuInput(RE::MenuControls* a_this, const RE::InputEvent* a_queueHead);
		static void PlayerInput(RE::PlayerControls* a_this, const RE::InputEvent* a_queueHead);

		[[nodiscard]] static bool IsPlayerGraph(
			const RE::BSAnimationGraphManager* a_manager,
			const RE::BSAnimationGraphEvent& a_event);

		[[nodiscard]] bool IsTracked(const RE::BSFixedString& a_tag) const;

		void Arm();
		void Cancel();
		void Wait(std::stop_token a_token);
		void Lower();
		void Raise();

		[[nodiscard]] static bool IsLightOn(RE::PlayerCharacter* a_player);

		[[nodiscard]] bool CanLower(RE::PlayerCharacter* a_player) const;
		[[nodiscard]] bool PlayIdle(RE::PlayerCharacter* a_player, RE::TESIdleForm* a_idle, bool a_testConditions) const;

		RE::TESIdleForm* _gunDown{ nullptr };
		RE::TESIdleForm* _gunUp{ nullptr };
		RE::BGSPerk* _perk{ nullptr };

		std::array<RE::BSFixedString, 7> _tracked;
		RE::BSFixedString _sprintStop;

		std::jthread _worker;
		std::mutex _lock;
		std::condition_variable_any _signal;
		std::chrono::steady_clock::time_point _deadline{};
		std::uint64_t _generation{ 0 };
		bool _armed{ false };
		bool _lowered{ false };
		bool _manualHold{ false };
		bool _inputInstalled{ false };

		static inline REL::Relocation<decltype(&LowerWeapon::MenuInput)> _originalMenuInput;
		static inline REL::Relocation<decltype(&LowerWeapon::PlayerInput)> _originalPlayerInput;
		std::atomic<bool> _inCombat{ false };

		static inline REL::Relocation<decltype(&LowerWeapon::ProcessEvent)> _originalProcessEvent;
		static inline REL::Relocation<decltype(&LowerWeapon::IsInCombat)> _originalIsInCombat;
	};
}
