#ifndef FAN_GAME_UI_H
#define FAN_GAME_UI_H

#include "os.h"
#include "platform.h"

typedef struct fan_element {
    uint32 flags;
    uint32 childCount;
    struct fan_element *parent;
    struct fan_element **children;
    void *cp;
} fan_element;

// fan_element *fan_element_create(fan_allocator *mem, ssize bytes, fan_element *parent, uint32 flags) {
//     fan_element *element = (fan_element *) mem->make(mem->ctx, bytes);
//     element->flags = flags;
//
//     if (parent) {
//         element->parent = parent;
//         parent->childCount++;
//         // TODO(liam): continue here...
//     }
// }

#endif // FAN_GAME_UI_H
