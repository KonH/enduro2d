/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "platform.hpp"

#if defined(E2D_PLATFORM_MODE) && E2D_PLATFORM_MODE == E2D_PLATFORM_MODE_ANDROID

#include <enduro2d/utils/java.hpp>

struct AAssetManager;

namespace e2d
{

    class platform_internal_state_android final : public platform::internal_state {
    public:
        platform_internal_state_android(int argc, char *argv[]);
        ~platform_internal_state_android() noexcept;

        AAssetManager* asset_manager() const noexcept;
    private:
        AAssetManager* asset_manager_ = nullptr;
    };
}

#endif
