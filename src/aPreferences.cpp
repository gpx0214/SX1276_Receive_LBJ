//
// Created by FLN1021 on 2024/2/8.
//

#include <cstdint>
#include "aPreferences.h"
#include "LBJ.hpp"

aPreferences::aPreferences() : pref{}, have_pref(false), lines(0), ret_lines(0), ids(0), ns_name{} {

}

bool aPreferences::begin(const char *name, bool read_only) {
    // nvs_flash_erase_partition(partition_name);
    // nvs_flash_erase();
    // nvs_flash_init();

    if (!pref.begin(name, read_only, partition_name))
        return false;
    have_pref = true;
    ns_name = name;

    // pref.clear(); // first time
    
    if (pref.isKey("ids"))
        ids = pref.getUInt("ids");
    if (pref.isKey("lines"))
        lines = pref.getUShort("lines");
    ret_lines = lines;

    // char key[8];
    // sprintf(key, "I%04d", lines + 1);
    // if (pref.isKey(key))
    //     overflow = true;

    getStats();
    return true;
}

bool aPreferences::append(const PagerClient::poc32 &poc32, const lbj_data &lbj, const rx_info &rx) {
    if (!have_pref)
        return false;
    if (lbj.type == -1)
        return false;

    if (lbj.speed[2] < '0' || '9' < lbj.speed[2] ) {
        return false;
    }

    char key[8];
    char value[160];

    lines = ids % PREF_MAX_LINES;
    sprintf(key, "I%04d", lines);
    formatCSV(value, poc32, lbj, rx, ids);

    pref.putString(key, value);
    lines++;
    pref.putUShort("lines", lines);
    ids++;
    pref.putUInt("ids", ids);
    return true;
}

bool
aPreferences::retrieve(
    PagerClient::poc32 *poc32, lbj_data *lbj, rx_info *rx, 
    uint32_t *id, int8_t bias) {
    if (!have_pref)
        return false;
    ret_lines += bias;
    if (ids >= PREF_MAX_LINES) {
        if (ret_lines < 0)
            ret_lines = PREF_MAX_LINES-1;
        if (ret_lines >= PREF_MAX_LINES)
            ret_lines = 0;
    } else {
        if (ret_lines < 0)
            ret_lines = lines - 1;
        if (ret_lines >= lines)
            ret_lines = 0;
    }

    char key[8];
    sprintf(key, "I%04d", ret_lines % PREF_MAX_LINES);
    if (!pref.isKey(key))
        return false;
    String line = pref.getString(key);
    Serial.printf("[D]%s:%s\n", key, line.c_str());
    try {
        parseCSV(poc32, lbj, rx, id, line);
    } catch (...) {
        Serial.printf("[D]except %s:%s\n", key, line.c_str());
        return false;
    }

    return true;
}

bool
aPreferences::retrieve2(
    PagerClient::poc32 *poc32, lbj_data *lbj, rx_info *rx, 
    uint32_t *id) {
    if (!have_pref)
        return false;
 
    char key[8];
    sprintf(key, "I%04d", *id % PREF_MAX_LINES);
    if (!pref.isKey(key))
        return false;
    String line = pref.getString(key);
    // Serial.printf("[D]%s:%s\n", key, line.c_str());
    try {
        parseCSV(poc32, lbj, rx, id, line);
    } catch (...) {
        Serial.printf("[D]except %s:%s\n", key, line.c_str());
        return false;
    }

    return true;
}

bool aPreferences::clearKeys() {
    uint32_t id = pref.getUInt("ids");
    if (!pref.clear())
        return false;
    // overflow = false;
    lines = 0;
    pref.putUShort("lines", lines);
    ret_lines = 0;
    pref.putUInt("ids", id);
    return true;
}

void aPreferences::toLatest(int8_t bias) {
    ret_lines = lines + bias;
    if (ids >= PREF_MAX_LINES) {
        if (ret_lines < 0)
            ret_lines = PREF_MAX_LINES-1;
        if (ret_lines >= PREF_MAX_LINES)
            ret_lines = 0;
    } else {
        if (ret_lines < 0)
            ret_lines = lines - 1;
        if (ret_lines >= lines)
            ret_lines = 0;
    }
}

void aPreferences::getStats() {
    if (!have_pref)
        return;
    nvs_stats_t stats;
    nvs_get_stats(partition_name, &stats);
    Serial.printf("[NVS] %s entries: used %d, free %d, total %d\n",
                  partition_name, stats.used_entries, stats.free_entries, stats.total_entries);
    nvs_handle_t handle;
    nvs_open_from_partition(partition_name, ns_name, NVS_READONLY, &handle);
    size_t used_entries;
    nvs_get_used_entry_count(handle, &used_entries);
    nvs_close(handle);
    Serial.printf("[NVS] %d entries in namespace %s, Current line %d, id %d\n", used_entries, ns_name, lines, ids);
}

uint32_t aPreferences::getID() {
    return ids;
}

bool aPreferences::isLatest(int8_t bias) const {
    if (ret_lines == lines + bias)
        return true;
    else
        return false;
}

int32_t aPreferences::getRetLines() {
    return ret_lines;
}

void aPreferences::setRetLines(int32_t line) {
    ret_lines = line;
}

uint32_t aPreferences::getStartTime() {
    if (!have_pref)
        return 0;
    return pref.getUInt("start_cnt");
}

void aPreferences::setStartTime(uint32_t cnt) {
    if (!have_pref)
        return;
    pref.putUInt("start_cnt", cnt);
    return;
}

uint32_t aPreferences::incStartTime() {
    if (!have_pref)
        return 0;
    
    uint32_t start_cnt = pref.getUInt("start_cnt");
    start_cnt++;
    pref.putUInt("start_cnt", start_cnt);
    return start_cnt;
}
