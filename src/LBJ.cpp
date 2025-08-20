#include "LBJ.hpp"

extern "C" {

// iconv /lib/utf8.h
static size_t utf8_wctomb(unsigned char *r, int32_t wc) {
  int count;
  if (wc < 0x80) count = 1;
  else if (wc < 0x800) count = 2;
  else if (wc < 0x10000) {
    if (0xd800 <= wc && wc < 0xe000) return 0;
    count = 3;
  } else if (wc < 0x110000)
    count = 4;
  else
    return 0;
  switch (count) { /* note: code falls through cases! */
    case 4: r[3] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x10000;
    case 3: r[2] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x800;
    case 2: r[1] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0xc0;
    case 1: r[0] = wc;
  }
  return count;
}

size_t gbk2utf8(char *utf8s, const char *gbk, const size_t gbk_len) {
    uint16_t utf16 = 0;
    size_t utf8s_idx = 0;

    for (size_t i = 0; i < gbk_len;) {
        if (gbk[i] == 0) {
            break;
        }
        if (gbk[i] & 0x80) {
            utf16 = ff_oem2uni((uint16_t) (gbk[i] << 8 | gbk[i + 1]), 936);
            if (!utf16) {
                utf16 = '_';
            }
            utf8s_idx += utf8_wctomb((unsigned char*)(utf8s+utf8s_idx), utf16);
            i+=2;
        } else {
            utf8s[utf8s_idx++] = gbk[i];
            i++;
        }
    }

    utf8s[utf8s_idx] = '\0';
    return utf8s_idx;
}

char chartobcd(char c) {
    // "0123456789.U -][";
    // "0123456789*U -][";
    // "0123456789*U -)(";
    // "0123456789ab -ef";
    // "0123456789abcdef";
    static const char bcdmap[128] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x0f,0x0e,0x0a,0x00,0x00,0x0d,0x0a,0x00,
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, 0x08,0x09,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x0b,0x00,0x00, 0x00,0x00,0x00,0x0f,0x00,0x0e,0x00,0x00,
        0x00,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x0b,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    };
    return bcdmap[c & 0x7f];
}

size_t bcdchar(char* dst, char* in, size_t start, size_t len) {
    size_t idx = 0;
    for(size_t i=start; i+2<=start+len; i+=2) {
        char c = (chartobcd(in[i]) << 4) | chartobcd(in[i+1]);
        if ((0x00 < c && c < 0x20) || (0x7e < c && c < 0xA1) || c > 0xfe) {
            c = '?';
        }
        dst[idx++]=c;
    }
    dst[idx]='\0';
    return idx;
}

size_t bcdcopy(char* dst, char* in, size_t start, size_t len) {
    size_t idx = 0;
    for(size_t i=start; i<start+len; i+=1) {
        char c = in[i];
        if (c < 0x20 || (0x7e < c && c < 0xA1) || c > 0xfe) {
            c = '?';
        }
        dst[idx++]=c;
    }
    dst[idx]='\0';
    return idx;
}

int getnumN(const char* s, int len) {
    int ret = 0;
    for (size_t c = 0; c < len; c++) {
        ret *= 10;
        if (isdigit(s[c])) {
            ret += s[c] - '0';
        } else {
            ret = -1;
            break;
        }
    }
    return ret;
}

const char * get_loco_type(const char* s) {
    int loco_type_num = getnumN(s, 3); 
    if (0 <= loco_type_num && loco_type_num < LEN(locos)) {
        return locos[loco_type_num];
    }
    return locos[0];
}

