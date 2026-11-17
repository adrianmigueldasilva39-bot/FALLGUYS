#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <io/pad.h>
#include <tiny3d.h>

typedef struct {
    float x, y, z;
    float vx, vz;
    int skin;
    int isPlayer;
    int finished;
    int alive;
} Guy;

Guy player;
Guy bots[11];
int gameState = 0;
int phase = 0;
int frame = 0;
float doorOffset = 0;
float hammerX = 640;

u32 skinColor[12] = {
    0xFF2E2EFF, 0xFF00BFFF, 0xFFFF69B4, 0xFFFFFF00,
    0xFF00FF00, 0xFF800080, 0xFFFFA500, 0xFF00FFFF,
    0xFF00FF7F, 0xFFFF1493, 0xFF1E90FF, 0xFFFFFFFF
};

void drawRect(float x, float y, float w, float h, u32 color) {
    tiny3d_SetPolygon(TINY3D_QUADS);
    tiny3d_VertexPos(x, y, 0); tiny3d_VertexColor(color);
    tiny3d_VertexPos(x + w, y, 0); tiny3d_VertexColor(color);
    tiny3d_VertexPos(x + w, y + h, 0); tiny3d_VertexColor(color);
    tiny3d_VertexPos(x, y + h, 0); tiny3d_VertexColor(color);
    tiny3d_End();
}

void project3D(float x, float y, float z, float *sx, float *sy, float *sc) {
    float scale = 900.0f / (900.0f + z);
    if (scale < 0.3f) scale = 0.3f;
    if (scale > 1.2f) scale = 1.2f;
    *sx = 640 + (x - 640) * scale;
    *sy = 400 + (y - 400) * scale - z * 0.35f;
    *sc = scale * 100.0f;
}

static void drawFallGuy(Guy *g) {
    float sx, sy, sc;
    project3D(g->x, g->y, g->z, &sx, &sy, &sc);
    float r = 30 * sc / 50.0f;
    if(r < 8) r = 8; if(r > 65) r = 65;
    u32 col = skinColor[g->skin % 12];

    // 10 desenhos so - MAX FPS
    drawRect(sx - r*0.8f, sy + r*0.95f, r*1.6f, r*0.2f, 0x55000000);
    drawRect(sx - r*0.6f, sy + r*0.5f, r*0.35f, r*0.45f, col);
    drawRect(sx + r*0.25f, sy + r*0.5f, r*0.35f, r*0.45f, col);
    drawRect(sx - r*0.95f, sy - r*0.3f, r*1.9f, r*0.8f, col);
    drawRect(sx - r*0.7f, sy - r*0.9f, r*1.4f, r*0.6f, col);
    drawRect(sx - r*0.75f, sy - r*0.45f, r*1.5f, r*0.65f, 0xFFFFFFFF);
    drawRect(sx - r*0.35f, sy - r*0.35f, r*0.22f, r*0.28f, 0xFF000000);
    drawRect(sx + r*0.13f, sy - r*0.35f, r*0.22f, r*0.28f, 0xFF000000);
    drawRect(sx - r*1.15f, sy - r*0.2f, r*0.35f, r*0.6f, col);
    drawRect(sx + r*0.8f, sy - r*0.2f, r*0.35f, r*0.6f, col);

    if(g->finished) {
        drawRect(sx - r*0.3f, sy - r*1.45f, r*0.6f, r*0.3f, 0xFFFFD700);
    }
    if(g->isPlayer) {
        drawRect(sx - r*0.25f, sy - r*1.8f, r*0.5f, r*0.15f, 0xFFFF5722);
    }
}

void drawCrown(float x, float y) {
    drawRect(x - 15, y, 30, 12, 0xFFFFD700);
    drawRect(x - 10, y - 10, 8, 10, 0xFFFFD700);
    drawRect(x + 2, y - 10, 8, 10, 0xFFFFD700);
}

void resetGame() {
    player.x = 640; player.y = 400; player.z = 800;
    player.vx = 0; player.vz = 0;
    player.skin = 0; player.isPlayer = 1; player.finished = 0; player.alive = 1;
    for (int i = 0; i < 11; i++) {
        bots[i].x = 200 + i * 80;
        bots[i].y = 400;
        bots[i].z = 800 + (rand() % 60);
        bots[i].vx = 0; bots[i].vz = 0;
        bots[i].skin = i + 1;
        bots[i].isPlayer = 0; bots[i].finished = 0; bots[i].alive = 1;
    }
    phase = 0;
    doorOffset = 0;
    hammerX = 640;
}

