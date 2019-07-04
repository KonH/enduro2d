/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "_base.hpp"

//
// E2D_COMPILER
//

#define E2D_COMPILER_CLANG 1
#define E2D_COMPILER_GCC   2
#define E2D_COMPILER_MSVC  3

#ifndef E2D_COMPILER
#  error E2D_COMPILER not detected
#endif

//
// E2D_PLATFORM
//

#define E2D_PLATFORM_LINUX   1
#define E2D_PLATFORM_IOS     2
#define E2D_PLATFORM_MACOSX  3
#define E2D_PLATFORM_WINDOWS 4
#define E2D_PLATFORM_ANDROID 5

#ifndef E2D_PLATFORM
#  error E2D_PLATFORM not detected
#endif

//
// E2D_BUILD_MODE
//

#define E2D_BUILD_MODE_DEBUG   1
#define E2D_BUILD_MODE_RELEASE 2

#ifndef E2D_BUILD_MODE
#  if defined(RELEASE) || defined(_RELEASE) || defined(NDEBUG)
#    define E2D_BUILD_MODE E2D_BUILD_MODE_RELEASE
#  else
#    define E2D_BUILD_MODE E2D_BUILD_MODE_DEBUG
#  endif
#endif

#ifndef E2D_BUILD_MODE
#  error E2D_BUILD_MODE not detected
#endif
