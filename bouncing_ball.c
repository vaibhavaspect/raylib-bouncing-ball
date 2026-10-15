#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdbool.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600
#define TRAIL_MAX_POINTS 24

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float radius;
    Color coreColor;
    Color glowColor;
    
    Vector2 trail[TRAIL_MAX_POINTS];
    int trailHead;
    float trailTimer;
} Ball;

typedef struct {
    float gravity;
    float restitution;
    float friction;
    float speedMultiplier;
    bool isPaused;
} SimulationConfig;

void InitBall(Ball *ball, float x, float y) {
    ball->position = (Vector2){ x, y };
    ball->velocity = (Vector2){ 450.0f, -250.0f };
    ball->radius = 24.0f;
    ball->coreColor = (Color){ 0, 225, 255, 255 };
    ball->glowColor = (Color){ 0, 150, 255, 70 };
    ball->trailHead = 0;
    ball->trailTimer = 0.0f;

    for (int i = 0; i < TRAIL_MAX_POINTS; i++) {
        ball->trail[i] = ball->position;
    }
}

void UpdateTrail(Ball *ball, float dt) {
    ball->trailTimer += dt;
    if (ball->trailTimer >= 0.016f) {
        ball->trailTimer = 0.0f;
        ball->trail[ball->trailHead] = ball->position;
        ball->trailHead = (ball->trailHead + 1) % TRAIL_MAX_POINTS;
    }
}

void HandleInput(Ball *ball, SimulationConfig *cfg) {
    // SPACE: Pause / Resume
    if (IsKeyPressed(KEY_SPACE)) {
        cfg->isPaused = !cfg->isPaused;
    }

    // R: Reset
    if (IsKeyPressed(KEY_R)) {
        InitBall(ball, 150.0f, 150.0f);
    }

    // UP ARROW: Increase ball velocity directly
    if (IsKeyDown(KEY_UP)) {
        float factor = 1.0f + 1.2f * GetFrameTime();
        ball->velocity.x *= factor;
        ball->velocity.y *= factor;

        // If the ball has come to rest on the floor, give it an upward boost
        if (fabsf(ball->velocity.y) < 10.0f) {
            ball->velocity.y = -350.0f;
        }
        if (fabsf(ball->velocity.x) < 10.0f) {
            ball->velocity.x = 250.0f;
        }
    }

    // DOWN ARROW: Decrease ball velocity directly
    if (IsKeyDown(KEY_DOWN)) {
        float factor = 1.0f - 1.2f * GetFrameTime();
        if (factor < 0.1f) factor = 0.1f;
        ball->velocity.x *= factor;
        ball->velocity.y *= factor;
    }
}

void UpdatePhysics(Ball *ball, const SimulationConfig *cfg, float dt) {
    if (cfg->isPaused) return;

    // Apply gravity
    ball->velocity.y += cfg->gravity * dt;

    // Apply position integration
    ball->position.x += ball->velocity.x * dt;
    ball->position.y += ball->velocity.y * dt;

    // Boundary Collisions
    // Floor
    if (ball->position.y + ball->radius >= (float)SCREEN_HEIGHT) {
        ball->position.y = (float)SCREEN_HEIGHT - ball->radius;
        ball->velocity.y = -ball->velocity.y * cfg->restitution;
        ball->velocity.x *= cfg->friction;

        if (fabsf(ball->velocity.y) < 30.0f) {
            ball->velocity.y = 0.0f;
        }
    }

    // Ceiling
    if (ball->position.y - ball->radius <= 0.0f) {
        ball->position.y = ball->radius;
        ball->velocity.y = -ball->velocity.y * cfg->restitution;
        ball->velocity.x *= cfg->friction;
    }

    // Right wall
    if (ball->position.x + ball->radius >= (float)SCREEN_WIDTH) {
        ball->position.x = (float)SCREEN_WIDTH - ball->radius;
        ball->velocity.x = -ball->velocity.x * cfg->restitution;
    }

    // Left wall
    if (ball->position.x - ball->radius <= 0.0f) {
        ball->position.x = ball->radius;
        ball->velocity.x = -ball->velocity.x * cfg->restitution;
    }

    UpdateTrail(ball, dt);
}

