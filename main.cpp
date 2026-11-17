#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sysutil/sysutil.h>
#include <sysutil/video.h>
#include <rsx/rsx.h>
#include <rsx/gcm_sys.h>
#include <io/pad.h>
#include <tiny3d.h>

#define WIDTH 1280
#define HEIGHT 720

enum GameState { MENU, HOWTO, PLAYING, VICTORY, ELIMINATED };
static GameState state = MENU;
static int menuSel = 0;
static int menuCooldown = 0;
static int phase = 0;
static float timer = 90.0f;
static int frame = 0;
static bool quitGame = false;
static int place = 1;

struct Guy {
    float x, y, z;
    float vx, vy, vz;
    float rot;
    int skin;
    bool finished;
    bool isPlayer;
    float decision;
};
static Guy player;
static Guy bots[11];
static float camX = 0, camZ = -8;

static u32 skinColor[5] = {0xFFFF69B4, 0xFF3498DB, 0xFF2ECC71, 0xFFF1C40F, 0xFF9B59B6};

static void drawRect(float x, float y, float w, float h, u32 color) {
    if(x < -300 || y < -300 || x > WIDTH+300 || y > HEIGHT+300) return;
    if(w < 1) w = 1; if(h < 1) h = 1;
    tiny3d_SetPolygon(TINY3D_QUADS);
    tiny3d_VertexPos(x, y, 0);
    tiny3d_VertexColor(color);
    tiny3d_VertexPos(x+w, y, 0);
    tiny3d_VertexPos(x+w, y+h, 0);
    tiny3d_VertexPos(x, y+h, 0);
    tiny3d_End();
}

static void project3D(float wx, float wy, float wz, float *sx, float *sy, float *scale) {
    float dz = (wz - camZ) + 8.0f;
    if(dz < 1.0f) dz = 1.0f;
    *scale = 400.0f / dz;
    *sx = WIDTH/2 + (wx - camX) * (*scale);
    *sy = HEIGHT/2 - (wy * (*scale)) + (wz - camZ) * 8.0f;
}

static void drawFallGuy(Guy *g) {
    float sx, sy, sc;
    project3D(g->x, g->y, g->z, &sx, &sy, &sc);
    float r = 28 * sc / 50.0f;
    if(r < 8) r = 8; if(r > 62) r = 62;
    u32 col = skinColor[g->skin % 5];
    drawRect(sx - r, sy + r*0.9f, r*2, 6, 0x66000000);
    drawRect(sx - r*1.25f, sy - r*0.2f, r*0.35f, r*0.7f, col);
    drawRect(sx + r*0.9f, sy - r*0.2f, r*0.35f, r*0.7f, col);
    drawRect(sx - r, sy - r, r*2, r*2, col);
    drawRect(sx - r*0.7f, sy - r*1.2f, r*1.4f, r*0.5f, col);
    drawRect(sx - r*0.45f, sy - r*0.2f, r*0.9f, r*0.9f, 0xFFFFFFFF);
    drawRect(sx - r*0.45f, sy - r*0.65f, r*0.25f, r*0.3f, 0xFFFFFFFF);
    drawRect(sx + r*0.2f, sy - r*0.65f, r*0.25f, r*0.3f, 0xFFFFFFFF);
    drawRect(sx - r*0.38f, sy - r*0.58f, r*0.12f, r*0.15f, 0xFF000000);
    drawRect(sx + r*0.27f, sy - r*0.58f, r*0.12f, r*0.15f, 0xFF000000);
    drawRect(sx - r*0.15f, sy - r*0.15f, r*0.3f, r*0.08f, 0xFF000000);
    drawRect(sx - r*0.55f, sy + r*0.85f, r*0.35f, r*0.45f, col);
    drawRect(sx + r*0.2f, sy + r*0.85f, r*0.35f, r*0.45f, col);
    if(g->finished) {
        drawRect(sx - r*0.3f, sy - r*1.55f, r*0.6f, r*0.35f, 0xFFFFD700);
        drawRect(sx - r*0.3f, sy - r*1.75f, r*0.12f, r*0.22f, 0xFFFFD700);
        drawRect(sx + r*0.18f, sy - r*1.75f, r*0.12f, r*0.22f, 0xFFFFD700);
    }
    if(g->isPlayer) {
        drawRect(sx - r*0.3f, sy - r*1.9f, r*0.6f, r*0.18f, 0xFFFFFFFF);
    }
}

