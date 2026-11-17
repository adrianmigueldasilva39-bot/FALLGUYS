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
    int coins;
    int dashTimer;
    int dashCooldown;
    int jumps;
    int place;
    float stun;
} Guy;

Guy player;
Guy bots[11];
int gameState = 0; // 0 menu, 1 jogando, 2 venceu, 3 perdeu
int phase = 0; // 0 porta, 1 martelo, 2 bolinhas, 3 girando, 4 piso some, 5 coroa
int frame = 0;
float doorOffset = 0;
float hammerX = 640;
float spinAngle = 0;
float gameTimer = 0;
int difficulty = 0; // 0 facil, 1 dificil
int selectedSkin = 0;
int coinsOnMap[10];
float coinsX[10], coinsZ[10];
int coinTaken[10];
float ballsX[6], ballsZ[6];
float ballsSpeed[6];

u32 skinColor[12] = {
    0xFF2E2EFF, 0xFF00BFFF, 0xFFFF69B4, 0xFFFFFF00,
    0xFF00FF00, 0xFF800080, 0xFFFFA500, 0xFF00FFFF,
    0xFF00FF7F, 0xFFFF1493, 0xFF1E90FF, 0xFFFFFFFF
};
char *skinNames[12] = {"VERMELHO","AZUL","ROSA","AMARELO","VERDE","ROXO","LARANJA","CIANO","VERDE CLARO","ROSA CHOQUE","AZUL CLARO","BRANCO"};

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
    // Se ta com dash, fica brilhando - borda branca
    if (g->dashTimer > 0) {
        drawRect(sx - r*1.2f, sy - r*1.2f, r*2.4f, r*2.4f, 0xFFFFFFFF);
    }
    if (g->stun > 0) col = 0xFF808080; // cinza quando atordoado
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
    if(g->finished) drawRect(sx - r*0.3f, sy - r*1.45f, r*0.6f, r*0.3f, 0xFFFFD700);
    if(g->isPlayer) drawRect(sx - r*0.25f, sy - r*1.8f, r*0.5f, r*0.15f, 0xFFFF5722);
    // Barra de dash em cima de tu
    if(g->isPlayer) {
        drawRect(sx - 20, sy - r*2.0f, 40, 5, 0xFF000000);
        int w = 40;
        if (g->dashCooldown > 0) w = 40 * (60 - g->dashCooldown) / 60;
        drawRect(sx - 20, sy - r*2.0f, w, 5, 0xFF00FFFF);
    }
}

void drawCrown(float x, float y) {
    drawRect(x - 15, y, 30, 12, 0xFFFFD700);
    drawRect(x - 10, y - 10, 8, 10, 0xFFFFD700);
    drawRect(x + 2, y - 10, 8, 10, 0xFFFFD700);
}

void drawCoin(float x, float z) {
    float sx, sy, sc;
    project3D(x, 400, z, &sx, &sy, &sc);
    drawRect(sx - 8, sy - 8, 16, 16, 0xFFFFD700);
    drawRect(sx - 3, sy - 8, 6, 16, 0xFFFFFF00);
}

void resetGame() {
    player.x = 640; player.y = 400; player.z = 800;
    player.vx = 0; player.vz = 0;
    player.skin = selectedSkin; player.isPlayer = 1; player.finished = 0; player.alive = 1;
    player.coins = 0; player.dashTimer = 0; player.dashCooldown = 0; player.jumps = 0; player.stun = 0;
    for (int i = 0; i < 11; i++) {
        bots[i].x = 200 + i * 80; bots[i].y = 400; bots[i].z = 800 + (rand() % 60);
        bots[i].vx = 0; bots[i].vz = 0;
        bots[i].skin = (selectedSkin + 1 + i) % 12; // nunca repete tua cor
        bots[i].isPlayer = 0; bots[i].finished = 0; bots[i].alive = 1;
        bots[i].coins = 0; bots[i].dashTimer = 0; bots[i].dashCooldown = 0; bots[i].jumps = 0; bots[i].stun = 0;
    }
    phase = 0; doorOffset = 0; hammerX = 640; spinAngle = 0; gameTimer = 0;
    for (int i = 0; i < 10; i++) {
        coinsX[i] = 200 + i * 100;
        coinsZ[i] = 600 - (i % 5) * 100;
        coinTaken[i] = 0;
    }
    for (int i = 0; i < 6; i++) {
        ballsX[i] = 200 + i * 180;
        ballsZ[i] = 400;
        ballsSpeed[i] = 2.0f + (i % 3);
    }
}

