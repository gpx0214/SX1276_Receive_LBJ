//
// Created by FLN1021 on 2023/9/2.
//

#include "networks.hpp"

#include "LBJ.hpp"

/* ------------------------------------------------ */
ESPTelnet telnet;
IPAddress ip;
uint16_t port = 23;

const char *time_zone = "CST-8";
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";

struct tm time_info{};
int last_hour = 0;

extern uint32_t start_cnt;

bool isConnected() {
    return (WiFiClass::status() == WL_CONNECTED);
}

bool connectWiFi(const char *ssid, const char *password, int max_tries, int pause) {
    int i = 0;
    WiFiClass::mode(WIFI_STA);
    WiFi.begin(ssid, password);
    do {
        delay(pause);
        Serial.print(".");
    } while (!isConnected() && i++ < max_tries);
    if (isConnected())
        Serial.print("SUCCESS.");
    else
        Serial.print("FAILED.");
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    return isConnected();
}

void silentConnect(const char *ssid, const char *password) {
    WiFiClass::mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.begin(ssid, password);
}

void changeCpuFreq(uint32_t freq_mhz) {
    /*TODO: The wireless function is giving me a headache.
     * changing frequency during wifi connected may trigger disconnection, while giving a restart WILL trigger
     * a disconnection and cause the loop to stuck up to 1000 MS!
     * A WiFi.setSleep(true) workaround was provided on https://github.com/espressif/arduino-esp32/issues/7240
     * currently unverified and may cause other problems.
     * The WiFi restart workaround currently using while wifi disconnected may stuck the main loop from 20ms up
     * to 1000ms, which needs further improvements. */
    if (ets_get_cpu_frequency() == freq_mhz) {
        return;
    }
    if (isConnected() || no_wifi) {
        if (ets_get_cpu_frequency() != freq_mhz) {
            setCpuFrequencyMhz(freq_mhz);
            if (no_wifi) {
                auto timer = millis64();
                WiFiClass::mode(WIFI_OFF);
                WiFi.setSleep(true);
                // Serial.printf("[D] Switch to 80MHz, WIFI OFF [%llu] \n", millis64() - timer);
            }
        }
    } else {
        // Serial.println("[D] CALL WIFI OFF");
        auto timer = millis64();
        WiFiClass::mode(WIFI_OFF);
        Serial.printf("[D] WIFI OFF [%llu] \n", millis64() - timer);
        if (ets_get_cpu_frequency() != freq_mhz)
            setCpuFrequencyMhz(freq_mhz);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        // Serial.println("[D] WIFI BEGIN");
        WiFiClass::mode(WIFI_MODE_STA);
        // Serial.println("[D] WIFI STA");
//        WiFi.setAutoReconnect(true);
//        WiFi.persistent(true);
    }
}

/* ------------------------------------------------ */

void timeAvailable(struct timeval *t) {
    tm ti2{};
    Serial.printf("[SNTP] Got time adjustment from NTP!");
    getLocalTime(&time_info);
    Serial.println(&time_info, " %Y-%m-%d %H:%M:%S");
#ifdef HAS_RTC
    if (have_rtc) {
        getLocalTime(&time_info);
        rtc.adjust(rtcLibtoC(time_info));
        // rtc.setDateTime(time_info);
        auto timer = esp_timer_get_time();
        ti2 = rtcLibtoC(rtc.now());
        // rtc.getDateTime(ti2);
        Serial.print(&ti2, "[eRTC] Time set to %Y-%m-%d %H:%M:%S ");
        Serial.printf("[%lld]\n", esp_timer_get_time() - timer);
    }
#endif
}

//void printLocalTime()
//{
//    struct tm timeinfo;
//    if(!getLocalTime(&timeinfo)){
//        Serial.println("No time available (yet)");
//        return;
//    }
//    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
//}

void timeSync(struct tm &time) {
    time_t t = mktime(&time);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, nullptr);
    setenv("TZ", time_zone, 1);
    tzset();
}

void now_time_str(char* time_str) {
    struct tm now{};
    getLocalTime(&now, 1);
    if (now.tm_year + 1900 > 2001) {
        sprintf(time_str, "%02d-%02d-%02d %02d:%02d:%02d", 
            (now.tm_year + 1900) % 100, now.tm_mon + 1, now.tm_mday, 
            now.tm_hour, now.tm_min, now.tm_sec);
    } else {
        sprintf(time_str, "[%6llu.%03llu]", 
            esp_timer_get_time()/1000000, esp_timer_get_time()/1000%1000);
    }
}

