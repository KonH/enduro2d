/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "../_high.hpp"

namespace e2d
{
    class model_render_system final : public ecs::system {
    public:
        model_render_system();
        ~model_render_system() noexcept final;
        void process(ecs::registry& owner, ecs::entity_id data_source) override;
    };
}
