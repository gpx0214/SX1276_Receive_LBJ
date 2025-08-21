//
// Created by FLN1021 on 2024/2/10.
//

#ifndef PAGER_RECEIVE_SCREENWRAPPER_H
#define PAGER_RECEIVE_SCREENWRAPPER_H

#include <cstdint>
#include "networks.hpp"
#include "LBJ.hpp"
#include "customfont.h"
#include "aPreferences.h"

#define AUTO_SLEEP_TIMEOUT 600000
#define AUTO_SLEEP true
#ifdef HAS_DISPLAY

enum top_sectors {
    TOP_SECTOR_TIME,
    TOP_SECTOR_TEMPERATURE,
    TOP_SECTOR_RSSI,
    TOP_SECTOR_ALL
};

enum bottom_sectors {
    BOTTOM_SECTOR_IP,
    BOTTOM_SECTOR_PPM,
    BOTTOM_SECTOR_IND,
    BOTTOM_SECTOR_CPU,
    BOTTOM_SECTOR_BAT,
    BOTTOM_SECTOR_ALL
};

class ScreenWrapper {
public:
    bool setDisplay(DISPLAY_MODEL *display_ptr);

    void setFlash(aPreferences *flash_cls);

    void drawIP();
    void drawTemperature();
    void drawCPUFrequency();
    void drawBattery();
    void drawPPM(float ppm);
    void drawRSSI(float rssi);
    void drawTime();
    void drawStatus(bool have_sd, bool wifi_conected);

    void updateInfo();

    void showInitComp();

    void showSTR(const String &str);

    void showLBJ0(const struct lbj_data &l, const struct rx_info &r);

    void showLBJ1(const struct lbj_data &l, const struct rx_info &r);

    void showLBJ2(const struct lbj_data &l, const struct rx_info &r);

    void drawColorUTF8(uint16_t x, uint16_t y, bool flag, const char* s);
    void drawColorUTF8f(uint16_t x, uint16_t y, bool flag, const char* format, ...);

    void showLBJ_V2(const PagerClient::poc32 &poc32, const struct lbj_data &l, const struct rx_info &r);

    void showLBJ_V1(const PagerClient::poc32 &poc32, const struct lbj_data &l, const struct rx_info &r);

    void showLBJ_time(const PagerClient::poc32 &poc32, const struct lbj_data &l, const struct rx_info &r);

    void showLBJ(const PagerClient::poc32 &poc32, const struct lbj_data &l, const struct rx_info &r);

    void showLBJ(const PagerClient::poc32 &poc32, const struct lbj_data &l, const struct rx_info &r, 
        uint32_t id);

    void showSelectedLBJ(int8_t bias);

    void showSelectedLBJ(aPreferences *flash_cls, int8_t bias);

    void showInfo(int8_t page = 1);

    void resumeUpdate();

    bool isAutoSleep() const;

    void autoSleep();

    void updateSleepTimestamp();

    void clearTop(top_sectors sector, bool sendBuffer);

    void clearCenter(bool sendBuffer);

    void clearBottom(bottom_sectors sector, bool sendBuffer);

    void clearAll();

    bool isEnabled() const;

    void setEnable(bool is_enable);

    void setSleep(bool is_sleep);

    bool isSleep() const;

protected:
    void pwordUTF8(const String &msg, int xloc, int yloc, int xmax, int ymax);

    bool update = true;
    DISPLAY_MODEL *display = nullptr;
    aPreferences *flash{};
    bool enabled = true;
    bool auto_sleep = AUTO_SLEEP;
    bool sleep = false;
    uint64_t last_operation_time = 0;
    bool draw_epi = true;

private:
    void pword(const char *msg, int xloc, int yloc);

    bool low_volt_warned = false;
    bool update_top = true;
    const uint8_t *font_12_alphanum = u8g2_font_profont12_custom_tf;
    const uint8_t *font_15_alphanum_bold = u8g2_font_spleen8x16_mu;
    const uint8_t *font_15_alphanum = u8g2_font_profont15_mr;
};

#endif
#endif //PAGER_RECEIVE_SCREENWRAPPER_H
