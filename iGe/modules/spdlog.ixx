module;
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

export module spdlog;

namespace spdlog
{

export using spdlog::logger;
export using spdlog::level::level_enum;
export using spdlog::set_pattern;
export using spdlog::stdout_color_mt;

} // namespace spdlog