static void drawMap() {
    drawRect(0, 0, WIDTH, HEIGHT, 0xFF1E88E5);
    drawRect(0, 0, WIDTH, 90, 0xFF87CEEB);
    drawRect(0, HEIGHT/2, WIDTH, HEIGHT/2, 0xFFFF69B4);
    drawRect(0, HEIGHT/2, WIDTH, 14, 0xFFFFFFFF);
    drawRect(0, HEIGHT-80, WIDTH, 80, 0xFF2ECC71);
    for(int i=0;i<8;i++){
        float z = -15 + i*5.5f;
        float sx, sy, sc;
        project3D(-6, 0, z, &sx, &sy, &sc);
        drawRect(sx, sy, 12*sc, 5, 0xFFFFFFFF);
        project3D(6, 0, z, &sx, &sy, &sc);
        drawRect(sx, sy, 12*sc, 5, 0xFFFFFFFF);
    }
    float spin = (frame % 120) / 120.0f * 6.28f;
    for(int i=0;i<3;i++){
        float ox = -4 + i*4;
        float oz = 4 + i*6;
        float sx, sy, sc;
        project3D(ox, 0, oz, &sx, &sy, &sc);
        float w = 55*sc/50.0f; if(w<10) w=10;
        float h = 95*sc/50.0f; if(h<20) h=20;
        u32 c = (i%2==0)?0xFF3498DB:0xFFFFD700;
        if(phase==1 && i==1) c = 0xFFE74C3C;
        drawRect(sx - w/2, sy - h, w, h, c);
        drawRect(sx - w/2, sy - h, w, 10, 0xFFFFFFFF);
        if(phase>=1){
            float bx = ox + sinf(spin + i)*1.5f;
            float bsx, bsy, bsc;
            project3D(bx, 0.5f, oz+1, &bsx, &bsy, &bsc);
            drawRect(bsx-20, bsy-20, 40, 40, 0xFFE74C3C);
        }
    }
    if(phase < 2){
        float sx, sy, sc;
        project3D(0, 0, 20, &sx, &sy, &sc);
        drawRect(sx - 340, sy - 12, 680, 16, 0xFF000000);
        for(int k=0;k<12;k++) drawRect(sx-340+k*56, sy-12, 28, 16, (k%2)?0xFFFFFFFF:0xFF000000);
    } else {
        float sx, sy, sc;
        project3D(0, 0.6f, 22, &sx, &sy, &sc);
        drawRect(sx - 70, sy, 140, 46, 0xFF9B59B6);
        drawRect(sx - 70, sy, 140, 10, 0xFFFFFFFF);
        drawRect(sx - 28, sy - 55, 56, 32, 0xFFFFD700);
        drawRect(sx - 28, sy - 68, 11, 16, 0xFFFFD700);
        drawRect(sx - 5, sy - 72, 11, 20, 0xFFFFD700);
        drawRect(sx + 17, sy - 68, 11, 16, 0xFFFFD700);
    }
}

static void drawMenu() {
    drawRect(0,0,WIDTH,HEIGHT, 0xFF1E88E5);
    drawRect(0,0,WIDTH,210, 0xFFFF69B4);
    drawRect(0,210,WIDTH,8, 0xFFFFFFFF);
    drawRect(280, 60, 720, 78, 0xFFFFFFFF);
    drawRect(300, 80, 680, 38, 0xFFFF69B4);
    drawRect(340, 230, 600, 36, 0xFF000000);
    for(int i=0;i<3;i++){
        u32 c = (i==menuSel)?0xFFFFD700:0xFFFFFFFF;
        drawRect(440, 300 + i*85, 400, 62, c);
        drawRect(440, 300 + i*85, 400, 8, 0xFF000000);
        drawRect(470, 322 + i*85, 200, 18, 0xFF000000);
    }
    drawRect(80, 620, 1120, 44, 0xFF000000);
    drawRect(100, 660, 1080, 22, 0xFF87CEEB);
    for(int i=0;i<5;i++){
        Guy g; g.x=-4+i*2; g.y=0; g.z=-2; g.skin=i; g.finished=false; g.isPlayer=false;
        drawFallGuy(&g);
    }
}

static void drawHowTo() {
    drawRect(0,0,WIDTH,HEIGHT, 0xFF1E88E5);
    drawRect(0,0,WIDTH,120, 0xFFFF69B4);
    drawRect(140, 180, 1000, 380, 0xFFFFFFFF);
    drawRect(160, 200, 960, 22, 0xFF000000);
    drawRect(160, 250, 700, 22, 0xFF000000);
    drawRect(160, 300, 850, 22, 0xFF000000);
    drawRect(160, 350, 600, 22, 0xFF000000);
    drawRect(160, 400, 800, 22, 0xFF000000);
    drawRect(160, 460, 700, 22, 0xFF000000);
    drawRect(440, 600, 400, 60, 0xFFFFD700);
}