int16_t readDataLBJ(struct PagerClient::poc32 &p, struct lbj_data *l) {
    l->direction = (int8_t) p.function_code;

    if (p.word_idx <= 2) {
        l->type = -1;
        return RADIOLIB_ERR_MSG_CORRUPT;
    }

    if (p.address == 1234008) {
        lbj_time:
        memset(l, 0, sizeof(*l));
        l->type = 8;
        bcdcopy(l->train, p.text+5, 1, 2);
        l->train[2] = ':';
        bcdcopy(l->train+3, p.text+5, 3, 2);
        return 0;
    }

    int32_t test = 0;
    for (uint32_t word_idx = 2; word_idx<=3; word_idx++) {
        if (word_idx >= p.word_idx) break;
        if (bin_cnt(p.origin_bin[word_idx] ^ RADIOLIB_PAGER_IDLE_CODE_WORD) <= 2) {
            Serial.printf("[LBJ]\x1b[33mTransformed type %7d to 1234008. origin_bin[%2d/%2d]=_\x1b[0m\n", p.address, word_idx, p.word_idx);
            p.address = 1234008;
            goto lbj_time;
            break;
        }
    }

    if (!p.function_code && (p.text[5+0] == '-' || p.text[5+0] == '*') &&
        '0' <= p.text[5+1] && p.text[5+1] <= '2' &&
        '0' <= p.text[5+2] && p.text[5+1] <= '9' &&
        '0' <= p.text[5+3] && p.text[5+1] <= '5' &&
        '0' <= p.text[5+4] && p.text[5+1] <= '9') {

        Serial.printf("[LBJ]\x1b[32mTransformed type %7d to 1234008.\x1b[0m\n", p.address);
        p.address = 1234008;
        goto lbj_time;
    }

    if (p.ebit[8] < 6 && p.ebit[7] > 1 && p.text[40]=='e' && p.text[41]=='a' && p.text[42]=='9') {
        p.ebit[7] = 6;
        p.text[39] = 'b';
    }

    /*
     * The standard LBJ Proximity Alarm info is formed in:
     * ----- --- -----
     * TRAIN SPD POS
     * 0-4   6-8 10-14 in total of 15 nibbles / 60 bits.
     */
    l->type = 1;

    bcdcopy(l->train, p.text, 5+0, 5);
    bcdcopy(l->speed, p.text, 5+6, 3);

    bcdcopy(l->position, p.text, 5+10, 4);
    l->position[4] = '.';
    bcdcopy(l->position+5, p.text, 5+14, 1);

    for (uint32_t word_idx = 4; word_idx<15; word_idx++) {
        if (word_idx >= p.word_idx) break;
        if (bin_cnt(p.origin_bin[word_idx] ^ RADIOLIB_PAGER_IDLE_CODE_WORD) <= 2) {
            // Serial.printf("[LBJ]\x1b[33mTransformed type %7d to 1234000. origin_bin[%2d/%2d]=_\x1b[0m\n", p.address, word_idx, p.word_idx);
            l->type = 1;
            p.address = 1234000;
            l->lbj_class[0] = '\0';
            l->loco[0] = '\0';
            l->loco_end[0] = ' ';
            l->loco_end[1] = '\0';
            l->route[0] = '\0';
            return 0;
        }
    }

    if (p.address != 1234002 || p.Ebit(3,3) > 2) {
    for (uint32_t word_idx = 4; word_idx<p.word_idx-1; word_idx++) {
        if (word_idx >= p.word_idx) break;
        if (p.ebit[word_idx] <= 0) {
            test=9;
            break;
        }
        if (p.ebit[word_idx] <= 2) {
            test++;
        }
        if (p.ebit[word_idx] >= 5) {
            test--;
        }
    }
    if (test < 3) {
        Serial.printf("[LBJ]\x1b[33m1234000 ?. %7d test=%2d\x1b[0m\n", p.address, test);
        p.address = 1234000;
    }
    }

    if (p.address == 1234000 && p.Ebit(0,0) > 0) {
    test = 0;
    for (uint32_t word_idx = 1; word_idx <= 3; word_idx++) {
        if (p.ebit[word_idx] == 0) {
            test=9;
            break;
        } 
        if (p.ebit[word_idx] <= 1) {
            test++;
        }
        if (p.ebit[word_idx] >= 7) {
            test--;
        }
    }
    if (test <= 0) {
        Serial.printf("[LBJ]\x1b[31m1234000 x. %7d test=%2d\x1b[0m\n", p.address, test);
        l->type = -1;
        return RADIOLIB_ERR_MSG_CORRUPT;
    }
    }

    /*
     * The LBJ Extend Info (info2) message does not appear on any standards,
     * decoding based purely on guess and formerly received type 1 messages.
     * A typical type 1 message is:
     * 204U2390093130U-(2 9U- (-(202011720927939053465000
     * |204U2|39009|3130U|-(2 9|U- (-|(2020|11720|92793|90534|65000|
     *   0-4   5-9  10-14 15-19 20-24 25-29 30-34 35-39 40-44 45-49
     * in which we phrase it to:
     * |204U|23900931|30|U-(2 9U- (-(2020|117209279|39053465|000
     *    I     II   III       IV            V         VI    VII
     * I.   00-03   Two ASCII bytes for class, in this case 4U = 4B = K.
     * II.  04-11   8 nibbles/32 bits for locomotive register number.
     * III. 12-13   2 nibbles/8 bits for locomotive ends, 31 for A, 32 for B.
     * IV.  14-29   4 GBK characters/8 bytes/16 nibbles/32 bits for route.
     * V.   30-38   9 nibbles for longitude in format XXX°XX.XXXX′ E.
     * VI.  39-46   8 nibbles for latitude in format XX°XX.XXXX′ N.
     * VII. 47-49   Unknown, usually 000, sometimes FFF("(((")/CCC("   ")?, maybe some sort of idle word.
     * In total of 50 nibbles / 200 bits.
     */

    bcdchar(l->lbj_class, p.text+25, 0, 4);
    if (l->lbj_class[0] < 0x30 || 0x5b <= l->lbj_class[0]) l->lbj_class[0] = ' ';
    if (l->lbj_class[1] < 0x30 || 0x5b <= l->lbj_class[1]) l->lbj_class[1] = ' ';

    bcdcopy(l->loco, p.text+25, 4, 3);
    bcdcopy(l->loco+3, p.text+25, 7, 5);
    bcdchar(l->loco_end, p.text+25, 12, 2);
    if (l->loco_end[0] == '0') l->loco_end[0] = ' ';
    if (l->loco_end[0] == '1') l->loco_end[0] = 'A';
    if (l->loco_end[0] == '2') l->loco_end[0] = 'B';

    // to GB2312 for route.
    bcdchar(l->route, p.text+25, 14, 16);

    // positions lon
    bcdcopy(l->lon, p.text+25, 30, 3);
    l->lon[3] = ':';
    bcdcopy(l->lon+4, p.text+25, 33, 2);
    l->lon[6] = '.';
    bcdcopy(l->lon+7, p.text+25, 35, 4);
    l->lon[12] = '\'';
    l->lon[13] = '\0';

    // position lat --°--.----'
    bcdcopy(l->lat, p.text+25, 39, 2);
    l->lat[2] = ':';
    bcdcopy(l->lat+3, p.text+25, 41, 2);
    l->lat[5] = '.';
    bcdcopy(l->lat+6, p.text+25, 43, 4);
    l->lat[11] = '\'';
    l->lat[12] = '\0';

    if (p.address == LBJ_INFO_ADDR) {
        l->type = 1;
        return 0;
    }

    l->type = 2;

    return 0;
}

