// Tiny Football - SDL2 with Kick Feature
// Single-file minimal implementation satisfying BTL2 criteria.
// Dependencies: SDL2, SDL2_ttf
// Compile (Linux/mac):
// g++ main.cpp -o tinyfootball `sdl2-config --cflags --libs` -lSDL2_ttf -std=c++17
// On Windows (MinGW):
// g++ main.cpp -o tinyfootball -I/path/to/SDL2/include -L/path/to/SDL2/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -std=c++17

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <cstdio>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Screen
constexpr int SCREEN_W = 1300;
constexpr int SCREEN_H = 800;

// Forward
enum class Team { Blue, Red };
struct Player;

// Utility
float clampf(float v, float a, float b){ return (v<a)?a:((v>b)?b:v); }

// =====================================
// Ball
// =====================================
struct Ball {
    float x, y;
    float vx, vy;
    int size;
    SDL_Texture *tex = nullptr;

    // Trạng thái xoay
    float angle     = 0.0f;   // góc hiện tại (độ)
    float spinSpeed = 0.0f;   // tốc độ xoay (độ/giây)

    // Tham số điều chỉnh
    static constexpr float FRICTION_PER_SEC = 0.98f; // ma sát mỗi giây (tịnh tiến)
    static constexpr float MIN_STOP_SPEED   = 10.0f; // ngưỡng dừng hẳn (px/s)
    static constexpr float SPIN_COEFF       = 5.0f;  // hệ số quy đổi px/s -> độ/giây
    static constexpr float SPIN_SMOOTH      = 0.85f; // trộn mượt spinSpeed (0..1)

    Ball(int sx=SCREEN_W/2, int sy=SCREEN_H/2, int s=12){
        x = sx; y = sy; size = s;
        // vx = 300.0f * (rand()%2 ? 1.0f : -1.0f);
        // vy = 120.0f * ((rand()%100)/100.0f - 0.5f);
        vx = 0.0f;
        vy = 0.0f;
    }

    SDL_Rect rect() const {
        return { (int)std::round(x), (int)std::round(y), size, size };
    }

    void reset(bool towardsLeft){
        x = SCREEN_W/2 - size/2;
        y = SCREEN_H/2 - size/2;
        vx = (towardsLeft? -1.0f : 1.0f) * 280.0f;
        vy = 80.0f * ((rand()%100)/100.0f - 0.5f);
        // reset xoay nhẹ
        spinSpeed = 0.0f;
        angle = 0.0f;
    }

    void update(float dt){
        // 1) Cập nhật vị trí
        x += vx * dt;
        y += vy * dt;

        // 2) Ma sát tịnh tiến (scale theo 60fps để ổn định)
        float factor = powf(FRICTION_PER_SEC, dt * 60.0f);
        vx *= factor;
        vy *= factor;

        // 3) Chặn nhỏ -> 0 để không “trôi”
        if (fabsf(vx) < MIN_STOP_SPEED) vx = 0.0f;
        if (fabsf(vy) < MIN_STOP_SPEED) vy = 0.0f;

        // 4) Tính tốc độ xoay dựa trên vận tốc & hướng:
        //    - Độ lớn: tỉ lệ với tốc độ tịnh tiến
        //    - Dấu: dùng dấu của vx để xác định chiều quay (đơn giản mà nhìn tự nhiên)
        float speed = std::sqrt(vx*vx + vy*vy);
        float dir   = (vx >= 0.0f) ? 1.0f : -1.0f;

        float targetSpin = dir * speed * SPIN_COEFF;

        // 5) Trộn mượt + ma sát quay đồng bộ với tịnh tiến
        spinSpeed = spinSpeed * SPIN_SMOOTH + targetSpin * (1.0f - SPIN_SMOOTH);
        spinSpeed *= factor;

        // 6) Cập nhật góc
        angle += spinSpeed * dt;

        // 7) Chuẩn hoá góc về [0,360)
        while (angle >= 360.0f) angle -= 360.0f;
        while (angle <    0.0f) angle += 360.0f;
    }

    // Kick ball from a player position with given force
    void kick(float fromX, float fromY, float force = 400.0f){
        float ballCenterX = x + size/2.0f;
        float ballCenterY = y + size/2.0f;

        float dx = ballCenterX - fromX;
        float dy = ballCenterY - fromY;
        float dist = std::sqrt(dx*dx + dy*dy);

        if(dist > 0.0001f){
            dx /= dist;  // normalize
            dy /= dist;

            // Cộng vận tốc do cú sút
            vx += dx * force;
            vy += dy * force;

            float side = (dx >= 0.0f) ? 1.0f : -1.0f; // xoáy phụ thuộc hướng x
            spinSpeed += side * 250.0f;
        }
    }
};


