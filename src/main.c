
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>

PSP_MODULE_INFO("FC26 PSP Edition", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define SCREEN_W 480
#define SCREEN_H 272
#define BUF_W 512

#define FRAME_SIZE (BUF_W * SCREEN_H * 4)

static unsigned int __attribute__((aligned(16))) displayList[262144];

typedef struct {
    unsigned int color;
    float x, y, z;
} Vertex;

static void drawQuad(
    float x1, float z1,
    float x2, float z2,
    unsigned int color
) {
    Vertex *v = (Vertex *)sceGuGetMemory(4 * sizeof(Vertex));

    v[0] = (Vertex){color, x1, 0.0f, z1};
    v[1] = (Vertex){color, x2, 0.0f, z1};
    v[2] = (Vertex){color, x1, 0.0f, z2};
    v[3] = (Vertex){color, x2, 0.0f, z2};

    sceGuDrawArray(
        GU_TRIANGLE_STRIP,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        4, 0, v
    );
}

static void drawLine(
    float x1, float z1,
    float x2, float z2
) {
    Vertex *v = (Vertex *)sceGuGetMemory(2 * sizeof(Vertex));

    unsigned int white = GU_RGBA(255, 255, 255, 255);

    v[0] = (Vertex){white, x1, 0.035f, z1};
    v[1] = (Vertex){white, x2, 0.035f, z2};

    sceGuDrawArray(
        GU_LINES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        2, 0, v
    );
}

static void drawCircle(float radius)
{
    int i;
    float angle1, angle2;

    for (i = 0; i < 48; i++) {
        angle1 = (float)i * 6.2831853f / 48.0f;
        angle2 = (float)(i + 1) * 6.2831853f / 48.0f;

        drawLine(
            radius * __builtin_cosf(angle1),
            radius * __builtin_sinf(angle1),
            radius * __builtin_cosf(angle2),
            radius * __builtin_sinf(angle2)
        );
    }
}

static void drawPitch(void)
{
    int i;

    unsigned int grass1 = GU_RGBA(35, 125, 48, 255);
    unsigned int grass2 = GU_RGBA(43, 145, 55, 255);

    // Alternating grass sections
    for (i = 0; i < 8; i++) {
        float z1 = -16.0f + i * 4.0f;
        float z2 = z1 + 4.0f;

        drawQuad(
            -10.0f, z1,
            10.0f, z2,
            (i % 2) ? grass1 : grass2
        );
    }

    // Outer touchlines
    drawLine(-10, -16, 10, -16);
    drawLine(-10, 16, 10, 16);
    drawLine(-10, -16, -10, 16);
    drawLine(10, -16, 10, 16);

    // Halfway line
    drawLine(-10, 0, 10, 0);

    // Centre circle
    drawCircle(2.2f);

    // Centre spot
    drawLine(-0.12f, 0, 0.12f, 0);

    // Penalty areas
    drawLine(-7, -16, -7, -12);
    drawLine(7, -16, 7, -12);
    drawLine(-7, -12, 7, -12);

    drawLine(-7, 16, -7, 12);
    drawLine(7, 16, 7, 12);
    drawLine(-7, 12, 7, 12);

    // Goal areas
    drawLine(-3, -16, -3, -14);
    drawLine(3, -16, 3, -14);
    drawLine(-3, -14, 3, -14);

    drawLine(-3, 16, -3, 14);
    drawLine(3, 16, 3, 14);
    drawLine(-3, 14, 3, 14);
}

int main(void)
{
    void *fbp0 = (void *)0;
    void *fbp1 = (void *)FRAME_SIZE;
    void *zbp = (void *)(FRAME_SIZE * 2);

    sceGuInit();

    sceGuStart(GU_DIRECT, displayList);

    sceGuDrawBuffer(GU_PSM_8888, fbp0, BUF_W);
    sceGuDispBuffer(SCREEN_W, SCREEN_H, fbp1, BUF_W);
    sceGuDepthBuffer(zbp, BUF_W);

    sceGuOffset(2048 - SCREEN_W / 2, 2048 - SCREEN_H / 2);
    sceGuViewport(2048, 2048, SCREEN_W, SCREEN_H);

    sceGuScissor(0, 0, SCREEN_W, SCREEN_H);
    sceGuEnable(GU_SCISSOR_TEST);

    sceGuDepthRange(65535, 0);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_DEPTH_TEST);

    sceGuDisable(GU_CULL_FACE);
    sceGuShadeModel(GU_SMOOTH);

    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    // Camera setup
    ScePspFVector3 camera = {0.0f, 14.0f, 21.0f};
    ScePspFVector3 target = {0.0f, 0.0f, 0.0f};
    ScePspFVector3 up = {0.0f, 1.0f, 0.0f};

    while (1) {
        sceGuStart(GU_DIRECT, displayList);

        sceGuClearColor(GU_RGBA(22, 38, 58, 255));
        sceGuClearDepth(0);
        sceGuClear(
            GU_COLOR_BUFFER_BIT |
            GU_DEPTH_BUFFER_BIT
        );

        // Perspective camera
        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(
            48.0f,
            (float)SCREEN_W / SCREEN_H,
            0.5f,
            100.0f
        );

        // Camera view
        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        sceGumLookAt(&camera, &target, &up);

        // World
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        drawPitch();

        sceGuFinish();
        sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    sceGuTerm();
    sceKernelExitGame();

    return 0;
}

