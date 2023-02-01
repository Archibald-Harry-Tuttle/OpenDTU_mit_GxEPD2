#include "Display_ePaper.h"
#include <Hoymiles.h>
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
    _typeEPaper = type;
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

        Serial.println("Display initializiert: ");
    }
}
//***************************************************************************
void DisplayEPaperClass::loop(float totalPower, float totalYieldDay, float totalYieldTotal, uint8_t isprod)
{
    if (_typeEPaper == DisplayType_t::None)
    {
        return;
    }

    // Berechnen des Mittelwerts

    //_display->setPartialWindow(0, 0, _display->width(), 30); // definieren Sie das partielle Fenster für die Anzeige des Werts
    //_display->fillScreen(GxEPD_WHITE); // Löschen Sie den vorherigen Wert
    // Anzeige auf dem Display

    _display->firstPage();
    do
    {
        _display->setTextSize(4);
        snprintf(_fmtText, sizeof(_fmtText), "%.0f W", totalPower);
        //_display->drawBitmap(10, 10, bmp_arrow, 20, 20, GxEPD_WHITE);
        _display->println(_fmtText);
        _display->setTextSize(3);
        _display->println();
        // Heutige Produktion
        _display->println("today:");
        snprintf(_fmtText, sizeof(_fmtText), "%.0f Wh", totalYieldDay);
        _display->println(_fmtText);
        // Gesamt Produktion
        _display->println("total:");
        snprintf(_fmtText, sizeof(_fmtText), "%.1f kWh", totalYieldTotal);
        _display->println(_fmtText);
        _display->setTextSize(2);
        _display->println();
        // IP-Adresse
        _display->println("IP-Adresse:");
        _display->println(NetworkSettings.localIP().toString().c_str());
    } while (_display->nextPage());
    _lastDisplayUpdate = millis();
}

DisplayEPaperClass DisplayEPaper;