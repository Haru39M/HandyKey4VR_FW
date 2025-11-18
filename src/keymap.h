#pragma once

// -- キーマトリクスの定義 --
#define ROWS 3 // 行数
#define COLS 4 // 列数
#define NUM_KEYS (ROWS * COLS)

#define TS 0//TOUCH SENSOR
#define KS 1//KEY SWITCH

// -- キーマップの定義 --
// 右手3段 (uiop, jkl;, m,./)
// BleKeyboard.press() / release() はASCII文字をそのままキーコードとして扱えます。
#ifdef RIGHT_HAND
const char keymap[NUM_KEYS] = {
    'j', 'k', 'l', ';', // 論理的な2行目 (インデックス 4～7)
    'u', 'i', 'o', 'p', // 論理的な1行目 (インデックス 0～3)
    'm', ',', '.', '/'  // 論理的な3行目 (インデックス 8～11)
};
const int keyType[NUM_KEYS] = {
    TS,TS,TS,TS,
    KS,KS,KS,KS,
    KS,TS,TS,-1
};
#endif
#ifdef LEFT_HAND
const char keymap[NUM_KEYS] = {
    'q', 'w', 'e', 'r', // ROW 0
    'a', 's', 'd', 'f', // ROW 1
    'z', 'x', 'c', 'v'  // ROW 2
};
const int keyType[NUM_KEYS] = {
    TS,TS,TS,TS,
    KS,KS,KS,KS,
    KS,TS,TS,-1
};
#endif
// #define TS 0//TOUCH SENSOR
// #define KS 1//KEY SWITCH
// struct inputKeys{
//     char keystr;
//     int row_w;
//     int col_w;
//     int type;
// };
// inputKeys key[ROWS][COLS] = {
//     {'q',0,0,TS}
// };