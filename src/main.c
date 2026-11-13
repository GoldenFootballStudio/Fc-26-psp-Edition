
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <math.h>

PSP_MODULE_INFO("FC26 PSP Football",0,1,0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define W 480
#define H 272
#define BW 512
#define FRAME_SIZE (BW * H * 4)
#define WHITE 0xFFFFFFFF
#define BLUE 0xFF2020D8
#define RED  0xFFD02020

static unsigned int __attribute__((aligned(16))) list[262144];
static unsigned int frameCount=0;

typedef struct {
    unsigned int color;
    float x,y,z;
} Vertex;

typedef struct {
    float x,z;
    float homeX,homeZ;
} Player;

static Player blue[11];
static Player red[11];

static float ballX=0.0f;
static float ballZ=1.5f;
static float ballVX=0.0f;
static float ballVZ=0.0f;

static int ballOwner=0;
static int ballMoving=0;
static int passTarget=-1;
static int controlled=0;
static int passCooldown=0;
static unsigned int previousButtons=0;

static float clampf(float v,float lo,float hi)
{
    if(v<lo) return lo;
    if(v>hi) return hi;
    return v;
}

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
                 float x2,float y2,float z2,
                 unsigned int c)
{
    Vertex v[2]={{c,x1,y1,z1},{c,x2,y2,z2}};

    sceGumDrawArray(GU_LINES,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        2,0,v);
}

static void triangle(float x1,float y1,float z1,
                     float x2,float y2,float z2,
                     float x3,float y3,float z3,
                     unsigned int c)
{
    Vertex v[3]={
        {c,x1,y1,z1},{c,x2,y2,z2},{c,x3,y3,z3}
    };

    sceGumDrawArray(GU_TRIANGLES,
        GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,
        3,0,v);
}

/* STADIUM */

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

static void drawStands(void)
{
    int s,r;

    for(s=-1;s<=1;s+=2) {
        for(r=0;r<8;r++) {
            float inner=11.3f+r*0.8f;
            float outer=inner+0.8f;
            float y=0.45f+r*0.55f;
            unsigned int c=(r%2)?0xFF3030B0:0xFF404080;

            quad(s*inner,y,-20,s*outer,y,-20,
                 s*inner,y,20,s*outer,y,20,c);
        }
    }

    for(s=-1;s<=1;s+=2) {
        for(r=0;r<7;r++) {
            float inner=17.4f+r*0.75f;
            float outer=inner+0.75f;
            float y=0.45f+r*0.55f;

            quad(-12,y,s*inner,12,y,s*inner,
                 -12,y,s*outer,12,y,s*outer,
                 (r%2)?0xFF3030B0:0xFF404080);
        }
    }

    for(s=-1;s<=1;s+=2) {
        quad(s*16,5.2f,-21,s*19,5.2f,-21,
             s*16,5.2f,21,s*19,5.2f,21,
             0xFF555555);
    }
}

static void drawCrowd(void)
{
    int s,r,n;

    unsigned int colors[6]={
        0xFFE03030,0xFF3030D0,WHITE,
        0xFF20A040,0xFF20A0E0,0xFF303030
    };

    for(s=-1;s<=1;s+=2) {
        for(r=0;r<7;r++) {
            for(n=0;n<22;n++) {
                float z=-18+n*1.65f;
                float x=s*(11.7f+r*0.8f);
                float y=0.65f+r*0.55f;
                float b=0.06f*sinf(frameCount*0.04f+n+r);

                quad(x,y+b,z,x,y+b,z+0.35f,
                     x,y+0.3f+b,z,x,y+0.3f+b,z+0.35f,
                     colors[(n+r)%6]);
            }
        }
    }
}

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
                 x+0.65f,9.8f,z+0.3f,
                 0xFFFFFFDD);
        }
    }
}

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

static void drawCorners(void)
{
    int sx,sz;

    for(sx=-1;sx<=1;sx+=2) {
        for(sz=-1;sz<=1;sz+=2) {
            float x=sx*9.8f;
            float z=sz*15.8f;

            line(x,0.05f,z,x,1.8f,z,WHITE);

            triangle(x,1.8f,z,
                     x+sx*0.8f,1.55f,z,
                     x,1.25f,z,0xFF2020E8);
        }
    }
}

