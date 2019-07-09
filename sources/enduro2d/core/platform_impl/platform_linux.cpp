/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "platform.hpp"

#if defined(E2D_PLATFORM_MODE) && E2D_PLATFORM_MODE == E2D_PLATFORM_MODE_LINUX

namespace
{
    using namespace e2d;

    class platform_internal_state_linux final : public platform::internal_state {
    public:
        platform_internal_state_linux(int argc, char *argv[])
            : internal_state(argc, argv) {}

        ~platform_internal_state_linux() noexcept = default;
    };
}

namespace e2d
{
    platform::platform(int argc, char *argv[])
    : state_(new platform_internal_state_linux(argc, argv)) {}
}

int main(int argc, char *argv[]) {
    return e2d_main(argc, argv);
}

#endif
