#ifndef __CONST_H__
#define __CONST_H__
	
// ゲーム定数
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int BUBBLE_RADIUS = 16;
const int BUBBLE_SIZE = BUBBLE_RADIUS * 2;
const int GRID_ROWS = 12;
const int GRID_COLS = 8; // 最大列数（偶数行は8、奇数行は7）
int OFFSET_X = 160; // 盤面の左端
int OFFSET_Y = 40;  // 盤面の上端
int stage_image = -1;
int stage_floor = -1;
bool stageLoaded = false;

#endif