char *fmtime(const struct tm &time) {
    static char buffer[20];
    sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour,
            time.tm_min, time.tm_sec);
    return buffer;
}

char *fmtms(uint64_t ms) {
    static char buffer[40];
    if (ms < 60000) // seconds
        sprintf(buffer, "%.3f Seconds", (double) ms / 1000);
    else if (ms < 3600000) // minutes
        sprintf(buffer, "%llu Minutes %.3f Seconds", ms / 60000, (double) (ms % 60000) / 1000);
    else if (ms < 86400000) // hours
        sprintf(buffer, "%llu Hours %llu Minutes %.3f Seconds", ms / 3600000, ms % 3600000 / 60000,
                (double) (ms % 3600000 % 60000) / 1000);
    else // days
        sprintf(buffer, "%llu Days %llu Hours %llu Minutes %.3f Seconds", ms / 86400000, ms % 86400000 / 3600000,
                ms % 86400000 % 3600000 / 60000, (double) (ms % 86400000 % 3600000 % 60000) / 1000);

    return buffer;
}

#ifdef HAS_RTC

tm rtcLibtoC(const DateTime &datetime) {
    tm time{};
    time.tm_year = datetime.year() - 1900;
    time.tm_mon = datetime.month() - 1;
    time.tm_mday = datetime.day();
    time.tm_wday = datetime.dayOfTheWeek();
    time.tm_yday = 0;
    time.tm_hour = datetime.hour();
    time.tm_min = datetime.minute();
    time.tm_sec = datetime.second();
    time.tm_isdst = 0;
    return time;
}

DateTime rtcLibtoC(const tm &ctime) {
    DateTime now(ctime.tm_year + 1900, ctime.tm_mon + 1, ctime.tm_mday, ctime.tm_hour, ctime.tm_min, ctime.tm_sec);
    return now;
}

#endif

static const char *reset_reason_str[] = {
    "ESP_RST_UNKNOWN, Reset reason can not be determined",
    "ESP_RST_POWERON, Reset due to power-on event",
    "ESP_RST_EXT, Reset by external pin (not applicable for ESP32)",
    "ESP_RST_SW, Software reset via esp_restart",
    "ESP_RST_PANIC, Software reset due to exception/panic",
    "ESP_RST_INT_WDT, Reset (software or hardware) due to interrupt watchdog",
    "ESP_RST_TASK_WDT, Reset due to task watchdog",
    "ESP_RST_WDT, Reset due to other watchdogs",
    "ESP_RST_DEEPSLEEP, Reset after exiting deep sleep mode",
    "ESP_RST_BROWNOUT, Brownout reset (software or hardware)",
    "ESP_RST_SDIO, Reset over SDIO",

};

const char * printResetReason(esp_reset_reason_t reset) {
    if (0 <= reset && reset < LEN(reset_reason_str)) {
        return reset_reason_str[reset];
    }
    return "";
}

/* ------------------------------------------------ */
static char buffer[384] = {};

