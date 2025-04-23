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

    int test_json_parser_sub_object() {
        // Test JSON light parsing with sub-object
        const char* json = "{\"key\": {\"subkey\": \"subvalue\"}}";
        JsonParser parser(json);
        JsonParser subParser("");
        zap::Str ret;
        assert(parser.getObject("key", subParser));
        assert(subParser.getString("subkey", ret));

        assert(parser.getStringByPath("key.subkey", ret));
        assert(ret == "subvalue");

        return 0;
    }

    int test_json_parser_sub_sub_object() {
        // Test JSON light parsing with sub-object
        const char* json = "{\"key\": {\"subkey\": {\"subsubkey\": \"subvalue\"}, \"key2\": 17}";
        JsonParser parser(json);
        JsonParser subParser("");
        zap::Str ret;
        assert(parser.getObject("key", subParser));
        assert(!subParser.getString("subsubkey", ret));

        assert(subParser.getObject("subkey", subParser));
        assert(subParser.getString("subsubkey", ret));
        assert(ret == "subvalue");

        assert(parser.getStringByPath("key.subkey.subsubkey", ret));

        assert(!parser.getStringByPath("poop.subkey.subsubkey", ret));

        int value;
        assert(parser.getIntByPath("key.key2", value));
        assert(value == 17);
        

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

    int test_json_parser_request() {
        const char * str = "{\"id\": \"MDasHAlXxnrp3HKKzTbwr\", \"body\": \"Hello World!\", \"path\": \"/api/echo\", \"query\": \"{}\", \"method\": \"POST\", \"headers\": \"{}\", \"timestamp\": 1745405662168}";


        JsonParser parser(str);

        assert((parser.contains("id") && parser.contains("path") && parser.contains("method")));

        zap::Str id; parser.getString("id", id);
        zap::Str path; parser.getString("path", path);
        zap::Str method; parser.getString("method", method);
        zap::Str body; parser.getString("body", body);
        zap::Str query; parser.getString("query", query);
        zap::Str headers; parser.getString("headers", headers);
        uint64_t timestamp; parser.getUInt64("timestamp", timestamp);

        assert(id == "MDasHAlXxnrp3HKKzTbwr");
        assert(path == "/api/echo");
        assert(method == "POST");
        assert(body == "Hello World!");
        assert(query == "{}");
        assert(headers == "{}");
        assert(timestamp == 1745405662168);

        return 0;

    }

    int run() {
        test_json_parser();
        test_json_builder();
        test_json_parser_sub_object();
        test_json_parser_sub_sub_object();
        return 0;
    }
    
}