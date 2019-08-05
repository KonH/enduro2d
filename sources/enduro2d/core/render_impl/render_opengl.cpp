/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "render.hpp"
#include "render_opengl_base.hpp"
#include "render_opengl_impl.hpp"

#if defined(E2D_RENDER_MODE)
#if E2D_RENDER_MODE == E2D_RENDER_MODE_OPENGL || E2D_RENDER_MODE == E2D_RENDER_MODE_OPENGLES

namespace
{
    using namespace e2d;

    const char* vertex_shader_header_cstr(render::api_profile profile) noexcept {
        switch ( profile ) {
        case e2d::render::api_profile::unknown:
            return "";
        case e2d::render::api_profile::opengles2:
        case e2d::render::api_profile::opengles3:
            return R"glsl(
                precision highp int;
                precision highp float;
            )glsl";
        case e2d::render::api_profile::opengl_compat:
            return R"glsl(
                #version 120
                #define highp
                #define mediump
                #define lowp
            )glsl";
        default:
            E2D_ASSERT_MSG(false, "unexpected render API profile");
            return "";
        }
    }

    const char* fragment_shader_header_cstr(render::api_profile profile) noexcept {
        switch ( profile ) {
        case e2d::render::api_profile::unknown:
            return "";
        case e2d::render::api_profile::opengles2:
        case e2d::render::api_profile::opengles3:
            return R"glsl(
                precision mediump int;
                precision mediump float;
            )glsl";
        case e2d::render::api_profile::opengl_compat:
            return R"glsl(
                #version 120
                #define highp
                #define mediump
                #define lowp
            )glsl";
        default:
            E2D_ASSERT_MSG(false, "unexpected render API profile");
            return "";
        }
    }
}

namespace
{
    using namespace e2d;
    using namespace e2d::opengl;

    class property_block_value_visitor final : private noncopyable {
    public:
        property_block_value_visitor(debug& debug, uniform_info ui) noexcept
        : debug_(debug)
        , ui_(std::move(ui)) {}

        void operator()(i32 v) const noexcept {
            if ( check_property_type(uniform_type::signed_integer) ) {
                GL_CHECK_CODE(debug_, glUniform1i(ui_.location, v));
            }
        }

        void operator()(f32 v) const noexcept {
            if ( check_property_type(uniform_type::floating_point) ) {
                GL_CHECK_CODE(debug_, glUniform1f(ui_.location, v));
            }
        }

        void operator()(const v2i& v) const noexcept {
            if ( check_property_type(uniform_type::v2i) ) {
                GL_CHECK_CODE(debug_, glUniform2iv(ui_.location, 1, v.data()));
            }
        }

        void operator()(const v3i& v) const noexcept {
            if ( check_property_type(uniform_type::v3i) ) {
                GL_CHECK_CODE(debug_, glUniform3iv(ui_.location, 1, v.data()));
            }
        }

        void operator()(const v4i& v) const noexcept {
            if ( check_property_type(uniform_type::v4i) ) {
                GL_CHECK_CODE(debug_, glUniform4iv(ui_.location, 1, v.data()));
            }
        }

        void operator()(const v2f& v) const noexcept {
            if ( check_property_type(uniform_type::v2f) ) {
                GL_CHECK_CODE(debug_, glUniform2fv(ui_.location, 1, v.data()));
            }
        }

        void operator()(const v3f& v) const noexcept {
            if ( check_property_type(uniform_type::v3f) ) {
                GL_CHECK_CODE(debug_, glUniform3fv(ui_.location, 1, v.data()));
            }
        }

        void operator()(const v4f& v) const noexcept {
            if ( check_property_type(uniform_type::v4f) ) {
                GL_CHECK_CODE(debug_, glUniform4fv(ui_.location, 1, v.data()));
            }
        }

        void operator()(const m2f& v) const noexcept {
            if ( check_property_type(uniform_type::m2f) ) {
                GL_CHECK_CODE(debug_, glUniformMatrix2fv(ui_.location, 1, GL_TRUE, v.data()));
            }
        }

        void operator()(const m3f& v) const noexcept {
            if ( check_property_type(uniform_type::m3f) ) {
                GL_CHECK_CODE(debug_, glUniformMatrix3fv(ui_.location, 1, GL_TRUE, v.data()));
            }
        }

        void operator()(const m4f& v) const noexcept {
            if ( check_property_type(uniform_type::m4f) ) {
                GL_CHECK_CODE(debug_, glUniformMatrix4fv(ui_.location, 1, GL_TRUE, v.data()));
            }
        }
    private:
        bool check_property_type(uniform_type type) const noexcept {
            if ( type == ui_.type ) {
                return true;
            }
            E2D_ASSERT_MSG(false, "unexpected property type");
            debug_.error("RENDER: unexpected property type:\n"
                "--> Type: %0\n"
                "--> Expected: %1",
                uniform_type_to_cstr(type),
                uniform_type_to_cstr(ui_.type));
            return false;
        }
    private:
        debug& debug_;
        uniform_info ui_;
    };