// =====================================
// Player (composed of body + arm + leg)
// =====================================
struct Player {
    // Kích thước sprite Kenney gốc
    static constexpr int SRC_BODY_W = 21;
    static constexpr int SRC_BODY_H = 31;
    static constexpr int SRC_ARM_W  = 19;
    static constexpr int SRC_ARM_H  = 13;
    static constexpr int SRC_LEG_W  = 19;
    static constexpr int SRC_LEG_H  = 13;

    // Hệ số phóng to để nhìn rõ
    static constexpr float SCALE = 1.4f;

    // Kích thước vẽ (đã scale)
    static constexpr int BODY_W = int(SRC_BODY_W * SCALE);
    static constexpr int BODY_H = int(SRC_BODY_H * SCALE);
    static constexpr int ARM_W  = int(SRC_ARM_W  * SCALE);
    static constexpr int ARM_H  = int(SRC_ARM_H  * SCALE);
    static constexpr int LEG_W  = int(SRC_LEG_W  * SCALE);
    static constexpr int LEG_H  = int(SRC_LEG_H  * SCALE);

    SDL_Rect r; // hitbox/logic = BODY_W/H
    float speed = 260.0f;

    // key mapping
    SDL_Scancode up, down, left, right, kick;
    bool active = true;
    bool isAI = false;
    float kickRange = 50.0f; // tăng nhẹ cho dễ sút

    // textures Kenney
    SDL_Texture* texBody = nullptr;
    SDL_Texture* texArm  = nullptr;
    SDL_Texture* texLeg  = nullptr;

    Team team = Team::Blue;
    SDL_Color jerseyTint = {255,255,255,255};

    // vị trí hiển thị mượt
    float visX = 0.f, visY = 0.f;
    float smooth = 12.0f;

    // animation
    float animTime = 0.0f;
    float moveX = 0, moveY = 0;

    Player(int x=0,int y=0,int w=BODY_W,int h=BODY_H){
        r.x=x; r.y=y; r.w=w; r.h=h;
        visX = (float)x;  // để cả cầu thủ inactive vẫn xuất hiện
        visY = (float)y;
    }

    void update_from_keyboard(const Uint8* keystate, float dt){
        if(!active || isAI) return;
        int dy = 0, dx = 0;
        if(keystate[up]) dy -= 1;
        if(keystate[down]) dy += 1;
        if(keystate[left]) dx -= 1;
        if(keystate[right]) dx += 1;
        float len = std::sqrt((float)(dx*dx + dy*dy));
        if(len > 0.01f){ dx = (int)std::round(dx/len); dy = (int)std::round(dy/len); }
        moveX = (float)dx;
        moveY = (float)dy;

        r.x += (int)std::round(dx * speed * dt);
        r.y += (int)std::round(dy * speed * dt);

        r.x = (int)clampf(r.x, 0, SCREEN_W - r.w);
        r.y = (int)clampf(r.y, 0, SCREEN_H - r.h);

        if(dx != 0 || dy != 0) animTime += dt; else animTime = 0;

        visX += ((float)r.x - visX) * clampf(smooth * dt, 0.f, 1.f);
        visY += ((float)r.y - visY) * clampf(smooth * dt, 0.f, 1.f);
    }

    void update_AI(const Ball& b, float dt){
        if(!isAI) return;
        float targetY = b.y + b.size/2 - r.h/2;
        float dy = targetY - r.y;
        moveX = 0; moveY = 0;
        if(std::abs(dy) > 6){
            float dir = (dy>0)?1:-1;
            r.y += (int)std::round(dir * speed * dt * 0.8f);
            moveY = dir;
        }
        r.y = (int)clampf(r.y, 0, SCREEN_H - r.h);
        if(moveX != 0 || moveY != 0) animTime += dt; else animTime = 0;

        visX += ((float)r.x - visX) * clampf(smooth * dt, 0.f, 1.f);
        visY += ((float)r.y - visY) * clampf(smooth * dt, 0.f, 1.f);
    }

    bool canKickBall(const Ball& ball) const {
        float cx = r.x + r.w/2.0f;
        float cy = r.y + r.h/2.0f;
        float bx = ball.x + ball.size/2.0f;
        float by = ball.y + ball.size/2.0f;
        float dx = cx - bx, dy = cy - by;
        return (dx*dx + dy*dy) <= (kickRange*kickRange);
    }

