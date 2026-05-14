#ifndef __BCH_H
#define __BCH_H

#include <stdint.h>

extern "C" {

uint32_t even_parity(uint32_t m);
int32_t bin_cnt(uint32_t m);
uint32_t bch_ebin(uint32_t m);
uint32_t bch_correct(uint32_t &m, const uint32_t origin_bin, uint32_t word_idx);
void bch_err_map_init(void);

}

#endif
