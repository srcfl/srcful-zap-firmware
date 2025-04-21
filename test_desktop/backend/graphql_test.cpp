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

    int run(){
        
        test_setConfigruation_success();

        return 0;
    }
}