#pragma once

#include "data/data_reader_task.h"
#include "endpoint_types.h"

class DataReaderHandler {
    protected:
        DataReaderTask& dataReader;
    public:
        explicit DataReaderHandler(DataReaderTask& dataReader) : dataReader(dataReader) {}
};

class DataReaderGetHandler : public EndpointFunction, protected DataReaderHandler {
    public:
        explicit DataReaderGetHandler(DataReaderTask& dataReader) : DataReaderHandler(dataReader) {}
        EndpointResponse handle(const zap::Str& contents) override;
};