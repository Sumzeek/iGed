#include "iGeEntryPoint.h"

import iGed.ExampleLayer;

class Sandbox : public iGe::Application {
public:
    Sandbox(iGe::ApplicationSpecification& specification) : iGe::Application{specification} {
        PushLayer(iGe::CreateRef<ExampleLayer>());
    }
    ~Sandbox() override {}
};

iGe::Application* iGe::CreateApplication(iGe::ApplicationCommandLineArgs args) {
    iGe::ApplicationSpecification spec;
    spec.Name = "Sandbox";
    spec.WorkingDirectory = "../iGed";
    spec.CommandLineArgs = args;

    return new Sandbox{spec};
}
