#include "iGed.h"
#include <iostream>

class Sandbox : public iGed::Application {
public:
    Sandbox() {}
    ~Sandbox() override {}
};

iGed::Application* iGed::CreateApplication() { return new Sandbox; }