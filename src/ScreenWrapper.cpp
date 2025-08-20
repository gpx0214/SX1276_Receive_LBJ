//
// Created by FLN1021 on 2024/2/10.
//

#include "ScreenWrapper.h"

static char buffer[32] = {'\0'};

void inline ScreenWrapper::drawIP() {
    if (!display) return;
    display->setFont(u8g2_font_NokiaSmallPlain_tf);
    display->setDrawColor(0);
    display->drawBox(0, 56, 48, 8);
    display->setDrawColor(1);
    if (!no_wifi) {
        display->drawStr(0, 64, WiFi.localIP().toString().c_str());
    } else {
        display->drawStr(0, 64, "WIFI OFF");
    }
    display->updateDisplayArea(0, 7, 6, 1);
}

void inline ScreenWrapper::drawBattery() {
    if (!display) return;
    display->setFont(u8g2_font_NokiaSmallPlain_tf);
    drawColorUTF8f(0, 7, 0, "%1.3f", battery.readVoltage() * 2);
}

void inline ScreenWrapper::drawCPUFrequency() {
    if (!display) return;
    display->setFont(u8g2_font_micro_mr);
    drawColorUTF8f(24, 5, 0, "%2d", ets_get_cpu_frequency() / 10);
}

void inline ScreenWrapper::drawStatus(bool have_sd, bool wifi_conected) {
    if (!display) return;
    display->setFont(u8g2_font_micro_mr);
    if (have_sd && wifi_conected)
        display->drawStr(36, 5, "D");
    else if (have_sd)
        display->drawStr(36, 5, "L");
    else if (wifi_conected)
        display->drawStr(36, 5, "N");
}

