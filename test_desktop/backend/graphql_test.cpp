#include <assert.h>

#include "../src/backend/graphql.h"
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

    int test_fetcGatewayName_success() {
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
        test_fetcGatewayName_success();

        return 0;
    }
}