int main() {
    tiny3d_Init(1024*1024);
    ioPadInit(7);
    padData pad;
    resetGame();

    while (1) {
        frame++;
        ioPadGetData(0, &pad);
        int btnX = pad.BTN_CROSS;
        int btnCircle = pad.BTN_CIRCLE;
        int btnSelect = pad.BTN_SELECT;
        int btnR1 = pad.BTN_R1;
        float ax = 0, az = 0;
        if (pad.ANA_L_H > 140) ax = 1;
        if (pad.ANA_L_H < 110) ax = -1;
        if (pad.ANA_L_V > 140) az = 1;
        if (pad.ANA_L_V < 110) az = -1;

        tiny3d_Clear(0xFF3A86FF, TINY3D_CLEAR_ALL);
        tiny3d_Project2D();

        if (gameState == 0) {
            drawRect(0, 0, 1280, 720, 0xFFFF69B4);
            drawRect(240, 120, 800, 80, 0xFFFFFFFF);
            drawRect(300, 140, 680, 40, 0xFF2E2EFF);
            Guy ex1 = player; ex1.x = 640; ex1.y = 400; ex1.z = 200;
            Guy ex2 = bots[1]; ex2.x = 400; ex2.y = 400; ex2.z = 200;
            Guy ex3 = bots[2]; ex3.x = 880; ex3.y = 400; ex3.z = 200;
            drawFallGuy(&ex1);
            drawFallGuy(&ex2);
            drawFallGuy(&ex3);
            drawRect(440, 550, 400, 60, 0xFF00FF00);
            drawRect(440, 630, 400, 30, 0xFFFFFFFF);
            if (btnX) { resetGame(); gameState = 1; }
        }
        else if (gameState == 1) {
            drawRect(0, 0, 1280, 720, 0xFF87CEEB);
            drawRect(0, 620, 1280, 100, 0xFF00FF00);

            float speed = btnR1 ? 6.0f : 3.5f;
            player.x += ax * speed;
            player.z -= az * speed;
            if (btnX) player.y -= 4; else if (player.y < 400) player.y += 4;
            if (player.x < 100) player.x = 100;
            if (player.x > 1180) player.x = 1180;
            if (player.z < 0) player.z = 0;
            if (player.z > 800) player.z = 800;

            for (int i = 0; i < 11; i++) {
                if (!bots[i].alive) continue;
                bots[i].z -= 2.0f + (i % 3) * 0.3f;
                bots[i].x += (i % 2 == 0) ? 0.4f : -0.4f;
                if (bots[i].x < 100) bots[i].x = 100;
                if (bots[i].x > 1180) bots[i].x = 1180;
                if (phase == 2 && bots[i].z < 80) {
                    bots[i].finished = 1;
                    gameState = 3;
                }
            }

            if (phase == 0) {
                doorOffset += 1.0f;
                if (doorOffset > 200) doorOffset = -200;
                float doorX = 640 + doorOffset;
                drawRect(0, 200, 1280, 25, 0xFFFFA500);
                drawRect(0, 200, doorX - 60, 90, 0xFFFFA500);
                drawRect(doorX + 60, 200, 1280, 90, 0xFFFFA500);
                if (player.z < 550) phase = 1;
            }
            else if (phase == 1) {
                hammerX += 3.0f;
                if (hammerX > 1100) hammerX = 180;
                drawRect(0, 200, 1280, 25, 0xFFFFA500);
                drawRect(hammerX - 40, 180, 80, 60, 0xFFFF0000);
                if (player.z < 350 && player.z > 250) {
                    if (player.x > hammerX - 50 && player.x < hammerX + 50) {
                        player.z = 500;
                    }
                }
                if (player.z < 250) phase = 2;
            }
            else if (phase == 2) {
                drawCrown(640, 180);
                drawRect(0, 200, 1280, 25, 0xFFFFA500);
                if (player.z < 80 && player.x > 580 && player.x < 700) {
                    player.finished = 1;
                    gameState = 2;
                }
            }

            drawFallGuy(&player);
            for (int i = 0; i < 11; i++) drawFallGuy(&bots[i]);

            drawRect(20, 20, 200, 20, 0xFFFFFFFF);
            drawRect(20, 20, 200 * (phase + 1) / 3, 20, 0xFF00FF00);
            if (btnSelect) gameState = 0;
        }
        else if (gameState == 2) {
            drawRect(0, 0, 1280, 720, 0xFFFFD700);
            Guy w = player; w.x = 640; w.y = 400; w.z = 200;
            drawFallGuy(&w);
            drawRect(400, 500, 480, 60, 0xFF00FF00);
            if (btnX) { resetGame(); gameState = 1; }
            if (btnCircle) gameState = 0;
        }
        else if (gameState == 3) {
            drawRect(0, 0, 1280, 720, 0xFFFF0000);
            Guy l = bots[1]; l.x = 640; l.y = 400; l.z = 200;
            drawFallGuy(&l);
            drawRect(400, 500, 480, 60, 0xFFFFFFFF);
            if (btnX) { resetGame(); gameState = 1; }
            if (btnCircle) gameState = 0;
        }

        tiny3d_Flip();
    }
    return 0;
}