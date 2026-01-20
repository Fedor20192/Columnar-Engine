#pragma once

#include "glog/logging.h"

class GlogFixture {
public:
    GlogFixture() {
        google::InitGoogleLogging("test");
    }

    ~GlogFixture() {
        google::ShutdownGoogleLogging();
    }
    GlogFixture(const GlogFixture &) = delete;
    GlogFixture &operator=(const GlogFixture &) = delete;
};