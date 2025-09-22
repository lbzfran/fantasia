
#include "game.h"
#include "os.h"
#include "platform.h"

fan_vec2 WorldToScreen(
    fan_vec2 world_coord,
    fan_vec2 camera_position,
    float32 camera_zoom,
    int32 pixels_per_unit,
    fan_vec2 render_size
) {
    fan_vec2 camera_coord = fan_vec2_sub(world_coord, camera_position);
    fan_vec2 px_coord     = fan_vec2_scale(camera_coord, (float32)pixels_per_unit * camera_zoom);

    fan_vec2 screen_coord = (fan_vec2) {
        (render_size.x / 2.0f) + px_coord.x,
        (render_size.y / 2.0f) - px_coord.y,
    };
    return screen_coord;
}

fan_vec2 ScreenToWorld(
    fan_vec2 screen_coord,
    fan_vec2 camera_position,
    float32 camera_zoom,
    int32 pixels_per_unit,
    fan_vec2 render_size
) {
    fan_vec2 centered_coord = (fan_vec2) {
        screen_coord.x - (render_size.x / 2.0f),
        (render_size.y / 2.0f) - screen_coord.y
    };

    fan_vec2 local_coord = fan_vec2_scale(centered_coord, 1.0f / ((float32)pixels_per_unit * camera_zoom));
    fan_vec2 world_coord = fan_vec2_add(camera_position, local_coord);

    return world_coord;
}

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
        velocity = m->velocity_force;
        float32 damp_factor = 0.98f;
        m->velocity_force = fan_vec2_scale(m->velocity_force, damp_factor);
        m->lock_time = 0.15f;
    }
    else {
        m->velocity_input = fan_vec2_zero();
        if (m->lock_time > 0.0f) {
            m->lock_time = max(m->lock_time - dt, 0.0f);
        }
        else if (fan_vec2_length(direction) > 0.0f) {
            m->direction       = direction;
            direction          = fan_vec2_normalize(direction);
            m->velocity_input  = fan_vec2_scale(direction, m->speed);
        }
        velocity = m->velocity_input;
    }

    t->position = fan_vec2_add(t->position, fan_vec2_scale(velocity, dt));

    // float32 damp_factor = 0.9f;
    // m->velocity_force = fan_vec2_scale(m->velocity_force, damp_factor);
    // m->velocity_force = FanVector2AddValue(m->velocity_force, -decay * dt);

    if (not (m->flags & MovementFlag_NoCollision)) {
        if (fan_vec2_length(t->scale) > 0.0f) {
            bound_zone.width  = (int32)((float32)bound_zone.width  - t->scale.x);
            bound_zone.height = (int32)((float32)bound_zone.height - t->scale.y);
        }

        float32 overlap;
        float32 softness = 0.005f;
        if (t->position.x < bound_zone.x) {
            overlap = (float32)bound_zone.x - t->position.x;
            t->position.x += overlap * softness;
        }
        else if (t->position.x > bound_zone.width) {
            overlap = t->position.x - (float32)bound_zone.width;
            t->position.x -= overlap * softness;
        }

        if (t->position.y < bound_zone.y) {
            overlap = (float32)bound_zone.y - t->position.y;
            t->position.y += overlap * softness;
        }
        else if (t->position.y > bound_zone.height) {
            overlap = t->position.y - (float32)bound_zone.height;
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

    result = not (a.x + a.width  < b.x or b.x + b.width  < a.x or
                  a.y + a.height < b.y or b.y + b.height < a.y);

    return result;
}

bool32 CollisionCheckV(fan_vec2 aPos, fan_vec2 aSize, fan_vec2 bPos, fan_vec2 bSize) {
    bool32 result = false;

    // AABB
    result = not (aPos.x + aSize.x < bPos.x or bPos.x + bSize.x < aPos.x or
                  aPos.y + aSize.y < bPos.y or bPos.y + bSize.y < aPos.y);

    return result;
}

/*
 * MATRIX
 */
int32 MatrixInt32Get_(MatrixInt32 m, ssize i, ssize j) {
    ssize idx = i * m.cols + j;
    return m.V[idx];
}
#define MatrixInt32Get(m, i, j) MatrixInt32Get_(m, max((ssize)i, (ssize)(m.rows - 1)), max((ssize)j, (ssize)(m.cols - 1)))

MatrixInt32 MatrixInt32Create(Allocator *a, ssize rows, ssize cols, int32 default_value) {
    ssize size = rows * cols;
    int32 *data = (int32 *)a->make(a->ctx, size * sizeof(int32));
    for (ssize i = 0; i < size; i++)
        data[i] = default_value;
    return (MatrixInt32) {
        .V = data,
        .rows = rows,
        .cols = cols
    };
}

fan_vec2 TileMapGetPosition(TileMap map, fan_vec2 position) {
    int32 mapX = fan_f32_truncate((position.x - map.origin.x) / (float32)map.tile_size);
    int32 mapY = fan_f32_truncate((position.y - map.origin.y) / (float32)map.tile_size);

    fan_vec2 tile_pos = (fan_vec2) {
        (float32)mapX,
        (float32)mapY
    };
    return tile_pos;
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

void TextureUpdate(CTexture *tx, fan_vec2 pos, fan_vec2 size, float dt) {
    (void)dt;

    tx->rect = (fan_rect){
        .x      = (int32)pos.x,
        .y      = (int32)pos.y,
        .width  = (int32)size.x,
        .height = (int32)size.y
    };
}

inline CAnimation AnimationApply_(AnimationData *table, int32 new_id, int32 flags, AnimationRequest ar) {
    assert(new_id != -1 && "Out of Bounds Access!");
    CAnimation new_state = (CAnimation) {
        .id = new_id,
        .name = table[new_id].name,
        .flags = flags,
        .request = ar
    };
    return new_state;
}

typedef enum {
    AnimationFlag_NotInterruptible = (1 << 0),
    AnimationFlag_DisableLoop      = (1 << 1),
} AnimationFlags;
/*
 * type: System
 * components: CAnimation, CTexture
 */
void AnimationSystem(CAnimation *a, CTexture *t, AnimationData *table, float dt) {
    if (table is null or a is null or t is null) return;
    if (a->finished or
        (not a->finished and a->request.id != -1 and (a->flags & AnimationFlag_NotInterruptible) == false)) {
        *a = AnimationApply_(table, a->request.id, a->request.flags, (AnimationRequest){ -1, 0 });
    }

    AnimationData *data = &table[a->id];
    if (data->frame_count <= 0) {
        return;
    }

    a->timer += dt;
    // printf("a->timer: %f\n", a->timer);
    // printf("data->frame_time: %f\n", data->frame_time);
    if (a->timer >= data->frame_time) {
        a->timer -= data->frame_time;
        a->current_frame++;

        if (a->current_frame >= data->frame_count) {
            if (data->loop and (a->flags & AnimationFlag_DisableLoop) == false) {
                a->current_frame = 0;
            }
            else if (data->next_id != -1) {
                *a = AnimationApply_(table, data->next_id, a->flags, a->request);
            }
            else {
                a->finished = true;
                a->current_frame = data->frame_count - 1;
            }
        }
    }

    if (a->current_frame >= data->frame_count) {
        a->current_frame = data->frame_count - 1;
    }

    fan_rect current = data->frames[a->current_frame];
    TextureUpdate(
        t,
        (fan_vec2){ (float32)current.x, (float32)current.y },
        (fan_vec2){
            coalesce((float32)current.width,  (float32)t->rect.width),
            coalesce((float32)current.height, (float32)t->rect.height)
        },
        dt
    );
}

void LightSystem(
        CLight       *l,
        CTransform   *t,
        CShape       *s,
        fan_rtexture  lightmap,
        fan_vec2      camera_pos,
        float32       camera_zoom,
        int32         pixels_per_unit,
        fan_vec2      render_size,
        float32       dt
    ) {
    (void)dt;

    fan_vec2 center_pos = fan_vec2_add(t->position, (fan_vec2){
        t->scale.x *   0.5f,
        t->scale.y *  -0.5f
    });
    fan_vec2 screen_pos = WorldToScreen(
        center_pos,
        camera_pos,
        camera_zoom,
        pixels_per_unit,
        render_size
    );

    fan_draw_circle(
        (int32)fan_f32_round(screen_pos.x),
        (int32)fan_f32_round(screen_pos.y),
        l->radius,
        l->color
    );
}

void LightingProcessPost(
    fan_rtexture lightmap,
    int32 window_width,
    int32 window_height
) {
    fan_mode_blend_begin(FanBlend_MULTIPLIED);

        fan_draw_texture(
            lightmap.texture,
            (fan_rect){ 0, 0, (float32)lightmap.texture.width, (float32)-lightmap.texture.height },
            (fan_rect){ 0, 0, (float32)window_width, (float32)window_height },
            fan_vec2_zero(),
            0.0f,
            fan_color_WHITE
        );

    fan_mode_blend_end();
}

typedef enum {
    RenderFlag_FlipX        = (1 << 0),
    RenderFlag_FlipY        = (1 << 1),
    RenderFlag_ShowInteract = (1 << 2)
} RenderFlags;
/*
 * type: System
 * component(s): Transform, Shape, Texture (opt), Physics (opt)
 */
void RenderSystem(
        CShape *s,
	    CTransform *t,
	    CTexture *tx,
	    CMovement *m,
	    bool32 interacting,
	    fan_rect_f32 zone,
        fan_vec2 camera_position,
        float32 camera_zoom,
        int32 pixels_per_unit,
        fan_vec2 render_size,
	    int32 flags
    ) {
    fan_vec2 screen_pos    = WorldToScreen(
        t->position,
        camera_position,
        camera_zoom,
        pixels_per_unit,
        render_size
    );
    fan_vec2 screen_offset = fan_vec2_scale(s->offset, (float32)pixels_per_unit * camera_zoom);
    fan_vec2 screen_scale  = fan_vec2_scale(t->scale,  (float32)pixels_per_unit * camera_zoom);
    if (tx is null) {
        if (fan_vec2_length(s->offset) > 0.0f) {
            fan_draw_rectv(fan_vec2_add(screen_pos, screen_offset), screen_scale, (fan_color){ 50, 50, 50, 255 });
        }
        fan_draw_rectv(screen_pos, screen_scale, s->color);
    }
    else {
        float32 width  = (tx->rect.width)  ? (float32)tx->rect.width  : (float32)tx->texture.width;
        float32 height = (tx->rect.height) ? (float32)tx->rect.height : (float32)tx->texture.height;

        if (m) {
            if (flags & RenderFlag_FlipX) {
                width  *= m->direction.x ? m->direction.x : 1.0f;
            }
            if (flags & RenderFlag_FlipY) {
                height *= m->direction.y ? m->direction.y : 1.0f;
            }
        }

        fan_rect src = (fan_rect) {
            tx->rect.x,
            tx->rect.y,
            width,
            height
        };

        fan_rect dst = (fan_rect) {
            fan_f32_round(screen_pos.x + screen_offset.x),
            fan_f32_round(screen_pos.y + screen_offset.y),
            screen_scale.x,
            screen_scale.y
        };

        if (flags & RenderFlag_ShowInteract) {
            fan_vec2 screen_zone_pos = WorldToScreen(
                (fan_vec2){ zone.x, zone.y },
                camera_position,
                camera_zoom,
                pixels_per_unit,
                render_size
            );
            fan_rect screen_zone = (fan_rect) {
                screen_zone_pos.x,
                screen_zone_pos.y,
                (float32)zone.width  * (float32)pixels_per_unit * camera_zoom,
                (float32)zone.height * (float32)pixels_per_unit * camera_zoom
            };
            fan_color zone_color = interacting ?
                (fan_color){ 255, 0, 0, 75 } : (fan_color){ 0, 255, 0, 75 };

            fan_draw_rectr(screen_zone, zone_color);
        }

        fan_draw_texture(
            tx->texture,
            src,
            dst,
            (fan_vec2) { 0.0f, 0.0f },
            0.0f,
            s->color
        );
    }
}

void RenderProcessPost(
    fan_rtexture map,
    int32 window_width,
    int32 window_height
) {
    fan_draw_texture(
        map.texture,
        (fan_rect){ 0, 0, (float32)map.texture.width, (float32)-map.texture.height },
        (fan_rect){ 0, 0, (float32)window_width, (float32)window_height },
        fan_vec2_zero(),
        0.0f,
        fan_color_WHITE
    );
}

typedef struct RenderEntry {
    int32   id;
    float32 height;
    int32   layer;
} RenderEntry;

ssize SortRenderPartition_(RenderEntry *entries, ssize low, ssize high) {
    RenderEntry pivot = entries[high];
    RenderEntry temp;

    ssize i = low - 1;

    for (ssize j = low; j <= high - 1; j++) {
        if ((entries[j].layer < pivot.layer) or \
            (entries[j].layer == pivot.layer and entries[j].height < pivot.height)) {
            i++;
            temp = entries[j];
            entries[j] = entries[i];
            entries[i] = temp;
        }
    }

    temp = entries[i + 1];
    entries[i + 1] = entries[high];
    entries[high] = temp;

    return i + 1;
}

void SortRender(RenderEntry *entries, ssize low, ssize high) {
    // qsort in-place
    if (low < high) {
        ssize pi = SortRenderPartition_(entries, low, high);

        SortRender(entries, low, pi - 1);
        SortRender(entries, pi + 1, high);
    }
}

void RenderEntities(World *world, GameState *state, float32 dt) {
    (void)dt;
    RenderEntry render_array[128] = { { -1, 0.0f, 0 } };
    ssize render_entry_count = 0;

    fan_vec2 camera_position = world->c_transform.data[world->spec_id.camera].position;
    float32 camera_zoom = state->camera_zoom;
    int32 pixels_per_unit = state->world_scale;

    for (ssize i = 0; i < world->c_shape.size; i++) {
        ssize id = world->c_shape.dense[i];
        if (id == -1)
            continue;
        CShape *shape = &world->c_shape.data[i];

        ssize transform_idx = world->c_transform.sparse[id];
        if (transform_idx == -1)
            continue;
        CTransform *transform = &world->c_transform.data[transform_idx];

        if (not shape->initialized) {
            if (shape->color.a == 0) {
                shape->color = fan_color_WHITE;
            }
            init_if_null(shape->layer, 2);

            shape->visible = true;
            shape->initialized = true;
        }

        render_array[render_entry_count++] = (RenderEntry) {
            (int32)id,
            transform->position.y + transform->scale.y,
            shape->layer
        };
    }

    SortRender(render_array, 0, world->c_shape.size - 1);

    fan_mode_texture_begin(state->rendermap);
        fan_draw_clear(fan_color_WHITE);
        for (ssize i = 0; i < render_entry_count; i++) {
            ssize id = render_array[i].id;
            if (id == -1)
                continue;

            ssize shape_idx        = world->c_shape.sparse[id];
            ssize transform_idx    = world->c_transform.sparse[id];
            ssize move_idx         = world->c_movement.sparse[id];
            ssize animation_idx    = world->c_animation.sparse[id];
            ssize texture_idx      = world->c_texture.sparse[id];
            ssize attack_idx       = world->c_attack.sparse[id];

            ssize tag_bg           = world->c_tag_background.sparse[id];
            (void)tag_bg;

            ssize interaction_idx  = world->c_interaction.sparse[id];
            ssize interactable_idx = world->c_interactable.sparse[id];
            ssize zone_idx         = world->c_zone.sparse[id];

            CShape         *shape       = &world->c_shape.data[shape_idx];
            CTransform     *transform   = &world->c_transform.data[transform_idx];
            CMovement      *move        = &world->c_movement.data[move_idx];
            CTexture       *texture     = null;
            // CAnimation     *animation   = null;
            CAttack        *attack      = &world->c_attack.data[attack_idx];
            bool32          interacting = false;
            bool32          interacted  = false;
            fan_rect        zone        = { 0 };

            if (not shape->visible) {
                continue;
            }

            int32 render_flags = 0;

            if (zone_idx != -1) {
                zone = world->c_zone.data[zone_idx];
                zone.x += transform->position.x;
                zone.y += transform->position.y;
            }
            else {
                zone = (fan_rect) {
                    .x      = transform->position.x,
                    .y      = transform->position.y,
                    .width  = transform->scale.x,
                    .height = transform->scale.y,
                };
            }

            if (interaction_idx != -1) {
                interacting = world->c_interaction.data[interaction_idx];
            }

            if (interactable_idx != -1) {
                interacted  = world->c_interactable.data[interactable_idx];
            }

            if (texture_idx != -1) {
                texture       = &world->c_texture.data[texture_idx];
                // if (animation_idx != -1) {
                //     animation = &world->c_animation.data[animation_idx];
                // }
            }

            if (move_idx != -1) {
                move = &world->c_movement.data[move_idx];
            }

            fan_vec2 center_pos = fan_vec2_add(
                transform->position,
                (fan_vec2) {
                    transform->scale.x * 0.5f,
                    transform->scale.y * 0.5f,
                }
            );

            fan_vec2 map_pos = TileMapGetPosition(world->map, center_pos);
            int32 tile_data = MatrixInt32Get(world->map.tiles, map_pos.x, map_pos.y);

            if (id == world->spec_id.player) {
                // fan_vec2 tile_world_pos = (fan_vec2) {
                //     map_pos.x * world->map.tile_size,
                //     map_pos.y * world->map.tile_size
                // };
                //
                // fan_vec2 screen_pos = WorldToScreen(tile_world_pos, camera_position, pixels_per_unit);
                // int32 screen_size     = world->map.tile_size * pixels_per_unit;

                if (tile_data == 1) {
                    // fan_draw_rectr(
                    //     (fan_rect_i32) {
                    //         screen_pos.x,
                    //         screen_pos.y,
                    //         screen_size,
                    //         screen_size,
                    //     },
                    //     FanColor_BLUE
                    // );
                }
            }

            if (animation_idx == -1)
                render_flags |= RenderFlag_FlipX;

            if (state->p_input.actions[1])
                render_flags |= RenderFlag_ShowInteract;

            RenderSystem(
                shape,
                transform,
                texture,
                move,
                interacting,
                zone,
                camera_position,
                camera_zoom,
                pixels_per_unit,
                state->render_size,
                render_flags
            );


            if (attack_idx != -1 and attack->attacking and attack->cast_timer <= 0.0f) {
                fan_vec2 center = fan_vec2_add(transform->position,
                        (fan_vec2){ transform->scale.x * 0.5f, transform->scale.y * -0.5f });
                fan_vec2 facing = move->direction;
                float32 length    = attack->attack_range;
                float32 angle     = attack->arc_angle;

                fan_vec2 prev = WorldToScreen(
                    center,
                    camera_position,
                    camera_zoom,
                    pixels_per_unit,
                    state->render_size
                );

                // Compute start and end angles
                float32 progress = attack->timer / attack->swing_time;
                int32 segments = 20;

                // Precompute start and end directions by rotating facing
                fan_vec2 start_dir = fan_vec2_rotate(facing, -angle * 0.5f);
                fan_vec2 end_dir   = fan_vec2_rotate(facing,  angle * 0.5f);

                for (int32 i = 0; i <= segments; i++) {
                    float32 t_seg = (float32)i / (float32)segments;

                    // Interpolate between start and end directions
                    fan_vec2 sweep_dir = fan_vec2_normalize(fan_vec2_lerp(start_dir, t_seg, end_dir));

                    fan_vec2 world_point = {
                        center.x + sweep_dir.x * length,
                        center.y + sweep_dir.y * length
                    };
                    fan_vec2 screen_point = WorldToScreen(
                        world_point,
                        camera_position,
                        camera_zoom,
                        pixels_per_unit,
                        state->render_size
                    );

                    // fan_vec2_print(sweep_dir);
                    // fan_vec2_print(screen_point);

                    fan_draw_linev(prev, screen_point, fan_color_RED);
                    prev = screen_point;
                }

                // Draw line from center to current sweep tip
                fan_vec2 sweep_tip = fan_vec2_normalize(fan_vec2_lerp(start_dir, progress, end_dir));
                fan_vec2 world_sweep_tip = {
                    center.x + sweep_tip.x * length,
                    center.y + sweep_tip.y * length
                };
                fan_vec2 screen_sweep_tip = WorldToScreen(
                    world_sweep_tip,
                    camera_position,
                    camera_zoom,
                    pixels_per_unit,
                    state->render_size
                );
                fan_draw_linev(
                    WorldToScreen(
                        center,
                        camera_position,
                        camera_zoom,
                        pixels_per_unit,
                        state->render_size
                    ),
                    screen_sweep_tip,
                    fan_color_RED
                );
            }

            if (state->called_object_dump) {
                printf("id: %td\n", id);
                printf("interacting: %s\n", interacting ? "true" : "false");
                printf("interacted: %s\n",  interacted  ? "true" : "false");
                fan_rect_print(zone);
                printf("\t");
                fan_vec2_print(transform->position);
                printf("\t");
                fan_vec2_print(transform->scale);
            }
        }
    fan_mode_texture_end();

    RenderProcessPost(state->rendermap, state->window_width, state->window_height);
    fan_color ambient = { 30, 30, 30, 255 };
    fan_mode_texture_begin(state->lightmap);
        fan_draw_clear(ambient);
        for (ssize i = 0; i < world->c_light.size; i++) {
            ssize id = world->c_light.dense[i];

            ssize transform_idx   = world->c_transform.sparse[id];
            ssize shape_idx       = world->c_shape.sparse[id];

            CLight       *light     = &world->c_light.data[i];
            CTransform   *transform = &world->c_transform.data[transform_idx];
            CShape       *shape     = &world->c_shape.data[shape_idx];

            LightSystem(
                light,
                transform,
                shape,
                state->lightmap,
                camera_position,
                camera_zoom,
                pixels_per_unit,
                state->render_size,
                dt
            );
        }
    fan_mode_texture_end();

    LightingProcessPost(state->lightmap, state->window_width, state->window_height);
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
    }

    accumulator += dt;
    bool32 last_iter = false;
    while (accumulator >= fixed_dt) {
        accumulator -= fixed_dt;
        if (accumulator < fixed_dt)
            last_iter = true;
        for (ssize i = 0; i < world->c_movement.size; i++) {
            ssize id = world->c_movement.dense[i];
            if (id == -1)
                continue;

            ssize transform_idx   = world->c_transform.sparse[id];
            ssize behavior_idx    = world->c_behavior.sparse[id];
            ssize interact_idx    = world->c_interaction.sparse[id];
            ssize interacted_idx  = world->c_interactable.sparse[id];
            ssize animation_idx   = world->c_animation.sparse[id];
            ssize attack_idx      = world->c_attack.sparse[id];
            // int32 zone_idx        = world->c_zone.sparse[id];

            CMovement       *move      = &world->c_movement.data[i];
            CTransform      *transform = &world->c_transform.data[transform_idx];
            CBehavior       *behavior  = null;
            CAttack         *attack    = null;
            CAnimation      *anim      = null;
            // fan_rect_f32  *zone      = null;

            fan_vec2 direction = fan_vec2_zero();

            // if (zone_idx != -1) {
            //     zone = &world->c_zone.data[zone_idx];
            // }

            if (interact_idx != -1) {
                world->c_interaction.data[interact_idx] = false;
            }

            if (interacted_idx != -1) {
                world->c_interactable.data[interacted_idx] = false;
            }

            if (attack_idx != -1) {
                attack = &world->c_attack.data[attack_idx];
            }

            if (id == world->spec_id.player) {
                if ((attack is null) or (attack and not attack->attacking)) {
                    direction = state->p_input.direction;

                    if (animation_idx != -1) {
                        anim = &world->c_animation.data[animation_idx];

                        if (direction.x > 0.0f) {
                            anim->request.id = 1;
                        }
                        else if (direction.x < 0.0f) {
                            anim->request.id = 2;
                        }

                        if (direction.y > 0.0f) {
                            anim->request.id = 3;
                        }
                        else if (direction.y < 0.0f) {
                            anim->request.id = 0;
                        }
                    }
                }
            }
            else if (behavior_idx != -1) {
                behavior = &world->c_behavior.data[behavior_idx];

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
                            CTransform *target_transform = &world->c_transform.data[world->spec_id.player];
                            fan_vec2 target_face         = target_transform->position;

                            if (id == world->spec_id.camera) {
                                // CShape *target_shape = &world->c_shape.data[world->spec_id.player];
                                fan_vec2 target_offset = target_transform->scale;
                                target_offset.y *= -1.0f;
                                target_offset = fan_vec2_scale(target_offset, 0.5f);
                                target_face = fan_vec2_add(target_face, target_offset);

                                float32 lerp_factor = 0.001f;
                                transform->position = fan_vec2_lerp(transform->position, lerp_factor, target_face);
                            }
                            else {
                                fan_vec2 face = fan_vec2_normalize(fan_vec2_sub(target_face, transform->position));
                                direction = (fan_vec2){ signof(face.x), signof(face.y) };
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

            MovementSystem(move, transform, direction, state->bound_zone, fixed_dt);

            if (last_iter and state->called_object_dump) {
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

            ssize move_idx        = world->c_movement.sparse[id];
            ssize transform_idx   = world->c_transform.sparse[id];
            ssize interact_idx    = world->c_interaction.sparse[id];
            ssize zone_idx        = world->c_zone.sparse[id];
            ssize attack_idx      = world->c_attack.sparse[id];

            ssize tag_enemy       = world->c_tag_enemy.sparse[id];

            CMovement      *move      = &world->c_movement.data[move_idx];
            CTransform     *transform = &world->c_transform.data[transform_idx];
            CAttack        *attack    = &world->c_attack.data[attack_idx];
            bool32         *interact  = null;
            fan_rect_f32  zone      = (fan_rect_f32) { 0 };

            if (interact_idx != -1) {
                interact = &world->c_interaction.data[interact_idx];
            }

            if (zone_idx != -1) {
                zone = world->c_zone.data[zone_idx];
                zone.x += transform->position.x;
                zone.y += transform->position.y;
            }
            else {
                zone = (fan_rect_f32) {
                    .x      = transform->position.x,
                    .y      = transform->position.y,
                    .width  = transform->scale.x,
                    .height = transform->scale.y,
                };
            }

            if (move->active and not (move->flags & MovementFlag_NoCollision)) {
                for (ssize j = i + 1; j < split->dynamic_count; j++) {
                    ssize other_id = split->dynamic_entities[j];

                    ssize other_transform_idx    = world->c_transform.sparse[other_id];
                    ssize other_move_idx         = world->c_movement.sparse[other_id];
                    assert(other_move_idx != -1);
                    ssize other_interacted_idx   = world->c_interactable.sparse[other_id];
                    ssize other_zone_idx         = world->c_zone.sparse[other_id];

                    ssize other_tag_enemy        = world->c_tag_enemy.sparse[other_id];

                    CTransform     *other_transform  = &world->c_transform.data[other_transform_idx];
                    CMovement      *other_move       = &world->c_movement.data[other_move_idx];
                    bool32         *other_interacted = null;
                    fan_rect_f32  other_zone       = (fan_rect_f32) { 0 };

                    if (other_interacted_idx != -1) {
                        other_interacted = &world->c_interactable.data[other_interacted_idx];
                    }

                    if (other_zone_idx != -1) {
                        other_zone = world->c_zone.data[other_zone_idx];
                        other_zone.x += other_transform->position.x;
                        other_zone.y += other_transform->position.y;
                    }
                    else {
                        other_zone = (fan_rect_f32) {
                            .x      = other_transform->position.x,
                            .y      = other_transform->position.y,
                            .width  = other_transform->scale.x,
                            .height = other_transform->scale.y,
                        };
                    }

                    if (attack_idx != -1) {
                        if (id == world->spec_id.player and state->p_input.actions[0] and not attack->attacking) {
                            attack->attacking = true;
                            attack->cast_timer = 0.5f;
                        }
                        if (move->lock_time <= 0.0f) {
                            AttackSystem(attack, move, transform, other_move, other_transform, fixed_dt);
                        }
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
                for (ssize i = 0; i < split->static_count; i++) {
                    ssize other_id = split->static_entities[i];

                    ssize other_transform_idx    = world->c_transform.sparse[other_id];
                    ssize other_move_idx         = world->c_movement.sparse[other_id];
                    assert(other_move_idx != -1);
                    ssize other_interacted_idx   = world->c_interactable.sparse[other_id];
                    ssize other_zone_idx         = world->c_zone.sparse[other_id];

                    ssize other_tag_enemy        = world->c_tag_enemy.sparse[other_id];

                    CTransform     *other_transform  = &world->c_transform.data[other_transform_idx];
                    CMovement      *other_move       = &world->c_movement.data[other_move_idx];
                    bool32         *other_interacted = null;
                    fan_rect_f32  other_zone       = (fan_rect_f32) { 0 };

                    if (other_interacted_idx != -1) {
                        other_interacted = &world->c_interactable.data[other_interacted_idx];
                    }

                    if (other_zone_idx != -1) {
                        other_zone = world->c_zone.data[other_zone_idx];
                        other_zone.x += other_transform->position.x;
                        other_zone.y += other_transform->position.y;
                    }
                    else {
                        other_zone = (fan_rect_f32) {
                            .x      = other_transform->position.x,
                            .y      = other_transform->position.y,
                            .width  = other_transform->scale.x,
                            .height = other_transform->scale.y,
                        };
                    }

                    if (attack_idx != -1) {
                        if (id == world->spec_id.player and state->p_input.actions[0]) {
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

            if (last_iter and state->called_object_dump) {
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

        ssize texture_idx = world->c_texture.sparse[id];
        ssize move_idx    = world->c_movement.sparse[id];

        CAnimation *anim    = &world->c_animation.data[i];
        CTexture   *texture = &world->c_texture.data[texture_idx];
        CMovement  *move    = &world->c_movement.data[move_idx];
        (void)move;

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

    fan_vec2 render_size = (fan_vec2){ 640, 480 };
    fan_vec2 world_offset = fan_vec2_zero();

    int32 scale_x = (int32)(window_width  / render_size.x);
    int32 scale_y = (int32)(window_height / render_size.y);
    int32 world_scale = TILE_SIZE * min(scale_x, scale_y);

    // float32 offset_x = ((float32)state->window_width  - render_width)  / 2.0f;
    // float32 offset_y = ((float32)state->window_height - render_height)  / 2.0f;
    // render_width  = render_width ;
    // render_height = render_height;

    state->window_width  = (int32)window_width;
    state->window_height = (int32)window_height;
    state->world_scale = world_scale;
    state->render_size = render_size;
}

void SceneSolo(World *world) {
    fan_texture tex_link = fan_texture_load("./resources/link.png");
    // fan_vec2 sprite_link_size = (fan_vec2){ tex_link.width / 10.0f, tex_link.height / 8.0f };
    // player_idle_down_frames[0]  = (fan_rect_i32){ 0,                         0,                         0, 0 };
    // player_idle_down_frames[1]  = (fan_rect_i32){ sprite_link_size.x,        0,                         0, 0 };
    // player_idle_down_frames[2]  = (fan_rect_i32){ 2.0f * sprite_link_size.x, 0,                         0, 0 };
    //
    // player_idle_up_frames[0]    = (fan_rect_i32){ 0,                         2.0f * sprite_link_size.y, 0, 0 };
    //
    // player_idle_left_frames[0]  = (fan_rect_i32){ 0,                         sprite_link_size.y,        0, 0 };
    // player_idle_left_frames[1]  = (fan_rect_i32){ sprite_link_size.x,        sprite_link_size.y,        0, 0 };
    // player_idle_left_frames[2]  = (fan_rect_i32){ 2.0f * sprite_link_size.x, sprite_link_size.y,        0, 0 };
    //
    // player_idle_right_frames[0] = (fan_rect_i32){ 0,                         3.0f * sprite_link_size.y, 0, 0 };
    // player_idle_right_frames[1] = (fan_rect_i32){ sprite_link_size.x,        3.0f * sprite_link_size.y, 0, 0 };
    // player_idle_right_frames[2] = (fan_rect_i32){ 2.0f * sprite_link_size.x, 3.0f * sprite_link_size.y, 0, 0 };


    ComponentAdd(&world->c_transform,     world->entity_count);
    ComponentAdd(&world->c_shape,         world->entity_count);
    ComponentAddArgs(&world->c_movement,  world->entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    ComponentAddArgs(&world->c_texture,   world->entity_count,
        .texture = tex_link,
        .rect    = { 0, 0, (float32)tex_link.width / 10.0f, (float32)tex_link.height / 8.0f }
    );
    // ComponentAddArgs(&world->c_animation, world->entity_count);
    world->spec_id.player = world->entity_count;
    world->entity_count++;

    // ComponentAddArgs(&world->c_transform,  world->entity_count,
    //     .scale = (fan_vec2){ (float32)render_width, (float32)render_height },
    // );
    ComponentAddArgs(&world->c_shape,      world->entity_count,
        .layer = 1,
        .color = (fan_color){ 155, 155, 155, 255 },
    );
    ComponentAddArgs(&world->c_movement,   world->entity_count,
        .flags = MovementFlag_NoCollision,
    );
    ComponentAdd(&world->c_tag_background, world->entity_count);
    world->entity_count++;
}


global void SceneMain(World *world) {
    fan_texture tex_sprite = fan_texture_load("./resources/Sprite-0001.png");

    fan_texture tex_girl = fan_texture_load("./resources/Citizens/Female/Nel/Nel.png");

    // fan_rect_i32 player_idle_up_frames[1]    = { 0 };
    // global fan_rect_i32 player_idle_down_frames[2]  = { 0 };
    // player_idle_down_frames[0] = (fan_rect_i32){ 0, 0,  .width = 32, .height = 32 };
    // player_idle_down_frames[1] = (fan_rect_i32){ 32, 0, .width = 32, .height = 32 };
    // fan_rect_i32 player_idle_left_frames[3]  = { 0 };
    // fan_rect_i32 player_idle_right_frames[3] = { 0 }tex_sprite.height;

    // global AnimationData anim_table[] = {
    //     { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.5f, .frame_count = 2, true, -1 },
        // { "player_idle_up",    player_idle_up_frames,    .frame_time = 1.5f, .frame_count = 1, false,  0 },
        // { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.5f, .frame_count = 3, false,  0 },
        // { "player_idle_right", player_idle_right_frames, .frame_time = 0.5f, .frame_count = 3, false,  0 },
    // };

    // fan_texture tex_link = fan_texture_load("./resources/link.png");
    // fan_vec2 sprite_link_size = (fan_vec2){ (float32)tex_link.width / 10.0f, (float32)tex_link.height / 8.0f };
    // global fan_rect player_idle_down_frames[3]  = { 0 };
    // global fan_rect player_idle_up_frames[1]    = { 0 };
    // global fan_rect player_idle_left_frames[3]  = { 0 };
    // global fan_rect player_idle_right_frames[3] = { 0 };
    // player_idle_down_frames[0]  = (fan_rect){
    //     0,                           0, sprite_link_size.x, sprite_link_size.y
    // };
    // player_idle_down_frames[1]  = (fan_rect){
    //     sprite_link_size.x,          0, sprite_link_size.x, sprite_link_size.y
    // };
    // player_idle_down_frames[2]  = (fan_rect){
    //     (2.0f * sprite_link_size.x), 0, sprite_link_size.x, sprite_link_size.y
    // };
    //
    // player_idle_up_frames[0]    = (fan_rect){
    //     0, (2.0f * sprite_link_size.y), sprite_link_size.x, sprite_link_size.y
    // };
    //
    // player_idle_left_frames[0]  = (fan_rect){
    //     0,                           sprite_link_size.y, sprite_link_size.x, sprite_link_size.y
    // };
    // player_idle_left_frames[1]  = (fan_rect){
    //     sprite_link_size.x,          sprite_link_size.y, sprite_link_size.x, sprite_link_size.y
    // };
    // player_idle_left_frames[2]  = (fan_rect){
    //     (2.0f * sprite_link_size.x), sprite_link_size.y, sprite_link_size.x, sprite_link_size.y
    // };
    //
    // player_idle_right_frames[0] = (fan_rect){
    //     0,                           (3.0f * sprite_link_size.y), sprite_link_size.x, sprite_link_size.y
    // };
    // player_idle_right_frames[1] = (fan_rect){
    //     sprite_link_size.x,          (3.0f * sprite_link_size.y), sprite_link_size.x, sprite_link_size.y
    // };
    // player_idle_right_frames[2] = (fan_rect){
    //     (2.0f * sprite_link_size.x), (3.0f * sprite_link_size.y), sprite_link_size.x, sprite_link_size.y
    // };
    //
    // global AnimationData anim_table[] = {
    //     { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.5f, .frame_count = 3, true,  -1 },
    //     { "player_idle_up",    player_idle_up_frames,    .frame_time = 1.5f, .frame_count = 1, true,  -1 },
    //     { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.5f, .frame_count = 3, true,  -1 },
    //     { "player_idle_right", player_idle_right_frames, .frame_time = 0.5f, .frame_count = 3, true,  -1 },
    // };

    global fan_rect player_idle_down_frames[3]  = { 0 };
    global fan_rect player_idle_up_frames[3]    = { 0 };
    global fan_rect player_idle_left_frames[3]  = { 0 };
    global fan_rect player_idle_right_frames[3] = { 0 };

    fan_vec2 sprite_girl_size = (fan_vec2){ (float32)16.0f, (float32)16.0f };

    player_idle_down_frames[0]  = (fan_rect){
        0,                           0, sprite_girl_size.x, sprite_girl_size.y
    };
    player_idle_down_frames[1]  = (fan_rect){
        sprite_girl_size.x,          0, sprite_girl_size.x, sprite_girl_size.y
    };
    player_idle_down_frames[2]  = (fan_rect){
        (2.0f * sprite_girl_size.x), 0, sprite_girl_size.x, sprite_girl_size.y
    };

    player_idle_down_frames[0]  = (fan_rect){
        0,                           (2.0f * sprite_girl_size.y), sprite_girl_size.x, sprite_girl_size.y
    };
    player_idle_down_frames[1]  = (fan_rect){
        sprite_girl_size.x,          (2.0f * sprite_girl_size.y), sprite_girl_size.x, sprite_girl_size.y
    };
    player_idle_down_frames[2]  = (fan_rect){
        (2.0f * sprite_girl_size.x), (2.0f * sprite_girl_size.y), sprite_girl_size.x, sprite_girl_size.y
    };

    player_idle_left_frames[0]  = (fan_rect){
        0,                           sprite_girl_size.y, sprite_girl_size.x, sprite_girl_size.y
    };
    player_idle_left_frames[1]  = (fan_rect){
        sprite_girl_size.x,          sprite_girl_size.y, sprite_girl_size.x, sprite_girl_size.y
    };
    player_idle_left_frames[2]  = (fan_rect){
        (2.0f * sprite_girl_size.x), sprite_girl_size.y, sprite_girl_size.x, sprite_girl_size.y
    };

    player_idle_right_frames[0] = (fan_rect){
        0,                           (3.0f * sprite_girl_size.y), sprite_girl_size.x, sprite_girl_size.y
    };
    player_idle_right_frames[1] = (fan_rect){
        sprite_girl_size.x,          (3.0f * sprite_girl_size.y), sprite_girl_size.x, sprite_girl_size.y
    };
    player_idle_right_frames[2] = (fan_rect){
        (2.0f * sprite_girl_size.x), (3.0f * sprite_girl_size.y), sprite_girl_size.x, sprite_girl_size.y
    };

    global AnimationData anim_table[] = {
        { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.5f, .frame_count = 3, true, -1 },
        { "player_idle_up",    player_idle_up_frames,    .frame_time = 0.5f, .frame_count = 3, true, -1 },
        { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.5f, .frame_count = 3, true, -1 },
        { "player_idle_right", player_idle_right_frames, .frame_time = 0.5f, .frame_count = 3, true, -1 },
    };

    world->anim_table = anim_table;

    ComponentAdd(&world->c_transform,     world->entity_count);
    ComponentAdd(&world->c_shape,         world->entity_count);
    ComponentAddArgs(&world->c_movement,  world->entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    ComponentAddArgs(&world->c_texture,   world->entity_count,
        .texture = tex_girl,
        // .rect = player_idle_down_frames[0],
        // .rect = (fan_rect_i32){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
        .rect = { .x = 0, .y = 0, .width = sprite_girl_size.x, .height = sprite_girl_size.y },
        // .rect = { .width = 36, .height = 36 },
    );
    ComponentAddArgs(&world->c_animation, world->entity_count);
    ComponentAdd(&world->c_interaction,  world->entity_count);
    ComponentAdd(&world->c_interactable, world->entity_count);
    ComponentAddArgs(&world->c_attack,   world->entity_count,
        .arc_angle    = fan_f32_rad(45.0f),
        .swing_time   = 0.4f,
        .knockback    = 1.0f,
        .attack_range = 1.5f,
    );
    // ComponentAddArgs(&world->c_animation, world->entity_count);
    ComponentAddArgs(&world->c_light,     world->entity_count,
        .color  = (fan_color){ 170, 170, 170, 170 },
        .radius = 200.0f,
    );
    world->spec_id.player = world->entity_count;
    world->entity_count++;

    // fan_texture tex_mewee = fan_texture_load("./resources/mewee.png");
    ComponentAdd(&world->c_transform,    world->entity_count);
    ComponentAddArgs(&world->c_shape,    world->entity_count,
        .color = (fan_color){ 50, 255, 255, 255 },
    );
    ComponentAddArgs(&world->c_movement, world->entity_count, .speed = 1.0f);
    ComponentAddArgs(&world->c_texture,  world->entity_count,
        .texture = tex_sprite,
        .rect = { .width = 36, .height = 36 },
    );
    ComponentAddArgs(&world->c_behavior, world->entity_count,
        .type = BehaviorType_Random,
        .update_time = 3.0f,
    );
    ComponentAdd(&world->c_interaction,  world->entity_count);
    ComponentAdd(&world->c_interactable, world->entity_count);
    // ComponentAddArgs(&world->c_zone,     world->entity_count,
    //     .x = 0, .y = 0, .width = 1, .height = 1,
    // );
    ComponentAdd(&world->c_tag_enemy,    world->entity_count);
    world->entity_count++;

    ComponentAdd(&world->c_transform,    world->entity_count);
    ComponentAddArgs(&world->c_shape,    world->entity_count,
        .color = (fan_color){ 255, 50, 255, 255 },
    );
    ComponentAddArgs(&world->c_movement, world->entity_count, .speed = 0.5f);
    ComponentAddArgs(&world->c_texture,  world->entity_count,
        .texture = tex_sprite,
        .rect = { .x = 64, .y = 0, .width = 14, .height = 16 },
    );
    ComponentAddArgs(&world->c_behavior, world->entity_count,
        .type = BehaviorType_Follow,
    );
    ComponentAdd(&world->c_interaction,  world->entity_count);
    ComponentAdd(&world->c_interactable, world->entity_count);
    ComponentAdd(&world->c_tag_enemy,    world->entity_count);
    world->entity_count++;

    ComponentAddArgs(&world->c_transform,  world->entity_count,
        .position = (fan_vec2){   0,  5 },
        .scale    = (fan_vec2){  10,  6 },
    );
    ComponentAddArgs(&world->c_shape,      world->entity_count,
        .layer = 1,
        .color = (fan_color){ 155, 155, 155, 255 },
    );
    ComponentAddArgs(&world->c_movement,   world->entity_count,
        .flags = MovementFlag_NoCollision,
    );
    ComponentAdd(&world->c_tag_background, world->entity_count);
    world->entity_count++;
    //
    // ComponentAddArgs(&world->c_transform, world->entity_count,
    //     .position = (fan_vec2){ 200.0f, 300.0f },
    //     .scale = (fan_vec2){ 400.0f, 150.0f }
    // );
    // ComponentAddArgs(&world->c_shape,     world->entity_count,
    //     .color = (fan_color){ 200, 165, 175, 255 },
    //     .layer = 3,
    // );
    // ComponentAddArgs(&world->c_movement,  world->entity_count,
    //     .flags = MovementFlag_NoCollision,
    // );
    // world->entity_count++;

    // ComponentAddArgs(&world->c_transform, world->entity_count,
    //     .position = (fan_vec2){ 200, 100 },
    // );
    // ComponentAddArgs(&world->c_shape,     world->entity_count,
    //     .color = (fan_color){ 50, 255, 255, 255 },
    // );
    // ComponentAddArgs(&world->c_movement,  world->entity_count,
    //     .flags = MovementFlag_Immovable,
    // );
    // world->entity_count++;

    ComponentAddArgs(&world->c_transform, world->entity_count,
        .position = (fan_vec2){ 5, 2 },
    );
    ComponentAddArgs(&world->c_shape,     world->entity_count,
        .color = (fan_color){ 50, 255, 255, 255 },
    );
    ComponentAddArgs(&world->c_movement,  world->entity_count,
        .flags = MovementFlag_Immovable,
    );
    world->entity_count++;

    ComponentAdd(&world->c_transform,    world->entity_count);
    ComponentAddArgs(&world->c_movement, world->entity_count,
        .speed = 5.0f,
        .flags = MovementFlag_NoCollision,
    );
    ComponentAddArgs(&world->c_behavior, world->entity_count,
        .type = BehaviorType_Follow,
    );
    world->spec_id.camera = world->entity_count;
    world->entity_count++;
}

void GameInit(Allocator *a, World *world, GameState *state) {
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

    ComponentCreate(&world->c_tag_background, a, component_size);
    ComponentCreate(&world->c_tag_enemy,      a, component_size);

    world->split.dynamic_entities = a->make(a->ctx, split_size);
    world->split.dynamic_capacity = split_size;

    world->split.static_entities  = a->make(a->ctx, split_size);
    world->split.static_capacity  = split_size;

    SceneMain(world);

    state->bound_zone  = (fan_rect_i32){ .width = 10, .height = 6 };
    state->camera_zoom = 1.0f;

    state->rendermap = fan_rtexture_load((int32)640, (int32)480);
    state->lightmap  = fan_rtexture_load((int32)640, (int32)480);

    state->music = fan_music_load("./resources/My Uncles Last Voyage.mp3");
    fan_music_play(state->music);
    fan_music_volume_set(state->music, 0.4f);

    TileMap map = (TileMap) {
        .tile_size = 1,
		.tiles     = MatrixInt32Create(a, 10, 10, 1),
    };

	world->map = map;

    printf("Successfully passed initialization!\n");
}

void GameUpdateAndRender(Allocator *a, World *world, GameState *state, float32 dt) {
    (void)a;

    if (state->resized) {
        StateGetView(state);
        state->resized = false;
    }

    fan_music_update(state->music);

    UpdateEntities(world, state, dt);
    RenderEntities(world, state, dt);
}

void GameClose(Allocator *a, World *world, GameState *state) {
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
    fan_music_unload(state->music);
}
