#include <pspctrl.h>
#include <engine_control.h>
#include <stdlib.h>

#define DEADZONE 30

static int press_up = 0;
static int press_down = 0;
static int press_left = 0;
static int press_right = 0;
static int press_square = 0;
static int press_triangle = 0;
static int press_circle = 0;
static int press_cross = 0;
static int press_start = 0;
static int press_select = 0;
static int press_ltrigger = 0;
static int press_rtrigger = 0;
static vec2 analog = {0.0f, 0.0f};

void
controlInit(void)
{
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

static inline float
_convertAxis(unsigned char value)
{
    int ival = ((int)value) - 128;
    if(abs(ival) < DEADZONE) return 0.0f;
    return ((float)ival) / 128.0f;
}

void
controlUpdate(void)
{
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);
    press_up = (pad.Buttons & PSP_CTRL_UP);
    press_down = (pad.Buttons & PSP_CTRL_DOWN);
    press_left = (pad.Buttons & PSP_CTRL_LEFT);
    press_right = (pad.Buttons & PSP_CTRL_RIGHT);
    press_square = (pad.Buttons & PSP_CTRL_SQUARE);
    press_triangle = (pad.Buttons & PSP_CTRL_TRIANGLE);
    press_circle = (pad.Buttons & PSP_CTRL_CIRCLE);
    press_cross = (pad.Buttons & PSP_CTRL_CROSS);
    press_start = (pad.Buttons & PSP_CTRL_START);
    press_select = (pad.Buttons & PSP_CTRL_SELECT);
    press_ltrigger = (pad.Buttons & PSP_CTRL_LTRIGGER);
    press_rtrigger = (pad.Buttons & PSP_CTRL_RTRIGGER);
    analog.x = _convertAxis(pad.Lx);
    analog.y = _convertAxis(pad.Ly);
}

int
isPressing(enum ControlBtn btn)
{
    switch(btn) {
    case CTRLUP: return press_up;
    case CTRLDOWN: return press_down;
    case CTRLLEFT: return press_left;
    case CTRLRIGHT: return press_right;
    case CTRLSQUARE: return press_square;
    case CTRLTRIANGLE: return press_triangle;
    case CTRLCIRCLE: return press_circle;
    case CTRLCROSS: return press_cross;
    case CTRLSTART: return press_start;
    case CTRLSELECT: return press_select;
    case CTRLLT: return press_ltrigger;
    case CTRLRT: return press_rtrigger;
    }
    return 0;
}

vec2
getAnalog(void)
{
    return analog;
}
