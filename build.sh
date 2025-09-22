gcc -ggdb main.c -no-pie -o tri \
    -D_DEBUG -DVK_USE_PLATFORM_WAYLAND_KHR \
    -lvolk -lvulkan -lglfw -lm && ./tri

