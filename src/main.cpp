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

// Screen
constexpr int SCREEN_W = 1300;
constexpr int SCREEN_H = 850;

// Forward
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
    float angle = 0.0f;        // góc xoay hiện tại
    float spinSpeed = 0.0f;    // tốc độ xoay

    Ball(int sx=SCREEN_W/2, int sy=SCREEN_H/2, int s=12){
        x = sx; y = sy; size = s; vx = 300.0f*(rand()%2?1:-1); vy = 120.0f*((rand()%100)/100.0f - 0.5f);
    }

    SDL_Rect rect() const { return {(int)std::round(x), (int)std::round(y), size, size}; }

    void reset(bool towardsLeft){
        x = SCREEN_W/2 - size/2;
        y = SCREEN_H/2 - size/2;
        vx = (towardsLeft? -1:1) * 280.0f;
        vy = 80.0f * ((rand()%100)/100.0f - 0.5f);
    }

    void update(float dt){
        x += vx * dt;
        y += vy * dt;

        // Áp dụng ma sát: giảm dần vận tốc
        float friction = 0.98f; // hệ số (mỗi giây)
        float factor = powf(friction, dt * 60.0f); // scale theo framerate ~60fps
        vx *= factor;
        vy *= factor;

        // Nếu vận tốc quá nhỏ thì dừng hẳn
        if (fabs(vx) < 10.0f) vx = 0;  // Tăng threshold để bóng không dừng quá sớm
        if (fabs(vy) < 10.0f) vy = 0;

        // --- Tính tốc độ xoay dựa trên tốc độ di chuyển ---
        float speed = std::sqrt(vx*vx + vy*vy);

        // Quy ước: 1 pixel dịch chuyển = 0.5 độ xoay
        spinSpeed = speed * 1.0f;

        // Áp dụng ma sát cho xoay (giảm dần khi bóng chậm lại)
        spinSpeed *= factor;

        // Cập nhật góc xoay
        angle += spinSpeed * dt;
        if(angle > 360.0f) angle -= 360.0f;
    }

    // Kick ball from a player position with given force
    void kick(float fromX, float fromY, float force = 400.0f){
        float ballCenterX = x + size/2.0f;
        float ballCenterY = y + size/2.0f;
        
        // Calculate direction from player to ball
        float dx = ballCenterX - fromX;
        float dy = ballCenterY - fromY;
        float dist = std::sqrt(dx*dx + dy*dy);
        
        if(dist > 0.1f){
            // Normalize direction and apply force
            dx /= dist;
            dy /= dist;
            
            // Add kick velocity
            vx += dx * force;
            vy += dy * force;
        }
    }
};

// =====================================
// Player (composed of body + arm + leg)
// =====================================
struct Player {
    SDL_Rect r; // position & size
    float speed = 260.0f;
    // key mapping
    SDL_Scancode up, down, left, right, kick;
    bool active = true; // whether it's controllable
    bool isAI = false;
    float kickRange = 40.0f; // Distance within which player can kick ball

    SDL_Texture* texBody = nullptr;
    SDL_Texture* texArm  = nullptr;
    SDL_Texture* texLeg  = nullptr;

    // animation state
    float animTime = 0.0f;   // tăng dần theo thời gian
    float moveX = 0, moveY = 0; // lưu hướng di chuyển

    Player(int x=0,int y=0,int w=24,int h=64){
        r.x=x; r.y=y; r.w=w; r.h=h;
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

        // clamp inside screen
        r.x = (int)clampf(r.x, 0, SCREEN_W - r.w);
        r.y = (int)clampf(r.y, 0, SCREEN_H - r.h);

        // cập nhật thời gian animation khi có di chuyển
        if(dx != 0 || dy != 0) {
            animTime += dt;
        } else {
            animTime = 0; // reset nếu đứng yên
        }
    }

    void update_AI(const Ball& b, float dt){
        if(!isAI) return;
        // simple AI: follow ball y
        float targetY = b.y + b.size/2 - r.h/2;
        float dy = targetY - r.y;
        moveX = 0; moveY = 0;
        if(std::abs(dy) > 6){
            float dir = (dy>0)?1:-1;
            r.y += (int)std::round(dir * speed * dt * 0.8f); // AI slower
            moveY = dir;
        }
        // clamp
        r.y = (int)clampf(r.y, 0, SCREEN_H - r.h);

        if(moveX != 0 || moveY != 0) animTime += dt; else animTime = 0;
    }

