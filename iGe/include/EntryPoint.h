#pragma once

#include <iostream>

import iGe;

int main(int argc, char** argv) {
    iGe::Log::Init();
    //iGe::CoreWarn("CORE!");
    //iGe::ClientWarn("CLIENT!");

    auto app = iGe::CreateApplication();
    app->Run();
    delete app;
}