    void bind_property_block(
        debug& debug,
        const shader_ptr& ps,
        const render::property_block& pb) noexcept
    {
        E2D_ASSERT(ps && gl_program_id::current(debug) == ps->state().id());
        pb.foreach_by_properties([&debug, &ps](str_hash name, const render::property_value& value) noexcept {
            ps->state().with_uniform_location(name, [&debug, &value](const uniform_info& ui) noexcept {
                E2D_ASSERT(!value.valueless_by_exception());
                stdex::visit(property_block_value_visitor(debug, ui), value);
            });
        });
        GLint unit = 0;
        pb.foreach_by_samplers([&debug, &ps, &unit](str_hash name, const render::sampler_state& sampler) noexcept {
            ps->state().with_uniform_location(name, [&debug, &sampler, &unit](const uniform_info& ui) noexcept {
                GL_CHECK_CODE(debug, glActiveTexture(
                    math::numeric_cast<GLenum>(GL_TEXTURE0 + unit)));
                GL_CHECK_CODE(debug, glUniform1i(
                    ui.location, math::numeric_cast<GLint>(unit)));
                if ( sampler.texture() ) {
                    const gl_texture_id& texture_id = sampler.texture()->state().id();
                    GL_CHECK_CODE(debug, glBindTexture(
                        texture_id.target(), *texture_id));
                    GL_CHECK_CODE(debug, glTexParameteri(
                        texture_id.target(),
                        GL_TEXTURE_WRAP_S,
                        convert_sampler_wrap(sampler.s_wrap())));
                    GL_CHECK_CODE(debug, glTexParameteri(
                        texture_id.target(),
                        GL_TEXTURE_WRAP_T,
                        convert_sampler_wrap(sampler.t_wrap())));
                    GL_CHECK_CODE(debug, glTexParameteri(
                        texture_id.target(),
                        GL_TEXTURE_MIN_FILTER,
                        convert_sampler_filter(sampler.min_filter())));
                    GL_CHECK_CODE(debug, glTexParameteri(
                        texture_id.target(),
                        GL_TEXTURE_MAG_FILTER,
                        convert_sampler_filter(sampler.mag_filter())));
                } else {
                    GL_CHECK_CODE(debug, glBindTexture(GL_TEXTURE_2D, 0));
                    GL_CHECK_CODE(debug, glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
                }
                ++unit;
            });
        });
    }

    void unbind_property_block(
        debug& debug,
        const shader_ptr& ps,
        const render::property_block& pb) noexcept
    {
        E2D_ASSERT(ps && gl_program_id::current(debug) == ps->state().id());
        GLint unit = 0;
        pb.foreach_by_samplers([&debug, &ps, &unit](str_hash name, const render::sampler_state& sampler) noexcept {
            E2D_UNUSED(sampler);
            ps->state().with_uniform_location(name, [&debug, &unit](const uniform_info& ui) noexcept {
                E2D_UNUSED(ui);
                GL_CHECK_CODE(debug, glActiveTexture(
                    math::numeric_cast<GLenum>(GL_TEXTURE0 + unit)));
                GL_CHECK_CODE(debug, glBindTexture(GL_TEXTURE_2D, 0));
                GL_CHECK_CODE(debug, glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
                ++unit;
            });
        });
    }

    void bind_vertex_declaration(
        debug& debug,
        const shader_ptr& ps,
        const vertex_buffer_ptr& vb) noexcept
    {
        E2D_ASSERT(ps && vb);
        opengl::with_gl_bind_buffer(debug, vb->state().id(), [&debug, &ps, &vb]() noexcept {
            const vertex_declaration& decl = vb->decl();
            for ( std::size_t i = 0, e = decl.attribute_count(); i < e; ++i ) {
                const vertex_declaration::attribute_info& vai = decl.attribute(i);
                ps->state().with_attribute_location(vai.name, [&debug, &decl, &vai](const attribute_info& ai) noexcept {
                    const GLuint rows = math::numeric_cast<GLuint>(vai.rows);
                    for ( GLuint row = 0; row < rows; ++row ) {
                        GL_CHECK_CODE(debug, glEnableVertexAttribArray(
                            math::numeric_cast<GLuint>(ai.location) + row));
                        GL_CHECK_CODE(debug, glVertexAttribPointer(
                            math::numeric_cast<GLuint>(ai.location) + row,
                            math::numeric_cast<GLint>(vai.columns),
                            convert_attribute_type(vai.type),
                            vai.normalized ? GL_TRUE : GL_FALSE,
                            math::numeric_cast<GLsizei>(decl.bytes_per_vertex()),
                            reinterpret_cast<const GLvoid*>(vai.stride + row * vai.row_size())));
                    }
                });
            }
        });
    }

    void unbind_vertex_declaration(
        debug& debug,
        const shader_ptr& ps,
        const vertex_buffer_ptr& vb) noexcept
    {
        E2D_ASSERT(ps && vb);
        const vertex_declaration& decl = vb->decl();
        for ( std::size_t i = 0, e = decl.attribute_count(); i < e; ++i ) {
            const vertex_declaration::attribute_info& vai = decl.attribute(i);
            ps->state().with_attribute_location(vai.name, [&debug, &vai](const attribute_info& ai) noexcept {
                const GLuint rows = math::numeric_cast<GLuint>(vai.rows);
                for ( GLuint row = 0; row < rows; ++row ) {
                    GL_CHECK_CODE(debug, glDisableVertexAttribArray(
                        math::numeric_cast<GLuint>(ai.location) + row));
                }
            });
        }
    }

    void draw_indexed_primitive(
        debug& debug,
        render::topology tp,
        const index_buffer_ptr& ib,
        std::size_t first,
        std::size_t count) noexcept
    {
        E2D_ASSERT(ib);
        with_gl_bind_buffer(debug, ib->state().id(), [&debug, &tp, &ib, &first, &count]() noexcept {
            const index_declaration& decl = ib->decl();
            if ( first < ib->index_count() ) {
                GL_CHECK_CODE(debug, glDrawElements(
                    convert_topology(tp),
                    math::numeric_cast<GLsizei>(math::min(count, ib->index_count() - first)),
                    convert_index_type(decl.type()),
                    reinterpret_cast<const GLvoid*>(first * decl.bytes_per_index())));
            }
        });
    }

    template < typename F, typename... Args >
    void with_material_shader(
        debug& debug,
        const shader_ptr& ps,
        const render::property_block& pb,
        F&& f,
        Args&&... args)
    {
        bind_property_block(debug, ps, pb);
        try {
            stdex::invoke(
                std::forward<F>(f),
                std::forward<Args>(args)...);
        } catch (...) {
            unbind_property_block(debug, ps, pb);
            throw;
        }
        unbind_property_block(debug, ps, pb);
    }

    template < typename F, typename... Args >
    void with_geometry_vertices(
        debug& debug,
        const shader_ptr& ps,
        const render::geometry& geo,
        F&& f,
        Args&&... args)
    {
        for ( std::size_t i = 0, e = geo.vertices_count(); i < e; ++i ) {
            const vertex_buffer_ptr& vb = geo.vertices(i);
            if ( vb ) {
                bind_vertex_declaration(debug, ps, vb);
            }
        }
        try {
            stdex::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        } catch (...) {
            for ( std::size_t i = 0, e = geo.vertices_count(); i < e; ++i ) {
                const vertex_buffer_ptr& vb = geo.vertices(i);
                if ( vb ) {
                    unbind_vertex_declaration(debug, ps, vb);
                }
            }
            throw;
        }
        for ( std::size_t i = 0, e = geo.vertices_count(); i < e; ++i ) {
            const vertex_buffer_ptr& vb = geo.vertices(i);
            if ( vb ) {
                unbind_vertex_declaration(debug, ps, vb);
            }
        }
    }

    render::property_block& main_property_cache() {
        static render::property_block props;
        return props;
    }
    
    image grab_framebuffer_content(
        debug& debug,
        const opengl::gl_framebuffer_id& fb,
        const b2u& region)
    {
        image result;
        with_gl_bind_framebuffer(debug, fb,
            [&debug, &region, &result]() {
                GLint format = GL_RGBA;
                GLint type = GL_UNSIGNED_BYTE;
            #if E2D_RENDER_MODE == E2D_RENDER_MODE_OPENGL
                // GL_IMPLEMENTATION_COLOR_READ_FORMAT and GL_IMPLEMENTATION_COLOR_READ_TYPE requires OpenGL 4.5 
            #elif E2D_RENDER_MODE == E2D_RENDER_MODE_OPENGLES
                // try to get more accurate format for reading
                GL_CHECK_CODE(debug, glGetIntegerv(
                    GL_IMPLEMENTATION_COLOR_READ_FORMAT,
                    &format));
                GL_CHECK_CODE(debug, glGetIntegerv(
                    GL_IMPLEMENTATION_COLOR_READ_TYPE,
                    &type));
            #   error unknown render mode
            #endif

                image_data_format img_format;
                if ( (format == GL_ALPHA || format == GL_LUMINANCE) && type == GL_UNSIGNED_BYTE ) {
                    img_format = image_data_format::g8;
                } else if ( format == GL_RGB && type == GL_UNSIGNED_BYTE ) {
                    img_format = image_data_format::rgb8;
                } else if ( format == GL_RGBA && type == GL_UNSIGNED_BYTE ) {
                    img_format = image_data_format::rgba8;
                } else {
                    E2D_ASSERT_MSG(false, "unsupported pixel format");
                    // OpenGL ES 2 already supports RGBA8 format for glReadPixels
                    format = GL_RGBA;
                    type = GL_UNSIGNED_BYTE;
                    img_format = image_data_format::rgba8;
                }
                
                pixel_declaration decl = convert_image_data_format_to_pixel_declaration(img_format);
                buffer pixels;
                pixels.resize(((decl.bits_per_pixel() * region.size.x) / 8u) * region.size.y);

                GL_CHECK_CODE(debug, glReadPixels(
                    math::numeric_cast<GLint>(region.position.x),
                    math::numeric_cast<GLint>(region.position.y),
                    math::numeric_cast<GLsizei>(region.size.x),
                    math::numeric_cast<GLsizei>(region.size.y),
                    format,
                    type,
                    pixels.data()));
                result = image(region.size, img_format, std::move(pixels));
            });
        return result;
    }

    image grab_compressed_texture(
        render& render,
        const texture_ptr& tex,
        const b2u& region)
    {
        const char vs_source[] = R"glsl(
            #version 120

            attribute vec2 a_position;
            varying vec2 v_uv;

            void main(){
              v_uv = a_position * 0.5 + 0.5;
              gl_Position = vec4(a_position.xy, 0.0, 1.0);
            }
        )glsl";

        const char fs_source[] = R"glsl(
            #version 120

            uniform sampler2D u_texture;
            varying vec2 v_uv;

            void main(){
              gl_FragColor = texture2D(u_texture, v_uv);
            }
        )glsl";

        const v2f vertices[] = {
            v2f(-1.0f, 1.0f), v2f(-1.0f, -1.0f), v2f(1.0f, 1.0f), v2f(1.0f, -1.0f)
        };
        const u16 indices[] = {0, 1, 2, 3};

        shader_ptr shader = render.create_shader(vs_source, fs_source);
        if ( !shader ) {
            throw bad_render_operation();
        }

        vertex_buffer_ptr vbuffer = render.create_vertex_buffer(
            buffer_view(vertices),
            vertex_declaration().add_attribute<v2f>("a_position"),
            vertex_buffer::usage::static_draw);
        if ( !vbuffer ) {
            throw bad_render_operation();
        }

        index_buffer_ptr ibuffer = render.create_index_buffer(
            buffer_view(indices),
            index_declaration::index_type::unsigned_short,
            index_buffer::usage::static_draw);
        if ( !ibuffer ) {
            throw bad_render_operation();
        }
        
        render::geometry geometry;
        geometry.add_vertices(vbuffer);
        geometry.indices(ibuffer);
        geometry.topo(render::topology::triangles_strip);
        
        render::material material;
        material.add_pass(render::pass_state()
            .shader(shader)
            .properties(render::property_block()
                .sampler("u_texture", render::sampler_state()
                    .texture(tex)
                    .min_filter(render::sampler_min_filter::nearest)
                    .mag_filter(render::sampler_mag_filter::nearest))));

        // convert compressed format to non-compressed
        const auto convert_to_noncompressed = [](pixel_declaration decl) {
            #define DEFINE_CASE(x, y) case pixel_declaration::pixel_type::x: return pixel_declaration::pixel_type::y;
            switch ( decl.type() ) {
                DEFINE_CASE(rgb_dxt1, rgba8)
                DEFINE_CASE(rgba_dxt1, rgba8)
                DEFINE_CASE(rgba_dxt3, rgba8)
                DEFINE_CASE(rgba_dxt5, rgba8)
                DEFINE_CASE(rgb_pvrtc2, rgba8)
                DEFINE_CASE(rgb_pvrtc4, rgba8)
                DEFINE_CASE(rgba_pvrtc2, rgba8)
                DEFINE_CASE(rgba_pvrtc4, rgba8)
                DEFINE_CASE(rgba_pvrtc2_v2, rgba8)
                DEFINE_CASE(rgba_pvrtc4_v2, rgba8)
                default:
                    E2D_ASSERT_MSG(false, "unexpected pixel format");
                    return  pixel_declaration::pixel_type::rgba8;
            }
            #undef DEFINE_CASE
        };
        
        render_target_ptr rt = render.create_render_target(
            tex->size(),
            convert_to_noncompressed(tex->decl()),
            pixel_declaration::pixel_type::depth16,
            render_target::external_texture::color);

        render.execute(render::target_command(rt));
        render.execute(render::viewport_command(b2u(tex->size())));
        render.execute(render::draw_command(material, geometry));
        
        return render.grab_render_target(rt, region);
    }
    
}

