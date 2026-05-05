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

    const std::string filename = "aboba.chsv";

    int start_query = 0, finish_query = kQueriesCount;

    if (num != -1) {
        start_query = num;
        finish_query = num + 1;
    }

    for (int i = start_query; i < finish_query; i++) {
        DLOG(INFO) << "[QueriesExecute]: Starting execute query number " << num << '\n';

        auto start_time = std::chrono::high_resolution_clock::now();

        auto query = kGenerators[i](filename);

        query->Open();

        while (auto ans = query->Next()) {
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

        std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(finish_time - start_time).count() << " ms\n\n" << std::endl;

        DLOG(INFO) << "[QueriesExecute]: Query successfully executed\n";
    }

    return 0;
}