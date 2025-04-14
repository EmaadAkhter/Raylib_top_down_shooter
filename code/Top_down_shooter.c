#include "raylib.h"
#include <math.h>
#include <stdio.h>

#define WALL_THICKNESS 35
#define MAX_BULLETS 10
#define MAX_ENEMIES 30
#define screenWidth 772
#define screenHeight 700

typedef struct Bullet {
    int x, y;
    int velocityX, velocityY;
    bool active;
} Bullet;

typedef struct Enemy {
    int x;
    int y;
    float speed;
    bool alive;
} Enemy;
Color transparentWhite = (Color){255, 255, 255, 0};
// Line segment intersection for bullet-wall collision
bool CheckLineIntersection(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4) {
    float denom = (float)((x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4));
    if (denom == 0) return false;
    float t = (float)((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    float u = (float)((x1 - x3) * (y1 - y2) - (y1 - y3) * (x1 - x2)) / denom;
    return (t >= 0 && t <= 1 && u >= 0 && u <= 1);
}

bool CheckBulletWallCollision(Bullet bullet, int walls[][4], int wallCount) {
    for (int i = 0; i < wallCount; i++) {
        if (CheckLineIntersection(bullet.x, bullet.y, bullet.x + bullet.velocityX, bullet.y + bullet.velocityY,
                                  walls[i][0], walls[i][1], walls[i][2], walls[i][3])) {
            return true;
        }
    }
    return false;
}

bool CheckCircleWallCollision(int nextX, int nextY, int radius, int walls[][4], int wallCount) {
    for (int i = 0; i < wallCount; i++) {
        float x1 = walls[i][0], y1 = walls[i][1];
        float x2 = walls[i][2], y2 = walls[i][3];
        float px = x2 - x1;
        float py = y2 - y1;
        float norm = px * px + py * py;
        float u = ((nextX - x1) * px + (nextY - y1) * py) / norm;
        u = fmaxf(0, fminf(1, u));
        float closestX = x1 + u * px;
        float closestY = y1 + u * py;
        float dx = nextX - closestX;
        float dy = nextY - closestY;
        float distSq = dx * dx + dy * dy;
        if (distSq <= radius * radius) return true;
    }
    return false;
}

void DrawWalls(int walls[][4], int wallCount) {
    for (int i = 0; i < wallCount; i++) {
        Vector2 start = { (float)walls[i][0], (float)walls[i][1] };
        Vector2 end = { (float)walls[i][2], (float)walls[i][3] };
        DrawLineEx(start, end, WALL_THICKNESS, transparentWhite);
    }
}

void ResetGame(int *playerX, int *playerY, Bullet bullets[], Enemy enemies[], int *enemyCount, int *score, int *kills, int *lives, bool *gameOver) {
    *playerX = screenWidth / 2;
    *playerY = screenHeight / 1.9;
    *enemyCount = 0;
    *score = 0;
    *kills = 0;
    *lives = 3;
    *gameOver = false;

    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = false;
    }

    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].alive = false;
    }
}

