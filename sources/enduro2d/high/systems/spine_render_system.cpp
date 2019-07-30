/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include <enduro2d/core/render.hpp>
#include <enduro2d/core/engine.hpp>

#include <enduro2d/high/systems/spine_render_system.hpp>
#include <enduro2d/high/components/spine_renderer.hpp>

#include <enduro2d/high/components/actor.hpp>
#include <enduro2d/high/components/camera.hpp>
#include <enduro2d/high/components/spine_renderer.hpp>
#include <enduro2d/high/components/renderer.hpp>

#include <enduro2d/high/assets/texture_asset.hpp>

#include <enduro2d/high/node.hpp>

#include "render_system_impl/render_system_batcher.hpp"

#include <spine/Skeleton.h>
#include <spine/VertexEffect.h>
#include <spine/SkeletonClipping.h>
#include <spine/RegionAttachment.h>
#include <spine/MeshAttachment.h>

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
    using batcher_type = render_system_impl::batcher<
        index_u16,
        vertex_v3f_t2f_c32b>;

    class spine_render_system::internal_state {
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
    const str_hash spine_texture_sampler_hash = "u_texture";

    void draw_spine(
        batcher_type& batcher,
        const spine_renderer& spine_r,
        const renderer& node_r,
        const actor& actor,
        render::property_block& property_cache)
    {
        static std::vector<batcher_type::vertex_type> temp_vertices;

        static_assert(sizeof(batcher_type::vertex_type) % sizeof(float) == 0, "invalid stride");
        constexpr int stride = sizeof(batcher_type::vertex_type) / sizeof(float);

        if ( !actor.node() || !node_r.enabled() ) {
            return;
        }

        if ( node_r.materials().empty() ) {
            return;
        }

        spSkeleton* skeleton = spine_r.skeleton().operator->();
        spSkeletonClipping* clipper = spine_r.clipper().operator->();
        spVertexEffect* effect = spine_r.effect().operator->();
        const material_asset::ptr& mat_a = node_r.materials().front();

        if ( !skeleton || !clipper || !mat_a ) {
            return;
        }

        const u16 quad_indices[6] = { 0, 1, 2, 2, 3, 0 };

        if ( skeleton->color.a == 0 ) {
            return;
        }

        if ( effect ) {
            effect->begin(effect, skeleton);
        }
        
        const m3f sm = m3f(v3f(actor.node()->world_matrix()[0]),
                           v3f(actor.node()->world_matrix()[1]),
                           v3f(actor.node()->world_matrix()[3]));

        for ( int i = 0; i < skeleton->slotsCount; ++i ) {
            spSlot* slot = skeleton->drawOrder[i];
            spAttachment* attachment = slot->attachment;
            if ( !attachment ) {
                continue;
            }
            if ( slot->color.a == 0 ) {
                spSkeletonClipping_clipEnd(clipper, slot);
                continue;
            }

            size_t vertex_count = 0;
            const float* uvs = nullptr;
            const u16* indices = nullptr;
            int index_count = 0;
            const spColor* attachment_color;
            texture_ptr texture;

            if ( attachment->type == SP_ATTACHMENT_REGION ) {
                spRegionAttachment* region = reinterpret_cast<spRegionAttachment*>(attachment);
                attachment_color = &region->color;

                if ( attachment_color->a == 0 ) {
                    spSkeletonClipping_clipEnd(clipper, slot);
                    continue;
                }
                if ( 4 > temp_vertices.size() ) {
                    temp_vertices.resize(4);
                }
                spRegionAttachment_computeWorldVertices(region, slot->bone, &temp_vertices.data()->v.x, 0, stride);
                vertex_count = 4;
                uvs = region->uvs;
                indices = quad_indices;
                index_count = 6;
                if ( texture_asset* asset = static_cast<texture_asset*>(static_cast<spAtlasRegion*>(region->rendererObject)->page->rendererObject) ) {
                    texture = asset->content();
                }
            } else if ( attachment->type == SP_ATTACHMENT_MESH ) {
                spMeshAttachment* mesh = reinterpret_cast<spMeshAttachment*>(attachment);
                attachment_color = &mesh->color;

                if ( attachment_color->a == 0 ) {
                    spSkeletonClipping_clipEnd(clipper, slot);
                    continue;
                }
                vertex_count = mesh->super.worldVerticesLength >> 1;
                if ( vertex_count > temp_vertices.size() ) {
                    temp_vertices.resize(vertex_count);
                }
                spVertexAttachment_computeWorldVertices(&mesh->super, slot, 0, mesh->super.worldVerticesLength, &temp_vertices.data()->v.x, 0, stride);
                uvs = mesh->uvs;
                indices = mesh->triangles;
                index_count = mesh->trianglesCount;
                if ( texture_asset* asset = static_cast<texture_asset*>(static_cast<spAtlasRegion*>(mesh->rendererObject)->page->rendererObject) ) {
                    texture = asset->content();
                }
            } else if ( attachment->type == SP_ATTACHMENT_CLIPPING ) {
                spClippingAttachment* clip = reinterpret_cast<spClippingAttachment*>(attachment);
                spSkeletonClipping_clipStart(clipper, slot, clip);
                E2D_ASSERT(false);
                continue;
            } else {
                continue;
            }

            const color32 vert_color(
                color(skeleton->color.r, skeleton->color.g, skeleton->color.b, skeleton->color.a) *
                color(slot->color.r, slot->color.g, slot->color.b, slot->color.a) *
                color(attachment_color->r, attachment_color->g, attachment_color->b, attachment_color->a));

            switch ( slot->data->blendMode ) {
                case SP_BLEND_MODE_NORMAL :
                    break;
                case SP_BLEND_MODE_ADDITIVE :
                    break;
                case SP_BLEND_MODE_MULTIPLY :
                    break;
                case SP_BLEND_MODE_SCREEN :
                    break;
                default :
                    break;
            }

            /*const float* vertices = temp_vertices.data();
            if ( spSkeletonClipping_isClipping(clipper) ) {
                spSkeletonClipping_clipTriangles(clipper, vertices, vertex_count, indices, index_count, uvs, 2);
                vertices = clipper->clippedVertices->items;
                vertex_count = clipper->clippedVertices->size >> 1;
                uvs = clipper->clippedUVs->items;
                indices = clipper->clippedTriangles->items;
                index_count = clipper->clippedTriangles->size;
            }*/

            if ( effect ) {
                E2D_ASSERT(false);
            } else {
                for ( int j = 0; j < vertex_count; ++j ) {
                    auto& vert = temp_vertices[j];
                    v2f v(vert.v.x, vert.v.y);
                    vert.v = v3f(v.x, v.y, 1.0f) * sm;
                    vert.t = v2f(uvs[j*2], uvs[j*2+1]);
                    vert.c = vert_color;
                }
            }
            
            try {
                property_cache
                    .sampler(spine_texture_sampler_hash, render::sampler_state()
                        .texture(texture)
                        .min_filter(render::sampler_min_filter::linear)
                        .mag_filter(render::sampler_mag_filter::linear))
                    .merge(node_r.properties());

                batcher.batch(
                    mat_a,
                    property_cache,
                    indices, index_count,
                    temp_vertices.data(), vertex_count);
            } catch (...) {
                property_cache.clear();
                throw;
            }

            spSkeletonClipping_clipEnd(clipper, slot);
        }

        spSkeletonClipping_clipEnd2(clipper);

        if ( effect ) {
            effect->end(effect);
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

        owner.for_joined_components<spine_renderer, renderer, actor>([&batcher, &property_cache](
            const ecs::const_entity&,
            const spine_renderer& spine_r,
            const renderer& node_r,
            const actor& actor)
        {
            draw_spine(batcher, spine_r, node_r, actor, property_cache);
        });
    }

    void for_all_cameras(
        batcher_type& batcher,
        ecs::registry& owner)
    {
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
                for_all_components(batcher, owner, p.first, p.second);
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
    spine_render_system::spine_render_system()
    : state_(new internal_state()) {}

    spine_render_system::~spine_render_system() noexcept = default;

    void spine_render_system::process(ecs::registry& owner) {
        for_all_cameras(state_->batcher(), owner);
        state_->batcher().flush();
        state_->batcher().clear(true);
    }
}
