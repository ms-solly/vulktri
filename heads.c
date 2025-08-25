#include <assert.h> // for fast fail
#include <stdbool.h>//dt 
#include <stdint.h>//dt 
#include <stdio.h>// ip/op 
#include <stdlib.h>// 
// api headers
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_wayland.h>
// window creation
// #define GLFW_INCLUDE_VULKAN
// #define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
// gui headers
// & macros
//flags
/*
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_VULKAN_IMPLEMENTATION
*/
#include "ext/Nuklear/nuklear.h"
#include "ext/Nuklear/demo/glfw_vulkan/nuklear_glfw_vulkan.h"