int main(void) {
    InitWindow(screenWidth, screenHeight, "Raylib Game");
    SetTargetFPS(60);

    float timer = 0.0f;
    float enemy_swpan_timer=0.0f;
    float spawn_interval = 1.0f;
    Enemy enemies[MAX_ENEMIES] = { 0 };
    int enemyCount = 0;
    int score = 0;
    int kills = 0;
    int lives = 3;
    bool gameOver = false;
    Image player_image = LoadImage("Soldier-Idle.png");
    ImageResize(&player_image, 165, 165);
    Texture2D player_texture = LoadTextureFromImage(player_image);
    UnloadImage(player_image);

    Image emeny_image = LoadImage("Orc-Idle.png");
    ImageResize(&emeny_image, 175, 175);
    Texture2D emeny_texture = LoadTextureFromImage(emeny_image);
    UnloadImage(emeny_image);

    Image bullet_image = LoadImage("bullet.png");
    ImageResize(&bullet_image, 180,80);
    Texture2D bullet_texture = LoadTextureFromImage(bullet_image);
    UnloadImage(bullet_image);

    Image background_image = LoadImage("back.png");
    ImageResize(&background_image, 772,700);
    Texture2D background_texture = LoadTextureFromImage(background_image);
    UnloadImage(background_image);

    int playerX = screenWidth / 2;
    int playerY = screenHeight / 1.9;

    Bullet bullets[MAX_BULLETS] = { 0 };

    int walls[][4] = {
        {0, 0, 772, 0}, {772, 0, 772, 767}, {772, 695, 0, 695}, {0, 767, 0, 0},
        {70, 70, 70, 160}, {70, 160, 160, 160}, {160, 160, 160, 70}, {610, 70, 610, 160},
        {610, 160, 700, 160}, {700, 160, 700, 70}, {310, 70, 460, 70}, {310, 70, 310, 180},
        {460, 70, 460, 180}, {240, 230, 530, 230}, {310, 280, 310, 340}, {460, 280, 460, 340},
        {310, 340, 460, 340}, {240, 400, 530, 400}, {310, 470, 310, 600}, {460, 470, 460, 600},
        {310, 470, 460, 470}, {70, 600, 160, 600}, {70, 600, 70, 650}, {160, 600, 160, 650},
        {610, 600, 700, 600}, {610, 600, 610, 650}, {700, 600, 700, 650}, {160, 230, 160, 400},
        {160, 400, 240, 400}, {610, 230, 610, 400}, {610, 400, 530, 400}, {380, 310, 380, 340},
        {390, 310, 390, 340}, {380, 310, 390, 310}, {380, 340, 390, 340}
    };
    int wallCount = sizeof(walls) / sizeof(walls[0]);
    

    while (!WindowShouldClose()) {
        if (!gameOver) {
            timer += GetFrameTime();
            enemy_swpan_timer+=GetFrameTime();
            score = (int)(timer * 10) + (kills * 100);

            if (enemy_swpan_timer >= spawn_interval && enemyCount < MAX_ENEMIES) {
                enemy_swpan_timer = 0.0f;
                enemies[enemyCount] = (Enemy){ GetRandomValue(0, screenWidth), GetRandomValue(0, screenHeight), GetRandomValue(1, 2), true };
                enemyCount++;
            }

            int newX = playerX;
            int newY = playerY;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) newX += 2;
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) newX -= 2;
            if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) newY += 2;
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) newY -= 2;

            if (!CheckCircleWallCollision(newX, newY, 15, walls, wallCount)) {
                playerX = newX;
                playerY = newY;
            }

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                for (int i = 0; i < MAX_BULLETS; i++) {
                    if (!bullets[i].active) {
                        bullets[i].x = playerX;
                        bullets[i].y = playerY;
                        int mx = GetMouseX();
                        int my = GetMouseY();
                        int dx = mx - playerX;
                        int dy = my - playerY;
                        float len = sqrtf(dx * dx + dy * dy);
                        bullets[i].velocityX = (int)(5 * dx / len);
                        bullets[i].velocityY = (int)(5 * dy / len);
                        bullets[i].active = true;
                        break;
                    }
                }
            }

            for (int i = 0; i < MAX_BULLETS; i++) {
                if (bullets[i].active) {
                    bullets[i].x += bullets[i].velocityX;
                    bullets[i].y += bullets[i].velocityY;

                    if (CheckBulletWallCollision(bullets[i], walls, wallCount) ||
                        bullets[i].x < 0 || bullets[i].x > screenWidth ||
                        bullets[i].y < 0 || bullets[i].y > screenHeight) {
                        bullets[i].active = false;
                    }

                    for (int j = 0; j < enemyCount; j++) {
                        if (enemies[j].alive && CheckCollisionCircles((Vector2){ bullets[i].x, bullets[i].y }, 5,
                                                                      (Vector2){ enemies[j].x, enemies[j].y }, 20)) {
                            enemies[j].alive = false;
                            bullets[i].active = false;
                            kills++;
                        }
                    }
                }
            }

            for (int i = 0; i < enemyCount; i++) {
                if (enemies[i].alive) {
                    float dx = playerX - enemies[i].x;
                    float dy = playerY - enemies[i].y;
                    float len = sqrtf(dx * dx + dy * dy);

                    if (len != 0) {
                        float dirX = dx / len;
                        float dirY = dy / len;

                        int nextX = enemies[i].x + enemies[i].speed * dirX;
                        int nextY = enemies[i].y + enemies[i].speed * dirY;

                        if (!CheckCircleWallCollision(nextX, nextY, 20, walls, wallCount)) {
                            enemies[i].x = nextX;
                            enemies[i].y = nextY;
                        } else {
                            bool moved = false;
                            float angleOffsets[] = { 0.5f, -0.5f, 1.0f, -1.0f };
                            for (int j = 0; j < 4; j++) {
                                float angle = angleOffsets[j];
                                float cosA = cosf(angle);
                                float sinA = sinf(angle);
                                float newDirX = dirX * cosA - dirY * sinA;
                                float newDirY = dirX * sinA + dirY * cosA;

                                int altX = enemies[i].x + enemies[i].speed * newDirX;
                                int altY = enemies[i].y + enemies[i].speed * newDirY;

                                if (!CheckCircleWallCollision(altX, altY, 20, walls, wallCount)) {
                                    enemies[i].x = altX;
                                    enemies[i].y = altY;
                                    moved = true;
                                    break;
                                }
                            }
                            if (!moved) {
                                // Enemy is stuck, do nothing
                            }
                        }

                        if (CheckCollisionCircles((Vector2){ playerX, playerY }, 15,
                                                  (Vector2){ enemies[i].x, enemies[i].y }, 20)) {
                            lives--;
                            enemies[i].alive = false;
                            if (lives <= 0) gameOver = true;
                        }
                    }
                }
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(background_texture,0 ,0, WHITE);

        DrawWalls(walls, wallCount);

        for (int i = 0; i < MAX_BULLETS; i++) {
            if (bullets[i].active) {DrawCircle(bullets[i].x, bullets[i].y, 5, transparentWhite);
                DrawTexture(bullet_texture,bullets[i].x-100, bullets[i].y-40, WHITE);}
        }

        for (int i = 0; i < enemyCount; i++) {
            if (enemies[i].alive) 
            {DrawCircle(enemies[i].x, enemies[i].y, 20, transparentWhite);
                DrawTexture(emeny_texture,enemies[i].x-100, enemies[i].y-86, WHITE);}
        }

        DrawCircle(playerX, playerY, 20, transparentWhite);
        DrawTexture(player_texture, playerX-86, playerY-79, WHITE);

        DrawText(TextFormat("Score: %d", score), 10, 10, 20, RAYWHITE);
        DrawText(TextFormat("Kills: %d", kills), 10, 40, 20, RED);
        DrawText(TextFormat("Lives: %d", lives), 10, 70, 20, GREEN);

        if (gameOver) {
            DrawText("GAME OVER!", screenWidth / 2 - 100, screenHeight / 2, 40, RED);
            DrawText("Press R to Restart", screenWidth / 2 - 100, screenHeight / 2 + 40, 20, BLACK);
            if (IsKeyPressed(KEY_R)) {
                ResetGame(&playerX, &playerY, bullets, enemies, &enemyCount, &score, &kills, &lives, &gameOver);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