static void drawBoards(void)
{
    int i;

    for(i=0;i<10;i++) {
        float z=-16+i*3.2f;
        unsigned int c=(i%2)?0xFF00A0E0:0xFFE05020;

        quad(-10.8f,0.1f,z,-10.8f,0.1f,z+2.8f,
             -10.8f,1.0f,z,-10.8f,1.0f,z+2.8f,c);

        quad(10.8f,0.1f,z,10.8f,0.1f,z+2.8f,
             10.8f,1.0f,z,10.8f,1.0f,z+2.8f,c);
    }
}

static void drawTechnical(void)
{
    int s;

    for(s=-1;s<=1;s+=2) {
        float x=s*12.0f;

        line(x,0.08f,-5,x,0.08f,5,WHITE);
        line(x,0.08f,-5,x+s*2,0.08f,-5,WHITE);
        line(x,0.08f,5,x+s*2,0.08f,5,WHITE);

        quad(x+s*0.2f,1.8f,-4,x+s*0.2f,1.8f,4,
             x+s*2,1.8f,-4,x+s*2,1.8f,4,
             0xFF505050);
    }
}

/* SCOREBOARD */

static void drawScoreboard(void)
{
    int minutes=(frameCount/3600)%100;
    int seconds=(frameCount/60)%60;

    quad(-5.5f,9,-22,5.5f,9,-22,
         -5.5f,12.5f,-22,5.5f,12.5f,-22,
         0xFF101010);

    quad(-5.1f,9.5f,-21.9f,-2.5f,9.5f,-21.9f,
         -5.1f,12,-21.9f,-2.5f,12,-21.9f,
         0xFF2020C0);

    quad(2.5f,9.5f,-21.9f,5.1f,9.5f,-21.9f,
         2.5f,12,-21.9f,5.1f,12,-21.9f,
         WHITE);

    line(-1,10.5f,-21.8f,1,10.5f,-21.8f,WHITE);
    line(-1,9.9f,-21.8f,1,9.9f,-21.8f,WHITE);

    line(-1.5f,8.1f,-21.8f,-0.6f,8.1f,-21.8f,WHITE);
    line(0.6f,8.1f,-21.8f,1.5f,8.1f,-21.8f,WHITE);

    (void)minutes;
    (void)seconds;
}

/* PLAYERS */

static void drawPlayer(float x,float z,
                       unsigned int kit,int selected)
{
    float bob=0.035f*sinf(frameCount*0.18f);

    line(x-0.22f,0.12f,z,x-0.25f,0.78f+bob,z,0xFF101010);
    line(x+0.22f,0.12f,z,x+0.25f,0.78f+bob,z,0xFF101010);

    line(x-0.25f,0.78f+bob,z,x-0.18f,1.12f+bob,z,0xFF202020);
    line(x+0.25f,0.78f+bob,z,x+0.18f,1.12f+bob,z,0xFF202020);

    quad(x-0.34f,1.05f+bob,z,
         x+0.34f,1.05f+bob,z,
         x-0.29f,1.95f+bob,z,
         x+0.29f,1.95f+bob,z,kit);

    line(x-0.30f,1.82f+bob,z,
         x-0.48f,1.18f+bob,z,0xFFB87945);

    line(x+0.30f,1.82f+bob,z,
         x+0.48f,1.18f+bob,z,0xFFB87945);

    quad(x-0.18f,1.95f+bob,z,
         x+0.18f,1.95f+bob,z,
         x-0.18f,2.28f+bob,z,
         x+0.18f,2.28f+bob,z,0xFFB87945);

    if(selected) {
        line(x-0.6f,2.5f+bob,z,
             x+0.6f,2.5f+bob,z,0xFFFFFF00);
    }
}

static void drawBall(float x,float z)
{
    float y=0.18f+0.035f*sinf(frameCount*0.25f);

    triangle(x,y+0.16f,z,x-0.16f,y,z,
             x,y,z-0.16f,WHITE);

    triangle(x,y+0.16f,z,x,y,z-0.16f,
             x+0.16f,y,z,WHITE);

    triangle(x,y+0.16f,z,x+0.16f,y,z,
             x,y,z+0.16f,0xFF202020);

    triangle(x,y+0.16f,z,x,y,z+0.16f,
             x-0.16f,y,z,WHITE);
}

/* INITIAL FORMATIONS */

