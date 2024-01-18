#include<Arduino.h>
#include<RadioLib.h>
#include "BCH3121.hpp"
#include "networks.hpp"

bool fixBCH(uint32_t &cw, CBCH3121 &bch, uint16_t &err) {
    //todo: this function is useless, it's highly unlikely to fix message like this. Delete this function to save time.
    bool p_check = true;
    uint16_t err_last = err;
    uint32_t cw_last = cw;
    if (bch.decode(cw, err, p_check) && p_check) {
        // Serial.printf("[D] NCorr\n");
        return true;
    }
    //uint16_t err;
    cw = cw_last;
    for (int i = 1; i < 11; ++i) {
        err = 0;
        uint32_t mask = 1 << i;
        cw ^= mask;
        if (bch.decode(cw, err, p_check) && p_check) {
            err += err_last;
            Serial.printf("[D] Correction Success!\n");
            sd1.append("[D] Correction Success!\n");
            return true;
        }
        cw = cw_last;
    }
    err += err_last;
    // Serial.printf("[D] Correction Failed. err %d\n", err);
    return false;
}
uint32_t reflect(uint32_t in, uint8_t bits) {
    uint32_t res = 0;
    for (uint8_t i = 0; i < bits; i++) {
        res |= (((in & ((uint32_t) 1 << i)) >> i) << (bits - i - 1));
    }
    return (res);
}

void decode20(uint32_t cw, char *char_array) {
    uint32_t data = (cw << 1) >> 12;
    uint8_t hex_array[5];
    hex_array[4] = data & 0x0000000f;
    hex_array[3] = (data & 0x000000f0) >> 4;
    hex_array[2] = (data & 0x00000f00) >> 8;
    hex_array[1] = (data & 0x0000f000) >> 12;
    hex_array[0] = (data & 0x000f0000) >> 16;

    // char char_array[5];
    for (uint8_t i = 0; i < 5; i++) {
        // reflect bits
        hex_array[i] = reflect(hex_array[i],4);

        switch (hex_array[i]) {
            case 0x0A:
                char_array[i] = '*';
                break;
            case 0x0B:
                char_array[i] = 'U';
                break;
            case 0x0C:
                char_array[i] = ' ';
                break;
            case 0X0D:
                char_array[i] = '-';
                break;
            case 0x0E:
                char_array[i] = ')';
                break;
            case 0x0F:
                char_array[i] = '(';
                break;
            default:
                char_array[i] = (char) (hex_array[i] + '0');
        }
    }

    char_array[5] = '\0';
}

int16_t PagerClient::readDataMSA(struct PagerClient::pocsag_data *p, size_t len) {
    int16_t state = RADIOLIB_ERR_NONE;
    bool complete = false;
    uint8_t framePos = 0;
    uint32_t addr_next = 0;
//        bool is_empty = true;
    for (size_t i = 0; i < POCDAT_SIZE; i++) {
        // determine the message length, based on user input or the amount of received data
        size_t length = len;
        if (len == 0) {
            // one batch can contain at most 80 message symbols
            len = available() * 80;
        }

        if (complete)
            break;

        // build a temporary buffer
#if defined(RADIOLIB_STATIC_ONLY)
        uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
#else
        // auto *data = new uint8_t[len + 1];
        // if (!data) {
        //     return (RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
        // }
#endif
        uint8_t data[len + 1];

        state = readDataMA(data, &length, &p[i].addr, &p[i].func, &framePos, &addr_next, &p[i].is_empty,
                           &complete, &p[i].errs_total, &p[i].errs_uncorrected);

        if (i && state == RADIOLIB_ERR_ADDRESS_NOT_FOUND) {
//                Serial.println("ADDR NO MATCH");
//             delete[] data;
//             data = nullptr;
            state = RADIOLIB_ERR_NONE;
            break;
        }

        if (i && state == RADIOLIB_ERR_MSG_CORRUPT) {
            // Serial.printf("[D] MSG%d CORRUPT.\n", i);
            // Serial.printf("[D] data[] len %d, message len %d, data addr %p\n", len + 1, length, data);
            // delete[] data; // Due to unknown reason crash often occurs here.
            // Fixed by stop using new and delete...no choice, sorry.
            // data = nullptr; // REMEMBER TO INITIALIZE POINTER AFTER DELETE!!!
            state = RADIOLIB_ERR_NONE;
            break;
        }

        if (state == RADIOLIB_ERR_NONE && !p[i].is_empty) {
            if (length == 0) {
                p[i].is_empty = true;
//                    length = 6;
//                    strncpy((char *) data, "<tone>", length + 1);
            }
            data[length] = 0;
            p[i].str = String((char *) data);
            if (!p[i].str.length())
                p[i].is_empty = true;
            p[i].len = length;
//                length = 0;

        }
#if !defined(RADIOLIB_STATIC_ONLY)
        // delete[] data;
        // data = nullptr;
#endif
        if (state != RADIOLIB_ERR_NONE)
            break;
    }
//    // build a temporary buffer
//#if defined(RADIOLIB_STATIC_ONLY)
//    uint8_t data[RADIOLIB_STATIC_ARRAY_SIZE + 1];
//#else
//    uint8_t *data = new uint8_t[length + 1];
//    if (!data) {
//        return (RADIOLIB_ERR_MEMORY_ALLOCATION_FAILED);
//    }
//#endif
//
//    // read the received data
////    state = readDataMSA(data, &length, addr, func, add, clen);
//
//    if (state == RADIOLIB_ERR_NONE) {
//        // check tone-only tramsissions
//        if (length == 0) {
//            length = 6;
//            strncpy((char *) data, "<tone>", length + 1);
//        }
//
//        // add null terminator
//        data[length] = 0;
//
//        // initialize Arduino String class
////        str = String((char *) data);
//    }
//
//    // deallocate temporary buffer
//#if !defined(RADIOLIB_STATIC_ONLY)
//    delete[] data;
//#endif

    return (state);
}

