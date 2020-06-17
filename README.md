# Hello Triangle Vulkan Tutorial

Just me learning how to use Vulkan.
I'm using [this wonderful Vulkan tutorial](https://vulkan-tutorial.com/).

It took **1204** LoC, but I can finally draw a single triangle on screen.
Well, almost (see **Bugs** below).

## How to build

Make sure the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) is installed, along with [GLFW](https://www.glfw.org/) and [GLM](https://glm.g-truc.net/0.9.9/index.html).
Then, run `make` to generate the executable.
Run `make clean` to remove all generated files.

## Bugs

Running the program makes my computer screen flicker every couple of seconds.
