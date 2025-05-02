#include <assert.h>

#include "../src/backend/request_handler.h"
#include "../src/backend/graphql.h"

#include "../src/json_light/json_light.h"
#include "../src/endpoints/endpoint_types.h"

namespace request_handler_test {

    class MockEndpointFunction : public EndpointFunction {
        public:
            virtual EndpointResponse handle(const zap::Str& contents) override {
                EndpointResponse response;
                response.statusCode = 200;
                response.contentType = "application/json";
                response.data = "{\"status\": \"ok\"}";
                return response;
            }
    };

    class MockExternals : public zap::backend::RequestHandler::Externals {
        public:
            MockExternals() : zap::backend::RequestHandler::Externals() {
                // Initialize any mock data or state here
            }

            GQL::BoolResponse setConfiguration(const zap::Str& jwt) override {
                // Mock implementation of setConfiguration
                GQL::BoolResponse response;
                response.status = GQL::Status::SUCCESS;
                return response;
            };

            const Endpoint& toEndpoint(const zap::Str& path, const zap::Str& verb) override {
                static MockEndpointFunction mockFunction;
                static Endpoint endpoint(Endpoint::Type::ECHO, Endpoint::Verb::POST, "api/echo", mockFunction);

                return endpoint;
            }

            EndpointResponse route(const EndpointRequest& request) override {
                // Mock implementation of route
                EndpointResponse response;
                
                response.statusCode = 200;
                response.contentType = "application/json";
                response.data = "{\"status\": \"ok\"}";
                return response;
            };
    };

    int test_wifi_request() {
        MockExternals mockExternals;
        zap::backend::RequestHandler requestHandler(mockExternals);
        const char* body =  "{\\u0022id\\u0022: \\u0022some-id\\u0022, \\u0022body\\u0022: {\\u0022psk\\u0022: \\u0022bamse-zorba\\u0022, \\u0022ssid\\u0022: \\u0022eather\\u0022}, \\u0022path\\u0022: \\u0022/api/wifi\\u0022, \\u0022query\\u0022: \\u0022{}\\u0022, \\u0022method\\u0022: \\u0022POST\\u0022, \\u0022headers\\u0022: \\u0022{}\\u0022, \\u0022timestamp\\u0022: 1746111306022}";
        zap::Str data = "{data=\"" + zap::Str(body) + "\"}";
        JsonParser configData(data.c_str());

        requestHandler.handleRequestTask(configData);

        return 0;
    }

    int run() {
        test_wifi_request();
        return 0;
    }
}