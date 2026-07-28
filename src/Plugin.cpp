#include "PCH.h"

#include "LowerWeapon.h"
#include "Settings.h"

namespace
{
	constexpr auto PLUGIN_NAME = "AutomaticallyLoweredWeapons"sv;
	constexpr auto PLUGIN_AUTHOR = "Ivy"sv;
	constexpr auto PLUGIN_VERSION_STRING = "2.0.0"sv;
	constexpr REL::Version PLUGIN_VERSION{ 2, 0, 0, 0 };

	void OnMessage(F4SE::MessagingInterface::Message* a_message)
	{
		if (!a_message) {
			return;
		}

		switch (a_message->type) {
		case F4SE::MessagingInterface::kGameDataReady:
			ALW::LowerWeapon::GetSingleton().OnDataReady();
			break;
		case F4SE::MessagingInterface::kNewGame:
		case F4SE::MessagingInterface::kPostLoadGame:
			ALW::LowerWeapon::GetSingleton().OnPlayerReady();
			break;
		default:
			break;
		}
	}
}

#if defined(F4_RUNTIME_PRENG)

namespace
{
	void SetupLog()
	{
		auto path = logger::log_directory();
		if (!path) {
			return;
		}

		*path /= std::format("{}.log", PLUGIN_NAME);

		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S:%e] [%l] %v"s);
	}
}

extern "C" [[maybe_unused]] __declspec(dllexport) bool F4SEAPI F4SEPlugin_Query(
	const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	SetupLog();

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = PLUGIN_NAME.data();
	a_info->version = PLUGIN_VERSION[0];

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor");
		return false;
	}

	const auto version = a_f4se->RuntimeVersion();
	if (version > F4SE::RUNTIME_1_10_163) {
		logger::critical("unsupported runtime v{}", version.string());
		return false;
	}

	logger::info("{} v{} loaded on runtime v{}", PLUGIN_NAME, PLUGIN_VERSION_STRING, version.string());

	return true;
}

extern "C" [[maybe_unused]] __declspec(dllexport) bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	ALW::Settings::GetSingleton().Load();
	ALW::LowerWeapon::GetSingleton().Install();

	F4SE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}

#elif defined(F4_RUNTIME_AE)

extern "C" [[maybe_unused]] __declspec(dllexport) constinit auto F4SEPlugin_Version = []() noexcept {
	F4SE::PluginVersionData data{};

	data.PluginName(PLUGIN_NAME);
	data.AuthorName(PLUGIN_AUTHOR);
	data.PluginVersion(PLUGIN_VERSION);
	data.UsesAddressLibrary(true);
	data.IsLayoutDependent(true);

	return data;
}();

extern "C" [[maybe_unused]] __declspec(dllexport) bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	logger::info("{} v{} loaded on runtime v{}",
		PLUGIN_NAME, PLUGIN_VERSION_STRING, a_f4se->RuntimeVersion().string());

	ALW::Settings::GetSingleton().Load();
	ALW::LowerWeapon::GetSingleton().Install();

	F4SE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}

#else
#error "F4_RUNTIME_PRENG or F4_RUNTIME_AE must be defined"
#endif
