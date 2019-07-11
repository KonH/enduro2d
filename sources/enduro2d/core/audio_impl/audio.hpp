/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include <enduro2d/core/debug.hpp>
#include <enduro2d/core/audio.hpp>

#define E2D_AUDIO_MODE_NONE 1
#define E2D_AUDIO_MODE_BASS 2

#if !defined(E2D_AUDIO_MODE) && defined(E2D_PLATFORM)
#  if E2D_PLATFORM == E2D_PLATFORM_IOS
#    define E2D_AUDIO_MODE E2D_AUDIO_MODE_NONE
#  elif E2D_PLATFORM == E2D_PLATFORM_LINUX && defined(E2D_HAS_BASS)
#    define E2D_AUDIO_MODE E2D_AUDIO_MODE_BASS
#  elif E2D_PLATFORM == E2D_PLATFORM_MACOSX && defined(E2D_HAS_BASS)
#    define E2D_AUDIO_MODE E2D_AUDIO_MODE_BASS
#  elif E2D_PLATFORM == E2D_PLATFORM_WINDOWS && defined(E2D_HAS_BASS)
#    define E2D_AUDIO_MODE E2D_AUDIO_MODE_BASS
#  elif E2D_PLATFORM == E2D_PLATFORM_ANDROID && defined(E2D_HAS_BASS)
#    define E2D_AUDIO_MODE E2D_AUDIO_MODE_BASS
#  endif
#endif

#ifndef E2D_AUDIO_MODE
    E2D_MESSAGE( "E2D_AUDIO_MODE not detected" )
#   define E2D_AUDIO_MODE E2D_AUDIO_MODE_NONE
#endif
