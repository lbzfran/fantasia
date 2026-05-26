
#include "game.h"

fan_rect CollisionAdjusted(const fan_rect boundary, const fan_vec2 position);
bool32 CollisionCheckR(fan_rect_f32 a, fan_rect_f32 b);

#include "game_visual.c"
#include "game_archetype.c"

void MovementSystem(CMovement *m, CTransform *t, fan_vec2 direction, fan_rect_i32 bound_zone, float32 dt) {
    if (not m->initialized) {
        if (not fan_f32_isvalid(m->speed) or m->speed == 0.0f) m->speed = 4.0f;
        if (not fan_f32_isvalid(m->max_speed) or m->max_speed == 0.0f) m->max_speed = 5.0f;

        if (not fan_f32_isvalid(m->direction.x) or not fan_f32_isvalid(m->direction.y) or fan_vec2_length(m->direction) == 0.0f) {
            m->direction = fan_vec2_one();
        }

        m->active      = true;
        m->initialized = true;
    }

    fan_vec2 velocity = fan_vec2_zero();

    if (fan_vec2_length(m->knockback_force) > 0.0f) {
        m->velocity_force = fan_vec2_add(m->velocity_force, m->knockback_force);
        m->knockback_force = fan_vec2_zero();
    }

    if (fan_vec2_length(m->velocity_force) > 0.001f) {
        velocity            = fan_vec2_add(velocity, m->velocity_force);
        float32 damp_factor = 8.0f;
        m->velocity_force   = fan_vec2_lerp(m->velocity_force, damp_factor * dt, fan_vec2_zero());
    }

    m->velocity_input = fan_vec2_zero();

    if (m->lock_time > 0.0f) {
        m->lock_time = max(m->lock_time - dt, 0.0f);
    }

    if (fan_vec2_length(direction) > 0.0f) {
        m->direction      = direction;
        direction         = fan_vec2_normalize(direction);
        m->velocity_input = fan_vec2_scale(direction, m->speed);
    }
    velocity = fan_vec2_add(velocity, m->velocity_input);

    if (fan_f32_abs(m->velocity_input.x) > 0.001f) {
        float32 max_tilt = fan_f32_rad(40.0f);
        float32 target_rotation = m->velocity_input.x > 0.0f ? max_tilt : -max_tilt;

        t->rotation = fan_f32_lerp(t->rotation, 64.0f * dt, target_rotation);
    }
    else {
        t->rotation = fan_f32_lerp(t->rotation, 24.0f * dt, t->default_rotation);
    }

    t->position = fan_vec2_add(t->position, fan_vec2_scale(velocity, dt));

    // if (not (m->flags & MovementFlag_NoCollision)) {
    //     if (fan_vec2_length(t->scale) > 0.0f) {
    //         bound_zone.w = (int32)((float32)bound_zone.w - t->scale.x);
    //         bound_zone.h = (int32)((float32)bound_zone.h - t->scale.y);
    //     }
    //
    //     float32 overlap;
    //     float32 softness = 0.005f;
    //
    //     if (t->position.x < bound_zone.x) {
    //         overlap = (float32)bound_zone.x - t->position.x;
    //         t->position.x += overlap * softness;
    //     }
    //     else if (t->position.x > bound_zone.w) {
    //         overlap = t->position.x - (float32)bound_zone.w;
    //         t->position.x -= overlap * softness;
    //     }
    //
    //     if (t->position.y < bound_zone.y) {
    //         overlap = (float32)bound_zone.y - t->position.y;
    //         t->position.y += overlap * softness;
    //     }
    //     else if (t->position.y > bound_zone.h) {
    //         overlap = t->position.y - (float32)bound_zone.h;
    //         t->position.y -= overlap * softness;
    //     }
    // }
}

void PhysicsSystem(
    CPhysics *p,
    CTransform *t,
    fan_vec2 force,
    float32 render_width,
    float32 render_height,
    float32 dt
) {
    // RepairTransform(t);

    if (not p->initialized) {
        init_if_null(p->last_position.x, t->position.x);
        init_if_null(p->last_position.y, t->position.y);

        init_if_null(p->speed,           500.0f);
        init_if_null(p->friction,        0.2f);
        init_if_null(p->mass,            1.0f);

        p->active = true;
        p->initialized = true;
    }

    fan_vec2 velocity = fan_vec2_sub(t->position, p->last_position);
    fan_vec2 acceleration = fan_vec2_zero();

    fan_vec2 screen_size = {
        render_width,
        render_height
    };
    if (fan_vec2_length(t->scale) > 0.0f) {
        screen_size.x -= t->scale.x;
        screen_size.y -= t->scale.y;
    }

    velocity = fan_vec2_scale(velocity, 1.0f - p->friction * dt);

    float32 safe_mass = max(p->mass, 0.0001f);
    acceleration = fan_vec2_scale(force, p->speed / safe_mass);

    fan_vec2 new_position = fan_vec2_add(
        t->position,
        fan_vec2_add(velocity, fan_vec2_scale(acceleration, dt * dt))
    );

    new_position.x = clamp(new_position.x, 0.0f, screen_size.x);
    new_position.y = clamp(new_position.y, 0.0f, screen_size.y);

    p->last_position = t->position;
    t->position = new_position;
}

