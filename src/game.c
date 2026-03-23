
#include "game.h"

#include "game_visual.c"
#include "game_archetype.c"

void MovementSystem(CMovement *m, CTransform *t, fan_vec2 direction, fan_rect_i32 bound_zone, float32 dt) {
    if (not m->initialized) {
        init_if_null(m->speed,       4.0f);
        init_if_null(m->max_speed,   5.0f);

        init_if_null(m->direction.x, 1.0f);
        init_if_null(m->direction.y, 1.0f);

        m->active      = true;
        m->initialized = true;
    }

    fan_vec2 velocity;
    if (fan_vec2_length(m->velocity_force) > 0.001f) {
        velocity            = m->velocity_force;
        float32 damp_factor = 0.98f;
        m->velocity_force   = fan_vec2_scale(m->velocity_force, damp_factor);
        m->lock_time        = 0.15f;
    }
    else {
        m->velocity_input = fan_vec2_zero();
        if (m->lock_time > 0.0f) {
            m->lock_time = max(m->lock_time - dt, 0.0f);
        }
        else if (fan_vec2_length(direction) > 0.0f) {
            m->direction      = direction;
            direction         = fan_vec2_normalize(direction);
            m->velocity_input = fan_vec2_scale(direction, m->speed);
        }
        velocity = m->velocity_input;
    }

    t->position = fan_vec2_add(t->position, fan_vec2_scale(velocity, dt));

    // float32 damp_factor = 0.9f;
    // m->velocity_force = fan_vec2_scale(m->velocity_force, damp_factor);
    // m->velocity_force = FanVector2AddValue(m->velocity_force, -decay * dt);

    if (not (m->flags & MovementFlag_NoCollision)) {
        if (fan_vec2_length(t->scale) > 0.0f) {
            bound_zone.w = (int32)((float32)bound_zone.w - t->scale.x);
            bound_zone.h = (int32)((float32)bound_zone.h - t->scale.y);
        }

        float32 overlap;
        float32 softness = 0.005f;

        // if (m->flags & MovementFlag_Ghost) {
        //     softness = 0.0f;
        // }

        if (t->position.x < bound_zone.x) {
            overlap = (float32)bound_zone.x - t->position.x;
            t->position.x += overlap * softness;
        }
        else if (t->position.x > bound_zone.w) {
            overlap = t->position.x - (float32)bound_zone.w;
            t->position.x -= overlap * softness;
        }

        if (t->position.y < bound_zone.y) {
            overlap = (float32)bound_zone.y - t->position.y;
            t->position.y += overlap * softness;
        }
        else if (t->position.y > bound_zone.h) {
            overlap = t->position.y - (float32)bound_zone.h;
            t->position.y -= overlap * softness;
        }
    }
}

