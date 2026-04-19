#pragma once

#include "glog/logging.h"

class GlogFixture {
public:
    GlogFixture() {
        FLAGS_logtostderr = true;
        google::InitGoogleLogging("test");
    }

    ~GlogFixture() {
        google::ShutdownGoogleLogging();
    }
    GlogFixture(const GlogFixture &) = delete;
    GlogFixture &operator=(const GlogFixture &) = delete;
};