int16_t PagerClient::readDataMA(uint8_t *data, size_t *len, uint32_t *addr, uint32_t *func, uint8_t *framePos,
                                uint32_t *addr_next, bool *is_empty, bool *complete, uint16_t *errs_total,
                                uint16_t *errs_uncorrected) {
    // find the correct address
    bool match = false;
//    uint8_t framePos = 0;
    uint8_t symbolLength = 0;
    uint16_t errors = 0;
    bool parity_check = true;
    uint16_t err_prev;
    uint32_t cw_prev;
    CBCH3121 PocsagFec;
    bool is_sync = true;

    if (*addr_next) {
        uint32_t addr_found =
                ((*addr_next & RADIOLIB_PAGER_ADDRESS_BITS_MASK) >> (RADIOLIB_PAGER_ADDRESS_POS - 3)) | (*framePos / 2);
        if ((addr_found & filterMask) == (filterAddr & filterMask)) {
            *is_empty = false;
            match = true;
            symbolLength = 4;
            *addr = addr_found;
            *func = (*addr_next & RADIOLIB_PAGER_FUNCTION_BITS_MASK) >> RADIOLIB_PAGER_FUNC_BITS_POS;
            *addr_next = 0;
        } else {
            return (RADIOLIB_ERR_ADDRESS_NOT_FOUND);
        }
    }

    while (!match && phyLayer->available()) {

        uint32_t cw = read();
//        *framePos++;
        *framePos = *framePos + 1;
        err_prev = errors;
        cw_prev = cw;
        if (!PocsagFec.decode(cw, errors, parity_check)) {
//            Serial.println("BCH Failed.");
            *errs_uncorrected += errors - err_prev;
            if (!is_sync) {
                *errs_total = errors;
                return (RADIOLIB_ERR_MSG_CORRUPT);
            }
            is_sync = false;
            continue;
        } else {
            if (!parity_check) {
                *errs_uncorrected += errors - err_prev;
                if (!is_sync) {
                    *errs_total = errors;
                    return (RADIOLIB_ERR_MSG_CORRUPT);
                }
                is_sync = false;
                parity_check = true;
                continue;
            }
            // Serial.printf("BCH SUCCESS, ERR %d\n", errors);
            is_sync = true;
        }

        // check if it's the idle code word
        if (cw == RADIOLIB_PAGER_IDLE_CODE_WORD) {
//            Serial.println("IDLE FOUND.");
            continue;
        }

        // check if it's the sync word
        if (cw == RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD) {
            *framePos = 0;
//            Serial.println("SYNC FOUND.");
            continue;
        }


        // not an idle code word, check if it's an address word
        if (cw & (RADIOLIB_PAGER_MESSAGE_CODE_WORD << (RADIOLIB_PAGER_CODE_WORD_LEN - 1))) {
            // this is pretty weird, it seems to be a message code word without address
            continue;
        }

        // should be an address code word, extract the address
        uint32_t addr_found =
                ((cw & RADIOLIB_PAGER_ADDRESS_BITS_MASK) >> (RADIOLIB_PAGER_ADDRESS_POS - 3)) | (*framePos / 2);
        if ((addr_found & filterMask) == (filterAddr & filterMask)) {
            // we have a match!
            *is_empty = false;
            match = true;
//            Serial.printf("GOT ADDR %d \n",addr_found);
//            Serial.printf("RAW CW %X \n",cw);
            if (addr) {
                *addr = addr_found;
                *func = (cw & RADIOLIB_PAGER_FUNCTION_BITS_MASK) >> RADIOLIB_PAGER_FUNC_BITS_POS;
            }

//            Serial.print("Function: ");
//            Serial.print((cw & RADIOLIB_PAGER_FUNCTION_BITS_MASK) >> RADIOLIB_PAGER_FUNC_BITS_POS);


            // determine the encoding from the function bits
            // No determine
            symbolLength = 4;
        }
    }

    if (!match) {
        // address not found
        return (RADIOLIB_ERR_ADDRESS_NOT_FOUND);
    }

    // we have the address, start pulling out the message
//    bool complete = false;
    size_t decodedBytes = 0;
    size_t deco = 0;
    uint32_t prevCw = 0;
    bool overflow = false;
    int8_t ovfBits = 0;
    errors = 0;
    while (!*complete && phyLayer->available()) {
//        *framePos++;
        *framePos = *framePos + 1;
        uint32_t cw = read();

        err_prev = errors;
        cw_prev = cw;
        if (PocsagFec.decode(cw, errors, parity_check)) {
            if (!parity_check) {
                for (size_t i = 0; i < 5; i++) {
                    data[decodedBytes++] = 'X';
                }
                *errs_uncorrected += errors - err_prev;
                if (!is_sync) {
                    *errs_total = errors;
                    if (deco != 0)
                        goto end;
                    return (RADIOLIB_ERR_MSG_CORRUPT);
                }
                is_sync = false;
                parity_check = true;
                continue;
            }
            is_sync = true;
            // Serial.printf("SYNC BCH CHECK PERFORMED, ERR %d, ERR_TTL %d \n", errors - err_prev, errors);
        } else {
            *errs_uncorrected += errors - err_prev;
            for (size_t i = 0; i < 5; i++) {
                data[decodedBytes++] = 'X';
            }
            // Serial.printf("BCH Failed. ERR %d \n", errors);
            if (!is_sync) {
                *errs_total = errors;
                if (deco != 0)
                    goto end;
                return (RADIOLIB_ERR_MSG_CORRUPT);
            }
            is_sync = false;
            continue;
        }

        // check if it's the idle code word
        if (cw == RADIOLIB_PAGER_IDLE_CODE_WORD) {
//            Serial.println("IDLE FOUND.");
            *complete = true;
            break;
        }

        // skip the sync words
        if (cw == RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD) {
//            Serial.println("SYNC FOUND.");
            continue;
        }

        if (!(cw & (RADIOLIB_PAGER_MESSAGE_CODE_WORD << (RADIOLIB_PAGER_CODE_WORD_LEN - 1)))) {
            *addr_next = cw; // returned codeword actually for function determination.
            break;
        }

        // check overflow from previous code word
        uint8_t bitPos = RADIOLIB_PAGER_CODE_WORD_LEN - 1 - symbolLength;
//        Serial.println("GOT DATA.");
        Serial.printf("RAW CW %X \n", cw);
        if (overflow) {
            overflow = false;

            // this is a bit convoluted - first, build masks for both previous and current code word
            uint8_t currPos = RADIOLIB_PAGER_CODE_WORD_LEN - 1 - symbolLength + ovfBits;
            uint8_t prevPos = RADIOLIB_PAGER_MESSAGE_END_POS;
            uint32_t prevMask =
                    (0x7FUL << prevPos) & ~((uint32_t) 0x7FUL << (RADIOLIB_PAGER_MESSAGE_END_POS + ovfBits));
            uint32_t currMask = (0x7FUL << currPos) & ~((uint32_t) 1 << (RADIOLIB_PAGER_CODE_WORD_LEN - 1));

            // next, get the two parts of the message symbol and stick them together
            uint8_t prevSymbol = (prevCw & prevMask) >> prevPos;
            uint8_t currSymbol = (cw & currMask) >> currPos;
            uint32_t symbol = prevSymbol << (symbolLength - ovfBits) | currSymbol;

            // finally, we can flip the bits
            symbol = Module::reflect((uint8_t) symbol, 8);
            symbol >>= (8 - symbolLength);

            // decode BCD and we're done
            if (symbolLength == 4) {
                symbol = decodeBCD(symbol);
            }
//            Serial.printf("DE LEN %d \n",decodedBytes);
            data[decodedBytes++] = symbol;
            deco++;

            // adjust the bit position of the next message symbol
            bitPos += ovfBits;
            bitPos -= symbolLength;
        }

        // get the message symbols based on the encoding type
        while (bitPos >= RADIOLIB_PAGER_MESSAGE_END_POS) {
            // get the message symbol from the code word and reverse bits
            uint32_t symbol = (cw & (0x7FUL << bitPos)) >> bitPos;
            symbol = Module::reflect((uint8_t) symbol, 8);
            symbol >>= (8 - symbolLength);

            // decode BCD if needed
            if (symbolLength == 4) {
                symbol = decodeBCD(symbol);
            }
//            Serial.printf("DE LEN %d \n",decodedBytes);
            data[decodedBytes++] = symbol;

            // now calculate if the next symbol is overflowing to the following code word
            int8_t remBits = bitPos - RADIOLIB_PAGER_MESSAGE_END_POS;
            if (remBits < symbolLength) {
                // overflow!
                prevCw = cw; // todo: consider cw_prev?
                overflow = true;
                ovfBits = remBits;
            }
            bitPos -= symbolLength;
        }

    }

    // save the number of decoded bytes
    end:
    *len = decodedBytes;
    *errs_total = errors;
    return (RADIOLIB_ERR_NONE);
}