void SoundSystem(CSound *s, bool32 playing, float32 volume, float32 dt) {
    if (s->playing and not playing) {
        fan_sound_stop(s->sound);
        s->playing = false;
    }
    if (playing) {
        fan_sound_volume_set(s->sound, coalesce(volume, s->volume));
        fan_sound_pitch_set(s->sound, s->pitch);
        fan_sound_play(s->sound);
        s->playing = true;
    }
}

inline bool32 CollisionCheckR(fan_rect_f32 a, fan_rect_f32 b) {
    bool32 result = false;

    result = not (a.x + a.w < b.x or b.x + b.w < a.x or
                  a.y + a.h < b.y or b.y + b.h < a.y);

    return result;
}

bool32 CollisionCheckV(fan_vec2 aPos, fan_vec2 aSize, fan_vec2 bPos, fan_vec2 bSize) {
    bool32 result = false;

    // AABB
    result = not (aPos.x + aSize.x < bPos.x or bPos.x + bSize.x < aPos.x or
                  aPos.y + aSize.y < bPos.y or bPos.y + bSize.y < aPos.y);

    return result;
}

fan_rect CollisionAdjusted(const fan_rect boundary, const fan_vec2 position) {
    return (fan_rect) {
        position.x + boundary.x,
        position.y + boundary.y,
        boundary.w,
        boundary.h
    };
}

bool32 CollisionSystem(
	    const float32 dt,
        CCollision *a,
        CTransform *a_t,
        CCollision *b,
        CTransform *b_t
    ) {
    // NOTE(liam): this check is prob unnecessary
    if (not a->active or not b->active) {
        return false;
    }

    fan_rect a_collision = CollisionAdjusted(a->boundary, a_t->position);
    fan_rect b_collision = CollisionAdjusted(b->boundary, b_t->position);

    if (CollisionCheckR(a_collision, b_collision)) {
        if (a->flags & MovementFlag_Ghost or b->flags & MovementFlag_Ghost) {
            return true;
        }

        fan_vec2 aMax = (fan_vec2) {
            a_collision.x + a_collision.w,
            a_collision.y + a_collision.h
        };
        fan_vec2 bMax = (fan_vec2) {
            b_collision.x + b_collision.w,
            b_collision.y + b_collision.h
        };

        fan_vec2 overlap = (fan_vec2){
            min(aMax.x, bMax.x) - max(a_collision.x, b_collision.x),
            min(aMax.y, bMax.y) - max(a_collision.y, b_collision.y)
        };

        const float32 tolerance = 0.01f;
        if (overlap.x <= tolerance || overlap.y <= tolerance)
            return false;

        float32 correction;
        float32 aMove = (a->flags & CollisionFlag_Immovable) ? 0.0f : 1.0f;
        float32 bMove = (b->flags & CollisionFlag_Immovable) ? 0.0f : 1.0f;
        // if (a->flags & MovementFlag_CollideSoftly) {
        //     aMove *= dt;
        // }
        // if (b->flags & MovementFlag_CollideSoftly) {
        //     bMove *= dt;
        // }

        float32 totalMove = aMove + bMove;
        float32 aFactor = (totalMove > 0.0f) ? (aMove / totalMove) : 0.0f;
        float32 bFactor = (totalMove > 0.0f) ? (bMove / totalMove) : 0.0f;

        const float32 softness = 0.005f;
        if (overlap.x < overlap.y) {
            correction = overlap.x * softness;
            if (a_collision.x < b_collision.x) {
                a_t->position.x -= correction * aFactor;
                b_t->position.x += correction * bFactor;
            } else {
                a_t->position.x += correction * aFactor;
                b_t->position.x -= correction * bFactor;
            }
        } else {
            correction = overlap.y * softness;
            if (a_collision.y < b_collision.y) {
                a_t->position.y -= correction * aFactor;
                b_t->position.y += correction * bFactor;
            } else {
                a_t->position.y += correction * aFactor;
                b_t->position.y -= correction * bFactor;
            }
        }

        return true;
    }
    // NOTE(liam): potentially handle 'tunneling' if needed
    // likely solution: https://blog.hamaluik.ca/posts/swept-aabb-collision-using-minkowski-difference/
    return false;
}