    // Check if player can kick the ball
    bool canKickBall(const Ball& ball) const {
        float playerCenterX = r.x + r.w/2.0f;
        float playerCenterY = r.y + r.h/2.0f;
        float ballCenterX = ball.x + ball.size/2.0f;
        float ballCenterY = ball.y + ball.size/2.0f;
        
        float dist = std::sqrt(
            (playerCenterX - ballCenterX) * (playerCenterX - ballCenterX) + 
            (playerCenterY - ballCenterY) * (playerCenterY - ballCenterY)
        );
        
        return dist <= kickRange;
    }

    void kickBall(Ball& ball) const {
        if(canKickBall(ball)){
            float playerCenterX = r.x + r.w/2.0f;
            float playerCenterY = r.y + r.h/2.0f;
            ball.kick(playerCenterX, playerCenterY, 450.0f);
        }
    }

    // Render player với animation
    // void render(SDL_Renderer* renderer){
    //     if(!texBody){
    //         SDL_SetRenderDrawColor(renderer, 0,200,255,255);
    //         SDL_RenderFillRect(renderer, &r);
    //         return;
    //     }

    //     double swing = sin(animTime * 10.0) * 60.0; // góc vung

    //     // --- Legs ---
    //     if(texLeg){
    //         // chân trái
    //         SDL_Rect dstLegR = {r.x + 15, r.y, 19, 13};
    //         SDL_Point pivotLegR = {dstLegR.w/2, 0};
    //         SDL_RenderCopyEx(renderer, texLeg, NULL, &dstLegR, -swing, &pivotLegR, SDL_FLIP_NONE);

    //         // chân phải
    //         SDL_Rect dstLegL = {r.x-12, r.y +15, 19, 13};
    //         SDL_Point pivotLegL = {dstLegL.w/2, 0};
    //         SDL_RenderCopyEx(renderer, texLeg, NULL, &dstLegL, swing, &pivotLegL, SDL_FLIP_HORIZONTAL);

            
    //     }

    //     // --- Body ---
    //     SDL_Rect dstBody = {r.x, r.y, r.w, r.h};
    //     SDL_RenderCopy(renderer, texBody, NULL, &dstBody);

    //     // --- Arms ---
    //     // if(texArm){
    //     //     // tay trái
    //     //     SDL_Rect dstArmL = {r.x - r.w/4, r.y, 19, 13};
    //     //     SDL_Point pivotArmL = {dstArmL.w/2, dstArmL.h/4};
    //     //     SDL_RenderCopyEx(renderer, texArm, NULL, &dstArmL, swing, &pivotArmL, SDL_FLIP_NONE);

