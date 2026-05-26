
#include "game.h"
#include "os.h"
#include "platform.h"

CAnimation AnimationApply_(AnimationData *table, int32 new_id, int32 flags, AnimationRequest ar);

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

GridAtlas GridAtlasCreate(int32 tile_size) {
    GridAtlas atlas = { 0 };

    atlas.sizes[0] = 0.0f;
    atlas.sizes[1] = (float32)tile_size;
    atlas.sizes[2] = 2.0f * (float32)tile_size;
    atlas.sizes[3] = 3.0f * (float32)tile_size;

    atlas.coordinates[0]  = (fan_vec2){ 0.0f, 0.0f };
    atlas.coordinates[1]  = (fan_vec2){ 1.0f, 0.0f };
    atlas.coordinates[2]  = (fan_vec2){ 2.0f, 0.0f };
    atlas.coordinates[3]  = (fan_vec2){ 3.0f, 0.0f };
    atlas.coordinates[4]  = (fan_vec2){ 0.0f, 1.0f };
    atlas.coordinates[5]  = (fan_vec2){ 1.0f, 1.0f };
    atlas.coordinates[6]  = (fan_vec2){ 2.0f, 1.0f };
    atlas.coordinates[7]  = (fan_vec2){ 3.0f, 1.0f };
    atlas.coordinates[8]  = (fan_vec2){ 0.0f, 2.0f };
    atlas.coordinates[9]  = (fan_vec2){ 1.0f, 2.0f };
    atlas.coordinates[10] = (fan_vec2){ 2.0f, 2.0f };
    atlas.coordinates[11] = (fan_vec2){ 3.0f, 2.0f };
    atlas.coordinates[12] = (fan_vec2){ 0.0f, 3.0f };
    atlas.coordinates[13] = (fan_vec2){ 1.0f, 3.0f };
    atlas.coordinates[14] = (fan_vec2){ 2.0f, 3.0f };
    atlas.coordinates[15] = (fan_vec2){ 3.0f, 3.0f };

    return atlas;
}

/*
 * STRICTLY returns a texture space coordinate and size.
 */
fan_rect GridAtlasGetRect(GridAtlas atlas, int32 index) {
    assert(index > -1 && "atlas index must be greater than -1.");
    assert(index < 16 && "atlas index must be less than 16.");
    fan_vec2 size_index = atlas.coordinates[index];
    fan_rect rect = (fan_rect){
        .x = atlas.sizes[(int32)size_index.x],
        .y = atlas.sizes[(int32)size_index.y],
        .w = atlas.sizes[1],
        .h = atlas.sizes[1]
    };
    return rect;
}

TileOffsets GridGetLogicalFromVisual(fan_vec2 v, int32 rows, int32 cols) {
    TileOffsets result;

    result.tl.x = (float32)fan_i32_clamp((int32)v.x, 0, cols - 1);
    result.tl.y = (float32)fan_i32_clamp((int32)v.y, 0, rows - 1);

    result.tr.x = (float32)fan_i32_clamp((int32)v.x + 1, 0, cols - 1);
    result.tr.y = result.tl.y;

    result.bl.x = result.tl.x;
    result.bl.y = (float32)fan_i32_clamp((int32)v.y + 1, 0, rows - 1);

    result.br.x = result.tr.x;
    result.br.y = result.bl.y;

    return result;
}

