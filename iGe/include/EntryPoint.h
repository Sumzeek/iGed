#pragma once

#include "Macro.h"

import std;
import iGe;

int main(int argc, char** argv) {
    iGe::Log::Init();

    auto app = iGe::CreateApplication();
    app->Run();
    delete app;
}
