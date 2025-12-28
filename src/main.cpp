#include <NimBleKeyboard.h>
#include "keymap.h" // 作成したキーマップヘッダをインクルード

#ifdef RIGHT_HAND
#define DEVICE_NAME "HandyKey4VR_R"

// #define FINGER_ID_T 8 //Thumb
// #define FINGER_ID_I 1 //Index
// #define FINGER_ID_M 3 //Middle
// #define FINGER_ID_R 5 //Ring
// #define FINGER_ID_P 7 //Pinky

enum FingerID{
    FINGER_T = 8,//Thumb
    FINGER_I = 1,//Index
    FINGER_M = 3,//Middle
    FINGER_R = 5,//Ring
    FINGER_P = 7,//Pinky
    FINGER_X = -1,//Not Finger
};

// (もし右手が対称的な配線なら、このように素直な並びになる)
const int physical_to_logical_map[ROWS][COLS] = {
    // Col 0(D0), Col 1(D1), Col 2(D2), Col 3(D3)
    {4, 5, 6, 7},  // Row 1 (D9)  -> logical_keymap[4] ('j') ～ [7] (';')
    {0, 1, 2, 3},  // Row 0 (D10) -> logical_keymap[0] ('u') ～ [3] ('p')
    {8, 9, 10, -1} // Row 2 (D8)  -> logical_keymap[8] ('m') ～ [11] ('/')
};
const int physical_to_finger_map[ROWS][COLS] = {
    {FINGER_I, FINGER_M, FINGER_R, FINGER_P}, // Row 0 (Top)
    {FINGER_I, FINGER_M, FINGER_R, FINGER_P}, // Row 1 (Home)
    {FINGER_T, FINGER_T, FINGER_T, FINGER_X} // Row 2 (Bottom) ★修正: 親指(ID=8)として割り当て
};
#endif
#ifdef LEFT_HAND
#define DEVICE_NAME "HandyKey4VR_L"

// #define FINGER_ID_T 8 //Thumb
// #define FINGER_ID_I 0 //Index
// #define FINGER_ID_M 2 //Middle
// #define FINGER_ID_R 4 //Ring
// #define FINGER_ID_P 6 //Pinky

enum FingerID{
    FINGER_T = 8,//Thumb
    FINGER_I = 0,//Index
    FINGER_M = 2,//Middle
    FINGER_R = 4,//Ring
    FINGER_P = 6,//Pinky
    FINGER_X = -1,//Not Finger
};

// (★左手の非対称な配線を、ここで吸収する *例*)
// 例えば、物理(D10, D0)のキーが、論理マップの'a' (インデックス4) の場合
// 例えば、物理(D10, D1)のキーが、論理マップの'q' (インデックス0) の場合
const int physical_to_logical_map[ROWS][COLS] = {
    // Col 0(D0), Col 1(D1), Col 2(D2), Col 3(D3)
    {7, 6, 5, 4},  // Row 0 (D10) -> 'f', 'd', 's', 'a'
    {3, 2, 1, 0},  // Row 1 (D9)  -> 'r', 'e', 'w', 'q'
    {9, 8, 10, -1} // Row 2 (D8)  -> 'z','x','c',(なし)
    // ※この並びは、あなたの実際の配線に合わせてください
};
const int physical_to_finger_map[ROWS][COLS] = {
    {FINGER_I, FINGER_M, FINGER_R, FINGER_P}, // Row 0
    {FINGER_I, FINGER_M, FINGER_R, FINGER_P}, // Row 1
    {FINGER_T, FINGER_T, FINGER_T, FINGER_X} // Row 2 ★修正: 親指(ID=8)として割り当て
};
#endif

// --- BLE カスタム通信設定 ---
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BleKeyboard bleKeyboard(DEVICE_NAME, "HandyKey", 100);
NimBLECharacteristic *pFingerCharacteristic = nullptr;

// LED
const int PinLED01 = LED_BUILTIN;

// ピン定義 (keymap.h の ROWS, COLS を使用)
const int colPins[COLS] = {D0, D1, D2, D3}; // 列 (入力)
const int rowPins[ROWS] = {D10, D9, D8};    // 行 (出力)

// キーの状態配列(true: 押されている, false: 離されている)
bool currentKeyState[ROWS][COLS] = {false};
bool prevKeyState[ROWS][COLS] = {false};

// ★追加: 実際にPCへ送信中のキーコードを記憶する配列 (スタック防止用)
uint8_t activeKeyCodes[ROWS][COLS] = {0};

