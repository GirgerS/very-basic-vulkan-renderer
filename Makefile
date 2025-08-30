ifndef VULKAN_SDK
$(error VULKAN_SDK variable is not defined)
endif

ifndef GLFW_HEADER_PATH
$(error GLFW_HEADER_PATH variable is not defined)
endif

ifndef GLFW_STATIC_LIB_PATH
$(error GLFW_STATIC_LIB_PATH is not defined)
endif

VULKAN_LIB_PATH      = $(VULKAN_SDK)/Lib
VULKAN_HEADERS_PATH  = $(VULKAN_SDK)/Include

glfw_test: glfw_test.c shaders/shader.frag shaders/shader.vert
	gcc -Wall -Wextra -I $(GLFW_HEADER_PATH) -I $(VULKAN_HEADERS_PATH) -I "./include" -c -o build/glfw_test.o glfw_test.c 
	gcc -Wall -Wextra -L$(VULKAN_LIB_PATH) -o build/glfw_test build/glfw_test.o $(GLFW_STATIC_LIB_PATH)/libglfw3.a -lgdi32 -lvulkan-1 
	glslc shaders/shader.frag -o shaders_out/frag.spv
	glslc shaders/shader.vert -o shaders_out/vert.spv

glfw_test_release: glfw_test.c shaders/shader.frag shaders/shader.vert
	gcc -O3 -D RELEASE_MODE -Wall -Wextra -I $(GLFW_HEADER_PATH) -I $(VULKAN_HEADERS_PATH) -I "./include" -c -o build/glfw_test.o glfw_test.c 
	gcc -O3 -Wall -Wextra -L$(VULKAN_LIB_PATH) -o build/glfw_test build/glfw_test.o $(GLFW_STATIC_LIB_PATH)/libglfw3.a -lgdi32 -lvulkan-1 
	glslc shaders/shader.frag -o shaders_out/frag.spv
	glslc shaders/shader.vert -o shaders_out/vert.spv

linal_tests: tests/linal_test.c
	gcc -Wall -Wextra -I "./lib" tests/linal_test.c -o build/linal_test
