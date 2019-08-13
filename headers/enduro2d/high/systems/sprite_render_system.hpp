/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "../_high.hpp"

namespace e2d
{
    class sprite_render_system final : public ecs::system {
    public:
        sprite_render_system();
        ~sprite_render_system() noexcept final;
        void process(ecs::registry& owner) override;
    private:
        class internal_state;
        std::unique_ptr<internal_state> state_;
    };
}
