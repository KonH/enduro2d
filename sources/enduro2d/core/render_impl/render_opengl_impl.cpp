/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018 Matvey Cherevko
 ******************************************************************************/

#include "render_opengl_impl.hpp"

#if defined(E2D_RENDER_MODE) && E2D_RENDER_MODE == E2D_RENDER_MODE_OPENGL

namespace
{
    using namespace e2d;
    using namespace e2d::opengl;
}

namespace e2d
{
    //
    // shader::internal_state
    //

    shader::internal_state::internal_state(
        debug& debug,
        gl_program_id id)
    : debug_(debug)
    , id_(std::move(id)){
        E2D_ASSERT(!id_.empty());

        vector<uniform_info> uniforms;
        grab_program_uniforms(debug_, *id_, uniforms);

        vector<attribute_info> attributes;
        grab_program_attributes(debug_, *id_, attributes);

        debug.trace("RENDER: Program info:\n"
            "--> active uniforms: %0\n"
            "--> active attributes: %1",
            uniforms.size(),
            attributes.size());

        for ( const auto& info : uniforms ) {
            debug.trace(
                "uniform: size: %0, type: %1, location: %2, name: %3",
                info.size,
                uniform_type_to_cstr(info.type),
                info.location,
                info.name.hash());
            uniforms_.emplace(info.name, info);
        }

        for ( const auto& info : attributes ) {
            debug.trace(
                "attribute: size: %0, type: %1, location: %2, name: %3",
                info.size,
                attribute_type_to_cstr(info.type),
                info.location,
                info.name.hash());
            attributes_.emplace(info.name, info);
        }
    }

    debug& shader::internal_state::dbg() const noexcept {
        return debug_;
    }

    const gl_program_id& shader::internal_state::id() const noexcept {
        return id_;
    }

    //
    // texture::internal_state
    //

    texture::internal_state::internal_state(debug& debug, gl_texture_id id)
    : debug_(debug)
    , id_(std::move(id)) {
        E2D_ASSERT(!id_.empty());
    }

    debug& texture::internal_state::dbg() const noexcept {
        return debug_;
    }

    const gl_texture_id& texture::internal_state::id() const noexcept {
        return id_;
    }

    //
    // index_buffer::internal_state
    //

    index_buffer::internal_state::internal_state(
        debug& debug,
        gl_buffer_id id,
        std::size_t size,
        const index_declaration& decl)
    : debug_(debug)
    , id_(std::move(id))
    , size_(size)
    , decl_(decl) {
        E2D_ASSERT(!id_.empty());
    }

    debug& index_buffer::internal_state::dbg() const noexcept {
        return debug_;
    }

    const gl_buffer_id& index_buffer::internal_state::id() const noexcept {
        return id_;
    }

    std::size_t index_buffer::internal_state::size() const noexcept {
        return size_;
    }

    const index_declaration& index_buffer::internal_state::decl() const noexcept {
        return decl_;
    }

    //
    // vertex_buffer::internal_state
    //

    vertex_buffer::internal_state::internal_state(
        debug& debug,
        gl_buffer_id id,
        std::size_t size,
        const vertex_declaration& decl)
    : debug_(debug)
    , id_(std::move(id))
    , size_(size)
    , decl_(decl) {
        E2D_ASSERT(!id_.empty());
    }

    debug& vertex_buffer::internal_state::dbg() const noexcept {
        return debug_;
    }

    const gl_buffer_id& vertex_buffer::internal_state::id() const noexcept {
        return id_;
    }

    std::size_t vertex_buffer::internal_state::size() const noexcept {
        return size_;
    }

    const vertex_declaration& vertex_buffer::internal_state::decl() const noexcept {
        return decl_;
    }

    //
    // render::internal_state
    //

    render::internal_state::internal_state(debug& debug, window& window)
    : debug_(debug)
    , window_(window) {}

    debug& render::internal_state::dbg() const noexcept {
        return debug_;
    }

    window& render::internal_state::wnd() const noexcept {
        return window_;
    }