#define STATE_FINGER_OPEN 0
#define STATE_FINGER_TOUCH 1
#define STATE_FINGER_CLOSE 2

// ★追加: 指の状態管理用 (T, I, M, R, P)
uint8_t currentFingerState[5] = {STATE_FINGER_OPEN}; // 0:OPEN, 1:TOUCH, 2:CLOSE
uint8_t prevFingerState[5] = {STATE_FINGER_OPEN};

// --- 関数プロトタイプ ---
void scanMatrix();
void processKeys();
void processFingerStates(); // ★追加
void setupBLECustomService();
// void sendFingerData(int fingerId); // 削除あるいは使用しない

/**
 * @brief キーマトリクスをスキャンし、現在の状態を currentKeyState に格納する
 */
void scanMatrix()
{
    for (int r = 0; r < ROWS; r++)
    {
        digitalWrite(rowPins[r], HIGH); // 1. 注目するROWピン(r)をHIGHにする
        delayMicroseconds(50);          // 50マイクロ秒// ピンの電圧が安定するのをわずかに待つ (重要)
        for (int c = 0; c < COLS; c++)  // 2. このROW(r)に接続されているCOLピン(c)の状態を読む
        {
            currentKeyState[r][c] = (digitalRead(colPins[c]) == HIGH); // INPUT_PULLDOWN: 押されるとHIGH == true
        }
        digitalWrite(rowPins[r], LOW); // 3. 注目したROWピン(r)をLOWに戻す (次のスキャンのため)
    }
}

/**
 * @brief 指の状態(OPEN/TOUCH/CLOSE)を判定してBLE送信する
 * データ形式: [Thumb, Index, Middle, Ring, Pinky] (5 bytes)
 * 値: 0=OPEN, 1=TOUCH, 2=CLOSE
 */
void processFingerStates()
{
    if (!bleKeyboard.isConnected() || pFingerCharacteristic == nullptr)
        return;

    // 一時的な集計用配列
    bool finger_ts_active[5] = {false};
    bool finger_ks_active[5] = {false};

    // 1. マトリクス全体を走査して、各指のTS/KS状態を集計
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            int f_id = physical_to_finger_map[r][c];
            if (f_id == FINGER_X) continue;

            // Finger IDを配列インデックス(0-4)にマッピング
            // Left(0,2,4,6) -> Index(1), Middle(2), Ring(3), Pinky(4)
            // Right(1,3,5,7) -> Index(1), Middle(2), Ring(3), Pinky(4)
            // Thumb(0)は常にOPENと仮定（または親指用のマップがあればそこに割り当たる）
            
            // ★修正: 親指用ID(8)の場合はインデックス0、それ以外は計算式でマッピング
            int idx = -1;
            if (f_id == FINGER_T) {
                idx = 0; // Thumb
            } else {
                idx = (f_id / 2) + 1; 
            }

            if (idx >= 0 && idx < 5)
            {
                // キーが反応している(HIGH)場合
                if (currentKeyState[r][c])
                {
                    int l_idx = physical_to_logical_map[r][c];
                    if (l_idx != -1)
                    {
                        if (keyType[l_idx] == TS) finger_ts_active[idx] = true;
                        if (keyType[l_idx] == KS) finger_ks_active[idx] = true;
                    }
                }
            }
        }
    }

    // 2. 集計結果から状態を決定 (0:OPEN, 1:TOUCH, 2:CLOSE)
    bool isChanged = false;
    for (int i = 0; i < 5; i++)
    {
        uint8_t state = STATE_FINGER_OPEN; // OPEN
        if (finger_ks_active[i] && finger_ts_active[i])
        {
            state = STATE_FINGER_CLOSE; // CLOSE (KS & TS both active)
        }
        else if (finger_ts_active[i])
        {
            state = STATE_FINGER_TOUCH; // TOUCH (TS only)
        }
        // KSのみONの場合は定義がないためOPEN(0)またはTOUCH扱いとするが、
        // プロンプト定義に従い「KS, TSともに反応」のみをCLOSEとするなら、ここは0で遷移なし。
        // (通常メカニカルSWを押せばTSも反応するため、KS ONならCLOSEになることが多い)
        
        currentFingerState[i] = state;

        if (currentFingerState[i] != prevFingerState[i])
        {
            isChanged = true;
        }
    }

    // 3. 変化があれば送信
    if (isChanged)
    {
        pFingerCharacteristic->setValue(currentFingerState, 5);
        pFingerCharacteristic->notify();
        
        // デバッグ出力
        Serial.printf("BLE Notify: %d %d %d %d %d\n", 
            currentFingerState[0], currentFingerState[1], currentFingerState[2], 
            currentFingerState[3], currentFingerState[4]);

        // 状態更新
        memcpy(prevFingerState, currentFingerState, 5);
    }
}

