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

    int test_json_parser_asString() {
        // Test JSON light parsing
        const char* json = "{\"key\": \"value\"}";
        JsonParser parser(json);
        zap::Str ret;
        parser.asString(ret);
        assert(ret == "{\"key\": \"value\"}");
        
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

        builder.beginObject();
        builder.add("key", "value");
        builder.add("number", 42);
        builder.add("boolean", true);
        builder.addArray("array", arr);
        builder.add("hex", data, sizeof(data));
        zap::Str json = builder.end();
        
        assert(json.indexOf("{") == 0);
        assert(json.indexOf("}") == json.length() - 1);
        assert(json.indexOf("\"key\":\"value\"") != -1);
        assert(json.indexOf("\"number\":42") != -1);
        assert(json.indexOf("\"boolean\":true") != -1);
        assert(json.indexOf("\"array\":[\"item1\",\"item2\"]") != -1);

        assert(json.indexOf("\"hex\":\"01ff00\"") != -1);

        return 0;
    }

    int test_fixed_builder() {
        // Test JSON light building
        char buffer[1024];
        JsonBuilderFixed builder(buffer, sizeof(buffer));
        std::vector<zap::Str> arr;
        uint8_t data [] = {0x01, 0xff, 0x00};

        arr.push_back("item1");
        arr.push_back("item2");

        builder.beginObject();
        builder.add("key", "value");
        builder.add("number", 42);
        builder.add("boolean", true);
        builder.addArray("array", arr);
        builder.add("hex", data, sizeof(data));
        zap::Str json = builder.end();
        
        assert(json.indexOf("{") == 0);
        assert(json.indexOf("}") == json.length() - 1);
        assert(json.indexOf("\"key\":\"value\"") != -1);
        assert(json.indexOf("\"number\":42") != -1);
        assert(json.indexOf("\"boolean\":true") != -1);
        assert(json.indexOf("\"array\":[\"item1\",\"item2\"]") != -1);

        assert(json.indexOf("\"hex\":\"01ff00\"") != -1);

        return 0;
    }

    int test_fixed_builder_buffer_overflow() {
        // Test JSON light building
        char buffer[16];
        JsonBuilderFixed builder(buffer, sizeof(buffer));
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
        
        assert(builder.hasOverflow() == true);

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
        assert(id == "\""); // in json \u0022 is translated to "

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

    int test_json_parser_test_websocket_wifi_request() {
        // this is in some respect more a test of GraphQLSubscriptionClient::handleRequestTask and handleRequest
        const char* str =  "{\\u0022id\\u0022: \\u0022some-id\\u0022, \\u0022body\\u0022: {\\u0022psk\\u0022: \\u0022bamse-zorba\\u0022, \\u0022ssid\\u0022: \\u0022eather\\u0022}, \\u0022path\\u0022: \\u0022/api/wifi\\u0022, \\u0022query\\u0022: \\u0022{}\\u0022, \\u0022method\\u0022: \\u0022POST\\u0022, \\u0022headers\\u0022: \\u0022{}\\u0022, \\u0022timestamp\\u0022: 1746111306022}";
        zap::Str data = zap::Str(str);

        data.replace("\\u0022", "\"");

        JsonParser configData(data.c_str());

        zap::Str id; configData.getString("id", id);
        zap::Str path; configData.getString("path", path);
        zap::Str method; configData.getString("method", method);

        zap::Str body; assert(!configData.getString("body", body));          // this is probably an object
        JsonParser bodyParser(""); assert(configData.getObject("body", bodyParser));

        zap::Str query; configData.getString("query", query);
        zap::Str headers; configData.getString("headers", headers);
        uint64_t timestamp; configData.getUInt64("timestamp", timestamp);

        assert(id == "some-id");
        assert(path == "/api/wifi");
        assert(method == "POST");
        assert(body == "{\"psk\":\"bamse-zorba\",\"ssid\":\"eather\"}");
        assert(query == "{}");
        assert(headers == "{}");
        assert(timestamp == 1746111306022);

        assert(false);
        return 0;
    }

    int test_json_parser_test_websocket_ota_request() {
        const char* str = "{\\u0022id\\u0022: \\u0022684c16c7-fcfd-4fe7-818e-802744e88b7c\\u0022, \\u0022body\\u0022: \\u0022{\\\\u0022url\\\\u0022: \\\\u0022https://github.com/srcfl/srcful-zap-firmware/raw/refs/heads/main/test_build/firmware_0_0_2.bin\\\\u0022, \\\\u0022version\\\\u0022: \\\\u00220.0.2\\\\u0022}\\u0022, \\u0022path\u0022: \\u0022/api/ota/update\\u0022, \\u0022query\\u0022: \\u0022{}\\u0022, \\u0022method\\u0022: \\u0022POST\\u0022, \\u0022headers\\u0022: \\u0022{}\\u0022, \\u0022timestamp\\u0022: 1746167058815}";
    
        zap::Str data = zap::Str(str);

        data.replace("\\u0022", "\"");

        JsonParser configData(data.c_str());

        zap::Str id; configData.getString("id", id);
        zap::Str path; configData.getString("path", path);
        zap::Str method; configData.getString("method", method);

        zap::Str body; assert(!configData.getString("body", body));          // this is probably an object
        JsonParser bodyParser(""); assert(configData.getObject("body", bodyParser));

        zap::Str query; configData.getString("query", query);
        zap::Str headers; configData.getString("headers", headers);
        uint64_t timestamp; configData.getUInt64("timestamp", timestamp);

        assert(id == "684c16c7-fcfd-4fe7-818e-802744e88b7c");
        assert(path == "/api/ota/update");
        assert(method == "POST");
        assert(query == "{}");
        assert(headers == "{}");
        assert(timestamp == 1746111306022);

        assert(false);
        return 0;
    }

    int run() {
        test_json_parser();
        test_json_parser_asString();
        test_json_parser_value_with_curly_brace();
        test_json_builder();
        test_fixed_builder();
        test_fixed_builder_buffer_overflow();
        test_json_parser_sub_object();
        test_json_parser_sub_sub_object();
        test_json_parser_request();
        test_json_parser_get_object_by_path();
        test_json_parser_test_websocket_data();
        // test_json_parser_test_websocket_ota_request();
        return 0;
    }
    
}