void PhysicsSystem(
    CPhysics *p,
    CTransform *t,
    fan_vec2 force,
    float32 render_width,
    float32 render_height,
    float32 dt
) {
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

bool32 CollisionSystem(
        CTransform *a,
	    CMovement *a_m,
	    CTransform *b,
	    CMovement *b_m,
	    float32 dt
    ) {
    // NOTE(liam): this check is prob unnecessary
    if (a_m->flags & MovementFlag_NoCollision or b_m->flags & MovementFlag_NoCollision)
        return false;

    if (CollisionCheckV(a->position, a->scale, b->position, b->scale)) {
        if (a_m->flags & MovementFlag_Ghost or b_m->flags & MovementFlag_Ghost) {
            return true;
        }

        fan_vec2 aMax = (fan_vec2){
            a->position.x + a->scale.x,
            a->position.y + a->scale.y
        };
        fan_vec2 bMax = (fan_vec2){
            b->position.x + b->scale.x,
            b->position.y + b->scale.y
        };

        fan_vec2 overlap = (fan_vec2){
            min(aMax.x, bMax.x) - max(a->position.x, b->position.x),
            min(aMax.y, bMax.y) - max(a->position.y, b->position.y)
        };

        const float32 tolerance = 0.01f;
        if (overlap.x <= tolerance || overlap.y <= tolerance)
            return false;

        float32 correction;
        float32 aMove = (a_m->flags & MovementFlag_Immovable) ? 0.0f : 1.0f;
        float32 bMove = (b_m->flags & MovementFlag_Immovable) ? 0.0f : 1.0f;
        if (a_m->flags & MovementFlag_CollideSoftly) {
            aMove *= dt;
        }
        if (b_m->flags & MovementFlag_CollideSoftly) {
            bMove *= dt;
        }

        float32 totalMove = aMove + bMove;
        float32 aFactor = (totalMove > 0.0f) ? (aMove / totalMove) : 0.0f;
        float32 bFactor = (totalMove > 0.0f) ? (bMove / totalMove) : 0.0f;

        const float32 softness = 0.005f;
        if (overlap.x < overlap.y) {
            correction = overlap.x * softness;
            if (a->position.x < b->position.x) {
                a->position.x -= correction * aFactor;
                b->position.x += correction * bFactor;
            } else {
                a->position.x += correction * aFactor;
                b->position.x -= correction * bFactor;
            }
        } else {
            correction = overlap.y * softness;
            if (a->position.y < b->position.y) {
                a->position.y -= correction * aFactor;
                b->position.y += correction * bFactor;
            } else {
                a->position.y += correction * aFactor;
                b->position.y -= correction * bFactor;
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
    if (not a->attacking) return;

    if (a->cast_timer > 0.0f) {
        // printf("cast_timer: %f\n", a->cast_timer);
        a->cast_timer = max(a->cast_timer - dt, 0.0f);
        return;
    }

    a->timer += dt;
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
            float32 knockback_base_factor = 1.0f;
            fan_vec2 knockback_dir  = fan_vec2_scale(target_dist, 1.0f / fan_f32_sqrt(dist_squared));
            fan_vec2 knockback_dist = fan_vec2_scale(knockback_dir, a->knockback * knockback_base_factor);
            o_m->velocity_force = fan_vec2_add(o_m->velocity_force, knockback_dist);
        }
    }
}

void MagicUpdate(CMagic *ma, int32 value, int32 count) {
    int32 iterations = fan_i32_clamp(count, 0, 8);
    for (ssize i = 0; i < iterations; i++) {
        ma->index = (ma->index + 1) & 7;
        ma->cast[ma->index] = value;
    }
}

void MagicSystem(CMagic *ma, int32 input, bool32 casting, float32 dt) {
    if (input >= 1 and input <= 3) {
        MagicUpdate(ma, input, 1);
    }

    if (not casting) {
        return;
    }

    int32 code = ma->cast[ma->index] +
        ma->cast[(ma->index + 8 - 1) % 8] * 10 +
        ma->cast[(ma->index + 8 - 2) % 8] * 100 +
        ma->cast[(ma->index + 8 - 3) % 8] * 1000;

    printf("cast: %04d\n", code);

    switch (code) {
        case 11: {
            printf("casting: bat!\n");
            break;
        };
        case 112: {
            printf("casting: pop!\n");
            break;
        };
        case 221: {
            printf("casting: slice!\n");
            break;
        };
        case 121: {
            printf("casting: spread!\n");
            break;
        };
        case 333: {
            printf("casting: heal!\n");
            break;
        };

        default:
            // printf("casting: not found...\n");
            break;
    }

    // null out last 4 code
    MagicUpdate(ma, 0, 4);
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

    EntitySplit *split = &world->split;
    if (world->update_entity_split) {
        printf("Updating Entity Split!\n");
        UpdateEntitySplit(world);

        for (ssize i = 0; i < world->c_transform.size; i++) {
            ssize id = world->c_transform.dense[i];
            if (id == -1)
                continue;

            CTransform *transform = &world->c_transform.data[i];
            if (not transform->initialized) {
                init_if_null(transform->scale.x, 1.0f);
                init_if_null(transform->scale.y, 1.0f);

                transform->initialized = true;
            }
        }
        world->update_entity_split = false;
    }

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

            CMovement    *move      = ComponentGet(&world->c_movement,  id);
            CTransform   *transform = ComponentGet(&world->c_transform, id);
            CBehavior    *behavior  = ComponentGet(&world->c_behavior,  id);
            CAttack      *attack    = ComponentGet(&world->c_attack,    id);
            CAnimation   *anim      = ComponentGet(&world->c_animation, id);

            bool32 *interact   = ComponentGet(&world->c_interaction, id);
            bool32 *interacted = ComponentGet(&world->c_interactable, id);

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
                            direction = (fan_vec2) {
                                (float32)fan_random_int(-1, 1),
                                (float32)fan_random_int(-1, 1)
                            };
                        }
                        else {
                            // keeps entity moving rather than staying still
                            direction = move->direction;
                        }
                    } break;
                    case BehaviorType_Follow: {
                        if (behavior->updating) {
                            CTransform *target_transform = ComponentGet(&world->c_transform, behavior->target_id);
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
                printf("\tid: %td\n", id);

                if (id == world->spec_id.player) {
                    printf("\t");
                    fan_vec2_print(transform->position);
                    printf("\t");
                    fan_vec2_print(transform->scale);

                    if (move) {
                        printf("\t");
                        fan_vec2_print(move->velocity_input);
                        printf("\t");
                        fan_vec2_print(move->direction);
                        printf("\tmove->speed: %f\n", (float64)move->speed);
                    }
                }
            }
        }

        for (ssize i = 0; i < split->dynamic_count; i++) {
            ssize id = split->dynamic_entities[i];

            CMovement     *move      = ComponentGetFast(&world->c_movement,    id);
            CTransform    *transform = ComponentGetFast(&world->c_transform,   id);
            CAttack       *attack    = ComponentGet(&world->c_attack,          id);
            CMagic        *magic     = ComponentGet(&world->c_magic,           id);
            bool32        *interact  = ComponentGet(&world->c_interaction,     id);
            ssize          tag_enemy = ComponentGetValue(&world->c_tag_enemy,  id);
            fan_rect_f32   zone      = ComponentGetValueOrElse(&world->c_zone, id, (fan_rect_f32){ 0 });

            if (fan_rect_f32_isempty(zone)) {
                zone = (fan_rect_f32) {
                    .x = transform->position.x,
                    .y = transform->position.y,
                    .w = transform->scale.x,
                    .h = transform->scale.y,
                };
            }
            else {
                zone.x += transform->position.x;
                zone.y += transform->position.y;
            }

            if (move->active and not (move->flags & MovementFlag_NoCollision)) {
                for (ssize j = i + 1; j < split->dynamic_count; j++) {
                    ssize other_id = split->dynamic_entities[j];

                    CTransform    *other_transform  = ComponentGetFast(&world->c_transform,   other_id);
                    CMovement     *other_move       = ComponentGetFast(&world->c_movement,    other_id);
                    bool32        *other_interacted = ComponentGet(&world->c_interactable,    other_id);
                    fan_rect_f32   other_zone       = ComponentGetValueOrElse(&world->c_zone, other_id, (fan_rect_f32) { 0 });
                    ssize          other_tag_enemy  = ComponentGetValue(&world->c_tag_enemy,  other_id);

                    if (fan_rect_f32_isempty(other_zone)) {
                        other_zone = (fan_rect_f32) {
                            .x = other_transform->position.x,
                            .y = other_transform->position.y,
                            .w = other_transform->scale.x,
                            .h = other_transform->scale.y,
                        };
                    }
                    else {
                        other_zone.x += other_transform->position.x;
                        other_zone.y += other_transform->position.y;
                    }

                    if (attack) {
                        if (id == world->spec_id.player and state->player_input.actions[0] and not attack->attacking) {
                            attack->attacking = true;
                            attack->cast_timer = 0.2f;
                        }
                        if (move->lock_time <= 0.0f) {
                            AttackSystem(attack, move, transform, other_move, other_transform, fixed_dt);
                        }
                    }

                    if (magic) {
                        // int32 magic_type = MagicType_None;
                        // bool32 casting = false;
                        // if (state->player_input.actions[5]) {
                        //     magic_type = MagicType_Mana;
                        //     state->player_input.actions[5] = 0;
                        // }
                        // else if (state->player_input.actions[6]) {
                        //     magic_type = MagicType_Energy;
                        //     state->player_input.actions[6] = 0;
                        // }
                        // else if (state->player_input.actions[7]) {
                        //     magic_type = MagicType_Soul;
                        //     state->player_input.actions[7] = 0;
                        // }
                        // else if (state->player_input.actions[2]) {
                            // casting = true;
                            // state->player_input.actions[2] = 0;
                            // SpawnBullet(world, transform->position, state->player_input.direction);
                        // }
                        // MagicSystem(magic, magic_type, casting, fixed_dt);
                    }

                    CollisionSystem(transform, move, other_transform, other_move, fixed_dt);
                    if (
                            interact and
                            other_interacted and
                            CollisionCheckR(zone, other_zone)
                            // and not (istagged(tag_enemy) and istagged(other_tag_enemy))
                        ) {
                        // FanRectInt32Print(zone);
                        // FanRectInt32Print(other_zone);
                        *interact = true;
                        *other_interacted = true;

                    }
                }
                for (ssize i = 0; i < split->static_count; i++) {
                    ssize other_id = split->static_entities[i];

                    CTransform     *other_transform  = ComponentGetFast(&world->c_transform,   other_id);
                    CMovement      *other_move       = ComponentGetFast(&world->c_movement,    other_id);
                    bool32         *other_interacted = ComponentGet(&world->c_interactable,    other_id);
                    fan_rect_f32    other_zone       = ComponentGetValueOrElse(&world->c_zone, other_id, (fan_rect_f32) { 0 });
                    ssize           other_tag_enemy  = ComponentGetValue(&world->c_tag_enemy,  other_id);

                    if (fan_rect_f32_isempty(other_zone)) {
                        other_zone = (fan_rect_f32) {
                            .x = other_transform->position.x,
                            .y = other_transform->position.y,
                            .w = other_transform->scale.x,
                            .h = other_transform->scale.y,
                        };
                    }
                    else {
                        other_zone.x += other_transform->position.x;
                        other_zone.y += other_transform->position.y;
                    }

                    if (attack) {
                        if (id == world->spec_id.player and state->player_input.actions[0]) {
                            attack->attacking = true;
                        }
                        AttackSystem(attack, move, transform, other_move, other_transform, fixed_dt);
                    }

                    CollisionSystem(transform, move, other_transform, other_move, fixed_dt);
                    if (
                            interact and
                            other_interacted and
                            CollisionCheckR(zone, other_zone) and
                            not (istagged(tag_enemy) and istagged(other_tag_enemy))
                        ) {
                        // FanRectInt32Print(zone);
                        // FanRectInt32Print(other_zone);
                        *interact = true;
                        *other_interacted = true;

                    }
                }
            }

            if (last_iter and state->player_called_object_dump) {
                printf("\tid: %td\n", id);

                if (id == world->spec_id.player) {
                    printf("\t");
                    fan_vec2_print(transform->position);
                    printf("\t");
                    fan_vec2_print(transform->scale);

                    if (move) {
                        printf("\t");
                        fan_vec2_print(move->velocity_input);
                        printf("\t");
                        fan_vec2_print(move->direction);
                        printf("\tmove->speed: %f\n", (float64)move->speed);
                    }
                    printf("\t");
                    fan_rect_print(zone);
                }
            }
        }
    }

    for (ssize i = 0; i < world->c_animation.size; i++) {
        ssize id = world->c_animation.dense[i];
        if (id == -1)
            continue;

        CAnimation *anim    = ComponentGet(&world->c_animation, id);
        CTexture   *texture = ComponentGet(&world->c_texture,   id);
        CMovement  *move    = ComponentGet(&world->c_movement,  id);
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
        0,                           0, citizen_size.x, citizen_size.y
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
        0,                           (2.0f * citizen_size.y), citizen_size.x, citizen_size.y
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
        0,                           citizen_size.y, citizen_size.x, citizen_size.y
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
        0,                           (3.0f * citizen_size.y), citizen_size.x, citizen_size.y
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
        0,                           (4.0f * citizen_size.y), citizen_size.x, citizen_size.y
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
        0,                           (6.0f * citizen_size.y), citizen_size.x, citizen_size.y
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
        0,                           (5.0f * citizen_size.y), citizen_size.x, citizen_size.y
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
        0,                           (7.0f * citizen_size.y), citizen_size.x, citizen_size.y
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

void GameInit(fan_allocator *a, World *world, GameState *state) {
    fan_fps_target(60);

    StateGetView(state);

    ssize split_size      = kilobytes(1);
    ssize component_size  = kilobytes(1);

    ComponentCreate(&world->c_transform,      a, component_size);
    ComponentCreate(&world->c_shape,          a, component_size);
    ComponentCreate(&world->c_movement,       a, component_size);
    ComponentCreate(&world->c_texture,        a, component_size);
    ComponentCreate(&world->c_behavior,       a, component_size);
    ComponentCreate(&world->c_animation,      a, component_size);
    ComponentCreate(&world->c_physics,        a, component_size);
    ComponentCreate(&world->c_sound,          a, component_size);
    ComponentCreate(&world->c_light,          a, component_size);

    ComponentCreate(&world->c_interaction,    a, component_size);
    ComponentCreate(&world->c_interactable,   a, component_size);
    ComponentCreate(&world->c_zone,           a, component_size);
    ComponentCreate(&world->c_attack,         a, component_size);
    ComponentCreate(&world->c_magic,          a, component_size);

    ComponentCreate(&world->c_tag_background, a, component_size);
    ComponentCreate(&world->c_tag_enemy,      a, component_size);

    world->split.dynamic_entities = a->make(a->ctx, split_size);
    world->split.dynamic_capacity = split_size;

    world->split.static_entities  = a->make(a->ctx, split_size);
    world->split.static_capacity  = split_size;

    world->tilesets = a->make(a->ctx, sizeof(fan_texture) * 2);

	int32 map_size_x = 8;
    int32 map_size_y = 8;
    TileMap map = (TileMap) {
        .tile_size    = 16,
		.logic_tiles  = fan_matrix_create(a, map_size_x, map_size_y),
        .visual_tiles = fan_matrix_create(a, map_size_x, map_size_y)
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

    printf("Successfully passed initialization!\n");
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

    UpdateEntities(world, state, dt);
    RenderEntities(world, state, dt);
}

void GameClose(fan_allocator *a, World *world, GameState *state) {
    (void)a;
    for (ssize i = 0; i < world->c_texture.size; i++) {
        if (world->c_texture.dense[i] == -1) {
            continue;
        }
        fan_texture_unload(world->c_texture.data[i].texture);
    }
    for (ssize i = 0; i < world->c_sound.size; i++) {
        if (world->c_texture.dense[i] == -1)
            continue;
        fan_sound_unload(world->c_sound.data[i].sound);
    }
    fan_rtexture_unload(state->rendermap);
    fan_rtexture_unload(state->lightmap);
    fan_rtexture_unload(state->tilemap);
    fan_music_unload(state->music);
}