namespace e2d
{
    //
    // shader
    //

    const shader::internal_state& shader::state() const noexcept {
        return *state_;
    }

    shader::shader(internal_state_uptr state)
    : state_(std::move(state)) {
        E2D_ASSERT(state_);
    }
    shader::~shader() noexcept = default;

    //
    // texture
    //

    const texture::internal_state& texture::state() const noexcept {
        return *state_;
    }

    texture::texture(internal_state_uptr state)
    : state_(std::move(state)) {
        E2D_ASSERT(state_);
    }
    texture::~texture() noexcept = default;

    const v2u& texture::size() const noexcept {
        return state_->size();
    }

    const pixel_declaration& texture::decl() const noexcept {
        return state_->decl();
    }

    //
    // index_buffer
    //

    const index_buffer::internal_state& index_buffer::state() const noexcept {
        return *state_;
    }

    index_buffer::index_buffer(internal_state_uptr state)
    : state_(std::move(state)) {
        E2D_ASSERT(state_);
    }
    index_buffer::~index_buffer() noexcept = default;

    std::size_t index_buffer::buffer_size() const noexcept {
        return state_->size();
    }

    std::size_t index_buffer::index_count() const noexcept {
        E2D_ASSERT(state_->size() % state_->decl().bytes_per_index() == 0);
        return state_->size() / state_->decl().bytes_per_index();
    }

