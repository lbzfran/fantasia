
#include "game.h"
#include "os.h"
#include "platform.h"

FanVector2 WorldToScreen(FanVector2 world_coord, FanVector2 camera_position, float32 camera_zoom, int32 pixels_per_unit) {
    int32 screen_width = FanWindowWidth();
    int32 screen_height = FanWindowHeight();

    FanVector2 camera_coord = FanVector2Sub(world_coord, camera_position);
    FanVector2 px_coord     = FanVector2Scale(camera_coord, pixels_per_unit * camera_zoom);

    FanVector2 screen_coord = (FanVector2) {
        (screen_width  / 2.0f) + px_coord.x,
        (screen_height / 2.0f) - px_coord.y,
    };

    return screen_coord;
}

FanVector2 ScreenToWorld(FanVector2 screen_coord, FanVector2 camera_position, float32 camera_zoom, int32 pixels_per_unit) {
    int32 screen_width = FanWindowWidth();
    int32 screen_height = FanWindowHeight();

    FanVector2 centered_coord = (FanVector2) {
        screen_coord.x - (screen_width / 2.0f),
        (screen_height / 2.0f) - screen_coord.y
    };

    FanVector2 local_coord = FanVector2Scale(centered_coord, 1.0f / (pixels_per_unit * camera_zoom));
    FanVector2 world_coord = FanVector2Add(camera_position, local_coord);

    return world_coord;
}

void MovementSystem(CMovement *m, CTransform *t, FanVector2 direction, FanRectInt32 bound_zone, float32 dt) {
    if (not m->initialized) {
        init_if_null(m->speed,       4.0f);
        init_if_null(m->max_speed,   5.0f);

        init_if_null(m->direction.x, 1.0f);
        init_if_null(m->direction.y, 1.0f);

        m->active      = true;
        m->initialized = true;
    }

    FanVector2 velocity;
    if (FanVector2Length(m->velocity_force) > 0.001f) {
        velocity = m->velocity_force;
        float32 damp_factor = 0.98f;
        m->velocity_force = FanVector2Scale(m->velocity_force, damp_factor);
        m->lock_time = 0.15f;
    }
    else {
        m->velocity_input = FanVector2Zero();
        if (m->lock_time > 0.0f) {
            m->lock_time = max(m->lock_time - dt, 0.0f);
        }
        else if (FanVector2Length(direction) > 0.0f) {
            m->direction       = direction;
            direction          = FanVector2Normalize(direction);
            m->velocity_input  = FanVector2Scale(direction, m->speed);
        }
        velocity = m->velocity_input;
    }

    t->position = FanVector2Add(t->position, FanVector2Scale(velocity, dt));

    // float32 damp_factor = 0.9f;
    // m->velocity_force = FanVector2Scale(m->velocity_force, damp_factor);
    // m->velocity_force = FanVector2AddValue(m->velocity_force, -decay * dt);

    if (not (m->flags & MovementFlag_NoCollision)) {
        if (FanVector2Length(t->scale) > 0.0f) {
            bound_zone.width  -= t->scale.x;
            bound_zone.height -= t->scale.y;
        }

        float32 overlap;
        float32 softness = 0.005f;
        if (t->position.x < bound_zone.x) {
            overlap = bound_zone.x - t->position.x;
            t->position.x += overlap * softness;
        }
        else if (t->position.x > bound_zone.width) {
            overlap = t->position.x - bound_zone.width;
            t->position.x -= overlap * softness;
        }

        if (t->position.y < bound_zone.y) {
            overlap = bound_zone.y - t->position.y;
            t->position.y += overlap * softness;
        }
        else if (t->position.y > bound_zone.height) {
            overlap = t->position.y - bound_zone.height;
            t->position.y -= overlap * softness;
        }
    }
}

void PhysicsSystem(CPhysics *p, CTransform *t, FanVector2 force, float32 dt) {
    if (not p->initialized) {
        init_if_null(p->last_position.x, t->position.x);
        init_if_null(p->last_position.y, t->position.y);

        init_if_null(p->speed,           500.0f);
        init_if_null(p->friction,        0.2f);
        init_if_null(p->mass,            1.0f);

        p->active = true;
        p->initialized = true;
    }

    FanVector2 velocity = FanVector2Sub(t->position, p->last_position);
    FanVector2 acceleration = FanVector2Zero();

    FanVector2 screen_size = {
        FanWindowWidth(),
        FanWindowHeight()
    };
    if (FanVector2Length(t->scale) > 0.0f) {
        screen_size.x -= t->scale.x;
        screen_size.y -= t->scale.y;
    }

    velocity = FanVector2Scale(velocity, 1.0f - p->friction * dt);

    float32 safe_mass = max(p->mass, 0.0001f);
    acceleration = FanVector2Scale(force, p->speed / safe_mass);

    FanVector2 new_position = FanVector2Add(
        t->position,
        FanVector2Add(velocity, FanVector2Scale(acceleration, dt * dt))
    );

    new_position.x = clamp(new_position.x, 0.0f, screen_size.x);
    new_position.y = clamp(new_position.y, 0.0f, screen_size.y);

    p->last_position = t->position;
    t->position = new_position;
}

