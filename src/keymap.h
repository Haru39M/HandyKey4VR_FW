#pragma once

// -- キーマトリクスの定義 --
#define ROWS 3 // 行数
#define COLS 4 // 列数

// -- キーマップの定義 --
// 右手3段 (uiop, jkl;, m,./)
// BleKeyboard.press() / release() はASCII文字をそのままキーコードとして扱えます。
#ifdef RIGHT_HAND
char keymap[ROWS][COLS] = {
    {'u', 'i', 'o', 'p'}, // ROW 0
    {'j', 'k', 'l', ';'}, // ROW 1
    {'m', ',', '.', '/'}  // ROW 2
};
#endif
#ifdef LEFT_HAND
char keymap[ROWS][COLS] = {
    {'q', 'w', 'e', 'r'}, // ROW 0
    {'a', 's', 'd', 'f'}, // ROW 1
    {'z', 'x', 'c', 'v'}  // ROW 2
};
#endif