#include "EntryPoint.h"

class ExampleLayer : public iGe::Layer {
public:
    ExampleLayer() : Layer{"Example"} {}

    void OnUpdate() override {
        auto [x, y] = iGe::Input::GetMousePosition();
        IGE_INFO("x:{}, y:{}", x, y);
    }

    void OnEvent(iGe::Event& event) override { IGE_INFO(event); }
};

class Sandbox : public iGe::Application {
public:
    Sandbox() { PushLayer(new ExampleLayer{}); }
    ~Sandbox() override {}
};

namespace iGe
{
Application* CreateApplication() { return new Sandbox(); }
} // namespace iGe