void DrawSceneBackground(void) {
    Color topDark = (Color){ 12, 18, 30, 255 };
    Color bottomDark = (Color){ 5, 7, 12, 255 };
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, topDark, bottomDark);
    DrawRectangleLines(2, 2, SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, (Color){ 40, 55, 80, 180 });
}

void DrawBall(const Ball *ball) {
    // 1. Trail
    for (int i = 0; i < TRAIL_MAX_POINTS; i++) {
        int index = (ball->trailHead + i) % TRAIL_MAX_POINTS;
        float progress = (float)i / (float)TRAIL_MAX_POINTS;
        float trailRadius = ball->radius * (0.25f + 0.65f * progress);
        unsigned char alpha = (unsigned char)(110.0f * progress * progress);

        Color trailColor = ball->glowColor;
        trailColor.a = alpha;
        DrawCircleV(ball->trail[index], trailRadius, trailColor);
    }

    // 2. Glow
    DrawCircleGradient(ball->position,
                       ball->radius * 2.2f,
                       (Color){ ball->coreColor.r, ball->coreColor.g, ball->coreColor.b, 65 },
                       (Color){ 0, 0, 0, 0 });

    // 3. Ring
    DrawCircleV(ball->position, ball->radius * 1.15f, (Color){ 120, 240, 255, 120 });

    // 4. Core
    DrawCircleV(ball->position, ball->radius, ball->coreColor);

    // 5. Specular
    Vector2 highlightOffset = {
        ball->position.x - ball->radius * 0.30f,
        ball->position.y - ball->radius * 0.35f
    };
    DrawCircleV(highlightOffset, ball->radius * 0.28f, (Color){ 255, 255, 255, 190 });
}

void DrawHUD(const Ball *ball, const SimulationConfig *cfg) {
    float currentSpeed = Vector2Length(ball->velocity);

    DrawRectangle(16, 16, 310, 110, (Color){ 10, 15, 25, 200 });
    DrawRectangleLines(16, 16, 310, 110, (Color){ 60, 80, 110, 180 });

    char hudText[64];

    DrawFPS(28, 26);

    snprintf(hudText, sizeof(hudText), "Speed: %.1f px/s", currentSpeed);
    DrawText(hudText, 28, 52, 18, RAYWHITE);

    if (cfg->isPaused) {
        DrawText("[PAUSED - PRESS SPACE]", 28, 80, 16, (Color){ 255, 90, 90, 255 });
    } else {
        DrawText("[RUNNING]", 28, 80, 16, (Color){ 80, 240, 120, 255 });
    }

    const char *controls = "SPACE: Pause/Play  |  UP/DOWN: Speed Up/Down  |  R: Reset  |  ESC: Exit";
    int textWidth = MeasureText(controls, 14);
    DrawRectangle((SCREEN_WIDTH - textWidth) / 2 - 10, SCREEN_HEIGHT - 32, textWidth + 20, 24, (Color){ 10, 15, 25, 180 });
    DrawText(controls, (SCREEN_WIDTH - textWidth) / 2, SCREEN_HEIGHT - 26, 14, (Color){ 180, 195, 215, 240 });
}

int main(void) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Bouncing Ball Simulation - raylib");
    SetTargetFPS(60);

    SimulationConfig cfg = {
        .gravity = 980.0f,
        .restitution = 0.85f,
        .friction = 0.99f,
        .speedMultiplier = 1.0f,
        .isPaused = false
    };

    Ball ball;
    InitBall(&ball, 150.0f, 150.0f);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        HandleInput(&ball, &cfg);
        UpdatePhysics(&ball, &cfg, dt);

        BeginDrawing();
            ClearBackground(BLACK);
            DrawSceneBackground();
            DrawBall(&ball);
            DrawHUD(&ball, &cfg);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}