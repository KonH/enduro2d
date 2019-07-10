/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include <enduro2d/core/debug.hpp>

#define E2D_DEBUG_CONSOLE_SINK_MODE_DEFAULT 1
#define E2D_DEBUG_CONSOLE_SINK_MODE_MSVC 2
#define E2D_DEBUG_CONSOLE_SINK_MODE_ANDROID 3

#ifndef E2D_DEBUG_CONSOLE_SINK_MODE
#  if defined(E2D_COMPILER) && E2D_COMPILER == E2D_COMPILER_MSVC && \
      defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_WINDOWS
#    define E2D_DEBUG_CONSOLE_SINK_MODE E2D_DEBUG_CONSOLE_SINK_MODE_MSVC
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_ANDROID
#    define E2D_DEBUG_CONSOLE_SINK_MODE E2D_DEBUG_CONSOLE_SINK_MODE_ANDROID
#  else
#    define E2D_DEBUG_CONSOLE_SINK_MODE E2D_DEBUG_CONSOLE_SINK_MODE_DEFAULT
#  endif
#endif

namespace e2d
{
    str log_text_format(debug::level lvl, str_view text);
}
