/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018-2019, by Matvey Cherevko (blackmatov@gmail.com)
 ******************************************************************************/

#include "../common.hpp"
#include <random>

using namespace e2d;

namespace
{
    sound_source_ptr sound_laser;
    sound_source_ptr sound_meteor_boom;

    struct player {
        f32 speed{120.f};
        f32 shoot_counter{0};
        f32 shoot_interval{0.25f};
        bool trigger_pressed{false};
    };
    
    struct distance {
        distance (f32 max) :
            dist(0),
            max_dist(max)
        {}
        f32 dist{0};
        f32 max_dist{100.f};
    };
    
    struct physical_body {
        f32 velocity_value{0};
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
        shape_type type{shape_type::circle};
        f32 radius{1};
        u32 mask_group{0u};
        u32 mask_collision{0u};
    };
    
    struct collision_detected {
        u32 mask_group{0u};
    };
    
    struct meteor_generator_timer {
        f32 counter{0};
        f32 max_counter{0.5f};
    };

    struct object_generator {
        struct object_data {
            enum object_type { none, laser, meteor };
            object_type type{object_type::none};
            f32 velocity_value{0.f};
            v3f translation;
            q4f rotation;
            rad<f32> velocity_angle;
            node_iptr parentNode;
        };

        void addObject (const object_generator::object_data& d) {
            E2D_ASSERT(count < objects.size());
            objects[count] = d;
            count++;
        }

