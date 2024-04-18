#include <engine_error.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>

void
error(char *msg)
{
    SceCtrlData pad;
    pspDebugScreenClear();
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf(msg);
    pspDebugScreenPrintf("Press X to quit.\n");
    while(1) {
        sceCtrlReadBufferPositive(&pad, 1);
        if (pad.Buttons & PSP_CTRL_CROSS)
            break;
        sceDisplayWaitVblankStart();
    }
    sceKernelExitGame();
}
