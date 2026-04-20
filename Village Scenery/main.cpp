/*
 * ============================================================
 *  Village Scenery with Animated Objects
 *  Computer Graphics Project
 * ============================================================
 *
 *  CONTROLS
 *  --------
 *  Left Click        = Zoom in toward clicked place
 *  Arrow Keys        = Explore scene
 *  R                 = Reset camera
 *  P                 = Pause
 *  1                 = Rain
 *  F                 = Fullscreen
 *  Right Click       = Menu
 *  ESC / Q           = Quit
 * ============================================================
 */

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>

static const int   WIN_W = 1100, WIN_H = 700;
static const float PI    = 3.14159265f;

/* ===============================================================
   ANIMATION STATE
   =============================================================== */
int   timer       = 0;
bool  paused      = false;

/* Boat */
float boatX       = 40.0f;

/* Car */
float carT           = 0.02f;
float carWheelAngle  = 0.0f;

/* Birds */
float birdX       = -350.0f;
float birdY       = 520.0f;
float wingAngle   = 0.0f;

/* Windmill */
float wmAngle     = 0.0f;

/* Smoke */
float smokeShear  = 0.0f;

/* Villager */
float villT          = 0.68f;
float villWalkPhase  = 0.0f;

/* Clouds */
struct Cloud { float x,y,sc,spd; };
Cloud clouds[5] = {
    {80,600,1.0f,0.32f},  {320,625,0.85f,0.22f},
    {550,608,1.1f,0.38f}, {780,618,0.9f,0.28f},
    {980,595,0.95f,0.35f},
};

/* Sun */
float sunScale=1.0f, sunDir=0.003f;

/* Pond ripple */
float rippleR = 5.0f;

/* Mist */
float mistShear = 0.0f;

/* Clock */
float clockSec = 0.0f;

/* Rain */
struct Drop { float x,y,spd,len; };
Drop rain[180];
bool rainOn = false;
float rainbowA = 0.0f;

/* Camera */
float camX=0, camY=0, camZoom=1.0f;
float targetCamX=0, targetCamY=0, targetZoom=1.0f;
float CSPD=28.0f;
bool  fullscr=false;
int   svW=WIN_W,svH=WIN_H,svPX=80,svPY=60;
int   curWinW=WIN_W, curWinH=WIN_H;

/* ===============================================================
   UTILS
   =============================================================== */
float clampf(float v,float lo,float hi){
    if(v<lo) return lo;
    if(v>hi) return hi;
    return v;
}

float lerpf(float a,float b,float t){
    return a + (b-a)*t;
}

void clampCameraTargets(){
    targetZoom = clampf(targetZoom,1.0f,3.2f);
    targetCamX = clampf(targetCamX,-260.0f,260.0f);
    targetCamY = clampf(targetCamY,-150.0f,150.0f);
}

/* Road goes from bottom to top (vertical) */
void roadCenter(float t,float &x,float &y){
    float x0 = 690.0f, y0 = 35.0f;
    float x1 = 680.0f, y1 = 255.0f;
    x = lerpf(x0,x1,t);
    y = lerpf(y0,y1,t);
}

float roadAngleDeg(){
    float x0 = 690.0f, y0 = 35.0f;
    float x1 = 680.0f, y1 = 255.0f;
    return atan2f(y1-y0, x1-x0) * 180.0f / PI - 90.0f;
}

void screenToWorld(int sx,int sy,float &wx,float &wy){
    float syb = (float)(curWinH - sy);
    float cx = WIN_W * 0.5f;
    float cy = WIN_H * 0.5f;
    wx = cx + (((float)sx) - cx) / camZoom - camX;
    wy = cy + (syb - cy) / camZoom - camY;
}

void zoomTowardWorld(float wx,float wy,float zoomStep){
    targetZoom = clampf(targetZoom + zoomStep, 1.0f, 3.2f);
    targetCamX = WIN_W*0.5f - wx;
    targetCamY = WIN_H*0.5f - wy;
    clampCameraTargets();
}

/* ===============================================================
   ALGORITHMS
   =============================================================== */
void ddaLine(float x1,float y1,float x2,float y2){
    float dx=x2-x1,dy=y2-y1;
    float s=fabsf(dx)>fabsf(dy)?fabsf(dx):fabsf(dy);
    if(s==0) return;
    float xi=dx/s,yi=dy/s,x=x1,y=y1;
    glBegin(GL_POINTS);
    for(int i=0;i<=(int)s;i++){glVertex2f(x,y);x+=xi;y+=yi;}
    glEnd();
}

void bresenhamLine(int x1,int y1,int x2,int y2){
    int dx=abs(x2-x1),dy=abs(y2-y1);
    int sx=(x1<x2)?1:-1,sy=(y1<y2)?1:-1,e=dx-dy;
    glBegin(GL_POINTS);
    while(true){
        glVertex2i(x1,y1);
        if(x1==x2&&y1==y2) break;
        int e2=2*e;
        if(e2>-dy){e-=dy;x1+=sx;}
        if(e2< dx){e+=dx;y1+=sy;}
    }
    glEnd();
}

