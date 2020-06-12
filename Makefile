COMPILER = g++
CFLAGS = -std=c++17 -I${VULKAN_SDK_PATH}/include -Wall -Wextra
LDFLAGS = -L${VULKAN_SDK_PATH}/lib `pkg-config --static --libs glfw3` -lvulkan
MAIN = main.cpp
MODULES = HelloTriangle.cpp
OUTPUT = build/HelloTriangle

$(OUTPUT): $(MAIN) $(MODULES)
	mkdir -p build
	$(COMPILER) $(CFLAGS) -o $(OUTPUT) $^ $(LDFLAGS)

.PHONY: test clean

test: $(OUTPUT)
	./$(OUTPUT)

clean:
	rm -f $(OUTPUT)
