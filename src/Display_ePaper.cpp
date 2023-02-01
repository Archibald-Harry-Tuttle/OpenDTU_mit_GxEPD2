#include "Display_ePaper.h"
#include <NetworkSettings.h>
#include "imagedata.h"

static const int spiClk = 1000000; // 1 MHz

#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
SPIClass hspi(HSPI);
#endif

std::map<DisplayType_t, std::function<GxEPD2_GFX *(uint8_t, uint8_t, uint8_t, uint8_t)>> _ePaperTypes = {
    // DEPG0150BN 200x200, SSD1681, TTGO T5 V2.4.1
    {DisplayType_t::ePaper154, [](uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY)
     { return new GxEPD2_BW<GxEPD2_150_BN, GxEPD2_150_BN::HEIGHT>(GxEPD2_150_BN(_CS, _DC, _RST, _BUSY)); }},
    // GDEW027C44   2.7 " b/w/r 176x264, IL91874
    {DisplayType_t::ePaper27, [](uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY)
     { return new GxEPD2_3C<GxEPD2_270c, GxEPD2_270c::HEIGHT>(GxEPD2_270c(_CS, _DC, _RST, _BUSY)); }},
};

DisplayEPaperClass::DisplayEPaperClass()
{
}

DisplayEPaperClass::~DisplayEPaperClass()
{
    delete _display;
}

void DisplayEPaperClass::init(DisplayType_t type, uint8_t _CS, uint8_t _DC, uint8_t _RST, uint8_t _BUSY, uint8_t _SCK, uint8_t _MOSI)
{
    if (type > DisplayType_t::None)
    {
        Serial.begin(115200);
        auto constructor = _ePaperTypes[type];
        _display = constructor(_CS, _DC, _RST, _BUSY);
        hspi.begin(_SCK, _BUSY, _MOSI, _CS);

#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
        _display->epd2.selectSPI(hspi, SPISettings(4000000, MSBFIRST, SPI_MODE0));
#endif
        _display->init(115200, true, 2, false);
        _display->setRotation(2);
        _display->setTextColor(GxEPD_BLACK);
        _display->setFullWindow();
        _display->setFont(&FreeSans9pt7b);

        // Logo
        _display->fillScreen(GxEPD_BLACK);
        _display->drawBitmap(0, 0, AhoyLogo, 200, 200, GxEPD_WHITE);
        while (_display->nextPage())
            ;

        // Bildschirm loeschen
        delay(2000);
        _display->fillScreen(GxEPD_WHITE);
        while (_display->nextPage())
            ;

        // Wifi Status ausgeben
        headlineIP();
        Serial.println("Display initializiert: ");
    }
}
//***************************************************************************
void DisplayEPaperClass::headlineIP()
{
    _changed = true;
    _display->setFont(&FreeSans9pt7b);

    _display->setTextColor(GxEPD_WHITE);
    _display->setPartialWindow(0, 0, _display->width(), headline_tbh + 3);
    do
    {
        _display->fillScreen(GxEPD_BLACK);
        _display->setCursor(0, headline_tbh + 1);
        if (NetworkSettings.isConnected() == true)
        {
            snprintf(_fmtText, sizeof(_fmtText), "IP: %s", NetworkSettings.localIP().toString().c_str());
            _display->println(_fmtText);
        }
        else
        {
            _display->println("Wifi nicht verbunden");
        }
    } while (_display->nextPage());
}

void DisplayEPaperClass::actualPowerPaged(float _totalPower, float _totalYieldDay, float _totalYieldTotal)
{
    _changed = true;
    _display->setFont(&FreeSans24pt7b);

    _display->setTextColor(GxEPD_BLACK);
    _display->setPartialWindow(0, headline_tbh + 3, _display->width(), _display->height() - (headline_tbh * 2 + 6));
    do
    {
        _display->fillScreen(GxEPD_WHITE);
        _display->setCursor(10, 65);
        if (_totalPower > 9999)
        {
            snprintf(_fmtText, sizeof(_fmtText), "%.1f kW", (_totalPower / 10000));
        }
        else if ((_totalPower > 0) && (_totalPower <= 9999))
        {
            snprintf(_fmtText, sizeof(_fmtText), "%.0f W", _totalPower);
        }
        else
        {
            snprintf(_fmtText, sizeof(_fmtText), "Offline");
        }
        _display->println(_fmtText);

        _display->setFont(&FreeSans12pt7b);
        snprintf(_fmtText, sizeof(_fmtText), "today: %4.0f Wh", _totalYieldDay);
        _display->println(_fmtText);

        snprintf(_fmtText, sizeof(_fmtText), "total: %.1f kWh", _totalYieldTotal);
        _display->println(_fmtText);

    } while (_display->nextPage());
}

void DisplayEPaperClass::lastUpdatePaged()
{
    _display->setFont(&FreeSans9pt7b);

    _display->setTextColor(GxEPD_WHITE);
    _display->setPartialWindow(0, _display->height() - (headline_tbh + 3), _display->width(), headline_tbh + 3);
    do
    {
        _display->fillScreen(GxEPD_BLACK);
        _display->setCursor(0, _display->height() - 3);

        time_t now = time(nullptr);
        strftime(_fmtText, sizeof(_fmtText), "%a %d.%m.%Y %H:%M", localtime(&now));
        _display->println(_fmtText);

    } while (_display->nextPage());
}

void DisplayEPaperClass::loop(float totalPower, float totalYieldDay, float totalYieldTotal, uint8_t isprod)
{
    if (NetworkSettings.isConnected() == true)
    {
        headlineIP();
    }

    actualPowerPaged(totalPower, totalYieldDay, totalYieldTotal);

    if ((isprod > 0) && (_changed))
    {
        _changed = false;
        lastUpdatePaged();
    }

    _display->powerOff();
}

DisplayEPaperClass DisplayEPaper;