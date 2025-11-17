#pragma once

// -- キーマトリクスの定義 --
#define ROWS 3 // 行数
#define COLS 4 // 列数

// -- キーマップの定義 --
// 右手3段 (uiop, jkl;, m,./)
// BleKeyboard.press() / release() はASCII文字をそのままキーコードとして扱えます。
char keymap[ROWS][COLS] = {
    {'u', 'i', 'o', 'p'}, // ROW 0
    {'j', 'k', 'l', ';'}, // ROW 1
    {'m', ',', '.', '/'}  // ROW 2
};
