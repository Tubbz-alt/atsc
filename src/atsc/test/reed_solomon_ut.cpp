#include "catch2/catch.hpp"
#include "../reed_solomon.h"
#include "vector_data.h"
#include <cstring>

void generate_test_data() {

    atsc_reed_solomon<atsc_parameters> rs;

    auto input = load_vector_data<atsc_parameters::atsc_field_data>("reed_solomon_input.data");
    for (size_t i = 0; i < atsc_parameters::ATSC_DATA_SEGMENTS; i++) {
        memset(input->data() + atsc_parameters::ATSC_SEGMENT_FEC_BYTES * i + atsc_parameters::ATSC_SEGMENT_BYTES, 0, atsc_parameters::ATSC_RS_BYTES);
    }
    save_vector_data("reed_solomon_input.data", input.get());

    rs.process_field(input->data());

    save_vector_data("reed_solomon_output.data", input.get());
}

TEST_CASE("ATSC Reed Solomon", "[rs_encode]") {

    atsc_reed_solomon<atsc_parameters> rs;

    auto input = load_vector_data<atsc_parameters::atsc_field_data>("reed_solomon_input.data");
    auto output = load_vector_data<atsc_parameters::atsc_field_data>("reed_solomon_output.data");

    rs.process_field(input->data());
    for (size_t i = 0; i < input->size(); i++) {
        REQUIRE((*input)[i] == (*output)[i]);
    }
}