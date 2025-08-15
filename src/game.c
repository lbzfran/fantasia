
#include "game.h"
#include "platform.h"


void MovementSystem(CMovement *m, CTransform *t, FanVector2 direction, FanRect bounding_zone, float32 dt) {
    if (not m->initialized) {
        init_if_null(m->speed,       400.0f);
        init_if_null(m->max_speed,   500.0f);

        init_if_null(m->direction.x, 1.0f);
        init_if_null(m->direction.y, 1.0f);

        m->active      = true;
        m->initialized = true;
    }

    FanVector2 velocity = FanVector2Zero();
    if (FanVector2Length(direction) > 0.0f) {
        m->direction = direction;
        direction    = FanVector2Normalize(direction);
        velocity     = FanVector2Scale(direction, m->speed);
    }
    m->velocity = velocity;

    t->position = FanVector2Add(t->position, FanVector2Scale(m->velocity, dt));

    float32 softness = 0.005f;
    if (not (m->flags & MovementFlag_NoCollision)) {
        if (FanVector2Length(t->scale) > 0) {
            bounding_zone.width  -= t->scale.x;
            bounding_zone.height -= t->scale.y;
        }

        float32 overlap;
        if (t->position.x < bounding_zone.x) {
            overlap = bounding_zone.x - t->position.x;
            t->position.x += overlap * softness;
        }
        else if (t->position.x > bounding_zone.width) {
            overlap = t->position.x - bounding_zone.width;
            t->position.x -= overlap * softness;
        }

        if (t->position.y < bounding_zone.y) {
            overlap = bounding_zone.y - t->position.y;
            t->position.y += overlap * softness;
        }
        else if (t->position.y > bounding_zone.height) {
            overlap = t->position.y - bounding_zone.height;
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
    if (FanVector2Length(t->scale) > 0) {
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

inline bool32 CollisionCheckR(FanRect a, FanRect b) {
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

void TextureUpdate(CTexture *t, FanVector2 pos, FanVector2 size, float dt) {
    (void)dt;

    t->rect = (FanRect){
        .x      = pos.x,
        .y      = pos.y,
        .width  = coalesce(size.x, t->rect.width),
        .height = coalesce(size.y, t->rect.height)
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
    if (table is null) return;
    if (a->finished or
        (a->request.id != -1 and (a->flags & AnimationFlag_NotInterruptible) == false)) {
        *a = AnimationApply_(table, a->request.id, a->request.flags, (AnimationRequest){ -1, 0 });
    }

    AnimationData *data = &table[a->id];

    a->timer += dt;
    if (a->timer >= data->frame_time) {
        a->timer -= data->frame_time;
        a->current_frame++;

        if (a->current_frame >= data->frame_count) {
            if (data->loop and (a->flags & AnimationFlag_DisableLoop) == false) {
                a->current_frame = 0;
            }
            else {
                if (data->next_id != -1) {
                    *a = AnimationApply_(table, data->next_id, a->flags, a->request);
                }
                else {
                    a->finished = true;
                    a->current_frame = data->frame_count - 1;
                }
            }
        }
    }

    if (a->current_frame >= data->frame_count) {
        a->current_frame = data->frame_count > 0 ? data->frame_count - 1 : 0;
    }

    FanRect current_data = data->frames[a->current_frame];
    TextureUpdate(
        t,
        (FanVector2){ current_data.x,     current_data.y },
        (FanVector2){ current_data.width, current_data.height },
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
	    FanRect zone,
	    int32 flags
    ) {
    if (tx is null) {
        if (FanVector2Length(s->offset) > 0.0f) {
            FanDrawRectV(FanVector2Add(t->position, s->offset), t->scale, (FanColor){ 50, 50, 50, 255 });
        }
        FanDrawRectV(t->position, t->scale, s->color);
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

        FanRect src = (FanRect) {
            tx->rect.x,
            tx->rect.y,
            width,
            height
        };

        FanRect dst = (FanRect) {
            t->position.x + s->offset.x,
            t->position.y + s->offset.y,
            t->scale.x,
            t->scale.y
        };

        if (flags & RenderFlag_ShowInteract) {
            FanColor zone_color = interacting ?
                (FanColor){ 255, 0, 0, 75 } : (FanColor){ 0, 255, 0, 75 };

            FanDrawRectR(zone, zone_color);
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

void RenderEntities(World *world, PlayerInput p_input, float32 dt) {
    RenderEntry render_array[128] = { { -1, 0.0f, 0 } };
    ssize render_entry_count = 0;

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

        int32 tag_bg           = world->c_tag_background.sparse[id];

        int32 interaction_idx  = world->c_interaction.sparse[id];
        int32 interactable_idx = world->c_interactable.sparse[id];
        int32 zone_idx         = world->c_zone.sparse[id];

        CShape       *shape       = &world->c_shape.data[shape_idx];
        CTransform   *transform   = &world->c_transform.data[transform_idx];
        CMovement    *move        = &world->c_movement.data[move_idx];
        CTexture     *texture     = null;
        CAnimation   *animation   = null;
        bool32        interacting = false;
        bool32        interacted  = false;
        FanRect zone              = { 0 };

        if (not shape->visible) {
            printf("skipping %d!\n", id);
            continue;
        }

        int32 render_flags = 0;

        if (zone_idx != -1) {
            zone = world->c_zone.data[zone_idx];
            zone.x += transform->position.x;
            zone.y += transform->position.y;
        }
        else {
            zone = (FanRect) {
                transform->position.x,
                transform->position.y,
                transform->scale.x,
                transform->scale.y,
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
            if (animation_idx != -1) {
                animation = &world->c_animation.data[animation_idx];
            }
        }

        if (move_idx != -1) {
            move = &world->c_movement.data[move_idx];
        }

            FanVector2 center_pos = FanVector2Add(
                transform->position,
                (FanVector2) {
                    transform->scale.x * 0.5f,
                    transform->scale.y * 0.75f,
                }
            );

            FanVector2 map_pos = TileMapGetPosition(world->map, center_pos);
            int32 tile_data = MatrixInt32Get(world->map.tiles, map_pos.x, map_pos.y);


            if (id == world->spec_id.player) {
                if (tile_data == 1) {
                    FanDrawRectR(
                        (FanRect) {
                        map_pos.x * world->map.tile_size,
                        map_pos.y * world->map.tile_size,
                        world->map.tile_size,
                        world->map.tile_size,
                        },
                        FanColor_BLUE
                    );
                }
            }

        render_flags |= RenderFlag_FlipX;

        if (p_input.actions[1])
            render_flags |= RenderFlag_ShowInteract;

        RenderSystem(shape, transform, texture, move, interacting, zone, render_flags);

        if (world->called_object_dump) {
            printf("id: %d\n", id);
            printf("interacting: %s\n", interacting ? "true" : "false");
            printf("interacted: %s\n",  interacted  ? "true" : "false");
            FanRectPrint(zone);
        }
    }
}

// int32 dynamic_entities[128] = { -1 };
// int32 static_entities[128]  = { -1 };
// ssize dynamic_entity_count  = 0;
// ssize static_entity_count   = 0;

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
        PlayerInput p_input,
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
            init_if_null(transform->scale.x, 96.0f);
            init_if_null(transform->scale.y, 96.0f);

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
        int32 zone_idx        = world->c_zone.sparse[id];

        CMovement  *move      = &world->c_movement.data[i];
        CTransform *transform = &world->c_transform.data[transform_idx];
        CBehavior  *behavior  = null;
        FanRect    *zone      = null;

        FanVector2 direction = FanVector2Zero();

        if (zone_idx != -1) {
            zone = &world->c_zone.data[zone_idx];
        }

        if (interact_idx != -1) {
            world->c_interaction.data[interact_idx] = false;
        }

        if (interacted_idx != -1) {
            world->c_interactable.data[interacted_idx] = false;
        }

        if (id == world->spec_id.player) {
            direction = p_input.direction;
        }
        else if (behavior_idx != -1) {
            behavior = &world->c_behavior.data[behavior_idx];
            switch (behavior->type) {
                case BehaviorType_Random: {
                    if (world->current_time - behavior->start_time > behavior->duration) {
                        direction = (FanVector2) {
                            FanRandomInt(-1, 1),
                            FanRandomInt(-1, 1)
                        };
                        behavior->start_time = world->current_time;
                    }
                    else {
                        // keeps entity moving rather than staying still
                        direction = move->direction;
                    }
                } break;
                case BehaviorType_Follow: {
                    CTransform *target_transform = &world->c_transform.data[0];
                    FanVector2 target_face = target_transform->position;
                       if (id == world->spec_id.camera) {
                        CShape *target_shape = &world->c_shape.data[0];
                        target_face = FanVector2Add(target_face, target_shape->offset);
                    }
                    FanVector2 face = FanVector2Normalize(FanVector2Sub(target_face, transform->position));

                    direction = (FanVector2){ signof(face.x), signof(face.y) };
                } break;
                case BehaviorType_None:
                default: {
                } break;
            }
        }

        MovementSystem(move, transform, direction, world->bounding_zone, dt);

        if (world->called_object_dump) {
            printf("\tid: %d\n", id);

            if (id == world->spec_id.player) {
                FanVector2Print(transform->position);
                FanVector2Print(transform->scale);

                if (move) {
                    printf("\t");
                    FanVector2Print(move->velocity);
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

        CAnimation *anim  = &world->c_animation.data[i];
        CTexture *texture = &world->c_texture.data[texture_idx];

        AnimationSystem(anim, texture, world->anim_table, dt);
    }

    for (ssize i = 0; i < split->dynamic_count; i++) {
        int32 id = split->dynamic_entities[i];

        int32 move_idx        = world->c_movement.sparse[id];
        int32 transform_idx   = world->c_transform.sparse[id];
        int32 interact_idx    = world->c_interaction.sparse[id];
        int32 zone_idx        = world->c_zone.sparse[id];

        int32 tag_enemy       = world->c_tag_enemy.sparse[id];

        CMovement  *move      = &world->c_movement.data[move_idx];
        CTransform *transform = &world->c_transform.data[transform_idx];
        bool32     *interact  = null;
        FanRect     zone      = (FanRect) { 0 };

        if (interact_idx != -1) {
            interact = &world->c_interaction.data[interact_idx];
        }

        if (zone_idx != -1) {
            zone = world->c_zone.data[zone_idx];
            zone.x += transform->position.x;
            zone.y += transform->position.y;
        }
        else {
            zone = (FanRect) {
                transform->position.x,
                transform->position.y,
                transform->scale.x,
                transform->scale.y,
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

                CTransform *other_transform  = &world->c_transform.data[other_transform_idx];
                CMovement  *other_move       = &world->c_movement.data[other_move_idx];
                bool32     *other_interacted = null;
                FanRect     other_zone       = (FanRect) { 0 };

                if (other_interacted_idx != -1) {
                    other_interacted = &world->c_interactable.data[other_interacted_idx];
                }

                if (other_zone_idx != -1) {
                    other_zone = world->c_zone.data[other_zone_idx];
                    other_zone.x += other_transform->position.x;
                    other_zone.y += other_transform->position.y;
                }
                else {
                    other_zone = (FanRect) {
                        other_transform->position.x,
                        other_transform->position.y,
                        other_transform->scale.x,
                        other_transform->scale.y,
                    };
                }

                CollisionSystem(transform, move, other_transform, other_move, dt);
                if (
                        interact and
                        other_interacted and
                        CollisionCheckR(zone, other_zone) and
                        not (istagged(tag_enemy) and istagged(other_tag_enemy))
                    ) {
                    // FanRectPrint(zone);
                    // FanRectPrint(other_zone);
                    *interact = true;
                    *other_interacted = true;
                }
            }
            for (ssize i = 0; i < split->static_count; i++) {
                int32 other_id = split->static_entities[i];

                int32 other_transform_idx    = world->c_transform.sparse[other_id];
                int32 other_move_idx         = world->c_movement.sparse[other_id];
                assert(other_move_idx != -1);

                CTransform *other_transform  = &world->c_transform.data[other_transform_idx];
                CMovement  *other_move       = &world->c_movement.data[other_move_idx];

                CollisionSystem(transform, move, other_transform, other_move, dt);
            }
        }

        if (world->called_object_dump) {
            printf("\tid: %d\n", id);

            if (id == world->spec_id.player) {
                printf("\t");
                FanVector2Print(transform->position);
                printf("\t");
                FanVector2Print(transform->scale);

                if (move) {
                    printf("\t");
                    FanVector2Print(move->velocity);
                    printf("\t");
                    FanVector2Print(move->direction);
                    printf("\tmove->speed: %f\n", move->speed);
                }
                printf("\t");
                FanRectPrint(zone);
            }
        }
    }
}




FanRect player_idle_up_frames[1]    = { 0 };
FanRect player_idle_down_frames[3]  = { 0 };
FanRect player_idle_left_frames[3]  = { 0 };
FanRect player_idle_right_frames[3] = { 0 };

AnimationData anim_table[] = {
    { "player_idle_down",  player_idle_down_frames,  .frame_time = 0.5f, .frame_count = 3, true,  -1 },
    { "player_idle_up",    player_idle_up_frames,    .frame_time = 1.5f, .frame_count = 1, false,  0 },
    { "player_idle_left",  player_idle_left_frames,  .frame_time = 0.5f, .frame_count = 3, false,  0 },
    { "player_idle_right", player_idle_right_frames, .frame_time = 0.5f, .frame_count = 3, false,  0 },
};

global void SceneSolo(World *world) {
    FanTexture tex_link = FanTextureLoad("./resources/link.png");
    FanVector2 sprite_link_size = (FanVector2){ tex_link.width / 10.0f, tex_link.height / 8.0f };
    player_idle_down_frames[0]  = (FanRect){ 0,                         0,                         0, 0 };
    player_idle_down_frames[1]  = (FanRect){ sprite_link_size.x,        0,                         0, 0 };
    player_idle_down_frames[2]  = (FanRect){ 2.0f * sprite_link_size.x, 0,                         0, 0 };

    player_idle_up_frames[0]    = (FanRect){ 0,                         2.0f * sprite_link_size.y, 0, 0 };

    player_idle_left_frames[0]  = (FanRect){ 0,                         sprite_link_size.y,        0, 0 };
    player_idle_left_frames[1]  = (FanRect){ sprite_link_size.x,        sprite_link_size.y,        0, 0 };
    player_idle_left_frames[2]  = (FanRect){ 2.0f * sprite_link_size.x, sprite_link_size.y,        0, 0 };

    player_idle_right_frames[0] = (FanRect){ 0,                         3.0f * sprite_link_size.y, 0, 0 };
    player_idle_right_frames[1] = (FanRect){ sprite_link_size.x,        3.0f * sprite_link_size.y, 0, 0 };
    player_idle_right_frames[2] = (FanRect){ 2.0f * sprite_link_size.x, 3.0f * sprite_link_size.y, 0, 0 };

    ComponentAdd(&world->c_transform,     world->entity_count);
    ComponentAdd(&world->c_shape,         world->entity_count);
    ComponentAddArgs(&world->c_movement,  world->entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    ComponentAddArgs(&world->c_texture,   world->entity_count,
        .texture = tex_link,
        .rect = (FanRect){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
    );
    ComponentAddArgs(&world->c_animation, world->entity_count);
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

    ComponentAdd(&world->c_transform,     world->entity_count);
    ComponentAdd(&world->c_shape,         world->entity_count);
    ComponentAddArgs(&world->c_movement,  world->entity_count,
        // .flags = MovementFlag_CollideSoftly
    );
    ComponentAddArgs(&world->c_texture,   world->entity_count,
        .texture = tex_sprite,
        .rect = { .x = 64, .y = 0, .width = 14, .height = 16 },
        // .rect = (FanRect){ 0, 0, tex_link.width / 10.0f, tex_link.height / 8.0f }
    );
    // ComponentAddArgs(&world->c_animation, world->entity_count);
    ComponentAdd(&world->c_interaction, world->entity_count);
    ComponentAdd(&world->c_interactable, world->entity_count);
    world->spec_id.player = world->entity_count;
    world->entity_count++;

    // FanTexture tex_mewee = FanTextureLoad("./resources/mewee.png");
    ComponentAddArgs(&world->c_transform,    world->entity_count,
        .scale = (FanVector2){ 108, 108 },
    );
    ComponentAddArgs(&world->c_shape,    world->entity_count,
        .color = (FanColor){ 50, 255, 255, 255 },
        .offset = (FanVector2){ 0, 6 },
    );
    ComponentAddArgs(&world->c_movement, world->entity_count, .speed = 100.0f);
    ComponentAddArgs(&world->c_texture,  world->entity_count,
        .texture = tex_sprite,
        .rect = { .width = 36, .height = 36 },
    );
    ComponentAddArgs(&world->c_behavior, world->entity_count,
        .type = BehaviorType_Follow,
    );
    ComponentAdd(&world->c_interaction,  world->entity_count);
    ComponentAdd(&world->c_interactable, world->entity_count);
    ComponentAddArgs(&world->c_zone,     world->entity_count,
        .x = 12, .y = 12, .width = 64, .height = 64,
    );
    ComponentAdd(&world->c_tag_enemy,    world->entity_count);
    world->entity_count++;

    // ComponentAdd(&world->c_transform,    world->entity_count);
    // ComponentAddArgs(&world->c_shape,    world->entity_count,
    //     .color = (FanColor){ 255, 50, 255, 255 },
    // );
    // ComponentAddArgs(&world->c_movement, world->entity_count, .speed = 150.0f);
    // ComponentAddArgs(&world->c_texture,  world->entity_count,
    //     .texture = tex_sprite,
    //     .rect = { .x = 64, .y = 0, .width = 14, .height = 16 },
    // );
    // ComponentAddArgs(&world->c_behavior, world->entity_count,
    //     .type = BehaviorType_Follow,
    // );
    // ComponentAdd(&world->c_interaction,  world->entity_count);
    // ComponentAdd(&world->c_interactable, world->entity_count);
    // ComponentAdd(&world->c_tag_enemy,    world->entity_count);
    // world->entity_count++;

    ComponentAddArgs(&world->c_transform,  world->entity_count,
        .scale = (FanVector2){ FanWindowWidth() * 2, FanWindowHeight() * 2 },
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

    ComponentAddArgs(&world->c_transform, world->entity_count,
        .position = (FanVector2){ 200.0f, 300.0f },
        .scale = (FanVector2){ 400.0f, 150.0f }
    );
    ComponentAddArgs(&world->c_shape,     world->entity_count,
        .color = (FanColor){ 200, 165, 175, 255 },
        .layer = 3,
    );
    ComponentAddArgs(&world->c_movement,  world->entity_count,
        .flags = MovementFlag_NoCollision,
    );
    world->entity_count++;

    ComponentAddArgs(&world->c_transform, world->entity_count,
        .position = (FanVector2){ 200, 100 },
    );
    ComponentAddArgs(&world->c_shape,     world->entity_count,
        .color = (FanColor){ 50, 255, 255, 255 },
    );
    ComponentAddArgs(&world->c_movement,  world->entity_count,
        .flags = MovementFlag_Immovable,
    );
    world->entity_count++;

    ComponentAddArgs(&world->c_transform, world->entity_count,
        .position = (FanVector2){ 296, 100 },
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
        .speed = 380.0f,
        .flags = MovementFlag_NoCollision,
    );
    ComponentAddArgs(&world->c_behavior, world->entity_count,
        .type = BehaviorType_Follow,
    );
    world->spec_id.camera = world->entity_count;
    world->entity_count++;

}

FanMusic muse = { 0 };
void GameInit(Allocator *a, World *world) {

    int32 split_size = kilobytes(1);
    ssize component_size  = kilobytes(1);

    ComponentStorageCreate(&world->c_transform,      a, component_size);
    ComponentStorageCreate(&world->c_shape,          a, component_size);
    ComponentStorageCreate(&world->c_movement,       a, component_size);
    ComponentStorageCreate(&world->c_texture,        a, component_size);
    ComponentStorageCreate(&world->c_behavior,       a, component_size);
    ComponentStorageCreate(&world->c_animation,      a, component_size);
    ComponentStorageCreate(&world->c_physics,        a, component_size);

    ComponentStorageCreate(&world->c_interaction,    a, component_size);
    ComponentStorageCreate(&world->c_interactable,   a, component_size);
    ComponentStorageCreate(&world->c_zone,           a, component_size);

    ComponentStorageCreate(&world->c_tag_background, a, component_size);
    ComponentStorageCreate(&world->c_tag_enemy,      a, component_size);

    world->split.dynamic_entities = a->make(a->ctx, split_size);
    world->split.dynamic_capacity = split_size;

    world->split.static_entities  = a->make(a->ctx, split_size);
    world->split.static_capacity  = split_size;

    SceneMain(world);

    world->bounding_zone = (FanRect){ .width = FanWindowWidth() * 2, .height = FanWindowHeight() * 2 };

    muse = FanMusicLoad("./resources/My Uncles Last Voyage.mp3");
    FanMusicPlay(muse);
    FanMusicSetVolume(muse, 0.6f);

    TileMap map = (TileMap) {
        .tile_size = 96,
		.tiles = MatrixInt32Create(a, 10, 10, 1),
    };

	world->map = map;

    printf("Successfully passed initialization!\n");
}

void GameUpdateAndRender(Allocator *a, World *world, PlayerInput p_input, float32 dt) {
    (void)a;

    FanMusicUpdate(muse);

    UpdateEntities(world, p_input, dt);
    RenderEntities(world, p_input, dt);
}

void GameClose(Allocator *a, World *world) {
    for (ssize i = 0; i < world->c_texture.size; i++) {
        if (world->c_texture.dense[i] == -1) {
            continue;
        }
        FanTextureUnload(world->c_texture.data[i].texture);
    }

    a->free(a->ctx, world->split.dynamic_entities, world->split.dynamic_capacity);
    a->free(a->ctx, world->split.static_entities, world->split.static_capacity);
}
