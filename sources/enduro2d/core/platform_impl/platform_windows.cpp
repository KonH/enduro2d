/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "platform.hpp"

#if defined(E2D_PLATFORM_MODE) && E2D_PLATFORM_MODE == E2D_PLATFORM_MODE_WINDOWS

#include <windows.h>

namespace
{
    using namespace e2d;

    class platform_internal_state_windows final : public platform::internal_state {
    public:
        platform_internal_state_windows(int argc, char *argv[])
        : internal_state(argc, argv) {
            TIMECAPS tc;
            if ( MMSYSERR_NOERROR == ::timeGetDevCaps(&tc, sizeof(tc)) ) {
                if ( TIMERR_NOERROR == ::timeBeginPeriod(tc.wPeriodMin) ) {
                    timers_resolution_ = tc.wPeriodMin;
                }
            }
        }

        ~platform_internal_state_windows() noexcept {
            if ( timers_resolution_ > 0 ) {
                ::timeEndPeriod(timers_resolution_);
            }
        }
    private:
        UINT timers_resolution_{0u};
    };
}

namespace e2d
{
    platform::platform(int argc, char *argv[])
    : state_(new platform_internal_state_windows(argc, argv)) {}
}

int main(int argc, char *argv[]) {
    return e2d_main(argc, argv);
}

#endif
