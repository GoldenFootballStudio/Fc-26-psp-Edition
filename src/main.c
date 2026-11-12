
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>

PSP_MODULE_INFO("FC26 3D Pitch", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define W 480
#define H 272
#define BW 512
#define FRAME_SIZE (BW * H * 4)

static unsigned int __attribute__((aligned(16))) list[262144];

typedef struct {
    unsigned int color;
    float x, y, z;
} Vertex;

static void drawQuad(float x1, float z1,
                     float x2, float z2,
                     unsigned int color)
{
    Vertex *v = sceGuGetMemory(4 * sizeof(Vertex));

    v[0] = (Vertex){color, x1, 0.0f, z1};
    v[1] = (Vertex){color, x2, 0.0f, z1};
    v[2] = (Vertex){color, x1, 0.0f, z2};
    v[3] = (Vertex){color, x2, 0.0f, z2};

    sceGumDrawArray(
        GU_TRIANGLE_STRIP,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        4, 0, v
    );
}

static void drawLine(float x1, float z1,
                     float x2, float z2)
{
    unsigned int white = GU_RGBA(255,255,255,255);

    Vertex *v = sceGuGetMemory(2 * sizeof(Vertex));

    v[0] = (Vertex){white, x1, 0.035f, z1};
    v[1] = (Vertex){white, x2, 0.035f, z2};

    sceGumDrawArray(
        GU_LINES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        2, 0, v
    );
}

static void drawCircle(float radius)
{
    for (int i = 0; i < 64; i++) {
        float a = i * 6.2831853f / 64.0f;
        float b = (i + 1) * 6.2831853f / 64.0f;

        drawLine(
            radius * __builtin_cosf(a),
            radius * __builtin_sinf(a),
            radius * __builtin_cosf(b),
            radius * __builtin_sinf(b)
        );
    }
}

static void drawPitch(void)
{
    unsigned int grass1 = GU_RGBA(30, 115, 42, 255);
    unsigned int grass2 = GU_RGBA(43, 145, 55, 255);

    for (int i = 0; i < 8; i++) {
        float z1 = -16.0f + i * 4.0f;
        float z2 = z1 + 4.0f;

        drawQuad(-10.0f, z1, 10.0f, z2,
                 (i % 2) ? grass1 : grass2);
    }

    // Outer boundary
    drawLine(-10,-16, 10,-16);
    drawLine(-10, 16, 10, 16);
    drawLine(-10,-16,-10,16);
    drawLine(10,-16,10,16);

    // Halfway line
    drawLine(-10,0,10,0);

    // Centre circle
    drawCircle(2.2f);

    // Centre spot
    drawCircle(0.08f);

    // Penalty areas
    drawLine(-7,-16,-7,-12);
    drawLine(7,-16,7,-12);
    drawLine(-7,-12,7,-12);

    drawLine(-7,16,-7,12);
    drawLine(7,16,7,12);
    drawLine(-7,12,7,12);

    // Goal areas
    drawLine(-3,-16,-3,-14);
    drawLine(3,-16,3,-14);
    drawLine(-3,-14,3,-14);

    drawLine(-3,16,-3,14);
    drawLine(3,16,3,14);
    drawLine(-3,14,3,14);
}

int main(void)
{
    sceGuInit();

    sceGuStart(GU_DIRECT, list);

    sceGuDrawBuffer(GU_PSM_8888, (void *)0, BW);
    sceGuDispBuffer(W, H, (void *)FRAME_SIZE, BW);
    sceGuDepthBuffer((void *)(FRAME_SIZE * 2), BW);

    sceGuOffset(2048-W/2, 2048-H/2);
    sceGuViewport(2048,2048,W,H);
    sceGuScissor(0,0,W,H);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_CULL_FACE);

    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    ScePspFVector3 camera = {0.0f, 22.0f, 30.0f};
    ScePspFVector3 target = {0.0f, 0.0f, 0.0f};
    ScePspFVector3 up = {0.0f, 1.0f, 0.0f};

    while (1) {
        sceGuStart(GU_DIRECT, list);

        sceGuClearColor(GU_RGBA(18,30,48,255));
        sceGuClear(GU_COLOR_BUFFER_BIT);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(48.0f, (float)W/H, 0.5f, 100.0f);

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        sceGumLookAt(&camera, &target, &up);

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        drawPitch();

        sceGuFinish();
        sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    return 0;
}
