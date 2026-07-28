#pragma once

namespace ALW
{
	class Settings
	{
	public:
		[[nodiscard]] static Settings& GetSingleton();

		void Load();

		[[nodiscard]] float LowerDelay() const noexcept { return _lowerDelay; }
		[[nodiscard]] bool LowerAfterSprint() const noexcept { return _lowerAfterSprint; }
		[[nodiscard]] bool RequireLightOff() const noexcept { return _requireLightOff; }

	private:
		float _lowerDelay{ 3.0f };
		bool _lowerAfterSprint{ false };
		bool _requireLightOff{ false };
	};
}