size_t formatEbit(char* dst, const uint8_t* err) {
    size_t i=0;
    int total = 0;
    for (i=0; i<16; i++) {
        dst[i] = '0'+err[i];
        total += err[i];
    }
    dst[16] = '\0';
    if (total <= 1) {
        dst[0] = '\0';
        i = 0;
    }
    return i;
}

void parseEbit(uint8_t* err, const char* s) {
    for (int i=0; s[i] && i<16; i++) {
        err[i] = s[i]-'0';
    }
    return;
}

// static uint16_t fontcolor[8] = {0x07, 0x0e, 0x06, 0x0b, 0x01, 0x0d, 0x05, 0x04};
static const uint16_t fontcolor[8] = {37, 36, 33, 34, 32, 35, 95, 31}; // 黑红绿黄蓝紫青白

size_t colorsprintf(int8_t color, bool under, char* dst, const char* format, ...) {
    size_t idx=0;
    if (color) {
        idx += sprintf(dst+idx, "\x1b[%02d%sm", fontcolor[color], under?";4":""); 
        // p.ebit[word_idx]-even_parity(p.correct_bin[word_idx])
        // even_parity(p.correct_bin[word_idx])
    }
    va_list args;
    va_start(args, format);
    idx += vsprintf(dst+idx, format, args);
    va_end(args);
    if (color) {
        idx += sprintf(dst+idx, "\x1b[0m");
    }
    return idx;
}

size_t poc32_color(char* dst, const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r) {
    /* 
     * [PGR][1234000/1: 52011  70  1503][1234002/1: 20202350019930U)*9UU*6 (-(202111719082039060616000][-85.6dBm/+123Hz/3.0ppm][E:12/12]
     * [LBJ]方向:下 车次:  52011 速度: 70km/h 公里标: 150.3km 线路: 京西动走 车号:23500199 位置: 117°19.0820′ 39°06.0616′
     */
    int32_t idx = 0;

    idx += sprintf(dst+idx, "[PGR]");
    for (int32_t word_idx = 0; word_idx < p.word_idx; word_idx++) {
        if (p.ebit[word_idx]) {
            idx += sprintf(dst+idx, "\x1b[%02d%sm", fontcolor[p.Ebit(word_idx,word_idx)], even_parity(p.correct_bin[word_idx])?";4":"");
        }
        memcpy(dst+idx, p.text+word_idx*5, 5);
        idx += 5;
        if (p.ebit[word_idx]) {
            idx += sprintf(dst+idx, "\x1b[0m");
        }
    }

    idx += sprintf(dst+idx, "[%3.1fdBm/%+4.0fHz/%.2fppm]", r.rssi, r.fer, r.ppm); // ->%.2fppm , getBias((float) (actual_frequency + r.fer * 1e-6))
    idx += sprintf(dst+idx, "\n");
    return idx;
}

