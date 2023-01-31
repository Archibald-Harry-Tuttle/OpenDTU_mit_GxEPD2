// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>

enum DisplayType_t
{
    None,
    PCD8544,
    SSD1306,
    SH1106,
    ePaper154,
    ePaper27
};

class DisplayClass
{
public:
    void init(DisplayType_t type, uint8_t data, uint8_t clk, uint8_t cs, uint8_t dc, uint8_t reset, uint8_t busy);
    void loop();

    bool enablePowerSafe;
    bool enableScreensaver;
    uint8_t contrast;
    uint16_t _period = 10000;
    uint32_t _lastDisplayUpdate = 0;

private:
    DisplayType_t init_type;
};

extern DisplayClass Display;