#pragma once

//#ifdef IGE_PLATFORM_WINDOWS

extern iGe::Application* iGe::CreateApplication();

int main(int argc, char** argv) {
    iGe::Log::Init();
    IGE_CORE_WARN("CORE!");
    IGE_WARN("CLIENT!");

    auto app = iGe::CreateApplication();
    app->Run();
    delete app;
}

//#endif