static void drawHUD() {
    drawRect(0,0,WIDTH,54, 0xCC000000);
    float pct = timer/90.0f; if(pct<0) pct=0; if(pct>1) pct=1;
    drawRect(20, 16, 300*pct, 22, 0xFFFFD700);
    drawRect(360, 14, 140, 26, 0xFFFF69B4);
    drawRect(540, 14, 140, 26, 0xFF3498DB);
    drawRect(720, 14, 140, 26, 0xFF2ECC71);
    drawRect(1080, 10, 160, 34, 0xFFFFFFFF);
}

static void drawEnd(bool win) {
    drawRect(0,0,WIDTH,HEIGHT, win?0xFFFFD700:0xFFE74C3C);
    drawRect(0,0,WIDTH,180, 0xFF1E88E5);
    drawRect(240, 260, 800, 120, 0xFFFFFFFF);
    drawRect(240, 260, 800, 18, 0xFF000000);
    drawRect(440, 460, 400, 62, 0xFF000000);
    drawRect(460, 480, 360, 22, 0xFFFFFFFF);
}

static void resetRace() {
    player.x=0; player.y=0; player.z=-14; player.vx=0; player.vy=0; player.vz=0; player.rot=0; player.skin=0; player.finished=false; player.isPlayer=true; player.decision=0;
    for(int i=0;i<11;i++){
        bots[i].x=-6 + i*1.2f; bots[i].y=0; bots[i].z=-14 + (i%3)*0.8f;
        bots[i].vx=0; bots[i].vy=0; bots[i].vz=0.07f + (i%4)*0.015f;
        bots[i].skin=(i+1)%5; bots[i].finished=false; bots[i].isPlayer=false; bots[i].decision=0; bots[i].rot=0;
    }
    timer=90.0f; camX=0; camZ=-8; place=1;
}

static void updatePlayer(padData *pad) {
    float ax=0, az=0;
    if(pad->ANA_L_H > 140) ax=0.09f;
    if(pad->ANA_L_H < 110) ax=-0.09f;
    if(pad->ANA_L_V > 140) az=0.09f;
    if(pad->ANA_L_V < 110) az=-0.09f;
    if(pad->BTN_R1){ ax*=1.8f; az*=1.8f; }
    if(pad->BTN_L1){ ax*=0.6f; az*=0.6f; }
    player.x += ax;
    player.z += az + 0.045f;
    if(pad->BTN_CROSS && player.y <= 0.01f){ player.vy=0.30f; }
    player.vy -= 0.012f;
    player.y += player.vy;
    if(player.y <= 0){ player.y=0; player.vy=0; }
    if(player.x < -7.5f) player.x=-7.5f;
    if(player.x > 7.5f) player.x=7.5f;
    if(player.z < -15) player.z=-15;
    camX = player.x*0.5f;
    camZ = player.z - 6;
}

static void updateBots() {
    for(int i=0;i<11;i++){
        Guy *b = &bots[i];
        if(b->finished) continue;
        b->decision += 0.016f;
        float targetX = 0;
        if(i%3==0) targetX = -3.0f;
        else if(i%3==1) targetX = 0.0f;
        else targetX = 3.0f;
        if(b->decision > 1.0f){
            b->decision = 0;
            if(rand()%100 < 30) targetX += (rand()%5 - 2)*0.8f;
            b->vx = (targetX - b->x)*0.02f;
            if(b->vx > 0.06f) b->vx=0.06f;
            if(b->vx < -0.06f) b->vx=-0.06f;
        }
        float doorX = -4 + (i%3)*4;
        float doorZ = 4 + (i%3)*6;
        if(fabsf(b->z - doorZ) < 1.2f && fabsf(b->x - doorX) < 0.8f){
            b->vx += (b->x < doorX ? -0.06f : 0.06f);
            if(b->y <= 0.01f) b->vy = 0.24f;
        }
        b->x += b->vx;
        b->z += b->vz;
        b->vy -= 0.012f;
        b->y += b->vy;
        if(b->y <= 0){ b->y=0; b->vy=0; }
        if(b->x < -7.5f){ b->x=-7.5f; b->vx=0.05f; }
        if(b->x > 7.5f){ b->x=7.5f; b->vx=-0.05f; }
    }
}