void inline ScreenWrapper::drawTime() {
    if (!display) return;
    display->setDrawColor(0);
    display->drawBox(48, 0, 48, 7);

    display->setDrawColor(1);
    display->setFont(u8g2_font_NokiaSmallPlain_tf);

    if (!getLocalTime(&time_info, 0))
        display->drawStr(48, 7, "NO TIME");
    else {
        // sprintf(buffer, "%2d-%02d-%02d %02d:%02d:%02d", (time_info.tm_year + 1900)%100, time_info.tm_mon + 1, time_info.tm_mday,
        //         time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
        drawColorUTF8f(48, 7, 0, "%02d:%02d:%02d", time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
    }
    display->updateDisplayArea(6, 0, 6, 1);
}

void inline ScreenWrapper::drawTemperature() {
    if (!display) return;
#ifdef HAS_RTC
    display->setFont(u8g2_font_NokiaSmallPlain_tf);
    drawColorUTF8f(86, 7, 0, "%d", (int) rtc.getTemperature()); // °C
#endif
}

void inline ScreenWrapper::drawPPM(float ppm) {
    if (!display) return;
    display->setDrawColor(0);
    display->drawBox(96, 0, 12, 7);
    display->setDrawColor(1);
    display->setFont(u8g2_font_micro_mr);
    drawColorUTF8f(96, 5, 0, "%.1f", ppm);
    display->updateDisplayArea(12, 0, 2, 1);
    //display->drawStr(80, 5, buffer);
}

void inline ScreenWrapper::drawRSSI(float rssi) {
    if (!display) return;
    // draw RSSI
    display->setFont(u8g2_font_NokiaSmallPlain_tf);
    display->setDrawColor(0);
    display->drawBox(108, 0, 20, 7);
    display->setDrawColor(1);
    sprintf(buffer, "%6.1f", rssi);
    display->drawStr(128-display->getStrWidth(buffer+1), 7, buffer+1);
    display->updateDisplayArea(14, 0, 2, 1);
}

bool ScreenWrapper::setDisplay(DISPLAY_MODEL *display_ptr) {
    this->display = display_ptr;
    if (!this->display)
        return false;

    return true;
}

void ScreenWrapper::updateInfo() {
    if (!display || !update || !enabled)
        return;

    // update top
    display->setDrawColor(0);
    display->drawBox(0, 0, 96, 7);
    display->setDrawColor(1);

    display->setFont(u8g2_font_NokiaSmallPlain_tf);
    drawTime();
    drawTemperature();
    // drawPPM(getBias(actual_frequency));
    // update bottom
    // display->setDrawColor(0);
    // display->drawBox(0, 56, 128, 8);
    // display->setDrawColor(1);
    // drawIP();
    drawStatus(sd1.status(), WiFiClass::status() == WL_CONNECTED);
    drawCPUFrequency();
    drawBattery(); // todo: Implement average voltage reading.
    if (voltage < 3.15 && !low_volt_warned) {
        Serial.printf("Warning! Low Voltage detected, %1.2fV\n", voltage);
        sd1.append("低压警告，电池电压%1.2fV\n", voltage);
        low_volt_warned = true;
    }
    display->updateDisplayArea(0, 0, 12, 1);
}

void ScreenWrapper::showInitComp() {
    if (!display)
        return;
    display->clearBuffer();
    // top (0,0,128,8)
    display->setFont(u8g2_font_NokiaSmallPlain_tf);
    drawTime();
    drawTemperature();
    // bottom (0,56,128,8)
    display->setFont(u8g2_font_NokiaSmallPlain_tf);
    drawIP();
    drawStatus(have_sd, WiFiClass::status() == WL_CONNECTED);
    drawCPUFrequency();
    drawBattery();
    display->sendBuffer();
}

void ScreenWrapper::pword(const char *msg, int xloc, int yloc) {
    int dspW = display->getDisplayWidth();
    int strW = 0;
    char glyph[2];
    glyph[1] = 0;
    for (const char *ptr = msg; *ptr; *ptr++) {
        glyph[0] = *ptr;
        strW += display->getStrWidth(glyph);
        ++strW;
        if (xloc + strW > dspW) {
            int sxloc = xloc;
            while (msg < ptr) {
                glyph[0] = *msg++;
                xloc += display->drawStr(xloc, yloc, glyph);
            }
            strW -= xloc - sxloc;
            yloc += display->getMaxCharHeight();
            xloc = 0;
        }
    }
    while (*msg) {
        glyph[0] = *msg++;
        xloc += display->drawStr(xloc, yloc, glyph);
    }
}

void ScreenWrapper::showSTR(const String &str) {
    if (!display)
        return;
    display->setFontMode(0);
    display->setFont(u8g2_font_NokiaSmallPlain_tf);
    pword(str.c_str(), 0, 7);
    display->setFontMode(1);
    display->updateDisplayArea(0, 0, 6, 1);
}


void ScreenWrapper::drawColorUTF8(uint16_t x, uint16_t y, bool flag, const char* s) {
    // display->drawButtonUTF8(x, y, flag?U8G2_BTN_INV:0, 0, 0, 0, s);
    int width = display->drawUTF8(x, y, s);
    if (flag) {
        display->setDrawColor(2);
        display->drawBox(x, y-8-1, display->getUTF8Width(s), 8+2);
        display->setDrawColor(1);
    }
}

void ScreenWrapper::drawColorUTF8f(uint16_t x, uint16_t y, bool flag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args); // 格式化字符串
    va_end(args);

    // display->drawButtonUTF8(x, y, flag?U8G2_BTN_INV:0, 0, 0, 0, s);
    display->setFontMode(0);
    int width = display->drawUTF8(x, y, buffer);
    display->setFontMode(1);
    if (flag) {
        display->setDrawColor(2);
        display->drawBox(x, y-8-1, display->getUTF8Width(buffer), 8+2);
        display->setDrawColor(1);
    }
}

int32_t countleft(const char* s, const char c) {
    int8_t trim = 0;
    for (int ii = 0; s[ii] != '\0'; ii++) {
        if(s[ii] == c) {
            trim++;
        } else {
            break;
        }
    }
    return trim;
}

#define LINENUM 6