    void kickBall(Ball& ball) const {
        if(canKickBall(ball)){
            float cx = r.x + r.w/2.0f;
            float cy = r.y + r.h/2.0f;
            ball.kick(cx, cy, 450.0f);
        }
    }

void render(SDL_Renderer* renderer){
    // Fallback nếu thiếu sprite → vẽ rect màu đội
    if(!texBody || !texLeg){
        if(team == Team::Blue) SDL_SetRenderDrawColor(renderer, 80,150,255,255);
        else                   SDL_SetRenderDrawColor(renderer, 255,170,60,255);
        SDL_Rect rr = { (int)std::round(visX), (int)std::round(visY), r.w, r.h };
        SDL_RenderFillRect(renderer, &rr);
        if(active){
            SDL_SetRenderDrawColor(renderer, 255,235,80,230);
            SDL_Rect bd = { rr.x-2, rr.y-2, rr.w+4, rr.h+4 };
            SDL_RenderDrawRect(renderer, &bd);
        }
        return;
    }

    // ===== 1) Tâm nhân vật & bóng đổ nhẹ
    const int baseX = (int)std::round(visX);
    const int baseY = (int)std::round(visY);
    const float cx  = baseX + BODY_W * 0.5f;
    const float cy  = baseY + BODY_H * 0.5f;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0,0,0,70);
    SDL_Rect shadow = { (int)(cx - BODY_W*0.25f), (int)(baseY + BODY_H - 8), BODY_W/2, 7 };
    SDL_RenderFillRect(renderer, &shadow);

// ===== 2) Hướng nhìn (idle nhìn xuống)
float angleDeg = atan2f(moveY, moveX) * 180.0f / (float)M_PI;
if (fabsf(moveX) < 0.1f && fabsf(moveY) < 0.1f) angleDeg = 90.0f;
const float angRad = angleDeg * (float)M_PI / 180.0f;

// forward & right (LOCAL axes của nhân vật)
const float fx = cosf(angRad), fy = sinf(angRad);
const float rx = fy,          ry =  -fx;

// ===== 3) Offset quanh tâm (đã tinh chỉnh cho bộ Kenney)
const float ARM_DIST   = BODY_W * 0.36f;   // tay bám sát thân
const float LEG_DIST   = BODY_H * 0.05f;   // hông ngay sau thân
const float LEG_SPREAD = BODY_W * 0.26f;   // xòe chân
const float STEP_LEN   = BODY_H * 0.20f;   // biên độ bước
const float runCycle   = sinf(animTime * 10.0f);
const float stepF      = runCycle * STEP_LEN;

// Vai (tay)
const float shoulderLx = cx - ARM_DIST * rx;
const float shoulderLy = cy - ARM_DIST * ry;
const float shoulderRx = cx + ARM_DIST * rx;
const float shoulderRy = cy + ARM_DIST * ry;

// ===== SỬA Ở ĐÂY: tính hông theo RIGHT + FORWARD (không dùng cos/sin với độ)
// "sang phải" của thân: +HIP_OFFSET_X * (rx, ry)
// "xuống dưới" theo hướng nhìn: +HIP_OFFSET_Y * (fx, fy)
const float HIP_OFFSET_X = BODY_W * 0.2f;   // lệch sang phải
const float HIP_OFFSET_Y = BODY_H * 0.25f;   // lệch xuống theo hướng nhìn

const float hipBaseX = cx + HIP_OFFSET_X * rx + HIP_OFFSET_Y * fx;
const float hipBaseY = cy + HIP_OFFSET_X * ry + HIP_OFFSET_Y * fy;

// Hông (chân) + hiệu ứng bước chạy dọc theo forward
const float hipLx = hipBaseX - LEG_SPREAD * rx + stepF * fx;   // chân trái
const float hipLy = hipBaseY - LEG_SPREAD * ry + stepF * fy;
const float hipRx = hipBaseX + LEG_SPREAD * rx - stepF * fx;   // chân phải
const float hipRy = hipBaseY + LEG_SPREAD * ry - stepF * fy;



    // ===== 4) Vẽ theo "điểm khớp" (pivot)
    auto drawAtPivot = [&](SDL_Texture* tex, float jx, float jy,
                           int w, int h, float deg,
                           int pivotX, int pivotY)
    {
        SDL_Rect dst{ int(jx - pivotX), int(jy - pivotY), w, h };
        SDL_Point pivot{ pivotX, pivotY };
        SDL_RenderCopyEx(renderer, tex, nullptr, &dst, deg, &pivot, SDL_FLIP_NONE);
    };

    // Pivot của sprite (điểm dính vào thân)
    const int ARM_PIVOT_L_X = int(ARM_W * 0.15f), ARM_PIVOT_L_Y = ARM_H/2; // tay trái: mép trong
    const int ARM_PIVOT_R_X = int(ARM_W * 0.15f), ARM_PIVOT_R_Y = ARM_H/2; // tay phải: mép trong
    const int LEG_PIVOT_X   = LEG_W/2,            LEG_PIVOT_Y   = int(LEG_H * 0.10f); // đỉnh trên

    // ===== 5) Tint đồng phục
    SDL_SetTextureColorMod(texBody, jerseyTint.r, jerseyTint.g, jerseyTint.b);
    SDL_SetTextureColorMod(texLeg,  jerseyTint.r, jerseyTint.g, jerseyTint.b);
    if (texArm) SDL_SetTextureColorMod(texArm, jerseyTint.r, jerseyTint.g, jerseyTint.b);

