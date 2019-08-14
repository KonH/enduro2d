/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "render_system_drawer.hpp"

#include <enduro2d/high/components/renderer.hpp>
#include <enduro2d/high/components/model_renderer.hpp>
#include <enduro2d/high/components/sprite_renderer.hpp>

namespace
{
    using namespace e2d;

    const str_hash matrix_m_property_hash = "u_matrix_m";
    const str_hash sprite_texture_sampler_hash = "u_texture";
}

namespace e2d::render_system_impl
{
    //
    // drawer::context
    //

    drawer::context::context(
        const camera& cam,
        const const_node_iptr& cam_n,
        render& render)
    : render_(render)
    {
        render_.begin_pass(
            render::renderpass_desc()
                .target(cam.target())
                .color_clear(cam.background())
                .color_store()
                .depth_clear(1.0f)
                .depth_discard()
                .viewport(cam.viewport()),
            cam.constants());
    }

    drawer::context::~context() noexcept {
        the<batcher>().flush();
        render_.end_pass();
    }

    void drawer::context::draw(
        const const_node_iptr& node)
    {
        if ( !node || !node->owner() ) {
            return;
        }

        E2D_ASSERT(node->owner()->entity().valid());
        ecs::const_entity node_e = node->owner()->entity();
        const renderer* node_r = node_e.find_component<renderer>();

        if ( node_r && node_r->enabled() ) {
            const model_renderer* mdl_r = node_e.find_component<model_renderer>();
            if ( mdl_r ) {
                draw(node, *node_r, *mdl_r);
            }
            const sprite_renderer* spr_r = node_e.find_component<sprite_renderer>();
            if ( spr_r ) {
                draw(node, *node_r, *spr_r);
            }
        }
    }

    void drawer::context::draw(
        const const_node_iptr& node,
        const renderer& node_r,
        const model_renderer& mdl_r)
    {
        if ( !node || !node_r.enabled() ) {
            return;
        }

        if ( !mdl_r.model() || !mdl_r.model()->content().mesh() ) {
            return;
        }
            
        const model& mdl = mdl_r.model()->content();
        const mesh& msh = mdl.mesh()->content();
            
        E2D_ASSERT(msh.indices_submesh_count() == node_r.materials().size());
        const std::size_t submesh_count = math::min(
            msh.indices_submesh_count(),
            node_r.materials().size());

        // TODO: use temporary const_buffer
        if ( mdl_r.constants() ) {
            render_.update_buffer(
                mdl_r.constants(),
                render::property_map()
                    .assign(matrix_m_property_hash, node->world_matrix()));
        }

        render::bind_vertex_buffers_command vb_cmd;
        for ( std::size_t i = 0; i < mdl.vertices_count(); ++i ) {
            vb_cmd.add(mdl.vertices(i), mdl.attribute(i));
        }
        render_.execute(vb_cmd);

        for ( std::size_t i = 0, index_offset = 0; i < submesh_count; ++i ) {
            const u32 index_count = math::numeric_cast<u32>(msh.indices(i).size());
            const material_asset::ptr& mat = node_r.materials()[i];
            E2D_ASSERT(mat);
            if ( mat ) {
                render_.set_material(mat->content());
                render_.execute(render::draw_indexed_command()
                    .constants(mdl_r.constants())
                    .topo(mdl.topo())
                    .indices(mdl.indices())
                    .index_count(index_count)
                    .index_offset(index_offset));
            }
            index_offset += index_count * mdl.indices()->decl().bytes_per_index();
        }
    }

    void drawer::context::draw(
        const const_node_iptr& node,
        const renderer& node_r,
        const sprite_renderer& spr_r)
    {
        if ( !node || !node_r.enabled() ) {
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

        const m4f& sm = node->world_matrix();
        const color32& tc = spr_r.tint();

        const render::sampler_min_filter min_filter = spr_r.filtering()
            ? render::sampler_min_filter::linear
            : render::sampler_min_filter::nearest;

        const render::sampler_mag_filter mag_filter = spr_r.filtering()
            ? render::sampler_mag_filter::linear
            : render::sampler_mag_filter::nearest;

        auto batch = the<batcher>().alloc_batch<vertex_v3f_t2f_c32b>(4, 6,
            render::topology::triangles,
            render::material(mat_a->content())
                .sampler(sprite_texture_sampler_hash, render::sampler_state()
                    .texture(tex_a->content())
                    .min_filter(min_filter)
                    .mag_filter(mag_filter)));

        batch.vertices++ = { v3f(p1 * sm), {tx + 0.f, ty + 0.f}, tc };
        batch.vertices++ = { v3f(p2 * sm), {tx + tw,  ty + 0.f}, tc };
        batch.vertices++ = { v3f(p3 * sm), {tx + tw,  ty + th }, tc };
        batch.vertices++ = { v3f(p4 * sm), {tx + 0.f, ty + th }, tc };

        batch.indices++ = 0;  batch.indices++ = 1;  batch.indices++ = 2;
        batch.indices++ = 2;  batch.indices++ = 3;  batch.indices++ = 0;
    }

    void drawer::context::flush() {
        the<batcher>().flush();
    }

    //
    // drawer
    //

    drawer::drawer(render& r)
    : render_(r) {}
}
