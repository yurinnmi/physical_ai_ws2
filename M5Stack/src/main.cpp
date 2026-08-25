#include <Arduino.h>
#include <M5Unified.h>

#include "clock_display.h"

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
// ============================================================

namespace
{

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
