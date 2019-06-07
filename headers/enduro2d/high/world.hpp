/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "_high.hpp"

#include "prefab.hpp"
#include "gobject.hpp"

namespace e2d
{
    class world final : public module<world> {
    public:
        enum priorities : ecs::priority_t {
            priority_update_section_begin = 0,
                priority_pre_update = 500,
                priority_update = 1000,
                priority_post_update = 1500,
            priority_update_section_end = 2000,

            priority_render_section_begin = 2500,
                priority_pre_render = 3000,
                priority_render = 3500,
                priority_post_render = 4000,
            priority_render_section_end = 4500
        };
    public:
        world() = default;
        ~world() noexcept final;

        [[nodiscard]] ecs::registry& registry() noexcept;
        [[nodiscard]] const ecs::registry& registry() const noexcept;

        [[nodiscard]] gobject_iptr instantiate();
        [[nodiscard]] gobject_iptr instantiate(const prefab& prefab);
        void destroy_instance(const gobject_iptr& inst) noexcept;

        [[nodiscard]] gobject_iptr resolve(ecs::entity_id ent) const noexcept;
        [[nodiscard]] gobject_iptr resolve(const ecs::const_entity& ent) const noexcept;
    private:
        ecs::registry registry_;
        hash_map<ecs::entity_id, gobject_iptr> gobjects_;
    };
}
