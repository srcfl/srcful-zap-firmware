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

    int test_json_parser_get_object_by_path() {
        // Test JSON light parsing with sub-object
        const char* json = "{\"key\": {\"subobject\": {\"subsubobject\": {\"subsubkey\": \"hello\"}, \"subsubkey\": \"world\"}, \"key2\": 17}";
        JsonParser parser(json);
        JsonParser subParser("");
        zap::Str ret;
        assert(parser.getObjectByPath("key.subobject", subParser));
        assert(subParser.getString("subsubkey", ret));
        assert(ret == "world");

        assert(parser.getObjectByPath("key.subobject.subsubobject", subParser));
        assert(subParser.getString("subsubkey", ret));
        assert(ret == "hello");


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
        const char * str = "{\"id\": \"MDasHAlXxnrp3HKKzTbwr\", \"body\": \"Hello World!\"}";


        JsonParser parser(str);
    
        // assert((parser.contains("id") && parser.contains("path") && parser.contains("method")));
    
        zap::Str body; parser.getString("body", body);
        assert(body == "Hello World!");
    
    
        zap::Str id; parser.getString("id", id);
        assert(id == "MDasHAlXxnrp3HKKzTbwr");

        return 0;
    }

    int test_json_parser_value_with_curly_brace() {
        const char * str = "{\"id\": \"\\u0022\", \"body\": \"Hello World!\"}";


        JsonParser parser(str);
    
        // assert((parser.contains("id") && parser.contains("path") && parser.contains("method")));
    
        zap::Str body;
        assert(parser.getString("body", body));
        assert(body == "Hello World!");
    
    
        zap::Str id; parser.getString("id", id);
        assert(id == "\\u0022");

        return 0;
    }

    int test_json_parser_test_websocket_data() {
        const char* str = "{\"type\":\"data\",\"id\":\"1\",\"payload\":{\"data\":{\"configurationDataChanges\":{\"data\":\"{\\u0022id\\u0022: \\u0022njnMiKW6PmcVxxZOp-ErA\\u0022, \\u0022body\\u0022: \\u0022Wabisabi\\u0022, \\u0022path\\u0022: \\u0022/api/echo\\u0022, \\u0022query\\u0022: \\u0022{}\\u0022, \\u0022method\\u0022: \\u0022POST\\u0022, \\u0022headers\\u0022: \\u0022{}\\u0022, \\u0022timestamp\\u0022: 1745506313254}\",\"subKey\":\"request\"}}}}";
        

        JsonParser parser(str);
        zap::Str res;

        assert(parser.getString("type", res));
        assert(res == "data");
        JsonParser subParser("");
        assert(parser.getObjectByPath("payload.data.configurationDataChanges", subParser));
        assert(subParser.getString("subKey", res));
        assert(res == "request");

        return 0;
    }

    int run() {
        test_json_parser();
        test_json_parser_value_with_curly_brace();
        test_json_builder();
        test_json_parser_sub_object();
        test_json_parser_sub_sub_object();
        test_json_parser_request();
        test_json_parser_get_object_by_path();
        test_json_parser_test_websocket_data();
        return 0;
    }
    
}