/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include <enduro2d/core/vfs.hpp>

#define E2D_ASSET_MODE_NONE 1
#define E2D_ASSET_MODE_ANDROID 2

#ifndef E2D_ASSET_MODE
#  if defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_ANDROID
#    define E2D_ASSET_MODE E2D_ASSET_MODE_ANDROID
#  else
#    define E2D_ASSET_MODE E2D_ASSET_MODE_NONE
#  endif
#endif

#ifndef E2D_ASSET_MODE
#  error E2D_ASSET_MODE not detected
#endif