TileOffsets GridGetVisualFromLogical(fan_vec2 v, int32 rows, int32 cols) {
    TileOffsets result;

    result.br.x = (float32)fan_i32_clamp((int32)v.x, 0, cols - 1);
    result.br.y = (float32)fan_i32_clamp((int32)v.y, 0, rows - 1);

    result.tr.x = result.br.x;
    result.tr.y = (float32)fan_i32_clamp((int32)v.y - 1, 0, rows - 1);

    result.bl.x = (float32)fan_i32_clamp((int32)v.x - 1, 0, cols - 1);
    result.bl.y = result.br.y;

    result.tl.x = result.bl.x;
    result.tl.y = result.tr.y;

    return result;
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

void GridWorldGenerate(TileMap map) {
    for (int32 i = 0; i < map.visual_tiles.rows; i++) {
        for (int32 j = 0; j < map.visual_tiles.cols; j++) {
            TileOffsets offsets = GridGetLogicalFromVisual((fan_vec2){ (float32)j, (float32)i },
                                                           (int32)map.visual_tiles.rows,
                                                           (int32)map.visual_tiles.cols);

            // TODO(liam): get the atlas indexing correct.
            int32 bits = 0;
            if (fan_matrix_at(map.logic_tiles, offsets.tl.x, offsets.tl.y)) {
                bits |= (1 << 0);
            }
            if (fan_matrix_at(map.logic_tiles, offsets.tr.x, offsets.tr.y)) {
                bits |= (1 << 1);
            }
            if (fan_matrix_at(map.logic_tiles, offsets.bl.x, offsets.bl.y)) {
                bits |= (1 << 2);
            }
            if (fan_matrix_at(map.logic_tiles, offsets.br.x, offsets.br.y)) {
                bits |= (1 << 3);
            }

            fan_matrix_at(map.visual_tiles, j, i) = bits;
            fan_log_debug("%d\t", bits);
            // fan_rect src = GridAtlasGetRect(atlas, bits);
            // fan_rect dst = (fan_rect) {
            //     0, 0, map.tile_size, map.tile_size
            // };
            //
            //
            // fan_draw_texture(tile_texture, src, dst, fan_vec2_zero(), 0.0f, fan_color_WHITE);
        }
        fan_log_debug("\n");
    }
}

void GridWorldDraw(TileMap map, GridAtlas atlas, int32 pixels_per_unit, fan_texture tile_texture) {
    for (int32 i = 0; i < map.visual_tiles.rows; i++) {
        for (int32 j = 0; j < map.visual_tiles.cols; j++) {
            int32 bits = fan_matrix_at(map.visual_tiles, i, j);

            fan_rect src = GridAtlasGetRect(atlas, bits);
            // fan_rect src = (fan_rect) {
            //     48, 48, 16, 16
            // };
            fan_rect dst = (fan_rect) {
                .x = (float32)(i * pixels_per_unit),
                .y = (float32)(j * pixels_per_unit),
                .w = (float32)pixels_per_unit,
                .h = (float32)pixels_per_unit
            };

            fan_draw_texture(tile_texture, src, dst, fan_vec2_zero(), 0.0f, fan_color_WHITE);
        }
    }
}

/*
 * type: System
 * component(s): Transform, Shape, Texture (opt), Physics (opt)
 */
void RenderSystem(
        CShape *s,
	    CTransform *t,
	    CTexture *tx,
	    CMovement *m,
        CText *ts,
	    bool32 interacting,
	    fan_rect_f32 zone,
        fan_vec2 camera_position,
        float32 camera_zoom,
        int32 pixels_per_unit,
        fan_vec2 render_size,
	    int32 flags
    ) {
    fan_vec2 screen_pos = WorldToScreen(
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
            fan_draw_rectv(fan_vec2_add(screen_pos, screen_offset), screen_scale, fan_vec2_zero(), 0.0f, (fan_color){ 50, 50, 50, 255 });
        }
        fan_draw_rectv(screen_pos, screen_scale, fan_vec2_zero(), 0.0f, s->color);
    }
    else {
        float32 width  = (tx->rect.w) ? (float32)tx->rect.w : (float32)tx->texture.width;
        float32 height = (tx->rect.h) ? (float32)tx->rect.h : (float32)tx->texture.height;

        if (m) {
            if (flags & RenderFlag_FlipX) {
                width  *= m->direction.x ? m->direction.x : 1.0f;
            }
            if (flags & RenderFlag_FlipY) {
                height *= m->direction.y ? m->direction.y : 1.0f;
            }
        }

        fan_rect src = (fan_rect) {
            .x = tx->rect.x,
            .y = tx->rect.y,
            .w = width,
            .h = height
        };

        fan_rect dst = (fan_rect) {
            fan_f32_round(screen_pos.x + screen_offset.x),
            fan_f32_round(screen_pos.y + screen_offset.y),
            screen_scale.x,
            screen_scale.y
        };

        fan_draw_texture(
            tx->texture,
            src,
            dst,
            fan_vec2_zero(),
            t->rotation,
            s->color
        );

        if (flags & RenderFlag_ShowInteract) {
            fan_vec2 screen_zone_pos = WorldToScreen(
                (fan_vec2){ zone.x, zone.y },
                camera_position,
                camera_zoom,
                pixels_per_unit,
                render_size
            );

            float32 width_px = (float32)zone.w * (float32)pixels_per_unit * camera_zoom;
            float32 height_px = (float32)zone.h * (float32)pixels_per_unit * camera_zoom;

            fan_rect screen_zone = (fan_rect) {
                screen_zone_pos.x,
                screen_zone_pos.y,
                width_px,
                height_px
            };
            fan_color zone_color = interacting ?
                (fan_color){ 255, 0, 0, 75 } : (fan_color){ 0, 255, 0, 75 };

            fan_draw_rectr(screen_zone, zone_color);
        }

    }

    if (ts) {
        // TODO: add api call here
        fan_draw_text(
            "TESTING",
            (fan_vec2) {
                fan_f32_round(screen_pos.x),
                fan_f32_round(screen_pos.y - (screen_scale.y * 0.25f))
            },
            14,
            fan_color_BLACK,
            fan_font_DEFAULT
        );

    }
}