void midpointCircle(int cx,int cy,int r){
    if(r<=0) return;
    int x=0,y=r,d=1-r;
    glBegin(GL_POINTS);
    while(x<=y){
        glVertex2i(cx+x,cy+y);glVertex2i(cx-x,cy+y);
        glVertex2i(cx+x,cy-y);glVertex2i(cx-x,cy-y);
        glVertex2i(cx+y,cy+x);glVertex2i(cx-y,cy+x);
        glVertex2i(cx+y,cy-x);glVertex2i(cx-y,cy-x);
        if(d<0) d+=2*x+3; else{d+=2*(x-y)+5;y--;}
        x++;
    }
    glEnd();
}

/* ===============================================================
   HELPERS
   =============================================================== */
void fc(float cx,float cy,float r,int sg=48){
    if(r<=0) return;
    glBegin(GL_TRIANGLE_FAN); glVertex2f(cx,cy);
    for(int i=0;i<=sg;i++){
        float a=2*PI*i/sg;
        glVertex2f(cx+r*cosf(a),cy+r*sinf(a));
    }
    glEnd();
}

void drawText(float x,float y,const char* s,void* f=GLUT_BITMAP_HELVETICA_10){
    glRasterPos2f(x,y);
    for(const char* c=s;*c;c++) glutBitmapCharacter(f,*c);
}

void initParticles(){
    for(int i=0;i<180;i++){
        rain[i].x=(float)(rand()%1200);
        rain[i].y=(float)(650+rand()%250);
        rain[i].spd=7.0f+(rand()%40)/10.0f;
        rain[i].len=12.0f+(rand()%18);
    }
}

float dm(){ return 1.0f; }

/* ===============================================================
   SKY
   =============================================================== */
void drawSky(){
    glBegin(GL_QUADS);
        glColor3f(0.52f,0.80f,0.98f); glVertex2f(0,WIN_H);
        glColor3f(0.52f,0.80f,0.98f); glVertex2f(WIN_W,WIN_H);
        glColor3f(0.70f,0.88f,0.55f); glVertex2f(WIN_W,280);
        glColor3f(0.70f,0.88f,0.55f); glVertex2f(0,280);
    glEnd();
}

/* ===============================================================
   SUN
   =============================================================== */
void drawSunMoon(){
    glPushMatrix();
    glTranslatef(920,620,0);
    glScalef(sunScale,sunScale,1);

    glColor4f(1.0f,0.95f,0.30f,0.18f); fc(0,0,60);
    glColor3f(1.0f,0.90f,0.10f);       fc(0,0,42);
    glColor3f(1.0f,0.72f,0.0f);
    glPointSize(2.0f); midpointCircle(0,0,42); glPointSize(1.0f);

    glColor4f(1.0f,0.88f,0.2f,0.30f);
    for(int i=0;i<12;i++){
        float a=i*PI/6.0f+timer*0.005f;
        ddaLine(50*cosf(a),50*sinf(a),72*cosf(a),72*sinf(a));
    }

    glPopMatrix();
}

/* ===============================================================
   CLOUDS
   =============================================================== */
void drawClouds(){
    for(int i=0;i<5;i++){
        float cx=clouds[i].x,cy=clouds[i].y;
        float sc=clouds[i].sc+0.04f*sinf(timer*0.014f+i*1.6f);
        glColor4f(0.96f,0.98f,1.0f,0.78f);
        fc(cx,cy,24*sc); fc(cx+30*sc,cy-4,20*sc);
        fc(cx-28*sc,cy-3,17*sc); fc(cx+12*sc,cy+13*sc,15*sc);
        fc(cx-14*sc,cy+11*sc,15*sc);
    }
}

/* ===============================================================
   MIST
   =============================================================== */
void drawMist(){
    float sh=mistShear;
    for(int i=0;i<4;i++){
        float y0=70.0f+i*40.0f;
        float al=0.09f-i*0.02f;
        glColor4f(0.82f,0.90f,0.85f,al);
        glBegin(GL_QUADS);
            glVertex2f(sh*y0,y0); glVertex2f(WIN_W+sh*y0,y0);
            glVertex2f(WIN_W+sh*(y0+20),y0+20); glVertex2f(sh*(y0+20),y0+20);
        glEnd();
    }
}

/* ===============================================================
   GROUND
   =============================================================== */
void drawGround(){
    float d=dm();

    glColor3f(0.15f*d,0.52f*d,0.12f*d);
    glBegin(GL_QUADS);
        glVertex2f(0,0);glVertex2f(WIN_W,0);
        glVertex2f(WIN_W,280);glVertex2f(0,280);
    glEnd();

    glColor3f(0.10f*d,0.40f*d,0.08f*d);
    glBegin(GL_QUADS);
        glVertex2f(0,255);glVertex2f(WIN_W,255);
        glVertex2f(WIN_W,285);glVertex2f(0,285);
    glEnd();

    glColor3f(0.55f*d,0.45f*d,0.30f*d);
    glBegin(GL_QUADS);
        glVertex2f(600,0);glVertex2f(780,0);
        glVertex2f(720,280);glVertex2f(640,280);
    glEnd();

    glColor3f(0.80f*d,0.75f*d,0.55f*d);
    for(int i=0;i<6;i++){
        float y=18.0f+i*45.0f;
        ddaLine(658,y,668,y+35);
    }

    glColor3f(0.20f*d,0.62f*d,0.18f*d);
    glLineWidth(1.3f);
    for(int i=0;i<90;i++){
        float bx=5.0f+i*12.2f;
        if(bx>595 && bx<785) continue;
        float tip=sinf(i*1.6f+timer*0.05f)*8.0f;
        float ht=14.0f+sinf(i*0.8f)*6.0f;
        ddaLine(bx,280,bx+tip,280+ht);
    }
    glLineWidth(1.0f);
}