    const index_declaration& index_buffer::decl() const noexcept {
        return state_->decl();
    }

    //
    // vertex_buffer
    //

    const vertex_buffer::internal_state& vertex_buffer::state() const noexcept {
        return *state_;
    }

    vertex_buffer::vertex_buffer(internal_state_uptr state)
    : state_(std::move(state)) {
        E2D_ASSERT(state_);
    }
    vertex_buffer::~vertex_buffer() noexcept = default;

    std::size_t vertex_buffer::buffer_size() const noexcept {
        return state_->size();
    }

    std::size_t vertex_buffer::vertex_count() const noexcept {
        E2D_ASSERT(state_->size() % state_->decl().bytes_per_vertex() == 0);
        return state_->size() / state_->decl().bytes_per_vertex();
    }

    const vertex_declaration& vertex_buffer::decl() const noexcept {
        return state_->decl();
    }

    //
    // render_target
    //

    const render_target::internal_state& render_target::state() const noexcept {
        return *state_;
    }

    render_target::render_target(internal_state_uptr state)
    : state_(std::move(state)) {
        E2D_ASSERT(state_);
    }
    render_target::~render_target() noexcept = default;

    const v2u& render_target::size() const noexcept {
        return state_->size();
    }

    const texture_ptr& render_target::color() const noexcept {
        return state_->color();
    }

    const texture_ptr& render_target::depth() const noexcept {
        return state_->depth();
    }

    //
    // render
    //

    render::render(debug& ndebug, window& nwindow)
    : state_(new internal_state(ndebug, nwindow)) {
        E2D_ASSERT(main_thread() == nwindow.main_thread());
    }
    render::~render() noexcept = default;

    shader_ptr render::create_shader(
        str_view vertex_source,
        str_view fragment_source)
    {
        E2D_ASSERT(is_in_main_thread());

        gl_shader_id vs = gl_compile_shader(
            state_->dbg(),
            vertex_shader_header_cstr(device_capabilities().profile),
            vertex_source,
            GL_VERTEX_SHADER);

        if ( vs.empty() ) {
            return nullptr;
        }

        gl_shader_id fs = gl_compile_shader(
            state_->dbg(),
            fragment_shader_header_cstr(device_capabilities().profile),
            fragment_source,
            GL_FRAGMENT_SHADER);

        if ( fs.empty() ) {
            return nullptr;
        }

        gl_program_id ps = gl_link_program(
            state_->dbg(),
            std::move(vs),
            std::move(fs));

        if ( ps.empty() ) {
            return nullptr;
        }

        return std::make_shared<shader>(
            std::make_unique<shader::internal_state>(
                state_->dbg(), std::move(ps)));
    }

    shader_ptr render::create_shader(
        const input_stream_uptr& vertex,
        const input_stream_uptr& fragment)
    {
        E2D_ASSERT(is_in_main_thread());

        str vertex_source, fragment_source;
        return streams::try_read_tail(vertex_source, vertex)
            && streams::try_read_tail(fragment_source, fragment)
            ? create_shader(vertex_source, fragment_source)
            : nullptr;
    }

    texture_ptr render::create_texture(
        const image& image)
    {
        E2D_ASSERT(is_in_main_thread());

        const pixel_declaration decl =
            convert_image_data_format_to_pixel_declaration(image.format());

        if ( !is_pixel_supported(decl) ) {
            state_->dbg().error("RENDER: Failed to create texture:\n"
                "--> Info: unsupported pixel declaration\n"
                "--> Pixel type: %0",
                pixel_declaration::pixel_type_to_cstr(decl.type()));
            return nullptr;
        }

        if ( decl.is_depth() && !device_capabilities().depth_texture_supported ) {
            state_->dbg().error("RENDER: Failed to create texture:\n"
                "--> Info: depth textures is unsupported\n"
                "--> Pixel type: %0",
                pixel_declaration::pixel_type_to_cstr(decl.type()));
            return nullptr;
        }

        if ( math::maximum(image.size()) > device_capabilities().max_texture_size ) {
            state_->dbg().error("RENDER: Failed to create texture:\n"
                "--> Info: unsupported texture size: %0\n"
                "--> Max size: %1",
                image.size(), device_capabilities().max_texture_size);
            return nullptr;
        }

        if ( !device_capabilities().npot_texture_supported ) {
            if ( !math::is_power_of_2(image.size().x) || !math::is_power_of_2(image.size().y) ) {
                state_->dbg().error("RENDER: Failed to create texture:\n"
                    "--> Info: non power of two texture is unsupported\n"
                    "--> Size: %0",
                    image.size());
            }
        }

        gl_texture_id id = gl_texture_id::create(state_->dbg(), GL_TEXTURE_2D);
        if ( id.empty() ) {
            state_->dbg().error("RENDER: Failed to create texture:\n"
                "--> Info: failed to create texture id");
            return nullptr;
        }

        with_gl_bind_texture(state_->dbg(), id, [this, &id, &image, &decl]() noexcept {
            if ( decl.is_compressed() ) {
                GL_CHECK_CODE(state_->dbg(), glCompressedTexImage2D(
                    id.target(),
                    0,
                    convert_pixel_type_to_internal_format_e(decl.type()),
                    math::numeric_cast<GLsizei>(image.size().x),
                    math::numeric_cast<GLsizei>(image.size().y),
                    0,
                    math::numeric_cast<GLsizei>(image.data().size()),
                    image.data().data()));
            } else {
                GL_CHECK_CODE(state_->dbg(), glTexImage2D(
                    id.target(),
                    0,
                    convert_pixel_type_to_internal_format(decl.type()),
                    math::numeric_cast<GLsizei>(image.size().x),
                    math::numeric_cast<GLsizei>(image.size().y),
                    0,
                    convert_image_data_format_to_external_format(image.format()),
                    convert_image_data_format_to_external_data_type(image.format()),
                    image.data().data()));
            }
        });

        return std::make_shared<texture>(
            std::make_unique<texture::internal_state>(
                state_->dbg(), std::move(id), image.size(), decl));
    }

