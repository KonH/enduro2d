/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#pragma once

#include "_utils.hpp"
#include "strings.hpp"

namespace e2d
{
    class shader_source final {
    public:
        enum class value_type : u8 {
            f32,
            v2f,
            v3f,
            v4f,
            m2f,
            m3f,
            m4f,
            unknown
        };

        enum class sampler_type : u8 {
            _2d,
            cube_map,
            unknown
        };

        enum class scope_type : u8 {
            render_pass,
            material,
            draw_command,
            unknown
        };

        struct uniform {
            str name;
            u16 offset = 0;
            //u16 array_size = 0; // not supported
            value_type type = value_type::unknown;
            scope_type scope = scope_type::unknown;
        };

        struct sampler {
            str name;
            u8 unit = 0;
            sampler_type type = sampler_type::unknown;
            scope_type scope = scope_type::unknown;
        };

        struct attribute {
            str name;
            u8 index = 0;
            value_type type = value_type::unknown;
        };
    public:
        shader_source() = default;

        shader_source(shader_source&& other) noexcept;
        shader_source& operator=(shader_source&& other) noexcept;

        shader_source(const shader_source& other);
        shader_source& operator=(const shader_source& other);

        shader_source& assign(shader_source&& other) noexcept;
        shader_source& assign(const shader_source& other);

        void swap(shader_source& other) noexcept;
        void clear() noexcept;
        bool empty() const noexcept;

        shader_source& vertex_shader(str source);
        shader_source& fragment_shader(str source);
        shader_source& add_uniform(str name, size_t offset, value_type type, scope_type scope);
        shader_source& add_sampler(str name, u32 unit, sampler_type type, scope_type scope);
        shader_source& add_attribute(str name, u32 index, value_type type);

        const str& vertex_shader() const noexcept;
        const str& fragment_shader() const noexcept;
        const vector<uniform>& uniforms() const noexcept;
        const vector<sampler>& samplers() const noexcept;
        const vector<attribute>& attributes() const noexcept;
    private:
        str vs_;
        str fs_;
        vector<uniform> uniforms_;
        vector<sampler> samplers_;
        vector<attribute> attributes_;
    };
}
