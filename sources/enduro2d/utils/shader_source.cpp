/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include <enduro2d/utils/shader_source.hpp>

namespace e2d
{
    shader_source::shader_source(shader_source&& other) noexcept {
        assign(std::move(other));
    }

    shader_source& shader_source::operator=(shader_source&& other) noexcept {
        return assign(std::move(other));
    }

    shader_source::shader_source(const shader_source& other) {
        assign(other);
    }

    shader_source& shader_source::operator=(const shader_source& other) {
        return assign(other);
    }

    shader_source& shader_source::assign(shader_source&& other) noexcept {
        if ( this != &other ) {
            swap(other);
            other.clear();
        }
        return *this;
    }

    shader_source& shader_source::assign(const shader_source& other) {
        if ( this != &other ) {
            shader_source s;
            s.vs_ = other.vs_;
            s.fs_ = other.fs_;
            s.uniforms_ = other.uniforms_;
            s.samplers_ = other.samplers_;
            s.attributes_ = other.attributes_;
            swap(s);
        }
        return *this;
    }

    void shader_source::swap(shader_source& other) noexcept {
        using std::swap;
        swap(vs_, other.vs_);
        swap(fs_, other.fs_);
        swap(uniforms_, other.uniforms_);
        swap(samplers_, other.samplers_);
        swap(attributes_, other.attributes_);
    }
    
    void shader_source::clear() noexcept {
        vs_.clear();
        fs_.clear();
        uniforms_.clear();
        samplers_.clear();
        attributes_.clear();
    }
    
    bool shader_source::empty() const noexcept {
        return vs_.empty() || fs_.empty();
    }
    
    shader_source& shader_source::vertex_shader(str source) {
        vs_ = std::move(source);
        return *this;
    }

    shader_source& shader_source::fragment_shader(str source) {
        fs_ = std::move(source);
        return *this;
    }

    shader_source& shader_source::add_uniform(
        str name,
        size_t offset,
        size_t size,
        value_type type,
        scope_typr scope)
    {
        uniforms_.push_back({
            std::move(name),
            math::numeric_cast<u16>(offset),
            math::numeric_cast<u16>(size),
            type,
            scope});
        return *this;
    }

    shader_source& shader_source::add_sampler(
        str name,
        u32 unit,
        sampler_type type,
        scope_typr scope)
    {
        samplers_.push_back({
            std::move(name),
            math::numeric_cast<u8>(unit),
            type,
            scope});
        return *this;
    }
    
    shader_source& shader_source::add_attribute(
        str name,
        u32 index,
        value_type type)
    {
        attributes_.push_back({
            std::move(name),
            math::numeric_cast<u8>(index),
            type});
        return *this;
    }
    
    const str& shader_source::vertex_shader() const noexcept {
        return vs_;
    }

    const str& shader_source::fragment_shader() const noexcept {
        return fs_;
    }

    const vector<shader_source::uniform>& shader_source::uniforms() const noexcept {
        return uniforms_;
    }

    const vector<shader_source::sampler>& shader_source::samplers() const noexcept {
        return samplers_;
    }

    const vector<shader_source::attribute>& shader_source::attributes() const noexcept {
        return attributes_;
    }
}