    render::internal_state& render::internal_state::set_states(const state_block& rs) noexcept {
        set_depth_state(rs.depth());
        set_stencil_state(rs.stencil());
        set_culling_state(rs.culling());
        set_blending_state(rs.blending());
        set_capabilities_state(rs.capabilities());
        return *this;
    }

    render::internal_state& render::internal_state::set_depth_state(const depth_state& ds) noexcept {
        GL_CHECK_CODE(debug_, glDepthRange(
            math::numeric_cast<GLclampd>(math::saturate(ds.near_)),
            math::numeric_cast<GLclampd>(math::saturate(ds.far_))));
        GL_CHECK_CODE(debug_, glDepthMask(
            ds.write_ ? GL_TRUE : GL_FALSE));
        GL_CHECK_CODE(debug_, glDepthFunc(
            convert_compare_func(ds.func_)));
        return *this;
    }

    render::internal_state& render::internal_state::set_stencil_state(const stencil_state& ss) noexcept {
        GL_CHECK_CODE(debug_, glStencilMask(
            math::numeric_cast<GLuint>(ss.write_)));
        GL_CHECK_CODE(debug_, glStencilFunc(
            convert_compare_func(ss.func_),
            math::numeric_cast<GLint>(ss.ref_),
            math::numeric_cast<GLuint>(ss.read_)));
        GL_CHECK_CODE(debug_, glStencilOp(
            convert_stencil_op(ss.sfail_),
            convert_stencil_op(ss.zfail_),
            convert_stencil_op(ss.pass_)));
        return *this;
    }

    render::internal_state& render::internal_state::set_culling_state(const culling_state& cs) noexcept {
        GL_CHECK_CODE(debug_, glFrontFace(
            convert_culling_mode(cs.mode_)));
        GL_CHECK_CODE(debug_, glCullFace(
            convert_culling_face(cs.face_)));
        return *this;
    }

    render::internal_state& render::internal_state::set_blending_state(const blending_state& bs) noexcept {
        GL_CHECK_CODE(debug_, glBlendColor(
            math::numeric_cast<GLclampf>(math::saturate(bs.constant_color_.r)),
            math::numeric_cast<GLclampf>(math::saturate(bs.constant_color_.g)),
            math::numeric_cast<GLclampf>(math::saturate(bs.constant_color_.b)),
            math::numeric_cast<GLclampf>(math::saturate(bs.constant_color_.a))));
        GL_CHECK_CODE(debug_, glBlendFuncSeparate(
            convert_blending_factor(bs.src_rgb_factor_),
            convert_blending_factor(bs.dst_rgb_factor_),
            convert_blending_factor(bs.src_alpha_factor_),
            convert_blending_factor(bs.dst_alpha_factor_)));
        GL_CHECK_CODE(debug_, glBlendEquationSeparate(
            convert_blending_equation(bs.rgb_equation_),
            convert_blending_equation(bs.alpha_equation_)));
        GL_CHECK_CODE(debug_, glColorMask(
            (math::enum_to_number(bs.color_mask_) & math::enum_to_number(blending_color_mask::r)) != 0,
            (math::enum_to_number(bs.color_mask_) & math::enum_to_number(blending_color_mask::g)) != 0,
            (math::enum_to_number(bs.color_mask_) & math::enum_to_number(blending_color_mask::b)) != 0,
            (math::enum_to_number(bs.color_mask_) & math::enum_to_number(blending_color_mask::a)) != 0));
        return *this;
    }

    render::internal_state& render::internal_state::set_capabilities_state(const capabilities_state& cs) noexcept {
        const auto enable_or_disable = [](GLenum cap, bool enable) noexcept {
            if ( enable ) {
                glEnable(cap);
            } else {
                glDisable(cap);
            }
        };
        GL_CHECK_CODE(debug_, enable_or_disable(GL_CULL_FACE, cs.culling_));
        GL_CHECK_CODE(debug_, enable_or_disable(GL_BLEND, cs.blending_));
        GL_CHECK_CODE(debug_, enable_or_disable(GL_DEPTH_TEST, cs.depth_test_));
        GL_CHECK_CODE(debug_, enable_or_disable(GL_STENCIL_TEST, cs.stencil_test_));
        return *this;
    }
}
#endif