static int line_cnt = 0;
static unsigned int line_type = 0; // bit1 need clean
void ScreenWrapper::showLBJ_V2(const PagerClient::poc32 &poc32, const struct lbj_data &l, const struct rx_info &rxInfo) {
    int offset = line_cnt%LINENUM*19/2;
    int lines = 2; // 需要刷新的行数
    display->setDrawColor(0);
    if (line_cnt%LINENUM > (LINENUM-2)) {
        display->drawBox(0, offset+2*19/2+7, 128, 10);
        display->updateDisplayArea(0, 6, 16, 2);
        line_cnt+=1;
        line_type<<=1;
        offset = line_cnt%LINENUM*19/2;
    }
    if ((line_cnt%LINENUM < (LINENUM-2)) && (line_type & (1 << (LINENUM-2-1)))) { // 0b1000
        lines+=1;
    }
    display->drawBox(0, offset+7, 128, lines*19/2+1);
    display->setDrawColor(1);
    // line 1
    int trim = countleft(l.lbj_class, ' ');

    display->setFont(u8g2_font_6x13_mf);
    drawColorUTF8(8+6*trim, offset+7+9, poc32.EbitEven(5,5) > 2, l.lbj_class+trim);
    display->setFont(u8g2_font_wqy12_t_gb2312a);
    drawColorUTF8(20, offset+7+8+1, poc32.EbitEven(1,1) > 2, l.train);
    drawColorUTF8(20+6*6, offset+7+8+1, poc32.EbitEven(2,2) > 2, l.speed);
    drawColorUTF8(20+6*10, offset+7+8+1, poc32.EbitEven(3,3) > 2, l.position);
    display->setFont(u8g2_font_wqy12_t_gb2312a);
    drawColorUTF8(116, offset+7+9, poc32.EbitEven(4,4) > 2, function_code_map[l.direction]); // poc32.EbitEven(0,0) > 2 || poc32.EbitEven(4,4) > 2

    // draw RSSI
    display->setFont(u8g2_font_micro_mr);
    sprintf(buffer, "%4.0f", rxInfo.rssi);
    drawColorUTF8(0, offset+7+5, 0, buffer+1 + (trim<1?1:0));
    drawColorUTF8f(0, offset+7+10, 0, "%.0f", getBias(actual_frequency)*10);

    // line 2
    if (get_loco_type(l.loco)[0]) {
        display->setFont(u8g2_font_profont12_mf);
        sprintf(buffer, "%7s", get_loco_type(l.loco));
        trim = countleft(buffer, ' ');
        drawColorUTF8(0+6*trim, offset+7+18, poc32.EbitEven(5,6) > 2, buffer+trim);
    } else {
        display->setFont(u8g2_font_helvR08_tf);
        strncpy(buffer, l.loco, 3); buffer[3] = '\0';
        drawColorUTF8(26, offset+7+18, poc32.EbitEven(5,6) > 2, buffer);
    }
    display->setFont(u8g2_font_helvR08_tf);
    display->drawUTF8(44, offset+7+18, l.loco+3);
    display->setFont(u8g2_font_6x10_mf);
    display->drawUTF8(74, offset+7+18, l.loco_end);

    display->setDrawColor(2);
    if (poc32.EbitEven(6,6) > 2) display->drawBox(44, offset+7+10-1, 6*3, 8+2);
    if (poc32.EbitEven(7,7) > 2) display->drawBox(44+6*3, offset+7+10-1, 6*3, 8+2);
    display->setDrawColor(1);

    display->setFont(u8g2_font_wqy12_t_gb2312a);
    gbk2utf8(buffer, l.route, 9);
    drawColorUTF8(80, offset+7+18, poc32.EbitEven(8,10) > 2, buffer); // 7-8 8-9 9-10 10

    // bottom
    if ('0' <= l.lon[0] && l.lon[0] <= '9' && '0' <= l.lat[0] && l.lat[0] <= '9') {
        display->setDrawColor(0);
        display->drawBox(0, 5, 86, 2);
        display->setDrawColor(1);
        display->setFontMode(0);
        display->setFont(u8g2_font_micro_mr);
        drawColorUTF8(0, 5, poc32.EbitEven(11,12) > 2, l.lon);
        drawColorUTF8(4*11, 5, poc32.EbitEven(13,14) > 2, l.lat);
        display->setFontMode(1);
    }

    drawTemperature();
    drawPPM(rxInfo.ppm);
    drawRSSI(rxInfo.rssi);
    // display->sendBuffer();
    display->updateDisplayArea(0, 0, 16, 1);
    display->updateDisplayArea(0, (offset+7)/8, 16, (7+(line_cnt%LINENUM+lines)*19/2-1)/8 - (offset+7)/8 + 1);

    line_cnt+=2;
    line_type<<=2;
    line_type+=0x01;
}

