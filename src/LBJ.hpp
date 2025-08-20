#ifndef PAGER_RECEIVE_LBJ_HPP
#define PAGER_RECEIVE_LBJ_HPP

#include <stdint.h>
#include <string>

#include <RadioLib.h>

#include "unicon.hpp"

#include "BCH.hpp"
#include "loco.h"

#define LEN(x)  (sizeof(x)/sizeof(x[0]))

#define LBJ_INFO_ADDR 1234000
#define LBJ_INFO2_ADDR 1234002
#define LBJ_TIME_ADDR 1234008

#define TARGET_FREQ 821.2375 // MHz

#define FUNCTION_DOWN 1
#define FUNCTION_UP 3
static const char* function_code_map[] = {" 0", "下", " 2", "上", "  "};

#define POCDAT_SIZE 2 // defines number of the pocsag data structures.

struct lbj_data {
    int8_t type = -1;
    int8_t direction = 0;
    char train[6] = "?????";
    char speed[6] = "???";
    char position[7] = "????.?";
    char lbj_class[3] = "  "; // '0X' or ' X'
    char loco[9] = "___?????"; // such as 23500331
    char loco_end[2] = " "; // '0'-' ' '1'-'A' '2'-'B'
    char route[9] = {"\0"}; // 16 bytes GBK data.
    char lon[13] = "??? ?? ???? "; // ---°--.----'
    char lat[12] = "?? ?? ???? "; // --°--.----'
};

struct rx_info {
    float rssi = 0;
    float fer = 0;
    float ppm = 0;
    uint16_t cnt = 0;
    uint64_t timer = 0;
    char date_time_str[20] = {'\0'};
};

struct data_bond {
    PagerClient::poc32 poc32;
    lbj_data lbjData;
};

extern "C" {

// size_t enc_unicode_to_utf8_one(uint32_t unic, unsigned char *pOutput);
size_t gbk2utf8(char *utf8s, const char *gbk1, const size_t gbk_len);

char chartobcd(char c);
// size_t bcdchar(char* dst, char* in, size_t start, size_t len);

int getnumN(const char* s, int len);
const char * get_loco_type(const char* s);
int16_t readDataLBJ(struct PagerClient::poc32 &p, struct lbj_data *l);

size_t formatEbit(char* dst, const uint8_t* err);
void parseEbit(uint8_t* err, const char* s);

size_t poc32_color(char* dst, const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r);
size_t LBJ_color(char* dst, const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r);
size_t poc32_log(char* dst, const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r);
size_t LBJ_log(char* dst, const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r);
size_t formatCSV(char* buffer, const PagerClient::poc32 &poc32, const lbj_data &lbj, const rx_info &rx, uint32_t ids);
void parseCSV(PagerClient::poc32 *poc32, lbj_data *lbj, rx_info *rx, uint32_t *id, const String line);
size_t formatCSVfile(char* buffer, const PagerClient::poc32 &p, const lbj_data &l, const rx_info &rx);
}


#endif //PAGER_RECEIVE_LBJ_HPP
