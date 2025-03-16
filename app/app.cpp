#include "iGe.h"
#include <iostream>

class Sandbox : public iGe::Application {
public:
    Sandbox() {}
    ~Sandbox() override {}
};

iGe::Application* iGe::CreateApplication() { return new Sandbox; }