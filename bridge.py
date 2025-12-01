import asyncio
from bleak import BleakScanner, BleakClient
from pythonosc import udp_client

# --- 設定 ---
# ESP32のCharacteristic UUID
CHARACTERISTIC_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

# main4.py への送信設定
OSC_IP = "127.0.0.1"
OSC_PORT = 8000  # main4.py の待ち受けポート
OSC_ADDRESS = "/input/char" # main4.py が期待しているアドレス

# OSCクライアント
client_osc = udp_client.SimpleUDPClient(OSC_IP, OSC_PORT)

def notification_handler(sender, data):
    """ESP32からデータが届いた時の処理"""
    # 1バイトのデータを整数に変換
    finger_id = int.from_bytes(data, byteorder='little')
    
    print(f"[BLE Received] Finger ID: {finger_id}")
    
    # OSCで main4.py へ送信
    client_osc.send_message(OSC_ADDRESS, finger_id)
    print(f"   -> [OSC Sent] {OSC_ADDRESS} {finger_id}")

async def run_ble_client():
    print("ESP32 (HandyKey) を探しています...")
    
    # デバイススキャン ("HandyKey"を含むデバイスを探す)
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: d.name and "HandyKey" in d.name
    )

    if not device:
        print("デバイスが見つかりませんでした。")
        return

    print(f"接続中... {device.name}")

    async with BleakClient(device) as client:
        print("BLE接続完了！ OSC転送を開始します。")
        print(f"Target: {OSC_IP}:{OSC_PORT} {OSC_ADDRESS}")
        
        # 通知の購読を開始
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)

        # 接続維持 (Ctrl+Cで終了されるまで)
        while True:
            await asyncio.sleep(1)

if __name__ == "__main__":
    try:
        asyncio.run(run_ble_client())
    except KeyboardInterrupt:
        print("\n終了します。")