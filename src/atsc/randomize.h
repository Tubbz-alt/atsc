#pragma once
#include <cassert>
#include <cstring>
#include "common/atsc_parameters.h"

template<typename T>
class atsc_randomize {
public:
    constexpr atsc_randomize() {}

private:
    struct rnd_initializer {
        static constexpr uint16_t initial_state = 0xf180;
        static constexpr unsigned random_data = ATSC_SEGMENT_BYTES * ATSC_DATA_SEGMENTS;
        static constexpr uint32_t generator = 0x9c65;   // x16 x13 x12 x11 x7 x6 x3 x1

        constexpr rnd_initializer() : table() {
            uint32_t state = initial_state;
            for (size_t i = 0; i < random_data; i++) {
                uint16_t output = 0;
                output  = (state & 0x3c00) >> 6;
                output |= (state & 0x0040) >> 3;
                output |= (state & 0x000c) >> 1;
                output |= (state & 0x0001) >> 0;

                table[i] = output;

                // advance state
                state <<= 1;
                if (state & 0x10000) {
                    state ^= (generator << 1) | 1;
                }
            }
        }
        std::array<uint16_t, random_data> table;
    };

public:
    uint8_t randomize(uint8_t data, unsigned index) {
        return data ^ table[index];
    }

    void randomize_pkts(atsc_field_data& output, atsc_field_mpeg2& input) {
        unsigned rindex = 0;
        unsigned oindex = 0;
        for (unsigned packet = 0; packet < ATSC_DATA_SEGMENTS; packet++) {
            for (unsigned index = 0; index < ATSC_SEGMENT_BYTES; index++) {
                unsigned iindex = packet * ATSC_MPEG2_BYTES + index + 1;
                output[oindex + index] = randomize(input[iindex], rindex++);
            }
            memset(&output[oindex + ATSC_SEGMENT_BYTES], 0, ATSC_RS_BYTES);
            oindex += ATSC_SEGMENT_FEC_BYTES;
        }
    }

public:
    static inline constexpr std::array<uint16_t, rnd_initializer::random_data> table = rnd_initializer().table;
};


