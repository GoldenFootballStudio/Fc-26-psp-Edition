
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <math.h>

PSP_MODULE_INFO("FC26 PSP Edition",0,1,0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define W 480
#define H 272
#define BW 512
#define FRAME_SIZE (BW*H*4)
#define WHITE 0xFFFFFFFF

static unsigned int __attribute__((aligned(16))) list[262144];
static unsigned int frameCount=0;

typedef struct {
    unsigned int color;
    float x,y,z;
} Vertex;

static void quad(float x1,float y1,float z1,
                 float x2,float y2,float z2,
                 float x3,float y3,float z3,
                 float x4,float y4,float z4,
                 unsigned int c)
{
    Vertex v[4]={
        {c,x1,y1,z1},{c,x2,y2,z2},
        {c,x3,y3,z3},{c,x4,y4,z4}
    };
    sceGumDrawArray(GU_TRIANGLE_STRIP,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        4,0,v);
}

static void line(float x1,float y1,float z1,
                 float x2,float y2,float z2,unsigned int c)
{
    Vertex v[2]={{c,x1,y1,z1},{c,x2,y2,z2}};
    sceGumDrawArray(GU_LINES,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        2,0,v);
}

static void triangle(float x1,float y1,float z1,
                     float x2,float y2,float z2,
                     float x3,float y3,float z3,unsigned int c)
{
    Vertex v[3]={
        {c,x1,y1,z1},{c,x2,y2,z2},{c,x3,y3,z3}
    };
    sceGumDrawArray(GU_TRIANGLES,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        3,0,v);
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

    /* Penalty areas */
    line(-5.5f,0.06f,-16,5.5f,0.06f,-16,WHITE);
    line(-5.5f,0.06f,-16,-5.5f,0.06f,-10,WHITE);
    line(5.5f,0.06f,-16,5.5f,0.06f,-10,WHITE);
    line(-5.5f,0.06f,-10,5.5f,0.06f,-10,WHITE);

    line(-5.5f,0.06f,16,5.5f,0.06f,16,WHITE);
    line(-5.5f,0.06f,16,-5.5f,0.06f,10,WHITE);
    line(5.5f,0.06f,16,5.5f,0.06f,10,WHITE);
    line(-5.5f,0.06f,10,5.5f,0.06f,10,WHITE);

    /* Goal areas */
    line(-3,0.07f,-16,3,0.07f,-16,WHITE);
    line(-3,0.07f,-16,-3,0.07f,-13,WHITE);
    line(3,0.07f,-16,3,0.07f,-13,WHITE);
    line(-3,0.07f,-13,3,0.07f,-13,WHITE);

    line(-3,0.07f,16,3,0.07f,16,WHITE);
    line(-3,0.07f,16,-3,0.07f,13,WHITE);
    line(3,0.07f,16,3,0.07f,13,WHITE);
    line(-3,0.07f,13,3,0.07f,13,WHITE);
}

/* FOUR-SIDED STADIUM SEATING */

static void drawStands(void)
{
    int s,r;
    unsigned int red=0xFF3030B0;
    unsigned int blue=0xFF404080;

    /* Long-side stands */
    for(s=-1;s<=1;s+=2) {
        for(r=0;r<8;r++) {
            float inner=11.3f+r*0.8f;
            float outer=inner+0.8f;
            float y=0.45f+r*0.55f;
            unsigned int c=(r%2)?red:blue;

            quad(s*inner,y,-20,
                 s*outer,y,-20,
                 s*inner,y,20,
                 s*outer,y,20,c);
        }
    }

    /* Stands behind both goals */
    for(s=-1;s<=1;s+=2) {
        for(r=0;r<7;r++) {
            float inner=17.4f+r*0.75f;
            float outer=inner+0.75f;
            float y=0.45f+r*0.55f;
            unsigned int c=(r%2)?red:blue;

            quad(-12,y,s*inner,
                 12,y,s*inner,
                 -12,y,s*outer,
                 12,y,s*outer,c);
        }
    }

    /* Narrow roof sections above the side stands */
    for(s=-1;s<=1;s+=2) {
        quad(s*16.0f,5.2f,-21,
             s*19.0f,5.2f,-21,
             s*16.0f,5.2f,21,
             s*19.0f,5.2f,21,0xFF555555);
    }
}

/* SPECTATORS */

static void drawCrowd(void)
{
    int s,r,n;
    unsigned int colors[6]={
        0xFFE03030,0xFF3030D0,0xFFFFFFFF,
        0xFF20A040,0xFF20A0E0,0xFF303030
    };

    for(s=-1;s<=1;s+=2) {
        for(r=0;r<7;r++) {
            for(n=0;n<22;n++) {
                float z=-18+n*1.65f;
                float x=s*(11.7f+r*0.8f);
                float y=0.65f+r*0.55f;
                float b=0.06f*sinf(frameCount*0.04f+n+r);
                unsigned int c=colors[(n+r)%6];

                quad(x,y+b,z,x,y+b,z+0.35f,
                     x,y+0.3f+b,z,x,y+0.3f+b,z+0.35f,c);
            }
        }

        for(r=0;r<6;r++) {
            for(n=0;n<14;n++) {
                float x=-10+n*1.45f;
                float z=s*(17.8f+r*0.75f);
                float y=0.65f+r*0.55f;
                float b=0.06f*sinf(frameCount*0.04f+n+r);
                unsigned int c=colors[(n+r*2)%6];

                quad(x,y+b,z,x+0.35f,y+b,z,
                     x,y+0.3f+b,z,x+0.35f,y+0.3f+b,z,c);
            }
        }
    }
}

/* FLOODLIGHTS */

static void drawLights(void)
{
    int sx,sz;

    for(sx=-1;sx<=1;sx+=2) {
        for(sz=-1;sz<=1;sz+=2) {
            float x=sx*14.5f;
            float z=sz*19.5f;

            line(x,0,z,x,9,z,0xFFAAAAAA);
            line(x,9,z,x+sx*0.5f,9.5f,z,0xFFAAAAAA);

            quad(x-0.65f,9.2f,z-0.3f,
                 x+0.65f,9.2f,z-0.3f,
                 x-0.65f,9.8f,z+0.3f,
                 x+0.65f,9.8f,z+0.3f,0xFFFFFFDD);
        }
    }
}

/* GOALS AND NETS */

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

/* CORNER FLAGS AND ARCS */

static void drawCorners(void)
{
    int sx,sz,i;

    for(sx=-1;sx<=1;sx+=2) {
        for(sz=-1;sz<=1;sz+=2) {
            float x=sx*9.8f;
            float z=sz*15.8f;

            line(x,0.05f,z,x,1.8f,z,WHITE);
            triangle(x,1.8f,z,
                     x+sx*0.8f,1.55f,z,
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
        float z=-16+i*3.2f;
        unsigned int c=(i%2)?0xFF00A0E0:0xFFE05020;

        quad(-10.8f,0.1f,z,
             -10.8f,0.1f,z+2.8f,
             -10.8f,1.0f,z,
             -10.8f,1.0f,z+2.8f,c);

        quad(10.8f,0.1f,z,
             10.8f,0.1f,z+2.8f,
             10.8f,1.0f,z,
             10.8f,1.0f,z+2.8f,c);
    }
}

/* TECHNICAL AREAS */

static void drawTechnical(void)
{
    int s;

    for(s=-1;s<=1;s+=2) {
        float x=s*12.0f;

        line(x,0.08f,-5,x,0.08f,5,WHITE);
        line(x,0.08f,-5,x+s*2.0f,0.08f,-5,WHITE);
        line(x,0.08f,5,x+s*2.0f,0.08f,5,WHITE);

        quad(x+s*0.2f,1.8f,-4,
             x+s*0.2f,1.8f,4,
             x+s*2.0f,1.8f,-4,
             x+s*2.0f,1.8f,4,0xFF505050);
    }
}

/* SCOREBOARD DIGITS */

static void digit(float x,float y,float z,int number)
{
    static const unsigned char segments[10]={
        0x3F,0x06,0x5B,0x4F,0x66,
        0x6D,0x7D,0x07,0x7F,0x6F
    };

    unsigned char b=segments[number%10];
    float w=0.65f;
    float h=1.0f;
    unsigned int c=WHITE;

    if(b&1) line(x,y+h,z,x+w,y+h,z,c);
    if(b&2) line(x+w,y+h,z,x+w,y+h/2,z,c);
    if(b&4) line(x+w,y+h/2,z,x+w,y,z,c);
    if(b&8) line(x,y,z,x+w,y,z,c);
    if(b&16) line(x,y+h/2,z,x,y,z,c);
    if(b&32) line(x,y+h,z,x,y+h/2,z,c);
    if(b&64) line(x,y+h/2,z,x+w,y+h/2,z,c);
}

static void drawScoreboard(void)
{
    int minutes=(frameCount/3600)%100;
    int seconds=(frameCount/60)%60;

    /* Board body */
    quad(-5.5f,9.0f,-22,
          5.5f,9.0f,-22,
         -5.5f,12.5f,-22,
          5.5f,12.5f,-22,0xFF101010);

    /* Team colour panels */
    quad(-5.1f,9.5f,-21.9f,
         -2.5f,9.5f,-21.9f,
         -5.1f,12.0f,-21.9f,
         -2.5f,12.0f,-21.9f,0xFF2020C0);

    quad(2.5f,9.5f,-21.9f,
         5.1f,9.5f,-21.9f,
         2.5f,12.0f,-21.9f,
         5.1f,12.0f,-21.9f,0xFFFFFFFF);

    /* 0 - 0 score */
    digit(-1.3f,10.0f,-21.8f,0);
    digit(0.2f,10.0f,-21.8f,0);

    /* Clock */
    digit(-1.5f,8.1f,-21.8f,minutes/10);
    digit(-0.6f,8.1f,-21.8f,minutes%10);
    digit(0.6f,8.1f,-21.8f,seconds/10);
    digit(1.5f,8.1f,-21.8f,seconds%10);
}

/* MAIN */

int main(void)
{
    ScePspFVector3 camera={0.0f,29.0f,44.0f};
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
        sceGumPerspective(48.0f,(float)W/H,1.0f,200.0f);

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
