//
// Created by FLN1021 on 2023/9/2.
//

#ifndef PAGER_RECEIVE_NETWORKS_HPP
#define PAGER_RECEIVE_NETWORKS_HPP

#include <WiFi.h>
#include <ctime>
#include "esp_sntp.h"
#include "ESPTelnet.h"
// #include <RadioLib.h>
#include "sdlog.hpp"
#include "aPreferences.h"
#include "boards.hpp"
#include "LBJ.hpp"
#include "freertos/FreeRTOS.h"
#include <esp_task_wdt.h>

const long  gmtOffset_sec = 8 * 3600; // +8 hours
const int   daylightOffset_sec = 0; // no daylight offset
extern const char *time_zone;
extern const char *ntpServer1;
extern const char *ntpServer2;

extern struct tm time_info;
extern int last_hour;

// you'll have to change this!
#define WIFI_SSID       ""
#define WIFI_PASSWORD   ""
#define NETWORK_TIMEOUT 1800000 // 30 minutes

extern ESPTelnet telnet;
extern IPAddress ip;
extern uint16_t port;
extern bool is_startline;
extern SD_LOG sd1;
extern class aPreferences flash;
extern bool give_tel_rssi;
extern bool give_tel_gain;
extern bool tel_set_ppm;
extern bool no_wifi;
extern bool first_rx;
extern bool always_new;
extern float actual_frequency;
extern uint64_t prb_timer;
extern uint64_t car_timer;
extern uint32_t prb_count;
extern uint32_t car_count;
extern float ppm;
extern bool freq_correction;
extern bool telnet_online;

bool isConnected();

bool connectWiFi(const char *ssid, const char *password, int max_tries = 20, int pause = 500);

void silentConnect(const char *ssid, const char *password);

void changeCpuFreq(uint32_t freq_mhz);

void timeAvailable(struct timeval *t);

void timeSync(struct tm &time);

void now_time_str(char* time_str);

char *fmtime(const struct tm &time);

char *fmtms(uint64_t ms);

const char * printResetReason(esp_reset_reason_t reset);

void onTelnetConnect(String ip);

void onTelnetDisconnect(String ip);

void onTelnetReconnect(String ip);

void onTelnetConnectionAttempt(String ip);

void onTelnetInput(String str);

void setupTelnet();

void timeTask(void *pVoid);

//extern bool ipChanged(uint16_t interval);

void telPrintf(bool time_stamp, const char *format, ...);

void telPrintLog(int chars);

void printDataSerial(const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r);

void appendDataLog(const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r);

void printDataTelnet(const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r);

void appendDataCSV(const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r);

float getBias(float freq);

#ifdef HAS_RTC

tm rtcLibtoC(const DateTime &datetime);

DateTime rtcLibtoC(const tm &ctime);

#endif

#endif //PAGER_RECEIVE_NETWORKS_HPP
