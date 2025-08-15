// overlay.h
#pragma once

#include <vulkan/vulkan.h>
#include "ext/Nuklear/src/nuklear.h"

enum overlay_direction {
    OVERLAY_DIRECTION_NONE = 0,
    OVERLAY_DIRECTION_LEFT,
    OVERLAY_DIRECTION_RIGHT,
    OVERLAY_DIRECTION_UP,
    OVERLAY_DIRECTION_DOWN
};

struct overlay_settings {
    float bg_color[4];
    enum overlay_direction direction;
    int fps;
    int zoom;

};

void init_overlay(GLFWwindow* window, VkDevice device, VkPhysicalDevice physicalDevice,
                 uint32_t graphicsQueueFamily, VkQueue graphicsQueue,
                 VkImageView* imageViews, uint32_t imageViewsLen,
                 uint32_t width, uint32_t height, VkFormat format);

void resize_overlay(uint32_t width, uint32_t height);
VkSemaphore submit_overlay(struct overlay_settings* settings, VkQueue graphicsQueue,
                          uint32_t buffer_index, VkSemaphore wait_semaphore);
void shutdown_overlay();

// Add these Nuklear access functions
struct nk_context* get_overlay_context();
void overlay_new_frame();
