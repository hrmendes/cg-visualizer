CXX = g++
CXXFLAGS = -std=c++17 -I. -fsanitize=address -g
LDFLAGS = -lglfw -lvulkan

TARGET = sample

SRCS = $(wildcard *.cpp) \
       $(wildcard geom/*.cpp) \
       $(wildcard utils/*.cpp) \
       $(wildcard vulkan_renderer/*.cpp)


all: shaders $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

shaders:
	glslangValidator -V vulkan_renderer/shaders/shader.vert \
		-o vulkan_renderer/shaders/vert.spv
	glslangValidator -V vulkan_renderer/shaders/shader.frag \
		-o vulkan_renderer/shaders/frag.spv

clean:
	rm -f $(TARGET) vulkan_renderer/shaders/*.spv