void ScreenWrapper::showLBJ_V1(const PagerClient::poc32 &poc32, const struct lbj_data &l, const struct rx_info &rxInfo) {
    int offset = line_cnt%LINENUM*19/2;
    int lines = 1; // 需要刷新的行数
    display->setDrawColor(0);
    if ((line_cnt%LINENUM < (LINENUM-1)) && (line_type & (1 << (LINENUM-1-1)))) {  // 0b10000
        lines+=1;
    }
    display->drawBox(0, offset+7, 128, lines*19/2+1);
    display->setDrawColor(1);
    // line 1
    display->setFont(u8g2_font_wqy12_t_gb2312a);
    drawColorUTF8(20, offset+7+8+1, poc32.EbitEven(1,1) > 2, l.train);
    drawColorUTF8(20+6*6, offset+7+8+1, poc32.EbitEven(2,2) > 2, l.speed);
    drawColorUTF8(20+6*10, offset+7+8+1, poc32.EbitEven(3,3) > 2, l.position);
    display->setFont(u8g2_font_wqy12_t_gb2312a);
    drawColorUTF8(116, offset+7+9, poc32.EbitEven(0,0) > 2, function_code_map[l.direction]); // poc32.EbitEven(0,0) > 2 || poc32.EbitEven(4,4) > 2,

    // draw RSSI
    display->setFont(u8g2_font_micro_mr);
    sprintf(buffer, "%4.0f", rxInfo.rssi);
    display->drawStr(0, offset+7+5, buffer+1);
    drawColorUTF8f(0, offset+7+10, 0, "%.0f", getBias(actual_frequency)*10);

    // display->setDrawColor(0);
    // display->drawBox(0, 0, 80, 7);
    // display->setDrawColor(1);
    // drawTime();
    // drawBattery();
    // drawTemperature();
    updateInfo();
    drawPPM(rxInfo.ppm);
    drawRSSI(rxInfo.rssi);
    display->updateDisplayArea(12, 0, 4, 1);
    // display->updateDisplayArea(0, 0, 16, 1);

    display->updateDisplayArea(0, (offset+7)/8, 16, (7+(line_cnt%LINENUM+lines)*19/2-1)/8 - (offset+7)/8 + 1);

    line_cnt+=1;
    line_type<<=1;
}

void ScreenWrapper::showLBJ_time(const PagerClient::poc32 &poc32, const struct lbj_data &l, const struct rx_info &rxInfo) {
    display->setDrawColor(0);
    display->drawBox(48, 0, 48, 7);
    display->setDrawColor(1);
    display->setFont(u8g2_font_NokiaSmallPlain_tf);
    // top
    display->drawUTF8(48, 7, l.train); // 00:00

    drawBattery();
    drawTemperature();
    drawPPM(rxInfo.ppm);
    drawRSSI(rxInfo.rssi);
    display->updateDisplayArea(0, 0, 16, 1);
}

void ScreenWrapper::showLBJ(const PagerClient::poc32 &poc32, const lbj_data &l, const rx_info &r) {
    if (!display || !update || !enabled)
        return;

    if (l.type == 1)
        showLBJ_V1(poc32, l, r);
    else if (l.type == 2) {
        showLBJ_V2(poc32, l, r);
    } else if (l.type == 8) {
        showLBJ_time(poc32, l, r);
    } else {
        showSTR("NO ADDR");
    }
}

void ScreenWrapper::showLBJ(const PagerClient::poc32 &poc32, const struct lbj_data &l, const struct rx_info &r, 
    uint32_t id) {
    if (!display)
        return;
    if (update_top)
        update_top = false;

    line_cnt=4;
    line_type=0;

    display->setDrawColor(0);
    display->drawBox(0, 32, 128, 13);
    display->setFont(u8g2_font_NokiaSmallPlain_tf);
    display->setDrawColor(1);
    display->setFontMode(0);
    drawColorUTF8f(0, 38+7, 0, r.date_time_str);
    drawColorUTF8f(80, 38+7, 0, "%8u", id);
    display->setFontMode(1);
    display->updateDisplayArea(0, 4, 16, 1);
    // display->updateDisplayArea(0, 0, 16, 1);

    if (l.type == 1) {
        display->setDrawColor(0);
        display->drawBox(0, 54, 128, 10);
        display->setDrawColor(1);
        showLBJ_V1(poc32, l, r);
        display->updateDisplayArea(0, 7, 16, 1);
    } else if (l.type == 2) {
        showLBJ_V2(poc32, l, r);
    } else if (l.type == 8) {
        showLBJ_time(poc32, l, r);
    }
}

