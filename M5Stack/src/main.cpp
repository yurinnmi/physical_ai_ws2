#include <Arduino.h>
#include <M5Unified.h>

#include "clock_display.h"

// SG92Rサーボ制御を無効にする場合はこの行をコメントアウトする。
#define ENABLE_SG92R_SERVO

#ifdef ENABLE_SG92R_SERVO
#include <ESP32Servo.h>
#endif

// 複数物体検出表示 (teddy bear / cup / bottle) を無効にする場合は
// この行をコメントアウトする（無効時は従来のPERSON,0/1のみの表示に戻る）。
#define ENABLE_MULTI_OBJECT_DISPLAY

// ============================================================
// Physical AI Demo - M5Stack side
//
// Raspberry Pi -> M5Stack UART protocol
//
//   TIME,HH:MM:SS
//   PERSON,0
//   PERSON,1
//
// TIME:
//   Raspberry Pi起動時またはUART接続時に1回送信。
//   M5Stackは受信した時刻を基準にmillis()で時計を進める。
//   必要なら後からTIMEを再送して再同期してもよい。
//
// PERSON:
//   状態変化時だけ送信。
//   PERSON,1 : 人物検出
//   PERSON,0 : 人物未検出
//
// M5Stack Basic UART2
//   RX = GPIO16
//   TX = GPIO17
//
// SG92Rサーボ (ENABLE_SG92R_SERVO 有効時)
//   信号線 = GPIO5 (使用するM5Stackモデル・配線に応じて変更可)
//   時計表示中   : 120度
//   人物検出表示中: 30度
//
// 複数物体検出表示 (ENABLE_MULTI_OBJECT_DISPLAY 有効時)
//   TEDDY_BEAR,0 / TEDDY_BEAR,1
//   CUP,0        / CUP,1
//   BOTTLE,0     / BOTTLE,1
//   3つのうち検出中のものをすべて画面に表示する。
//   全て未検出に戻ったら時計表示に戻る。
// ============================================================