static void setupTeams(void)
{
    int i;

    blue[0].x=0;
    blue[0].z=2;

    blue[0].homeX=0;
    blue[0].homeZ=2;

    blue[1].homeX=0;  blue[1].homeZ=14.3f;
    blue[2].homeX=-7; blue[2].homeZ=10;
    blue[3].homeX=-2.3f; blue[3].homeZ=11;
    blue[4].homeX=2.3f; blue[4].homeZ=11;
    blue[5].homeX=7; blue[5].homeZ=10;
    blue[6].homeX=-5; blue[6].homeZ=5;
    blue[7].homeX=5; blue[7].homeZ=5;
    blue[8].homeX=-6; blue[8].homeZ=-3;
    blue[9].homeX=0; blue[9].homeZ=-5;
    blue[10].homeX=6; blue[10].homeZ=-3;

    for(i=1;i<11;i++) {
        blue[i].x=blue[i].homeX;
        blue[i].z=blue[i].homeZ;
    }

    red[0].homeX=0; red[0].homeZ=-14.3f;
    red[1].homeX=-7; red[1].homeZ=-10;
    red[2].homeX=-2.3f; red[2].homeZ=-11;
    red[3].homeX=2.3f; red[3].homeZ=-11;
    red[4].homeX=7; red[4].homeZ=-10;
    red[5].homeX=-5; red[5].homeZ=-4;
    red[6].homeX=0; red[6].homeZ=-3;
    red[7].homeX=5; red[7].homeZ=-4;
    red[8].homeX=-6; red[8].homeZ=2;
    red[9].homeX=0; red[9].homeZ=4;
    red[10].homeX=6; red[10].homeZ=2;

    for(i=0;i<11;i++) {
        red[i].x=red[i].homeX;
        red[i].z=red[i].homeZ;
    }

    ballOwner=0;
    ballMoving=0;
    ballX=blue[0].x;
    ballZ=blue[0].z-0.45f;
}

/* TEAM AI */

static void updateAI(void)
{
    int i;
    float bx=ballX;
    float bz=ballZ;

    for(i=1;i<11;i++) {
        float targetX=blue[i].homeX+bx*0.10f;
        float targetZ=blue[i].homeZ+bz*0.08f;

        /* Teammates keep their formation instead of
           all chasing the player with the ball. */

        targetX=clampf(targetX,-8.5f,8.5f);
        targetZ=clampf(targetZ,-14.5f,14.5f);

        blue[i].x+=(targetX-blue[i].x)*0.012f;
        blue[i].z+=(targetZ-blue[i].z)*0.012f;
    }

    /* Opponents maintain their shape.
       The two closest outfield players press the ball. */

    for(i=1;i<11;i++) {
        float dx=bx-red[i].x;
        float dz=bz-red[i].z;
        float d=sqrtf(dx*dx+dz*dz);

        float tx=red[i].homeX;
        float tz=red[i].homeZ;

        if(d<7.0f && i>=5) {
            tx=red[i].homeX+dx*0.12f;
            tz=red[i].homeZ+dz*0.12f;
        }

        red[i].x+=(tx-red[i].x)*0.018f;
        red[i].z+=(tz-red[i].z)*0.018f;
    }

    /* Goalkeepers stay close to their goals. */

    red[0].x+=(clampf(bx,-2.5f,2.5f)-red[0].x)*0.02f;
}

/* FIND A TEAMMATE FOR A PASS */

static int findPassTarget(int passer,int through)
{
    int i,best=-1;
    float bestScore=99999.0f;

    for(i=0;i<11;i++) {
        float dx,dz,d,score;

        if(i==passer) continue;

        dx=blue[i].x-blue[passer].x;
        dz=blue[i].z-blue[passer].z;
        d=sqrtf(dx*dx+dz*dz);

        if(d<1.0f || d>17.0f) continue;

        score=d;

        /* Prefer teammates ahead of the passer. */
        if(dz>=0) score+=7.0f;

        /* Through balls favour players further forward. */
        if(through && dz<0) score-=3.0f;

        if(score<bestScore) {
            bestScore=score;
            best=i;
        }
    }

    return best;
}

static void kickToPlayer(int passer,int target,float power)
{
    float dx,dz,d;

    if(target<0) return;

    dx=blue[target].x-blue[passer].x;
    dz=blue[target].z-blue[passer].z;
    d=sqrtf(dx*dx+dz*dz);

    if(d<0.1f) return;

    ballX=blue[passer].x;
    ballZ=blue[passer].z-0.4f;

    ballVX=(dx/d)*power;
    ballVZ=(dz/d)*power;

    ballMoving=1;
    ballOwner=-1;
    passTarget=target;
}

/* BALL AND POSSESSION */