    //     //     // tay phải
    //     //     SDL_Rect dstArmR = {r.x + r.w - r.w/4, r.y, 19, 13};
    //     //     SDL_Point pivotArmR = {dstArmR.w/2, dstArmR.h/4};
    //     //     SDL_RenderCopyEx(renderer, texArm, NULL, &dstArmR, -swing, &pivotArmR, SDL_FLIP_NONE);
    //     // }
    // }

void render(SDL_Renderer* renderer){
    if(!texBody){
        SDL_SetRenderDrawColor(renderer, 0,200,255,255);
        SDL_RenderFillRect(renderer, &r);
        return;
    }

    // --- Chu kỳ bước chạy ---
    float runCycle = sin(animTime * 12.0f); // tần số cao hơn cho nhịp chạy nhanh
    float stepLength = 12.0f;               // độ dài bước

    // --- Offset chân theo hướng di chuyển ---
    float rightLegOffsetX = runCycle * stepLength * moveX;
    float rightLegOffsetY = runCycle * stepLength * moveY;
    float leftLegOffsetX  = -runCycle * stepLength * moveX;
    float leftLegOffsetY  = -runCycle * stepLength * moveY;

    // Nếu đứng yên, cho chạy tại chỗ hướng xuống
    if(fabs(moveX) < 0.1f && fabs(moveY) < 0.1f){
        rightLegOffsetX = 0; rightLegOffsetY = runCycle * stepLength * 0.5f;
        leftLegOffsetX  = 0; leftLegOffsetY  = -runCycle * stepLength * 0.5f;
    }

    // --- Vị trí gốc của chân ---
    float baseLegX = r.x + r.w / 2.0f;
    float baseLegY = r.y + r.h - 5;

    // --- Tính góc xoay của chân dựa trên hướng di chuyển ---
    float angle = atan2(moveY, moveX) * 180.0f / M_PI; // độ
    if(fabs(moveX) < 0.1f && fabs(moveY) < 0.1f) angle = 90.0f; // mặc định nhìn xuống

    // --- Chân phải ---
    SDL_Rect dstLegR = {
        (int)(baseLegX + 6 + rightLegOffsetX),
        (int)(baseLegY + rightLegOffsetY),
        19, 13
    };
    SDL_Point pivotR = { dstLegR.w/2, 0 };

    // --- Chân trái ---
    SDL_Rect dstLegL = {
        (int)(baseLegX - 6 + leftLegOffsetX),
        (int)(baseLegY + leftLegOffsetY),
        19, 13
    };
    SDL_Point pivotL = { dstLegL.w/2, 0 };

    // --- Render theo độ sâu ---
    bool rightLegFront = runCycle > 0;
    if(rightLegFront){
        // Chân trái ở sau
        SDL_SetTextureColorMod(texLeg, 180,180,180);
        SDL_RenderCopyEx(renderer, texLeg, NULL, &dstLegL, angle+180, &pivotL, SDL_FLIP_NONE);
        SDL_SetTextureColorMod(texLeg, 255,255,255);
    } else {
        // Chân phải ở sau
        SDL_SetTextureColorMod(texLeg, 180,180,180);
        SDL_RenderCopyEx(renderer, texLeg, NULL, &dstLegR, angle+180, &pivotR, SDL_FLIP_NONE);
        SDL_SetTextureColorMod(texLeg, 255,255,255);
    }

    // --- Chân trước ---
    if(rightLegFront){
        SDL_RenderCopyEx(renderer, texLeg, NULL, &dstLegR, angle, &pivotR, SDL_FLIP_NONE);
    } else {
        SDL_RenderCopyEx(renderer, texLeg, NULL, &dstLegL, angle, &pivotL, SDL_FLIP_NONE);
    }

    // --- Body ---
    SDL_Rect dstBody = { r.x, r.y, 21, 31 };
    SDL_RenderCopyEx(renderer, texBody, NULL, &dstBody, angle, NULL, SDL_FLIP_NONE);

    // --- Arms (nếu cần, có thể cho swing nhẹ như chân) ---
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
        if(!window){ printf("CreateWindow failed: %s\n", SDL_GetError()); return false; }
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

        font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 22);
        if(!font){
            // try relative
            font = TTF_OpenFont("./DejaVuSans.ttf", 22);
            if(!font) printf("Warning: could not open font, text rendering may fail\n");
        }

        // init players: simple config: left two players (team left), right two players (team right)
        players.clear();
        // left team: two players vertically separated  
        // Team Blue uses WASD + Q controls for whichever player is active
        
        Player p1(60, SCREEN_H/2 - 80, 21, 31);
        p1.up = SDL_SCANCODE_W; p1.down = SDL_SCANCODE_S; p1.left = SDL_SCANCODE_A; p1.right = SDL_SCANCODE_D; p1.kick = SDL_SCANCODE_Q;
        p1.active = true; p1.isAI = false;  // P1 starts as active player for left team
        players.push_back(p1);

        Player p2(60, SCREEN_H/2 + 20, 21, 31);
        p2.up = SDL_SCANCODE_W; p2.down = SDL_SCANCODE_S; p2.left = SDL_SCANCODE_A; p2.right = SDL_SCANCODE_D; p2.kick = SDL_SCANCODE_Q;  // Same controls as P1
        p2.active = false; p2.isAI = false;  // P2 starts inactive
        players.push_back(p2);

        // right team: two players
        // Team Orange uses Arrow Keys + RShift controls for whichever player is active
        Player p3(SCREEN_W - 80, SCREEN_H/2 - 80, 21, 31);
        p3.up = SDL_SCANCODE_UP; p3.down = SDL_SCANCODE_DOWN; p3.left = SDL_SCANCODE_LEFT; p3.right = SDL_SCANCODE_RIGHT; p3.kick = SDL_SCANCODE_RSHIFT;
        p3.active = true; p3.isAI = aiEnabled;  // P3 starts as active player for right team
        players.push_back(p3);
        Player p4(SCREEN_W - 80, SCREEN_H/2 + 20, 21, 31);
        p4.up = SDL_SCANCODE_UP; p4.down = SDL_SCANCODE_DOWN; p4.left = SDL_SCANCODE_LEFT; p4.right = SDL_SCANCODE_RIGHT; p4.kick = SDL_SCANCODE_RSHIFT;  // Same controls as P3
        p4.active = false; p4.isAI = false;  // P4 starts inactive
        players.push_back(p4);