bool32 AttackInArc(fan_vec2 target, fan_vec2 facing, float32 arc_angle, float32 progress) {
    float32 half = arc_angle * 0.5f;

    fan_vec2 start_dir = fan_vec2_rotate(facing, -half);
    fan_vec2 end_dir   = fan_vec2_rotate(facing,  half);

    fan_vec2 sweep_dir  = fan_vec2_normalize(fan_vec2_lerp(start_dir, progress, end_dir));
    fan_vec2 target_dir = fan_vec2_normalize(target);

    float32 dot_value = fan_vec2_dot(target_dir, sweep_dir);

    float32 tolerance = fan_f32_cos(fan_f32_rad(10));
    bool32 result = dot_value > tolerance;

    return result;
}

void AttackSystem(CAttack *a, CMovement *m, CTransform *t, CMovement *o_m, CTransform *o_t, float32 dt) {
    (void)dt;
    if (not a->attacking) return;

    if (a->cast_timer > 0.0f) {
        return;
    }

    float32 progress = a->timer / a->swing_time;

    if (progress >= 1.0f) {
        a->attacking = false;
        a->timer = 0.0f;
        m->lock_time = coalesce(a->cooldown_time, 0.2f);
        return;
    }

    fan_vec2 target_dist = (fan_vec2) {
        (o_t->position.x + (o_t->scale.x / 2)) - (t->position.x + (t->scale.x / 2)),
        (o_t->position.y + (o_t->scale.y / 2)) - (t->position.y + (t->scale.y / 2))
    };
    float32 dist_squared = fan_vec2_lengthsqr(target_dist);

    if (dist_squared <= a->attack_range * a->attack_range) {
        if (AttackInArc(target_dist, m->direction, a->arc_angle, progress)) {
            if (dist_squared > 0.000001f) {
                fan_vec2 knockback_dir  = fan_vec2_scale(target_dist, 1.0f / fan_f32_sqrt(dist_squared));
                fan_vec2 knockback_dist = fan_vec2_scale(knockback_dir, a->knockback * 0.1f);
                o_m->knockback_force = fan_vec2_add(o_m->knockback_force, knockback_dist);

                // :knockback animation
                o_t->scale = fan_vec2_scale(o_t->default_scale, 0.9f);
                float32 distance_ratio = clamp(fan_f32_sqrt(dist_squared), 0.0f, 1.0f);
                float32 knockback_rotation = 45.0f * (1.0f - distance_ratio);
                o_t->rotation = fan_vec2_dot(knockback_dir, m->direction) >= 0.0f ? knockback_rotation : -knockback_rotation;
            }
        }
    }
}

void AttackTick(CAttack *a, CMovement *m, float32 dt) {
    if (not a->attacking) return;

    if (a->cast_timer > 0.0f) {
        a->cast_timer = max(a->cast_timer - dt, 0.0f);
        return;
    }

    a->timer += dt;
    if (a->timer / a->swing_time >= 1.0f) {
        a->attacking = false;
        a->timer = 0.0f;
        m->lock_time = coalesce(a->cooldown_time, 0.2f);
    }
}

void QuestionSystem(CQuestion *question, CAnswer answer, float32 dt) {
    if (question and answer and not question->answered) {
        fan_log_debug("NOTICE: Question '%d' triggered with answer '%d'.\n", question->id, answer);
        question->answered = true;
        question->timer = max(question->cooldown_time, 5.0f);
    }
}

// NOTE(liam): must call whenever entities are added/removed
global void UpdateEntitySplit(World *world) {
    EntitySplit *split = &world->split;

    split->dynamic_count = 0;
    split->static_count  = 0;
    for (ssize i = 0; i < world->c_transform.size; i++) {
        ssize id = world->c_transform.dense[i];
        if (id == -1) continue;

        ssize move_index = world->c_movement.sparse[id];
        ssize bg_tag = world->c_tag_background.sparse[id];

        bool32 is_static = true;
        if (bg_tag < 0 or move_index != -1) {
            CMovement *movement = &world->c_movement.data[move_index];
            if (movement->flags & MovementFlag_NoCollision) continue;

            if (not (movement->flags & MovementFlag_Immovable)) {
                is_static = false;
            }
        }

        if (is_static)
            split->static_entities[split->static_count++]   = (int32)id;
        else
            split->dynamic_entities[split->dynamic_count++] = (int32)id;
    }
}

