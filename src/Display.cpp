#include "Display.h"
#include "Display_Mono.h"
#include "Display_ePaper.h"

#include <Hoymiles.h>

void DisplayClass::init(DisplayType_t type, uint8_t _data, uint8_t _clk, uint8_t _cs, uint8_t _dc, uint8_t _reset, uint8_t _busy)
{
    type = ePaper154;
    _cs = 33;
    _dc = 27;
    _reset = 34;
    _busy = 14;
    _clk = 25;
    _data = 32;

    init_type = type;
    if ((type == PCD8544) || (type == SSD1306) || (type == SH1106))
    {
        Serial.println("Initialize Mono ");
        DisplayMono.init(type, _data, _clk, _cs, _dc, _reset);
        DisplayMono.enablePowerSafe = enablePowerSafe;
        DisplayMono.enableScreensaver = enableScreensaver;
        DisplayMono.contrast = contrast;
        Serial.println("Initialize Mono done");
        _period = 1000;
    }
    else if ((type == ePaper154) || (type == ePaper27))
    {
        // #define EPD_SID 32 // a.k.a. as MOSI ("Master Out Slave In") or DIN on the Waveshare module
        // #define EPD_CLK 25

        // MISO(Busy) 12
        // RST 34
        // DC 27
        // CS(SS) 33
        // SCK(CLK) 14
        // MOSI(DIN) 13

        Serial.println("Initialize ePaper ");
        DisplayEPaper.init(type, _cs, _dc, _reset, _busy, _clk, _data); // Type, CS, DC, RST, BUSY, SCK, MOSI
        Serial.println("Initialize ePaper done");
        _period = 10000;
    }
}

void DisplayClass::loop()
{
    if (init_type == DisplayType_t::None)
    {
        return;
    }

    if ((millis() - _lastDisplayUpdate) > _period)
    {
        float totalPower = 0;
        float totalYieldDay = 0;
        float totalYieldTotal = 0;

        uint8_t isprod = 0;

        for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++)
        {
            auto inv = Hoymiles.getInverterByPos(i);
            if (inv == nullptr)
            {
                continue;
            }

            if (inv->isProducing())
            {
                isprod++;
            }

            totalPower += (inv->Statistics()->getChannelFieldValue(CH0, FLD_PAC) * inv->isReachable());
            totalYieldDay += (inv->Statistics()->getChannelFieldValue(CH0, FLD_YD));
            totalYieldTotal += (inv->Statistics()->getChannelFieldValue(CH0, FLD_YT));
        }

        if ((init_type == PCD8544) || (init_type == SSD1306) || (init_type == SH1106))
        {
            DisplayMono.loop(totalPower, totalYieldDay, totalYieldTotal, isprod);
        }
        else
        {
            DisplayEPaper.loop(totalPower, totalYieldDay, totalYieldTotal, isprod);
        }
        _lastDisplayUpdate = millis();
    }
}

DisplayClass Display;