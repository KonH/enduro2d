/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include <enduro2d/core/platform.hpp>
#include <enduro2d/core/vfs.hpp>

#define E2D_PLATFORM_MODE_NONE 1
#define E2D_PLATFORM_MODE_IOS 2
#define E2D_PLATFORM_MODE_LINUX 3
#define E2D_PLATFORM_MODE_MACOSX 4
#define E2D_PLATFORM_MODE_WINDOWS 5
#define E2D_PLATFORM_MODE_ANDROID 6

#ifndef E2D_PLATFORM_MODE
#  if defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_IOS
#    define E2D_PLATFORM_MODE E2D_PLATFORM_MODE_IOS
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_LINUX
#    define E2D_PLATFORM_MODE E2D_PLATFORM_MODE_LINUX
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_MACOSX
#    define E2D_PLATFORM_MODE E2D_PLATFORM_MODE_MACOSX
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_WINDOWS
#    define E2D_PLATFORM_MODE E2D_PLATFORM_MODE_WINDOWS
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_ANDROID
#    define E2D_PLATFORM_MODE E2D_PLATFORM_MODE_ANDROID
#  endif
#endif

#ifndef E2D_PLATFORM_MODE
#  error E2D_PLATFORM_MODE not detected
#endif

namespace e2d
{
    //
    // platform::internal_state
    //

    class platform::internal_state : private e2d::noncopyable {
    public:
        internal_state(int argc, char *argv[]);
        virtual ~internal_state() noexcept = default;
    public:
        const vector<str>& command_line_arguments() const noexcept;
        virtual void register_scheme_aliases(vfs&) = 0;
    private:
        vector<str> command_line_arguments_;
    };
}