void UpdateEntities(
        World *world,
        GameState *state,
        float32 dt
    ) {
    static const float32 fixed_dt = 0.00025f;
    static float32 accumulator = 0.0f;

    for (ssize i = 0; i < world->c_transform.size; i++) {
        ssize id = world->c_transform.dense[i];
        if (id == -1)
            continue;

        CTransform *transform = fan_component_get_fast(&world->c_transform, id);
        // RepairTransform(transform);
        if (not transform->initialized) {
            transform->default_scale    = transform->scale;
            transform->default_rotation = transform->rotation;

            transform->initialized = true;
        }

        // NOTE(liam): unconditional transform system for dynamic entities.
        transform->scale    = fan_vec2_lerp(transform->scale,   7.5f * dt, transform->default_scale);
        // transform->rotation = fan_f32_lerp(transform->rotation, 15.0f * dt, transform->default_rotation);
    }

    EntitySplit *split = &world->split;
    if (world->update_entity_split) {
        fan_log_debug("Updating Entity Split!\n");
        UpdateEntitySplit(world);

        world->update_entity_split = false;
    }

    bool32 player_asks = false;
    accumulator += dt;
    while (accumulator >= fixed_dt) {
        bool32 last_iter = false;
        accumulator -= fixed_dt;
        if (accumulator < fixed_dt)
            last_iter = true;
        for (ssize i = 0; i < world->c_movement.size; i++) {
            ssize id = world->c_movement.dense[i];
            if (id == -1)
                continue;

            CMovement    *move      = fan_component_get(&world->c_movement,  id);
            CTransform   *transform = fan_component_get(&world->c_transform, id);
            CBehavior    *behavior  = fan_component_get(&world->c_behavior,  id);
            CAttack      *attack    = fan_component_get(&world->c_attack,    id);
            CAnimation   *anim      = fan_component_get(&world->c_animation, id);
            (void)anim;

            bool32 *interact   = fan_component_get(&world->c_interaction,  id);
            bool32 *interacted = fan_component_get(&world->c_interactable, id);

            // CAnswer *answer = fan_component_get(&world->c_tag_answer, id);

            fan_vec2 direction = fan_vec2_zero();

            if (interact != null) {
                *interact = false;
            }

            if (interacted != null) {
                *interacted = false;
            }

            if (id == world->spec_id.player) {
                if ((attack is null) or (attack and not attack->attacking)) {
                    direction = state->player_input.direction;
                }

            }
            else if (behavior) {
                behavior->updating = false;
                behavior->timer += dt;
                if (behavior->timer >= behavior->update_time) {
                    behavior->timer = 0.0f;
                    behavior->updating = true;
                }

                switch (behavior->type) {
                    case BehaviorType_Random: {
                        if (behavior->updating) {
                            float32 phase_time = behavior->update_time > 0.0f ? behavior->update_time : 120.0f;
                            behavior->update_time = phase_time * (0.5f + ((float32)fan_random_int(0, 100) / 100.0f));

                            if (fan_random_int(0, 100) < 60) {
                                behavior->direction = (fan_vec2) {
                                    (float32)fan_random_int(-1, 1),
                                    (float32)fan_random_int(-1, 1)
                                };

                                if (fan_vec2_length(behavior->direction) == 0.0f) {
                                    behavior->direction = (fan_vec2){ 1.0f, 0.0f };
                                }
                            }
                            else {
                                behavior->direction = fan_vec2_zero();
                            }
                        }
                        else {
                            direction = behavior->direction;
                        }
                    } break;
                    case BehaviorType_Follow: {
                        if (behavior->updating) {
                            CTransform *target_transform = fan_component_get(&world->c_transform, behavior->target_id);
                            fan_vec2 target_face         = target_transform->position;

                            if (id == world->spec_id.camera) {
                                fan_vec2 target_offset = target_transform->scale;
                                target_offset.y *= -1.0f;
                                target_offset = fan_vec2_scale(target_offset, 0.5f);
                                target_face = fan_vec2_add(target_face, target_offset);

                                float32 lerp_factor = 0.001f;
                                transform->position = fan_vec2_lerp(transform->position, lerp_factor, target_face);
                            }
                            else {
                                // fan_vec2 face = fan_vec2_normalize(fan_vec2_sub(target_face, transform->position));
                                //
                                // direction = (fan_vec2){ signof(face.x), signof(face.y) };

                                fan_vec2 to_target = fan_vec2_sub(target_face, transform->position);
                                float32 dist = fan_vec2_length(to_target);
                                float32 follow_radius = 1.5f; // entity stops within this radius

                                if (dist > follow_radius) {
                                    fan_vec2 to_target = fan_vec2_sub(target_face, transform->position);
                                    fan_vec2 face = fan_vec2_normalize(to_target);

                                    float ax = fan_f32_abs(face.x);
                                    float ay = fan_f32_abs(face.y);
                                    float diagonal_threshold = 0.25f; // tweak: smaller = stricter snapping

                                    if (fan_f32_abs(ax - ay) <= diagonal_threshold) {
                                        direction = (fan_vec2){ signof(face.x), signof(face.y) };
                                    }
                                    else if (ax > ay) {
                                        direction = (fan_vec2){ signof(face.x), 0 };
                                    }
                                    else {
                                        direction = (fan_vec2){ 0, signof(face.y) };
                                    }
                                }
                                else {
                                    direction = (fan_vec2){ 0, 0 }; // stop moving when close enough
                                }
                            }
                        }
                        else {
                            direction = move->direction;
                        }
                    } break;
                    case BehaviorType_None:
                    default: {
                    } break;
                }
            }

            MovementSystem(move, transform, direction, state->world_bound_zone, fixed_dt);

            if (last_iter and state->player_called_object_dump) {
                fan_log_nested_debug("\tid: %td\n", id);

                if (id == world->spec_id.player) {
                    fan_log_nested_debug("\t");
                    fan_vec2_print(transform->position);
                    fan_log_nested_debug("\t");
                    fan_vec2_print(transform->scale);

                    if (move) {
                        fan_log_nested_debug("\t");
                        fan_vec2_print(move->velocity_input);
                        fan_log_nested_debug("\t");
                        fan_vec2_print(move->direction);
                        fan_log_debug("\tmove->speed: %f\n", (float64)move->speed);
                    }
                }
            }
        }

        for (ssize i = 0; i < split->dynamic_count; i++) {
            ssize id = split->dynamic_entities[i];

            CMovement     *move      = fan_component_get_fast(&world->c_movement,   id);
            CTransform    *transform = fan_component_get_fast(&world->c_transform,  id);
            CAttack       *attack    = fan_component_get(&world->c_attack,          id);
            bool32        *interact  = fan_component_get(&world->c_interaction,     id);
            CCollision    *collision = fan_component_get(&world->c_collision,       id);
            ssize          tag_enemy = fan_component_get_value(&world->c_tag_enemy, id);
            // fan_rect_f32   zone      = fan_component_get_value_or_else(&world->c_zone, id, (fan_rect_f32){ 0 });

            (void)tag_enemy;

            CQuestion *question = fan_component_get(&world->c_question, id);
            CAnswer *answer = fan_component_get(&world->c_tag_answer,   id);

            if (answer exists and id == world->spec_id.player) {
                *answer = CAnswer_NONE;
                if (state->player_input.actions[3])
                    *answer = CAnswer_A;
                else if (state->player_input.actions[4])
                    *answer = CAnswer_B;
                else if (state->player_input.actions[5])
                    *answer = CAnswer_C;
                else if (state->player_input.actions[6])
                    *answer = CAnswer_D;

                player_asks = state->player_input.actions[2];
            }

            if (question exists and question->answered) {
                question->timer -= fixed_dt;
                if (question->timer <= 0.0f) {
                    question->timer = 0.0f;
                    question->answered = false;
                }
            }

            if (attack and id == world->spec_id.player and state->player_input.actions[0] and not attack->attacking) {
                attack->attacking = true;
                attack->cast_timer = 0.2f;
            }

            if (attack) {
                AttackTick(attack, move, fixed_dt);
            }

            if (collision and collision->active) {
                for (ssize j = i + 1; j < split->dynamic_count; j++) {
                    ssize other_id = split->dynamic_entities[j];

                    CTransform    *other_transform  = fan_component_get_fast(&world->c_transform,     other_id);
                    CMovement     *other_move       = fan_component_get_fast(&world->c_movement,      other_id);
                    bool32        *other_interacted = fan_component_get(&world->c_interactable,       other_id);
                    CCollision    *other_collision  = fan_component_get(&world->c_collision,          other_id);
                    ssize          other_tag_enemy  = fan_component_get_value(&world->c_tag_enemy,    other_id);
                    CQuestion     *other_question   = fan_component_get(&world->c_question,           other_id);

                    (void)other_tag_enemy;

                    // RepairTransform(other_transform);

                    if (attack) {
                        if (move->lock_time <= 0.0f) {
                            AttackSystem(attack, move, transform, other_move, other_transform, fixed_dt);
                        }
                    }

                    if (other_collision and collision->active) {
                        if (CollisionSystem(fixed_dt, collision, transform, other_collision, other_transform)) {
                            *interact = true;
                            *other_interacted = true; // NOTE: does nothing
                            if (
                                player_asks and
                                answer exists and
                                other_question exists and
                                other_question->answered is false
                               ) {
                                QuestionSystem(other_question, *answer, fixed_dt);

                                *answer = 0;
                                // player_answered = true;
                            }
                        }
                    }
                }
                for (ssize i = 0; i < split->static_count; i++) {
                    ssize other_id = split->static_entities[i];

                    CTransform     *other_transform  = fan_component_get_fast(&world->c_transform,   other_id);
                    CMovement      *other_move       = fan_component_get_fast(&world->c_movement,    other_id);
                    bool32         *other_interacted = fan_component_get(&world->c_interactable,     other_id);
                    CCollision     *other_collision  = fan_component_get(&world->c_collision,        other_id);
                    // fan_rect_f32    other_zone       = fan_component_get_value_or_else(&world->c_zone, other_id, (fan_rect_f32) { 0 });
                    ssize           other_tag_enemy  = fan_component_get_value(&world->c_tag_enemy,  other_id);

                    // RepairTransform(other_transform);
                    (void)other_tag_enemy;

                    if (attack) {
                        AttackSystem(attack, move, transform, other_move, other_transform, fixed_dt);
                    }

                    if (other_collision) {
                        if (CollisionSystem(fixed_dt, collision, transform, other_collision, other_transform)) {
                            // FanRectInt32Print(zone);
                            // FanRectInt32Print(other_zone);
                            *interact = true;
                            *other_interacted = true;

                        }
                    }
                }
            }

            if (last_iter and state->player_called_object_dump) {
                fan_log_debug("\tid: %td\n", id);

                if (id == world->spec_id.player) {
                    fan_log_debug("\t");
                    fan_vec2_print(transform->position);
                    fan_log_debug("\t");
                    fan_vec2_print(transform->scale);

                    if (move) {
                        fan_log_debug("\t");
                        fan_vec2_print(move->velocity_input);
                        fan_log_debug("\t");
                        fan_vec2_print(move->direction);
                        fan_log_debug("\tmove->speed: %f\n", (float64)move->speed);
                    }
                    fan_log_debug("\t");
                    fan_rect_print(collision->boundary);
                }
            }
        }
    }

    for (ssize i = 0; i < world->c_animation.size; i++) {
        ssize id = world->c_animation.dense[i];
        if (id == -1)
            continue;

        CAnimation *anim    = fan_component_get(&world->c_animation, id);
        CTexture   *texture = fan_component_get(&world->c_texture,   id);
        CMovement  *move    = fan_component_get(&world->c_movement,  id);
        (void)move;

        fan_vec2 direction = move->direction;
        // if (id == world->spec_id.player) {
            if ((fan_vec2_length(move->velocity_input)) > 0.0f) {
                if (direction.x > 0.0f) { // right
                    anim->request.id = 6;
                }
                else if (direction.x < 0.0f) { // left
                    anim->request.id = 7;
                }

                if (direction.y > 0.0f) { // up
                    anim->request.id = 4;
                }
                else if (direction.y < 0.0f) { // down
                    anim->request.id = 5;
                }
            }
            else {
                if (direction.x > 0.0f) { // right
                    anim->request.id = 2;
                }
                else if (direction.x < 0.0f) { // left
                    anim->request.id = 3;
                }

                if (direction.y > 0.0f) { // up
                    anim->request.id = 0;
                }
                else if (direction.y < 0.0f) { // down
                    anim->request.id = 1;
                }
            }
        // }

        AnimationSystem(anim, texture, world->anim_table, dt);
    }

    for (ssize i = 0; i < world->c_sound.size; i++) {
        ssize id = world->c_sound.dense[i];
        if (id == -1)
            continue;

        CSound *sound = &world->c_sound.data[i];

        bool32 playing = false;
        float32 volume = 0.0f;

        SoundSystem(sound, playing, volume, dt);
    }
}

