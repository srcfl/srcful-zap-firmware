#pragma once
#include "zap_str.h"


class GQL {
public:
    // Status enum to represent different outcomes
    enum class Status {
        SUCCESS,             // Everything worked as expected
        NETWORK_ERROR,       // Connection failure, HTTP error
        PARSE_ERROR,         // Couldn't parse JSON response
        GQL_ERROR,           // GraphQL returned errors field
        OPERATION_FAILED,    // GraphQL operation returned but with failure status
        INVALID_RESPONSE     // Response missing expected fields
    };
    
    // Generic response template for any return type
    template<typename T>
    struct Response {
        Status status;      // The outcome of the request
        T data;             // The parsed data of type T
        zap::Str error;     // Error message if any
        
        // Check if the response indicates success
        bool isSuccess() const {
            return status == Status::SUCCESS;
        }
        
        // Constructors for common responses
        static Response<T> ok(const T& result) {
            return {Status::SUCCESS, result, zap::Str()};
        }
        
        static Response<T> operationFailed(const zap::Str& errorMsg) {
            return {Status::OPERATION_FAILED, T(), errorMsg};
        }
        
        static Response<T> networkError(const zap::Str& errorMsg) {
            return {Status::NETWORK_ERROR, T(), errorMsg};
        }
        
        static Response<T> parseError(const zap::Str& errorMsg) {
            return {Status::PARSE_ERROR, T(), errorMsg};
        }
        
        static Response<T> invalidResponse(const zap::Str& errorMsg) {
            return {Status::INVALID_RESPONSE, T(), errorMsg};
        }
        
        static Response<T> gqlError(const zap::Str& errorMsg) {
            return {Status::GQL_ERROR, T(), errorMsg};
        }
    };
    
    // Common response types
    using BoolResponse = Response<bool>;
    using StringResponse = Response<zap::Str>;
    
    // Updated method signatures
    static BoolResponse setConfiguration(const zap::Str& jwt);
    static StringResponse fetchGatewayName(const zap::Str& serialNumber);
    static StringResponse getConfiguration(const zap::Str& subKey);
    
private:
    static StringResponse makeGraphQLRequest(const zap::Str& query, const char* endpoint);
    static zap::Str prepareGraphQLQuery(const zap::Str& rawQuery);
};
