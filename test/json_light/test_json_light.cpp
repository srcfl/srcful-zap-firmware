#include <unity.h>
#include "json_light/json_light.h"

void setUp(void) {
    // Setup code if needed
}

void tearDown(void) {
    // Cleanup code if needed
}

void test_json_builder() {
    JsonBuilder json;
    String result = json
        .beginObject()
        .add("status", "success")
        .add("code", 200)
        .add("enabled", true)
        .end();
    
    TEST_ASSERT_EQUAL_STRING(
        "{\"status\":\"success\",\"code\":200,\"enabled\":true}",
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
    String result = json
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
    
    // Reset and parse again
    parser.reset();
    TEST_ASSERT_TRUE(parser.getString("status", status, sizeof(status)));
    TEST_ASSERT_TRUE(parser.getInt("code", code));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_json_builder);
    RUN_TEST(test_json_parser);
    RUN_TEST(test_json_array);
    RUN_TEST(test_json_parser_reset);
    
    return UNITY_END();
} 