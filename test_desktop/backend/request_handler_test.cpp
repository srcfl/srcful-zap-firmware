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

                requestContent = request.content;
                
                response.statusCode = 200;
                response.contentType = "application/json";
                response.data = "{\"status\": \"ok\"}";
                return response;
            };

            zap::Str requestContent;
    };

    zap::Str getTimeMilliseconds() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t epochTimeMs = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
        return zap::Str(epochTimeMs);
    }

    int test_wifi_request() {
        MockExternals mockExternals;
        zap::backend::RequestHandler requestHandler(mockExternals);
        zap::Str body("{\\u0022id\\u0022: \\u0022some-id\\u0022, \\u0022body\\u0022: {\\u0022psk\\u0022: \\u0022test-psk\\u0022, \\u0022ssid\\u0022: \\u0022test-ssid\\u0022}, \\u0022path\\u0022: \\u0022/api/wifi\\u0022, \\u0022query\\u0022: \\u0022{}\\u0022, \\u0022method\\u0022: \\u0022POST\\u0022, \\u0022headers\\u0022: \\u0022{}\\u0022, \\u0022timestamp\\u0022: #ts#}");
        
        // Replace #ts# with the current timestamp so we can test the request
        zap::Str ts = getTimeMilliseconds();
        body.replace("#ts#", ts.c_str());
        
        zap::Str data = "{\"data\":\"" + zap::Str(body) + "\"}";
        JsonParser configData(data.c_str());

        requestHandler.handleRequestTask(configData);

        assert(mockExternals.requestContent == "{\"psk\": \"test-psk\", \"ssid\": \"test-ssid\"}");


        return 0;
    }

    int test_echo_request_hello_world() {
        MockExternals mockExternals;
        zap::backend::RequestHandler requestHandler(mockExternals);

        zap::Str body("{\\u0022id\\u0022: \\u0022di34bavH72FOxMbk9m8A9\\u0022, \\u0022body\\u0022: \\u0022Hello World\\u0022, \\u0022path\\u0022: \\u0022/api/echo\\u0022, \\u0022query\\u0022: \\u0022{}\\u0022, \\u0022method\\u0022: \\u0022POST\\u0022, \\u0022headers\\u0022: \\u0022{}\\u0022, \\u0022timestamp\\u0022: #ts#}");
        
        // Replace #ts# with the current timestamp so we can test the request
        zap::Str ts = getTimeMilliseconds();
        body.replace("#ts#", ts.c_str());

        zap::Str data = "{\"data\":\"" + zap::Str(body) + "\"}";
        JsonParser configData(data.c_str());

        requestHandler.handleRequestTask(configData);
        assert(mockExternals.requestContent == "Hello World");
        return 0;
    }

    int test_echo_request_quoted_hello_world() {
        MockExternals mockExternals;
        zap::backend::RequestHandler requestHandler(mockExternals);

        zap::Str body("{\\u0022id\\u0022: \\u002297EmcwI6gWHuUPWqBTzyr\\u0022, \\u0022body\\u0022: \\u0022\\\\u0022Hello World\\\\u0022\\u0022, \\u0022path\\u0022: \\u0022/api/echo\\u0022, \\u0022query\\u0022: \\u0022{}\\u0022, \\u0022method\\u0022: \\u0022POST\\u0022, \\u0022headers\\u0022: \\u0022{}\\u0022, \\u0022timestamp\\u0022: #ts#}");
        
        // Replace #ts# with the current timestamp so we can test the request
        zap::Str ts = getTimeMilliseconds();
        body.replace("#ts#", ts.c_str());

        zap::Str data = "{\"data\":\"" + zap::Str(body) + "\"}";
        JsonParser configData(data.c_str());

        requestHandler.handleRequestTask(configData);
        assert(mockExternals.requestContent == "\"Hello World\"");
        return 0;
    }

    int run() {
        test_wifi_request();
        test_echo_request_hello_world();
        test_echo_request_quoted_hello_world();
        return 0;
    }
}