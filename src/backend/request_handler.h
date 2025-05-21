#pragma once

#include "graphql.h"
#include "endpoints/endpoint_types.h"
#include "json_light/json_light.h"
#include "zap_str.h"
namespace zap {
namespace backend {

class RequestHandler {
public:

    class Externals {
        public:
            virtual GQL::BoolResponse setConfiguration(const zap::Str& jwt) = 0;

            virtual const Endpoint& toEndpoint(const zap::Str& path, const zap::Str& verb) = 0;
            virtual EndpointResponse route(const EndpointRequest& request) = 0;
    };

    explicit RequestHandler(Externals& ext);

    /**
     * @brief Processes incoming configuration data, specifically looking for requests.
     *
     * @param configData A JsonParser instance containing the configuration data payload.
     */
    void handleRequestTask(JsonParser& configData);

private:
    /**
     * @brief Handles a specific request extracted from the configuration data.
     *
     * @param requestData A JsonParser instance containing the request details (id, path, method, etc.).
     */
    void handleRequest(JsonParser& requestData);

    /**
     * @brief Sends a response back to the backend via a GraphQL mutation.
     *
     * @param requestId The ID of the original request.
     * @param statusCode The HTTP status code for the response.
     * @param responseData The JSON string representing the response body.
     */
    void sendResponse(const zap::Str& requestId, int statusCode, const zap::Str& responseData);

    /**
     * @brief Sends a formatted error response back to the backend.
     *
     * @param requestId The ID of the original request.
     * @param errorMessage A descriptive error message.
     */
    void sendErrorResponse(const zap::Str& requestId, const char* errorMessage);

    Externals& _ext;  // Reference to external dependencies
};

}
}

