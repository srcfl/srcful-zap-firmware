#include "data_reader_endpoint_handlers.h"
#include "json_light/json_light.h"
#include "zap_log.h"

static constexpr LogTag TAG_DREH = LogTag("data_reader_endpoint_handlers", ZLOG_LEVEL_DEBUG);


EndpointResponse DataReaderGetHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";

    // Get the last decoded P1 data
    const P1Data& lastData = dataReader.getLastDecodedData();

    // Create a JSON response with the last decoded data
    JsonBuilder json;
    json.beginObject()
        .add("status", "success")
        .add("ts", lastData.timestamp)
        .addArray("data", (const char*)lastData.obisStrings, lastData.obisStringCount, P1Data::MAX_OBIS_STRING_LEN);
    
    LOG_TI(TAG_DREH, "Last decoded P1 data lines: %d", lastData.obisStringCount);
    
    response.data = json.end();
    response.statusCode = 200;

    return response;
}