    // Xác định "bên trước" theo pha bước chạy; khi đứng yên giữ mặc định tay trái trước
    const bool moving = (fabsf(moveX) > 0.1f || fabsf(moveY) > 0.1f);
    const bool rightFront = runCycle > 0.0f;      // chân phải trước khi pha > 0
    const bool leftArmFront = moving ? rightFront : true; // chân phải trước → tay trái trước
    const bool rightArmFrontAlt = !leftArmFront;

    // ===== 6) LỚP VẼ: CHÂN → TAY → BODY =====

    // --- CHÂN (cả hai chân vẽ TRƯỚC tay & body)
    SDL_SetTextureColorMod(texLeg,
        (Uint8)(jerseyTint.r*0.88f),
        (Uint8)(jerseyTint.g*0.88f),
        (Uint8)(jerseyTint.b*0.88f));          // hơi tối cho có chiều sâu
    drawAtPivot(texLeg, hipLx, hipLy, LEG_W, LEG_H, angleDeg, LEG_PIVOT_X, LEG_PIVOT_Y);
    drawAtPivot(texLeg, hipRx, hipRy, LEG_W, LEG_H, angleDeg, LEG_PIVOT_X, LEG_PIVOT_Y);
    SDL_SetTextureColorMod(texLeg, jerseyTint.r, jerseyTint.g, jerseyTint.b);

    // --- TAY (nằm TRÊN chân nhưng DƯỚI body) ---
if (texArm) {
    // Pha đánh tay (ngược pha với chân để chạy tự nhiên hơn)
    float armSwing = sinf(animTime * 6.0f) * 35.0f;

    // Khi chạy: tay trái & tay phải đối xứng
    // Nếu đang đứng yên → mặc định tay trái ra trước
    const bool moving = (fabsf(moveX) > 0.1f || fabsf(moveY) > 0.1f);
    bool leftArmFront = moving ? (armSwing > 0.0f) : true;

    // --- Tay sau (làm tối màu 15%) ---
    SDL_SetTextureColorMod(texArm,
        (Uint8)(jerseyTint.r * 0.85f),
        (Uint8)(jerseyTint.g * 0.85f),
        (Uint8)(jerseyTint.b * 0.85f));

    if (leftArmFront) {
        // Tay phải là tay sau
        drawAtPivot(texArm, shoulderRx, shoulderRy, ARM_W, ARM_H,
                    angleDeg - armSwing + 180.0f,
                    ARM_PIVOT_R_X, ARM_PIVOT_R_Y);
    } else {
        // Tay trái là tay sau
        drawAtPivot(texArm, shoulderLx, shoulderLy, ARM_W, ARM_H,
                    angleDeg + armSwing + 180.0f,
                    ARM_PIVOT_L_X, ARM_PIVOT_L_Y);
    }

    // --- Tay trước (giữ màu gốc) ---
    SDL_SetTextureColorMod(texArm,
        jerseyTint.r, jerseyTint.g, jerseyTint.b);

    if (leftArmFront) {
        // Tay trái là tay trước
        drawAtPivot(texArm, shoulderLx, shoulderLy, ARM_W, ARM_H,
                    angleDeg + armSwing,
                    ARM_PIVOT_L_X, ARM_PIVOT_L_Y);
    } else {
        // Tay phải là tay trước
        drawAtPivot(texArm, shoulderRx, shoulderRy, ARM_W, ARM_H,
                    angleDeg - armSwing,
                    ARM_PIVOT_R_X, ARM_PIVOT_R_Y);
    }
}



    // --- BODY (vẽ CUỐI CÙNG)
    SDL_Rect dstBody{ int(cx - BODY_W*0.5f), int(cy - BODY_H*0.5f), BODY_W, BODY_H };
    SDL_RenderCopyEx(renderer, texBody, nullptr, &dstBody, angleDeg, nullptr, SDL_FLIP_NONE);

    // Viền người active
    if (active){
        SDL_SetRenderDrawColor(renderer, 255,235,80,230);
        SDL_Rect border{ dstBody.x-2, dstBody.y-2, dstBody.w+4, dstBody.h+4 };
        SDL_RenderDrawRect(renderer, &border);
    }

    // reset tint
    SDL_SetTextureColorMod(texBody, 255,255,255);
    SDL_SetTextureColorMod(texLeg,  255,255,255);
    if (texArm) SDL_SetTextureColorMod(texArm, 255,255,255);
}



};




// =====================================
// Scoreboard
// =====================================
struct ScoreBoard {
    int left = 0;
    int right = 0;
};

// =====================================
// Collision helpers
// =====================================
bool rect_intersect(const SDL_Rect& a, const SDL_Rect& b){
    return !(a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y);
}

