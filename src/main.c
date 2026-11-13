
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspctrl.h>
#include <math.h>

PSP_MODULE_INFO("FC26 PSP Football", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define W 480
#define H 272
#define BW 512
#define FRAME_SIZE (BW * H * 4)

#define WHITE 0xFFFFFFFF
#define BLUE  0xFF2020D8
#define RED   0xFFD02020

#define RED_ID(i) (11+(i))
#define IS_RED(i) ((i)>=11 && (i)<22)

typedef struct {
    unsigned int color;
    float x,y,z;
} Vertex;

typedef struct {
    float x,z;
    float homeX,homeZ;
} Player;

static unsigned int __attribute__((aligned(16))) list[262144];
static unsigned int frameCount=0;

static Player blue[11],red[11];

static float ballX=0,ballZ=0;
static float ballVX=0,ballVZ=0;

static int ballOwner=9;
static int ballMoving=0;
static int passTarget=-1;

static int controlled=9;
static int passCooldown=0;
static int tackleCooldown=0;

static int blueScore=0,redScore=0;
static int paused=0;
static int secondPress=0;
static int goalTimer=0;
static unsigned int previousButtons=0;

static float clampf(float v,float lo,float hi)
{
    if(v<lo) return lo;
    if(v>hi) return hi;
    return v;
}

static float distance2(float x1,float z1,float x2,float z2)
{
    float dx=x1-x2;
    float dz=z1-z2;
    return dx*dx+dz*dz;
}

static Player *getPlayer(int id)
{
    if(id>=11) return &red[id-11];
    return &blue[id];
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

            quad(s*inner,y,-20,s*outer,y,-20,
                 s*inner,y,20,s*outer,y,20,
                 (r%2)?0xFF3030B0:0xFF404080);
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

/* PLAYERS AND BALL */

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

/* SCOREBOARD */

static void drawScoreboard(void)
{
    int i;

    quad(-5.5f,9,-22,5.5f,9,-22,
         -5.5f,12.5f,-22,5.5f,12.5f,-22,
         0xFF101010);

    quad(-5.1f,9.5f,-21.9f,-2.5f,9.5f,-21.9f,
         -5.1f,12,-21.9f,-2.5f,12,-21.9f,BLUE);

    quad(2.5f,9.5f,-21.9f,5.1f,9.5f,-21.9f,
         2.5f,12,-21.9f,5.1f,12,-21.9f,RED);

    for(i=0;i<blueScore && i<5;i++) {
        float x=-1.8f+i*0.55f;
        line(x,10.0f,-21.7f,x,11.4f,-21.7f,WHITE);
    }

    for(i=0;i<redScore && i<5;i++) {
        float x=2.8f+i*0.4f;
        line(x,10.0f,-21.7f,x,11.4f,-21.7f,WHITE);
    }
}

/* FORMATIONS */

static void setupTeams(void)
{
    int i;

    float bx[11]={0,0,-7,-2.3f,2.3f,7,-5,5,-6,0,6};
    float bz[11]={-14.3f,-10,-11,-11,-10,-4,-3,-4,2,4,2};

    float rx[11]={0,-7,-2.3f,2.3f,7,-5,0,5,-6,0,6};
    float rz[11]={14.3f,10,11,11,10,4,3,4,-2,-4,-2};

    for(i=0;i<11;i++) {
        blue[i].homeX=bx[i];
        blue[i].homeZ=bz[i];
        blue[i].x=bx[i];
        blue[i].z=bz[i];

        red[i].homeX=rx[i];
        red[i].homeZ=rz[i];
        red[i].x=rx[i];
        red[i].z=rz[i];
    }

    controlled=9;
    ballOwner=9;
    ballMoving=0;
    ballX=blue[9].x;
    ballZ=blue[9].z;
}

/* KICK-OFF */

static void kickoff(int blueStarts)
{
    int i;

    for(i=0;i<11;i++) {
        blue[i].x=blue[i].homeX;
        blue[i].z=blue[i].homeZ;

        red[i].x=red[i].homeX;
        red[i].z=red[i].homeZ;
    }

    ballMoving=0;
    ballVX=0;
    ballVZ=0;
    passTarget=-1;
    passCooldown=30;
    goalTimer=90;

    if(blueStarts) {
        blue[9].x=0;
        blue[9].z=0;
        ballOwner=9;
        ballX=0;
        ballZ=0;
    } else {
        red[9].x=0;
        red[9].z=0;
        ballOwner=RED_ID(9);
        ballX=0;
        ballZ=0;
    }
}

/* PASSING */

static int findPassTarget(int passer,int teamRed,int through)
{
    int i,best=-1;
    float bestScore=99999.0f;
    Player *from=getPlayer(passer);

    for(i=0;i<11;i++) {
        int id=teamRed?RED_ID(i):i;
        Player *to=getPlayer(id);

        float dx=to->x-from->x;
        float dz=to->z-from->z;
        float d=sqrtf(dx*dx+dz*dz);
        float forward=teamRed?dz:-dz;
        float score;

        if(id==passer || d<1.0f || d>18.0f)
            continue;

        score=d;
        if(forward<0) score+=7.0f;
        if(through && forward>0) score-=3.0f;

        if(score<bestScore) {
            bestScore=score;
            best=id;
        }
    }

    return best;
}

static void kickToPlayer(int passer,int target,float power)
{
    Player *from;
    Player *to;
    float dx,dz,d;

    if(target<0 || target>=22) return;

    from=getPlayer(passer);
    to=getPlayer(target);

    dx=to->x-from->x;
    dz=to->z-from->z;
    d=sqrtf(dx*dx+dz*dz);

    if(d<0.1f) return;

    ballX=from->x;
    ballZ=from->z+(IS_RED(passer)?0.4f:-0.4f);

    ballVX=(dx/d)*power;
    ballVZ=(dz/d)*power;

    ballMoving=1;
    ballOwner=-1;
    passTarget=target;
}

/* SHOOTING: BLUE ATTACKS TOWARD NEGATIVE Z */

static void shootBall(int power,int chip,int shooter)
{
    Player *p=getPlayer(shooter);
    float aimX=0.0f;
    float direction=IS_RED(shooter)?1.0f:-1.0f;

    if(p->x>0.8f) aimX=-0.12f;
    if(p->x<-0.8f) aimX=0.12f;

    ballX=p->x;
    ballZ=p->z+direction*0.45f;
    ballVX=aimX*(chip?0.7f:1.0f);
    ballVZ=direction*(power?0.78f:0.59f);

    ballOwner=-1;
    ballMoving=1;
    passTarget=-1;
}

/* TACKLING */

static void attemptTackle(int sliding)
{
    int enemy=ballOwner;
    Player *p=&blue[controlled];
    Player *e;
    float d;

    if(enemy<0 || !IS_RED(enemy) || tackleCooldown>0)
        return;

    e=getPlayer(enemy);
    d=sqrtf(distance2(p->x,p->z,e->x,e->z));

    if(d<(sliding?2.0f:1.45f)) {
        ballOwner=controlled;
        ballMoving=0;
        ballVX=0;
        ballVZ=0;
        passTarget=-1;
        tackleCooldown=sliding?65:35;
    } else {
        tackleCooldown=sliding?45:20;
    }
}

/* PLAYER SWITCHING */

static void switchNearestPlayer(void)
{
    int i,best=0;
    float bestD=99999.0f;

    for(i=0;i<11;i++) {
        float d=distance2(blue[i].x,blue[i].z,ballX,ballZ);

        if(d<bestD) {
            bestD=d;
            best=i;
        }
    }

    controlled=best;
}

static void switchDirectional(float dx,float dz)
{
    int i,best=-1;
    float bestScore=-99999.0f;

    for(i=0;i<11;i++) {
        float x=blue[i].x-blue[controlled].x;
        float z=blue[i].z-blue[controlled].z;
        float d=sqrtf(x*x+z*z);
        float dot;

        if(d<0.5f) continue;

        dot=(x*dx+z*dz)/d;

        if(dot>0.25f && dot*2.0f-d*0.04f>bestScore) {
            bestScore=dot*2.0f-d*0.04f;
            best=i;
        }
    }

    if(best>=0) controlled=best;
}

/* AI */

static void updateAI(void)
{
    int i;
    int nearestBlue=0;
    int nearestRed=0;
    float db=99999.0f;
    float dr=99999.0f;

    for(i=0;i<11;i++) {
        float d1=distance2(blue[i].x,blue[i].z,ballX,ballZ);
        float d2=distance2(red[i].x,red[i].z,ballX,ballZ);

        if(d1<db) { db=d1; nearestBlue=i; }
        if(d2<dr) { dr=d2; nearestRed=i; }
    }

    /* Blue teammates support the controlled player. */
    for(i=0;i<11;i++) {
        float tx,tz;

        if(i==controlled) continue;

        tx=blue[i].homeX+ballX*0.10f;
        tz=blue[i].homeZ+ballZ*0.08f;

        if(ballOwner>=11 && ballOwner<22) {
            if(i==nearestBlue || i==(nearestBlue+1)%11) {
                tx=ballX;
                tz=ballZ;
            }
        }

        if(ballOwner>=0 && !IS_RED(ballOwner)) {
            if(ballOwner==i) {
                tz-=0.045f;
            } else {
                tx+=(ballX-tx)*0.10f;
                tz+=(ballZ-tz)*0.08f;
            }
        }

        blue[i].x+=(clampf(tx,-8.7f,8.7f)-blue[i].x)*0.018f;
        blue[i].z+=(clampf(tz,-14.5f,14.5f)-blue[i].z)*0.018f;
    }

    /* Red opponents press, mark and support attacks. */
    for(i=0;i<11;i++) {
        float tx,tz;

        tx=red[i].homeX+ballX*0.10f;
        tz=red[i].homeZ+ballZ*0.08f;

        if(ballOwner>=0 && !IS_RED(ballOwner)) {
            if(i==nearestRed || (secondPress && i==(nearestRed+1)%11)) {
                tx=ballX;
                tz=ballZ;
            }
        }

        if(ballOwner>=11 && ballOwner<22) {
            if(i==ballOwner-11) {
                tz+=0.045f;
            } else if(i>=5) {
                tz-=0.15f;
            }
        }

        red[i].x+=(clampf(tx,-8.7f,8.7f)-red[i].x)*0.016f;
        red[i].z+=(clampf(tz,-14.5f,14.5f)-red[i].z)*0.016f;
    }

    /* Goalkeepers stay on their own goal lines. */
    blue[0].x+=(clampf(ballX,-2.5f,2.5f)-blue[0].x)*0.025f;
    blue[0].z+=(-14.3f-blue[0].z)*0.025f;

    red[0].x+=(clampf(ballX,-2.5f,2.5f)-red[0].x)*0.025f;
    red[0].z+=(14.3f-red[0].z)*0.025f;
}

/* BALL PHYSICS */

static void updateBall(void)
{
    int i;
    float bestD=99999.0f;
    int best=-1;

    if(ballOwner>=0 && !ballMoving) {
        Player *p=getPlayer(ballOwner);

        ballX=p->x;
        ballZ=p->z+(IS_RED(ballOwner)?0.42f:-0.42f);
    }

    if(ballMoving) {
        ballX+=ballVX;
        ballZ+=ballVZ;

        ballVX*=0.992f;
        ballVZ*=0.992f;

        if(passTarget>=0) {
            Player *target=getPlayer(passTarget);

            if(distance2(target->x,target->z,ballX,ballZ)<0.55f) {
                ballOwner=passTarget;
                ballMoving=0;
                passTarget=-1;
                ballVX=0;
                ballVZ=0;
            }
        }

        /* Blue scores at the negative-Z goal. */
        if(ballZ<-16.2f && fabsf(ballX)<3.66f) {
            blueScore++;
            kickoff(0);
            return;
        }

        /* Red scores at the positive-Z goal. */
        if(ballZ>16.2f && fabsf(ballX)<3.66f) {
            redScore++;
            kickoff(1);
            return;
        }

        if(ballX<-9.7f || ballX>9.7f ||
           ballZ<-17.5f || ballZ>17.5f) {
            ballX=clampf(ballX,-9.3f,9.3f);
            ballZ=clampf(ballZ,-15.0f,15.0f);
            ballVX=0;
            ballVZ=0;
            ballMoving=0;
            ballOwner=-1;
            passTarget=-1;
        }

        if(fabsf(ballVX)+fabsf(ballVZ)<0.025f) {
            ballMoving=0;
            ballOwner=-1;
            passTarget=-1;
        }
    }

    /* Loose-ball collection. */
    if(!ballMoving && ballOwner<0) {
        for(i=0;i<22;i++) {
            Player *p=getPlayer(i);
            float d=distance2(p->x,p->z,ballX,ballZ);

            if(d<bestD) {
                bestD=d;
                best=i;
            }
        }

        if(best>=0 && bestD<0.75f) {
            ballOwner=best;
            ballMoving=0;
        }
    }

    if(passCooldown>0) passCooldown--;
    if(tackleCooldown>0) tackleCooldown--;
    if(goalTimer>0) goalTimer--;
}

/* BLUE GOALKEEPER RUSH */

static void goalkeeperRush(void)
{
    float dx=ballX-blue[0].x;
    float dz=ballZ-blue[0].z;
    float d=sqrtf(dx*dx+dz*dz);

    if(d>0.1f && ballZ<0.0f) {
        blue[0].x+=dx/d*0.32f;
        blue[0].z+=dz/d*0.32f;
    }

    if(distance2(blue[0].x,blue[0].z,ballX,ballZ)<1.0f) {
        ballOwner=0;
        ballMoving=0;
        ballVX=0;
        ballVZ=0;
        passTarget=-1;
    }
}

/* CONTROL SYSTEM */

static void updateMatch(void)
{
    SceCtrlData pad;
    Player *p;
    float speed=0.12f;
    float ax,ay;
    float dx=0,dz=0;

    unsigned int pressed;
    unsigned int held;

    sceCtrlPeekBufferPositive(&pad,1);

    held=pad.Buttons;
    pressed=held & ~previousButtons;

    if(pressed & PSP_CTRL_START)
        paused=!paused;

    if(paused) {
        previousButtons=held;
        return;
    }

    if(held & PSP_CTRL_RTRIGGER)
        speed=0.20f;

    p=&blue[controlled];

    /* D-pad and analog movement. */
    if(held & PSP_CTRL_LEFT)  dx-=1;
    if(held & PSP_CTRL_RIGHT) dx+=1;
    if(held & PSP_CTRL_UP)    dz-=1;
    if(held & PSP_CTRL_DOWN)  dz+=1;

    ax=((int)pad.Lx-128)/127.0f;
    ay=((int)pad.Ly-128)/127.0f;

    if(fabsf(ax)>0.18f) dx+=ax;
    if(fabsf(ay)>0.18f) dz+=ay;

    if((held & PSP_CTRL_RTRIGGER) &&
       (held & PSP_CTRL_LTRIGGER) &&
       ballOwner==controlled &&
       (fabsf(dx)+fabsf(dz)>0.1f)) {
        p->x+=dx*speed*1.5f;
        p->z+=dz*speed*1.5f;
    } else {
        p->x+=dx*speed;
        p->z+=dz*speed;
    }

    p->x=clampf(p->x,-9.0f,9.0f);
    p->z=clampf(p->z,-14.5f,14.5f);

    /* L switches player. */
    if((pressed & PSP_CTRL_LTRIGGER) &&
       !(held & PSP_CTRL_CROSS)) {
        if(fabsf(dx)+fabsf(dz)>0.2f)
            switchDirectional(dx,dz);
        else
            switchNearestPlayer();
    }

    if((held & PSP_CTRL_LTRIGGER) &&
       (pressed & PSP_CTRL_CROSS) &&
       ballOwner>=11) {
        switchNearestPlayer();
    }

    secondPress=((held & PSP_CTRL_LTRIGGER) &&
                 (held & PSP_CTRL_RTRIGGER))?1:0;

    /* ATTACKING: BLUE HAS THE BALL. */
    if(ballOwner==controlled &&
       !ballMoving && passCooldown==0) {

        /* X + Triangle: one-two pass. */
        if((pressed & PSP_CTRL_CROSS) &&
           (held & PSP_CTRL_TRIANGLE)) {
            int target=findPassTarget(controlled,0,1);

            if(target>=0) {
                kickToPlayer(controlled,target,0.38f);
                passCooldown=25;
            }
        }

        /* X + R: pass and forward run. */
        else if((pressed & PSP_CTRL_CROSS) &&
                (held & PSP_CTRL_RTRIGGER)) {
            int target=findPassTarget(controlled,0,0);

            if(target>=0) {
                kickToPlayer(controlled,target,0.36f);
                passCooldown=25;
            }
        }

        /*
