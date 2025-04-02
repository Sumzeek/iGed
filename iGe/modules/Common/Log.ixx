module;
#include "iGeMacro.h"

export module iGe.Log;
import std;
import spdlog;

namespace iGe
{

export class IGE_API Log {
public:
    static void Init();

    static std::shared_ptr<spdlog::logger>& GetCoreLogger();
    static std::shared_ptr<spdlog::logger>& GetClientLogger();

private:
    Log();
    ~Log();

    static std::shared_ptr<spdlog::logger> m_CoreLogger;
    static std::shared_ptr<spdlog::logger> m_ClientLogger;
};

} // namespace iGe
