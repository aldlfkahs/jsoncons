// Copyright 2016 Daniel Parker
// Distributed under Boost license

#include <cassert>
#include <string>
#include <vector>
#include <list>
#include <iomanip>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/csv/csv.hpp>
#include <jsoncons_ext/bson/bson.hpp>
#include <jsoncons_ext/cbor/cbor.hpp>
#include <jsoncons_ext/msgpack/msgpack.hpp>
#include <jsoncons_ext/ubjson/ubjson.hpp>

using namespace jsoncons;

namespace {

    using qualifying_results_type = std::tuple<std::size_t,std::string,std::string,std::string,std::chrono::milliseconds>;

    void tuple_example()
    {
        std::vector<qualifying_results_type> results = {
            {1,"Lewis Hamilton","Mercedes","1'24.303",std::chrono::milliseconds(0)},
            {2,"Valtteri Bottas","Mercedes","1'24.616",std::chrono::milliseconds(313)},
            {3,"Max Verstappen","Red Bull","1'25.325",std::chrono::milliseconds(1022)}
        };

        std::string json_data;
        encode_json_pretty(results, json_data);
        std::cout << json_data << "\n\n";
        auto results1 = decode_json<std::vector<qualifying_results_type>>(json_data);
        assert(results1 == results);

        csv::csv_options options2;
        options2.column_names("Pos,Driver,Entrant,Time,Gap");
        options2.mapping(csv::mapping_kind::n_rows);
        std::string csv_data;
        options2.header_lines(1);
        csv::encode_csv(results, csv_data, options2);
        std::cout << csv_data << "\n\n";
        auto results2 = csv::decode_csv<std::vector<qualifying_results_type>>(csv_data, options2);
        assert(results2 == results);

        std::vector<uint8_t> bson_data;
        bson::encode_bson(results, bson_data);
        auto results3 = bson::decode_bson<std::vector<qualifying_results_type>>(bson_data);
        assert(results3 == results);

        std::vector<uint8_t> cbor_data;
        cbor::encode_cbor(results, cbor_data);
        auto results4 = cbor::decode_cbor<std::vector<qualifying_results_type>>(cbor_data);
        assert(results4 == results);

        std::vector<uint8_t> msgpack_data;
        msgpack::encode_msgpack(results, msgpack_data);
        auto results5 = msgpack::decode_msgpack<std::vector<qualifying_results_type>>(msgpack_data);
        assert(results5 == results);

        std::vector<uint8_t> ubjson_data;
        ubjson::encode_ubjson(results, ubjson_data);
        auto results6 = ubjson::decode_ubjson<std::vector<qualifying_results_type>>(ubjson_data);
        assert(results6 == results);
    }

} // namespace

void json_type_traits_tuple_examples()
{
    std::cout << "\njson_type_traits tuple examples\n\n";

#if defined(JSONCONS_HAS_STD_VARIANT)
    tuple_example();
#endif

    std::cout << std::endl;
}

