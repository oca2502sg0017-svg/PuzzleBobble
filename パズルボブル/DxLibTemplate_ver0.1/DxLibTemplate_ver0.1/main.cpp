#include "DxLib.h"
#include <math.h>
#include <vector>
#include <queue>
#include <string>
#include "keyManager.h"
#include "const.h"


// バブルの色
enum BubbleColor {
    NONE = 0,
    RED,
    BLUE,
    GREEN,
    YELLOW,
    PURPLE,
    COLOR_COUNT
};

unsigned int ColorMap[] = {
    0x000000, // NONE
    0xFF0000, // RED
    0x0000FF, // BLUE
    0x00FF00, // GREEN
    0xFFFF00, // YELLOW
    0xFF00FF  // PURPLE
};

struct Bubble {
    bool active;
    BubbleColor color;
};

Bubble grid[GRID_ROWS][GRID_COLS];

struct Bullet {
    float x, y;
    float vx, vy;
    BubbleColor color;
    bool moving;
};

Bullet bullet;
BubbleColor nextColor;
float playerAngle = -DX_PI_F / 2.0f; // 真上向き
int score = 0;
// ゲームオーバーフラグ
bool gameOver = false;
// ゲームクリアフラグ
bool gameClear = false;
// デバッグ: 一回当たると全消し
bool debugClearOnHit = false;

// スコアポップアップ
struct ScorePopup {
    float x, y;
    int value;
    int life; // フレーム数
    float vy;
};
std::vector<ScorePopup> scorePopups;

// グリッド座標から描画座標(中心)を取得
void GetBubblePos(int r, int c, float* x, float* y) {
    *x = (float)(OFFSET_X + c * BUBBLE_SIZE + BUBBLE_RADIUS);
    if (r % 2 == 1) {
        *x += BUBBLE_RADIUS; // 奇数行は右にずらす
    }
    *y = (float)(OFFSET_Y + r * (BUBBLE_SIZE - 4) + BUBBLE_RADIUS); // 少し詰める
}

// 指定行の列数（偶数行はGRID_COLS、奇数行はGRID_COLS-1）
int GetCols(int r) {
    return (r % 2 == 0) ? GRID_COLS : (GRID_COLS - 1);
}

// 底の行にバブルが存在するかをチェック
bool IsGameOver() {
    int bottom = GRID_ROWS - 1;
    for (int j = 0; j < GetCols(bottom); j++) {
        if (grid[bottom][j].color != NONE) return true;
    }
    return false;
}

// 全バブルが消えたかチェック
bool IsCleared() {
    for (int i = 0; i < GRID_ROWS; i++) {
        for (int j = 0; j < GetCols(i); j++) {
            if (grid[i][j].color != NONE) return false;
        }
    }
    return true;
}

// 描画座標から最も近いグリッドインデックスを取得
void GetNearestGrid(float x, float y, int* r, int* c) {
    float minDist = 100000.0f;
    for (int i = 0; i < GRID_ROWS; i++) {
        for (int j = 0; j < GetCols(i); j++) {
            float gx, gy;
            GetBubblePos(i, j, &gx, &gy);
            float dist = (x - gx) * (x - gx) + (y - gy) * (y - gy);
            if (dist < minDist) {
                minDist = dist;
                *r = i;
                *c = j;
            }
        }
    }
}

struct Point { int r, c; };

