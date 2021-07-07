COMPILER = g++
CFLAGS = -std=c++17 -I${VULKAN_SDK_PATH}/include -Wall -Wextra
LDFLAGS = -L${VULKAN_SDK_PATH}/lib `pkg-config --static --libs glfw3` -lvulkan

OUTPUT_DIR = build

MAIN = main.cpp
MODULES = HelloTriangle.cpp

SHADER_DIR = shader
SHADERS = $(SHADER_DIR)/shader.vert $(SHADER_DIR)/shader.frag
SHADERS_OUT = $(OUTPUT_DIR)/shader.vert.spv $(OUTPUT_DIR)/shader.frag.spv

OUTPUT = $(OUTPUT_DIR)/HelloTriangle

$(OUTPUT): $(MAIN) $(MODULES)
	@mkdir -p build
	@make --no-print-directory shaders
	@echo -n "Compiling program .. "
	@$(COMPILER) $(CFLAGS) -o $(OUTPUT) $^ $(LDFLAGS)
	@echo "done"

shaders: $(SHADERS_OUT)

$(SHADERS_OUT): $(SHADERS)
	@echo -n "Compiling shaders .. "
	@mkdir -p build
	@for s in $(^F) ; do \
		cd $(SHADER_DIR) > /dev/null && \
		glslc $$s -o $$s.spv && \
		cd - > /dev/null && \
		mv $(SHADER_DIR)/$$s.spv $(OUTPUT_DIR)/ ; \
		done
	@echo "done"

.PHONY: test clean

test: $(OUTPUT)
ifeq ($(offload), yes)
	__NV_PRIME_RENDER_OFFLOAD=1 __VK_LAYER_NV_optimus=NVIDIA_only ./$(OUTPUT)
else
	./$(OUTPUT)
endif

clean:
	@echo -n "Cleaning directory .. "
	-@rm -rf $(OUTPUT_DIR)
	@echo "done"
