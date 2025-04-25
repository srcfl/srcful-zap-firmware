#include <unity.h>
#include "json_light/json_light.h"
#include <vector>
#include <Arduino.h>

void setUp(void) {
    // Setup code if needed
}

void tearDown(void) {
    // Cleanup code if needed
}

void test_json_builder() {
    JsonBuilder json;
    uint8_t data [] = {0x01, 0xff, 0x00};
    zap::Str result = json
        .beginObject()
        .add("status", "success")
        .add("code", 200)
        .add("enabled", true)
        .add("hex", data, sizeof(data))
        .end();
    
    TEST_ASSERT_EQUAL_STRING(
        "{\"status\":\"success\",\"code\":200,\"enabled\":true,\"hex\":\"01ff00\"}",
        result.c_str()
    );
}

void test_json_parser() {
    const char* json = "{\"status\":\"success\",\"code\":200,\"enabled\":true}";
    JsonParser parser(json);
    
    char status[32];
    int code;
    bool enabled;
    
    // Add debug output
    Serial.println("Testing JSON parser with:");
    Serial.println(json);
    
    bool result = parser.getString("status", status, sizeof(status));
    Serial.print("getString result: ");
    Serial.println(result ? "true" : "false");
    Serial.print("status value: ");
    Serial.println(status);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("success", status);
    
    TEST_ASSERT_TRUE(parser.getInt("code", code));
    TEST_ASSERT_EQUAL_INT(200, code);
    
    TEST_ASSERT_TRUE(parser.getBool("enabled", enabled));
    TEST_ASSERT_TRUE(enabled);
}

void test_json_array() {
    JsonBuilder json;
    const char* items[] = {"item1", "item2", "item3"};
    zap::Str result = json
        .beginObject()
        .add("name", "test")
        .addArray("items", items, 3)
        .end();
    
    TEST_ASSERT_EQUAL_STRING(
        "{\"name\":\"test\",\"items\":[\"item1\",\"item2\",\"item3\"]}",
        result.c_str()
    );
}

void test_json_parser_reset() {
    const char* json = "{\"status\":\"success\",\"code\":200}";
    JsonParser parser(json);
    
    char status[32];
    int code;
    
    TEST_ASSERT_TRUE(parser.getString("status", status, sizeof(status)));
    TEST_ASSERT_TRUE(parser.getInt("code", code));
}

void test_json_builder_simple_object() {
    JsonBuilder builder;
    builder.beginObject()
        .add("key1", "value1")
        .add("key2", 123)
        .add("key3", true);
    
    zap::Str json = builder.end();
    TEST_ASSERT_EQUAL_STRING("{\"key1\":\"value1\",\"key2\":123,\"key3\":true}", json.c_str());
}

void test_json_builder_nested_objects() {
    JsonBuilder builder;
    builder.beginObject()
        .add("name", "Device")
        .beginObject("config")
            .add("enabled", true)
            .add("interval", 5000)
            .beginObject("network")
                .add("ssid", "TestNetwork")
                .add("security", "WPA2")
            .endObject()
        .endObject()
        .add("version", "1.0");
    
    zap::Str json = builder.end();
    TEST_ASSERT_EQUAL_STRING(
        "{\"name\":\"Device\",\"config\":{\"enabled\":true,\"interval\":5000,\"network\":{\"ssid\":\"TestNetwork\",\"security\":\"WPA2\"}},\"version\":\"1.0\"}", 
        json.c_str()
    );
}

void test_json_parser_request() {
    // const char * str = "{\"id\": \"MDasHAlXxnrp3HKKzTbwr\", \"body\": \"Hello World!\", \"path\": \"/api/echo\", \"query\": \"{}\", \"method\": \"POST\", \"headers\": \"{}\", \"timestamp\": 1745405662168}";
    const char * str = "{\"id\": \"MDasHAlXxnrp3HKKzTbwr\", \"body\": \"Hello World!\"}";


    JsonParser parser(str);

    // assert((parser.contains("id") && parser.contains("path") && parser.contains("method")));

    zap::Str body; parser.getString("body", body);
    TEST_ASSERT_EQUAL_STRING("Hello World!", body.c_str());


    zap::Str id; parser.getString("id", id);
    TEST_ASSERT_EQUAL_STRING("MDasHAlXxnrp3HKKzTbwr", id.c_str());


    // zap::Str path; parser.getString("path", path);
    // zap::Str method; parser.getString("method", method);
    
    // zap::Str query; parser.getString("query", query);
    // zap::Str headers; parser.getString("headers", headers);
    // uint64_t timestamp; parser.getUInt64("timestamp", timestamp);

    // TEST_ASSERT_EQUAL_STRING("/api/echo", path.c_str());
    // TEST_ASSERT_EQUAL_STRING("POST", method.c_str());
    // TEST_ASSERT_EQUAL_STRING("{}", query.c_str());
    // TEST_ASSERT_EQUAL_STRING("{}", headers.c_str());
    // TEST_ASSERT_EQUAL_UINT64(1745405662168, timestamp);


}

void test_json_builder_array() {
    JsonBuilder builder;
    std::vector<zap::Str> items = {"item1", "item2", "item3"};
    
    builder.beginObject()
        .add("title", "Test Array")
        .addArray("items", items);
    
    zap::Str json = builder.end();
    TEST_ASSERT_EQUAL_STRING("{\"title\":\"Test Array\",\"items\":[\"item1\",\"item2\",\"item3\"]}", json.c_str());
}

void test_all_json_light() {
    
    RUN_TEST(test_json_builder);
    RUN_TEST(test_json_parser);
    RUN_TEST(test_json_array);
    RUN_TEST(test_json_parser_reset);
    RUN_TEST(test_json_builder_simple_object);
    RUN_TEST(test_json_builder_nested_objects);
    RUN_TEST(test_json_builder_array);
    RUN_TEST(test_json_parser_request);
    
}