#ifndef __KEY_MANAGER_H__
#define __KEY_MANAGER_H__

// キーマネージャの初期化
void initKeyManager();

// キーの状態を更新
void updateKeyState();

// キーが押されているか調べる
// 戻り値:
//		0: 押されていない
//		1: 押されている
int checkHitKey(int key);

// キーが押された瞬間か調べる
// 戻り値:
//		0: 押された瞬間ではない
//		1: 押さた瞬間
int pushHitKey(int key);

// 全てのキーの押下状態を文字列で取得
// 戻り値:
//		全てのキーの状態を示す文字列
const char* getStrKeyState();

// 全てのキーの押下瞬間状態を文字列で取得
// 戻り値:
//		全てのキーの押下瞬間状態を示す文字列
const char* getStrKeyStateTrigger();

#endif