static void updateBall(void)
{
    int i;

    if(ballOwner>=0 && !ballMoving) {
        ballX=blue[ballOwner].x;
        ballZ=blue[ballOwner].z-0.42f;
    }

    if(ballMoving) {
        ballX+=ballVX;
        ballZ+=ballVZ;

        ballVX*=0.992f;
        ballVZ*=0.992f;

        if(passTarget>=0) {
            float dx=blue[passTarget].x-ballX;
            float dz=blue[passTarget].z-ballZ;

            if(dx*dx+dz*dz<1.0f) {
                ballOwner=passTarget;
                controlled=passTarget;
                passTarget=-1;
                ballMoving=0;
                ballVX=0;
                ballVZ=0;
            }
        }

        if(ballZ<-15.5f || ballZ>15.5f ||
           ballX<-9.5f || ballX>9.5f) {
            ballMoving=0;
            passTarget=-1;
            ballVX=0;
            ballVZ=0;
            ballOwner=-1;
        }
    }

    if(!ballMoving && ballOwner<0) {
        for(i=0;i<11;i++) {
            float dx=blue[i].x-ballX;
            float dz=blue[i].z-ballZ;

            if(dx*dx+dz*dz<0.55f) {
                ballOwner=i;
                controlled=i;
                break;
            }
        }
    }

    if(passCooldown>0) passCooldown--;
}

/* CONTROLS */

static void updateMatch(void)
{
    SceCtrlData pad;
    Player *p;
    float speed=0.12f;
    float ax,ay;
    unsigned int pressed;

    sceCtrlPeekBufferPositive(&pad,1);

    pressed=pad.Buttons & ~previousButtons;

    p=&blue[controlled];

    if(pad.Buttons & PSP_CTRL_RTRIGGER)
        speed=0.20f;

    if(pad.Buttons & PSP_CTRL_LEFT)  p->x-=speed;
    if(pad.Buttons & PSP_CTRL_RIGHT) p->x+=speed;
    if(pad.Buttons & PSP_CTRL_UP)    p->z-=speed;
    if(pad.Buttons & PSP_CTRL_DOWN)  p->z+=speed;

    ax=((int)pad.Lx-128)/127.0f;
    ay=((int)pad.Ly-128)/127.0f;

    if(fabsf(ax)>0.18f) p->x+=ax*speed;
    if(fabsf(ay)>0.18f) p->z+=ay*speed;

    p->x=clampf(p->x,-9.0f,9.0f);
    p->z=clampf(p->z,-14.5f,14.5f);

    if(passCooldown==0 && ballOwner==controlled &&
       !ballMoving) {

        if(pressed & PSP_CTRL_CROSS) {
            int target=findPassTarget(controlled,0);
            if(target>=0) {
                kickToPlayer(controlled,target,0.34f);
                passCooldown=20;
            }
        }

        if(pressed & PSP_CTRL_CIRCLE) {
            int target=findPassTarget(controlled,0);
            if(target>=0) {
                kickToPlayer(controlled,target,0.48f);
                passCooldown=25;
            }
        }

        if(pressed & PSP_CTRL_TRIANGLE) {
            int target=findPassTarget(controlled,1);
            if(target>=0) {
                kickToPlayer(controlled,target,0.42f);
                passCooldown=25;
            }
        }

        if(pressed & PSP_CTRL_SQUARE) {
            ballX=p->x;
            ballZ=p->z-0.4f;
            ballVX=0;
            ballVZ=-0.62f;
            ballOwner=-1;
            ballMoving=1;
            passTarget=-1;
            passCooldown=30;
        }
    }

    if(pressed & PSP_CTRL_LTRIGGER) {
        controlled=(controlled+1)%11;
    }

    updateBall();
    updateAI();

    previousButtons=pad.Buttons;
}

/* MAIN */

int main(void)
{
    ScePspFVector3 camera={0.0f,29.0f,44.0f};
    ScePspFVector3 target={0.0f,2.0f,0.0f};
    ScePspFVector3 up={0.0f,1.0f,0.0f};
    int i;

    setupTeams();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

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
        updateMatch();

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

        for(i=0;i<11;i++) {
            drawPlayer(blue[i].x,blue[i].z,
                       BLUE,i==controlled);
        }

        for(i=0;i<11;i++) {
            unsigned int kit=(i==0)?0xFF20A020:RED;

            drawPlayer(red[i].x,red[i].z,kit,0);
        }

        drawBall(ballX,ballZ);
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