    texture_ptr render::create_texture(
        const input_stream_uptr& image_stream)
    {
        E2D_ASSERT(is_in_main_thread());

        image image;
        if ( !images::try_load_image(image, image_stream) ) {
            return nullptr;
        }
        return create_texture(image);
    }

    texture_ptr render::create_texture(
        const v2u& size,
        const pixel_declaration& decl)
    {
        E2D_ASSERT(is_in_main_thread());

        if ( !is_pixel_supported(decl) ) {
            state_->dbg().error("RENDER: Failed to create texture:\n"
                "--> Info: unsupported pixel declaration\n"
                "--> Pixel type: %0",
                pixel_declaration::pixel_type_to_cstr(decl.type()));
            return nullptr;
        }

        if ( decl.is_depth() && !device_capabilities().depth_texture_supported ) {
            state_->dbg().error("RENDER: Failed to create texture:\n"
                "--> Info: depth textures is unsupported\n"
                "--> Pixel type: %0",
                pixel_declaration::pixel_type_to_cstr(decl.type()));
            return nullptr;
        }

        if ( math::maximum(size) > device_capabilities().max_texture_size ) {
            state_->dbg().error("RENDER: Failed to create texture:\n"
                "--> Info: unsupported texture size: %0\n"
                "--> Max size: %1",
                size, device_capabilities().max_texture_size);
            return nullptr;
        }

        if ( !device_capabilities().npot_texture_supported ) {
            if ( !math::is_power_of_2(size.x) || !math::is_power_of_2(size.y) ) {
                state_->dbg().error("RENDER: Failed to create texture:\n"
                    "--> Info: non power of two texture is unsupported\n"
                    "--> Size: %0",
                    size);
            }
        }

        gl_texture_id id = gl_texture_id::create(state_->dbg(), GL_TEXTURE_2D);
        if ( id.empty() ) {
            state_->dbg().error("RENDER: Failed to create texture:\n"
                "--> Info: failed to create texture id");
            return nullptr;
        }

        with_gl_bind_texture(state_->dbg(), id, [this, &id, &size, &decl]() noexcept {
            if ( decl.is_compressed() ) {
                buffer empty_data(decl.bits_per_pixel() * size.x * size.y / 8);
                GL_CHECK_CODE(state_->dbg(), glCompressedTexImage2D(
                    id.target(),
                    0,
                    convert_pixel_type_to_internal_format_e(decl.type()),
                    math::numeric_cast<GLsizei>(size.x),
                    math::numeric_cast<GLsizei>(size.y),
                    0,
                    math::numeric_cast<GLsizei>(empty_data.size()),
                    empty_data.data()));
            } else {
                GL_CHECK_CODE(state_->dbg(), glTexImage2D(
                    id.target(),
                    0,
                    convert_pixel_type_to_internal_format(decl.type()),
                    math::numeric_cast<GLsizei>(size.x),
                    math::numeric_cast<GLsizei>(size.y),
                    0,
                    convert_pixel_type_to_external_format(decl.type()),
                    convert_pixel_type_to_external_data_type(decl.type()),
                    nullptr));
            }
        #if E2D_RENDER_MODE == E2D_RENDER_MODE_OPENGL
            GL_CHECK_CODE(state_->dbg(), glTexParameteri(
                id.target(),
                GL_TEXTURE_MAX_LEVEL,
                0));
            GL_CHECK_CODE(state_->dbg(), glTexParameteri(
                id.target(),
                GL_TEXTURE_BASE_LEVEL,
                0));
        #endif
        });

        return std::make_shared<texture>(
            std::make_unique<texture::internal_state>(
                state_->dbg(), std::move(id), size, decl));
    }

    index_buffer_ptr render::create_index_buffer(
        buffer_view indices,
        const index_declaration& decl,
        index_buffer::usage usage)
    {
        E2D_ASSERT(is_in_main_thread());
        E2D_ASSERT(indices.size() % decl.bytes_per_index() == 0);

        if ( !is_index_supported(decl) ) {
            state_->dbg().error("RENDER: Failed to create index buffer:\n"
                "--> Info: unsupported index declaration\n"
                "--> Index type: %0",
                index_declaration::index_type_to_cstr(decl.type()));
            return nullptr;
        }

        gl_buffer_id id = gl_buffer_id::create(state_->dbg(), GL_ELEMENT_ARRAY_BUFFER);
        if ( id.empty() ) {
            state_->dbg().error("RENDER: Failed to create index buffer:\n"
                "--> Info: failed to create index buffer id");
            return nullptr;
        }

        with_gl_bind_buffer(state_->dbg(), id, [this, &id, &indices, &usage]() {
            GL_CHECK_CODE(state_->dbg(), glBufferData(
                id.target(),
                math::numeric_cast<GLsizeiptr>(indices.size()),
                indices.data(),
                convert_buffer_usage(usage)));
        });

        return std::make_shared<index_buffer>(
            std::make_unique<index_buffer::internal_state>(
                state_->dbg(), std::move(id), indices.size(), decl));
    }

    vertex_buffer_ptr render::create_vertex_buffer(
        buffer_view vertices,
        const vertex_declaration& decl,
        vertex_buffer::usage usage)
    {
        E2D_ASSERT(is_in_main_thread());
        E2D_ASSERT(vertices.size() % decl.bytes_per_vertex() == 0);

        if ( !is_vertex_supported(decl) ) {
            state_->dbg().error("RENDER: Failed to create vertex buffer:\n"
                "--> Info: unsupported vertex declaration");
            return nullptr;
        }

        gl_buffer_id id = gl_buffer_id::create(state_->dbg(), GL_ARRAY_BUFFER);
        if ( id.empty() ) {
            state_->dbg().error("RENDER: Failed to create vertex buffer:\n"
                "--> Info: failed to create vertex buffer id");
            return nullptr;
        }

        with_gl_bind_buffer(state_->dbg(), id, [this, &id, &vertices, &usage]() {
            GL_CHECK_CODE(state_->dbg(), glBufferData(
                id.target(),
                math::numeric_cast<GLsizeiptr>(vertices.size()),
                vertices.data(),
                convert_buffer_usage(usage)));
        });

        return std::make_shared<vertex_buffer>(
            std::make_unique<vertex_buffer::internal_state>(
                state_->dbg(), std::move(id), vertices.size(), decl));
    }