// (optional) callback functions for telnet events
void onTelnetConnect(String ip) {
    Serial.printf("[Telnet] %s connected\n", ip);

    telnet.println("===[ESP32 DEV MODULE TELNET SERVICE]===");
    getLocalTime(&time_info, 10);
    char timeStr[20];
    sprintf(timeStr, "%d-%02d-%02d %02d:%02d:%02d", time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday,
            time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
    telnet.printf("%d start %llu.%03llus\n\r%s\n\r", start_cnt, millis64()/1000, millis64()%1000, printResetReason(esp_reset_reason())+8);
    telnet.print("System time is ");
    telnet.print(timeStr);
    telnet.println("\n\rWelcome " + telnet.getIP());
    telnet.println("(Use ^] + q  to disconnect.)");
    telnet.println("=======================================");
    telnet.print("< ");
}

void onTelnetDisconnect(String ip) {
    Serial.print("[Telnet] ");
    Serial.print(ip);
    Serial.println(" disconnected");
}

void onTelnetReconnect(String ip) {
    Serial.print("[Telnet] ");
    Serial.print(ip);
    Serial.println(" reconnected");
}

void onTelnetConnectionAttempt(String ip) {
    Serial.print("[Telnet] ");
    Serial.print(ip);
    Serial.println(" tried to connected");
}

void upTask(void *pVoid) {
    PagerClient::poc32 poc32;
    lbj_data lbj;
    rx_info rx;
    uint32_t id = flash.getID() - 1;
    for (int i=0; i<1000; i++) {
        if (id <= 0) break;
        bool flag = flash.retrieve2(&poc32, &lbj, &rx, &id);
        id--;
        if (flag && 
            '0' <= lbj.train[4] && lbj.train[4] <= '9' && 
            '0' <= lbj.speed[2] && lbj.speed[2] <= '9' && 
            '0' <= lbj.position[5] && lbj.position[5] <= '9') {
                    
            telnet.print('\r');
            // telnet.printf("%6d ", id);
            sprintf(buffer, "%6.1f ", rx.rssi);
            telnet.print(buffer+1);
            LBJ_color(buffer, poc32, lbj, rx);
            telnet.print(buffer);
            telnet.print('\r');
        }
        if (i % 100 == 0) {
            telnet.printf("%d\r", id);
        }
    }
    vTaskDelete(nullptr);
}

void onTelnetInput(String str) {
    bool ext_call = false;
    size_t args_count = 3;
    char strc[256];
    str.toCharArray(strc, 256);
    String args[3];
    bool toolong = false;
    for (size_t i = 0, c = 0; i < 256; i++) {
        if (strc[i] == 0)
            break;
        if (strc[i] == ' ') {
            i++;
            c++;
        }
        if (c > args_count - 1) {
            toolong = true;
            break;
        }
        args[c] += strc[i];
    }
    if (!toolong) {
        if (args[0] == "test") {
            if (args[1] == "test") {
                telnet.println("> Args test.");
            } else
                telnet.println("> Unknown Command.");
        }
        if (args[0] == "log") {
            if (args[1] == "read") {
                bool valid = true;
                if (args[2].length() == 0)
                    valid = false;
                for (auto c: args[2]) {
                    if (!isDigit(c))
                        valid = false;
                }
                if (!valid)
                    telnet.println("> Invalid Format, digits only.");
                else {
                    if (sd1.status())
                        sd1.printTel(std::stoi(args[2].c_str()), telnet);
//                        telPrintLog(std::stoi(args[2].c_str()));
                    else
                        telnet.println("> Can not access SD card.");
                }
            } else if (args[1] == "status") {
                if (sd1.status())
                    telnet.println("> True.");
                else
                    telnet.println("> False.");
            } else
                telnet.println("> Unknown Command.");
        }
        if (args[0] == "u") {
            xTaskCreatePinnedToCore(upTask, "upTask", 3000,
                nullptr, 1, nullptr, ARDUINO_RUNNING_CORE);
        }
        if (args[0] == "afc") {
            if (args[1] == "off") {
                prb_count = 0;
                prb_timer = 0;
                freq_correction = false;
                telnet.println("> Frequency Correction Disabled");
                Serial.println("[Telnet] > Frequency Correction Disabled");
            } else if (args[1] == "on") {
                freq_correction = true;
                telnet.println("> Frequency Correction Enabled");
                Serial.println("[Telnet] > Frequency Correction Enabled");
            }
        }
        if (args[0] == "ppm") {
            if (args[1].length() != 0) {
                bool valid = true;
                bool point = false;
                for (auto c: args[1]) {
                    if (!isDigit(c)) {
                        if (c == '.' && !point)
                            point = true;
                        else
                            valid = false;
                    }
                }
                if (valid) {
                    ppm = std::stof(args[1].c_str());
                    tel_set_ppm = true;
                } else
                    telnet.println("> Invalid Format, float only.");
            }
        }
    }

    // checks for a certain command
    if (str == "ping") {
        telnet.println("> pong");
        Serial.println("[Telnet] > pong");
        // disconnect the client
    } else if (str == "bye") {
        telnet.println("> disconnecting you...");
        telnet.disconnectClient();
    } else if (str == "read") {
        if (sd1.status())
            telPrintLog(1000);
        else
            telnet.println("> Can not access SD card.");
    } else if (str == "bat") {
        telnet.printf("Current Battery Voltage %1.2fV\n", battery.readVoltage() * 2);
    } else if (str == "time") {
        getLocalTime(&time_info, 1);
        telnet.printf("> SYS Time %s, Up time %llu.%03llus\n", fmtime(time_info), millis64()/1000, millis64()%1000);
    } else if (str == "mem" || str == "m") {
        telnet.printf("> Mem left: %d Bytes, id %d\n", esp_get_free_heap_size(), flash.getID());
    } else if (str == "rssi") {
        give_tel_rssi = true;
        ext_call = true;
    } else if (str == "gain") {
        give_tel_gain = true;
        ext_call = true;
    } else if (str == "time") {
        telPrintf(true, "Time requested.\n");
        Serial.printf("[Telnet] > Time requested.\n");
        ext_call = true;
    } else if (str == "task time") {
        xTaskCreatePinnedToCore(timeTask, "timeTask", 2048,
                                nullptr, 1, nullptr, ARDUINO_RUNNING_CORE);
        Serial.println("[Telnet] > Thread time requested.");
    }
    if (!ext_call)
        telnet.print("< ");
}


/* ------------------------------------------------- */

void setupTelnet() {
    // passing on functions for various telnet events
    telnet.onConnect(onTelnetConnect);
    telnet.onConnectionAttempt(onTelnetConnectionAttempt);
    telnet.onReconnect(onTelnetReconnect);
    telnet.onDisconnect(onTelnetDisconnect);
    telnet.onInputReceived(onTelnetInput);

    Serial.print("[Telnet] ");
    if (telnet.begin(port)) {
        Serial.printf("running %s:%d WIFI Connection to %s established.\n", 
            WiFi.localIP().toString().c_str(), port, WIFI_SSID);
        telnet_online = true;
    } else {
        Serial.print("error.");
    }
}

/* ------------------------------------------------- */

void timeTask(void *pVoid) {
    telPrintf(true, "Thread time requested.\n");
    delay(5000);
    Serial.printf("Running on Core %d\n", xPortGetCoreID());
    telPrintf(true, "Thread time requested.\n");
    Serial.println("Thread time requested.");
    vTaskDelete(nullptr);
}

//bool ipChanged(uint16_t interval, uint64_t *timer, uint32_t *last_ip) {
//    uint32_t ip1 = WiFi.localIP();
//    if (millis64()-timer >= interval){
//        if (WiFi.localIP() != ip1){
//            return true;
//        }
//    }
//    return false;
//}

void telPrintf(bool time_stamp, const char *format, ...) { // Generated by ChatGPT.
    char buffer[128]; // 创建一个足够大的缓冲区来容纳格式化后的字符串
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args); // 格式化字符串
    va_end(args);

    // 输出到 Telnet
    if (telnet_online) { // code from Multimon-NG unixinput.c 还得是multimon-ng，chatgpt写了四五个版本都没解决。
        if (is_startline) {
            telnet.print("\r>");
            if (time_stamp && getLocalTime(&time_info, 1))
                telnet.printf("\r%d-%02d-%02d %02d:%02d:%02d>", (time_info.tm_year + 1900), time_info.tm_mon + 1,
                              time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
            is_startline = false;
        }
        telnet.print(buffer);
        if (nullptr != strchr(buffer, '\n')) {
            is_startline = true;
            telnet.print("\r<");
        }
    }
}

void telPrintLog(int chars) {
    File log = sd1.logFile('r');
    uint32_t pos, left;
    if (chars < log.size())
        pos = log.size() - chars;
    else
        pos = 0;
    left = chars - log.size();
    String line;
    if (!log.seek(pos))
        Serial.println("[SDLOG] seek failed!");
    while (log.available()) {
        line = log.readStringUntil('\n');
        if (line) {
            telnet.print(line);
            telnet.print("\n");
        } else
            telPrintf(false, "[SDLOG] Read failed!");
    }
    if (left)

        sd1.reopen();
}

void printDataSerial( const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r) {
    poc32_color(buffer, p, l, r);
    Serial.print(buffer);
    LBJ_color(buffer, p, l, r);
    Serial.print(buffer);
    return;
}

void printDataTelnet(const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r) {
    // poc32_color(buffer, p, l, r);
    // telPrintf(true, "%s", buffer);
    // LBJ_color(buffer, p, l, r);
    // telPrintf(true, "%s", buffer+14);

    // poc32_color(buffer, p, l, r);
    // telnet.print('\r');
    // telnet.print(buffer+5);
    telnet.print('\r');
    sprintf(buffer, "%6.1f ", r.rssi);
    telnet.print(buffer+1);
    LBJ_color(buffer, p, l, r);
    telnet.print(buffer);
    telnet.print('\r');
}

void appendDataLog(const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r) {
    poc32_log(buffer, p, l, r);
    sd1.appendBuffer(buffer);
    LBJ_log(buffer, p, l, r);
    sd1.appendBuffer(buffer);
    sd1.sendBufferLOG();
    return;
}

void appendDataCSV(const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r) {
    formatCSVfile(buffer, p, l, r);
    sd1.addBufferCSV(buffer);
    sd1.sendBufferCSV();
    return;
}

float getBias(float freq) {
    return (float) ((freq - TARGET_FREQ) * 1e6 / TARGET_FREQ);
}