// Reflect ball off rectangle (player) with simple physics
void reflect_ball_off_player(Ball &ball, const SDL_Rect& p){
    SDL_Rect br = ball.rect();
    // find intersection vector
    float bx = ball.x + ball.size/2.0f;
    float by = ball.y + ball.size/2.0f;
    float px = p.x + p.w/2.0f;
    float py = p.y + p.h/2.0f;

    // Determine where it hit relative to player's center
    float relativeY = (by - p.y) / (float)p.h; // 0..1
    float hitPos = (relativeY - 0.5f) * 2.0f; // -1..1

    // reverse x velocity and add angle based on hitPos
    ball.vx = -ball.vx;
    // nudging speed
    float speed = std::sqrt(ball.vx*ball.vx + ball.vy*ball.vy);
    float angle = hitPos * (75.0f * M_PI / 180.0f); // up to +-75 deg
    float dir = (ball.vx>0)?1:-1; // after flip, direction of x
    ball.vx = std::cos(angle) * speed * ((dir>0)?1:-1);
    ball.vy = std::sin(angle) * speed;

    // small boost
    ball.vx *= 1.05f;
    ball.vy *= 1.05f;
}

// =====================================
// Game
// =====================================
struct Game {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    bool running = true;
    SDL_Texture* bgTex = nullptr;
    SDL_Texture* elementsTex = nullptr;  // texture chứa cầu môn

    Ball ball;
    std::vector<Player> players; // left players first, then right
    ScoreBoard score;

    bool showDebug = false;
    bool aiEnabled = false; // let player 2 be AI

    Game(){ }

    bool init(const char* title="Tiny Football (SDL2)"){
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0){
            printf("SDL_Init Error: %s\n", SDL_GetError());
            return false;
        }
        if(TTF_Init() != 0){
            printf("TTF_Init Error: %s\n", TTF_GetError());
            return false;
        }