    render_target_ptr render::create_render_target(
        const v2u& size,
        const pixel_declaration& color_decl,
        const pixel_declaration& depth_decl,
        render_target::external_texture external_texture)
    {
        E2D_ASSERT(is_in_main_thread());

        E2D_ASSERT(
            depth_decl.is_depth() &&
            color_decl.is_color() &&
            !color_decl.is_compressed());

        if ( !device_capabilities().render_target_supported ) {
            state_->dbg().error("RENDER: Failed to create framebuffer:\n"
                "--> Info: render target is unsupported");
            return nullptr;
        }

        if ( math::maximum(size) > device_capabilities().max_renderbuffer_size ) {
            state_->dbg().error("RENDER: Failed to create framebuffer:\n"
                "--> Info: unsupported render target size: %0\n"
                "--> Max size: %1",
                size, device_capabilities().max_renderbuffer_size);
            return nullptr;
        }

        if ( !device_capabilities().npot_texture_supported ) {
            if ( !math::is_power_of_2(size.x) || !math::is_power_of_2(size.y) ) {
                state_->dbg().error("RENDER: Failed to create framebuffer:\n"
                    "--> Info: non power of two render target is unsupported\n"
                    "--> Size: %0",
                    size);
            }
        }

        gl_framebuffer_id id = gl_framebuffer_id::create(state_->dbg(), GL_FRAMEBUFFER);
        if ( id.empty() ) {
            state_->dbg().error("RENDER: Failed to create framebuffer:\n",
                "--> Info: failed to create framebuffer id");
            return nullptr;
        }

        bool need_color =
            !!(utils::enum_to_underlying(external_texture)
            & utils::enum_to_underlying(render_target::external_texture::color));

        bool need_depth =
            !!(utils::enum_to_underlying(external_texture)
            & utils::enum_to_underlying(render_target::external_texture::depth));

        texture_ptr color;
        texture_ptr depth;

        gl_renderbuffer_id color_rb(state_->dbg());
        gl_renderbuffer_id depth_rb(state_->dbg());

        if ( need_color ) {
            color = create_texture(size, color_decl);
            if ( !color ) {
                state_->dbg().error("RENDER: Failed to create framebuffer:\n"
                    "--> Info: failed to create color texture");
                return nullptr;
            }
            gl_attach_texture(state_->dbg(), id, color->state().id(), GL_COLOR_ATTACHMENT0);
        } else {
            color_rb = gl_compile_renderbuffer(
                state_->dbg(),
                size,
                convert_pixel_type_to_internal_format_e(color_decl.type()));
            if ( color_rb.empty() ) {
                state_->dbg().error("RENDER: Failed to create framebuffer:\n"
                    "--> Info: failed to create color renderbuffer");
                return nullptr;
            }
            gl_attach_renderbuffer(state_->dbg(), id, color_rb, GL_COLOR_ATTACHMENT0);
        }

        if ( need_depth ) {
            depth = create_texture(size, depth_decl);
            if ( !depth ) {
                state_->dbg().error("RENDER: Failed to create framebuffer:\n"
                    "--> Info: failed to create depth texture");
                return nullptr;
            }
            gl_attach_texture(state_->dbg(), id, depth->state().id(), GL_DEPTH_ATTACHMENT);
            if ( depth_decl.is_stencil() ) {
                gl_attach_texture(state_->dbg(), id, depth->state().id(), GL_STENCIL_ATTACHMENT);
            }
        } else {
            depth_rb = gl_compile_renderbuffer(
                state_->dbg(),
                size,
                convert_pixel_type_to_internal_format_e(depth_decl.type()));
            if ( depth_rb.empty() ) {
                state_->dbg().error("RENDER: Failed to create framebuffer:\n"
                    "--> Info: failed to create depth renderbuffer");
                return nullptr;
            }
            gl_attach_renderbuffer(state_->dbg(), id, depth_rb, GL_DEPTH_ATTACHMENT);
            if ( depth_decl.is_stencil() ) {
                gl_attach_renderbuffer(state_->dbg(), id, depth_rb, GL_STENCIL_ATTACHMENT);
            }
        }

        GLenum fb_status = GL_FRAMEBUFFER_COMPLETE;
        if ( !gl_check_framebuffer(state_->dbg(), id, &fb_status) ) {
            state_->dbg().error("RENDER: Failed to create framebuffer:\n"
                "--> Info: framebuffer is incomplete\n"
                "--> Status: %0",
                gl_framebuffer_status_to_cstr(fb_status));
            return nullptr;
        }

        return std::make_shared<render_target>(
            std::make_unique<render_target::internal_state>(
                state_->dbg(),
                std::move(id),
                size,
                std::move(color),
                std::move(depth),
                std::move(color_rb),
                std::move(depth_rb)));
    }

    render& render::execute(const draw_command& command) {
        E2D_ASSERT(is_in_main_thread());

        const material& mat = command.material_ref();
        const geometry& geo = command.geometry_ref();
        const property_block& props = command.properties_ref();

        for ( std::size_t i = 0, e = mat.pass_count(); i < e; ++i ) {
            const pass_state& pass = mat.pass(i);
            if ( !pass.shader() || !geo.indices() ) {
                continue;
            }
            try {
                const property_block& main_props = main_property_cache()
                    .merge(mat.properties())
                    .merge(pass.properties())
                    .merge(props);
                state_->set_states(pass.states());
                state_->set_shader_program(pass.shader());
                with_material_shader(state_->dbg(), pass.shader(), main_props, [this, &command, &pass, &geo]() noexcept {
                    with_geometry_vertices(state_->dbg(), pass.shader(), command.geometry_ref(), [this, &command, &geo]() noexcept {
                        draw_indexed_primitive(
                            state_->dbg(),
                            geo.topo(),
                            geo.indices(),
                            command.first_index(),
                            command.index_count());
                    });
                });
            } catch (...) {
                main_property_cache().clear();
                throw;
            }
            main_property_cache().clear();
        }
        return *this;
    }

