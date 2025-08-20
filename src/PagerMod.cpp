#include<Arduino.h>
#include<RadioLib.h>
#include "BCH.hpp"
#include "networks.hpp"

static const char hex[] = "0123456789abcdef";
static const char c[] = "0123456789*u -][";
static const char epi_map[] = "0123456X";

__inline uint32_t pocsag_word_reverse4bit(uint32_t n) {
    n >>= 11;
    n &= 0xfffff;
    n = (n & 0x55555555) << 1 | (n & 0xAAAAAAAA) >> 1;
    n = (n & 0x33333333) << 2 | (n & 0xCCCCCCCC) >> 2;
    return n;
}

__inline void pocsag_msg(char* dst, uint32_t n, const char* char_map) {
    for (int ii = 0; ii < 5; ii++) {
        int bcd = (n >> (4 * (4 - ii))) & 0xf;
        if (bcd >= 0 && bcd < 16) {
            dst[ii] = char_map[bcd];
        }
    }
    dst[5] = '\0';
}

uint32_t pocsag_msg_encode(const char* s) {
    uint32_t bin = 0;
    for(int ii=0; ii<5; ii++) {
        bin <<=4;
        bin |= (uint32_t) chartobcd(s[ii]);
    }
    bin = (bin & 0x55555555) << 1 | (bin & 0xAAAAAAAA) >> 1;
    bin = (bin & 0x33333333) << 2 | (bin & 0xCCCCCCCC) >> 2;
    bin |= (1<<20);
    bin <<= 10;
    bin |= bch_ebin(bin);
    bin <<= 1;
    bin |= even_parity(bin);
    return bin;
}

int16_t PagerClient::readDataAll(struct PagerClient::poc32 &p, size_t len) {
    int16_t state = RADIOLIB_ERR_NONE;
    for(p.word_idx = 0; phyLayer->available() && p.word_idx < 16; p.word_idx++) {
        p.origin_bin[p.word_idx] = read();
        p.ebit[p.word_idx] = bch_correct(p.correct_bin[p.word_idx], p.origin_bin[p.word_idx], p.word_idx);

        if (p.origin_bin[p.word_idx] == 0xFFFFFFFF || p.origin_bin[p.word_idx] == 0x00000000) {
            break;
        }

        if (p.origin_bin[p.word_idx] == 0x55555555 || p.origin_bin[p.word_idx] == 0xAAAAAAAA) {
            break;
        }

        if(p.correct_bin[p.word_idx] == RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD) {
            sprintf(p.text+p.word_idx*5, "^^^^^");
            break;
        }

        if (p.correct_bin[p.word_idx] == RADIOLIB_PAGER_IDLE_CODE_WORD) {
            sprintf(p.text+p.word_idx*5, "_____");
            continue;
        }

        if(p.correct_bin[p.word_idx] >> (RADIOLIB_PAGER_CODE_WORD_LEN - 1) || !(1 << (p.word_idx) & 0x0011)) { // 0b0000000000010001
            pocsag_msg(
                p.text+p.word_idx*5,
                pocsag_word_reverse4bit(p.correct_bin[p.word_idx]),
                (1<<(p.word_idx) & 0x07A0 && p.address == 1234002)?hex:c // 0b0000011110100000
            );
        } else {
            p.address = ((p.correct_bin[p.word_idx] >> 13) & 0x3ffff) << 3;
            p.address += (p.word_idx)/2%8; // (word_bit_idx-1)/32/2%8;
            p.function_code = (p.correct_bin[p.word_idx] >> 11) & 0x3;

            sprintf(p.text+p.word_idx*5, "%02d %d:", p.address % 100, p.function_code);
        }
    }

    return (state);
}

bool PagerClient::gotPreambleState() {
    if (phyLayer->gotPreamble) {
        phyLayer->gotPreamble = false;
        phyLayer->preambleBuffer = 0;
        return true;
    } else
        return false;
}

bool PagerClient::gotCarrierState() {
    if (phyLayer->gotCarrier) {
        phyLayer->gotCarrier = false;
        phyLayer->carrierBuffer = 0x9877FA3CA50B1DBD; // just a random number,
        // if initialize to 0 it will consider as a carrier.
        return true;
    } else
        return false;
}

int16_t PagerClient::changeFreq(float base) {
    baseFreq = base;
    baseFreqRaw = (baseFreq * 1000000.0) / phyLayer->getFreqStep();

    int16_t state = phyLayer->setFrequency(baseFreq);
    RADIOLIB_ASSERT(state)

    state = phyLayer->receiveDirect();
    RADIOLIB_ASSERT(state)

    return (state);
}
//int16_t do_one_bit(){
//
//}

uint8_t PagerClient::poc32::EbitEven(size_t st, size_t ed) const {
    uint8_t ret = 0;
    for (size_t i=st; i<=ed; i++){
        uint8_t e = ebit[i]; // - even_parity(correct_bin[i]);
        if (ret < e) ret = e;
    }
    return ret;
}

uint8_t PagerClient::poc32::Ebit(size_t st, size_t ed) const {
    uint8_t ret = 0;
    for (size_t i=st; i<=ed; i++){
        uint8_t e = ebit[i] - even_parity(correct_bin[i]);
        if (ret < e) ret = e;
    }
    return ret;
}
