#pragma once
#include <cstring>
#include "common/atsc_parameters.h"
#include "common/lfsr.h"
#include "signal.h"

template<typename T>
struct atsc_field_sync {

    atsc_field_sync() : current_sync(&field_sync_even), next_sync(&field_sync_odd) {}

    void process_field(atsc_field_symbol_padded& field, atsc_reserved_symbols& saved_symbols) {
        const field_sync_array& field_sync = *current_sync;

        memcpy(field.data(), field_sync.data(), field_sync.size() * sizeof(atsc_symbol_type));
        memcpy(field.data() + ATSC_SYMBOLS_PER_FIELD, field_sync.data(), field_sync.size() * sizeof(atsc_symbol_type));
        memcpy(field.data() + ATSC_SYMBOLS_PER_SEGMENT - ATSC_RESERVED_SYMBOLS, saved_symbols.data(), ATSC_RESERVED_SYMBOLS * sizeof(atsc_symbol_type));

        for (size_t i = ATSC_SYMBOLS_PER_SEGMENT; i < ATSC_SYMBOLS_PER_FIELD; i += ATSC_SYMBOLS_PER_SEGMENT) {
            memcpy(field.data() + i, segment_sync.data(), segment_sync.size() * sizeof(atsc_symbol_type));
        }

        memcpy(saved_symbols.data(), field.data() + ATSC_SYMBOLS_PER_FIELD - ATSC_RESERVED_SYMBOLS, ATSC_RESERVED_SYMBOLS * sizeof(atsc_symbol_type));

        std::swap(current_sync, next_sync);
    }

private:
    using xformer = atsc_symbol_to_signal<atsc_symbol_type>;
    struct segment_sync_generator {
        constexpr segment_sync_generator() : table() {
            table[0] = xformer::xform(6);
            table[1] = xformer::xform(1);
            table[2] = xformer::xform(1);
            table[3] = xformer::xform(6);
        }
        std::array<atsc_symbol_type, 4> table;
    };

    struct field_sync_generator {

        constexpr field_sync_generator(bool even, std::array<uint8_t, 511> pn511, std::array<uint8_t, 63> pn63) : table() {
            table[0] = xformer::xform(6);
            table[1] = xformer::xform(1);
            table[2] = xformer::xform(1);
            table[3] = xformer::xform(6);

            size_t sym = 4;
            for (size_t i = 0; i < pn511.size(); i++, sym++) {
                table[sym] = xformer::xform(pn511[i] ? 6 : 1);
            }
            for (size_t i = 0; i < pn63.size(); i++, sym++) {
                table[sym] = xformer::xform(pn63[i] ? 6 : 1);
            }
            for (size_t i = 0; i < pn63.size(); i++, sym++) {
                table[sym] = xformer::xform((!!pn63[i] == even) ? 6 : 1);
            }
            for (size_t i = 0; i < pn63.size(); i++, sym++) {
                table[sym] = xformer::xform(pn63[i] ? 6 : 1);
            }

            std::array<uint8_t, 24> vsb_mode = {0,0,0,0, 1,0,1,0, 0,1,0,1, 1,1,1,1, 0,1,0,1, 1,0,1,0};
            for (size_t i = 0; i < vsb_mode.size(); i++, sym++) {
                table[sym] = xformer::xform(vsb_mode[i] ? 6 : 1);
            }

            for (size_t i = 0; i < 104 - ATSC_RESERVED_SYMBOLS; i++, sym++) {
                table[sym] = xformer::xform(pn63[i % pn63.size()] ? 6 : 1);
            }
        }

        std::array<atsc_symbol_type, ATSC_SYMBOLS_PER_SEGMENT - ATSC_RESERVED_SYMBOLS> table;
    };

    static inline constexpr std::array<atsc_symbol_type, 4> segment_sync = segment_sync_generator().table;

    using field_sync_array = std::array<atsc_symbol_type, ATSC_SYMBOLS_PER_SEGMENT - ATSC_RESERVED_SYMBOLS>;
    static inline constexpr field_sync_array field_sync_even = 
        field_sync_generator(
            true,
            lfsr<0b10, 8, 0b110110110, 511>::table,
            lfsr<0b111001, 5, 0b110000, 63>::table
        ).table;
    static inline constexpr field_sync_array field_sync_odd = 
        field_sync_generator(
            false,
            lfsr<0b10, 8, 0b110110110, 511>::table,
            lfsr<0b111001, 5, 0b110000, 63>::table
        ).table;

    const field_sync_array* current_sync;
    const field_sync_array* next_sync;
};