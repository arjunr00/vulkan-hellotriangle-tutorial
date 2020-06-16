COMPILER = g++
CFLAGS = -std=c++17 -I${VULKAN_SDK_PATH}/include -Wall -Wextra
LDFLAGS = -L${VULKAN_SDK_PATH}/lib `pkg-config --static --libs glfw3` -lvulkan

MAIN = main.cpp
MODULES = HelloTriangle.cpp
SHADER_DIR = shader
SHADERS = $(SHADER_DIR)/shader.vert $(SHADER_DIR)/shader.frag

OUTPUT = build/HelloTriangle

$(OUTPUT): $(MAIN) $(MODULES)
	@mkdir -p build
	$(COMPILER) $(CFLAGS) -o $(OUTPUT) $^ $(LDFLAGS)

shaders: $(SHADERS)
	@for s in $^ ; do glslc $$s -o $$s.spv ; done

.PHONY: test clean

test: $(OUTPUT)
	./$(OUTPUT)

clean:
	rm -f $(OUTPUT)