namespace
{

#ifdef ENABLE_SG92R_SERVO
constexpr int SERVO_PIN = 5;
constexpr int SERVO_ANGLE_CLOCK = 120;
constexpr int SERVO_ANGLE_PERSON_DETECTED = 30;

Servo sg92rServo;
#endif

constexpr uint32_t UART_BAUD = 115200;
constexpr int UART_RX_PIN = 16;
constexpr int UART_TX_PIN = 17;

constexpr size_t RX_BUFFER_SIZE = 64;
char rxBuffer[RX_BUFFER_SIZE];
size_t rxIndex = 0;

M5Canvas canvas(&M5.Display);
ClockDisplay clockDisplay(canvas);

enum class ScreenState
{
    Clock,
    PersonDetected
#ifdef ENABLE_MULTI_OBJECT_DISPLAY
    ,
    MultiObjectDetected
#endif
};

ScreenState screenState = ScreenState::Clock;

void drawPersonDetected()
{
    canvas.fillSprite(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextSize(5);

    canvas.drawString(
        "PERSON",
        canvas.width() / 2,
        92);

    canvas.drawString(
        "DETECTED",
        canvas.width() / 2,
        150);

    canvas.pushSprite(0, 0);
}

void redrawCurrentScreen()
{
    if (screenState == ScreenState::PersonDetected)
    {
        drawPersonDetected();
    }
    else
    {
        clockDisplay.draw();
    }
}

void setScreenState(ScreenState newState)
{
    if (screenState == newState)
    {
        return;
    }

    screenState = newState;
    redrawCurrentScreen();

#ifdef ENABLE_SG92R_SERVO
    sg92rServo.write(
        screenState == ScreenState::PersonDetected
            ? SERVO_ANGLE_PERSON_DETECTED
            : SERVO_ANGLE_CLOCK);
#endif
}

void handleTimeCommand(const char* payload)
{
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (sscanf(
            payload,
            "%d:%d:%d",
            &hour,
            &minute,
            &second) != 3)
    {
        Serial.printf(
            "Invalid TIME command: %s\n",
            payload);
        return;
    }

    if (!clockDisplay.setTime(hour, minute, second))
    {
        Serial.printf(
            "Out-of-range TIME command: %s\n",
            payload);
        return;
    }

    Serial.printf(
        "TIME <- %02d:%02d:%02d\n",
        hour,
        minute,
        second);

    if (screenState == ScreenState::Clock)
    {
        clockDisplay.draw();
    }
}

void handlePersonCommand(const char* payload)
{
    if (strcmp(payload, "1") == 0)
    {
        Serial.println("PERSON <- 1");
        setScreenState(ScreenState::PersonDetected);
        return;
    }

    if (strcmp(payload, "0") == 0)
    {
        Serial.println("PERSON <- 0");
        setScreenState(ScreenState::Clock);
        return;
    }

    Serial.printf(
        "Invalid PERSON command: %s\n",
        payload);
}

#ifdef ENABLE_MULTI_OBJECT_DISPLAY

bool teddyBearDetected = false;
bool cupDetected = false;
bool bottleDetected = false;

bool anyMultiObjectDetected()
{
    return teddyBearDetected || cupDetected || bottleDetected;
}

void drawMultiObjectDetected()
{
    struct Item
    {
        bool detected;
        const char* label;
    };

    const Item items[] = {
        {teddyBearDetected, "TEDDY BEAR"},
        {cupDetected, "CUP"},
        {bottleDetected, "BOTTLE"},
    };

    int activeCount = 0;
    for (const auto& item : items)
    {
        if (item.detected)
        {
            activeCount++;
        }
    }

    canvas.fillSprite(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.setTextDatum(textdatum_t::middle_center);
    canvas.setTextSize(4);

    if (activeCount == 0)
    {
        canvas.pushSprite(0, 0);
        return;
    }

    int lineHeight = canvas.height() / (activeCount + 1);
    int y = lineHeight;

    for (const auto& item : items)
    {
        if (!item.detected)
        {
            continue;
        }

        canvas.drawString(item.label, canvas.width() / 2, y);
        y += lineHeight;
    }

    canvas.pushSprite(0, 0);
}

void updateMultiObjectScreen()
{
    if (anyMultiObjectDetected())
    {
        screenState = ScreenState::MultiObjectDetected;
        drawMultiObjectDetected();

#ifdef ENABLE_SG92R_SERVO
        sg92rServo.write(SERVO_ANGLE_PERSON_DETECTED);
#endif
        return;
    }

    if (screenState == ScreenState::MultiObjectDetected)
    {
        screenState = ScreenState::Clock;
        redrawCurrentScreen();

#ifdef ENABLE_SG92R_SERVO
        sg92rServo.write(SERVO_ANGLE_CLOCK);
#endif
    }
}

void handleMultiObjectCommand(const char* tag, const char* payload)
{
    bool* target = nullptr;

    if (strcmp(tag, "TEDDY_BEAR") == 0)
    {
        target = &teddyBearDetected;
    }
    else if (strcmp(tag, "CUP") == 0)
    {
        target = &cupDetected;
    }
    else if (strcmp(tag, "BOTTLE") == 0)
    {
        target = &bottleDetected;
    }

    if (target == nullptr)
    {
        return;
    }

    if (strcmp(payload, "1") == 0)
    {
        *target = true;
    }
    else if (strcmp(payload, "0") == 0)
    {
        *target = false;
    }
    else
    {
        Serial.printf(
            "Invalid %s command: %s\n",
            tag,
            payload);
        return;
    }

    Serial.printf(
        "%s <- %s\n",
        tag,
        payload);

    updateMultiObjectScreen();
}

#endif // ENABLE_MULTI_OBJECT_DISPLAY

void processCommand(char* line)
{
    if (line[0] == '\0')
    {
        return;
    }

    if (strncmp(line, "TIME,", 5) == 0)
    {
        handleTimeCommand(line + 5);
        return;
    }

    if (strncmp(line, "PERSON,", 7) == 0)
    {
        handlePersonCommand(line + 7);
        return;
    }

#ifdef ENABLE_MULTI_OBJECT_DISPLAY
    if (strncmp(line, "TEDDY_BEAR,", 11) == 0)
    {
        handleMultiObjectCommand("TEDDY_BEAR", line + 11);
        return;
    }

    if (strncmp(line, "CUP,", 4) == 0)
    {
        handleMultiObjectCommand("CUP", line + 4);
        return;
    }

    if (strncmp(line, "BOTTLE,", 7) == 0)
    {
        handleMultiObjectCommand("BOTTLE", line + 7);
        return;
    }
#endif

    Serial.printf(
        "Unknown command: %s\n",
        line);
}

void receiveUart()
{
    while (Serial2.available() > 0)
    {
        const char c =
            static_cast<char>(Serial2.read());

        if (c == '\n')
        {
            rxBuffer[rxIndex] = '\0';
            processCommand(rxBuffer);
            rxIndex = 0;
            continue;
        }

        // CRLFにも対応
        if (c == '\r')
        {
            continue;
        }

        if (rxIndex < RX_BUFFER_SIZE - 1)
        {
            rxBuffer[rxIndex++] = c;
        }
        else
        {
            rxIndex = 0;

            Serial.println(
                "UART RX buffer overflow; line discarded.");
        }
    }
}

} // namespace

void setup()
{
    Serial.begin(115200);

    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setRotation(1);
    M5.Display.fillScreen(TFT_BLACK);

    canvas.setColorDepth(8);

    if (canvas.createSprite(
            M5.Display.width(),
            M5.Display.height()) == nullptr)
    {
        Serial.println(
            "ERROR: Failed to allocate display sprite.");

        while (true)
        {
            delay(1000);
        }
    }

    Serial2.begin(
        UART_BAUD,
        SERIAL_8N1,
        UART_RX_PIN,
        UART_TX_PIN);

#ifdef ENABLE_SG92R_SERVO
    sg92rServo.setPeriodHertz(50);
    sg92rServo.attach(SERVO_PIN, 500, 2400);
    sg92rServo.write(SERVO_ANGLE_CLOCK);
#endif

    Serial.println();
    Serial.println(
        "Physical AI Demo - M5Stack");

    Serial.printf(
        "UART2: %lu bps, RX=GPIO%d, TX=GPIO%d\n",
        UART_BAUD,
        UART_RX_PIN,
        UART_TX_PIN);

    // TIME未受信時は "--:--" を表示
    redrawCurrentScreen();
}

void loop()
{
    M5.update();

    receiveUart();

    if (screenState == ScreenState::Clock)
    {
        clockDisplay.update();
    }

    delay(1);
}
