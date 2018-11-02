%VULKAN_SDK%\bin32\glslangValidator.exe --variable-name basic_vert_spv -V shaders/basic.vert -o ref_vk/spirv/basic_vert.c
%VULKAN_SDK%\bin32\glslangValidator.exe --variable-name basic_frag_spv -V shaders/basic.frag -o ref_vk/spirv/basic_frag.c
%VULKAN_SDK%\bin32\glslangValidator.exe --variable-name basic_color_quad_vert_spv -V shaders/basic_color_quad.vert -o ref_vk/spirv/basic_color_quad_vert.c
%VULKAN_SDK%\bin32\glslangValidator.exe --variable-name basic_color_quad_frag_spv -V shaders/basic_color_quad.frag -o ref_vk/spirv/basic_color_quad_frag.c
