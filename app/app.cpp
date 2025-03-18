#include "EntryPoint.h"

class ExampleLayer : public iGe::Layer {
public:
    ExampleLayer() : Layer("Example") {}

    void OnUpdate() override {}

    void OnEvent(iGe::Event& event) override { iGe::ClientInfo(event); }
};

class Sandbox : public iGe::Application {
public:
    Sandbox() { PushLayer(new ExampleLayer); }
    ~Sandbox() override {}
};

namespace iGe
{
Application* CreateApplication() { return new Sandbox(); }
} // namespace iGe