//ボールの位置
std::vector<Point> GetNeighbors(int r, int c) {
    std::vector<Point> neighbors;
    int dr[] = { -1, -1, 0, 0, 1, 1 };
    int dc_even[] = { -1, 0, -1, 1, -1, 0 }; // 偶数行の時の列移動
    int dc_odd[] = { 0, 1, -1, 1, 0, 1 };  // 奇数行の時の列移動

    for (int i = 0; i < 6; i++) {
        int nr = r + dr[i];
        int nc = c + (r % 2 == 0 ? dc_even[i] : dc_odd[i]);
        if (nr >= 0 && nr < GRID_ROWS && nc >= 0 && nc < GetCols(nr)) {
            neighbors.push_back({ nr, nc });
        }
    }
    return neighbors;
}
// ボールの確認＆削除
void CheckAndRemove(int r, int c) {
    if (grid[r][c].color == NONE) return;

    BubbleColor targetColor = grid[r][c].color;
    std::vector<Point> sameGroup;
    std::queue<Point> q;
    bool visited[GRID_ROWS][GRID_COLS] = { false };

    q.push({ r, c });
    visited[r][c] = true;

    while (!q.empty()) {
        Point curr = q.front(); q.pop();
        sameGroup.push_back(curr);

        for (auto n : GetNeighbors(curr.r, curr.c)) {
            if (!visited[n.r][n.c] && grid[n.r][n.c].color == targetColor) {
                visited[n.r][n.c] = true;
                q.push(n);
            }
        }
    }

    // 3つ以上なら消す
    if (sameGroup.size() >= 3) {
        // 同色グループを削除してスコア加算
        for (auto p : sameGroup) {
            // ポップアップを作成
            float px, py; GetBubblePos(p.r, p.c, &px, &py);
            scorePopups.push_back({ px, py, 30, 60, -0.8f });
            grid[p.r][p.c].color = NONE;
            score += 30;
        }

        // 浮いているバブルを落とす
        bool connectedToCeiling[GRID_ROWS][GRID_COLS] = { false };
        std::queue<Point> cq;

        // 天井に接しているバブルを起点に探索
        for (int j = 0; j < GetCols(0); j++) {
            if (grid[0][j].color != NONE) {
                cq.push({ 0, j });
                connectedToCeiling[0][j] = true;
            }
        }

        while (!cq.empty()) {
            Point curr = cq.front(); cq.pop();
            for (auto n : GetNeighbors(curr.r, curr.c)) {
                if (!connectedToCeiling[n.r][n.c] && grid[n.r][n.c].color != NONE) {
                    connectedToCeiling[n.r][n.c] = true;
                    cq.push(n);
                }
            }
        }

        // 天井と繋がっていないものを削除（未削除のものだけをスコアに加算）
        for (int i = 0; i < GRID_ROWS; i++) {
            for (int j = 0; j < GetCols(i); j++) {
                if (!connectedToCeiling[i][j] && grid[i][j].color != NONE) {
                    // ポップアップを作成 (落とした奴は20点)
                    float px, py; GetBubblePos(i, j, &px, &py);
                    scorePopups.push_back({ px, py, 20, 60, -0.8f });
                    grid[i][j].color = NONE;
                    score += 20;
                }
            }
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    ChangeWindowMode(TRUE);
    SetGraphMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32);
    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);

    // ==================== 初期化 ====================
    if (stage_image == -1) {
        stage_image = LoadGraph("stage_1.png");
		stage_floor = LoadGraph("scaffold_1.png");
    }
    if (stage_image != -1) {
        stageLoaded = true;
        // 画像サイズを取得して、中央の四角にグリッドが収まるようOFFSETを計算する
        int imgW = 0, imgH = 0;
        GetGraphSize(stage_image, &imgW, &imgH);
        // グリッド表示に必要な幅・高さ（ピクセル）を計算
        int gridPixelWidth = GRID_COLS * BUBBLE_SIZE; // 横幅（おおよそ）
        int gridPixelHeight = (GRID_ROWS - 1) * (BUBBLE_SIZE - 4) + 2 * BUBBLE_RADIUS; // 縦幅
        // 画像中央に配置（横方向は中央に合わせる）
        OFFSET_X = (imgW - gridPixelWidth) / 2;
        // 縦方向は中央ではなく天井（上側）に合わせて少し上寄せする
        // 値を調整すれば上寄せ量を変更できます
        const int TOP_SHIFT = 20; // 上方向へのシフト量（ピクセル）
        OFFSET_Y = (imgH - gridPixelHeight) / 2 - TOP_SHIFT;
        if (OFFSET_Y < 0) OFFSET_Y = 0;
        // 画面全体に描画する場合（画面サイズ != 画像サイズ）でも、画面の中心に合わせたいなら次を使う
        // OFFSET_X = (SCREEN_WIDTH - gridPixelWidth) / 2;
        // OFFSET_Y = (SCREEN_HEIGHT - gridPixelHeight) / 2;
    }

 

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < GetCols(i); j++) {
            grid[i][j].color = (BubbleColor)(GetRand(COLOR_COUNT - 2) + 1);
        }
    }

    bullet.moving = false;
    bullet.color = (BubbleColor)(GetRand(COLOR_COUNT - 2) + 1);
    nextColor = (BubbleColor)(GetRand(COLOR_COUNT - 2) + 1);
  

    while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0) {
        ClearDrawScreen();

        // ==================== 更新 (Update) ====================
       
        if (!gameOver && !gameClear) {
            if (!bullet.moving) {
                if (CheckHitKey(KEY_INPUT_LEFT)) playerAngle -= 0.05f;
                if (CheckHitKey(KEY_INPUT_RIGHT)) playerAngle += 0.05f;
                if (playerAngle < -DX_PI_F + 0.4f) playerAngle = -DX_PI_F + 0.4f;
                if (playerAngle > -0.4f) playerAngle = -0.4f;

                if (CheckHitKey(KEY_INPUT_SPACE)) {
                    bullet.x = SCREEN_WIDTH / 2;
                    bullet.y = SCREEN_HEIGHT - 40;

                    bullet.vx = cos(playerAngle) * 10.0f;
                    bullet.vy = sin(playerAngle) * 10.0f;
                    bullet.moving = true;
                }
            }
            else {
                bullet.x += bullet.vx;
                bullet.y += bullet.vy;

                // 壁反射
                if (bullet.x < OFFSET_X + BUBBLE_RADIUS || bullet.x > OFFSET_X + GRID_COLS * BUBBLE_SIZE) {
                    bullet.vx *= -1;
                }

                // 衝突判定
                bool hit = false;
                int tr, tc;
                if (bullet.y < OFFSET_Y + BUBBLE_RADIUS) {
                    hit = true; // 天井衝突
                }
                else {
                    for (int i = 0; i < GRID_ROWS; i++) {
                        for (int j = 0; j < GetCols(i); j++) {
                            if (grid[i][j].color != NONE) {
                                float gx, gy;
                                GetBubblePos(i, j, &gx, &gy);
                                float dist = sqrtf((bullet.x - gx) * (bullet.x - gx) + (bullet.y - gy) * (bullet.y - gy));
                                if (dist < BUBBLE_SIZE - 4) { hit = true; break; }
                            }
                        }
                        if (hit) break;
                    }
                }

                if (hit) {
                    if (debugClearOnHit) {
                        // デバッグ用: 全バブルを消す
                        for (int i = 0; i < GRID_ROWS; i++) {
                            for (int j = 0; j < GetCols(i); j++) {
                                grid[i][j].color = NONE;
                            }
                        }
                        gameClear = true;
                        bullet.moving = false;
                        // 次の弾の色を更新
                        bullet.color = nextColor;
                        nextColor = (BubbleColor)(GetRand(COLOR_COUNT - 2) + 1);
                    }
                    else {
                        GetNearestGrid(bullet.x, bullet.y, &tr, &tc);
                        grid[tr][tc].color = bullet.color;
                        CheckAndRemove(tr, tc);

                        // 全消しチェック
                        if (IsCleared()) {
                            gameClear = true;
                        }

                        bullet.moving = false;

                        // 底に到達していたらゲームオーバー
                        if (IsGameOver()) {
                            gameOver = true;
                        }

                        bullet.color = nextColor;
                        nextColor = (BubbleColor)(GetRand(COLOR_COUNT - 2) + 1);
                    }
                }
            }
        }
        

        // ==================== 描画 (Render) ====================
        //背景
        if (stageLoaded) {
            DrawGraph(0, 0, stage_image, TRUE);
			DrawGraph(0, 472, stage_floor, TRUE);
        }

       

        // スコア表示
        char scoreBuf[64];
        sprintf_s(scoreBuf, sizeof(scoreBuf), "SCORE: %d", score);
        DrawString(10, 70, scoreBuf, GetColor(255, 255, 255));

        // ゲーム終了表示
        if (gameOver) {
            DrawString(SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT/2, "GAME OVER", GetColor(255, 0, 0));
        }
        else if (gameClear) {
            DrawString(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2, "GAME CLEAR", GetColor(0, 255, 0));
        }
        else {
            // グリッド描画
            for (int i = 0; i < GRID_ROWS; i++) {
                for (int j = 0; j < GetCols(i); j++) {
                    if (grid[i][j].color != NONE) {
                        float x, y;
                        GetBubblePos(i, j, &x, &y);
                        DrawCircle((int)x, (int)y, BUBBLE_RADIUS - 1, ColorMap[grid[i][j].color], TRUE);
                        DrawCircle((int)x, (int)y, BUBBLE_RADIUS - 1, GetColor(255, 255, 255), FALSE);
                    }
                }
            }
        }

        // 矢印（発射方向）とバレット描画（ゲームクリア時は非表示）
        if (!gameClear) {
            if (!bullet.moving) {
                int lineLen = 60;
                DrawLine(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 40,
                    SCREEN_WIDTH / 2 + (int)(cos(playerAngle) * lineLen),
                    SCREEN_HEIGHT - 40 + (int)(sin(playerAngle) * lineLen), GetColor(200, 200, 200));
            }

            // バレット描画
            if (bullet.moving) {
                DrawCircle((int)bullet.x, (int)bullet.y, BUBBLE_RADIUS - 1, ColorMap[bullet.color], TRUE);
            }
            else {
                DrawCircle(SCREEN_WIDTH / 2, SCREEN_HEIGHT - 40, BUBBLE_RADIUS - 1, ColorMap[bullet.color], TRUE);
                DrawCircle(SCREEN_WIDTH / 2 + 50, SCREEN_HEIGHT - 40, BUBBLE_RADIUS / 2, ColorMap[nextColor], TRUE);
                DrawString(SCREEN_WIDTH / 2 + 70, SCREEN_HEIGHT - 45, "NEXT", GetColor(255, 255, 255));
            }
        }
        // スコアポップアップ更新・描画
        for (int idx = (int)scorePopups.size() - 1; idx >= 0; --idx) {
            ScorePopup &sp = scorePopups[idx];
            sp.life -= 1;
            sp.y += sp.vy;
            // 落とした奴は青、消したグループは黄色で表示
            int color = (sp.value == 20) ? GetColor(0, 0, 255) : GetColor(255, 255, 0);
            DrawString((int)sp.x, (int)sp.y, std::to_string(sp.value).c_str(), color);
            if (sp.life <= 0) {
                scorePopups.erase(scorePopups.begin() + idx);
            }
        }

        //DrawString(10, 10, "Left/Right: Aim  Space: Shoot", GetColor(255, 255, 255));

        ScreenFlip();
    }

    DxLib_End();
    DeleteGraph(stage_image);
	DeleteGraph(stage_floor);
    return 0;
}