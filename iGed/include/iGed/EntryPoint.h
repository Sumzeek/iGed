#pragma once

//#ifdef IGED_PLATFORM_WINDOWS

extern iGed::Application* iGed::CreateApplication();

int main(int argc, char** argv) {
    auto app = iGed::CreateApplication();
    app->Run();
    delete app;
}

//#endif