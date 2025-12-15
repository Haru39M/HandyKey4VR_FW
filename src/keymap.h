#pragma once

#define KEY_SPACE 0x20
#define KEY_BACKSPACE 0xB2
#define KEY_RETURN 0xB0
#define KEY_FN 0xF0

// -- キーマトリクスの定義 --
#define ROWS 3 // 行数
#define COLS 4 // 列数
#define NUM_KEYS (ROWS * COLS)

#define TS 0 // TOUCH SENSOR
#define KS 1 // KEY SWITCH

// -- キーマップの定義 --
// 右手3段 (uiop, jkl;, m,./)
// BleKeyboard.press() / release() はASCII文字をそのままキーコードとして扱えます。
#ifdef RIGHT_HAND
const uint8_t keymap[2][NUM_KEYS] = {
    {
        // layer 0
        'j', 'k', 'l', ';',   // 論理的な2行目 (インデックス 4～7)
        'u', 'i', 'o', 'p',   // 論理的な1行目 (インデックス 0～3)
        KEY_FN, ',', '.', '/' // 論理的な3行目 (インデックス 8～11)
    },
    {
        // layer 1
        'j', 'k', 'l', ';',                      // 論理的な2行目 (インデックス 4～7)
        KEY_BACKSPACE, 0, KEY_SPACE, KEY_RETURN, // 論理的な1行目 (インデックス 0～3)
        KEY_FN, ',', '.', '/'                    // 論理的な3行目 (インデックス 8～11)
    }};
const int keyType[NUM_KEYS] = {
    TS, TS, TS, TS,
    KS, KS, KS, KS,
    KS, TS, TS, -1};
#endif
#ifdef LEFT_HAND
// const char keymap[NUM_KEYS] = {
//     'q', 'w', 'e', 'r', // ROW 0
//     'a', 's', 'd', 'f', // ROW 1
//     'z', 'x', 'c', 'v'  // ROW 2
// };
const uint8_t keymap[2][NUM_KEYS] = {
    {
        // layer 0
        'q', 'w', 'e', 'r',   // ROW 0
        'a', 's', 'd', 'f',   // ROW 1
        KEY_FN, 'x', 'c', 'v' // ROW 2
    },
    {
        // layer 1
        'q', 'w', 'e', 'r',                      // ROW 0
        KEY_RETURN, KEY_SPACE, 0, KEY_BACKSPACE, // ROW 1
        KEY_FN, 'x', 'c', 'v'                    // ROW 2
    }};
const int keyType[NUM_KEYS] = {
    TS, TS, TS, TS,
    KS, KS, KS, KS,
    KS, TS, TS, -1};
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