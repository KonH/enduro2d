/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "../common.hpp"
using namespace e2d;

namespace
{
    intrusive_ptr<e2d::prefab_asset> laser_prefab_ref;
    intrusive_ptr<e2d::prefab_asset> meteor_big3_prefab_ref;
    
    struct player {
        f32 speed = 100.f;
    };
    
    struct distance {
        distance (f32 max) :
            dist(0),
            max_dist(max)
        {}
        f32 dist;
        f32 max_dist;
    };
    
    struct physical_body {
        f32 velocity_value;
        rad<f32> velocity_angle;
        rad<f32> rotate_angle;
        rad<f32> rotate_speed;
    };
    
    struct collision {
        enum shape_type { line, circle };
        enum flag_group : u32 {
            player = 1u << 0,
            laser  = 1u << 1,
            meteor = 1u << 2
        };
        shape_type type;
        f32 radius;
        u32 mask_group{0u};
        u32 mask_collision{0u};
    };

    class game_system final : public ecs::system {
    public:
        void process(ecs::registry& owner) override {
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
            
            if ( k.is_key_pressed(keyboard_key::lsuper) && k.is_key_just_released(keyboard_key::enter) ) {
                the<window>().toggle_fullscreen(!the<window>().fullscreen());
            }
        }
    };

    class camera_system final : public ecs::system {
    public:
        void process(ecs::registry& owner) override {
            owner.for_joined_components<camera>(
            [](const ecs::const_entity&, camera& cam){
                if ( !cam.target() ) {
                    cam.viewport(
                        the<window>().real_size());
                    cam.projection(math::make_orthogonal_lh_matrix4(
                        the<window>().real_size().cast_to<f32>(), 0.f, 1000.f));
                }
            });
        }
    };
    
    class spaceship_system final : public ecs::system {
    public:
        void process(ecs::registry& owner) override {
            const f32 dt = the<engine>().delta_time();
            owner.for_joined_components<player, physical_body, actor>(
                [&dt](const ecs::const_entity&, player& p, physical_body& body, actor& act){
                    const keyboard& k = the<input>().keyboard();
                    auto move_angle_speed = e2d::math::pi<f32>(); //360 by sec
                    const node_iptr node = act.node();
                    if ( node ) {
                        if ( k.is_key_pressed(keyboard_key::left) ) {
                            body.velocity_angle += move_angle_speed * dt;
                        }
                        if ( k.is_key_pressed(keyboard_key::right) ) {
                            body.velocity_angle += -move_angle_speed * dt;
                        }
                        q4f q = math::make_quat_from_axis_angle(body.velocity_angle, v3f::unit_z());
                        node->rotation(q);
                        
                        body.velocity_value = 0;
                        if ( k.is_key_pressed(keyboard_key::up) ) {
                            body.velocity_value = p.speed;
                        }
                        if ( k.is_key_pressed(keyboard_key::down) ) {
                            body.velocity_value = -p.speed;
                        }
                        
                        if ( k.is_key_just_released(keyboard_key::space) ) {
                            auto laser_i = the<world>().instantiate(laser_prefab_ref->content());
                            laser_i->entity_filler()
                                .component<actor>(node::create(laser_i, node->parent()))
                                .component<distance>(distance(1000.f))
                                .component<physical_body>(physical_body{
                                    500.f,
                                    body.velocity_angle,
                                    rad<f32>(0),
                                    rad<f32>(0)
                                })
                                .component<collision>(collision{
                                    collision::shape_type::line,
                                    28,
                                    collision::flag_group::laser,
                                    collision::flag_group::meteor
                                });
                            
                            node_iptr laser_n = laser_i->get_component<actor>().get().node();
                            laser_n->translation(node->translation());
                            laser_n->rotation(node->rotation());
                        }
                    }
                });
        }
    };
    
    class meteor_generation_system final : public ecs::system {
    public:
        f32 counter = 0;
    public:
        void process(ecs::registry& owner) override {
            const f32 dt = the<engine>().delta_time();
            owner.for_joined_components<scene, actor>(
                [this, &dt](const ecs::const_entity& e, const scene& s, actor& act){
                    this->counter += dt;
                    if ( this->counter > 2.f  ) {
                        this->counter = 0;
                        
                        const node_iptr scene_r = act.node();
                        if ( scene_r ) {
                            auto meteor_i = the<world>().instantiate(meteor_big3_prefab_ref->content());
                            meteor_i->entity_filler()
                            .component<actor>(node::create(meteor_i, scene_r))
                            .component<distance>(distance(1000.f))
                            .component<physical_body>(physical_body{
                                80.f,
                                rad<f32>(0),
                                rad<f32>(0),
                                e2d::math::quarter_pi<f32>()
                            })
                            .component<collision>(collision{
                                collision::shape_type::circle,
                                43,
                                collision::flag_group::meteor,
                                collision::flag_group::laser | collision::flag_group::player
                            });
                            
                            node_iptr sprite_n = meteor_i->get_component<actor>().get().node();
                            sprite_n->translation(v3f{0, 0, 0});
                        }
                    }
                });
        }
    };
    
    
    class physical_system final : public ecs::system {
    public:
        void process(ecs::registry& owner) override {
            const f32 dt = the<engine>().delta_time();
            owner.for_joined_components<physical_body, actor>(
                [&dt](const ecs::const_entity& e, physical_body& body, actor& act){
                    const node_iptr node = act.node();
                    if ( node ) {
                        if ( body.rotate_speed != rad<f32>(0) ) {
                            body.rotate_angle += body.rotate_speed * dt;
                            q4f q = math::make_quat_from_axis_angle(body.rotate_angle, v3f::unit_z());
                            node->rotation(q);
                        }
                        
                        if ( body.velocity_value != 0 ) {
                            f32 dist = body.velocity_value * dt;
                            auto r_mat = math::make_rotation_matrix3(body.velocity_angle,0.f,0.f,1.f);
                            auto shift = v3f::unit_y() * r_mat * dist;
                            node->translation(node->translation() + shift);
                            
                            if ( e.exists_component<distance>() ) {
                                auto dist_comp = e.get_component<distance>();
                                dist_comp.dist += dist;
                                if ( dist_comp.dist >= dist_comp.max_dist ) {
                                    the<world>().destroy_instance(node->owner());
                                }
                            }
                        }
                    }
                });
        }
    };
    
