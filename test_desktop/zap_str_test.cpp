#include "../src/zap_str.h"

#include <assert.h>

namespace zap_str_test {

    int test_replace() {
        zap::Str str("Hello, \\u0022World!\\u0022");
        str.replace("\\u0022", "\"");
        str.replace("World", "Zap");
        assert(str == "Hello, \"Zap!\"");
        return 0;
    }

    int test_complex_replace() {
        zap::Str str("{\"data\":{\"gatewayConfiguration\":{\"configuration\":{\"data\":\"{\\u0022status\\u0022: {\\u0022uptime\\u0022: 13615, \\u0022version\\u0022: \\u00221.0.3\\u0022}, \\u0022timestamp\\u0022: 1745331729711}\"}}}}");
    
        str.replace("\\u0022", "\"");

        assert(str == "{\"data\":{\"gatewayConfiguration\":{\"configuration\":{\"data\":\"{\"status\": {\"uptime\": 13615, \"version\": \"1.0.3\"}, \"timestamp\": 1745331729711}\"}}}}");

        return 0;
    }

    int run(){
        
        test_replace();

        return 0;
    }
}