static void checkFinish() {
    int finishedCount = 0;
    for(int i=0;i<11;i++) if(bots[i].finished) finishedCount++;
    if(phase < 2){
        if(player.z >= 20.0f && !player.finished){
            player.finished=true;
            place = finishedCount + 1;
            if(phase==0){ phase=1; player.z=-14; player.x=0; for(int i=0;i<11;i++){ if(!bots[i].finished){ bots[i].z=-14+(i%3)*0.8f; bots[i].x=-6+i*1.2f; } } timer=90.0f; player.finished=false; }
            else if(phase==1){ phase=2; player.z=-14; player.x=0; for(int i=0;i<11;i++){ if(!bots[i].finished){ bots[i].z=-14+(i%3)*0.8f; } } timer=90.0f; player.finished=false; }
        }
        for(int i=0;i<11;i++){
            if(!bots[i].finished && bots[i].z >= 20.0f) bots[i].finished=true;
        }
    } else {
        float dx = player.x - 0;
        float dz = player.z - 22.0f;
        float dist = sqrtf(dx*dx + dz*dz);
        if(dist < 1.6f && player.y >= 0.8f){
            player.finished=true;
            state = VICTORY;
        }
        for(int i=0;i<11;i++){
            float bx = bots[i].x;
            float bz = bots[i].z - 22.0f;
            float bd = sqrtf(bx*bx + bz*bz);
            if(!bots[i].finished && bd < 1.6f && bots[i].y >= 0.8f){
                bots[i].finished=true;
                state = ELIMINATED;
            }
        }
    }
    if(timer <= 0){
        if(phase==2) state = ELIMINATED;
        else { phase++; player.z=-14; player.x=0; timer=90.0f; player.finished=false; }
    }
}

static void sysutilCallback(u64 status, u64 param, void *userdata) {
    (void)param; (void)userdata;
    if(status == SYSUTIL_EXIT_GAME){ quitGame = true; }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    sysUtilRegisterCallback(0, sysutilCallback, NULL);
    ioPadInit(7);
    srand(12345);
    tiny3d_Init(1024*1024);
    tiny3d_Project(1000.0f, 1.0f, 0.01f, 100.0f, 0.0f, 0.0f);
    resetRace();
    padData pad;
    memset(&pad, 0, sizeof(pad));
    while(!quitGame){
        sysUtilCheckCallback();
        ioPadGetData(0, &pad);
        if(pad.BTN_SELECT){ quitGame=true; break; }
        if(menuCooldown > 0) menuCooldown--;
        tiny3d_Clear(0xFF1E88E5, TINY3D_CLEAR_ALL);
        tiny3d_Project(1000.0f, 1.0f, 0.01f, 100.0f, 0.0f, 0.0f);
        if(state==MENU){
            if(menuCooldown==0){
                if(pad.ANA_L_V > 180 || pad.BTN_DOWN){ menuSel++; if(menuSel>2) menuSel=0; menuCooldown=12; }
                if(pad.ANA_L_V < 70 || pad.BTN_UP){ menuSel--; if(menuSel<0) menuSel=2; menuCooldown=12; }
                if(pad.BTN_CROSS){
                    menuCooldown=15;
                    if(menuSel==0){ resetRace(); phase=0; state=PLAYING; }
                    else if(menuSel==1){ state=HOWTO; }
                    else { quitGame=true; break; }
                }
            }
            drawMenu();
        } else if(state==HOWTO){
            if(pad.BTN_CROSS && menuCooldown==0){ state=MENU; menuCooldown=15; }
            if(pad.BTN_CIRCLE && menuCooldown==0){ state=MENU; menuCooldown=15; }
            drawHowTo();
        } else if(state==PLAYING){
            frame++;
            timer -= 1.0f/60.0f;
            updatePlayer(&pad);
            updateBots();
            checkFinish();
            if(pad.BTN_CIRCLE && menuCooldown==0){ state=MENU; menuCooldown=15; }
            drawMap();
            for(int i=0;i<11;i++) drawFallGuy(&bots[i]);
            drawFallGuy(&player);
            drawHUD();
        } else if(state==VICTORY || state==ELIMINATED){
            if(pad.BTN_CROSS && menuCooldown==0){ resetRace(); phase=0; state=MENU; menuCooldown=15; }
            if(pad.BTN_CIRCLE && menuCooldown==0){ resetRace(); phase=0; state=MENU; menuCooldown=15; }
            drawMap();
            drawFallGuy(&player);
            drawEnd(state==VICTORY);
        }
        tiny3d_Flip();
        sysUtilCheckCallback();
    }
    tiny3d_Close();
    ioPadEnd();
    sysUtilExitGame();
    return 0;
}