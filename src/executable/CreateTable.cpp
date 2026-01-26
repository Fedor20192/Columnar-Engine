#include <string>

#include "../interface/CHSVConverter.h"
#include "glog/logging.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        DLOG(ERROR) << "Bad files count";
        return 111;
    }

    DLOG(INFO) << "Creating table " << argv[3] << '\n';

    std::string schema_file(argv[1]), data_file(argv[2]), table_name(argv[3]);

    cngn::FromCsvToFormat(schema_file, data_file, table_name);

    DLOG(INFO) << "Table created.";

    return 0;
}