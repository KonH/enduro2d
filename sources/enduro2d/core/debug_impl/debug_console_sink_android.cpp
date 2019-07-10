/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "debug.hpp"

#if defined(E2D_DEBUG_CONSOLE_SINK_MODE) && E2D_DEBUG_CONSOLE_SINK_MODE == E2D_DEBUG_CONSOLE_SINK_MODE_ANDROID

#include <android/log.h>

namespace e2d
{
    bool debug_console_sink::on_message(debug::level lvl, str_view text) noexcept {
        android_LogPriority android_log_lvl = ANDROID_LOG_DEFAULT;
		switch ( lvl ) {
			case debug::level::trace:
				android_log_lvl = ANDROID_LOG_DEBUG;
				break;
			case debug::level::warning:
				android_log_lvl = ANDROID_LOG_WARN;
				break;
			case debug::level::error:
				android_log_lvl = ANDROID_LOG_ERROR;
				break;
			case debug::level::fatal:
				android_log_lvl = ANDROID_LOG_FATAL;
				break;
        }
        (void)__android_log_write(android_log_lvl, "enduro2d", text.data());
        return true;
    }
}

#endif
