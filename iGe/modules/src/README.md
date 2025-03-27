## Known Issues

If the module interface file (``.ixx``) and the implementation file (``.cpp``) are separated, it may result in inconsistent memory addresses between the data in the ``.ixx`` file and the ``.cpp`` file.

In the iGe library, Sumzeek attempted to separate the module interface file (``.ixx``) and the implementation file (``.cpp``) for the Log class. During window initialization, the WindowsWindow class called a function from the Log class:

```cpp
// #define IGE_CORE_INFO(...) ::iGe::Log::GetCoreLogger()->info(__VA_ARGS__)  
IGE_CORE_INFO("Creating window {0} ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);
```

At this point, the retrieved static variable of Log was nullptr. Additionally, when printing the addresses of the static variable during the initialization of both the Log class and WindowsWindow, they were found to be different.

Similarly, if OpenGL is initialized in the ``.ixx`` file within the WindowsWindow class of the iGe library:

```cpp
int status = gladLoadGL((GLADloadfunc) glfwGetProcAddress);  
```

But OpenGL functions are called in ``.cpp`` within the Application class, the pointer address of glClearColor becomes nullptr:

```cpp
void Application::Run() {  
    while (m_Running) {  
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);  
        glClear(GL_COLOR_BUFFER_BIT);  

        for (Layer* layer : m_LayerStack) {  
            layer->OnUpdate();  
        }  

        m_Window->OnUpdate();  
    }  
}
```

## My Solution

To ensure consistency, all modules should be **separated into module interface files and implementation files**, with all function implementations placed in ``.cpp`` files.