/* ===============================================================
   RIVER
   =============================================================== */
void drawRiver(){
    float d=dm();
    glColor3f(0.12f*d,0.45f*d,0.72f*d);
    glBegin(GL_QUADS);
        glVertex2f(0,80);glVertex2f(480,80);
        glVertex2f(480,235);glVertex2f(0,235);
    glEnd();

    glColor4f(0.55f,0.88f,0.98f,0.30f);
    glBegin(GL_QUADS);
        glVertex2f(0,195);glVertex2f(480,195);
        glVertex2f(480,235);glVertex2f(0,235);
    glEnd();

    glColor4f(1,1,1,0.35f);
    glPointSize(1.3f);
    for(int i=0;i<3;i++) midpointCircle(80+i*145,155,(int)(rippleR+i*5));
    glPointSize(1.0f);
}

/* ===============================================================
   BRIDGE
   =============================================================== */
void drawBridge(){
    float d=dm();

    glColor3f(0.52f*d,0.38f*d,0.18f*d);
    glBegin(GL_QUADS);
        glVertex2f(380,220);glVertex2f(480,220);
        glVertex2f(480,240);glVertex2f(380,240);
    glEnd();

    glColor3f(0.40f*d,0.28f*d,0.10f*d);
    glLineWidth(2.5f);
    bresenhamLine(380,220,380,252);
    bresenhamLine(480,220,480,252);
    bresenhamLine(380,252,480,252);
    bresenhamLine(410,220,410,252);
    bresenhamLine(450,220,450,252);
    glLineWidth(1.0f);
}

/* ===============================================================
   HUT
   =============================================================== */
void drawHut(float bx,float by,bool hasFlag){
    float d=dm();

    glColor3f(0.82f*d,0.72f*d,0.50f*d);
    glBegin(GL_QUADS);
        glVertex2f(bx,by);glVertex2f(bx+120,by);
        glVertex2f(bx+120,by+72);glVertex2f(bx,by+72);
    glEnd();

    glColor3f(0.70f*d,0.60f*d,0.40f*d);
    for(int i=0;i<4;i++)
        bresenhamLine((int)bx,(int)(by+15+i*16),(int)(bx+120),(int)(by+15+i*16));

    glColor3f(0.45f*d,0.28f*d,0.10f*d);
    glBegin(GL_QUADS);
        glVertex2f(bx+42,by);glVertex2f(bx+76,by);
        glVertex2f(bx+76,by+45);glVertex2f(bx+42,by+45);
    glEnd();

    glColor3f(0.75f*d,0.60f*d,0.15f*d);
    fc(bx+72,by+22,2);

    glColor3f(0.55f*d,0.85f*d,0.95f*d);
    glBegin(GL_QUADS);
        glVertex2f(bx+88,by+32);glVertex2f(bx+112,by+32);
        glVertex2f(bx+112,by+55);glVertex2f(bx+88,by+55);
    glEnd();

    glColor3f(0.40f*d,0.25f*d,0.08f*d);
    bresenhamLine((int)(bx+100),(int)(by+32),(int)(bx+100),(int)(by+55));
    bresenhamLine((int)(bx+88),(int)(by+44),(int)(bx+112),(int)(by+44));

    glColor3f(0.58f*d,0.40f*d,0.12f*d);
    glBegin(GL_TRIANGLES);
        glVertex2f(bx-14,by+72);glVertex2f(bx+134,by+72);
        glVertex2f(bx+60,by+138);
    glEnd();

    glColor3f(0.38f*d,0.24f*d,0.06f*d);
    glLineWidth(1.5f);
    bresenhamLine((int)(bx-14),(int)(by+72),(int)(bx+60),(int)(by+138));
    bresenhamLine((int)(bx+134),(int)(by+72),(int)(bx+60),(int)(by+138));
    glLineWidth(1.0f);

    if(hasFlag){
        glColor3f(0.55f*d,0.45f*d,0.30f*d);
        glLineWidth(2.0f);
        bresenhamLine((int)(bx+60),(int)(by+138),(int)(bx+60),(int)(by+175));
        glLineWidth(1.0f);

        glColor3f(0.0f,0.60f*d,0.30f*d);
        glBegin(GL_QUADS);
            glVertex2f(bx+60,by+155);glVertex2f(bx+95,by+155);
            glVertex2f(bx+95,by+175);glVertex2f(bx+60,by+175);
        glEnd();

        glColor3f(0.90f,0.10f,0.10f);
        fc(bx+75,by+165,8);
    }
}