void ScreenWrapper::resumeUpdate() {
    update_top = true;
    updateSleepTimestamp();
}

void ScreenWrapper::showSelectedLBJ(aPreferences *flash_cls, int8_t bias) {
    PagerClient::poc32 poc32;
    lbj_data lbj;
    rx_info rx;
    uint32_t id = 0;
    for (int i=0; i<100; i++) {
        bool flag = flash_cls->retrieve(&poc32, &lbj, &rx, &id, bias);
        printDataTelnet(poc32, lbj, rx);
        if (bias == 0) break;
        if (flag && 
            '0' <= lbj.train[4] && lbj.train[4] <= '9' && 
            '0' <= lbj.speed[2] && lbj.speed[2] <= '9') { //  && '0' <= lbj.position[5] && lbj.position[5] <= '9'
            break;
        }
        if (i % 5 == 0) {
            drawColorUTF8f(80, 38+7, 0, "%8u", id);
            display->updateDisplayArea(12, 4, 3, 2);
        }
    }
    showLBJ(poc32, lbj, rx, id);
}

void ScreenWrapper::clearTop(top_sectors sector, bool sendBuffer) {
    // if (!display)
    //     return;
    bool set_color = false;
    if (display->getDrawColor() != 0) {
        set_color = true;
        display->setDrawColor(0);
    }
    switch (sector) {
        case TOP_SECTOR_TIME:
            display->drawBox(0, 0, 79, 8);
            break;
        case TOP_SECTOR_TEMPERATURE:
            display->drawBox(80, 0, 98, 8);
            break;
        case TOP_SECTOR_RSSI:
            display->drawBox(99, 0, 128, 8);
            break;
        case TOP_SECTOR_ALL:
            display->drawBox(0, 0, 128, 8);
            break;
    }
    if (set_color)
        display->setDrawColor(1);
    if (sendBuffer)
        display->sendBuffer();
}

void ScreenWrapper::clearCenter(bool sendBuffer) {
    bool set_color = false;
    if (display->getDrawColor() != 0) {
        set_color = true;
        display->setDrawColor(0);
    }

    display->drawBox(0, 8, 128, 48);

    if (set_color)
        display->setDrawColor(1);
    if (sendBuffer)
        display->sendBuffer();
}

void ScreenWrapper::clearBottom(bottom_sectors sector, bool sendBuffer) {
    bool set_color = false;
    if (display->getDrawColor() != 0) {
        set_color = true;
        display->setDrawColor(0);
    }
    switch (sector) {
        case BOTTOM_SECTOR_IP:
            display->drawBox(0, 56, 72, 8);
            break;
        case BOTTOM_SECTOR_PPM:
            display->drawBox(73, 56, 15, 8); // 73->88
            break;
        case BOTTOM_SECTOR_IND:
            display->drawBox(89, 56, 6, 8); // 89->95
            break;
        case BOTTOM_SECTOR_CPU:
            display->drawBox(96, 56, 11, 8); // 96->107
            break;
        case BOTTOM_SECTOR_BAT:
            display->drawBox(108, 56, 20, 8); // 108->128
            break;
        case BOTTOM_SECTOR_ALL:
            display->drawBox(0, 56, 128, 8);
            break;
    }
    if (set_color)
        display->setDrawColor(1);
    if (sendBuffer)
        display->sendBuffer();
}

void ScreenWrapper::clearAll() {
    display->clearBuffer();
}

void ScreenWrapper::setFlash(aPreferences *flash_cls) {
    flash = flash_cls;
}

void ScreenWrapper::showSelectedLBJ(int8_t bias) {
    showSelectedLBJ(flash, bias);
}