size_t LBJ_color(char* dst, const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r) {
    /* 
     * [PGR][1234000/1: 52011  70  1503][1234002/1: 20202350019930U)*9UU*6 (-(202111719082039060616000][-85.6dBm/+123Hz/3.0ppm][E:12/12]
     * [PGR][00 1:52011  70  150302 1:20202350019930U)*9UU*6 (-(202111719082039060616000_____][-85.6dBm/+123Hz/3.0ppm][E:12/12]
     * [LBJ]下 车次:  52011 速度: 70km/h 公里标: 150.3km 车号:23500199 线路: 京西动走 位置: 117°19.0820′ 39°06.0616′
     * [LBJ]00  136  70km/h  150.3km下 24300002A[京西动走] 117°19.0820′ 39°06.0616′
     */
    int32_t idx = 0;

    if (l.type <= 0) {
        dst[0] = '\0';
        return idx;
    }

    // idx += sprintf(dst+idx, "[LBJ]");
    idx += sprintf(dst+idx, r.date_time_str);
    idx += sprintf(dst+idx, ":");
    if (l.type == 8) {
        idx += colorsprintf(p.Ebit(1,1), false, dst+idx, "时间%c%s", p.text[5], l.train);
        idx += sprintf(dst+idx, "\n");
        return idx;
    }

    idx += colorsprintf(p.Ebit(5,5), false, dst+idx, "%-2s", l.lbj_class);
    idx += colorsprintf(p.Ebit(1,1), false, dst+idx, "%5s", l.train);
    idx += colorsprintf(p.Ebit(2,2), false, dst+idx, " %3skm/h", l.speed);
    idx += colorsprintf(p.Ebit(3,3), false, dst+idx, " %skm", l.position);
    idx += colorsprintf(p.Ebit(0,0), false, dst+idx, "%2s", function_code_map[l.direction]);

    if (l.type == 1) {
        idx += sprintf(dst+idx, "\n");
        return idx;
    }

    idx += colorsprintf(p.Ebit(5,7), false, dst+idx, " %s%s", l.loco, l.loco_end);
    idx += sprintf(dst+idx, "[");
    idx += sprintf(dst+idx, "\x1b[%02d%sm", fontcolor[p.Ebit(8,10)], false?";4":"");
    idx += gbk2utf8(dst+idx, l.route, 8);
    idx += sprintf(dst+idx, "\x1b[0m");
    idx += sprintf(dst+idx, "]");
    idx += colorsprintf(p.Ebit(11,12), false, dst+idx, " %s", l.lon);
    idx += colorsprintf(p.Ebit(13,14), false, dst+idx, " %s", l.lat);
    idx += sprintf(dst+idx, "\n");
    return idx;
}

size_t poc32_log(char* dst, const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r) {
    int32_t idx = 0;
    idx += sprintf(dst+idx, "[PGR][%s]", p.text);
    // idx += sprintf(dst+idx, "[R:%3.1f dBm/F:%5.2f Hz/P:%.1f]", r.rssi, r.fer, r.ppm);
    idx += sprintf(dst+idx, "[R:%3.1fdBm/F:%+4.0fHz/P:%.2fppm]", r.rssi, r.fer, r.ppm); // ->%.2f , getBias((float) (actual_frequency + r.fer * 1e-6))
    if (p.Ebit(0,14) >= 2) {
        idx += sprintf(dst+idx, "[");
        idx += formatEbit(dst+idx, p.ebit);
        idx += sprintf(dst+idx, "]");
    }
    idx += sprintf(dst+idx, "\n");
    return idx;
}

size_t LBJ_log(char* dst, const PagerClient::poc32 &p, const struct lbj_data &l, const struct rx_info &r) {
    int32_t idx = 0;
 
    if (l.type <= 0) {
        dst[0] = '\0';
        return idx;
    }

    // idx += sprintf(dst+idx, "[LBJ]");

    if (l.type == 8) {
        idx += sprintf(dst+idx, "当前时间%s\n", l.train);
        return idx;
    }

    sprintf(dst+idx, "方向:%2s 车次:%-2s%5s 速度:%3skm/h 公里标:%skm", function_code_map[l.direction], l.lbj_class, l.train, l.speed, l.position);

    if (l.type == 1) {
        idx += sprintf(dst+idx, "\n");
        return idx;
    }

    idx += sprintf(dst+idx, "车号:%s%s", l.loco, l.loco_end);
    idx += sprintf(dst+idx, "线路:");
    idx += gbk2utf8(dst+idx, l.route, 8);
    idx += sprintf(dst+idx, " 位置: %s %s", l.lon, l.lat);
    idx += sprintf(dst+idx, "\n");
    return idx;
}