/* ===============================================================
   SMOKE
   =============================================================== */
void drawSmoke(float sx,float sy){
    for(int i=0;i<6;i++){
        float t=i*0.20f;
        float px=sx+smokeShear*t;
        float py=sy+i*20.0f;
        float r=5.0f+i*3.5f;
        glColor4f(0.72f,0.72f,0.72f, 0.75f*(1.0f-t));
        fc(px,py,r);
    }
}

/* ===============================================================
   TREE
   =============================================================== */
void drawTree(int tx,int ty,int h,float lr){
    float d=dm();

    for(int off=-3;off<=3;off++){
        glColor3f((0.35f+off*0.01f)*d,0.20f*d,0.06f*d);
        bresenhamLine(tx+off,ty,tx+off+off/3,ty+h);
    }

    glColor3f(0.30f*d,0.18f*d,0.05f*d);
    bresenhamLine(tx,ty+h*2/3, tx-30,ty+h*2/3+20);
    bresenhamLine(tx,ty+h*2/3, tx+30,ty+h*2/3+20);

    glColor3f(0.06f*d,0.42f*d,0.06f*d); fc(tx,ty+h,lr*1.15f);
    glColor3f(0.10f*d,0.55f*d,0.10f*d); fc(tx,ty+h+lr*0.35f,lr);
    glColor3f(0.15f*d,0.65f*d,0.15f*d); fc(tx,ty+h+lr*0.70f,lr*0.60f);
}

/* ===============================================================
   PALM TREE
   =============================================================== */
void drawPalm(int px,int py){
    float d=dm();

    glColor3f(0.42f*d,0.26f*d,0.08f*d);
    glLineWidth(4.0f);
    for(int i=0;i<6;i++)
        ddaLine(px+i*1.5f,py+i*22.0f,px+i*1.5f+1.5f,py+i*22.0f+22.0f);
    glLineWidth(1.0f);

    int topX=px+9, topY=py+132;
    glColor3f(0.12f*d,0.58f*d,0.12f*d);
    glLineWidth(2.0f);
    for(int i=0;i<8;i++){
        float a=PI/7+i*PI/5.0f;
        ddaLine(topX,topY,topX+(int)(55*cosf(a)),topY+(int)(55*sinf(a)));
    }
    glLineWidth(1.0f);

    glColor3f(0.42f*d,0.28f*d,0.08f*d);
    fc(topX-5,topY-5,5); fc(topX+5,topY-3,4);
}

/* ===============================================================
   MOSQUE
   =============================================================== */
void drawMosque(float mx,float my){
    float d=dm();

    glColor3f(0.80f*d,0.74f*d,0.62f*d);
    glBegin(GL_QUADS);
        glVertex2f(mx,my);glVertex2f(mx+90,my);
        glVertex2f(mx+90,my+60);glVertex2f(mx,my+60);
    glEnd();

    glColor3f(0.68f*d,0.62f*d,0.52f*d);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(mx+45,my+60);
    for(int i=0;i<=20;i++){float a=PI*i/20.0f;glVertex2f(mx+45+32*cosf(a),my+60+22*sinf(a));}
    glEnd();

    glColor3f(0.92f*d,0.85f*d,0.15f*d);
    fc(mx+45,my+85,5);

    for(int m=0;m<2;m++){
        float tx=mx+m*76;
        glColor3f(0.75f*d,0.68f*d,0.58f*d);
        glBegin(GL_QUADS);
            glVertex2f(tx,my);glVertex2f(tx+14,my);
            glVertex2f(tx+14,my+82);glVertex2f(tx,my+82);
        glEnd();
        glBegin(GL_TRIANGLES);
            glVertex2f(tx,my+82);glVertex2f(tx+14,my+82);
            glVertex2f(tx+7,my+100);
        glEnd();
    }

    glColor3f(0.55f*d,0.48f*d,0.38f*d);
    glBegin(GL_TRIANGLE_FAN); glVertex2f(mx+45,my+25);
    for(int i=0;i<=15;i++){float a=PI*i/15.0f;glVertex2f(mx+45+18*cosf(a),my+25+14*sinf(a));}
    glEnd();
}

/* ===============================================================
   CLOCK TOWER
   =============================================================== */
