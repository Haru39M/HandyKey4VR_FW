# HandyKey4VR Firmware

Seeed Studio XIAO ESP32C6 を使用した VR用ウェアラブルキーボードデバイス「HandyKey4VR」のファームウェアです。
本デバイスは Bluetooth Low Energy (BLE) を介して、標準的な HID キーボードとしての入力と、特定の指の動きを識別するためのカスタムデータ通信を同時に行います。

## システム概要

* **MCU**: Seeed Studio XIAO ESP32C6
* **Framework**: Arduino (PlatformIO)
* **BLE Stack**: NimBLE-Arduino
* **デバイス構成**: 左手用 (`HK4VR_L`) / 右手用 (`HK4VR_R`) の2構成（ビルドフラグで切り替え）

## 通信プロトコル仕様

本デバイスは BLE サーバー（ペリフェラル）として動作し、以下の2つのサービスを提供します。

### 1. HID over GATT (標準キーボード)
PCやスマートフォンからは標準的な Bluetooth キーボードとして認識されます。
`BleKeyboard` ライブラリを使用しており、キーマップに基づいたキーストローク（Press/Release）を送信します。

* **Device Name**:
    * 右手用: `HandyKey4VR_R`
    * 左手用: `HandyKey4VR_L`

### 2. カスタムデータ通信 (Finger ID Notification)
キー入力時に、どの「指」で入力が行われたかを識別するための独自データを送信します。受信側のアプリケーションはこのデータを使用して、VR空間内での指の動きを連動させることが可能です。

#### BLE サービス定義

| 項目 | UUID | プロパティ | 備考 |
| :--- | :--- | :--- | :--- |
| **Service UUID** | `4fafc201-1fb5-459e-8fcc-c5c9c331914b` | - | カスタムサービス |
| **Characteristic UUID** | `beb5483e-36e1-4688-b7f5-ea07361b26a8` | Read, **Notify** | Finger ID 送信用 |

#### データパケット構造 (Characteristic)

Characteristic `beb5483e...` に対して **Notify** で送信されるデータは以下の通りです。

* **データ長**: 1 byte (`uint8_t`)
* **送信タイミング**: キーが押された瞬間 (`Pressed`) のみ送信されます（Release時は送信されません）。
* **値の定義 (Finger ID)**:
    ハードウェアの物理配線に基づき、キーマトリクスの位置から以下のIDがマッピングされます。

| 値 (int) | 意味 | 備考 |
| :--- | :--- | :--- |
| `0` ～ `7` | 指ID | 配線定義 `physical_to_finger_map` に基づく |

> **注意**: キータイプが `TS` (Touch Sensor) と定義されているキー、および論理マップが無効 (`-1`) なキー入力時は、このデータは送信されません。

## ハードウェア・ピンアサイン

3行4列 (3 Rows x 4 Cols) のキーマトリクス構成です。

* **Column Pins (Input / Pull-down)**: D0, D1, D2, D3
* **Row Pins (Output)**: D10, D9, D8

### キーマップとレイヤー

`src/keymap.h` に定義されています。
* **Layer 0**: デフォルト
* **Layer 1**: `KEY_FN` 押下中にアクティブ化

## 受信クライアント実装ガイド

本リポジトリには、BLEデータを受信して OSC (Open Sound Control) に変換するブリッジスクリプトが含まれています。サーバーサイド開発の参考にしてください。

### Python ブリッジ (`bridge.py`) の仕様

`Bleak` ライブラリを使用してBLE通信を行い、受信したFinger IDをUDP (OSC) で転送します。

1.  **接続フロー**:
    * デバイス名に "HandyKey" を含むデバイスをスキャンして接続。
    * Characteristic `beb5483e...` の Notification を購読 (Subscribe)。

2.  **データ転送 (OSC)**:
    * **宛先**: `127.0.0.1` (localhost)
    * **ポート**: `8000`
    * **アドレス**: `/input/char`
    * **引数**: Finger ID (int)

```python
# 受信データの処理イメージ (Python)
def notification_handler(sender, data):
    finger_id = int.from_bytes(data, byteorder='little')
    # OSC送信: /input/char <finger_id>
```

## ビルドと書き込み

PlatformIO を使用してビルド・書き込みを行います。

### 環境の選択 (`platformio.ini`)

| 環境名 (`[env:...]`) | 用途 | ビルドフラグ |
| --- | --- | --- |
| `HK4VR_R_seeed_xiao_esp32c6` | 右手用ファームウェア | `-D RIGHT_HAND` |
| `HK4VR_L_seeed_xiao_esp32c6` | 左手用ファームウェア | `-D LEFT_HAND` |

### コマンド例
```bash
# 右手用をビルドして書き込み
pio run -e HK4VR_R_seeed_xiao_esp32c6 -t upload

# 左手用をビルドして書き込み
pio run -e HK4VR_L_seeed_xiao_esp32c6 -t upload
```
## ディレクトリ構成
* `src/main.cpp`: メインロジック (BLEセットアップ、キースキャン、データ送信)
* `src/keymap.h`: キーマップ定義 (レイヤー、キーコード)
* `platformio.ini`: ビルド設定、依存ライブラリ、ボード定義
* `bridge.py`: テスト用 BLE-OSC ブリッジスクリプト