inline bool32 CollisionCheckR(FanRectFloat32 a, FanRectFloat32 b) {
    bool32 result = false;

    result = not (a.x + a.width  < b.x or b.x + b.width  < a.x or
                  a.y + a.height < b.y or b.y + b.height < a.y);

    return result;
}

bool32 CollisionCheckV(FanVector2 aPos, FanVector2 aSize, FanVector2 bPos, FanVector2 bSize) {
    bool32 result = false;

    // AABB
    result = not (aPos.x + aSize.x < bPos.x or bPos.x + bSize.x < aPos.x or
                  aPos.y + aSize.y < bPos.y or bPos.y + bSize.y < aPos.y);

    return result;
}

/*
 * MATRIX
 */
int32 MatrixInt32Get_(MatrixInt32 m, int32 i, int32 j) {
    int32 idx = i * m.cols + j;
    return m.V[idx];
}
#define MatrixInt32Get(m, i, j) MatrixInt32Get_(m, max(i, m.rows - 1), max(j, m.cols - 1))

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

FanVector2 TileMapGetPosition(TileMap map, FanVector2 position) {
    int32 mapX = FanFloat32Truncate((position.x - map.origin.x) / map.tile_size);
    int32 mapY = FanFloat32Truncate((position.y - map.origin.y) / map.tile_size);

    FanVector2 tile_pos = (FanVector2) {
        mapX,
        mapY
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
        FanVector2 aMax = (FanVector2){
            a->position.x + a->scale.x,
            a->position.y + a->scale.y
        };
        FanVector2 bMax = (FanVector2){
            b->position.x + b->scale.x,
            b->position.y + b->scale.y
        };

        FanVector2 overlap = (FanVector2){
            min(aMax.x, bMax.x) - max(a->position.x, b->position.x),
            min(aMax.y, bMax.y) - max(a->position.y, b->position.y)
        };

        const float32 tolerance = 0.0f;
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

bool32 AttackInArc(FanVector2 target, FanVector2 facing, float32 arc_angle, float32 progress) {
    float32 half = arc_angle * 0.5f;

    FanVector2 start_dir = FanVector2Rotate(facing, -half);
    FanVector2 end_dir   = FanVector2Rotate(facing,  half);

    FanVector2 sweep_dir  = FanVector2Normalize(FanVector2Lerp(start_dir, progress, end_dir));
    FanVector2 target_dir = FanVector2Normalize(target);

    float32 dot_value = FanVector2Dot(target_dir, sweep_dir);

    float32 tolerance = FanFloat32Cos(FanFloat32Rad(10));
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

    FanVector2 target_dist = (FanVector2) {
        (o_t->position.x + (o_t->scale.x / 2)) - (t->position.x + (t->scale.x / 2)),
        (o_t->position.y + (o_t->scale.y / 2)) - (t->position.y + (t->scale.y / 2))
    };
    float32 dist_squared = FanVector2LengthSqr(target_dist);

    if (dist_squared <= a->attack_range * a->attack_range) {
        if (AttackInArc(target_dist, m->direction, a->arc_angle, progress)) {
            float32 knockback_base_factor = 1.0f;
            FanVector2 knockback_dir  = FanVector2Scale(target_dist, 1.0 / FanFloat32Sqrt(dist_squared));
            FanVector2 knockback_dist = FanVector2Scale(knockback_dir, a->knockback * knockback_base_factor);
            o_m->velocity_force = FanVector2Add(o_m->velocity_force, knockback_dist);
        }
    }
}

void TextureUpdate(CTexture *tx, FanVector2 pos, FanVector2 size, float dt) {
    (void)dt;

    tx->rect = (FanRectInt32){
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

    FanRectInt32 current = data->frames[a->current_frame];
    TextureUpdate(
        t,
        (FanVector2){ current.x,     current.y },
        (FanVector2){
            coalesce(current.width,  t->rect.width),
            coalesce(current.height, t->rect.height)
        },
        dt
    );
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
	    FanRectFloat32 zone,
        FanVector2 camera_position,
        float32 camera_zoom,
        int32 pixels_per_unit,
	    int32 flags
    ) {
    FanVector2 screen_pos    = WorldToScreen(t->position, camera_position, camera_zoom, pixels_per_unit);
    FanVector2 screen_offset = FanVector2Scale(s->offset, pixels_per_unit * camera_zoom);
    FanVector2 screen_scale  = FanVector2Scale(t->scale,  pixels_per_unit * camera_zoom);
    if (tx is null) {
        if (FanVector2Length(s->offset) > 0.0f) {
            FanDrawRectV(FanVector2Add(screen_pos, screen_offset), screen_scale, (FanColor){ 50, 50, 50, 255 });
        }
        FanDrawRectV(screen_pos, screen_scale, s->color);
    }
    else {
        float width  = (tx->rect.width)  ? tx->rect.width  : tx->texture.width;
        float height = (tx->rect.height) ? tx->rect.height : tx->texture.height;

        if (m) {
            if (flags & RenderFlag_FlipX) {
                width  *= m->direction.x ? m->direction.x : 1.0f;
            }
            if (flags & RenderFlag_FlipY) {
                height *= m->direction.y ? m->direction.y : 1.0f;
            }
        }


        FanRectInt32 src = (FanRectInt32) {
            tx->rect.x,
            tx->rect.y,
            width,
            height
        };

        FanRectInt32 dst = (FanRectInt32) {
            screen_pos.x + screen_offset.x,
            screen_pos.y + screen_offset.y,
            screen_scale.x,
            screen_scale.y
        };

        if (flags & RenderFlag_ShowInteract) {
            FanVector2 screen_zone_pos = WorldToScreen((FanVector2){ zone.x, zone.y }, camera_position, camera_zoom, pixels_per_unit);
            FanRectInt32 screen_zone = (FanRectInt32) {
                screen_zone_pos.x,
                screen_zone_pos.y,
                zone.width  * pixels_per_unit * camera_zoom,
                zone.height * pixels_per_unit * camera_zoom
            };
            FanColor zone_color = interacting ?
                (FanColor){ 255, 0, 0, 75 } : (FanColor){ 0, 255, 0, 75 };

            FanDrawRectR(screen_zone, zone_color);
        }

        FanDrawTexture(
            tx->texture,
            src,
            dst,
            (FanVector2) { 0.0f, 0.0f },
            0.0f,
            s->color
        );
    }
}

typedef struct RenderEntry {
    int32   id;
    float32 height;
    int32   layer;
} RenderEntry;

int32 SortRenderPartition_(RenderEntry *entries, int32 low, int32 high) {
    RenderEntry pivot = entries[high];
    RenderEntry temp;

    int32 i = low - 1;

    for (int32 j = low; j <= high - 1; j++) {
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

void SortRender(RenderEntry *entries, int32 low, int32 high) {
    // qsort in-place
    if (low < high) {
        int32 pi = SortRenderPartition_(entries, low, high);

        SortRender(entries, low, pi - 1);
        SortRender(entries, pi + 1, high);
    }
}

void RenderEntities(World *world, GameState *state, float32 dt) {
    (void)dt;
    RenderEntry render_array[128] = { { -1, 0.0f, 0 } };
    ssize render_entry_count = 0;

    FanVector2 camera_position = world->c_transform.data[world->spec_id.camera].position;
    float32 camera_zoom = state->camera_zoom;
    int32 pixels_per_unit = world->pixels_per_unit;

    for (ssize i = 0; i < world->c_shape.size; i++) {
        int32 id = world->c_shape.dense[i];
        if (id == -1)
            continue;
        CShape *shape = &world->c_shape.data[i];

        int32 transform_idx = world->c_transform.sparse[id];
        if (transform_idx == -1)
            continue;
        CTransform *transform = &world->c_transform.data[transform_idx];

        if (not shape->initialized) {
            if (shape->color.a == 0) {
                shape->color = FanColor_WHITE;
            }
            init_if_null(shape->layer, 2);

            shape->visible = true;
            shape->initialized = true;
        }

        render_array[render_entry_count++] = (RenderEntry) {
            id,
            transform->position.y + transform->scale.y,
            shape->layer
        };
    }

    SortRender(render_array, 0, world->c_shape.size - 1);

    for (ssize i = 0; i < render_entry_count; i++) {
        int32 id = render_array[i].id;
        if (id == -1)
            continue;

        int32 shape_idx        = world->c_shape.sparse[id];
        int32 transform_idx    = world->c_transform.sparse[id];
        int32 move_idx         = world->c_movement.sparse[id];
        int32 animation_idx    = world->c_animation.sparse[id];
        int32 texture_idx      = world->c_texture.sparse[id];
        int32 attack_idx       = world->c_attack.sparse[id];

        int32 tag_bg           = world->c_tag_background.sparse[id];
        (void)tag_bg;

        int32 interaction_idx  = world->c_interaction.sparse[id];
        int32 interactable_idx = world->c_interactable.sparse[id];
        int32 zone_idx         = world->c_zone.sparse[id];

        CShape         *shape       = &world->c_shape.data[shape_idx];
        CTransform     *transform   = &world->c_transform.data[transform_idx];
        CMovement      *move        = &world->c_movement.data[move_idx];
        CTexture       *texture     = null;
        // CAnimation     *animation   = null;
        CAttack        *attack      = &world->c_attack.data[attack_idx];
        bool32          interacting = false;
        bool32          interacted  = false;
        FanRectFloat32  zone        = { 0 };

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
            zone = (FanRectFloat32) {
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

        FanVector2 center_pos = FanVector2Add(
            transform->position,
            (FanVector2) {
                transform->scale.x * 0.5f,
                transform->scale.y * 0.5f,
            }
        );

        FanVector2 map_pos = TileMapGetPosition(world->map, center_pos);
        int32 tile_data = MatrixInt32Get(world->map.tiles, map_pos.x, map_pos.y);

        if (id == world->spec_id.player) {
            // FanVector2 tile_world_pos = (FanVector2) {
            //     map_pos.x * world->map.tile_size,
            //     map_pos.y * world->map.tile_size
            // };
            //
            // FanVector2 screen_pos = WorldToScreen(tile_world_pos, camera_position, pixels_per_unit);
            // int32 screen_size     = world->map.tile_size * pixels_per_unit;

            if (tile_data == 1) {
                // FanDrawRectR(
                //     (FanRectInt32) {
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
            render_flags
        );


        if (attack_idx != -1 and attack->attacking and attack->cast_timer <= 0.0f) {
            FanVector2 center = FanVector2Add(transform->position, (FanVector2){ transform->scale.x * 0.5f, transform->scale.y * -0.5f });
            FanVector2 facing = move->direction;
            float32 length    = attack->attack_range;
            float32 angle     = attack->arc_angle;

            FanVector2 prev = WorldToScreen(center, camera_position, camera_zoom, pixels_per_unit);

            // Compute start and end angles
            float32 progress = attack->timer / attack->swing_time;
            int32 segments = 20;

            // Precompute start and end directions by rotating facing
            FanVector2 start_dir = FanVector2Rotate(facing, -angle * 0.5f);
            FanVector2 end_dir   = FanVector2Rotate(facing,  angle * 0.5f);

            for (int32 i = 0; i <= segments; i++) {
                float32 t_seg = (float32)i / (float32)segments;

                // Interpolate between start and end directions
                FanVector2 sweep_dir = FanVector2Normalize(FanVector2Lerp(start_dir, t_seg, end_dir));

                FanVector2 world_point = {
                    center.x + sweep_dir.x * length,
                    center.y + sweep_dir.y * length
                };
                FanVector2 screen_point = WorldToScreen(world_point, camera_position, camera_zoom, pixels_per_unit);

                // FanVector2Print(sweep_dir);
                // FanVector2Print(screen_point);

                FanDrawLineV(prev, screen_point, FanColor_RED);
                prev = screen_point;
            }

            // Draw line from center to current sweep tip
            FanVector2 sweep_tip = FanVector2Normalize(FanVector2Lerp(start_dir, progress, end_dir));
            FanVector2 world_sweep_tip = {
                center.x + sweep_tip.x * length,
                center.y + sweep_tip.y * length
            };
            FanVector2 screen_sweep_tip = WorldToScreen(world_sweep_tip, camera_position, camera_zoom, pixels_per_unit);
            FanDrawLineV(WorldToScreen(center, camera_position, camera_zoom, pixels_per_unit), screen_sweep_tip, FanColor_RED);
        }

        if (state->called_object_dump) {
            printf("id: %d\n", id);
            printf("interacting: %s\n", interacting ? "true" : "false");
            printf("interacted: %s\n",  interacted  ? "true" : "false");
            FanRectPrint(zone);
            printf("\t");
            FanVector2Print(transform->position);
            printf("\t");
            FanVector2Print(transform->scale);
        }
    }
}

// NOTE(liam): must call whenever entities are added/removed
global void UpdateEntitySplit(World *world) {
    EntitySplit *split = &world->split;

    split->dynamic_count = 0;
    split->static_count  = 0;
    for (ssize i = 0; i < world->c_transform.size; i++) {
        int32 id = world->c_transform.dense[i];
        if (id == -1) continue;

        int32 move_index = world->c_movement.sparse[id];
        int32 bg_tag = world->c_tag_background.sparse[id];

        bool32 is_static = true;
        if (bg_tag < 0 or move_index != -1) {
            CMovement *movement = &world->c_movement.data[move_index];
            if (movement->flags & MovementFlag_NoCollision) continue;

            if (not (movement->flags & MovementFlag_Immovable)) {
                is_static = false;
            }
        }

        if (is_static)
            split->static_entities[split->static_count++]   = id;
        else
            split->dynamic_entities[split->dynamic_count++] = id;
    }
}

void UpdateEntities(
        World *world,
        GameState *state,
        float32 dt
    ) {
    EntitySplit *split = &world->split;
    if (world->update_entity_split) {
        UpdateEntitySplit(world);
    }

    for (ssize i = 0; i < world->c_transform.size; i++) {
        int32 id = world->c_transform.dense[i];
        if (id == -1)
            continue;

        CTransform *transform = &world->c_transform.data[i];
        if (not transform->initialized) {
            init_if_null(transform->scale.x, 1.0f);
            init_if_null(transform->scale.y, 1.0f);

            transform->initialized = true;
        }
    }

    for (ssize i = 0; i < world->c_movement.size; i++) {
        int32 id = world->c_movement.dense[i];
        if (id == -1)
            continue;

        int32 transform_idx   = world->c_transform.sparse[id];
        int32 behavior_idx    = world->c_behavior.sparse[id];
        int32 interact_idx    = world->c_interaction.sparse[id];
        int32 interacted_idx  = world->c_interactable.sparse[id];
        int32 attack_idx      = world->c_attack.sparse[id];
        // int32 zone_idx        = world->c_zone.sparse[id];

        CMovement       *move      = &world->c_movement.data[i];
        CTransform      *transform = &world->c_transform.data[transform_idx];
        CBehavior       *behavior  = null;
        CAttack         *attack    = null;
        // FanRectFloat32  *zone      = null;

        FanVector2 direction = FanVector2Zero();

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
                        direction = (FanVector2) {
                            FanRandomInt(-1, 1),
                            FanRandomInt(-1, 1)
                        };
                    }
                    else {
                        // keeps entity moving rather than staying still
                        direction = move->direction;
                    }
                } break;
                case BehaviorType_Follow: {
                    if (behavior->updating) {
                        CTransform *target_transform = &world->c_transform.data[0];
                        FanVector2 target_face       = target_transform->position;
                        if (id == world->spec_id.camera) {
                            CShape *target_shape = &world->c_shape.data[0];
                            target_face = FanVector2Add(target_face, target_shape->offset);
                        }
                        FanVector2 face = FanVector2Normalize(FanVector2Sub(target_face, transform->position));
                        direction = (FanVector2){ signof(face.x), signof(face.y) };
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

        MovementSystem(move, transform, direction, state->bound_zone, dt);

        if (state->called_object_dump) {
            printf("\tid: %d\n", id);

            if (id == world->spec_id.player) {
                printf("\t");
                FanVector2Print(transform->position);
                printf("\t");
                FanVector2Print(transform->scale);

                if (move) {
                    printf("\t");
                    FanVector2Print(move->velocity_input);
                    printf("\t");
                    FanVector2Print(move->direction);
                    printf("\tmove->speed: %f\n", move->speed);
                }
            }
        }
    }

    for (ssize i = 0; i < world->c_animation.size; i++) {
        int32 id = world->c_animation.dense[i];
        if (id == -1)
            continue;

        int32 texture_idx = world->c_texture.sparse[id];
        int32 move_idx    = world->c_movement.sparse[id];

        CAnimation *anim    = &world->c_animation.data[i];
        CTexture   *texture = &world->c_texture.data[texture_idx];
        CMovement  *move    = &world->c_movement.data[move_idx];

        if (id == world->spec_id.player) {
            FanVector2 direction = state->p_input.direction;
            if (direction.x > 0.0f) {
                anim->request.id = 3;
            }
            else if (direction.x < 0.0f) {
                anim->request.id = 2;
            }

            if (direction.y > 0.0f) {
                anim->request.id = 1;
            }
            else if (direction.y < 0.0f) {
                anim->request.id = 0;
            }
        }

        AnimationSystem(anim, texture, world->anim_table, dt);
    }

    for (ssize i = 0; i < split->dynamic_count; i++) {
        int32 id = split->dynamic_entities[i];

        int32 move_idx        = world->c_movement.sparse[id];
        int32 transform_idx   = world->c_transform.sparse[id];
        int32 interact_idx    = world->c_interaction.sparse[id];
        int32 zone_idx        = world->c_zone.sparse[id];
        int32 attack_idx      = world->c_attack.sparse[id];

        int32 tag_enemy       = world->c_tag_enemy.sparse[id];

        CMovement      *move      = &world->c_movement.data[move_idx];
        CTransform     *transform = &world->c_transform.data[transform_idx];
        CAttack        *attack    = &world->c_attack.data[attack_idx];
        bool32         *interact  = null;
        FanRectFloat32  zone      = (FanRectFloat32) { 0 };

        if (interact_idx != -1) {
            interact = &world->c_interaction.data[interact_idx];
        }

        if (zone_idx != -1) {
            zone = world->c_zone.data[zone_idx];
            zone.x += transform->position.x;
            zone.y += transform->position.y;
        }
        else {
            zone = (FanRectFloat32) {
                .x      = transform->position.x,
                .y      = transform->position.y,
                .width  = transform->scale.x,
                .height = transform->scale.y,
            };
        }

        if (move->active and not (move->flags & MovementFlag_NoCollision)) {
            for (ssize j = i + 1; j < split->dynamic_count; j++) {
                int32 other_id = split->dynamic_entities[j];

                int32 other_transform_idx    = world->c_transform.sparse[other_id];
                int32 other_move_idx         = world->c_movement.sparse[other_id];
                assert(other_move_idx != -1);
                int32 other_interacted_idx   = world->c_interactable.sparse[other_id];
                int32 other_zone_idx         = world->c_zone.sparse[other_id];

                int32 other_tag_enemy        = world->c_tag_enemy.sparse[other_id];

                CTransform     *other_transform  = &world->c_transform.data[other_transform_idx];
                CMovement      *other_move       = &world->c_movement.data[other_move_idx];
                bool32         *other_interacted = null;
                FanRectFloat32  other_zone       = (FanRectFloat32) { 0 };

                if (other_interacted_idx != -1) {
                    other_interacted = &world->c_interactable.data[other_interacted_idx];
                }

                if (other_zone_idx != -1) {
                    other_zone = world->c_zone.data[other_zone_idx];
                    other_zone.x += other_transform->position.x;
                    other_zone.y += other_transform->position.y;
                }
                else {
                    other_zone = (FanRectFloat32) {
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
                        AttackSystem(attack, move, transform, other_move, other_transform, dt);
                    }
                }

                CollisionSystem(transform, move, other_transform, other_move, dt);
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
                int32 other_id = split->static_entities[i];

                int32 other_transform_idx    = world->c_transform.sparse[other_id];
                int32 other_move_idx         = world->c_movement.sparse[other_id];
                assert(other_move_idx != -1);
                int32 other_interacted_idx   = world->c_interactable.sparse[other_id];
                int32 other_zone_idx         = world->c_zone.sparse[other_id];

                int32 other_tag_enemy        = world->c_tag_enemy.sparse[other_id];

                CTransform     *other_transform  = &world->c_transform.data[other_transform_idx];
                CMovement      *other_move       = &world->c_movement.data[other_move_idx];
                bool32         *other_interacted = null;
                FanRectFloat32  other_zone       = (FanRectFloat32) { 0 };

                if (other_interacted_idx != -1) {
                    other_interacted = &world->c_interactable.data[other_interacted_idx];
                }

                if (other_zone_idx != -1) {
                    other_zone = world->c_zone.data[other_zone_idx];
                    other_zone.x += other_transform->position.x;
                    other_zone.y += other_transform->position.y;
                }
                else {
                    other_zone = (FanRectFloat32) {
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
                    AttackSystem(attack, move, transform, other_move, other_transform, dt);
                }

                CollisionSystem(transform, move, other_transform, other_move, dt);
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

        if (state->called_object_dump) {
            printf("\tid: %d\n", id);

            // if (id == world->spec_id.player) {
                printf("\t");
                FanVector2Print(transform->position);
                printf("\t");
                FanVector2Print(transform->scale);

                if (move) {
                    printf("\t");
                    FanVector2Print(move->velocity_input);
                    printf("\t");
                    FanVector2Print(move->direction);
                    printf("\tmove->speed: %f\n", move->speed);
                }
                printf("\t");
                FanRectPrint(zone);
            // }
        }
    }
}

void SceneSolo(World *world) {
    FanTexture tex_link = FanTextureLoad("./resources/link.png");
    // FanVector2 sprite_link_size = (FanVector2){ tex_link.width / 10.0f, tex_link.height / 8.0f };
    // player_idle_down_frames[0]  = (FanRectInt32){ 0,                         0,                         0, 0 };
    // player_idle_down_frames[1]  = (FanRectInt32){ sprite_link_size.x,        0,                         0, 0 };
    // player_idle_down_frames[2]  = (FanRectInt32){ 2.0f * sprite_link_size.x, 0,                         0, 0 };
    //
    // player_idle_up_frames[0]    = (FanRectInt32){ 0,                         2.0f * sprite_link_size.y, 0, 0 };
    //
    // player_idle_left_frames[0]  = (FanRectInt32){ 0,                         sprite_link_size.y,        0, 0 };
    // player_idle_left_frames[1]  = (FanRectInt32){ sprite_link_size.x,        sprite_link_size.y,        0, 0 };
    // player_idle_left_frames[2]  = (FanRectInt32){ 2.0f * sprite_link_size.x, sprite_link_size.y,        0, 0 };
    //
    // player_idle_right_frames[0] = (FanRectInt32){ 0,                         3.0f * sprite_link_size.y, 0, 0 };
    // player_idle_right_frames[1] = (FanRectInt32){ sprite_link_size.x,        3.0f * sprite_link_size.y, 0, 0 };
    // player_idle_right_frames[2] = (FanRectInt32){ 2.0f * sprite_link_size.x, 3.0f * sprite_link_size.y, 0, 0 };


    ComponentAdd(&world->c_transform,     world->entity_count);
    ComponentAdd(&world->c_shape,         world->entity_count);
    ComponentAddArgs(&world->c_movement,  world->entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    ComponentAddArgs(&world->c_texture,   world->entity_count,
        .texture = tex_link,
        .rect = (FanRectInt32){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
    );
    // ComponentAddArgs(&world->c_animation, world->entity_count);
    world->spec_id.player = world->entity_count;
    world->entity_count++;

    ComponentAddArgs(&world->c_transform,  world->entity_count,
        .scale = (FanVector2){ FanWindowWidth(), FanWindowHeight() },
    );
    ComponentAddArgs(&world->c_shape,      world->entity_count,
        .layer = 1,
        .color = (FanColor){ 155, 155, 155, 255 },
    );
    ComponentAddArgs(&world->c_movement,   world->entity_count,
        .flags = MovementFlag_NoCollision,
    );
    ComponentAdd(&world->c_tag_background, world->entity_count);
    world->entity_count++;
}


global void SceneMain(World *world) {
    FanTexture tex_sprite = FanTextureLoad("./resources/Sprite-0001.png");

    // FanRectInt32 player_idle_up_frames[1]    = { 0 };
    // global FanRectInt32 player_idle_down_frames[2]  = { 0 };
    // player_idle_down_frames[0] = (FanRectInt32){ 0, 0,  .width = 32, .height = 32 };
    // player_idle_down_frames[1] = (FanRectInt32){ 32, 0, .width = 32, .height = 32 };
    // FanRectInt32 player_idle_left_frames[3]  = { 0 };
    // FanRectInt32 player_idle_right_frames[3] = { 0 }tex_sprite.height;

    // global AnimationData anim_table[] = {
    //     { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.5f, .frame_count = 2, true, -1 },
        // { "player_idle_up",    player_idle_up_frames,    .frame_time = 1.5f, .frame_count = 1, false,  0 },
        // { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.5f, .frame_count = 3, false,  0 },
        // { "player_idle_right", player_idle_right_frames, .frame_time = 0.5f, .frame_count = 3, false,  0 },
    // };

    FanTexture tex_link = FanTextureLoad("./resources/link.png");
    FanVector2 sprite_link_size = (FanVector2){ tex_link.width / 10.0f, tex_link.height / 8.0f };
    global FanRectInt32 player_idle_down_frames[3]  = { 0 };
    global FanRectInt32 player_idle_up_frames[1]    = { 0 };
    global FanRectInt32 player_idle_left_frames[3]  = { 0 };
    global FanRectInt32 player_idle_right_frames[3] = { 0 };
    player_idle_down_frames[0]  = (FanRectInt32){ 0,                         0,                         sprite_link_size.x, sprite_link_size.y };
    player_idle_down_frames[1]  = (FanRectInt32){ sprite_link_size.x,        0,                         sprite_link_size.x, sprite_link_size.y };
    player_idle_down_frames[2]  = (FanRectInt32){ 2.0f * sprite_link_size.x, 0,                         sprite_link_size.x, sprite_link_size.y };

    player_idle_up_frames[0]    = (FanRectInt32){ 0,                         2.0f * sprite_link_size.y, sprite_link_size.x, sprite_link_size.y };

    player_idle_left_frames[0]  = (FanRectInt32){ 0,                         sprite_link_size.y,        sprite_link_size.x, sprite_link_size.y };
    player_idle_left_frames[1]  = (FanRectInt32){ sprite_link_size.x,        sprite_link_size.y,        sprite_link_size.x, sprite_link_size.y };
    player_idle_left_frames[2]  = (FanRectInt32){ 2.0f * sprite_link_size.x, sprite_link_size.y,        sprite_link_size.x, sprite_link_size.y };

    player_idle_right_frames[0] = (FanRectInt32){ 0,                         3.0f * sprite_link_size.y, sprite_link_size.x, sprite_link_size.y };
    player_idle_right_frames[1] = (FanRectInt32){ sprite_link_size.x,        3.0f * sprite_link_size.y, sprite_link_size.x, sprite_link_size.y };
    player_idle_right_frames[2] = (FanRectInt32){ 2.0f * sprite_link_size.x, 3.0f * sprite_link_size.y, sprite_link_size.x, sprite_link_size.y };

    global AnimationData anim_table[] = {
        { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.5f, .frame_count = 3, true,  -1 },
        { "player_idle_up",    player_idle_up_frames,    .frame_time = 1.5f, .frame_count = 1, true,  -1 },
        { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.5f, .frame_count = 3, true,  -1 },
        { "player_idle_right", player_idle_right_frames, .frame_time = 0.5f, .frame_count = 3, true,  -1 },
    };

    world->anim_table = anim_table;

    ComponentAdd(&world->c_transform,     world->entity_count);
    ComponentAdd(&world->c_shape,         world->entity_count);
    ComponentAddArgs(&world->c_movement,  world->entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    ComponentAddArgs(&world->c_texture,   world->entity_count,
        .texture = tex_link,
        .rect = player_idle_down_frames[0],
        // .rect = (FanRectInt32){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
    );
    // ComponentAddArgs(&world->c_animation, world->entity_count);
    ComponentAdd(&world->c_interaction,  world->entity_count);
    ComponentAdd(&world->c_interactable, world->entity_count);
    ComponentAddArgs(&world->c_attack,   world->entity_count,
        .arc_angle    = FanFloat32Rad(45.0f),
        .swing_time   = 0.4f,
        .knockback    = 1.0f,
        .attack_range = 1.5f,
    );
    ComponentAddArgs(&world->c_animation, world->entity_count);
    world->spec_id.player = world->entity_count;
    world->entity_count++;

    // FanTexture tex_mewee = FanTextureLoad("./resources/mewee.png");
    ComponentAdd(&world->c_transform,    world->entity_count);
    ComponentAddArgs(&world->c_shape,    world->entity_count,
        .color = (FanColor){ 50, 255, 255, 255 },
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
        .color = (FanColor){ 255, 50, 255, 255 },
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
        .position = (FanVector2){   0,  5 },
        .scale    = (FanVector2){  10,  6 },
    );
    ComponentAddArgs(&world->c_shape,      world->entity_count,
        .layer = 1,
        .color = (FanColor){ 155, 155, 155, 255 },
    );
    ComponentAddArgs(&world->c_movement,   world->entity_count,
        .flags = MovementFlag_NoCollision,
    );
    ComponentAdd(&world->c_tag_background, world->entity_count);
    world->entity_count++;
    //
    // ComponentAddArgs(&world->c_transform, world->entity_count,
    //     .position = (FanVector2){ 200.0f, 300.0f },
    //     .scale = (FanVector2){ 400.0f, 150.0f }
    // );
    // ComponentAddArgs(&world->c_shape,     world->entity_count,
    //     .color = (FanColor){ 200, 165, 175, 255 },
    //     .layer = 3,
    // );
    // ComponentAddArgs(&world->c_movement,  world->entity_count,
    //     .flags = MovementFlag_NoCollision,
    // );
    // world->entity_count++;

    // ComponentAddArgs(&world->c_transform, world->entity_count,
    //     .position = (FanVector2){ 200, 100 },
    // );
    // ComponentAddArgs(&world->c_shape,     world->entity_count,
    //     .color = (FanColor){ 50, 255, 255, 255 },
    // );
    // ComponentAddArgs(&world->c_movement,  world->entity_count,
    //     .flags = MovementFlag_Immovable,
    // );
    // world->entity_count++;

    ComponentAddArgs(&world->c_transform, world->entity_count,
        .position = (FanVector2){ 5, 2 },
    );
    ComponentAddArgs(&world->c_shape,     world->entity_count,
        .color = (FanColor){ 50, 255, 255, 255 },
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

FanMusic muse = { 0 };
void GameInit(Allocator *a, World *world, GameState *state) {

    int32 split_size = kilobytes(1);
    ssize component_size  = kilobytes(1);

    ComponentCreate(&world->c_transform,      a, component_size);
    ComponentCreate(&world->c_shape,          a, component_size);
    ComponentCreate(&world->c_movement,       a, component_size);
    ComponentCreate(&world->c_texture,        a, component_size);
    ComponentCreate(&world->c_behavior,       a, component_size);
    ComponentCreate(&world->c_animation,      a, component_size);
    ComponentCreate(&world->c_physics,        a, component_size);

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

    state->bound_zone = (FanRectInt32){ .width = 10, .height = 6 };
    state->camera_zoom = 1.0f;

    muse = FanMusicLoad("./resources/My Uncles Last Voyage.mp3");
    FanMusicPlay(muse);
    FanMusicSetVolume(muse, 0.6f);


    TileMap map = (TileMap) {
        .tile_size = 1,
		.tiles = MatrixInt32Create(a, 10, 10, 1),
    };

	world->map = map;
    world->pixels_per_unit = 100;

    printf("Successfully passed initialization!\n");
}

void GameUpdateAndRender(Allocator *a, World *world, GameState *state, float32 dt) {
    (void)a;

    FanMusicUpdate(muse);

    UpdateEntities(world, state, dt);
    RenderEntities(world, state, dt);
}

void GameClose(Allocator *a, World *world, GameState *state) {
    (void)a;
    (void)state;
    for (ssize i = 0; i < world->c_texture.size; i++) {
        if (world->c_texture.dense[i] == -1) {
            continue;
        }
        FanTextureUnload(world->c_texture.data[i].texture);
    }
}
