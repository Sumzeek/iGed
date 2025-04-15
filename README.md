# iGed(iGame Game Engine Demo)

``iGed`` is a lightweight game engine demo designed to showcase the core functionalities of a modern game engine. The
project is built as a practical demonstration of game engine development, with its core architecture and components
heavily inspired by ``Hazel``, a well-known open-source game engine.

---

## Getting Started

Visual Studio 2022 is recommended. iGed is officially untested on other development environments while we focus on a
Windows build. Due to the use of C++ modules, MSVC version 17.6 or higher is required.

<ins>**1. Downloading the repository:**</ins>

Start by cloning the repository with:

```git
git clone --recursive https://github.com/Sumzeek/iGed.git
```

If the repository was cloned non-recursively previously, use ``git submodule update --init --recursive`` to clone the
necessary submodules.

If you encounter the error:

```pgsql
Failed to connect to 127.0.0.1 port 1080 after 2104 ms: Couldn't connect to server
```

It means you need to manually configure Git's proxy settings and enable your proxy (VPN or similar).

Use the following commands to set up Git's proxy (for example, using Clash with default port 7890; adjust the port based
on your own proxy software):

```git
git config --global http.proxy http://127.0.0.1:7890
git config --global https.proxy http://127.0.0.1:7890
```

After setting the proxy, re-run the previous commands to retry your operation.

Once finished, restore the default Git settings by removing the proxy:

```git
git config --global --unset http.proxy
git config --global --unset https.proxy
```

<ins>**2. Building the project:**</ins>

1. Ensure that you have Visual Studio 2022 installed with MSVC version 17.6 or higher. You can check your MSVC version
   in Visual Studio under ``Help > About Microsoft Visual Studio``.

2. Using Visual Studio:
    - Configure the project using CMake. If you are using the Visual Studio CMake integration, open the
      ``CMakeLists.txt`` file in the root directory, and Visual Studio will automatically configure the project.
    - Build the project by selecting ``Build Solution`` from the ``Build`` menu.

3. Using CLion:
    - Open the project in CLion by selecting ``File > Open`` and choosing the root directory of the repository.
    - CLion will automatically detect the ``CMakeLists.txt`` file and configure the project.
    - Ensure that the CMake profile is set to use the correct toolchain. Go to
      ``File > Settings > Build, Execution, Deployment > Toolchains`` and ensure that the selected toolchain points to
      your MSVC installation (e.g., ``Visual Studio``).
    - Build the project by clicking the ``Build`` button in the toolbar or selecting ``Build > Build All`` from the
      menu.

4. Running the project:
    - After a successful build, you can run the project directly from Visual Studio or CLion.
    - In Visual Studio, select ``Start Without Debugging`` or ``Start Debugging`` from the Debug menu.
    - In CLion, click the Run button in the toolbar or select ``Run > Run`` from the menu.

---

## Known Issues

If the module interface file (``.ixx``) and the implementation file (``.cpp``) are separated, it may result in
inconsistent memory addresses between the data in the ``.ixx`` file and the ``.cpp`` file.

In the iGe library, Sumzeek attempted to separate the module interface file (``.ixx``) and the implementation file (
``.cpp``) for the Log class. During window initialization, the WindowsWindow class called a function from the Log class:

```cpp
// #define IGE_CORE_INFO(...) ::iGe::Log::GetCoreLogger()->info(__VA_ARGS__)  
IGE_CORE_INFO("Creating window {0} ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);
```

At this point, the retrieved static variable of Log was nullptr. Additionally, when printing the addresses of the static
variable during the initialization of both the Log class and WindowsWindow, they were found to be different.

Similarly, if OpenGL is initialized in the ``.ixx`` file within the WindowsWindow class of the iGe library:

```cpp
int status = gladLoadGL((GLADloadfunc) glfwGetProcAddress);  
```

But OpenGL functions are called in ``.cpp`` within the Application class, the pointer address of glClearColor becomes
nullptr:

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

To ensure consistency, all modules should be **separated into module interface files and implementation files**, with
all function implementations placed in ``.cpp`` files.

---
