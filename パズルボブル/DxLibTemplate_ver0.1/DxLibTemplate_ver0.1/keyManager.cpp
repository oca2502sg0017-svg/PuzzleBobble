#include "dxlib.h"
#include <string.h>

static const int KEY_STATE_BUFFER_LENGTH = 256;
static const int KEY_STATE_BUFFER_ROW = 2;
static char keyStateBuffer[KEY_STATE_BUFFER_ROW][KEY_STATE_BUFFER_LENGTH];
static char keyStateBufferTrigger[KEY_STATE_BUFFER_LENGTH];
static int currentBufferIdx = 0;

static const int STR_KEY_STATE_BUFFER_LENGTH = KEY_STATE_BUFFER_LENGTH + 1;
static const int STR_KEY_STATE_BUFFER_ROW = 2;
static char strKeyStateBuffer[STR_KEY_STATE_BUFFER_ROW][STR_KEY_STATE_BUFFER_LENGTH];

// キーマネージャの初期化
void initKeyManager()
{
	for ( int row = 0; row < KEY_STATE_BUFFER_ROW; row++)
	{
		char* curBuffer = keyStateBuffer[row];
		for ( int col = 0; col < KEY_STATE_BUFFER_LENGTH; col++ )
		{
			curBuffer[col] = 0;
		}
	}
	currentBufferIdx = 0;

	for (int row = 0; row < STR_KEY_STATE_BUFFER_ROW; row++)
	{
		char* curBuffer = strKeyStateBuffer[row];
		for (int col = 0; col < STR_KEY_STATE_BUFFER_LENGTH; col++)
		{
			curBuffer[col] = '0';
		}
		curBuffer[STR_KEY_STATE_BUFFER_LENGTH - 1] = '\0';
	}

}

// キーの状態を更新
void updateKeyState()
{
	int nextCurBufferIdx = (currentBufferIdx + 1) % KEY_STATE_BUFFER_ROW;
	char* curBuffer = keyStateBuffer[nextCurBufferIdx];
	char* prevBuffer = keyStateBuffer[currentBufferIdx];

	GetHitKeyStateAll(curBuffer);

	for ( int col = 0; col < KEY_STATE_BUFFER_LENGTH; col++ )
	{
		keyStateBufferTrigger[col] = (prevBuffer[col] == 0) && (curBuffer[col] == 1) ? 1 : 0;
	}

	currentBufferIdx = nextCurBufferIdx;
}

// キーが押されているか調べる
// 戻り値:
//		0: 押されていない
//		1: 押されている
int checkHitKey(int key)
{
	return (int)keyStateBuffer[currentBufferIdx][key];
}

// キーが押された瞬間か調べる
// 戻り値:
//		0: 押された瞬間ではない
//		1: 押さた瞬間
int pushHitKey(int key)
{
	return (int)keyStateBufferTrigger[key];
}

// 全てのキーの押下状態を文字列で取得
// 戻り値:
//		全てのキーの状態を示す文字列
const char* getStrKeyState()
{
	char* orgBuffer = keyStateBuffer[currentBufferIdx];
	char* strBuffer = strKeyStateBuffer[0];
	for (int col = 0; col < KEY_STATE_BUFFER_LENGTH; col++)
	{
		strBuffer[col] = '0' + orgBuffer[col];
	}

	return strBuffer;
}

// 全てのキーの押下瞬間状態を文字列で取得
// 戻り値:
//		全てのキーの押下瞬間状態を示す文字列
const char* getStrKeyStateTrigger()
{
	char* orgBuffer = keyStateBufferTrigger;
	char* strBuffer = strKeyStateBuffer[1];
	for (int col = 0; col < KEY_STATE_BUFFER_LENGTH; col++)
	{
		strBuffer[col] = '0' + orgBuffer[col];
	}

	return strBuffer;
}