        std::array<object_data, 5> objects;
        i32 count{0};
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
                [&dt,&owner](const ecs::const_entity& e, player& p, physical_body& body, actor& act){
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

                        if ( k.is_key_pressed(keyboard_key::space) ) {
                            bool need_create_laser = false;
                            if ( p.trigger_pressed ) {
                                p.shoot_counter += dt;
                                if ( p.shoot_counter >= p.shoot_interval ) {
                                    p.shoot_counter -= p.shoot_interval;
                                    need_create_laser = true;
                                }
                            } else {
                                p.trigger_pressed = true;
                                p.shoot_counter = 0;
                                need_create_laser = true;
                            }

                            if ( need_create_laser ) {
                                owner.for_joined_components<scene, object_generator>(
                                    [&node,&body](const ecs::const_entity& e, const scene& s, object_generator& og){
                                        object_generator::object_data data;
                                        data.type = object_generator::object_data::object_type::laser;
                                        data.translation = node->translation();
                                        data.rotation = node->rotation();
                                        data.velocity_angle = body.velocity_angle;
                                        data.parentNode = node->parent();
                                        og.addObject(data);
                                    });
                            }
                        }

                        if ( k.is_key_just_released(keyboard_key::space) ) {
                            p.trigger_pressed = false;
                        }
                    }
                });
        }
    };
    
    class meteor_generation_timer_system final : public ecs::system {
    public:
        void process(ecs::registry& owner) override {
            const f32 dt = the<engine>().delta_time();
            owner.for_joined_components<scene, meteor_generator_timer, actor>(
                [&dt](ecs::entity e, const scene& s, meteor_generator_timer& timer, actor& act){
                    timer.counter += dt;
                    if ( timer.counter >= timer.max_counter  ) {
                        timer.counter = 0;
                        object_generator::object_data data;
                        data.type = object_generator::object_data::object_type::meteor;

                        v2u win_size = the<window>().real_size();
                        f32 outer_radius = 1.3f * math::maximum(win_size);
                        f32 inner_radius = 0.3f * math::minimum(win_size);

                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_real_distribution<f32> dis(-M_PI, M_PI);
                        auto outer_angle = make_rad(dis(gen));
                        auto inner_angle = make_rad(dis(gen));

                        auto r_mat = math::make_rotation_matrix3(outer_angle,0.f,0.f,1.f);
                        data.translation = v3f::unit_x() * r_mat * outer_radius;

                        r_mat = math::make_rotation_matrix3(inner_angle,0.f,0.f,1.f);
                        auto target = v3f::unit_x() * r_mat * inner_radius;
                        auto dir = math::normalized(data.translation - target);
                        auto unit_x = v3f::unit_x();
                        f32 dz = dir.x * unit_x.y - dir.y * unit_x.x;
                        rad<f32> move_angle = -math::atan2(math::abs(dz) + 1.0e-37f, math::dot(dir, unit_x));
                        data.velocity_angle = move_angle + math::half_pi<f32>();

                        std::uniform_real_distribution<f32> vel(80.f, 200.f);
                        data.velocity_value = vel(gen);

                        data.parentNode = act.node();

                        auto& og = e.get_component<object_generator>();
                        og.addObject(data);
                    }
                });
        }
    };

    class object_generation_system final : public ecs::system {
    public:
        void process(ecs::registry& owner) override {
            owner.for_joined_components<scene, object_generator>(
                [](const ecs::const_entity& e, const scene& s, object_generator& og){
                    for ( int i = 0; i < og.count; i++ ) {
                        auto& o = og.objects[i];
                        switch (o.type) {
                            case object_generator::object_data::object_type::meteor: {
                                auto meteor_big3_prefab_ref = the<library>().load_asset<prefab_asset>("meteor_big3_prefab.json");
                                if ( !meteor_big3_prefab_ref ) {
                                    break;
                                }
                                auto meteor_i = the<world>().instantiate(meteor_big3_prefab_ref->content());
                                meteor_i->entity_filler()
                                    .component<actor>(node::create(meteor_i, o.parentNode))
                                    .component<distance>(distance(2000.f))
                                    .component<physical_body>(physical_body{
                                        o.velocity_value,
                                        o.velocity_angle,
                                        rad<f32>(0),
                                        e2d::math::quarter_pi<f32>()
                                    })
                                    .component<collision>(collision{
                                        collision::shape_type::circle,
                                        45,
                                        collision::flag_group::meteor,
                                        collision::flag_group::laser | collision::flag_group::player
                                    });

                                node_iptr sprite_n = meteor_i->get_component<actor>().get().node();
                                sprite_n->translation(o.translation);
                            } break;

                            case object_generator::object_data::object_type::laser: {
                                auto laser_prefab_ref = the<library>().load_asset<prefab_asset>("laser_prefab.json");
                                if ( !laser_prefab_ref ) {
                                    break;
                                }
                                auto laser_i = the<world>().instantiate(laser_prefab_ref->content());
                                laser_i->entity_filler()
                                    .component<actor>(node::create(laser_i, o.parentNode))
                                    .component<distance>(distance(1000.f))
                                    .component<physical_body>(physical_body{
                                        500.f,
                                        o.velocity_angle,
                                        rad<f32>(0),
                                        rad<f32>(0)
                                    })
                                    .component<collision>(collision{
                                        collision::shape_type::line,
                                        57,
                                        collision::flag_group::laser,
                                        collision::flag_group::meteor
                                    });

                                node_iptr laser_n = laser_i->get_component<actor>().get().node();
                                laser_n->translation(o.translation);
                                laser_n->rotation(o.rotation);
                                sound_laser->play();
                            } break;

                            case object_generator::object_data::object_type::none: {
                                E2D_ASSERT(false);
                            } break;
                        }

                        og.count = 0;
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
            owner.for_joined_components<collision, physical_body, actor>(
                  [this,&owner](ecs::entity e1, collision& c1, physical_body& b1, actor& act1){
                      const node_iptr node1 = act1.node();
                      if ( node1 ) {
                          owner.for_joined_components<collision, physical_body, actor>(
                              [this,&e1,&c1,&b1,&act1,&node1](ecs::entity e2, collision& c2, physical_body& b2, actor& act2){
                                  if ( e1 != e2 ) {
                                      if ( c1.mask_group & c2.mask_collision || c2.mask_group & c1.mask_collision ) {
                                          const node_iptr node2 = act2.node();
                                          if ( node2 ) {
                                              if ( c1.type == collision::shape_type::line ) {
                                                  auto start_line_point = node1->translation();
                                                  auto r_mat = math::make_rotation_matrix3(b1.velocity_angle,0.f,0.f,1.f);
                                                  auto end_line_point = start_line_point + v3f::unit_y() * r_mat * c1.radius;
                                                  if ( collisionLineCircle(start_line_point.x,
                                                                           start_line_point.y,
                                                                           end_line_point.x,
                                                                           end_line_point.y,
                                                                           node2->translation().x,
                                                                           node2->translation().y,
                                                                           c2.radius
                                                                           ) ) {
                                                      collided(e1,c2.mask_group);
                                                      collided(e2,c1.mask_group);
                                                  }
                                              } else if ( c2.type == collision::shape_type::line ) {
                                                  auto start_line_point = node2->translation();
                                                  auto r_mat = math::make_rotation_matrix3(b2.velocity_angle,0.f,0.f,1.f);
                                                  auto end_line_point = start_line_point + v3f::unit_y() * r_mat * c2.radius;
                                                  if ( collisionLineCircle(start_line_point.x,
                                                                           start_line_point.y,
                                                                           end_line_point.x,
                                                                           end_line_point.y,
                                                                           node1->translation().x,
                                                                           node1->translation().y,
                                                                           c1.radius
                                                                           ) ) {
                                                      collided(e1,c2.mask_group);
                                                      collided(e2,c1.mask_group);
                                                  }
                                              } else {
                                                  if ( collisionCircleCircle(node1->translation().x,
                                                                             node1->translation().y,
                                                                             c1.radius,
                                                                             node2->translation().x,
                                                                             node2->translation().y,
                                                                             c2.radius
                                                                            ) ) {
                                                      collided(e1,c2.mask_group);
                                                      collided(e2,c1.mask_group);
                                                  }
                                              }
                                          }
                                      }
                                  }
                              });
                      }
                  });
        }
        
        bool collisionLineCircle (f32 x1, f32 y1, f32 x2, f32 y2, f32 cx, f32 cy, f32 r) {
            x1 -= cx;
            y1 -= cy;
            x2 -= cx;
            y2 -= cy;
            
            f32 dx = x2 - x1;
            f32 dy = y2 - y1;

            f32 a = dx*dx + dy*dy;
            f32 b = 2.*(x1*dx + y1*dy);
            f32 c = x1*x1 + y1*y1 - r*r;
            
            if (-b < 0)
                return (c < 0);
            if (-b < (2.*a))
                return ((4.*a*c - b*b) < 0);
            
            return (a+b+c < 0);
        }
        
        bool collisionCircleCircle (f32 x1, f32 y1, f32 r1, f32 x2, f32 y2, f32 r2) {
            f32 distX = x1 - x2;
            f32 distY = y1 - y2;
            f32 len = math::sqrt( (distX*distX) + (distY*distY) );
            
            if ( len <= r1 + r2 ) {
                return true;
            }
            return false;
        }
        
        void collided (ecs::entity e, u32 group) {
            if ( e.exists_component<collision_detected>() ) {
                e.get_component<collision_detected>().mask_group |= group;
            } else {
                e.assign_component<collision_detected>(collision_detected{group});
            }
        }
    };
    
    class collision_processing_system final : public ecs::system {
    public:
        void process(ecs::registry& owner) override {
            owner.for_joined_components<collision_detected, actor>(
                  [](const ecs::entity& e, collision_detected& c, actor& act){
                      if ( !e.exists_component<player>() ) {
                          if ( c.mask_group & collision::flag_group::laser ) {
                              sound_meteor_boom->play();
                          }

                          const node_iptr node = act.node();
                          if ( node ) {
                              the<world>().destroy_instance(node->owner());
                          }
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

            auto sstream_laser_sound = the<audio>().create_stream(the<vfs>().read(url("resources://bin/library/sfx_laser1.ogg")));
            auto sstream_meteor_boom_sound = the<audio>().create_stream(the<vfs>().read(url("resources://bin/library/sfx_zap.ogg")));
            auto spaceship_prefab_ref = the<library>().load_asset<prefab_asset>("player_spaceship_prefab.json");
            auto asteroids_bg_prefab_ref = the<library>().load_asset<prefab_asset>("asteroids_bg_prefab.json");

            if ( !spaceship_prefab_ref || !asteroids_bg_prefab_ref || !sstream_laser_sound || !sstream_meteor_boom_sound ) {
                return false;
            }

            sound_laser = the<audio>().create_source(sstream_laser_sound);
            sound_meteor_boom = the<audio>().create_source(sstream_meteor_boom_sound);

            auto scene_i = the<world>().instantiate();

            scene_i->entity_filler()
                .component<scene>()
                .component<meteor_generator_timer>()
                .component<object_generator>()
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
                sprite_n->translation(v3f{0, 0, 0});
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
                .system<meteor_generation_timer_system>(world::priority_update)
                .system<object_generation_system>(world::priority_update)
                .system<physical_system>(world::priority_update)
                .system<collision_system>(world::priority_update)
                .system<collision_processing_system>(world::priority_update)
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