void drawClockTower(float cx,float cy){
    float d=dm();

    glColor3f(0.72f*d,0.65f*d,0.52f*d);
    glBegin(GL_QUADS);
        glVertex2f(cx-18,cy);glVertex2f(cx+18,cy);
        glVertex2f(cx+18,cy+95);glVertex2f(cx-18,cy+95);
    glEnd();

    glColor3f(0.55f*d,0.35f*d,0.10f*d);
    glBegin(GL_TRIANGLES);
        glVertex2f(cx-22,cy+95);glVertex2f(cx+22,cy+95);
        glVertex2f(cx,cy+125);
    glEnd();

    glColor3f(0.95f*d,0.92f*d,0.85f*d);
    fc(cx,cy+72,16);
    glColor3f(0.20f*d,0.15f*d,0.08f*d);
    glPointSize(1.5f);
    midpointCircle((int)cx,(int)(cy+72),16);
    midpointCircle((int)cx,(int)(cy+72),15);
    glPointSize(1.0f);

    glColor3f(0.15f*d,0.10f*d,0.05f*d);
    for(int i=0;i<12;i++){
        float a=i*PI/6.0f;
        ddaLine(cx+13*cosf(a),cy+72+13*sinf(a),cx+15*cosf(a),cy+72+15*sinf(a));
    }

    float ha = clockSec/12.0f;
    glColor3f(0.10f,0.08f,0.05f);
    glLineWidth(2.5f);
    ddaLine(cx,cy+72,cx+9*cosf(ha-PI/2),cy+72+9*sinf(ha-PI/2));
    glLineWidth(1.0f);

    float ma = clockSec;
    glColor3f(0.15f,0.10f,0.05f);
    glLineWidth(1.5f);
    ddaLine(cx,cy+72,cx+13*cosf(ma-PI/2),cy+72+13*sinf(ma-PI/2));
    glLineWidth(1.0f);

    glColor3f(0.90f*d,0.20f,0.10f);
    fc(cx,cy+72,2);
}

/* ===============================================================
   BOAT
   =============================================================== */
void drawBoat(float bx,float by){
    glPushMatrix();
    glTranslatef(bx,by + 2.0f*sinf(timer*0.05f),0);

    glColor3f(0.50f,0.30f,0.10f);
    glBegin(GL_POLYGON);
        glVertex2f(-55,0);glVertex2f(55,0);
        glVertex2f(44,-22);glVertex2f(-44,-22);
    glEnd();

    glColor3f(0.35f,0.22f,0.08f);
    glLineWidth(2.0f); bresenhamLine(0,0,0,55); glLineWidth(1.0f);

    glColor3f(0.96f,0.94f,0.85f);
    glBegin(GL_TRIANGLES);
        glVertex2f(0,55);glVertex2f(0,12);glVertex2f(42,30);
    glEnd();

    glColor3f(0.20f,0.15f,0.10f);
    fc(-22,14,7);
    bresenhamLine(-22,7,-22,-6);
    bresenhamLine(-22,2,-32,-10);
    bresenhamLine(-22,2,-12,-10);

    glColor3f(0.45f,0.35f,0.20f);
    ddaLine(-32,-10,-50,25);

    glColor4f(0.5f,0.5f,0.5f,0.6f);
    ddaLine(-50,25,-50,-18);

    glPopMatrix();
}

/* ===============================================================
   BIRD
   =============================================================== */
void drawBird(float bx,float by){
    glPushMatrix();
    glTranslatef(bx,by,0);

    glPushMatrix(); glRotatef(-wingAngle,0,0,1);
    glColor3f(0.08f,0.06f,0.06f);
    glBegin(GL_TRIANGLES);glVertex2f(0,0);glVertex2f(-20,0);glVertex2f(-10,12);glEnd();
    glPopMatrix();

    glPushMatrix(); glRotatef(wingAngle,0,0,1);
    glColor3f(0.08f,0.06f,0.06f);
    glBegin(GL_TRIANGLES);glVertex2f(0,0);glVertex2f(20,0);glVertex2f(10,12);glEnd();
    glPopMatrix();

    glColor3f(0.06f,0.05f,0.05f);
    fc(0,0,5);

    glPopMatrix();
}

/* ===============================================================
   CAR - MOVES VERTICALLY ALONG ROAD
   =============================================================== */
void drawCar(float cx,float cy,float roadAng){
    glPushMatrix();
    glTranslatef(cx,cy,0);

    glRotatef(roadAng + 90.0f,0,0,1);

    glColor4f(0.0f,0.0f,0.0f,0.15f);
    glBegin(GL_POLYGON);
        glVertex2f(-18,-30);
        glVertex2f( 18,-30);
        glVertex2f( 24, 24);
        glVertex2f(-24, 24);
    glEnd();

    glColor3f(0.08f,0.08f,0.08f);
    glPointSize(2.0f);
    midpointCircle(-12,-18,7);
    midpointCircle( 12,-18,7);
    midpointCircle(-12, 18,7);
    midpointCircle( 12, 18,7);
    glPointSize(1.0f);

    for(int s=0;s<4;s++){
        float a=(carWheelAngle+s*90.0f)*PI/180.0f;
        int dx=(int)(5*cosf(a)), dy=(int)(5*sinf(a));

        glColor3f(0.70f,0.70f,0.70f);
        bresenhamLine(-12,-18,-12+dx,-18+dy);
        bresenhamLine(-12,-18,-12-dx,-18-dy);

        bresenhamLine( 12,-18, 12+dx,-18+dy);
        bresenhamLine( 12,-18, 12-dx,-18-dy);

        bresenhamLine(-12, 18,-12+dx, 18+dy);
        bresenhamLine(-12, 18,-12-dx, 18-dy);

        bresenhamLine( 12, 18, 12+dx, 18+dy);
        bresenhamLine( 12, 18, 12-dx, 18-dy);
    }

    glColor3f(0.82f,0.12f,0.12f);
    glBegin(GL_QUADS);
        glVertex2f(-18,-24);
        glVertex2f( 18,-24);
        glVertex2f( 18, 24);
        glVertex2f(-18, 24);
    glEnd();

    glColor3f(0.90f,0.18f,0.18f);
    glBegin(GL_POLYGON);
        glVertex2f(-14,-6);
        glVertex2f( 14,-6);
        glVertex2f( 10,12);
        glVertex2f(-10,12);
    glEnd();

    glColor3f(0.90f,0.15f,0.15f);
    fc(-6,-26,1.8f);
    fc( 6,-26,1.8f);

    glPopMatrix();
}

