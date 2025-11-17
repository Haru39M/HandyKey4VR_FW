// #include <BleKeyboard.h> // 元のコード
#include <NimBleKeyboard.h>
#include "keymap.h" // 作成したキーマップヘッダをインクルード

BleKeyboard bleKeyboard("HandyKey4VR");

// LED
const int PinLED01 = LED_BUILTIN;

// ピン定義 (keymap.h の ROWS, COLS を使用)
const int colPins[COLS] = {D0, D1, D2, D3}; // 列 (入力)
const int rowPins[ROWS] = {D10, D9, D8};    // 行 (出力)

// キーの状態を保持する配列 (QMKの基本的な考え方)
// true: 押されている, false: 離されている
bool currentKeyState[ROWS][COLS] = {false};
bool prevKeyState[ROWS][COLS] = {false};

void setup()
{
    Serial.begin(115200);
    pinMode(PinLED01, OUTPUT);
    digitalWrite(PinLED01, LOW);

    // COLピン (入力・プルダウン)
    // PULLDOWNなので、ROWからHIGHが流れてきたらHIGHを検出する
    for (int c = 0; c < COLS; c++)
    {
        pinMode(colPins[c], INPUT_PULLDOWN);
    }

    // ROWピン (出力)
    // スキャン時に順番にHIGHにするため、初期状態はLOWにする
    for (int r = 0; r < ROWS; r++)
    {
        pinMode(rowPins[r], OUTPUT);
        digitalWrite(rowPins[r], LOW);
    }

    bleKeyboard.begin();
}

/**
 * @brief キーマトリクスをスキャンし、現在の状態を currentKeyState に格納する
 */
void scanMatrix()
{
    for (int r = 0; r < ROWS; r++)
    {
        // 1. 注目するROWピン(r)をHIGHにする
        digitalWrite(rowPins[r], HIGH);

        // ピンの電圧が安定するのをわずかに待つ (重要)
        delayMicroseconds(50); // 50マイクロ秒

        // 2. このROW(r)に接続されているCOLピン(c)の状態を読む
        for (int c = 0; c < COLS; c++)
        {
            if (digitalRead(colPins[c]) == HIGH)
            {
                // HIGH = 電流が流れている = キーが押されている
                currentKeyState[r][c] = true;
            }
            else
            {
                // LOW = 電流が流れていない = キーが離されている
                currentKeyState[r][c] = false;
            }
        }

        // 3. 注目したROWピン(r)をLOWに戻す (次のスキャンのため)
        digitalWrite(rowPins[r], LOW);
    }
}

/**
 * @brief キーの状態変化を検出し、BLE経由でキー送信を処理する
 */
void processKeys()
{
    if (!bleKeyboard.isConnected())
        return;

    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {

            // 状態が変化したキーのみ処理する (前回と今回で状態が違う)
            if (currentKeyState[r][c] != prevKeyState[r][c])
            {

                char key = keymap[r][c];

                if (currentKeyState[r][c])
                {
                    // 押された (Pressed)
                    Serial.print("Pressed: ");
                    Serial.print(key);
                    Serial.print(" (");
                    Serial.print(r);
                    Serial.print(", ");
                    Serial.print(c);
                    Serial.println(")");

                    // キーを押す (押しっぱなしにする)
                    bleKeyboard.press(key);
                }
                else
                {
                    // 離された (Released)
                    Serial.print("Released: ");
                    Serial.print(key);
                    Serial.print(" (");
                    Serial.print(r);
                    Serial.print(", ");
                    Serial.print(c);
                    Serial.println(")");

                    // キーを離す
                    bleKeyboard.release(key);
                }
            }

            // 処理が終わったら、今回の状態を「前回の状態」として保存する
            prevKeyState[r][c] = currentKeyState[r][c];
        }
    }
}

void loop()
{
    if (bleKeyboard.isConnected())
    {
        // 接続中はLED消灯
        digitalWrite(PinLED01, LOW);

        // 1. マトリクススキャンを実行
        scanMatrix();

        // 2. キー処理 (押下/リリースの判定と送信)
        processKeys();

        // スキャン間隔 (ポーリングレート)
        // このdelayが簡易的なデバウンス（チャタリング防止）の役割も果たす
        // 10ms = 100Hz。キーボードとしては十分な速度。
        delay(10);
    }
    else
    {
        // 未接続時はLEDを点滅
        Serial.println("not connected");
        digitalWrite(PinLED01, LOW);
        delay(1000);
        digitalWrite(PinLED01, HIGH);
        delay(1000);
    }
}