size_t formatCSV(char* buffer, const PagerClient::poc32 &p, const lbj_data &l, const rx_info &rx, uint32_t ids) {
    size_t idx = 0;
    idx += sprintf(buffer+idx, "%04d,%s", ids, rx.date_time_str);
    idx += sprintf(buffer+idx, ",%d,%d,%s,%s,%s,%s", l.type, l.direction, l.lbj_class, l.train, l.speed, l.position);
    idx += sprintf(buffer+idx, ",%s%c", l.loco, l.loco_end[0]);
    if (buffer[idx] != 'A' && buffer[idx] != 'B') {
        idx--;
    }
    idx += sprintf(buffer+idx, ",%s,%s,%s", l.route, l.lon, l.lat);
    idx += sprintf(buffer+idx, ",%.1f,%.0f,%.2f,", rx.rssi, rx.fer, rx.ppm);
    if (p.Ebit(0,14) >= 2) {
        idx += formatEbit(buffer+idx, p.ebit);
    }
    return 0;
}

size_t formatCSVfile(char* buffer, const PagerClient::poc32 &p, const lbj_data &l, const rx_info &rx) {
    size_t idx = 0;
    idx += sprintf(buffer+idx, "%s", rx.date_time_str);
    idx += sprintf(buffer+idx, ",%d,%s,%s,%s,%s", function_code_map[l.direction], l.lbj_class, l.train, l.speed, l.position);

    if (l.type == 2) {
    idx += sprintf(buffer+idx, ",%s%c", l.loco, l.loco_end[0]);
    if (buffer[idx] != 'A' && buffer[idx] != 'B') {
        idx--;
    }
    idx += sprintf(buffer+idx, ",%s,%s,%s", l.route, l.lon, l.lat);
    } else {
        idx += sprintf(buffer+idx, ",,,,");
    }

    idx += sprintf(buffer+idx, ",%.1f,%.0f,%.2f,", rx.rssi, rx.fer, rx.ppm);
    idx += sprintf(buffer+idx, ",\"%s\"", p.text);
    if (p.Ebit(0,14) >= 2) {
        idx += sprintf(buffer+idx, ",");
        idx += formatEbit(buffer+idx, p.ebit);
    }
    return 0;
}

void parseCSV(PagerClient::poc32 *poc32, lbj_data *lbj, rx_info *rx, uint32_t *id, const String line) {
    // Tokenize
    String tokens[16];
    for (size_t i = 0, c = 0; i < line.length(); i++) {
        if (line[i] == ',') {
            c++;
            continue;
        }
        if (c < LEN(tokens))
            tokens[c] += line[i];
    }

    *id = std::stoul(tokens[0].c_str());
    tokens[1].toCharArray(rx->date_time_str, sizeof(rx->date_time_str));

    lbj->type = (int8_t) std::stoi(tokens[2].c_str());
    lbj->direction = (int8_t) std::stoi(tokens[3].c_str());
    tokens[4].toCharArray(lbj->lbj_class, sizeof(lbj->lbj_class));
    tokens[5].toCharArray(lbj->train, sizeof(lbj->train));
    tokens[6].toCharArray(lbj->speed, sizeof(lbj->speed));
    tokens[7].toCharArray(lbj->position, sizeof(lbj->position));

    tokens[8].toCharArray(lbj->loco, sizeof(lbj->loco));
    if (lbj->loco[8]) {
        lbj->loco_end[0] = lbj->loco[8];
        lbj->loco_end[1] = '\0';
        lbj->loco[8] = '\0';
    }

    tokens[9].toCharArray(lbj->route, sizeof(lbj->route));
    tokens[10].toCharArray(lbj->lon, sizeof(lbj->lon));
    tokens[11].toCharArray(lbj->lat, sizeof(lbj->lat));

    rx->rssi = std::stof(tokens[12].c_str());
    rx->fer = std::stof(tokens[13].c_str());
    rx->ppm = std::stof(tokens[14].c_str());

    parseEbit(poc32->ebit, tokens[15].c_str());

    return;
}

} // extern "C"
