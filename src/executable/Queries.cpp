#include <chrono>
#include <iostream>

#include "HardOperators.h"
#include "glog/logging.h"

int main(int argc, char* argv[]) {
    int num = -1;

    if (argc >= 2) {
        num = std::stoi(argv[1]);

        if (num < 0 || num >= kQueriesCount) {
            return 11;
        }
    }

    std::string filename = "aboba.chsv";

    if (argc >= 3) {
        filename = argv[2];
    }

    int start_query = 0, finish_query = kQueriesCount;

    if (num != -1) {
        start_query = num;
        finish_query = num + 1;
    }

    auto global_start_time = std::chrono::high_resolution_clock::now();

    for (int i = start_query; i < finish_query; i++) {
        std::cout << "[QueriesExecute]: Starting execute query number " << i << '\n';

        auto start_time = std::chrono::high_resolution_clock::now();

        auto query = kGenerators[i](filename);

        query->Open();

        while (auto ans = query->Next()) {
            const auto& batch = ans.value();
            if (batch->RowCount() == 0) {
                continue;
            }
            const auto& serialized = ans.value()->Serialize();
            for (const auto& row : serialized) {
                for (const auto& column : row) {
                    std::cout << column << ' ';
                }
                std::cout << '\n';
            }
            std::cout << std::endl;
        }

        query->Close();

        auto finish_time = std::chrono::high_resolution_clock::now();

        std::cout << "Time: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(finish_time - start_time)
                         .count()
                  << " ms\n\n"
                  << std::endl;

        DLOG(INFO) << "[QueriesExecute]: Query successfully executed\n";
    }

    std::cout << "Total time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::high_resolution_clock::now() - global_start_time)
                     .count()
              << " ms\n\n"
              << std::endl;

    return 0;
}