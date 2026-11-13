
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <math.h>

PSP_MODULE_INFO("FC26 PSP Edition", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define W 480
#define H 272
#define BW 512
#define FRAME_SIZE (BW * H * 4)
#define WHITE 0xFFFFFFFF

static unsigned int __attribute__((aligned(16))) list[262144];
static unsigned int frameCount = 0;

typedef struct {
    unsigned int color;
    float x, y, z;
} Vertex;

static void quad(float x1,float y1,float z1,
                 float x2,float y2,float z2,
                 float x3,float y3,float z3,
                 float x4,float y4,float z4,
                 unsigned int c)
{
    Vertex v[4] = {
        {c,x1,y1,z1},{c,x2,y2,z2},
        {c,x3,y3,z3},{c,x4,y4,z4}
    };
    sceGumDrawArray(GU_TRIANGLE_STRIP,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        4,0,v);
}

static void line(float x1,float y1,float z1,
                 float x2,float y2,float z2,unsigned int c)
{
    Vertex v[2] = {{c,x1,y1,z1},{c,x2,y2,z2}};
    sceGumDrawArray(GU_LINES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        2,0,v);
}

static void triangle(float x1,float y1,float z1,
                     float x2,float y2,float z2,
                     float x3,float y3,float z3,unsigned int c)
{
    Vertex v[3] = {
        {c,x1,y1,z1},{c,x2,y2,z2},{c,x3,y3,z3}
    };
    sceGumDrawArray(GU_TRIANGLES,
        GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        3,0,v);
}

/* STADIUM STANDS */

static void drawStands(void)
{
    int s,i;

    for(s=-1;s<=1;s+=2) {
        for(i=0;i<8;i++) {
            float y=0.5f+i*0.65f;
            float z=17.5f+i*0.35f;
            unsigned int c=(i%2)?0xFF3030A0:0xFF4040C0;

            quad(s*12,y,-z,s*12,y,z,
                 s*12,y+0.55f,-z,s*12,y+0.55f,z,c);

            quad(-z,y,s*18.5f,-0.0f,y,s*18.5f,
                 -z,y+0.55f,s*18.5f,
                 z,y+0.55f,s*18.5f,c);
            quad(-z,y,-s*18.5f,z,y,-s*18.5f,
                 -z,y+0.55f,-s*18.5f,
                 z,y+0.55f,-s*18.5f,c);
        }
    }

    quad(-13,6.5f,-20,13,6.5f,-20,
         -13,6.5f,20,13,6.5f,20,0xFF555555);
}

/* ANIMATED CROWD */

static void drawCrowd(void)
{
    int s,r,n;
    unsigned int colors[5]={
        0xFFE03030,0xFF3030D0,0xFFFFFFFF,
        0xFF20A040,0xFF20A0E0
    };

    for(s=-1;s<=1;s+=2) {
        for(r=0;r<7;r++) {
            for(n=0;n<20;n++) {
                float z=-15.5f+n*1.6f;
                float y=0.8f+r*0.65f;
                float b=0.07f*sinf(frameCount*0.04f+n+r);
                unsigned int c=colors[(n+r)%5];

                quad(s*12.15f,y+b,z,
                     s*12.15f,y+b,z+0.45f,
                     s*12.15f,y+0.32f+b,z,
                     s*12.15f,y+0.32f+b,z+0.45f,c);

                quad(s*12.15f,y+0.32f+b,z+0.1f,
                     s*12.15f,y+0.32f+b,z+0.35f,
                     s*12.15f,y+0.5f+b,z+0.1f,
                     s*12.15f,y+0.5f+b,z+0.35f,
                     0xFFB08060);
            }
        }

        for(r=0;r<7;r++) {
            for(n=0;n<15;n++) {
                float x=-9.5f+n*1.35f;
                float y=0.8f+r*0.65f;
                float b=0.06f*sinf(frameCount*0.04f+n+r);
                float z=s*18.6f;
                unsigned int c=colors[(n+r*2)%5];

                quad(x,y+b,z,x+0.4f,y+b,z,
                     x,y+0.32f+b,z,x+0.4f,y+0.32f+b,z,c);
                quad(x+0.1f,y+0.32f+b,z,
                     x+0.3f,y+0.32f+b,z,
                     x+0.1f,y+0.5f+b,z,
                     x+0.3f,y+0.5f+b,z,0xFFB08060);
            }
        }
    }
}

/* FLOODLIGHT STRUCTURES */

static void drawLights(void)
{
    int s;

    for(s=-1;s<=1;s+=2) {
        float x=s*14.0f;

        line(x,0,-18,x,9,-18,0xFFAAAAAA);
        line(x,0,18,x,9,18,0xFFAAAAAA);

        quad(x-0.8f,9,-18.5f,x+0.8f,9,-18.5f,
             x-0.8f,9.7f,-18.5f,x+0.8f,9.7f,-18.5f,
             0xFFFFFFDD);

        quad(x-0.8f,9,18.5f,x+0.8f,9,18.5f,
             x-0.8f,9.7f,18.5f,x+0.8f,9.7f,18.5f,
             0xFFFFFFDD);
    }
}

/* PITCH */

static void drawPitch(void)
{
    int i;

    for(i=0;i<8;i++) {
        float z=-16+i*4;
        unsigned int c=(i%2)?0xFF287A20:0xFF328C25;

        quad(-10,0.02f,z,10,0.02f,z,
             -10,0.02f,z+4,10,0.02f,z+4,c);
    }

    line(-10,0.06f,-16,10,0.06f,-16,WHITE);
    line(-10,0.06f,16,10,0.06f,16,WHITE);
    line(-10,0.06f,-16,-10,0.06f,16,WHITE);
    line(10,0.06f,-16,10,0.06f,16,WHITE);
    line(-10,0.06f,0,10,0.06f,0,WHITE);

    for(i=0;i<64;i++) {
        float a=i*6.2831853f/64;
        float b=(i+1)*6.2831853f/64;
        line(2.5f*cosf(a),0.06f,2.5f*sinf(a),
             2.5f*cosf(b),0.06f,2.5f*sinf(b),WHITE);
    }

    line(-5.5f,0.06f,-16,5.5f,0.06f,-16,WHITE);
    line(-5.5f,0.06f,-16,-5.5f,0.06f,-10,WHITE);
    line(5.5f,0.06f,-16,5.5f,0.06f,-10,WHITE);
    line(-5.5f,0.06f,-10,5.5f,0.06f,-10,WHITE);

    line(-5.5f,0.06f,16,5.5f,0.06f,16,WHITE);
    line(-5.5f,0.06f,16,-5.5f,0.06f,10,WHITE);
    line(5.5f,0.06f,16,5.5f,0.06f,10,WHITE);
    line(-5.5f,0.06f,10,5.5f,0.06f,10,WHITE);
}

/* GOALS */

static void drawGoals(void)
{
    int s,n;

    for(s=-1;s<=1;s+=2) {
        float z=s*16.0f;

        line(-3.66f,0.05f,z,3.66f,0.05f,z,WHITE);
        line(-3.66f,0.05f,z,-3.66f,2.44f,z,WHITE);
        line(3.66f,0.05f,z,3.66f,2.44f,z,WHITE);
        line(-3.66f,2.44f,z,3.66f,2.44f,z,WHITE);

        for(n=0;n<=6;n++) {
            float x=-3.66f+n*1.22f;
            line(x,0.05f,z,x,2.44f,z+s*1.5f,WHITE);
        }

        for(n=0;n<=4;n++) {
            float y=n*0.61f;
            line(-3.66f,y,z,3.66f,y,z,WHITE);
        }
    }
}

/* CORNER FLAGS */

static void drawCorners(void)
{
    int sx,sz,i;

    for(sx=-1;sx<=1;sx+=2) {
        for(sz=-1;sz<=1;sz+=2) {
            float x=sx*9.8f;
            float z=sz*15.8f;

            line(x,0.05f,z,x,1.8f,z,WHITE);
            triangle(x,1.8f,z,x+sx*0.8f,1.55f,z,
                     x,1.25f,z,0xFF2020E8);

            for(i=0;i<12;i++) {
                float a=i*1.5707963f/12;
                float b=(i+1)*1.5707963f/12;

                line(sx*(10-0.8f*cosf(a)),0.07f,
                     sz*(16-0.8f*sinf(a)),
                     sx*(10-0.8f*cosf(b)),0.07f,
                     sz*(16-0.8f*sinf(b)),WHITE);
            }
        }
    }
}

/* ADVERTISING BOARDS */

static void drawBoards(void)
{
    int i;

    for(i=0;i<10;i++) {
        float z=-16.5f+i*3.3f;
        unsigned int c=(i%2)?0xFF00A0E0:0xFFE05020;

        quad(-11,0.1f,z,-11,0.1f,z+3,
             -11,1,z,-11,1,z+3,c);
        quad(11,0.1f,z,11,0.1f,z+3,
             11,1,z,11,1,z+3,c);
    }

    for(i=0;i<7;i++) {
        float x=-10+i*3;
        unsigned int c=(i%2)?0xFF00A0E0:0xFFE05020;

        quad(x,0.1f,-17.2f,x+2.7f,0.1f,-17.2f,
             x,1,-17.2f,x+2.7f,1,-17.2f,c);

        quad(x,0.1f,17.2f,x+2.7f,0.1f,17.2f,
             x,1,17.2f,x+2.7f,1,17.2f,c);
    }
}

/* TECHNICAL AREAS */

static void drawTechnical(void)
{
    int s;

    for(s=-1;s<=1;s+=2) {
        float x=s*12.5f;

        line(x,0.08f,-5,x,0.08f,5,WHITE);
        line(x,0.08f,-5,x+s,0.08f,-5,WHITE);
        line(x,0.08f,5,x+s,0.08f,5,WHITE);

        quad(x+s*0.2f,1.8f,-4,x+s*0.2f,1.8f,4,
             x+s*2,1.8f,-4,x+s*2,1.8f,4,0xFF505050);
    }
}

/* 3D SCOREBOARD */

static void drawScoreboard(void)
{
    int minutes=(frameCount/3600)%100;
    int seconds=(frameCount/60)%60;

    /* Large board facing the broadcast camera */
    quad(-5.5f,10.0f,-22,
          5.5f,10.0f,-22,
         -5.5f,13.0f,-22,
          5.5f,13.0f,-22,0xFF101010);

    /* Arsenal side */
    quad(-5.2f,10.3f,-21.9f,
         -2.0f,10.3f,-21.9f,
         -5.2f,12.7f,-21.9f,
         -2.0f,12.7f,-21.9f,0xFF2020C0);

    /* Real Madrid side */
    quad(2.0f,10.3f,-21.9f,
         5.2f,10.3f,-21.9f,
         2.0f,12.7f,-21.9f,
         5.2f,12.7f,-21.9f,0xFFFFFFFF);

    /* Score digits */
    line(-1.3f,10.6f,-21.8f,-1.3f,12.4f,-21.8f,WHITE);
    line(-1.3f,12.4f,-21.8f,0.0f,12.4f,-21.8f,WHITE);
    line(0.0f,12.4f,-21.8f,0.0f,10.6f,-21.8f,WHITE);

    line(1.3f,10.6f,-21.8f,1.3f,12.4f,-21.8f,WHITE);
    line(1.3f,12.4f,-21.8f,2.6f,12.4f,-21.8f,WHITE);
    line(2.6f,12.4f,-21.8f,2.6f,10.6f,-21.8f,WHITE);

    /* Timer marks */
    line(-1.0f,9.8f,-21.8f,-0.5f,9.8f,-21.8f,WHITE);
    line(0.5f,9.8f,-21.8f,1.0f,9.8f,-21.8f,WHITE);

    /* Small clock indicator bars */
    {
        float m=(float)minutes;
        float s=(float)seconds;
        line(-1.8f,9.3f,-21.8f,-1.8f+(m/100.0f)*3.6f,
             9.3f,-21.8f,0xFF00FF00);
        line(-1.8f,9.0f,-21.8f,-1.8f+(s/60.0f)*3.6f,
             9.0f,-21.8f,0xFFFFD000);
    }
}

/* MAIN GAME LOOP */

int main(void)
{
    ScePspFVector3 camera={0.0f,30.0f,43.0f};
    ScePspFVector3 target={0.0f,2.0f,0.0f};
    ScePspFVector3 up={0.0f,1.0f,0.0f};

    sceGuInit();

    sceGuStart(GU_DIRECT,list);
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,BW);
    sceGuDispBuffer(W,H,(void*)FRAME_SIZE,BW);
    sceGuDepthBuffer((void*)(2*FRAME_SIZE),BW);
    sceGuOffset(2048-W/2,2048-H/2);
    sceGuViewport(2048,2048,W,H);
    sceGuDepthRange(65535,0);
    sceGuScissor(0,0,W,H);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDisable(GU_CULL_FACE);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuClearColor(0xFF202020);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
    sceGuFinish();
    sceGuSync(0,0);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    for(;;) {
        sceGuStart(GU_DIRECT,list);

        sceGuClearColor(0xFF202020);
        sceGuClearDepth(0);
        sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

        sceGuEnable(GU_DEPTH_TEST);

        sceGumMatrixMode(GU_PROJECTION);
        sceGumLoadIdentity();
        sceGumPerspective(45.0f,(float)W/H,1.0f,200.0f);

        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();
        sceGumLookAt(&camera,&target,&up);

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        drawStands();
        drawCrowd();
        drawLights();
        drawPitch();
        drawGoals();
        drawCorners();
        drawBoards();
        drawTechnical();
        drawScoreboard();

        sceGuFinish();
        sceGuSync(0,0);

        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();

        frameCount++;
    }

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