        SDL_Texture *texBody = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Blue/characterBlue (1).png");
        SDL_Texture *texArm  = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Blue/characterBlue (11).png");
        SDL_Texture *texLeg  = IMG_LoadTexture(renderer, "../kenney_sports-pack/PNG/Blue/characterBlue (13).png");

        if(!texBody || !texArm || !texLeg){
            printf("Error loading player textures: %s\n", IMG_GetError());
        }
        for(auto &p : players){
            p.texBody = texBody;
            p.texArm  = texArm;
            p.texLeg  = texLeg;
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
                if(e.key.keysym.scancode == SDL_SCANCODE_F2){ aiEnabled = !aiEnabled; players[2].isAI = aiEnabled; }
                // Team switching: Q+Tab for Left team, P+Tab for Right team
                if(e.key.keysym.scancode == SDL_SCANCODE_TAB){
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
                // AI toggle for specific player
                if(e.key.keysym.scancode == SDL_SCANCODE_I){ players[2].isAI = !players[2].isAI; }
            }
        }
    }

    void activate_only(int idx){
        for(size_t i=0;i<players.size();++i) players[i].active = (int)i==idx;
    }
    void cycle_left_team(){
        // Cycle between player 0 and 1 (left team - blue)
        int cur = -1;
        for(int i = 0; i <= 1; i++){
            if(players[i].active){
                cur = i;
                players[i].active = false;
                break;
            }
        }
        int next = (cur == 0) ? 1 : 0;
        players[next].active = true;
        // DON'T deactivate right team - let them play independently
    }
    
    void cycle_right_team(){
        // Cycle between player 2 and 3 (right team - orange)
        int cur = -1;
        for(int i = 2; i <= 3; i++){
            if(players[i].active){
                cur = i;
                players[i].active = false;
                break;
            }
        }
        int next = (cur == 2) ? 3 : 2;
        players[next].active = true;
        // DON'T deactivate left team - let them play independently
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

        // scoring: left goal is x<=0, right goal x+size>=SCREEN_W
        if(ball.x <= -10){ // right team scores
            score.right += 1;
            ball.reset(false); // towards right team? false means to right
        }
        if(ball.x + ball.size >= SCREEN_W + 10){ // left team scores
            score.left += 1;
            ball.reset(true);
        }

        // small friction to avoid runaway velocities
        float maxSpeed = 900.0f;
        float sp = std::sqrt(ball.vx*ball.vx + ball.vy*ball.vy);
        if(sp > maxSpeed){ ball.vx *= maxSpeed/sp; ball.vy *= maxSpeed/sp; }
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
                SDL_SetRenderDrawColor(renderer, 255,255,0,80);
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
        if(ball.tex){
            SDL_Point center = { brect.w/2, brect.h/2 };
            SDL_RenderCopyEx(renderer, ball.tex, NULL, &brect, ball.angle, &center, SDL_FLIP_NONE);
        } else {
            SDL_SetRenderDrawColor(renderer, 255,255,255,255);
            SDL_RenderFillRect(renderer, &brect);
        }

        // draw goals (simple)
        SDL_SetRenderDrawColor(renderer, 70,70,70,255);
        SDL_Rect leftGoal = {0, SCREEN_H/3, 6, SCREEN_H/3};
        SDL_Rect rightGoal = {SCREEN_W-6, SCREEN_H/3, 6, SCREEN_H/3};
        SDL_RenderFillRect(renderer, &leftGoal);
        SDL_RenderFillRect(renderer, &rightGoal);

        // HUD
        render_text("Tiny Football - F1 debug, F2 toggle AI", 8, 8);
        render_text("Controls: WASD+Q (Blue Team), Arrows+RShift (Orange Team)", 8, 32);
        render_text("Switch Player: Q+Tab (Blue), P+Tab (Orange)", 8, 56);
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
        if(renderer) SDL_DestroyRenderer(renderer);
        if(window) SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
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