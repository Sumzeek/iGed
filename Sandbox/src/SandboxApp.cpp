#include "iGeEntryPoint.h"

import iGed.ExampleLayer;
import iGed.RuntimeLodLayer;

class Sandbox : public iGe::Application {
public:
    Sandbox(iGe::ApplicationSpecification& specification) : iGe::Application{specification} {
        //PushLayer(new ExampleLayer{});
        PushLayer(new RuntimeLodLayer{});
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
