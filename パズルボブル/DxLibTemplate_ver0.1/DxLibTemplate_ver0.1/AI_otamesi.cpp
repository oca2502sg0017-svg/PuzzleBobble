#include "DxLib.h"
#include <math.h>
#include <vector>
using namespace std;
// --- 定数定義 ---
const int SCREEN_W = 640;
const int SCREEN_H = 480;
const int COLS = 8;        // 列数
const int ROWS = 15;       // 行数
const float RADIUS = 16.0f; // バブルの半径
const float DIAMETER = RADIUS * 2;
const float OFFSET_X = RADIUS;
const float OFFSET_Y = RADIUS * 1.732f; // √3 (六角形に並べるための高さのズレ)
const float FIELD_LEFT = 180.0f; // フィールドの左端位置
// フィールド (0は空、1〜4は色のID)
int field[ROWS][COLS];
// 色を保存する配列
unsigned int colors[5];
// --- プレイヤー状態 ---
float angle = 3.14159f / 2.0f; // 発射角度 (真上が90度 = PI/2)
float shooterX = SCREEN_W / 2.0f;
float shooterY = SCREEN_H - 40.0f;
bool isShooting = false;
float bulletX = 0, bulletY = 0;
float bulletVx = 0, bulletVy = 0;
int bulletColor = 0;
int nextColor = 0;
// --- 消去判定用 ---
vector<pair<int, int>> matchList;
bool visited[ROWS][COLS];
// 6方向チェック用 (偶数行と奇数行で繋がる先が少し違う)
int dr[2][6] = { {-1, -1, 0, 0, 1, 1}, {-1, -1, 0, 0, 1, 1} };
int dc[2][6] = { {-1, 0, -1, 1, -1, 0}, {0, 1, -1, 1, 0, 1} };