    render& render::execute(const clear_command& command) {
        E2D_ASSERT(is_in_main_thread());

        bool clear_color =
            !!(utils::enum_to_underlying(command.clear_buffer())
            & utils::enum_to_underlying(clear_command::buffer::color));
        bool clear_depth =
            !!(utils::enum_to_underlying(command.clear_buffer())
            & utils::enum_to_underlying(clear_command::buffer::depth));
        bool clear_stencil =
            !!(utils::enum_to_underlying(command.clear_buffer())
            & utils::enum_to_underlying(clear_command::buffer::stencil));

        const render_target_ptr& rt = state_->render_target();
        bool has_color = !rt || rt->state().color() || !rt->state().color_rb().empty();
        bool has_depth = !rt || rt->state().depth() || !rt->state().depth_rb().empty();

        GLbitfield clear_mask = 0;
        if ( has_color && clear_color  ) {
            clear_mask |= GL_COLOR_BUFFER_BIT;
            GL_CHECK_CODE(state_->dbg(), glClearColor(
                math::numeric_cast<GLclampf>(math::saturate(command.color_value().r)),
                math::numeric_cast<GLclampf>(math::saturate(command.color_value().g)),
                math::numeric_cast<GLclampf>(math::saturate(command.color_value().b)),
                math::numeric_cast<GLclampf>(math::saturate(command.color_value().a))));
        }
        if ( has_depth ) {
            if ( clear_depth ) {
                clear_mask |= GL_DEPTH_BUFFER_BIT;
            #if E2D_RENDER_MODE == E2D_RENDER_MODE_OPENGL
                GL_CHECK_CODE(state_->dbg(), glClearDepth(
                    math::numeric_cast<GLclampd>(math::saturate(command.depth_value()))));
            #elif E2D_RENDER_MODE == E2D_RENDER_MODE_OPENGLES
                GL_CHECK_CODE(state_->dbg(), glClearDepthf(
                    math::numeric_cast<GLclampf>(math::saturate(command.depth_value()))));
            #else
            #   error unknown render mode
            #endif
            }
            if ( clear_stencil ) {
                clear_mask |= GL_STENCIL_BUFFER_BIT;
                GL_CHECK_CODE(state_->dbg(), glClearStencil(
                    math::numeric_cast<GLint>(command.stencil_value())));
            }
        }
        GL_CHECK_CODE(state_->dbg(), glClear(clear_mask));
        return *this;
    }

    render& render::execute(const target_command& command) {
        E2D_ASSERT(is_in_main_thread());
        state_->set_render_target(command.target());
        return *this;
    }

    render& render::execute(const viewport_command& command) {
        E2D_ASSERT(is_in_main_thread());

        const b2u viewport = math::make_minmax_rect(command.viewport_rect());
        GL_CHECK_CODE(state_->dbg(), glViewport(
            math::numeric_cast<GLint>(viewport.position.x),
            math::numeric_cast<GLint>(viewport.position.y),
            math::numeric_cast<GLsizei>(viewport.size.x),
            math::numeric_cast<GLsizei>(viewport.size.y)));

        if ( command.scissoring() ) {
            const b2u scissor = math::make_minmax_rect(command.scissor_rect());
            GL_CHECK_CODE(state_->dbg(), glScissor(
                math::numeric_cast<GLint>(scissor.position.x),
                math::numeric_cast<GLint>(scissor.position.y),
                math::numeric_cast<GLsizei>(scissor.size.x),
                math::numeric_cast<GLsizei>(scissor.size.y)));
            GL_CHECK_CODE(state_->dbg(), glEnable(GL_SCISSOR_TEST));
        } else {
            GL_CHECK_CODE(state_->dbg(), glDisable(GL_SCISSOR_TEST));
        }

        return *this;
    }

    render& render::update_buffer(
        const index_buffer_ptr& ibuffer,
        buffer_view indices,
        std::size_t offset)
    {
        E2D_ASSERT(is_in_main_thread());
        E2D_ASSERT(ibuffer);
        const std::size_t buffer_offset = offset * ibuffer->state().decl().bytes_per_index();
        E2D_ASSERT(indices.size() + buffer_offset <= ibuffer->state().size());
        E2D_ASSERT(indices.size() % ibuffer->state().decl().bytes_per_index() == 0);
        opengl::with_gl_bind_buffer(ibuffer->state().dbg(), ibuffer->state().id(),
            [&ibuffer, &indices, &buffer_offset]() noexcept {
                GL_CHECK_CODE(ibuffer->state().dbg(), glBufferSubData(
                    ibuffer->state().id().target(),
                    math::numeric_cast<GLintptr>(buffer_offset),
                    math::numeric_cast<GLsizeiptr>(indices.size()),
                    indices.data()));
            });
        return *this;
    }

    render& render::update_buffer(
        const vertex_buffer_ptr& vbuffer,
        buffer_view vertices,
        std::size_t offset)
    {
        E2D_ASSERT(is_in_main_thread());
        E2D_ASSERT(vbuffer);
        const std::size_t buffer_offset = offset * vbuffer->state().decl().bytes_per_vertex();
        E2D_ASSERT(vertices.size() + buffer_offset <= vbuffer->state().size());
        E2D_ASSERT(vertices.size() % vbuffer->state().decl().bytes_per_vertex() == 0);
        opengl::with_gl_bind_buffer(vbuffer->state().dbg(), vbuffer->state().id(),
            [&vbuffer, &vertices, &buffer_offset]() noexcept {
                GL_CHECK_CODE(vbuffer->state().dbg(), glBufferSubData(
                    vbuffer->state().id().target(),
                    math::numeric_cast<GLintptr>(buffer_offset),
                    math::numeric_cast<GLsizeiptr>(vertices.size()),
                    vertices.data()));
            });
        return *this;
    }

    render& render::update_texture(
        const texture_ptr& tex,
        const image& img,
        v2u offset)
    {
        E2D_ASSERT(is_in_main_thread());
        E2D_ASSERT(tex);

        const pixel_declaration decl =
            convert_image_data_format_to_pixel_declaration(img.format());
        if ( tex->decl() != decl ) {
            state_->dbg().error("RENDER: Failed to update texture:\n"
                "--> Info: incompatible pixel formats\n"
                "--> Texture format: %0\n"
                "--> Image format: %1",
                pixel_declaration::pixel_type_to_cstr(tex->decl().type()),
                pixel_declaration::pixel_type_to_cstr(decl.type()));
            throw bad_render_operation();
        }

        return update_texture(tex, img.data(), b2u(offset, img.size()));
    }

