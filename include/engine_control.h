#ifndef ENGINE_CONTROL_H
#define ENGINE_CONTROL_H

#include <engine_types.h>

enum ControlBtn
{
    CTRLUP,
    CTRLDOWN,
    CTRLLEFT,
    CTRLRIGHT,
    CTRLSQUARE,
    CTRLTRIANGLE,
    CTRLCIRCLE,
    CTRLCROSS,
    CTRLSTART,
    CTRLSELECT,
    CTRLLT,
    CTRLRT,
};

void controlInit(void);
void controlUpdate(void);

int  isPressing(enum ControlBtn);
vec2 getAnalog(void);

#endif
