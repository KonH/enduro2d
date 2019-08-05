/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include <enduro2d/core/debug.hpp>
#include <enduro2d/core/render.hpp>
#include <enduro2d/core/window.hpp>

#define E2D_RENDER_MODE_NONE 1
#define E2D_RENDER_MODE_OPENGL 2
#define E2D_RENDER_MODE_OPENGLES 3

#ifndef E2D_RENDER_MODE
#  if defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_IOS
#    define E2D_RENDER_MODE E2D_RENDER_MODE_OPENGLES
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_LINUX
#    define E2D_RENDER_MODE E2D_RENDER_MODE_OPENGL
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_MACOSX
#    define E2D_RENDER_MODE E2D_RENDER_MODE_OPENGL
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_WINDOWS
#    define E2D_RENDER_MODE E2D_RENDER_MODE_OPENGL
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_ANDROID
#    define E2D_RENDER_MODE E2D_RENDER_MODE_OPENGLES
#  endif
#endif

#ifndef E2D_RENDER_MODE
#  error E2D_RENDER_MODE not detected
#endif
