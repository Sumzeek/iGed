#include "iGe.h"

class ExampleLayer : public iGe::Layer {
public:
    ExampleLayer() : Layer("Example") {}

    void OnUpdate() override { IGE_INFO("ExampleLayer::Update"); }
    void OnEvent(iGe::Event& event) override { IGE_TRACE("{0}", event); }
};

class Sandbox : public iGe::Application {
public:
    Sandbox() { PushLayer(new ExampleLayer); }
    ~Sandbox() override {}
};

iGe::Application* iGe::CreateApplication() { return new Sandbox; }
