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