        window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
        if(!window){ printf("CreateWindow failed: %s\n", SDL_GetError()); return false; }
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if ((IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG)) == 0) {
        printf("IMG_Init Error: %s\n", IMG_GetError());
         // vẫn chạy tiếp được nếu thiếu decoder, nhưng nên có ảnh PNG/JPG
        }
        if(!renderer){ printf("CreateRenderer failed: %s\n", SDL_GetError()); return false; }

        SDL_Texture* texBall = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Equipment/ball_soccer2.png");
        if(!texBall){
            printf("Error loading ball texture: %s\n", IMG_GetError());
        }
        ball.tex = texBall;
        ball.size = 20;

        bgTex = IMG_LoadTexture(renderer, "../kenney_sports-pack/soccer-field-background-vector.jpg");
        if(!bgTex){
            printf("IMG_LoadTexture Error: %s\n", IMG_GetError());
            return false;
        }

          // Load elements texture (chứa cầu môn)

        elementsTex = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Elements/element (41).png");
        if(!elementsTex){
            printf("Warning: Elements texture not found\n");
        }

        font = TTF_OpenFont("./build/OpenSans-Regular.ttf", 22);
        if(!font){
            // try relative
            font = TTF_OpenFont("./OpenSans-Regular.ttf", 22);
            if(!font) printf("Warning: could not open font, text rendering may fail\n");
        }

        // init players: simple config: left two players (team left), right two players (team right)
        players.clear();
        // left team: two players vertically separated  
        // Team Blue uses WASD + Q controls for whichever player is active
        
        // === BLUE TEAM (3 players) ===
        Player p1(60, SCREEN_H/2 - 120, 21, 31);
        p1.up = SDL_SCANCODE_W; p1.down = SDL_SCANCODE_S; 
        p1.left = SDL_SCANCODE_A; p1.right = SDL_SCANCODE_D; 
        p1.kick = SDL_SCANCODE_Q;
        p1.active = true; p1.isAI = false;
        p1.team = Team::Blue;
        players.push_back(p1);

        Player p2(60, SCREEN_H/2 - 20, 21, 31);
        p2.up = SDL_SCANCODE_W; p2.down = SDL_SCANCODE_S; 
        p2.left = SDL_SCANCODE_A; p2.right = SDL_SCANCODE_D; 
        p2.kick = SDL_SCANCODE_Q;
        p2.active = false; p2.isAI = false;
        p2.team = Team::Blue;
        players.push_back(p2);

        Player p3(60, SCREEN_H/2 + 80, 21, 31);
        p3.up = SDL_SCANCODE_W; p3.down = SDL_SCANCODE_S; 
        p3.left = SDL_SCANCODE_A; p3.right = SDL_SCANCODE_D; 
        p3.kick = SDL_SCANCODE_Q;
        p3.active = false; p3.isAI = false;
        p3.team = Team::Blue;
        players.push_back(p3);

        // === RED TEAM (3 players) ===
        Player p4(829, 171, 21, 31);
        p4.up = SDL_SCANCODE_UP; p4.down = SDL_SCANCODE_DOWN; 
        p4.left = SDL_SCANCODE_LEFT; p4.right = SDL_SCANCODE_RIGHT; 
        p4.kick = SDL_SCANCODE_RETURN;
        p4.active = true; p4.isAI = aiEnabled;
        p4.team = Team::Red;
        players.push_back(p4);

        Player p5(829, 581, 21, 31);
        p5.up = SDL_SCANCODE_UP; p5.down = SDL_SCANCODE_DOWN; 
        p5.left = SDL_SCANCODE_LEFT; p5.right = SDL_SCANCODE_RIGHT; 
        p5.kick = SDL_SCANCODE_RETURN;
        p5.active = false; p5.isAI = false;
        p5.team = Team::Red;
        players.push_back(p5);

        Player p6(1159, 370, 21, 31);
        p6.up = SDL_SCANCODE_UP; p6.down = SDL_SCANCODE_DOWN; 
        p6.left = SDL_SCANCODE_LEFT; p6.right = SDL_SCANCODE_RIGHT; 
        p6.kick = SDL_SCANCODE_RETURN;
        p6.active = false; p6.isAI = false;
        p6.team = Team::Red;
        players.push_back(p6);

        // Blue
        SDL_Texture *bodyBlue = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Blue/characterBlue (1).png");
        SDL_Texture *armBlue  = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Blue/characterBlue (11).png");
        SDL_Texture *legBlue  = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Blue/characterBlue (13).png");

        // Red (nếu pack của bạn có thư mục Red, còn không thì dùng lại Blue)
        SDL_Texture *bodyRed = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Red/characterRed (1).png");
        SDL_Texture *armRed  = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Red/characterRed (11).png");
        SDL_Texture *legRed  = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Red/characterRed (13).png");

        if(!bodyBlue || !armBlue || !legBlue){
            printf("Error loading Blue textures: %s\n", IMG_GetError());
        }
        if(!bodyRed || !armRed || !legRed){
            // fallback: dùng Blue thay Red nếu thiếu
            bodyRed = bodyBlue; armRed = armBlue; legRed = legBlue;
        }

        // Gán textures cho từng player
        for(int i = 0; i < 3; i++){ // Blue team (0,1,2)
            players[i].texBody = bodyBlue;
            players[i].texArm = armBlue;
            players[i].texLeg = legBlue;
        }
        for(int i = 3; i < 6; i++){ // Red team (3,4,5)
            players[i].texBody = bodyRed;
            players[i].texArm = armRed;
            players[i].texLeg = legRed;
        }

        return true;
    }

    void handle_input(){
        SDL_Event e;
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        
        // Handle kick input for each player
        for(auto &p : players){
            if(p.active && !p.isAI && keystate[p.kick]){
                p.kickBall(ball);
            }
        }
        
        while(SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT) running = false;
            else if(e.type == SDL_KEYDOWN){
                if(e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) running = false;
                if(e.key.keysym.scancode == SDL_SCANCODE_F1) showDebug = !showDebug;
                if(e.key.keysym.scancode == SDL_SCANCODE_F2){ 
                    aiEnabled = !aiEnabled; 
                    players[3].isAI = aiEnabled; // player thứ 4 (index 3)
                }
                // Team switching: Q+Tab for Left team, P+Tab for Right team
                if(e.key.keysym.scancode == SDL_SCANCODE_TAB || e.key.keysym.scancode == SDL_SCANCODE_RSHIFT){
                    const Uint8* keystate = SDL_GetKeyboardState(NULL);
                    if(keystate[SDL_SCANCODE_Q]){
                        cycle_left_team();
                    }
                    else if(keystate[SDL_SCANCODE_P]){
                        cycle_right_team();  
                    }
                }
                // Individual player activation (for testing)
                if(e.key.keysym.scancode == SDL_SCANCODE_1){ activate_only(0); }
                if(e.key.keysym.scancode == SDL_SCANCODE_2){ activate_only(1); }
                if(e.key.keysym.scancode == SDL_SCANCODE_3){ activate_only(2); }
                if(e.key.keysym.scancode == SDL_SCANCODE_4){ activate_only(3); }
                if(e.key.keysym.scancode == SDL_SCANCODE_5){ activate_only(4); }
                if(e.key.keysym.scancode == SDL_SCANCODE_6){ activate_only(5); }
                // AI toggle for specific player
                if(e.key.keysym.scancode == SDL_SCANCODE_I){ players[3].isAI = !players[3].isAI; }
            }
        }
    }

    void activate_only(int idx){
        for(size_t i=0;i<players.size();++i) players[i].active = (int)i==idx;
    }
    void cycle_left_team(){
        // Cycle giữa 3 players của Blue team (0,1,2)
        int cur = -1;
        for(int i = 0; i <= 2; i++){
            if(players[i].active){
                cur = i;
                players[i].active = false;
                break;
            }
        }
        int next = (cur + 1) % 3; // 0->1->2->0
        players[next].active = true;
    }
    
    void cycle_right_team(){
        // Cycle giữa 3 players của Red team (3,4,5)
        int cur = -1;
        for(int i = 3; i <= 5; i++){
            if(players[i].active){
                cur = i;
                players[i].active = false;
                break;
            }
        }
        int next = ((cur - 3 + 1) % 3) + 3; // 3->4->5->3
        players[next].active = true;
    }

    void update(float dt){
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        // keyboard update for players
        for(auto &p : players) p.update_from_keyboard(keystate, dt);
        // AI update
        for(auto &p : players) if(p.isAI) p.update_AI(ball, dt);

        // ball physics
        ball.update(dt);

        // collision with top/bottom -> reflect
        if(ball.y <= 0){ ball.y = 0; ball.vy = -ball.vy; }
        if(ball.y + ball.size >= SCREEN_H){ ball.y = SCREEN_H - ball.size; ball.vy = -ball.vy; }

        // collision with left/right -> reflect
        if (ball.x <= 0) {
            ball.x = 0;
            ball.vx = -ball.vx;
        }
        if (ball.x + ball.size >= SCREEN_W) {
            ball.x = SCREEN_W - ball.size;
            ball.vx = -ball.vx;
        }
        // collision with players
        SDL_Rect brect = ball.rect();
        for(auto &p : players){
            if(rect_intersect(brect, p.r)){
                // Better collision resolution: push ball away from player center
                float ballCenterX = ball.x + ball.size/2.0f;
                float ballCenterY = ball.y + ball.size/2.0f;
                float playerCenterX = p.r.x + p.r.w/2.0f;
                float playerCenterY = p.r.y + p.r.h/2.0f;
                
                float dx = ballCenterX - playerCenterX;
                float dy = ballCenterY - playerCenterY;
                float distance = std::sqrt(dx*dx + dy*dy);
                
                if(distance > 0.1f){
                    // Normalize and push ball outside player
                    dx /= distance;
                    dy /= distance;
                    
                    // Calculate minimum separation distance
                    float minDist = (ball.size + std::max(p.r.w, p.r.h))/2.0f + 2.0f;
                    
                    // Position ball outside player
                    ball.x = playerCenterX + dx * minDist - ball.size/2.0f;
                    ball.y = playerCenterY + dy * minDist - ball.size/2.0f;
                }

                reflect_ball_off_player(ball, p.r);
                break;
            }
        }

        float scale = 0.8f;
        int goalWidth  = SCREEN_W * 0.108 * scale;   // giảm cả width
        int goalHeight = SCREEN_H * 0.15 * scale;    // giảm cả height

        int goalCenterY = SCREEN_H / 2;  
        int goalY = goalCenterY - goalHeight / 2;  // cập nhật lại Y

        // --- Ghi bàn bên trái (bóng lọt vào goal trái) ---
        if (ball.x <= 80) { // bóng vượt qua vạch trong
            if (ball.y + ball.size >= goalY && ball.y <= goalY + goalHeight) {
                score.right += 1;     // đội phải ghi bàn
                ball.reset(false);    // giao bóng cho đội trái
            }
        }

        // --- Ghi bàn bên phải (bóng lọt vào goal phải) ---
        if (ball.x + ball.size >= SCREEN_W - 80) { // bóng vượt qua vạch trong
            if (ball.y + ball.size >= goalY && ball.y <= goalY + goalHeight) {
                score.left += 1;      // đội trái ghi bàn
                ball.reset(true);     // giao bóng cho đội phải
            }
        }

        // small friction to avoid runaway velocities
        float maxSpeed = 900.0f;
        float sp = std::sqrt(ball.vx*ball.vx + ball.vy*ball.vy);
        if(sp > maxSpeed){ ball.vx *= maxSpeed/sp; ball.vy *= maxSpeed/sp; }
    }

    // Hàm vẽ cầu môn từ elements.png (dùng toàn bộ ảnh, không cắt sprite)
    void render_goal(int x, int y, int width, int height, bool leftGoal = true){
        if(!elementsTex) return;
        // Không dùng srcRect (NULL = lấy toàn bộ ảnh)
        SDL_Rect dstGoal = {x, y, 60, 100};

        // Nếu muốn lật cho cầu môn bên phải
        SDL_RendererFlip flip = leftGoal ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
        SDL_RenderCopyEx(renderer, elementsTex, NULL, &dstGoal, 0, NULL, flip);
    }

    void render_text(const std::string &txt, int x, int y){
        if(!font) return;
        SDL_Color white = {255,255,255,255};
        SDL_Surface* surf = TTF_RenderText_Solid(font, txt.c_str(), white);
        if(!surf) return;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_Rect dst = {x,y,surf->w,surf->h};
        SDL_FreeSurface(surf);
        if(tex){ SDL_RenderCopy(renderer, tex, NULL, &dst); SDL_DestroyTexture(tex); }
    }

    void render(){
        // background
        SDL_RenderClear(renderer);

        if(bgTex){
            SDL_Rect dst = {0, 0, SCREEN_W, SCREEN_H};
            SDL_RenderCopy(renderer, bgTex, NULL, &dst);
        }
        // mid line
        SDL_SetRenderDrawColor(renderer, 200,200,200,120);
        SDL_Rect mid = {SCREEN_W/2 - 2, 0, 4, SCREEN_H};
        SDL_RenderFillRect(renderer, &mid);

        // draw players with kick range indicator
        for(size_t i=0;i<players.size();++i){
            auto &p = players[i];
            
            // Draw kick range if player is active and can kick
            if(p.active && p.canKickBall(ball)){
                // màu vòng theo đội (alpha 80)
                if(p.team == Team::Blue) SDL_SetRenderDrawColor(renderer, 120,170,255,80);
                else                     SDL_SetRenderDrawColor(renderer, 255,170,60,80);
                int kickX = p.r.x + p.r.w/2 - (int)p.kickRange;
                int kickY = p.r.y + p.r.h/2 - (int)p.kickRange;
                SDL_Rect kickRange = {kickX, kickY, (int)p.kickRange*2, (int)p.kickRange*2};
                SDL_RenderFillRect(renderer, &kickRange);
            }

            
            // Draw player
            p.render(renderer);

            // if(p.active){ // highlight border
            //     SDL_SetRenderDrawColor(renderer, 255,255,0,200);
            //     SDL_Rect border = {p.r.x-2, p.r.y-2, p.r.w+4, p.r.h+4};
            //     SDL_RenderDrawRect(renderer, &border);
            // }
        }

        // ball
        SDL_Rect brect = ball.rect();
        brect.x -= 12;
        brect.y -= 12;
        if(ball.tex){
            SDL_Point center = { brect.w/2, brect.h/2};
            SDL_RenderCopyEx(renderer, ball.tex, NULL, &brect, ball.angle, &center, SDL_FLIP_NONE);
        } else {
            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderFillRect(renderer, &brect);
        }

        float scale = 0.8f;
        int goalWidth  = SCREEN_W * 0.108 * scale;   // giảm cả width
        int goalHeight = SCREEN_H * 0.15 * scale;    // giảm cả height

        int goalCenterY = SCREEN_H / 2;  
        int goalY = goalCenterY - goalHeight / 2;  // cập nhật lại Y
        render_goal(+39, goalY, goalWidth, goalHeight, true);   // cầu môn trái
        render_goal(SCREEN_W - goalWidth +16, goalY, goalWidth, goalHeight, false); // cầu môn phải

        // HUD
        render_text("Tiny Football", 8, 8);
        render_text("Controls: WASD+Q (Blue Team), Arrows+Enter (Orange Team)", 8, 770);
        render_text("Switch Player: Q+Tab (Blue), P+R_Shift (Orange)", 800, 770);
        char scoreText[64]; snprintf(scoreText, sizeof(scoreText), "Score: %d  -  %d", score.left, score.right);
        render_text(scoreText, SCREEN_W/2 - 60, 12);

        if(showDebug){
            char dbg[128];
            snprintf(dbg, sizeof(dbg), "Ball: (%.1f,%.1f) v(%.1f,%.1f)", ball.x, ball.y, ball.vx, ball.vy);
            render_text(dbg, 8, 80);
            snprintf(dbg, sizeof(dbg), "Players active: ");
            render_text(dbg, 8, 104);
            for(size_t i=0;i<players.size();++i){
                char pinfo[64]; snprintf(pinfo, sizeof(pinfo), "P%d: x=%d y=%d AI=%d act=%d kick=%d", (int)i+1, players[i].r.x, players[i].r.y, players[i].isAI?1:0, players[i].active?1:0, players[i].canKickBall(ball)?1:0);
                render_text(pinfo, 8, 124 + (int)i*20);
            }
        }

        SDL_RenderPresent(renderer);
    }

    void cleanup(){
        if(font) TTF_CloseFont(font);
        if(bgTex) SDL_DestroyTexture(bgTex);
        if(elementsTex) SDL_DestroyTexture(elementsTex);
        if(renderer) SDL_DestroyRenderer(renderer);
        if(window) SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        IMG_Quit();
    }
};

int main(int argc, char** argv){
    srand((unsigned int)SDL_GetTicks());
    Game game;
    if(!game.init()) return 1;

    Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    double deltaTime = 0;

    while(game.running){
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency());
        float dt = (float)(deltaTime / 1000.0);

        game.handle_input();
        game.update(dt);
        game.render();

        // cap to ~60fps (optional) - SDL_Renderer with vsync may already cap
        SDL_Delay(1);
    }

    game.cleanup();
    return 0;
}