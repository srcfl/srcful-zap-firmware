#include "../src/json_light/json_light.h"

#include <assert.h>

namespace json_light_test {

    int test_json_parser() {
        // Test JSON light parsing
        const char* json = "{\"key\": \"value\"}";
        JsonParser parser(json);
        zap::Str ret;
        assert(parser.getString("key", ret));
        assert(ret == "value");

        return 0;
    }

    int test_json_builder() {
        // Test JSON light building
        JsonBuilder builder;
        std::vector<zap::Str> arr;
        uint8_t data [] = {0x01, 0xff, 0x00};

        arr.push_back("item1");
        arr.push_back("item2");

        builder.add("key", "value");
        builder.add("number", 42);
        builder.add("boolean", true);
        builder.addArray("array", arr);
        builder.add("hex", data, sizeof(data));
        zap::Str json = builder.end();
        
        assert(json.indexOf("\"key\":\"value\"") != -1);
        assert(json.indexOf("\"number\":42") != -1);
        assert(json.indexOf("\"boolean\":true") != -1);
        assert(json.indexOf("\"array\":[\"item1\",\"item2\"]") != -1);

        assert(json.indexOf("\"hex\":\"01ff00\"") != -1);

        return 0;
    }

    int run() {
        test_json_parser();
        test_json_builder();
        return 0;
    }
    
}