// --- 関数 ---
// バブルの実際の画面座標 (X, Y) を計算する
void GetBubblePos(int r, int c, float& px, float& py) {
    px = FIELD_LEFT + c * DIAMETER + (r % 2 == 1 ? OFFSET_X : 0);
    py = RADIUS + r * OFFSET_Y;
}
// 繋がっている同じ色のバブルを数える（再帰関数）
void GetMatches(int r, int c, int targetColor) {
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS) return; // 画面外
    if (visited[r][c] || field[r][c] != targetColor) return; // 既に見た、または違う色
    visited[r][c] = true;
    matchList.push_back({ r, c });
    int odd = r % 2;
    for (int i = 0; i < 6; i++) {
        GetMatches(r + dr[odd][i], c + dc[odd][i], targetColor);
    }
}
// マッチしたバブルが3つ以上なら消す
void CheckAndClearMatches(int r, int c) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) visited[i][j] = false;
    }
    matchList.clear();
    GetMatches(r, c, field[r][c]);
    if (matchList.size() >= 3) {
        for (auto p : matchList) {
            field[p.first][p.second] = 0; // 色を0(空)にして消す
        }
    }
}
// 当たったバブルを一番近いマス目にピタッとはめる
void SnapBubble(float x, float y, int color) {
    int bestRow = -1, bestCol = -1;
    float minDist = 999999.0f;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (field[r][c] != 0) continue; // 既にバブルがある場所は無視
            float px, py;
            GetBubblePos(r, c, px, py);
            float dist = (px - x) * (px - x) + (py - y) * (py - y);
            if (dist < minDist) {
                minDist = dist;
                bestRow = r;
                bestCol = c;
            }
        }
    }
    if (bestRow != -1 && bestCol != -1) {
        field[bestRow][bestCol] = color;
        CheckAndClearMatches(bestRow, bestCol); // 消えるかチェック
    }
}
// 他のバブルや天井にぶつかったかチェック
bool CheckCollision(float x, float y) {
    if (y <= RADIUS) return true; // 天井にドスン
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (field[r][c] != 0) {
                float px, py;
                GetBubblePos(r, c, px, py);
                float dist = (px - x) * (px - x) + (py - y) * (py - y);
                // 半径の2倍より近づいたら当たったとみなす
                if (dist <= DIAMETER * DIAMETER * 0.85f) return true;
            }
        }
    }
    return false;
}
// ゲーム開始時の準備
void InitGame() {
    colors[0] = GetColor(50, 50, 50);   // 空の背景色
    colors[1] = GetColor(255, 50, 50);  // 赤
    colors[2] = GetColor(50, 150, 255); // 水色
    colors[3] = GetColor(50, 255, 50);  // 緑
    colors[4] = GetColor(255, 255, 50); // 黄
    // フィールドを空にする
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) field[r][c] = 0;
    }
    // 最初から上の方に適当にバブルを配置する
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < COLS; c++) {
            if (r % 2 == 1 && c == COLS - 1) continue; // 奇数行の右端ははみ出さないように
            field[r][c] = GetRand(3) + 1;
        }
    }
    nextColor = GetRand(3) + 1;
}
// --- メイン関数 ---
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ChangeWindowMode(TRUE); // ウィンドウモードで起動
    SetGraphMode(SCREEN_W, SCREEN_H, 32);
    if (DxLib_Init() == -1) return -1;
    SetDrawScreen(DX_SCREEN_BACK);
    InitGame();
    while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0) {
        ClearDrawScreen();
        // -------------------------
        // 1. 計算・移動処理
        // -------------------------
        if (!isShooting) {
            // 角度調整
            if (CheckHitKey(KEY_INPUT_LEFT)) angle += 0.05f;
            if (CheckHitKey(KEY_INPUT_RIGHT)) angle -= 0.05f;
            // 撃ちすぎないように角度を制限
            if (angle > 2.9f) angle = 2.9f;
            if (angle < 0.2f) angle = 0.2f;
            // バブル発射！
            if (CheckHitKey(KEY_INPUT_SPACE)) {
                isShooting = true;
                bulletColor = nextColor;
                nextColor = GetRand(3) + 1;
                bulletX = shooterX;
                bulletY = shooterY;
                float speed = 12.0f; // 弾のスピード
                bulletVx = cos(angle) * speed;
                bulletVy = -sin(angle) * speed;
            }
        }
        else {
            // 弾を動かす
            bulletX += bulletVx;
            bulletY += bulletVy;
            // 左右の壁で反射させる
            float leftWall = FIELD_LEFT - RADIUS;
            float rightWall = FIELD_LEFT + COLS * DIAMETER + RADIUS;
            if (bulletX <= leftWall) {
                bulletX = leftWall;
                bulletVx = -bulletVx;
            }
            if (bulletX >= rightWall) {
                bulletX = rightWall;
                bulletVx = -bulletVx;
            }
            // 当たり判定
            if (CheckCollision(bulletX, bulletY)) {
                SnapBubble(bulletX, bulletY, bulletColor);
                isShooting = false; // 発射終了、次の弾へ
            }
            // 画面下まで落ちたら（ミスしたとき）
            if (bulletY > SCREEN_H) {
                isShooting = false;
            }
        }
        // -------------------------
        // 2. 描画処理
        // -------------------------
        // 背景の暗い枠
        DrawBox(FIELD_LEFT - RADIUS * 2, 0, FIELD_LEFT + COLS * DIAMETER + RADIUS * 2, SCREEN_H, GetColor(30, 30, 40), TRUE);
        // フィールド上のバブルを描く
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                if (field[r][c] != 0) {
                    float px, py;
                    GetBubblePos(r, c, px, py);
                    DrawCircle((int)px, (int)py, (int)RADIUS, colors[field[r][c]], TRUE);
                    DrawCircle((int)px, (int)py, (int)RADIUS, GetColor(255, 255, 255), FALSE); // 白いフチ
                }
            }
        }
        // 発射台と大砲の筒（線）
        DrawCircle((int)shooterX, (int)shooterY, 20, GetColor(100, 100, 100), TRUE);
        DrawLine((int)shooterX, (int)shooterY,
            (int)(shooterX + cos(angle) * 40),
            (int)(shooterY - sin(angle) * 40), GetColor(255, 255, 255), 4);
        // 操作説明とNEXTバブル
        DrawString(20, 20, "NEXT", GetColor(255, 255, 255));
        DrawCircle(40, 50, (int)RADIUS, colors[nextColor], TRUE);
        DrawString(20, 100, "[SPACE] : 発射", GetColor(255, 255, 255));
        DrawString(20, 120, "[←][→] : 角度調整", GetColor(255, 255, 255));
        // プレイヤーの弾を描く
        if (!isShooting) {
            DrawCircle((int)shooterX, (int)shooterY, (int)RADIUS, colors[nextColor], TRUE);
        }
        else {
            DrawCircle((int)bulletX, (int)bulletY, (int)RADIUS, colors[bulletColor], TRUE);
        }
        ScreenFlip();
    }
    DxLib_End();
    return 0;
}
