rm -f ./build/tri ./build/main.o 

glslangValidator -V shaders/tri.vert.glsl -o shaders/tri.vert.spv
glslangValidator -V shaders/tri.frag.glsl -o shaders/tri.frag.spv
echo "successfully compiled shaders"

clang -ggdb main.c -no-pie -o tri -std=c99\
    -D_DEBUG -DVK_USE_PLATFORM_WAYLAND_KHR \
     -lvulkan -lglfw -lm

cp shaders/*.spv .  
./tri

