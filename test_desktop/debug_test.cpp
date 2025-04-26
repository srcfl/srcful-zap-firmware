#include "../src/debug.h"
#include "../src/data/circular_buffer.h"

#include <assert.h>

namespace debug_test {

    

    int test_meterDatabuffer() {
        CircularBuffer buffer(10);

        JsonBuilder report;

        report.beginObject();
        Debug::getJsonReport(report);
        zap::Str meterDataBuffer;

        JsonParser parser(report.end().c_str());

        assert(!parser.getStringByPath("report.meterDataBuffer", meterDataBuffer));

        Debug::setMeterDataBuffer(&buffer);

        report.clear();
        report.beginObject();
        Debug::getJsonReport(report);
        parser = JsonParser(report.end().c_str());
        assert(parser.getStringByPath("report.meterDataBuffer", meterDataBuffer));

        report.clear();
        report.beginObject();
        buffer.addByte(0x01, 1000);
        buffer.addByte(0xff, 1000);
        Debug::getJsonReport(report);
        parser = JsonParser(report.end().c_str());
        assert(parser.getStringByPath("report.meterDataBuffer", meterDataBuffer));
        assert(meterDataBuffer == "01ff");
        
        return 0;
    }

    int run(){
        
        test_meterDatabuffer();

        return 0;
    }
}