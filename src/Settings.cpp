#include "PCH.h"

#include "Settings.h"

namespace ALW
{
	namespace
	{
		constexpr auto INI_PATH = "Data/F4SE/Plugins/AutomaticallyLoweredWeapons.ini";
		constexpr auto SECTION = "General";
	}

	Settings& Settings::GetSingleton()
	{
		static Settings singleton;
		return singleton;
	}

	void Settings::Load()
	{
		CSimpleIniA ini;
		ini.SetUnicode();

		if (const auto result = ini.LoadFile(INI_PATH); result < 0) {
			logger::info("no ini at {}, using defaults", INI_PATH);
			return;
		}

		_lowerDelay = static_cast<float>(ini.GetDoubleValue(SECTION, "fLowerDelay", _lowerDelay));
		_lowerAfterSprint = ini.GetBoolValue(SECTION, "bLowerAfterSprint", _lowerAfterSprint);
		_requireLightOff = ini.GetBoolValue(SECTION, "bRequireLightOff", _requireLightOff);

		if (_lowerDelay < 0.0f) {
			_lowerDelay = 0.0f;
		}

		logger::info("fLowerDelay = {}", _lowerDelay);
		logger::info("bLowerAfterSprint = {}", _lowerAfterSprint);
		logger::info("bRequireLightOff = {}", _requireLightOff);
	}
}