int main() {
    tiny3d_Init(1024*1024);
    ioPadInit(7);
    padData pad;
    resetGame();
    int wasX = 0, wasSquare = 0, wasR1 = 0, wasL1 = 0, wasUp = 0;

    while (1) {
        frame++;
        ioPadGetData(0, &pad);
        int btnX = pad.BTN_CROSS;
        int btnCircle = pad.BTN_CIRCLE;
        int btnSquare = pad.BTN_SQUARE;
        int btnSelect = pad.BTN_SELECT;
        int btnR1 = pad.BTN_R1;
        int btnL1 = pad.BTN_L1;
        int btnStart = pad.BTN_START;
        float ax = 0, az = 0;
        if (pad.ANA_L_H > 140) ax = 1;
        if (pad.ANA_L_H < 110) ax = -1;
        if (pad.ANA_L_V > 140) az = 1;
        if (pad.ANA_L_V < 110) az = -1;

        tiny3d_Clear(0xFF3A86FF, TINY3D_CLEAR_ALL);
        tiny3d_Project2D();

        // ===== MENU - ESCOLHER SKIN + DIFICULDADE =====
        if (gameState == 0) {
            drawRect(0, 0, 1280, 720, 0xFFFF69B4);
            drawRect(240, 80, 800, 60, 0xFFFFFFFF);
            drawRect(300, 95, 680, 30, 0xFF2E2EFF);
            // Preview tua skin
            Guy prev = player; prev.skin = selectedSkin; prev.x = 640; prev.y = 400; prev.z = 200; prev.isPlayer = 1;
            drawFallGuy(&prev);
            // Barra da cor - 12 quadradinhos
            for (int i = 0; i < 12; i++) {
                drawRect(200 + i * 75, 500, 60, 40, skinColor[i]);
                if (i == selectedSkin) drawRect(200 + i * 75, 500, 60, 8, 0xFFFFFFFF);
            }
            // Dificuldade
            drawRect(440, 570, 180, 40, difficulty == 0 ? 0xFF00FF00 : 0xFF808080);
            drawRect(660, 570, 180, 40, difficulty == 1 ? 0xFFFF0000 : 0xFF808080);
            drawRect(440, 640, 400, 50, 0xFF00FF00);
            // Troca skin L1/R1
            if (btnR1 && !wasR1) { selectedSkin = (selectedSkin + 1) % 12; }
            if (btnL1 && !wasL1) { selectedSkin = (selectedSkin + 11) % 12; }
            // Troca dificuldade Quadrado
            if (btnSquare && !wasSquare) difficulty = 1 - difficulty;
            if (btnX) { resetGame(); gameState = 1; }
            wasR1 = btnR1; wasL1 = btnL1; wasSquare = btnSquare;
        }
        // ===== JOGANDO =====
        else if (gameState == 1) {
            gameTimer += 0.016f;
            drawRect(0, 0, 1280, 720, 0xFF87CEEB);
            drawRect(0, 620, 1280, 100, 0xFF00FF00);

            // --- TU ---
            if (player.stun > 0) player.stun -= 0.016f;
            if (player.dashCooldown > 0) player.dashCooldown--;
            if (player.dashTimer > 0) player.dashTimer--;

            float speed = 3.5f;
            if (player.dashTimer > 0) speed = 7.0f; // DASH
            else if (btnR1 && player.dashCooldown <= 0) { player.dashTimer = 30; player.dashCooldown = 90; }

            if (player.stun <= 0) {
                player.x += ax * speed;
                player.z -= az * speed;
            }
            // PULO DUPLO
            if (btnX && !wasX) {
                if (player.y >= 399) { player.y -= 80; player.jumps = 1; }
                else if (player.jumps == 1) { player.y -= 80; player.jumps = 2; }
            }
            wasX = btnX;
            if (!btnX && player.y < 400) player.y += 3;
            if (player.y > 400) player.y = 400;
            if (player.x < 100) player.x = 100;
            if (player.x > 1180) player.x = 1180;
            if (player.z < 0) player.z = 0;
            if (player.z > 800) player.z = 800;

            // EMPURRAR - Quadrado perto do bot
            if (btnSquare && !wasSquare) {
                for (int i = 0; i < 11; i++) {
                    float dx = bots[i].x - player.x;
                    float dz = bots[i].z - player.z;
                    if (dx*dx + dz*dz < 80*80) {
                        bots[i].x += dx * 1.5f;
                        bots[i].z += dz * 1.5f;
                        bots[i].stun = 1.0f;
                    }
                }
            }
            wasSquare = btnSquare;

            // --- BOTS ---
            float botSpeed = difficulty == 0 ? 2.0f : 3.2f;
            for (int i = 0; i < 11; i++) {
                if (!bots[i].alive) continue;
                if (bots[i].stun > 0) { bots[i].stun -= 0.016f; continue; }
                bots[i].z -= botSpeed + (i % 3) * 0.3f;
                bots[i].x += (i % 2 == 0) ? 0.4f : -0.4f;
                if (bots[i].x < 100) bots[i].x = 100;
                if (bots[i].x > 1180) bots[i].x = 1180;
                // Bot pega moeda as vezes
                if (rand() % 300 == 0 && bots[i].coins < 5) bots[i].coins++;
                if (phase == 5 && bots[i].z < 80) { bots[i].finished = 1; gameState = 3; }
            }

            // --- MOEDAS ---
            for (int i = 0; i < 10; i++) {
                if (coinTaken[i]) continue;
                drawCoin(coinsX[i], coinsZ[i]);
                float dx = player.x - coinsX[i];
                float dz = player.z - coinsZ[i];
                if (dx*dx + dz*dz < 40*40) {
                    coinTaken[i] = 1; player.coins++;
                    if (player.coins >= 5) { player.dashTimer = 60; player.coins = 0; } // 5 moedas = turbo
                }
            }

            // --- FASES ---
            if (phase == 0) { // PORTA
                doorOffset += 1.0f;
                if (doorOffset > 200) doorOffset = -200;
                float doorX = 640 + doorOffset;
                drawRect(0, 200, 1280, 25, 0xFFFFA500);
                drawRect(0, 200, doorX - 60, 90, 0xFFFFA500);
                drawRect(doorX + 60, 200, 1280, 90, 0xFFFFA500);
                if (player.z < 550) phase = 1;
            }
            else if (phase == 1) { // MARTELO
                hammerX += difficulty == 0 ? 3.0f : 5.0f;
                if (hammerX > 1100) hammerX = 180;
                drawRect(0, 200, 1280, 25, 0xFFFFA500);
                drawRect(hammerX - 40, 180, 80, 60, 0xFFFF0000);
                if (player.z < 350 && player.z > 250) {
                    if (player.x > hammerX - 50 && player.x < hammerX + 50) { player.z = 500; player.stun = 1.0f; }
                }
                if (player.z < 250) phase = 2;
            }
            else if (phase == 2) { // BOLINHAS CAINDO
                for (int i = 0; i < 6; i++) {
                    ballsZ[i] += ballsSpeed[i] + (difficulty ? 1.0f : 0);
                    if (ballsZ[i] > 700) { ballsZ[i] = 100; ballsX[i] = 150 + rand() % 900; }
                    float sx, sy, sc;
                    project3D(ballsX[i], 400, ballsZ[i], &sx, &sy, &sc);
                    drawRect(sx - 15, sy - 15, 30, 30, 0xFFFF0000);
                    float dx = player.x - ballsX[i];
                    float dz = player.z - ballsZ[i];
                    if (dx*dx + dz*dz < 35*35) { player.z = 400; player.stun = 1.0f; }
                }
                drawRect(0, 200, 1280, 25, 0xFFFFA500);
                if (player.z < 250) phase = 3;
            }
            else if (phase == 3) { // PLATAFORMA GIRANDO
                spinAngle += 0.03f;
                float px = 640 + cosf(spinAngle) * 200;
                drawRect(0, 200, 1280, 25, 0xFFFFA500);
                drawRect(px - 100, 180, 200, 20, 0xFF00FF00);
                // Se tu nao ta na plataforma, volta
                if (player.z < 350 && player.z > 200) {
                    if (player.x < px - 100 || player.x > px + 100) { player.z = 450; }
                }
                if (player.z < 200) phase = 4;
            }
            else if (phase == 4) { // PISO QUE SOME
                // 5 quadrados, 1 some a cada 2 seg
                int hide = (frame / 120) % 5;
                for (int i = 0; i < 5; i++) {
                    if (i == hide) continue;
                    drawRect(180 + i * 200, 200, 150, 60, 0xFF00BFFF);
                }
                if (player.z < 350 && player.z > 200) {
                    int myTile = (int)((player.x - 180) / 200);
                    if (myTile == hide) { player.z = 450; player.stun = 0.5f; }
                }
                if (player.z < 150) phase = 5;
            }
            else if (phase == 5) { // COROA FINAL
                drawCrown(640, 180);
                drawRect(0, 200, 1280, 25, 0xFFFFA500);
                if (player.z < 80 && player.x > 580 && player.x < 700) {
                    player.finished = 1; gameState = 2;
                }
            }

            // Desenha todo mundo
            drawFallGuy(&player);
            for (int i = 0; i < 11; i++) drawFallGuy(&bots[i]);

            // HUD - fase, moedas, tempo, lugar
            drawRect(20, 20, 300, 20, 0xFFFFFFFF);
            drawRect(20, 20, 300 * (phase + 1) / 6, 20, 0xFF00FF00);
            // Moedas - quadradinhos amarelos
            for (int i = 0; i < player.coins; i++) drawRect(20 + i * 25, 50, 18, 18, 0xFFFFD700);
            // Lugar - baseado no z
            int place = 1;
            for (int i = 0; i < 11; i++) if (bots[i].z < player.z) place++;
            drawRect(20, 80, 20 + place * 10, 20, 0xFF00FFFF);
            // Timer barra
            int t = (int)gameTimer;
            drawRect(1100, 20, 140, 20, 0xFFFFFFFF);
            drawRect(1100, 20, 140 - (t % 60), 20, 0xFFFFA500);

            if (btnSelect) gameState = 0;
        }
        else if (gameState == 2) {
            drawRect(0, 0, 1280, 720, 0xFFFFD700);
            Guy w = player; w.x = 640; w.y = 400; w.z = 200;
            drawFallGuy(&w);
            drawCrown(640, 150);
            drawRect(400, 500, 480, 60, 0xFF00FF00);
            // Mostra tempo e moedas
            drawRect(500, 580, 280, 20, 0xFFFFFFFF);
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