    render& render::update_texture(
        const texture_ptr& tex,
        buffer_view pixels,
        const b2u& region)
    {
        E2D_ASSERT(is_in_main_thread());
        E2D_ASSERT(tex);
        E2D_ASSERT(region.position.x < tex->size().x && region.position.y < tex->size().y);
        E2D_ASSERT(region.position.x + region.size.x <= tex->size().x);
        E2D_ASSERT(region.position.y + region.size.y <= tex->size().y);
        E2D_ASSERT(pixels.size() == region.size.y * ((region.size.x * tex->decl().bits_per_pixel()) / 8u));

        if ( tex->decl().is_compressed() ) {
            const v2u block_size = tex->decl().compressed_block_size();
            E2D_ASSERT(region.position.x % block_size.x == 0 && region.position.y % block_size.y == 0);
            E2D_ASSERT(region.size.x % block_size.x == 0 && region.size.y % block_size.y == 0);
            opengl::with_gl_bind_texture(state_->dbg(), tex->state().id(),
                [&tex, &pixels, &region]() noexcept {
                    GL_CHECK_CODE(tex->state().dbg(), glCompressedTexSubImage2D(
                        tex->state().id().target(),
                        0,
                        math::numeric_cast<GLint>(region.position.x),
                        math::numeric_cast<GLint>(region.position.y),
                        math::numeric_cast<GLsizei>(region.size.x),
                        math::numeric_cast<GLsizei>(region.size.y),
                        convert_pixel_type_to_internal_format_e(tex->state().decl().type()),
                        math::numeric_cast<GLsizei>(pixels.size()),
                        pixels.data()));
                });
        } else {
            opengl::with_gl_bind_texture(state_->dbg(), tex->state().id(),
                [&tex, &pixels, &region]() noexcept {
                    GL_CHECK_CODE(tex->state().dbg(), glTexSubImage2D(
                        tex->state().id().target(),
                        0,
                        math::numeric_cast<GLint>(region.position.x),
                        math::numeric_cast<GLint>(region.position.y),
                        math::numeric_cast<GLsizei>(region.size.x),
                        math::numeric_cast<GLsizei>(region.size.y),
                        convert_pixel_type_to_external_format(tex->state().decl().type()),
                        convert_pixel_type_to_external_data_type(tex->state().decl().type()),
                        pixels.data()));
                });
        }

        return *this;
    }

    image render::grab_texture(const texture_ptr& tex, const b2u& region)
    {
        E2D_ASSERT(tex);
        E2D_ASSERT(tex->decl().is_color());
        E2D_ASSERT(region.size.x > 0 && region.size.y > 0);
        E2D_ASSERT(region.position.x + region.size.x <= tex->size().x);
        E2D_ASSERT(region.position.y + region.size.y <= tex->size().y);

        if ( tex->decl().is_compressed() ) {
            return grab_compressed_texture(*this, tex, region);
        }

        gl_framebuffer_id id = gl_framebuffer_id::create(state_->dbg(), GL_FRAMEBUFFER);
        if ( id.empty() ) {
            throw bad_render_operation();
        }
        gl_attach_texture(state_->dbg(), id, tex->state().id(), GL_COLOR_ATTACHMENT0);
        GLenum fb_status = GL_FRAMEBUFFER_COMPLETE;
        if ( !gl_check_framebuffer(state_->dbg(), id, &fb_status) ) {
            throw bad_render_operation();
        }
        return grab_framebuffer_content(
            state_->dbg(),
            id,
            region);
    }
        
    image render::grab_render_target(const render_target_ptr& rt, const b2u& region)
    {
        E2D_ASSERT(rt);
        E2D_ASSERT(region.size.x > 0 && region.size.y > 0);
        E2D_ASSERT(region.position.x + region.size.x <= rt->size().x);
        E2D_ASSERT(region.position.y + region.size.y <= rt->size().y);
        return grab_framebuffer_content(
            state_->dbg(),
            rt->state().id(),
            region);
    }

    image render::grab_screen(const b2u& region)
    {
        E2D_ASSERT(region.size.x > 0 && region.size.y > 0);
        E2D_ASSERT(region.position.x + region.size.x <= state_->wnd().real_size().x);
        E2D_ASSERT(region.position.y + region.size.y <= state_->wnd().real_size().y);
        return grab_framebuffer_content(
            state_->dbg(),
            state_->default_fb(),
            region);
    }

    const render::device_caps& render::device_capabilities() const noexcept {
        E2D_ASSERT(is_in_main_thread());
        return state_->device_capabilities();
    }

    bool render::is_pixel_supported(const pixel_declaration& decl) const noexcept {
        E2D_ASSERT(is_in_main_thread());
        const device_caps& caps = device_capabilities();
        switch ( decl.type() ) {
            case pixel_declaration::pixel_type::depth16:
                return caps.depth_texture_supported
                    && caps.depth16_supported;
            case pixel_declaration::pixel_type::depth24:
                return caps.depth_texture_supported
                    && caps.depth24_supported;
            case pixel_declaration::pixel_type::depth24_stencil8:
                return caps.depth_texture_supported
                    && caps.depth24_stencil8_supported;
            case pixel_declaration::pixel_type::g8:
            case pixel_declaration::pixel_type::ga8:
            case pixel_declaration::pixel_type::rgb8:
            case pixel_declaration::pixel_type::rgba8:
                return true;
            case pixel_declaration::pixel_type::rgb_dxt1:
            case pixel_declaration::pixel_type::rgba_dxt1:
            case pixel_declaration::pixel_type::rgba_dxt3:
            case pixel_declaration::pixel_type::rgba_dxt5:
                return caps.dxt_compression_supported;
            case pixel_declaration::pixel_type::rgb_pvrtc2:
            case pixel_declaration::pixel_type::rgb_pvrtc4:
            case pixel_declaration::pixel_type::rgba_pvrtc2:
            case pixel_declaration::pixel_type::rgba_pvrtc4:
                return caps.pvrtc_compression_supported;
            case pixel_declaration::pixel_type::rgba_pvrtc2_v2:
            case pixel_declaration::pixel_type::rgba_pvrtc4_v2:
                return caps.pvrtc2_compression_supported;
            default:
                E2D_ASSERT_MSG(false, "unexpected pixel type");
                return false;
        }
    }

    bool render::is_index_supported(const index_declaration& decl) const noexcept {
        E2D_ASSERT(is_in_main_thread());
        const device_caps& caps = device_capabilities();
        switch ( decl.type() ) {
            case index_declaration::index_type::unsigned_byte:
            case index_declaration::index_type::unsigned_short:
                return true;
            case index_declaration::index_type::unsigned_int:
                return caps.element_index_uint;
            default:
                E2D_ASSERT_MSG(false, "unexpected index type");
                return false;
        }
    }

    bool render::is_vertex_supported(const vertex_declaration& decl) const noexcept {
        E2D_ASSERT(is_in_main_thread());
        return decl.attribute_count() <= device_capabilities().max_vertex_attributes;
    }
}

#endif
#endif
