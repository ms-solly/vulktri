glslangValidator -V shaders/tri.vert.glsl -o shaders/tri.vert.spv
glslangValidator -V shaders/tri.frag.glsl -o shaders/tri.frag.spv
gcc -ggdb main.c -o tri -D_DEBUG -DVK_USE_PLATFORM_WAYLAND_KHR  -lglfw -lvulkan  -lm && ./tri
