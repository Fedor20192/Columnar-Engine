#include <iostream>

#include "HardOperators.h"

#include "glog/logging.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return 111;
    }

    const int num = std::stoi(argv[1]);

    if (num < 0 || num >= kQueriesCount) {
        return 11;
    }

    const std::string filename = "aboba.chsv";

    auto query = kGenerators[num](filename);

    query->Open();

    while (auto ans = query->Next()) {
        const auto& serialized = ans.value().Serialize();
        for (const auto& row : serialized) {
            for (const auto& column : row) {
                std::cout << column << ' ';
            }
            std::cout << '\n';
        }
        std::cout << std::endl;
    }

    query->Close();

    return 0;
}