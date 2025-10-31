// #include <BleKeyboard.h>
#include <NimBleKeyboard.h>
BleKeyboard bleKeyboard("HandyKey4VR");

// LED
const int PinLED01 = 0;
// Button
const int PinButtonA = 1;
const int PinButtonB = 2;

void setup()
{
    Serial.begin(115200);
    pinMode(PinLED01, OUTPUT);
    digitalWrite(PinLED01, LOW);
    pinMode(PinButtonA, INPUT_PULLUP);
    pinMode(PinButtonB, INPUT_PULLUP);
    bleKeyboard.begin();
}

void loop()
{
    if (bleKeyboard.isConnected())
    {
        // Serial.println("Connected!");
        digitalWrite(PinLED01, HIGH);
        if (digitalRead(PinButtonA) != true)
        {
            Serial.println("PRESSED!");
            bleKeyboard.print("Hello world");
        }
        else if (digitalRead(PinButtonB) != true)
        {
            bleKeyboard.write(KEY_RETURN);
        }
        delay(10);
    }
    else
    {
        Serial.println("not connected");
        digitalWrite(PinLED01, HIGH);
        delay(1000);
        digitalWrite(PinLED01, LOW);
        delay(1000);
    }
}