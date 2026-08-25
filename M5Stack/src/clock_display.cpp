#include "clock_display.h"

#include <math.h>

namespace
{
uint32_t hmsToSeconds(int hour, int minute, int second)
{
    return static_cast<uint32_t>(
        hour * 3600 + minute * 60 + second);
}
}

ClockDisplay::ClockDisplay(M5Canvas& canvas)
    : _canvas(canvas)
{
}

bool ClockDisplay::setTime(int hour, int minute, int second)
{
    if (hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 ||
        second < 0 || second > 59)
    {
        return false;
    }

    _timeValid = true;
    _baseSeconds = hmsToSeconds(hour, minute, second);
    _baseMillis = millis();
    _lastDrawnSecond = -1;

    return true;
}

bool ClockDisplay::isTimeValid() const
{
    return _timeValid;
}

bool ClockDisplay::getCurrentTime(
    int& hour,
    int& minute,
    int& second) const
{
    if (!_timeValid)
    {
        return false;
    }

    const uint32_t elapsedSeconds =
        (millis() - _baseMillis) / 1000UL;

    const uint32_t totalSeconds =
        (_baseSeconds + elapsedSeconds) % 86400UL;

    hour = totalSeconds / 3600UL;
    minute = (totalSeconds % 3600UL) / 60UL;
    second = totalSeconds % 60UL;

    return true;
}

void ClockDisplay::draw()
{
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (!getCurrentTime(hour, minute, second))
    {
        drawWaiting();
        _lastDrawnSecond = -1;
        return;
    }

    drawClockFace(hour, minute, second);
    _lastDrawnSecond = second;
}

void ClockDisplay::update()
{
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (!getCurrentTime(hour, minute, second))
    {
        return;
    }

    if (second == _lastDrawnSecond)
    {
        return;
    }

    drawClockFace(hour, minute, second);
    _lastDrawnSecond = second;
}

void ClockDisplay::drawWaiting()
{
    _canvas.fillSprite(TFT_BLACK);
    _canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    _canvas.setTextDatum(textdatum_t::middle_center);
    _canvas.setTextSize(3);

    _canvas.drawString(
        "--:--",
        _canvas.width() / 2,
        _canvas.height() / 2);

    _canvas.pushSprite(0, 0);
}

void ClockDisplay::drawClockFace(
    int hour,
    int minute,
    int second)
{
    _canvas.fillSprite(TFT_BLACK);

    const int cx = _canvas.width() / 2;
    const int cy = _canvas.height() / 2;
    const int radius = 104;

    // 外周
    _canvas.drawCircle(cx, cy, radius, TFT_WHITE);
    _canvas.drawCircle(cx, cy, radius - 1, TFT_WHITE);

    // 目盛り
    for (int i = 0; i < 60; ++i)
    {
        const float angle =
            (i * 6.0f - 90.0f) * DEG_TO_RAD;

        const int outerX =
            cx + static_cast<int>(cosf(angle) * (radius - 4));

        const int outerY =
            cy + static_cast<int>(sinf(angle) * (radius - 4));

        const int innerRadius =
            (i % 5 == 0) ? radius - 14 : radius - 8;

        const int innerX =
            cx + static_cast<int>(cosf(angle) * innerRadius);

        const int innerY =
            cy + static_cast<int>(sinf(angle) * innerRadius);

        _canvas.drawLine(
            innerX,
            innerY,
            outerX,
            outerY,
            TFT_WHITE);
    }

    // 12 / 3 / 6 / 9
    _canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    _canvas.setTextDatum(textdatum_t::middle_center);
    _canvas.setTextSize(2);

    _canvas.drawString("12", cx, cy - 78);
    _canvas.drawString("3",  cx + 80, cy);
    _canvas.drawString("6",  cx, cy + 80);
    _canvas.drawString("9",  cx - 80, cy);

    // 各針の角度
    const float secondAngle =
        (second * 6.0f - 90.0f) * DEG_TO_RAD;

    const float minuteAngle =
        ((minute + second / 60.0f) * 6.0f - 90.0f)
        * DEG_TO_RAD;

    const float hourAngle =
        (((hour % 12) + minute / 60.0f) * 30.0f - 90.0f)
        * DEG_TO_RAD;

    // 時針
    const int hourX =
        cx + static_cast<int>(cosf(hourAngle) * 48);

    const int hourY =
        cy + static_cast<int>(sinf(hourAngle) * 48);

    _canvas.drawLine(cx, cy, hourX, hourY, TFT_WHITE);
    _canvas.drawLine(cx + 1, cy, hourX + 1, hourY, TFT_WHITE);
    _canvas.drawLine(cx, cy + 1, hourX, hourY + 1, TFT_WHITE);

    // 分針
    const int minuteX =
        cx + static_cast<int>(cosf(minuteAngle) * 70);

    const int minuteY =
        cy + static_cast<int>(sinf(minuteAngle) * 70);

    _canvas.drawLine(cx, cy, minuteX, minuteY, TFT_WHITE);
    _canvas.drawLine(cx + 1, cy, minuteX + 1, minuteY, TFT_WHITE);

    // 秒針
    const int secondX =
        cx + static_cast<int>(cosf(secondAngle) * 82);

    const int secondY =
        cy + static_cast<int>(sinf(secondAngle) * 82);

    _canvas.drawLine(cx, cy, secondX, secondY, TFT_WHITE);

    // 中心
    _canvas.fillCircle(cx, cy, 4, TFT_WHITE);

    _canvas.pushSprite(0, 0);
}
