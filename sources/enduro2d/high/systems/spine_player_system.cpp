/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include <enduro2d/high/systems/spine_player_system.hpp>

#include <enduro2d/high/components/spine_player.hpp>
#include <enduro2d/high/components/spine_renderer.hpp>

#include <spine/AnimationState.h>
#include <spine/Skeleton.h>

namespace e2d
{
    spine_player_system::spine_player_system() = default;

    spine_player_system::~spine_player_system() noexcept = default;

    void spine_player_system::process(ecs::registry& owner, ecs::entity_id) {
        float dt = the<engine>().delta_time();
        owner.for_joined_components<spine_player, spine_renderer>([dt](
            const ecs::const_entity&,
            spine_player& player,
            const spine_renderer& renderer)
        {
            spSkeleton* skeleton = renderer.skeleton().operator->();
            spAnimationState* anim_state = player.animation().operator->();
            
            if ( !skeleton || !anim_state ) {
                return;
            }

            spSkeleton_update(skeleton, dt);
            spAnimationState_update(anim_state, dt);
            spAnimationState_apply(anim_state, skeleton);
            spSkeleton_updateWorldTransform(skeleton);
        });
    }
}
