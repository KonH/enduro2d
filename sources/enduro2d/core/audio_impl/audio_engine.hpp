#pragma once

#include <enduro2d/core/audio_engine.hpp>

#define E2D_AUDIO_MODE_NONE 1
#define E2D_AUDIO_MODE_FMOD 2
#define E2D_AUDIO_MODE_BASS 2

#ifndef E2D_AUDIO_MODE
#  if defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_IOS
#    define E2D_AUDIO_MODE E2D_AUDIO_MODE_BASS
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_LINUX
#    define E2D_AUDIO_MODE E2D_AUDIO_MODE_BASS
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_MACOSX
#    define E2D_AUDIO_MODE E2D_AUDIO_MODE_BASS
#  elif defined(E2D_PLATFORM) && E2D_PLATFORM == E2D_PLATFORM_WINDOWS
#    define E2D_AUDIO_MODE E2D_AUDIO_MODE_BASS
#  endif
#endif

#ifndef E2D_AUDIO_MODE
#  error E2D_AUDIO_MODE not detected
#endif