/* ===============================================================
   VILLAGER - walks on LEFT side of road
   =============================================================== */
void drawVillager(float vx,float vy,float roadAng){
    glPushMatrix();
    glTranslatef(vx,vy,0);
    glRotatef(roadAng,0,0,1);

    float d = dm();
    float step = 24.0f * sinf(villWalkPhase);
    float arm  = 18.0f * sinf(villWalkPhase);
    float bob  = 2.5f * fabsf(sinf(villWalkPhase));
    glTranslatef(0,bob,0);

    glColor3f(0.75f*d,0.55f*d,0.40f*d);
    fc(0,34,9);

    glColor3f(0.72f*d,0.52f*d,0.38f*d);
    glLineWidth(2.0f);
    ddaLine(0,26,0,22);
    glLineWidth(1.0f);

    glColor3f(0.82f*d,0.22f*d,0.14f*d);
    glBegin(GL_QUADS);
        glVertex2f(-8,0); glVertex2f(8,0);
        glVertex2f(10,22); glVertex2f(-10,22);
    glEnd();

    glColor3f(0.18f*d,0.45f*d,0.72f*d);
    glBegin(GL_QUADS);
        glVertex2f(-8,-2); glVertex2f(8,-2);
        glVertex2f(8,7);   glVertex2f(-8,7);
    glEnd();

    glColor3f(0.74f*d,0.54f*d,0.40f*d);

    glPushMatrix();
    glTranslatef(-8,18,0);
    glRotatef(-arm,0,0,1);
    glLineWidth(3.0f);
    ddaLine(0,0,-12,-14);
    glLineWidth(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(8,18,0);
    glRotatef(arm,0,0,1);
    glLineWidth(3.0f);
    ddaLine(0,0,12,-14);
    glLineWidth(1.0f);
    glPopMatrix();

    glColor3f(0.35f*d,0.22f*d,0.10f*d);

    glPushMatrix();
    glTranslatef(-3,0,0);
    glRotatef(step,0,0,1);
    glLineWidth(3.2f);
    ddaLine(0,0,-2,-18);
    ddaLine(-2,-18,-1,-34);
    glLineWidth(1.0f);
    fc(-1,-35,3);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(3,0,0);
    glRotatef(-step,0,0,1);
    glLineWidth(3.2f);
    ddaLine(0,0,2,-18);
    ddaLine(2,-18,1,-34);
    glLineWidth(1.0f);
    fc(1,-35,3);
    glPopMatrix();

    glPopMatrix();
}

/* ===============================================================
   WINDMILL
   =============================================================== */
void drawWindmill(float wx,float wy){
    float d=dm();

    glColor3f(0.82f*d,0.78f*d,0.72f*d);
    glBegin(GL_QUADS);
        glVertex2f(wx-10,wy);glVertex2f(wx+10,wy);
        glVertex2f(wx+6,wy+90);glVertex2f(wx-6,wy+90);
    glEnd();

    glPushMatrix();
    glTranslatef(wx,wy+90,0);
    glRotatef(wmAngle,0,0,1);
    for(int b=0;b<4;b++){
        glPushMatrix();
        glRotatef(b*90.0f,0,0,1);
        glColor3f(0.94f*d,0.90f*d,0.80f*d);
        glBegin(GL_TRIANGLES);
            glVertex2f(0,0);glVertex2f(-8,45);glVertex2f(8,45);
        glEnd();
        glPopMatrix();
    }

    glColor3f(0.60f*d,0.55f*d,0.42f*d);
    fc(0,0,7);
    glPointSize(1.5f); midpointCircle(0,0,7); glPointSize(1.0f);

    glPopMatrix();
}

/* ===============================================================
   RAIN
   =============================================================== */
void drawRain(){
    if(!rainOn) return;
    glColor4f(0.50f,0.65f,0.92f,0.65f);
    glLineWidth(1.6f);
    for(int i=0;i<180;i++)
        ddaLine(rain[i].x,rain[i].y,rain[i].x-3,rain[i].y-rain[i].len);
    glLineWidth(1.0f);
}

/* ===============================================================
   RAINBOW
   =============================================================== */
void drawRainbow(){
    if(rainbowA<0.01f) return;
    float cols[][3]={
        {0.95f,0.15f,0.15f},{0.95f,0.55f,0.10f},{0.95f,0.90f,0.15f},
        {0.20f,0.80f,0.20f},{0.15f,0.45f,0.90f},{0.30f,0.15f,0.75f},
        {0.55f,0.15f,0.80f}
    };
    float cx=550,cy=120;
    for(int c=0;c<7;c++){
        float r=300.0f+c*13.0f;
        glColor4f(cols[c][0],cols[c][1],cols[c][2],rainbowA*0.32f);
        glLineWidth(4.0f);
        for(int i=0;i<40;i++){
            float a1=PI*0.12f+(PI*0.76f)*i/40.0f;
            float a2=PI*0.12f+(PI*0.76f)*(i+1)/40.0f;
            ddaLine(cx+r*cosf(a1),cy+r*sinf(a1),cx+r*cosf(a2),cy+r*sinf(a2));
        }
    }
    glLineWidth(1.0f);
}

/* ===============================================================
   FLOWER
   =============================================================== */
void drawFlower(float fx,float fy,float r,float pr,float pg,float pb){
    float d=dm();
    glColor3f(0.15f*d,0.50f*d,0.10f*d);
    glLineWidth(2.0f); ddaLine(fx,fy-r*2,fx,fy+r*3.5f); glLineWidth(1.0f);

    glColor3f(pr*d,pg*d,pb*d);
    for(int i=0;i<6;i++){float a=i*PI/3.0f;fc(fx+r*cosf(a),fy+r*sinf(a),r*0.72f);}
    glColor3f(1.0f*d,0.92f*d,0.12f*d); fc(fx,fy,r*0.48f);
}

/* ===============================================================
   DISPLAY
   =============================================================== */
void display(){
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    glTranslatef(WIN_W/2.0f,WIN_H/2.0f,0);
    glScalef(camZoom,camZoom,1.0f);
    glTranslatef(-WIN_W/2.0f+camX,-WIN_H/2.0f+camY,0);

    drawSky();
    drawSunMoon();
    drawClouds();
    drawRainbow();
    drawMist();
    drawGround();
    drawRiver();
    drawBridge();

    drawBoat(boatX,160);

    drawMosque(850,250);

    drawHut(500,160,true);
    drawHut(790,170,false);

    drawSmoke(560,298);
    drawSmoke(850,308);

    drawClockTower(980,250);
    drawWindmill(490,170);

    drawTree(130,180,110,35);
    drawTree(320,175,120,38);
    drawTree(850,180,100,32);
    drawTree(1050,175,115,36);

    drawPalm(50,100);
    drawPalm(440,95);
    drawPalm(1020,90);

    drawFlower(60,190,8,0.95f,0.22f,0.38f);
    drawFlower(200,185,7,0.90f,0.60f,0.12f);
    drawFlower(390,188,8,0.62f,0.25f,0.88f);
    drawFlower(760,182,7,0.95f,0.90f,0.15f);
    drawFlower(940,186,8,0.18f,0.68f,0.90f);

    float rAng = roadAngleDeg();

    {
        float x,y;
        roadCenter(carT,x,y);
        drawCar(x,y,rAng);
    }

    {
        float x,y;
        roadCenter(villT,x,y);
        drawVillager(x-35,y,rAng);
    }

    for(int i=0;i<6;i++)
        drawBird(birdX+i*42.0f, birdY+sinf(i*0.9f)*14.0f);

    drawRain();

    glLoadIdentity();

    glColor3f(1.0f,0.98f,0.88f);
    drawText(10,WIN_H-22,"Village Scenery with Animated Objects",
             GLUT_BITMAP_HELVETICA_12);

    glColor3f(0.95f,0.95f,0.88f);
    drawText(10,WIN_H-40,"Rasel Molla (222-15-6187)", GLUT_BITMAP_HELVETICA_12);
    drawText(10,WIN_H-56,"Remon Hassan (222-15-6375)", GLUT_BITMAP_HELVETICA_12);
    drawText(10,WIN_H-72,"Jayshri Ghosh (222-15-6503)", GLUT_BITMAP_HELVETICA_12);

    glColor3f(0.88f,0.95f,0.88f);
    drawText(10,30,"Left Click=Zoom In  Arrow Keys=Explore  P=Pause  1=Rain  F=Fullscreen  R=Reset  ESC=Quit");

    char st[160];
    sprintf(st,"Zoom: %.0f%%   Camera(%.0f, %.0f)   %s",
            camZoom*100.0f, camX, camY, paused?"PAUSED":"PLAYING");
    glColor3f(0.90f,1.0f,0.82f);
    drawText(10,12,st);

    glutSwapBuffers();
}

/* ===============================================================
   TIMER / ANIMATION
   =============================================================== */
void update(int){
    camX    += (targetCamX - camX) * 0.12f;
    camY    += (targetCamY - camY) * 0.12f;
    camZoom += (targetZoom - camZoom) * 0.12f;

    if(!paused){
        timer++;

        boatX += 0.45f;
        if(boatX > 360.0f) boatX = 40.0f;

        carT += 0.0018f;
        if(carT > 1.0f) carT = 0.02f;

        carWheelAngle += 8.0f;
        if(carWheelAngle >= 360.0f) carWheelAngle -= 360.0f;

        villT += 0.00070f;
        if(villT > 0.95f) villT = 0.68f;
        villWalkPhase += 0.11f;

        birdX += 0.9f;
        if(birdX > 1250) birdX = -400.0f;
        wingAngle = 32.0f*sinf(timer*0.14f);

        wmAngle += 1.3f;
        if(wmAngle >= 360) wmAngle -= 360;

        smokeShear = 14.0f*sinf(timer*0.025f);

        for(int i=0;i<5;i++){
            clouds[i].x += clouds[i].spd;
            if(clouds[i].x > 1180) clouds[i].x = -90.0f;
        }

        sunScale += sunDir;
        if(sunScale > 1.08f || sunScale < 0.92f) sunDir = -sunDir;

        rippleR += 0.20f;
        if(rippleR > 32) rippleR = 4.0f;

        mistShear = 0.06f*sinf(timer*0.016f);

        clockSec += 0.02f;

        if(rainOn){
            for(int i=0;i<180;i++){
                rain[i].y -= rain[i].spd;
                rain[i].x -= 1.0f;
                if(rain[i].y < 0){
                    rain[i].y = 700 + (float)(rand()%220);
                    rain[i].x = (float)(rand()%1200);
                }
            }
            if(rainbowA < 1.0f) rainbowA += 0.003f;
        } else {
            if(rainbowA > 0.0f) rainbowA -= 0.005f;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16,update,0);
}

/* ===============================================================
   KEYBOARD
   =============================================================== */
void keyboard(unsigned char key,int,int){
    switch(key){
        case 27: case 'q': case 'Q':
            exit(0);
            break;

        case 'r': case 'R':
            targetCamX = 0.0f;
            targetCamY = 0.0f;
            targetZoom = 1.0f;
            clampCameraTargets();
            break;

        case 'p': case 'P':
            paused = !paused;
            break;

        case '1':
            rainOn = !rainOn;
            break;

        case 'f': case 'F':
            if(!fullscr){
                svW=glutGet(GLUT_WINDOW_WIDTH);
                svH=glutGet(GLUT_WINDOW_HEIGHT);
                svPX=glutGet(GLUT_WINDOW_X);
                svPY=glutGet(GLUT_WINDOW_Y);
                glutFullScreen();
                fullscr=true;
            } else {
                glutReshapeWindow(svW,svH);
                glutPositionWindow(svPX,svPY);
                fullscr=false;
            }
            break;
    }
    glutPostRedisplay();
}

void specialKeys(int key,int,int){
    switch(key){
        case GLUT_KEY_UP:    targetCamY -= CSPD; break;
        case GLUT_KEY_DOWN:  targetCamY += CSPD; break;
        case GLUT_KEY_LEFT:  targetCamX += CSPD; break;
        case GLUT_KEY_RIGHT: targetCamX -= CSPD; break;
    }
    clampCameraTargets();
    glutPostRedisplay();
}

/* ===============================================================
   MOUSE
   =============================================================== */
void mouseBtn(int btn,int state,int x,int y){
    if(state != GLUT_DOWN) return;

    if(btn == GLUT_LEFT_BUTTON){
        float wx, wy;
        screenToWorld(x,y,wx,wy);
        zoomTowardWorld(wx,wy,0.35f);
        glutPostRedisplay();
    }
}

/* ===============================================================
   MENU
   =============================================================== */
void menuH(int c){
    switch(c){
        case 1: rainOn=!rainOn; break;
        case 3: paused=!paused; break;
        case 4:
            targetCamX=0; targetCamY=0; targetZoom=1.0f;
            clampCameraTargets();
            break;
        case 5: exit(0); break;
    }
    glutPostRedisplay();
}

void createMenu(){
    glutCreateMenu(menuH);
    glutAddMenuEntry("Toggle Rain       (1)",1);
    glutAddMenuEntry("Pause/Resume      (P)",3);
    glutAddMenuEntry("Reset Camera      (R)",4);
    glutAddMenuEntry("Quit            (ESC)",5);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

/* ===============================================================
   RESHAPE / MAIN
   =============================================================== */
void reshape(int w,int h){
    curWinW = w;
    curWinH = h;

    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc,char** argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);
    glutInitWindowSize(WIN_W,WIN_H);
    glutInitWindowPosition(60,40);
    glutCreateWindow("Village Scenery with Animated Objects");

    glClearColor(0.08f,0.40f,0.08f,1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    gluOrtho2D(0,WIN_W,0,WIN_H);

    initParticles();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouseBtn);
    glutTimerFunc(16,update,0);
    createMenu();

    printf("=== Village Scenery with Animated Objects ===\n");
    printf("Controls:\n");
    printf("  Left Click        = Zoom in toward clicked place\n");
    printf("  Arrow Keys        = Explore scene\n");
    printf("  R                 = Reset camera\n");
    printf("  P                 = Pause / Resume\n");
    printf("  1                 = Toggle Rain + Rainbow\n");
    printf("  F                 = Fullscreen toggle\n");
    printf("  Right Click       = Options menu\n");
    printf("  ESC / Q           = Quit\n");

    glutMainLoop();
    return 0;
}