void ScreenWrapper::showInfo(int8_t page) {
    if (!display)
        return;
    if (page > 3 || page < 1)
        return;

    PagerClient::poc32 poc32;
    lbj_data lbj;
    rx_info rx;
    uint32_t id = 0;
    if (!flash->retrieve(&poc32, &lbj, &rx, &id, 0)) {
        return;
    }

    /* Standard format of cache:
     * 条目数,电压,系统时间,温度,日期,时间,type,train,direction,speed,position,time,loco_type,lbj_class,loco,route,
     * lon,lat,rssi,fer,ppm,id
     */
    clearAll();
    display->setFont(FONT_12_GB2312);
    display->drawUTF8(0, 12, "接收信息");
    drawColorUTF8f(118, 12, 0, "%d", page);
    display->drawHLine(0, 14, 128);

    switch (page) {
        case 1: {
            drawTime();
            display->setFont(u8g2_font_NokiaSmallPlain_tf);
            drawColorUTF8f(0, 24, 0, "%1.3fV mem %dB id %04d", battery.readVoltage() * 2, esp_get_free_heap_size(), flash->getID());
            drawColorUTF8f(0, 32, 0, "%d start %6llu.%03llus\n", flash->getStartTime(), millis64()/1000, millis64()%1000);
            drawColorUTF8f(0, 40, 0, "0x%02x %s\n", esp_reset_reason(), printResetReason(esp_reset_reason())+8);
            display->setFont(FONT_12_GB2312);
            drawColorUTF8f(0, 52, 0, "测量频偏:%.0fHz %.2fppm", rx.fer, rx.ppm); // 测量频偏 设定频偏
            if (!no_wifi) {
                display->drawUTF8(0, 64, WiFi.localIP().toString().c_str());
            }
            drawColorUTF8f(96, 64, 0, "%.2fG", cardSize / 1024 / 1024 / 1024.0);
            break;
        }
        case 2: {
            // display->setFont(u8g2_font_NokiaSmallPlain_tf);
            // for (int i=0; i<16 && i<poc32.word_idx; i++) {
            //     strncpy(buffer, poc32.text+5*i, 5);
            //     buffer[5] = '\0';
            //     drawColorUTF8f(0+30*(i%4), 40+8*(i/4), poc32.Ebit(i,i)>=2, "%s", buffer);
            // }
            // display->setFont(FONT_12_GB2312);
            break;
        }
        case 3: {
            break;
        }
        default:
            break;
    }
    display->sendBuffer();
}

void ScreenWrapper::pwordUTF8(const String &msg, int xloc, int yloc, int xmax, int ymax) {
    int Width = xmax - xloc;
    int Height = ymax - yloc;
    int StrW = display->getUTF8Width(msg.c_str());
    int8_t CharHeight = display->getMaxCharHeight();
    auto lines = Height / CharHeight;
    // Serial.printf("[D] lines %d, Height %d, CharH %d\n",lines,Height,CharHeight);

    String str = msg;
    for (int i = 0, j = yloc; i <= lines; ++i, j += CharHeight) {
        auto c = str.length();
        while (StrW > Width) {
            StrW = display->getUTF8Width(str.substring(0, c).c_str());
            --c;
        }
        // Serial.printf("[D] %d, %d\n",xloc,j);
        if (c == str.length()) {
            display->drawUTF8(xloc, j, str.substring(0, c).c_str());
            break;
        }
        display->drawUTF8(xloc, j, str.substring(0, c + 1).c_str());
        str = str.substring(c - 1, str.length());
        // Serial.println("[D] " + str);
    }

    // display->sendBuffer();
}

bool ScreenWrapper::isEnabled() const {
    return enabled;
}

void ScreenWrapper::setEnable(bool is_enable) {
    // if (is_enable)
    //     display->setPowerSave(false);
    display->setPowerSave(!is_enable);
    enabled = is_enable;
}

bool ScreenWrapper::isAutoSleep() const {
    return auto_sleep;
}

void ScreenWrapper::setSleep(bool is_sleep) {
    if (!auto_sleep || !enabled)
        return;
    display->setPowerSave(is_sleep);
    sleep = is_sleep;
}

bool ScreenWrapper::isSleep() const {
    return sleep;
}

void ScreenWrapper::autoSleep() {
    // if (!update_top || !update)
    //     return;
    if (millis64() - last_operation_time > AUTO_SLEEP_TIMEOUT && !isSleep()) {
        setSleep(true);
        // updateSleepTimestamp();
    }
}

void ScreenWrapper::updateSleepTimestamp() {
    last_operation_time = millis64();
}
