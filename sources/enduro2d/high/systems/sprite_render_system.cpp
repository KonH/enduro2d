/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include <enduro2d/core/render.hpp>
#include <enduro2d/core/engine.hpp>

#include <enduro2d/high/systems/sprite_render_system.hpp>

#include <enduro2d/high/components/actor.hpp>
#include <enduro2d/high/components/camera.hpp>
#include <enduro2d/high/components/sprite_renderer.hpp>
#include <enduro2d/high/components/renderer.hpp>

#include <enduro2d/high/node.hpp>

#include "render_system_impl/render_system_batcher.hpp"

namespace
{
    using namespace e2d;

    struct index_u16 {
        using type = u16;
        static index_declaration decl() noexcept {
            return index_declaration::index_type::unsigned_short;
        }
    };

    struct vertex_v3f_t2f_c32b {
        struct type {
            v3f v;
            v2f t;
            color32 c;
        };
        static vertex_declaration decl() noexcept {
            return vertex_declaration()
                .add_attribute<v3f>("a_vertex")
                .add_attribute<v2f>("a_st")
                .add_attribute<color32>("a_tint").normalized();
        }
    };

    using batcher_type = render_system_impl::batcher<
        index_u16,
        vertex_v3f_t2f_c32b>;
}

namespace e2d
{
    class sprite_render_system::internal_state {
    public:
        internal_state()
        : batcher_(the<debug>(), the<render>()) {}

        batcher_type& batcher() noexcept {
            return batcher_;
        }
    private:
        batcher_type batcher_;
    };
}

namespace
{
    const str_hash matrix_v_property_hash = "u_matrix_v";
    const str_hash matrix_p_property_hash = "u_matrix_p";
    const str_hash matrix_vp_property_hash = "u_matrix_vp";
    const str_hash game_time_property_hash = "u_game_time";
    const str_hash sprite_texture_sampler_hash = "u_texture";

    void draw_sprite(
        batcher_type& batcher,
        const sprite_renderer& spr_r,
        const renderer& node_r,
        const actor& actor,
        render::property_block& property_cache)
    {
        if ( !actor.node() || !node_r.enabled() ) {
            return;
        }

        if ( !spr_r.sprite() || node_r.materials().empty() ) {
            return;
        }

        const sprite& spr = spr_r.sprite()->content();
        const texture_asset::ptr& tex_a = spr.texture();
        const material_asset::ptr& mat_a = node_r.materials().front();

        if ( !tex_a || !tex_a->content() || !mat_a ) {
            return;
        }

        const b2f& tex_r = spr.texrect();
        const v2f& tex_s = tex_a->content()->size().cast_to<f32>();

        const f32 sw = tex_r.size.x;
        const f32 sh = tex_r.size.y;

        const f32 px = tex_r.position.x - spr.pivot().x;
        const f32 py = tex_r.position.y - spr.pivot().y;

        const v4f p1{px + 0.f, py + 0.f, 0.f, 1.f};
        const v4f p2{px + sw,  py + 0.f, 0.f, 1.f};
        const v4f p3{px + sw,  py + sh,  0.f, 1.f};
        const v4f p4{px + 0.f, py + sh,  0.f, 1.f};

        const f32 tx = tex_r.position.x / tex_s.x;
        const f32 ty = tex_r.position.y / tex_s.y;
        const f32 tw = tex_r.size.x / tex_s.x;
        const f32 th = tex_r.size.y / tex_s.y;

        const m4f& sm = actor.node()->world_matrix();
        const color32& tc = spr_r.tint();

        const batcher_type::index_type indices[] = {
            0u, 1u, 2u, 2u, 3u, 0u};

        const batcher_type::vertex_type vertices[] = {
            { v3f(p1 * sm), {tx + 0.f, ty + 0.f}, tc },
            { v3f(p2 * sm), {tx + tw,  ty + 0.f}, tc },
            { v3f(p3 * sm), {tx + tw,  ty + th }, tc },
            { v3f(p4 * sm), {tx + 0.f, ty + th }, tc }};

        const render::sampler_min_filter min_filter = spr_r.filtering()
            ? render::sampler_min_filter::linear
            : render::sampler_min_filter::nearest;

        const render::sampler_mag_filter mag_filter = spr_r.filtering()
            ? render::sampler_mag_filter::linear
            : render::sampler_mag_filter::nearest;

        try {
            property_cache
                .sampler(sprite_texture_sampler_hash, render::sampler_state()
                    .texture(tex_a->content())
                    .min_filter(min_filter)
                    .mag_filter(mag_filter))
                .merge(node_r.properties());

            batcher.batch(
                mat_a,
                property_cache,
                indices, std::size(indices),
                vertices, std::size(vertices));
        } catch (...) {
            property_cache.clear();
            throw;
        }
        property_cache.clear();
    }
    
    void for_all_components(
        batcher_type& batcher,
        ecs::registry& owner,
        const ecs::const_entity& cam_e,
        const camera& cam)
    {
        const actor* const cam_a = cam_e.find_component<actor>();
        const const_node_iptr cam_n = cam_a ? cam_a->node() : nullptr;
        
        const m4f& cam_w = cam_n
            ? cam_n->world_matrix()
            : m4f::identity();
        const std::pair<m4f,bool> cam_w_inv = math::inversed(cam_w);

        const m4f& m_v = cam_w_inv.second
            ? cam_w_inv.first
            : m4f::identity();
        const m4f& m_p = cam.projection();

        render::property_block property_cache;

        batcher.flush()
            .property(matrix_v_property_hash, m_v)
            .property(matrix_p_property_hash, m_p)
            .property(matrix_vp_property_hash, m_v * m_p)
            .property(game_time_property_hash, the<engine>().time());

        owner.for_joined_components<sprite_renderer, renderer, actor>([&batcher, &property_cache](
            const ecs::const_entity&,
            const sprite_renderer& spr_r,
            const renderer& node_r,
            const actor& actor)
        {
            draw_sprite(batcher, spr_r, node_r, actor, property_cache);
        });
    }
}

namespace e2d
{
    sprite_render_system::sprite_render_system()
    : state_(new internal_state()) {}

    sprite_render_system::~sprite_render_system() noexcept = default;

    void sprite_render_system::process(ecs::registry& owner, ecs::entity_id cam_e_id) {
        ecs::const_entity cam_e(owner, cam_e_id);
        if ( cam_e.valid() ) {
            const camera* cam = owner.find_component<camera>(cam_e);
            if ( cam ) {
                for_all_components(state_->batcher(), owner, cam_e, *cam);
                state_->batcher().flush();
                state_->batcher().clear(true);
            }
        }
    }
}