    class collision_system final : public ecs::system {
    public:
        void process(ecs::registry& owner) override {
            owner.for_joined_components<collision, actor>(
                  [&owner](const ecs::const_entity& e1, collision& c1, actor& act1){
                      const node_iptr node1 = act1.node();
                      if ( node1 ) {
                          owner.for_joined_components<collision, actor>(
                              [&e1,&c1,&act1,&node1](const ecs::const_entity& e2, collision& c2, actor& act2){
                                  if ( e1 != e2 ) {
                                      const node_iptr node2 = act2.node();
                                      if ( node2 ) {
                                          
                                      }
                                  }
                              });
                      }
                  });
        }
    };

    class game final : public starter::application {
    public:
        bool initialize() final {
            return create_scene()
                && create_camera()
                && create_systems();
        }
    private:
        bool create_scene() {
            laser_prefab_ref = the<library>().load_asset<prefab_asset>("laser_prefab.json");
            meteor_big3_prefab_ref = the<library>().load_asset<prefab_asset>("meteor_big3_prefab.json");
            
            auto spaceship_prefab_ref = the<library>().load_asset<prefab_asset>("player_spaceship_prefab.json");
            auto asteroids_bg_prefab_ref = the<library>().load_asset<prefab_asset>("asteroids_bg_prefab.json");

            if ( !laser_prefab_ref || !meteor_big3_prefab_ref || !spaceship_prefab_ref || !asteroids_bg_prefab_ref ) {
                return false;
            }

            auto scene_i = the<world>().instantiate();

            scene_i->entity_filler()
                .component<scene>()
                .component<actor>(node::create(scene_i));

            auto scene_r = scene_i->get_component<actor>().get().node();
            
            {
                auto bg_i = the<world>().instantiate(asteroids_bg_prefab_ref->content());
                bg_i->entity_filler()
                .component<actor>(node::create(bg_i, scene_r));
                
                node_iptr sprite_n = bg_i->get_component<actor>().get().node();
                sprite_n->translation(v3f{-384.f,-384.f,0});
            }

            {
                auto spaceship_i = the<world>().instantiate(spaceship_prefab_ref->content());
                spaceship_i->entity_filler()
                    .component<player>()
                    .component<actor>(node::create(spaceship_i, scene_r))
                    .component<physical_body>(physical_body{
                        100.f,
                        e2d::math::pi<f32>(),
                        rad<f32>(0),
                        rad<f32>(0)
                    })
                    .component<collision>(collision{
                        collision::shape_type::circle,
                        42,
                        collision::flag_group::player,
                        collision::flag_group::meteor
                    });

                node_iptr sprite_n = spaceship_i->get_component<actor>().get().node();
                sprite_n->translation(v3f{0,-50.f,0});
            }

            return true;
        }

        bool create_camera() {
            auto camera_i = the<world>().instantiate();
            camera_i->entity_filler()
                .component<camera>(camera()
                    .background({1.f, 0.4f, 0.f, 1.f}))
                .component<actor>(node::create(camera_i));
            return true;
        }

        bool create_systems() {
            ecs::registry_filler(the<world>().registry())
                .system<game_system>(world::priority_update)
                .system<spaceship_system>(world::priority_update)
                .system<meteor_generation_system>(world::priority_update)
                .system<physical_system>(world::priority_update)
                .system<collision_system>(world::priority_update)
                .system<camera_system>(world::priority_pre_render);
            return true;
        }
    };
}

int e2d_main(int argc, char *argv[]) {
    const auto starter_params = starter::parameters(
        engine::parameters("sample_06", "enduro2d")
            .timer_params(engine::timer_parameters()
                          .maximal_framerate(100))
            .window_params(engine::window_parameters().size({768,768})));
    
    modules::initialize<starter>(argc, argv, starter_params).start<game>();
    modules::shutdown<starter>();
    return 0;
}