void RenderProcessPost(
    fan_rtexture map,
    int32 window_width,
    int32 window_height
) {
    float32 target_width = (float32)map.texture.width;
    float32 target_height = (float32)map.texture.height;

    float32 scale = min(
        (float32)window_width  / target_width,
        (float32)window_height / target_height
    );

    float32 render_width  = target_width  * scale;
    float32 render_height = target_height * scale;

    float32 offset_x = ((float32)window_width  - render_width)  * 0.5f;
    float32 offset_y = ((float32)window_height - render_height) * 0.5f;

    fan_draw_clear(fan_color_BLACK);

    fan_draw_texture(
        map.texture,
        (fan_rect){ 0, 0, target_width, -target_height },
        (fan_rect){ offset_x, offset_y, render_width, render_height },
        fan_vec2_zero(),
        0.0f,
        fan_color_WHITE
    );
}

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

void TextureUpdate(CTexture *tx, fan_vec2 pos, fan_vec2 size, float32 dt) {
    (void)dt;

    tx->rect = (fan_rect){
        .x = (int32)pos.x,
        .y = (int32)pos.y,
        .w = (int32)size.x,
        .h = (int32)size.y
    };
}

/*
 * type: System
 * components: CAnimation, CTexture
 */
void AnimationSystem(CAnimation *a, CTexture *t, AnimationData *table, float dt) {
    if (table is null or a is null or t is null) return;
    if (a->finished or
        (not a->finished and a->request.id != -1 and a->request.id != a->id and (a->flags & AnimationFlag_NotInterruptible) == false)) {
        *a = AnimationApply_(table, a->request.id, a->request.flags, (AnimationRequest){ -1, 0 });
    }

    AnimationData *data = &table[a->id];
    if (data->frame_count <= 0) {
        return;
    }

    a->timer += dt;
    // fan_log_debug("a->timer: %f\n", a->timer);
    // fan_log_debug("data->frame_time: %f\n", data->frame_time);
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
            coalesce((float32)current.w, (float32)t->rect.w),
            coalesce((float32)current.h, (float32)t->rect.h)
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
    float32 target_width = (float32)lightmap.texture.width;
    float32 target_height = (float32)lightmap.texture.height;

    float32 scale = min(
                        (float32)window_width  / target_width,
                        (float32)window_height / target_height
                       );

    float32 render_width  = target_width  * scale;
    float32 render_height = target_height * scale;

    float32 offset_x = ((float32)window_width  - render_width)  * 0.5f;
    float32 offset_y = ((float32)window_height - render_height) * 0.5f;

    fan_mode_blend_begin(FanBlend_MULTIPLIED);

        fan_draw_texture(
            lightmap.texture,
            (fan_rect){ 0, 0, (float32)lightmap.texture.width, (float32)-lightmap.texture.height },
            (fan_rect){ offset_x, offset_y, render_width, render_height },
            fan_vec2_zero(),
            0.0f,
            fan_color_WHITE
        );

    fan_mode_blend_end();
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

        CShape     *shape     = fan_component_get_fast(&world->c_shape, id);
        CTransform *transform = fan_component_get(&world->c_transform, id);
        if (transform == null)
            continue;

        if (not shape->initialized) {
            if (shape->color.a == 0) {
                shape->color = fan_color_WHITE;
            }
            init_if_null(shape->layer, 1);

            shape->visible = true;
            shape->initialized = true;
        }

        render_array[render_entry_count++] = (RenderEntry) {
            (int32)id,
            transform->position.y + transform->scale.y,
            shape->layer
        };
    }

    if (render_entry_count > 1) {
        SortRender(render_array, 0, render_entry_count - 1);
    }

    fan_mode_texture_begin(state->rendermap);
        fan_draw_clear(fan_color_WHITE);

        // NOTE: drawing world grid here!!

        // for (ssize i = 0; i < render_entry_count; i++) {
        for (ssize i = render_entry_count - 1; i >= 0; i--) {
            ssize id = render_array[i].id;
            if (id == -1)
                continue;

            CShape         *shape       = fan_component_get_fast(&world->c_shape, id);
            CTransform     *transform   = fan_component_get_fast(&world->c_transform, id);
            CMovement      *move        = fan_component_get(&world->c_movement, id);
            CTexture       *texture     = fan_component_get(&world->c_texture, id);
            CAnimation     *animation   = fan_component_get(&world->c_animation, id);
            CAttack        *attack      = fan_component_get(&world->c_attack, id);
            bool32          interacting = fan_component_get_value(&world->c_interaction, id);
            bool32          interacted  = fan_component_get_value(&world->c_interactable, id);
            CCollision     *collision   = fan_component_get(&world->c_collision, id);
            // fan_rect        zone        = fan_component_get_value_or_else(&world->c_zone, id, (fan_rect){ 0 });
            CText          *text        = fan_component_get(&world->c_text, id);

            if (not shape->visible) {
                continue;
            }

            fan_vec2 center_pos = fan_vec2_add(
                transform->position,
                (fan_vec2) {
                    transform->scale.x * 0.5f,
                    transform->scale.y * 0.5f,
                }
            );

            fan_vec2 map_pos = TileMapGetPosition(world->map, center_pos);
            int32 tile_data = fan_matrix_at(world->map.logic_tiles, map_pos.x, map_pos.y);

            // if (id == world->spec_id.tilemap) {
            //     fan_mode_texture_begin(state->tilemap);
            //         fan_draw_clear(fan_color_WHITE);
            //         GridWorldDraw(world->map, world->tile_atlas, state->world_scale, world->tilesets[0]);
            //     fan_mode_texture_end();
            //
            //     fan_draw_texture(
            //         state->tilemap.texture,
            //         (fan_rect){ 0, 0, (float32)state->tilemap.texture.width, (float32)-state->tilemap.texture.height },
            //         (fan_rect){ transform->position.x, transform->position.y, transform->scale.x, transform->scale.y },
            //         fan_vec2_zero(),
            //         0.0f,
            //         fan_color_WHITE
            //     );
                // continue;
            // }
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

            int32 render_flags = 0;
            if (animation == null)
                render_flags |= RenderFlag_FlipX;

            if (state->player_input.actions[1])
                render_flags |= RenderFlag_ShowInteract;

            fan_rect actual_collision = {};
            if (collision) {
                actual_collision = CollisionAdjusted(collision->boundary, transform->position);
            }

            RenderSystem(
                shape,
                transform,
                texture,
                move,
                text,
                interacting || interacted,
                actual_collision,
                camera_position,
                camera_zoom,
                pixels_per_unit,
                state->render_size,
                render_flags
            );


            if (attack && attack->attacking && attack->cast_timer <= 0.0f) {
                fan_vec2 center = fan_vec2_add(transform->position,
                                               (fan_vec2){ transform->scale.x * 0.5f, transform->scale.y * -0.5f });
                fan_vec2 facing = fan_vec2_normalize(move->direction);
                float32 length     = attack->attack_range;
                float32 arc        = attack->arc_angle;
                float32 half_width = 0.3f; // sword width, adjust as needed

                float32 progress = attack->timer / attack->swing_time;

                // Current swing angle: sweep from -arc/2 to +arc/2 around facing
                float32 current_angle = -arc * 0.5f + arc * progress;
                fan_vec2 sword_dir = fan_vec2_rotate(facing, current_angle);
                sword_dir = fan_vec2_normalize(sword_dir);

                // fan_vec2 perp = (fan_vec2){ -sword_dir.y, sword_dir.x };

                float32 rotation = -fan_f32_atan2(sword_dir.y, sword_dir.x);
                fan_vec2 rect_center = {
                    center.x + sword_dir.x * (length * 0.5f),
                    center.y + sword_dir.y * (length * 0.5f),
                };

                fan_vec2 rect_pos = WorldToScreen(rect_center, camera_position, camera_zoom, pixels_per_unit, state->render_size);
                fan_vec2 rect_scale = {
                    length * camera_zoom * (float32)pixels_per_unit,
                    half_width * 2.0f * camera_zoom * (float32)pixels_per_unit,
                };


                fan_draw_rectv(rect_pos, rect_scale, fan_vec2_scale(rect_scale, 0.5f), rotation, fan_color_RED);
                // fan_draw_linev(sc[0], sc[1], fan_color_RED);
                // fan_draw_linev(sc[1], sc[2], fan_color_RED);
                // fan_draw_linev(sc[2], sc[3], fan_color_RED);
                // fan_draw_linev(sc[3], sc[0], fan_color_RED);
            }

            if (state->player_called_object_dump) {
                fan_log_debug("id: %td\n", id);
                fan_log_debug("interacting: %s\n", interacting ? "true" : "false");
                fan_log_debug("interacted: %s\n",  interacted  ? "true" : "false");
                fan_rect_print(collision->boundary);
                fan_log_debug("\t");
                fan_vec2_print(transform->position);
                fan_log_debug("\t");
                fan_vec2_print(transform->scale);
            }
        }
    fan_mode_texture_end();

    RenderProcessPost(state->rendermap, state->window_width, state->window_height);
    fan_color ambient = { 255, 255, 255, 255 };
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
