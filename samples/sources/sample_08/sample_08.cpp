/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "../common.hpp"
using namespace e2d;

namespace
{
    const char* vs_source_cstr = R"glsl(
        attribute vec2 a_position;
        attribute vec2 a_uv;
        attribute vec4 a_color;

        uniform mat4 u_MVP;

        varying vec4 v_color;
        varying vec2 v_uv;

        void main(){
          v_color = a_color;
          v_uv = a_uv;
          gl_Position = vec4(a_position, 0.0, 1.0) * u_MVP;
        }
    )glsl";

    const char* fs_source_cstr = R"glsl(
        uniform sampler2D u_texture;
        varying vec4 v_color;
        varying vec2 v_uv;

        void main(){
            gl_FragColor = v_color * texture2D(u_texture, v_uv);
        }
    )glsl";

    struct vertex {
        v2f position;
        v2f uv;
        color32 color;

        vertex(const v2f& pos, const v2f& uv, color32 col)
        : position(pos)
        , uv(uv)
        , color(col) {}

        static vertex_declaration decl() noexcept {
            return vertex_declaration()
                .add_attribute<v2f>("a_position")
                .add_attribute<v2f>("a_uv")
                .add_attribute<color32>("a_color").normalized();
        }
    };

    using rect_batch = batcher::rectangle_batch<vertex>;


    class game final : public engine::application {
    public:
        bool initialize() final {
            shader_ = the<render>().create_shader(
                vs_source_cstr, fs_source_cstr);

            texture_ = the<render>().create_texture(
                the<vfs>().read(url("resources://bin/library/cube_0.png")));

            if ( !shader_ || !texture_ ) {
                return false;
            }

            return true;
        }

        bool frame_tick() final {
            const keyboard& k = the<input>().keyboard();

            if ( the<window>().should_close() || k.is_key_just_released(keyboard_key::escape) ) {
                return false;
            }

            if ( k.is_key_just_pressed(keyboard_key::f12) ) {
                the<dbgui>().toggle_visible(!the<dbgui>().visible());
            }

            if ( k.is_key_pressed(keyboard_key::lsuper) && k.is_key_just_released(keyboard_key::enter) ) {
                the<window>().toggle_fullscreen(!the<window>().fullscreen());
            }

            return true;
        }

        void frame_render() final {
            const auto framebuffer_size = the<window>().real_size().cast_to<f32>();
            const auto projection = math::make_orthogonal_lh_matrix4(
                framebuffer_size, 0.f, 1.f);

            //material_.properties()
            //    .property("u_MVP", projection);

            auto& the_batcher = the<batcher>();
            the_batcher.begin(nullptr);

            the_batcher.add_batch(
                batcher::material()
                    .shader(shader_)
                    .sampler("u_texture",
                        render::sampler_state().texture(texture_)),
                rect_batch(
                    b2f(-100.0f, -150.0f, 200.0f, 150.0f),
                    b2f(0.0f, 0.0f, 1.0f, 1.0f),
                    color32::white()));

            the_batcher.flush();

            the<render>().execute(render::command_block<64>()
                .add_command(render::viewport_command(
                    the<window>().real_size()))
                .add_command(render::clear_command()
                    .color_value({1.f, 0.4f, 0.f, 1.f})));
        }
    private:
        shader_ptr shader_;
        texture_ptr texture_;
    };
}

int e2d_main(int argc, char *argv[]) {
    auto params = engine::parameters("sample_00", "enduro2d")
        .timer_params(engine::timer_parameters()
            .maximal_framerate(100));
    modules::initialize<engine>(argc, argv, params).start<game>();
    modules::shutdown<engine>();
    return 0;
}
