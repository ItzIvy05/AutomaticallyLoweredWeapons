#pragma once

#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include <SimpleIni.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <thread>
#include <utility>

using namespace std::literals;

namespace logger = F4SE::log;
