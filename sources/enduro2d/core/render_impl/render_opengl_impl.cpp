/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "render_opengl_impl.hpp"

#if defined(E2D_RENDER_MODE)
#if E2D_RENDER_MODE == E2D_RENDER_MODE_OPENGL || E2D_RENDER_MODE == E2D_RENDER_MODE_OPENGLES

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

        for ( const auto& info : uniforms ) {
            uniforms_.emplace(info.name, info);
        }

        for ( const auto& info : attributes ) {
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

    texture::internal_state::internal_state(
        debug& debug,
        gl_texture_id id,
        const v2u& size,
        const pixel_declaration& decl)
    : debug_(debug)
    , id_(std::move(id))
    , size_(size)
    , decl_(decl){
        E2D_ASSERT(!id_.empty());
    }

    debug& texture::internal_state::dbg() const noexcept {
        return debug_;
    }

    const gl_texture_id& texture::internal_state::id() const noexcept {
        return id_;
    }

    const v2u& texture::internal_state::size() const noexcept {
        return size_;
    }

    const pixel_declaration& texture::internal_state::decl() const noexcept {
        return decl_;
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
    // render_target::internal_state
    //
    render_target::internal_state::internal_state(
        debug& debug,
        opengl::gl_framebuffer_id id,
        const v2u& size,
        texture_ptr color,
        texture_ptr depth,
        opengl::gl_renderbuffer_id color_rb,
        opengl::gl_renderbuffer_id depth_rb)
    : debug_(debug)
    , id_(std::move(id))
    , size_(size)
    , color_(std::move(color))
    , depth_(std::move(depth))
    , color_rb_(std::move(color_rb))
    , depth_rb_(std::move(depth_rb)){
        E2D_ASSERT(!id_.empty());
    }

    debug& render_target::internal_state::dbg() const noexcept {
        return debug_;
    }

    const gl_framebuffer_id& render_target::internal_state::id() const noexcept {
        return id_;
    }

    const v2u& render_target::internal_state::size() const noexcept {
        return size_;
    }

    const texture_ptr& render_target::internal_state::color() const noexcept {
        return color_;
    }

    const texture_ptr& render_target::internal_state::depth() const noexcept {
        return depth_;
    }

    const gl_renderbuffer_id& render_target::internal_state::color_rb() const noexcept {
        return color_rb_;
    }

    const gl_renderbuffer_id& render_target::internal_state::depth_rb() const noexcept {
        return depth_rb_;
    }

    //
    // render::internal_state
    //

    render::internal_state::internal_state(debug& debug, window& window)
    : debug_(debug)
    , window_(window)
    , default_sp_(gl_program_id::current(debug))
    , default_fb_(gl_framebuffer_id::current(debug, GL_FRAMEBUFFER))
    {
        if ( glewInit() != GLEW_OK ) {
            throw bad_render_operation();
        }

        gl_trace_info(debug_);
        gl_trace_limits(debug_);
        gl_fill_device_caps(debug_, device_caps_);

        GL_CHECK_CODE(debug_, glPixelStorei(GL_PACK_ALIGNMENT, 1));
        GL_CHECK_CODE(debug_, glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

        reset_states();
        reset_shader_program();
        reset_render_target();
    }

    debug& render::internal_state::dbg() const noexcept {
        return debug_;
    }

    window& render::internal_state::wnd() const noexcept {
        return window_;
    }

    const render::device_caps& render::internal_state::device_capabilities() const noexcept {
        return device_caps_;
    }

    const render_target_ptr& render::internal_state::render_target() const noexcept {
        return render_target_;
    }
    
    const opengl::gl_framebuffer_id& render::internal_state::default_fb() const noexcept {
        return default_fb_;
    }
    
    render::internal_state& render::internal_state::reset_states() noexcept {
        set_depth_state_(state_block_.depth());
        set_stencil_state_(state_block_.stencil());
        set_culling_state_(state_block_.culling());
        set_blending_state_(state_block_.blending());
        set_capabilities_state_(state_block_.capabilities());
        return *this;
    }

    render::internal_state& render::internal_state::set_states(const state_block& sb) noexcept {
        if ( sb.depth() != state_block_.depth() ) {
            set_depth_state_(sb.depth());
            state_block_.depth(sb.depth());
        }

        if ( sb.stencil() != state_block_.stencil() ) {
            set_stencil_state_(sb.stencil());
            state_block_.stencil(sb.stencil());
        }

        if ( sb.culling() != state_block_.culling() ) {
            set_culling_state_(sb.culling());
            state_block_.culling(sb.culling());
        }

        if ( sb.blending() != state_block_.blending() ) {
            set_blending_state_(sb.blending());
            state_block_.blending(sb.blending());
        }

        if ( sb.capabilities() != state_block_.capabilities() ) {
            set_capabilities_state_(sb.capabilities());
            state_block_.capabilities(sb.capabilities());
        }

        return *this;
    }

    render::internal_state& render::internal_state::reset_shader_program() noexcept {
        const gl_program_id& sp_id = shader_program_
            ? shader_program_->state().id()
            : default_sp_;
        GL_CHECK_CODE(debug_, glUseProgram(*sp_id));
        return *this;
    }

    render::internal_state& render::internal_state::set_shader_program(const shader_ptr& sp) noexcept {
        if ( sp == shader_program_ ) {
            return *this;
        }

        const gl_program_id& sp_id = sp
            ? sp->state().id()
            : default_sp_;
        GL_CHECK_CODE(debug_, glUseProgram(*sp_id));

        shader_program_ = sp;
        return *this;
    }

    render::internal_state& render::internal_state::reset_render_target() noexcept {
        const gl_framebuffer_id& rt_id = render_target_
            ? render_target_->state().id()
            : default_fb_;
        GL_CHECK_CODE(debug_, glBindFramebuffer(rt_id.target(), *rt_id));
        return *this;
    }
    
    render::internal_state& render::internal_state::set_render_target(const render_target_ptr& rt) noexcept {
        if ( rt == render_target_ ) {
            return *this;
        }

        const gl_framebuffer_id& rt_id = rt
            ? rt->state().id()
            : default_fb_;
        GL_CHECK_CODE(debug_, glBindFramebuffer(rt_id.target(), *rt_id));

        render_target_ = rt;
        return *this;
    }

    void render::internal_state::set_depth_state_(const depth_state& ds) noexcept {
        GL_CHECK_CODE(debug_, glDepthRange(
            math::numeric_cast<GLclampd>(math::saturate(ds.range_near())),
            math::numeric_cast<GLclampd>(math::saturate(ds.range_far()))));
        GL_CHECK_CODE(debug_, glDepthMask(
            ds.write() ? GL_TRUE : GL_FALSE));
        GL_CHECK_CODE(debug_, glDepthFunc(
            convert_compare_func(ds.func())));
    }

    void render::internal_state::set_stencil_state_(const stencil_state& ss) noexcept {
        GL_CHECK_CODE(debug_, glStencilMask(
            math::numeric_cast<GLuint>(ss.write())));
        GL_CHECK_CODE(debug_, glStencilFunc(
            convert_compare_func(ss.func()),
            math::numeric_cast<GLint>(ss.ref()),
            math::numeric_cast<GLuint>(ss.mask())));
        GL_CHECK_CODE(debug_, glStencilOp(
            convert_stencil_op(ss.sfail()),
            convert_stencil_op(ss.zfail()),
            convert_stencil_op(ss.pass())));
    }

    void render::internal_state::set_culling_state_(const culling_state& cs) noexcept {
        GL_CHECK_CODE(debug_, glFrontFace(
            convert_culling_mode(cs.mode())));
        GL_CHECK_CODE(debug_, glCullFace(
            convert_culling_face(cs.face())));
    }

    void render::internal_state::set_blending_state_(const blending_state& bs) noexcept {
        GL_CHECK_CODE(debug_, glBlendColor(
            math::numeric_cast<GLclampf>(math::saturate(bs.constant_color().r)),
            math::numeric_cast<GLclampf>(math::saturate(bs.constant_color().g)),
            math::numeric_cast<GLclampf>(math::saturate(bs.constant_color().b)),
            math::numeric_cast<GLclampf>(math::saturate(bs.constant_color().a))));
        GL_CHECK_CODE(debug_, glBlendFuncSeparate(
            convert_blending_factor(bs.src_rgb_factor()),
            convert_blending_factor(bs.dst_rgb_factor()),
            convert_blending_factor(bs.src_alpha_factor()),
            convert_blending_factor(bs.dst_alpha_factor())));
        GL_CHECK_CODE(debug_, glBlendEquationSeparate(
            convert_blending_equation(bs.rgb_equation()),
            convert_blending_equation(bs.alpha_equation())));
        GL_CHECK_CODE(debug_, glColorMask(
            (utils::enum_to_underlying(bs.color_mask()) & utils::enum_to_underlying(blending_color_mask::r)) != 0,
            (utils::enum_to_underlying(bs.color_mask()) & utils::enum_to_underlying(blending_color_mask::g)) != 0,
            (utils::enum_to_underlying(bs.color_mask()) & utils::enum_to_underlying(blending_color_mask::b)) != 0,
            (utils::enum_to_underlying(bs.color_mask()) & utils::enum_to_underlying(blending_color_mask::a)) != 0));
    }

    void render::internal_state::set_capabilities_state_(const capabilities_state& cs) noexcept {
        const auto enable_or_disable = [](GLenum cap, bool enable) noexcept {
            if ( enable ) {
                glEnable(cap);
            } else {
                glDisable(cap);
            }
        };

        GL_CHECK_CODE(debug_, enable_or_disable(GL_CULL_FACE, cs.culling()));
        GL_CHECK_CODE(debug_, enable_or_disable(GL_BLEND, cs.blending()));
        GL_CHECK_CODE(debug_, enable_or_disable(GL_DEPTH_TEST, cs.depth_test()));
        GL_CHECK_CODE(debug_, enable_or_disable(GL_STENCIL_TEST, cs.stencil_test()));
    }
}

#endif
#endif
