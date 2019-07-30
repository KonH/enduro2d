/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include <enduro2d/high/systems/actor_system.hpp>

#include <enduro2d/high/components/actor.hpp>
#include <enduro2d/high/node.hpp>

namespace e2d
{
    actor_system::actor_system() = default;

    actor_system::~actor_system() noexcept = default;

    void actor_system::process(ecs::registry& owner) {
        owner.for_each_component<actor>([](
            const ecs::const_entity&,
            actor& actor)
        {
            node_iptr node = actor.node();

            node->update_local_matrix();
            node->update_world_matrix();
        });
    }
}
