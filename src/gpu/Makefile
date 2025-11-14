# Makefile to compile shaders

GLSLC=../../shaderc/build/glslc/glslc

all:
	$(GLSLC) shader.vert -o shader_vert.spv
	$(GLSLC) shader.frag -o shader_frag.spv
	echo "/* compiled shaders - done with 'xxd -i <spirV_file>' */" > mlx___vulkan_shaders_code.h.tmp
	echo "/* vertex shader */" >> mlx___vulkan_shaders_code.h.tmp
	echo "_Alignas(uint64_t)" >> mlx___vulkan_shaders_code.h.tmp
	xxd -i shader_vert.spv >> mlx___vulkan_shaders_code.h.tmp
	echo "/* fragment shader */" >> mlx___vulkan_shaders_code.h.tmp
	echo "_Alignas(uint64_t)" >> mlx___vulkan_shaders_code.h.tmp
	xxd -i shader_frag.spv >> mlx___vulkan_shaders_code.h.tmp
	mv mlx___vulkan_shaders_code.h.tmp mlx___vulkan_shaders_code.h