bool PagerClient::gotPreambleState() {
    if (phyLayer->gotPreamble) {
        phyLayer->gotPreamble = false;
        phyLayer->preambleBuffer = 0;
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

int16_t readPacketData(struct PagerClient::pocsag_data *p, const uint8_t *data) {
    uint32_t data32[16];
    uint8_t c = 0;
    for (unsigned int &i: data32) {
        i = data[c] << 24 | data[c + 1] << 16 | data[c + 2] << 8 | data[c + 3];
        i = ~i;
        c += 4;
    }

    c = 0;
    uint8_t framePos = 0;
    // uint16_t errors = 0;
    bool is_sync = true;
    CBCH3121 bch;
    // uint16_t errors_prev = 0;
    for (size_t i = 0; i < 16; i++) {
        // Serial.printf("DATA %x\n",data32[i]);
        bool parity = true;
        uint16_t errors = 0;
        if (bch.decode(data32[i], errors, parity) && parity) {
            // p[c].errs_total += errors;
            // Serial.printf("BCH CORRECT\n");
            is_sync = true;
        } else {
            if (!is_sync) {
                break;
            }
            is_sync = false;
            // p[c].errs_total += errors;
            // p[c].errs_uncorrected += errors;
            // Serial.printf("BCH FAILED\n");
        }

        if (data32[i] == RADIOLIB_PAGER_IDLE_CODE_WORD) {
            break;
        }
        if (data32[i] == RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD) {
            framePos = 0;
            c++;
            continue;
        }

        if (data32[i] >> 31 == 0) { // Address
            // if (!is_sync && c == 0)
            //     return (RADIOLIB_ERR_MSG_CORRUPT);

            if (!p[c].is_empty)
                c++;
            // p[c].is_empty = false;
            if (is_sync) {
                p[c].addr = ((data32[i] << 1) >> 14) << 3 | framePos/2;
                p[c].func = (data32[i] << 19) >> (11 + 19);
            }
        }

        if (data32[i] >> 31 == 1) { // Data
            p[c].errs_total += errors;
            if (!is_sync) {
                for (uint8_t v = 0; v < 5; v++) {
                    // p[c].data[p[c].len++] = 'X';
                    p[c].str += 'X';
                    p[c].len++;
                }
                p[c].errs_uncorrected += errors;
                continue;
            }

            if (p[c].is_empty)
                p[c].is_empty = false;

            char bcd[6];
            decode20(data32[i], bcd);
            // Serial.printf("DECO %s\n",bcd);
            p[c].str += bcd;
            p[c].len += 5;

        }
        // p[c].len += 32;
        framePos++;
    }

    bool empty = true;
    for (uint8_t i=0;i<POCDAT_SIZE;i++){
        if (!p[i].is_empty) {
            empty = false;
            break;
        }
    }
    if (empty)
        return RADIOLIB_ERR_MSG_CORRUPT;


    // int16_t state = RADIOLIB_ERR_NONE;
    return RADIOLIB_ERR_NONE;
}
//int16_t do_one_bit(){
//
//}