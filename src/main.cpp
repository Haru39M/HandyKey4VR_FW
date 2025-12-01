#include <NimBleKeyboard.h>
#include "keymap.h" // 作成したキーマップヘッダをインクルード

#ifdef RIGHT_HAND
#define DEVICE_NAME "HandyKey4VR_R"
// (もし右手が対称的な配線なら、このように素直な並びになる)
const int physical_to_logical_map[ROWS][COLS] = {
    // Col 0(D0), Col 1(D1), Col 2(D2), Col 3(D3)
    { 4,  5,  6,  7 }, // Row 1 (D9)  -> logical_keymap[4] ('j') ～ [7] (';')
    { 0,  1,  2,  3 }, // Row 0 (D10) -> logical_keymap[0] ('u') ～ [3] ('p')
    { 8,  9, 10, -1 }  // Row 2 (D8)  -> logical_keymap[8] ('m') ～ [11] ('/')
};
const int physical_to_finger_map[ROWS][COLS] = {
      { 1,  3,  5,  7 }, // Row 0 (Top)
      { 1,  3,  5,  7 }, // Row 1 (Home)
      { 1,  3,  5, -1 }  // Row 2 (Bottom)
  };
#endif
#ifdef LEFT_HAND
#define DEVICE_NAME "HandyKey4VR_L"
// (★左手の非対称な配線を、ここで吸収する *例*)
// 例えば、物理(D10, D0)のキーが、論理マップの'a' (インデックス4) の場合
// 例えば、物理(D10, D1)のキーが、論理マップの'q' (インデックス0) の場合
const int physical_to_logical_map[ROWS][COLS] = {
    // Col 0(D0), Col 1(D1), Col 2(D2), Col 3(D3)
    { 7,  6,  5,  4 }, // Row 0 (D10) -> 'f', 'd', 's', 'a'
    { 3,  2,  1,  0 }, // Row 1 (D9)  -> 'r', 'e', 'w', 'q'
    { 9, 8, 10,  -1 }  // Row 2 (D8)  -> 'z','x','c',(なし)
    // ※この並びは、あなたの実際の配線に合わせてください
};
const int physical_to_finger_map[ROWS][COLS] = {
      { 6,  4,  2,  0 }, // Row 0
      { 6,  4,  2,  0 }, // Row 1
      { 6,  4,  2, -1 }  // Row 2
  };
#endif

// --- BLE カスタム通信設定 ---
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BleKeyboard bleKeyboard(DEVICE_NAME,"HandyKey",100);
NimBLECharacteristic* pFingerCharacteristic = nullptr;

// LED
const int PinLED01 = LED_BUILTIN;

// ピン定義 (keymap.h の ROWS, COLS を使用)
const int colPins[COLS] = {D0, D1, D2, D3}; // 列 (入力)
const int rowPins[ROWS] = {D10, D9, D8};    // 行 (出力)

// キーの状態配列(true: 押されている, false: 離されている)
bool currentKeyState[ROWS][COLS] = {false};
bool prevKeyState[ROWS][COLS] = {false};

// --- 関数プロトタイプ ---
void scanMatrix();
void processKeys();
void setupBLECustomService();
void sendFingerData(int fingerId);

/**
 * @brief キーマトリクスをスキャンし、現在の状態を currentKeyState に格納する
 */
void scanMatrix()
{
    for (int r = 0; r < ROWS; r++){
        digitalWrite(rowPins[r], HIGH);// 1. 注目するROWピン(r)をHIGHにする
        delayMicroseconds(50); // 50マイクロ秒// ピンの電圧が安定するのをわずかに待つ (重要)
        for (int c = 0; c < COLS; c++)// 2. このROW(r)に接続されているCOLピン(c)の状態を読む
        {
            currentKeyState[r][c] = (digitalRead(colPins[c]) == HIGH);// INPUT_PULLDOWN: 押されるとHIGH == true
        }
        digitalWrite(rowPins[r], LOW);// 3. 注目したROWピン(r)をLOWに戻す (次のスキャンのため)
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
// 1. 物理[r][c]から「論理インデックス」を取得
            int logical_index = physical_to_logical_map[r][c];

            // 2. インデックスが -1 (キーなし) の場合は、この交差点を無視
            if (logical_index == -1)
            {
                continue; 
            }

            // 3. 論理キーマップから、送信すべき「キー文字」を取得
            char key = keymap[logical_index];
            // 状態が変化したキーのみ処理する (前回と今回で状態が違う)
            if (currentKeyState[r][c] != prevKeyState[r][c])
            {
                if (currentKeyState[r][c])
                {
                    if(keyType[logical_index] == TS){//タッチセンサは処理しない
                        continue;
                    }
                    else
                    {
                        // 指IDを取得
                        int fingerId = physical_to_finger_map[r][c];

                        if (fingerId != -1) {
                            Serial.printf("Finger Input: %d (Row:%d, Col:%d)\n", fingerId, r, c);
                            // ★ ここで指IDをBLE送信
                            sendFingerData(fingerId);
                        }

                        // 押された (Pressed) キーを押す (押しっぱなしにする)
                        Serial.printf("Pressed: %c (%d,%d)",key,r,c);
                        bleKeyboard.press(key);
                    }
                }
                else
                {
                    // 離された (Released)
                    Serial.printf("Released: %c (%d,%d)",key,r,c);
                    bleKeyboard.release(key);
                }
            }
            prevKeyState[r][c] = currentKeyState[r][c];//状態更新
        }
    }
}

// ★ 指ID (0-7) を送信する関数
void sendFingerData(int value) {
    if (bleKeyboard.isConnected() && pFingerCharacteristic != nullptr) {
        // 1バイト(uint8_t)として送信
        uint8_t data = (uint8_t)value; 
        pFingerCharacteristic->setValue(&data, 1);
        pFingerCharacteristic->notify(); 
    }
}

void setupBLECustomService() {
    NimBLEServer* pServer = NimBLEDevice::getServer();
    NimBLEService* pService = pServer->createService(SERVICE_UUID);
    pFingerCharacteristic = pService->createCharacteristic(
                                CHARACTERISTIC_UUID,
                                NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
                            );
    pService->start();
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
}

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
    setupBLECustomService();
    Serial.println("Waiting for BLE connection...");
}

void loop()
{
    if (bleKeyboard.isConnected())
    {
        digitalWrite(PinLED01, LOW);
        scanMatrix();
        processKeys();
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
