#pragma once

namespace ALW
{
	class LowerWeapon
	{
	public:
		[[nodiscard]] static LowerWeapon& GetSingleton();

		void Install();
		void OnDataReady();
		void OnPlayerReady();

	private:
		static RE::BSEventNotifyControl ProcessEvent(
			RE::BSAnimationGraphManager* a_this,
			const RE::BSAnimationGraphEvent& a_event,
			RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source);

		static bool IsInCombat(const RE::Actor* a_this);

		[[nodiscard]] static bool IsPlayerGraph(const RE::BSAnimationGraphManager* a_manager);

		[[nodiscard]] bool IsTracked(const RE::BSFixedString& a_tag) const;

		void Arm();
		void Cancel();
		void Wait(std::stop_token a_token);
		void Lower();

		[[nodiscard]] bool CanLower(RE::PlayerCharacter* a_player) const;

		RE::TESIdleForm* _gunDown{ nullptr };
		RE::BGSPerk* _perk{ nullptr };

		std::array<RE::BSFixedString, 7> _tracked;
		RE::BSFixedString _sprintStop;

		std::jthread _worker;
		std::mutex _lock;
		std::condition_variable_any _signal;
		std::chrono::steady_clock::time_point _deadline{};
		std::uint64_t _generation{ 0 };
		bool _armed{ false };
		std::atomic<bool> _inCombat{ false };

		static inline REL::Relocation<decltype(&LowerWeapon::ProcessEvent)> _originalProcessEvent;
		static inline REL::Relocation<decltype(&LowerWeapon::IsInCombat)> _originalIsInCombat;
	};
}
