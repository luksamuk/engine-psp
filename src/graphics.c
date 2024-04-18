#include <engine_graphics.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <pspdebug.h>

#include <engine_types.h>

char      displaylist[0x20000]  __attribute__((aligned(64)));

void
initGu(void)
{
    sceGuInit();
    sceGuStart(GU_DIRECT, displaylist);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUFFER_WIDTH);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, (void*)0x88000, BUFFER_WIDTH);
    sceGuDepthBuffer((void*)0x110000, BUFFER_WIDTH);
    sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuDepthRange(65535, 0);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuFinish();
    sceGuDisplay(GU_TRUE);

    pspDebugScreenInit();
    pspDebugScreenClear();
}

void
disposeGu(void)
{
    sceGuDisplay(GU_FALSE);
    sceGuTerm();
}

void
guRenderStart(void)
{
    sceGuStart(GU_DIRECT, displaylist);
    sceGuClearColor(0xff000000); // ABGR
    sceGuClear(GU_COLOR_BUFFER_BIT);
}

void
guRenderEnd(void)
{
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

void
drawRect(float x, float y, float w, float h)
{
    // This is not permanent memory allocation. The memory will be invalid
    // as soon as the same display list starts being filled again, so no
    // need to deallocate stuff here
    vertex *vertices = (vertex*)sceGuGetMemory(2 * sizeof(vertex));

    vertices[0].x = x;
    vertices[0].y = y;
    vertices[1].x = x + w;
    vertices[1].y = y + h;

    sceGuColor(0xff0000ff); // red (ABGR)
    sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, 0, vertices);
}

