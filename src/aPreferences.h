//
// Created by FLN1021 on 2024/2/8.
//

#ifndef PAGER_RECEIVE_APREFERENCES_H
#define PAGER_RECEIVE_APREFERENCES_H

#include "LBJ.hpp"
#include <Preferences.h>
#include <nvs_flash.h>
#include <cstdint>

const int PREF_MAX_LINES = 10000;

class aPreferences {
public:
    aPreferences();

    bool begin(const char *name, bool read_only);

    bool append(const PagerClient::poc32 &poc32, const lbj_data &lbj, const rx_info &rx);

    bool retrieve(PagerClient::poc32 *poc32, lbj_data *lbj, rx_info *rx, 
        uint32_t *id, int8_t bias);

    bool retrieve2(PagerClient::poc32 *poc32, lbj_data *lbj, rx_info *rx, 
        uint32_t *id);

    // bool retrieve(String *str_array, uint32_t arr_size, int8_t bias);

    bool clearKeys();

    void toLatest(int8_t bias = -1);

    bool isLatest(int8_t bias = -1) const;

    void getStats();

    uint32_t getID();

    int32_t getRetLines();

    void setRetLines(int32_t line);

    uint32_t getStartTime();

    void setStartTime(uint32_t cnt);

    uint32_t incStartTime();

private:
    Preferences pref;
    const char *ns_name;
    bool have_pref;
    // bool overflow;
    uint16_t lines;
    int32_t ret_lines; // defaults -1
    uint32_t ids;
    const char *partition_name = "nvs_ext";
};


#endif //PAGER_RECEIVE_APREFERENCES_H