void StateGetView(GameState *state) {
    float32 window_width  = (float32)fan_window_width();
    float32 window_height = (float32)fan_window_height();

    // fan_vec2 world_offset = fan_vec2_zero();

    // float32 scale_x = (window_width  / render_size.x);
    // float32 scale_y = (window_height / render_size.y);
    // float32 render_scale = min(scale_x, scale_y);
    //
    // float32 render_width  = render_size.x * render_scale;
    // float32 render_height = render_size.y * render_scale;
    // fan_vec2 render_offset = (fan_vec2){
    //     (window_width  - render_width)  * 0.5f,
    //     (window_height - render_height) * 0.5f,
    // };

    state->window_width  = (int32)window_width;
    state->window_height = (int32)window_height;
    state->world_scale = TILE_SIZE;
    // state->render_scale = render_scale;
    // state->render_offset = render_offset;
}

void GameOnReload(World *world, GameState *state) {
    // SceneMain(world);
    global fan_rect player_idle_down_frames[4]  = { 0 };
    global fan_rect player_idle_up_frames[4]    = { 0 };
    global fan_rect player_idle_left_frames[4]  = { 0 };
    global fan_rect player_idle_right_frames[4] = { 0 };

    global fan_rect player_walk_down_frames[4]  = { 0 };
    global fan_rect player_walk_up_frames[4]    = { 0 };
    global fan_rect player_walk_left_frames[4]  = { 0 };
    global fan_rect player_walk_right_frames[4] = { 0 };

    fan_vec2 citizen_size = (fan_vec2){ (float32)16.0f, (float32)16.0f };

    player_idle_left_frames[0]  = (fan_rect){
        0,                       0, citizen_size.x, citizen_size.y
    };
    player_idle_left_frames[1]  = (fan_rect){
        citizen_size.x,          0, citizen_size.x, citizen_size.y
    };
    player_idle_left_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), 0, citizen_size.x, citizen_size.y
    };
    player_idle_left_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), 0, citizen_size.x, citizen_size.y
    };

    player_idle_down_frames[0]  = (fan_rect){
        0,                       (2.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_down_frames[1]  = (fan_rect){
        citizen_size.x,          (2.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_down_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), (2.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_down_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), (2.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };

    player_idle_right_frames[0]  = (fan_rect){
        0,                       citizen_size.y, citizen_size.x, citizen_size.y
    };
    player_idle_right_frames[1]  = (fan_rect){
        citizen_size.x,          citizen_size.y, citizen_size.x, citizen_size.y
    };
    player_idle_right_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), citizen_size.y, citizen_size.x, citizen_size.y
    };
    player_idle_right_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), citizen_size.y, citizen_size.x, citizen_size.y
    };

    player_idle_up_frames[0] = (fan_rect){
        0,                       (3.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_up_frames[1] = (fan_rect){
        citizen_size.x,          (3.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_up_frames[2] = (fan_rect){
        (2.0f * citizen_size.x), (3.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_idle_up_frames[3] = (fan_rect){
        (3.0f * citizen_size.x), (3.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };

    // WALK

    player_walk_left_frames[0]  = (fan_rect){
        0,                       (4.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_left_frames[1]  = (fan_rect){
        citizen_size.x,          (4.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_left_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), (4.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_left_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), (4.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };

    player_walk_down_frames[0]  = (fan_rect){
        0,                       (6.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_down_frames[1]  = (fan_rect){
        citizen_size.x,          (6.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_down_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), (6.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_down_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), (6.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };

    player_walk_right_frames[0]  = (fan_rect){
        0,                       (5.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_right_frames[1]  = (fan_rect){
        citizen_size.x,          (5.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_right_frames[2]  = (fan_rect){
        (2.0f * citizen_size.x), (5.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_right_frames[3]  = (fan_rect){
        (3.0f * citizen_size.x), (5.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };

    player_walk_up_frames[0] = (fan_rect){
        0,                       (7.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_up_frames[1] = (fan_rect){
        citizen_size.x,          (7.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_up_frames[2] = (fan_rect){
        (2.0f * citizen_size.x), (7.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };
    player_walk_up_frames[3] = (fan_rect){
        (3.0f * citizen_size.x), (7.0f * citizen_size.y), citizen_size.x, citizen_size.y
    };


    global AnimationData anim_table[] = {
        { "player_idle_up",    player_idle_up_frames,    .frame_time = 0.4f, .frame_count = 4, true, -1 },
        { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.4f, .frame_count = 4, true, -1 },
        { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.4f, .frame_count = 4, true, -1 },
        { "player_idle_right", player_idle_right_frames, .frame_time = 0.4f, .frame_count = 4, true, -1 },
        { "player_walk_up",    player_walk_up_frames,    .frame_time = 0.2f, .frame_count = 4, true, -1 },
        { "player_walk_down",  player_walk_down_frames,  .frame_time = 0.2f, .frame_count = 4, true, -1 },
        { "player_walk_left",  player_walk_left_frames,  .frame_time = 0.2f, .frame_count = 4, true, -1 },
        { "player_walk_right", player_walk_right_frames, .frame_time = 0.2f, .frame_count = 4, true, -1 },
    };

    world->anim_table = anim_table;
}

void GameInit(fan_allocator *mem, World *world, GameState *state) {
    fan_fps_target(60);

    StateGetView(state);

    ssize split_size      = kilobytes(1);
    ssize component_size  = 256;

    fan_component_create(&world->c_transform,      mem, component_size);
    fan_component_create(&world->c_shape,          mem, component_size);
    fan_component_create(&world->c_movement,       mem, component_size);
    fan_component_create(&world->c_texture,        mem, component_size);
    fan_component_create(&world->c_behavior,       mem, component_size);
    fan_component_create(&world->c_animation,      mem, component_size);
    fan_component_create(&world->c_physics,        mem, component_size);
    fan_component_create(&world->c_sound,          mem, component_size);
    fan_component_create(&world->c_light,          mem, component_size);
    fan_component_create(&world->c_text,           mem, component_size);

    fan_component_create(&world->c_interaction,    mem, component_size);
    fan_component_create(&world->c_interactable,   mem, component_size);
    fan_component_create(&world->c_collision,      mem, component_size);
    fan_component_create(&world->c_attack,         mem, component_size);

    fan_component_create(&world->c_tag_background, mem, component_size);
    fan_component_create(&world->c_tag_enemy,      mem, component_size);
    fan_component_create(&world->c_question,       mem, component_size);
    fan_component_create(&world->c_tag_answer,     mem, component_size);

    world->split.dynamic_entities = fan_make(mem, split_size);
    world->split.dynamic_capacity = split_size;

    world->split.static_entities  = fan_make(mem, split_size);
    world->split.static_capacity  = split_size;

    fan_texture default_sprite = fan_texture_load("./resources/Citizens/Male/Artun/Artun.png");
    fan_sprite_init(&world->assets, &default_sprite, mem);
    world->tilesets = fan_make(mem, sizeof(fan_texture) * 2);

    fan_sprite_load("./resources/Citizens/Female", &world->assets, mem);

	int32 map_size_x = 8;
    int32 map_size_y = 8;
    TileMap map = (TileMap) {
        .tile_size    = 16,
		.logic_tiles  = fan_matrix_create(mem, map_size_x, map_size_y),
        .visual_tiles = fan_matrix_create(mem, map_size_x, map_size_y)
    };

    world->tile_atlas = GridAtlasCreate(map.tile_size);
	world->map = map;

    SceneMain(world);

    // fan_matrix_randomize(world->map.logic_tiles, 0, 1);

    GridWorldGenerate(world->map);

    state->world_bound_zone  = (fan_rect_i32){ .w = 10, .h = 6 };
    state->camera_zoom = 1.0f;

    state->render_size = (fan_vec2){ 640, 480 };
    state->rendermap = fan_rtexture_load((int32)state->render_size.x, (int32)state->render_size.y);
    state->lightmap  = fan_rtexture_load((int32)state->render_size.x, (int32)state->render_size.y);

    state->tilemap = fan_rtexture_load((int32)state->render_size.x * 10, (int32)state->render_size.y * 6);

    state->music = fan_music_load("./resources/My Uncles Last Voyage.mp3");
    fan_music_play(state->music);
    fan_music_volume_set(state->music, 0.4f);

    fan_log_debug("Successfully passed initialization!\n");
}

void GameUpdateAndRender(fan_allocator *a, World *world, GameState *state, float32 dt) {
    (void)a;

    if (state->window_resized) {
        // TODO(liam):
        // There is a bug that causes issues with the tilemap to
        // have parts of it disappear when resizing screen.
        StateGetView(state);
        // fan_matrix_randomize(world->map.logic_tiles, 0, 1);
        GridWorldGenerate(world->map);
        state->window_resized = false;
    }

    fan_music_update(state->music);

    // CTransform *cam_transform = fan_component_get(&world.c_transform, world.spec_id.camera);
    // camera.target = fan_vec2_add(cam_transform->position, fan_vec2_scale(cam_transform->scale, 0.5f));

    UpdateEntities(world, state, dt);
    RenderEntities(world, state, dt);
}

void GameClose(fan_allocator *a, World *world, GameState *state) {
    (void)a;
    fan_sprite_unload(&world->assets, a);
    for (ssize i = 0; i < world->c_sound.size; i++) {
        if (world->c_texture.dense[i] == -1)
            continue;
        fan_sound_unload(world->c_sound.data[i].sound);
    }
    fan_texture_unload(state->rendermap.texture);
    fan_texture_unload(state->lightmap.texture);
    fan_texture_unload(state->tilemap.texture);
    fan_rtexture_unload(state->rendermap);
    fan_rtexture_unload(state->lightmap);
    fan_rtexture_unload(state->tilemap);
    fan_music_unload(state->music);
}
