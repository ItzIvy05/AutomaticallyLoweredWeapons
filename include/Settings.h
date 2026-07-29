#pragma once

namespace ALW
{
	class Settings
	{
	public:
		[[nodiscard]] static Settings& GetSingleton();

		void Load();

		[[nodiscard]] bool AutoLower() const noexcept { return _autoLower; }
		[[nodiscard]] float LowerDelay() const noexcept { return _lowerDelay; }
		[[nodiscard]] bool LowerAfterSprint() const noexcept { return _lowerAfterSprint; }
		[[nodiscard]] bool RequireLightOff() const noexcept { return _requireLightOff; }
		[[nodiscard]] std::int32_t ToggleKey() const noexcept { return _toggleKey; }
		[[nodiscard]] bool DebugLog() const noexcept { return _debugLog; }

	private:
		bool _autoLower{ true };
		float _lowerDelay{ 3.0f };
		bool _lowerAfterSprint{ false };
		bool _requireLightOff{ false };
		std::int32_t _toggleKey{ -1 };
		bool _debugLog{ false };
	};
}
