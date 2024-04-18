#ifndef ENGINE_TYPES_H
#define ENGINE_TYPES_H

typedef struct {
    float x, y;
} vec2;

typedef struct {
    unsigned short u, v;
    short x, y, z;
} vertex;

typedef vec2 Position;
typedef vec2 Velocity;

#endif
