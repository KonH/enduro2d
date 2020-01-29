/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "../core/_all.hpp"

#include <ecs.hpp/ecs.hpp>

#define SOL_ALL_SAFETIES_ON 1
#include <3rdparty/sol/sol.hpp>

namespace e2d
{
    namespace ecs
    {
        using namespace ecs_hpp;
    }
}

namespace e2d
{
    class atlas_asset;
    class binary_asset;
    class flipbook_asset;
    class font_asset;
    class image_asset;
    class json_asset;
    class material_asset;
    class mesh_asset;
    class model_asset;
    class prefab_asset;
    class script_asset;
    class shader_asset;
    class shape_asset;
    class sound_asset;
    class spine_asset;
    class sprite_asset;
    class text_asset;
    class texture_asset;
    class xml_asset;

    class actor;
    class behaviour;
    class camera;
    class rect_collider;
    class circle_collider;
    class polygon_collider;
    template < typename C >
    class commands;
    template < typename T >
    class disabled;
    template < typename E >
    class events;
    class flipbook_player;
    class label;
    class model_renderer;
    class named;
    class renderer;
    class rigid_body;
    class scene;
    class spine_player;
    class sprite_renderer;
    class touchable;

    class camera_system;
    class flipbook_system;
    class frame_system;
    class gizmos_system;
    class input_system;
    class label_system;
    class physics_system;
    class render_system;
    class script_system;
    class spine_system;
    class world_system;

    template < typename Asset, typename Content >
    class content_asset;
    class asset;

    class factory;
    class library;
    class asset_cache;
    class asset_group;
    class asset_dependencies;

    class atlas;
    class flipbook;
    class model;
    class node;
    class prefab;
    class script;
    class spine;
    class sprite;

    class editor;
    class inspector;
    class luasol;
    class physics;
    class starter;
    class world;

    class gobject;
    template < typename T >
    class gcomponent;
    template < typename T >
    class const_gcomponent;
}

namespace sol
{
    template < typename T >
    struct unique_usertype_traits<e2d::intrusive_ptr<T>> {
        using type = T;
        using actual_type = e2d::intrusive_ptr<T>;
        static const bool value = true;

        static bool is_null(const actual_type& ptr) {
            return !ptr;
        }

        static type* get(actual_type& ptr) {
            return ptr.get();
        }
    };
}

namespace e2d::strings
{
    template <>
    class format_arg<ecs::entity> {
        ecs::entity value_;
        u8 width_;
    public:
        template < typename U >
        explicit format_arg(U&& value, u8 width = 0) noexcept
        : value_(std::forward<U>(value)), width_(width) {}

        std::ptrdiff_t write(char* dst, size_t size) const {
            return math::numeric_cast<std::ptrdiff_t>(
                format(dst, size, "(%0,%1)",
                    make_format_arg(ecs::detail::entity_id_index(value_.id()), width_),
                    make_format_arg(ecs::detail::entity_id_version(value_.id()), width_)));
        }
    };

    template <>
    class format_arg<ecs::const_entity> {
        ecs::const_entity value_;
        u8 width_;
    public:
        template < typename U >
        explicit format_arg(U&& value, u8 width = 0) noexcept
        : value_(std::forward<U>(value)), width_(width) {}

        std::ptrdiff_t write(char* dst, size_t size) const {
            return math::numeric_cast<std::ptrdiff_t>(
                format(dst, size, "(%0,%1)",
                    make_format_arg(ecs::detail::entity_id_index(value_.id()), width_),
                    make_format_arg(ecs::detail::entity_id_version(value_.id()), width_)));
        }
    };
}
