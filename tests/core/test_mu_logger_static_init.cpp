#include "Core/Utilities/Log/MuLogger.h"

namespace
{
// Constructed before the logger, so this destructor runs after the logging statics
// would normally have been destroyed. Test-process exit is part of the regression.
struct LateLogger
{
    ~LateLogger()
    {
        const auto logger = mu::log::Get("exit-regression");
        logger->info("late static logger self-check");
        logger->flush();
    }
};

LateLogger g_lateLogger;
const auto g_earlyLogger = mu::log::Get("core");
}

std::shared_ptr<spdlog::logger> GetEarlyLogger()
{
    return g_earlyLogger;
}
