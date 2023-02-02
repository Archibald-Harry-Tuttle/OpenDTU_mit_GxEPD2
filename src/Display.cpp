#include "Display.h"
#include "Display_Mono.h"
#include "Display_ePaper.h"

#include <Hoymiles.h>

void DisplayClass::init(DisplayType_t _type, uint8_t MOSI_PIN, uint8_t CLK_PIN, uint8_t CS_PIN, uint8_t DC_PIN, uint8_t RST_PIN, uint8_t BUSY_PIN)
{
    /*************** HSPI Belegung *******************************
    MISO(Busy) 12   // ePaper Busy indicator (SPI MISO aquivalent)
    RST 26          // ePaper Reset switch
    DC 27           // ePaper Data/Command selection
    CS(SS) 15       // SPI Channel Chip Selection for ePaper
    SCK(CLK) 14     // SPI Channel Click
    MOSI(DIN) 13    // SPI Channel MOSI Pin
    *************************************************************/

    /************ Testdefinition start ***********/
    _type = ePaper154;
    BUSY_PIN = 12;
    RST_PIN = 26;
    DC_PIN = 27;
    CS_PIN = 15;
    CLK_PIN = 14;
    MOSI_PIN = 13;
    /************ Testdefinition ende ***********/

    _display_type = _type;
    if ((_type == PCD8544) || (_type == SSD1306) || (_type == SH1106))
    {
        Serial.println("Initialize Mono ");
        DisplayMono.init(_type, MOSI_PIN, CLK_PIN, CS_PIN, DC_PIN, RST_PIN);
        DisplayMono.enablePowerSafe = enablePowerSafe;
        DisplayMono.enableScreensaver = enableScreensaver;
        DisplayMono.contrast = contrast;
        Serial.println("Initialize Mono done");
        _period = 1000;
    }
    else if ((_type == ePaper154) || (_type == ePaper27))
    {
#if defined(ESP32)
        Serial.println("Initialize ePaper ");
        DisplayEPaper.init(_type, CS_PIN, DC_PIN, RST_PIN, BUSY_PIN, CLK_PIN, MOSI_PIN); // Type, CS, DC, RST, BUSY, SCK, MOSI
        Serial.println("Initialize ePaper done");
        _period = 10000;
#endif
    }
}

void DisplayClass::loop()
{
    if (_display_type == DisplayType_t::None)
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

        if ((_display_type == PCD8544) || (_display_type == SSD1306) || (_display_type == SH1106))
        {
            DisplayMono.loop(totalPower, totalYieldDay, totalYieldTotal, isprod);
        }
        else
        {
#if defined(ESP32)
            DisplayEPaper.loop(totalPower, totalYieldDay, totalYieldTotal, isprod);
#endif
        }
        _lastDisplayUpdate = millis();
    }
}

DisplayClass Display;