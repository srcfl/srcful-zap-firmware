#include "../src/json_light/json_light.h"

namespace json_light_test {

    int test_json_light() {
        // Test JSON light parsing
        const char* json = "{\"key\": \"value\"}";
        JsonLightParser parser;
        JsonLightData data = parser.parse(json);
        
        assert(data.getValue("key") == "value");
        
        return 0;
    }

    int run() {
        test_json_light();
        return 0;
    }
    
}