/**
 * @brief キーの状態変化を検出し、HIDキー送信を処理する
 */
void processKeys()
{
    if (!bleKeyboard.isConnected())
        return;

    // レイヤーキーが押されているか確認してレイヤー切り替え
    uint8_t curr_layer = 0;
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            if (currentKeyState[r][c])
            {
                int logical_index = physical_to_logical_map[r][c];
                if (logical_index != -1)
                {
                    // レイヤー0の定義を見て、KEY_FNならレイヤーを1にする
                    if (keymap[0][logical_index] == KEY_FN)
                    {
                        curr_layer = 1;
                    }
                }
            }
        }
    }
    //     // デバッグ用 (必要に応じて)
    // if (curr_layer == 1)
    //     Serial.println("Layer 1 Active");

    // スキャン
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            // 状態が変化していなければスキップ
            if (currentKeyState[r][c] == prevKeyState[r][c])
            {
                continue;
            }
            // 1. 物理[r][c]から「論理インデックス」を取得
            int logical_index = physical_to_logical_map[r][c];
            // 2. インデックスが -1 (キーなし) の場合は、この交差点を無視
            if (logical_index == -1)
                continue;

            // --- キーが押された (Pressed) ---
            if (currentKeyState[r][c])
            {
                if (keyType[logical_index] == TS)
                { // タッチセンサはHID入力としては処理しない
                    continue;
                }
                // 3. 論理キーマップから、送信すべき「キー文字」を取得
                uint8_t code = keymap[curr_layer][logical_index];
                if (code == KEY_FN)
                {
                }
                else
                {
                    //======FOR VRC=======
                    // 指IDの個別送信(sendFingerData)は削除し、processFingerStatesで一括送信する
                    // 指IDを取得
                    // int fingerId = physical_to_finger_map[r][c];

                    // if (fingerId != FINGER_X)
                    // {
                    //     Serial.printf("Finger Input: %d (Row:%d, Col:%d)\n", fingerId, r, c);
                    //     // ★ ここで指IDをBLE送信
                    //     sendFingerData(fingerId);
                    // }
                    //======FOR VRC=======
                    
                    // 押された (Pressed) キーを押す (押しっぱなしにする)
                    Serial.printf("Pressed: %c (%d,%d)\n", code, r, c);
                    bleKeyboard.press(code);
                    activeKeyCodes[r][c] = code; // ★重要: 何を送ったか記憶する
                }
            }
            // --- キーが離された (Released) ---
            else
            {
                if (keyType[logical_index] == TS)
                    continue;
                // ★重要: 現在のマップではなく「押した時に記憶したコード」を使ってReleaseする
                uint8_t code = activeKeyCodes[r][c];
                if (code != 0)
                {
                    Serial.printf("Released: %c (%d,%d)\n", code, r, c);
                    bleKeyboard.release(code);
                    activeKeyCodes[r][c] = 0;
                }
            }
            prevKeyState[r][c] = currentKeyState[r][c]; // 状態更新
        }
    }
}

// // ★ 指ID (0-7) を送信する関数
// void sendFingerData(int value)
// {
//     if (bleKeyboard.isConnected() && pFingerCharacteristic != nullptr)
//     {
//         // 1バイト(uint8_t)として送信
//         uint8_t data = (uint8_t)value;
//         pFingerCharacteristic->setValue(&data, 1);
//         pFingerCharacteristic->notify();
//     }
// }

void setupBLECustomService()
{
    NimBLEServer *pServer = NimBLEDevice::getServer();
    NimBLEService *pService = pServer->createService(SERVICE_UUID);
    pFingerCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pService->start();
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
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
    //==============================
    // 3. アドバタイズ情報を更新して再開 (重要)
    // NimBLEのアドバタイズインスタンスを取得
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    // 既存のHIDサービスに加えて、カスタムUUIDもリストに追加
    pAdvertising->addServiceUUID(SERVICE_UUID);
    // アドバタイズ設定を反映させるために停止＆再開（念のため）
    pAdvertising->stop();
    pAdvertising->start();
    //==============================
    Serial.println("Waiting for BLE connection...");
}

void loop()
{
    if (bleKeyboard.isConnected())
    {
        digitalWrite(PinLED01, LOW);
        scanMatrix();
        processFingerStates(); // ★指の状態変化をチェックして送信
        processKeys();         // HIDキー入力を処理
        delay(5);
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