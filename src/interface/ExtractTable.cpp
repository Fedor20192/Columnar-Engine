#include "CHSVConverter.h"
#include "glog/logging.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        DLOG(ERROR) << "Wrong number of arguments";
        return 111;
    }

    DLOG(INFO) << "Extracting table " << argv[1] << '\n';

    std::string table_name(argv[1]), target_name(argv[2]);

    cngn::FromFormatToCsv(table_name, target_name);

    DLOG(INFO) << "Table extracted to " << target_name << '\n';
}