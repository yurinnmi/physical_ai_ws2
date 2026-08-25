#pragma once

#include <Arduino.h>
#include <M5Unified.h>

class ClockDisplay
{
public:
    explicit ClockDisplay(M5Canvas& canvas);

    // TIME,HH:MM:SS 受信時に呼び出す。
    // 起動後に1回受信すれば、その後は millis() で時刻を進める。
    // 必要なら後から再度呼び出して再同期してもよい。
    bool setTime(int hour, int minute, int second);

    // 時計画面を直ちに描画する。
    void draw();

    // CLOCK状態中に loop() から呼び出す。
    // 秒が変化したときだけ再描画する。
    void update();

    bool isTimeValid() const;

private:
    bool getCurrentTime(int& hour, int& minute, int& second) const;
    void drawClockFace(int hour, int minute, int second);
    void drawWaiting();

    M5Canvas& _canvas;

//    bool _timeValid = false;
    bool _timeValid = true;
    uint32_t _baseSeconds = 0;
    uint32_t _baseMillis = 0;
    int32_t _lastDrawnSecond = -1;
};
