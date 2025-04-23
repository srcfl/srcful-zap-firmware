#include <assert.h>

#include "../src/backend/graphql.h"
#include "../src/json_light/json_light.h"
#include <HTTPClient.h>

namespace graphql_test {

    int test_setConfigruation_success() {
        // set up the mock response
        WiFiClient::read_buffer = (char*)R"({
                                        "data": {
                                            "setConfiguration": {
                                            "success": true
                                            }
                                        }
                                    })";

        GQL::BoolResponse response = GQL::setConfiguration("magic_jwt_token");
        WiFiClient::read_buffer = nullptr; // reset the mock response
        assert(response.isSuccess());
        assert(response.data == true);

        return 0;
    }

    int test_getConfiguration_success() {
        // set up the mock response
        WiFiClient::read_buffer = (char*)"{\"data\":{\"gatewayConfiguration\":{\"configuration\":{\"data\":\"{\\u0022status\\u0022: {\\u0022uptime\\u0022: 13615, \\u0022version\\u0022: \\u00221.0.3\\u0022}, \\u0022timestamp\\u0022: 1745331729711}\"}}}}";

        GQL::StringResponse response = GQL::getConfiguration("state");
        WiFiClient::read_buffer = nullptr; // reset the mock response
        assert(response.isSuccess());
        assert(response.data == "{\"status\": {\"uptime\": 13615, \"version\": \"1.0.3\"}, \"timestamp\": 1745331729711}");

        JsonParser parser(response.data.c_str());
        int uptime;
        // assert(parser.getIntByPath("status.uptime", uptime));
        // assert(uptime == 18671904);

        return 0;
    }

    int test_getConfiguration_null_data() {
        // set up the mock response
        WiFiClient::read_buffer = (char*)"{\"data\":{\"gatewayConfiguration\":{\"configuration\":{\"data\":null}}}}";

        GQL::StringResponse response = GQL::getConfiguration("state");
        WiFiClient::read_buffer = nullptr; // reset the mock response
        assert(response.isSuccess());
        assert(response.data == "");

        JsonParser parser(response.data.c_str());
        int uptime;
        // assert(parser.getIntByPath("status.uptime", uptime));
        // assert(uptime == 18671904);

        return 0;
    }

    int test_fetchGatewayName_success() {
        // set up the mock response
        WiFiClient::read_buffer = (char*)R"({
                                        "data": {
                                            "gatewayConfiguration": {
                                                "gatewayName": {
                                                    "name": "Mors Lilla Olle"
                                                }
                                            }
                                        }
                                    })";

        GQL::StringResponse response = GQL::fetchGatewayName("fake_serial_number");
        WiFiClient::read_buffer = nullptr; // reset the mock response
        assert(response.isSuccess());
        assert(response.data == "Mors Lilla Olle");

        return 0;
    }

    int run(){
        
        test_setConfigruation_success();
        test_getConfiguration_success();
        test_getConfiguration_null_data();
        test_fetchGatewayName_success();

        return 0;
    }
}