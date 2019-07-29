/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include <enduro2d/core/render.hpp>
#include <enduro2d/core/engine.hpp>

#include <enduro2d/high/systems/model_render_system.hpp>

#include <enduro2d/high/components/actor.hpp>
#include <enduro2d/high/components/camera.hpp>
#include <enduro2d/high/components/model_renderer.hpp>
#include <enduro2d/high/components/renderer.hpp>

#include <enduro2d/high/node.hpp>

namespace
{
    using namespace e2d;

    const str_hash matrix_m_property_hash = "u_matrix_m";
    const str_hash matrix_v_property_hash = "u_matrix_v";
    const str_hash matrix_p_property_hash = "u_matrix_p";
    const str_hash matrix_vp_property_hash = "u_matrix_vp";
    const str_hash game_time_property_hash = "u_game_time";
    const str_hash sprite_texture_sampler_hash = "u_texture";

    void draw_mesh(
        render& the_render,
        const model_renderer& mdl_r,
        const renderer& node_r,
        const actor& actor,
        render::property_block& property_cache)
    {
        if ( !actor.node() || !node_r.enabled() ) {
            return;
        }

        if ( !mdl_r.model() || !mdl_r.model()->content().mesh() ) {
            return;
        }
            
        const model& mdl = mdl_r.model()->content();
        const mesh& msh = mdl.mesh()->content();
            
        try {
            property_cache
                .property(matrix_m_property_hash, actor.node()->world_matrix())
                .merge(node_r.properties());

            const std::size_t submesh_count = math::min(
                msh.indices_submesh_count(),
                node_r.materials().size());

            for ( std::size_t i = 0, first_index = 0; i < submesh_count; ++i ) {
                const std::size_t index_count = msh.indices(i).size();
                const material_asset::ptr& mat = node_r.materials()[i];
                if ( mat ) {
                    the_render.execute(render::draw_command(
                        mat->content(),
                        mdl.geometry(),
                        property_cache
                    ).index_range(first_index, index_count));
                }
                first_index += index_count;
            }
        } catch (...) {
            property_cache.clear();
            throw;
        }
        property_cache.clear();
    }
    
    void for_all_components(
        ecs::registry& owner,
        const ecs::const_entity& cam_e,
        const camera& cam)
    {
        render& the_render = the<render>();

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
        property_cache
            .property(matrix_v_property_hash, m_v)
            .property(matrix_p_property_hash, m_p)
            .property(matrix_vp_property_hash, m_v * m_p)
            .property(game_time_property_hash, the<engine>().time());

        the_render.execute(render::command_block<3>()
            .add_command(render::target_command(cam.target()))
            .add_command(render::viewport_command(cam.viewport()))
            .add_command(render::clear_command()
                .color_value(cam.background())));

        owner.for_joined_components<model_renderer, renderer, actor>([&the_render, &property_cache](
            const ecs::const_entity&,
            const model_renderer& mdl_r,
            const renderer& node_r,
            const actor& actor)
        {
            draw_mesh(the_render, mdl_r, node_r, actor, property_cache);
        });
    }

    void for_all_cameras(ecs::registry& owner) {
        static vector<std::pair<ecs::const_entity,camera>> temp_components;
        try {
            temp_components.reserve(owner.component_count<camera>());
            owner.for_each_component<camera>([](const ecs::const_entity& e, const camera& cam){
                temp_components.emplace_back(e, cam);
            });
            std::sort(
                temp_components.begin(),
                temp_components.end(),
                [](const auto& l, const auto& r){
                    return l.second.depth() < r.second.depth();
                });
            for ( auto& p : temp_components ) {
                for_all_components(owner, p.first, p.second);
            }
        } catch (...) {
            temp_components.clear();
            throw;
        }
        temp_components.clear();
    }
}

namespace e2d
{
    model_render_system::model_render_system() = default;

    model_render_system::~model_render_system() noexcept = default;

    void model_render_system::process(ecs::registry& owner) {
        for_all_cameras(owner);
    }
}
