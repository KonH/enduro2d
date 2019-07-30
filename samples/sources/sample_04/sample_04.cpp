/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "../common.hpp"
using namespace e2d;

namespace
{
    class game_system final : public ecs::system {
    public:
        void process(ecs::registry& owner, ecs::entity_id) override {
            E2D_UNUSED(owner);
            const keyboard& k = the<input>().keyboard();

            if ( k.is_key_just_released(keyboard_key::f12) ) {
                the<dbgui>().toggle_visible(!the<dbgui>().visible());
            }

            if ( k.is_key_just_released(keyboard_key::escape) ) {
                the<window>().set_should_close(true);
            }

            if ( k.is_key_pressed(keyboard_key::lsuper) && k.is_key_just_released(keyboard_key::enter) ) {
                the<window>().toggle_fullscreen(!the<window>().fullscreen());
            }
            
            owner.process_systems_in_range(
                ecs::entity_id(0),
                world::priority_update_section_begin,
                world::priority_update_section_end);
        }
    };

    class camera_system final : public ecs::system {
    public:
        void process(ecs::registry& owner, ecs::entity_id) override {
            static vector<std::pair<ecs::const_entity,camera&>> temp_components;
            try {
                temp_components.reserve(owner.component_count<camera>());
                owner.for_joined_components<camera>([](const ecs::const_entity& e, camera& cam){
                    temp_components.emplace_back(e, cam);
                });
                std::sort(
                    temp_components.begin(),
                    temp_components.end(),
                    [](const auto& l, const auto& r){
                        return l.second.depth() < r.second.depth();
                    });
                for ( auto[e, cam] : temp_components ) {
                    if ( !cam.target() ) {
                        cam.viewport(
                            the<window>().real_size());
                        cam.projection(math::make_orthogonal_lh_matrix4(
                            the<window>().real_size().cast_to<f32>(), 0.f, 1000.f));
                        
                        the<render>().execute(render::command_block<3>()
                            .add_command(render::target_command(cam.target()))
                            .add_command(render::viewport_command(cam.viewport()))
                            .add_command(render::clear_command()
                                .color_value(cam.background())));

                        owner.process_systems_in_range(
                            e.id(),
                            world::priority_render_section_begin,
                            world::priority_render_section_end);
                    }
                }
            } catch (...) {
                temp_components.clear();
                throw;
            }
            temp_components.clear();
        }
    };

    class game final : public starter::application {
    public:
        bool initialize() final {
            return create_scene()
                && create_systems();
        }
    private:
        bool create_scene() {
            auto scene_prefab_res = the<library>().load_asset<prefab_asset>("scene_prefab.json");
            auto scene_go = scene_prefab_res
                ? the<world>().instantiate(scene_prefab_res->content())
                : nullptr;
            return !!scene_go;
        }

        bool create_systems() {
            ecs::registry_filler(the<world>().registry())
                .system<game_system>(world::priority_update_scheduler)
                .system<camera_system>(world::priority_render_scheduler);
            return true;
        }
    };
}

int e2d_main(int argc, char *argv[]) {
    const auto starter_params = starter::parameters(
        engine::parameters("sample_04", "enduro2d")
            .timer_params(engine::timer_parameters()
                .maximal_framerate(100)));
    modules::initialize<starter>(argc, argv, starter_params).start<game>();
    modules::shutdown<starter>();
    return 0;
}
