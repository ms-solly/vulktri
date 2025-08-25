// flags
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_VULKAN_IMPLEMENTATION




// unity header
#include"heads.c"
#define u32 uint32_t
#define VK_CHECK(call)                                                         \
  do {                                                                         \
    VkResult result_ = call;                                                   \
    assert(result_ == VK_SUCCESS);                                             \
  } while (0)
#ifndef ARRAYSIZE
#define ARRAYSIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

// Global variables for resize handling
static bool framebufferResized = false;
static bool show_gui = true;


// Callback for window resize
static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    framebufferResized = true;
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        show_gui = !show_gui;
    }
}

//-------------------  gui  ----------------- 

// needed macros
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024



int main() {
  int rc = glfwInit();
  assert(rc);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window = glfwCreateWindow(800, 600, "niagara", 0, 0);
  assert(window);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  glfwSetKeyCallback(window, key_callback);

  int windowWidth = 0, windowHeight = 0;
  glfwGetWindowSize(window, &windowWidth, &windowHeight);
  VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .apiVersion = VK_API_VERSION_1_3,
  };
  VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
  };
#ifdef _DEBUG
  const char *debugLayers[] = {"VK_LAYER_KHRONOS_validation"};
  createInfo.ppEnabledLayerNames = debugLayers;
  createInfo.enabledLayerCount = ARRAYSIZE(debugLayers);
#endif
  const char *extensions[] = {
      VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
      VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
#ifndef NDEBUG
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
  };
  createInfo.ppEnabledExtensionNames = extensions;
  createInfo.enabledExtensionCount = ARRAYSIZE(extensions);
  VkInstance instance;
  VK_CHECK(vkCreateInstance(&createInfo, 0, &instance));
  VkPhysicalDevice physicalDevices[8];
  u32 physicalDeviceCount = ARRAYSIZE(physicalDevices);
  VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
                                      physicalDevices));
  VkPhysicalDevice selectedPhysicalDevice = VK_NULL_HANDLE,
                   discrete = VK_NULL_HANDLE, fallback = VK_NULL_HANDLE;
  for (u32 i = 0; i < physicalDeviceCount; ++i) {
    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(physicalDevices[i], &props);
    printf("GPU%d: %s\n", i, props.deviceName);
    discrete =
        (!discrete && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            ? physicalDevices[i]
            : discrete;
    fallback = (!fallback) ? physicalDevices[i] : fallback;
  }
  selectedPhysicalDevice = discrete ? discrete : fallback;
  if (selectedPhysicalDevice) {
    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(selectedPhysicalDevice, &props);
    printf("Selected GPU: %s\n", props.deviceName);
  } else {
    printf("No suitable GPU found\n");
    exit(1);
  }
  u32 queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(selectedPhysicalDevice,
                                           &queueFamilyCount, NULL);
  VkQueueFamilyProperties *queueFamilies =
      malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(selectedPhysicalDevice,
                                           &queueFamilyCount, queueFamilies);
  u32 queuefamilyIndex = UINT32_MAX;
  for (u32 i = 0; i < queueFamilyCount; ++i) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queuefamilyIndex = i;
      break;
    }
  }
  assert(queuefamilyIndex != UINT32_MAX && "No suitable queue family found");
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queuefamilyIndex,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority,
  };
  VkPhysicalDeviceFeatures deviceFeatures = {0};
  const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueCreateInfo,
      .enabledExtensionCount = ARRAYSIZE(deviceExtensions),
      .ppEnabledExtensionNames = deviceExtensions,
      .pEnabledFeatures = &deviceFeatures,
  };
  VkDevice device;
  VK_CHECK(
      vkCreateDevice(selectedPhysicalDevice, &deviceCreateInfo, 0, &device));
  // surface createinfo need different for other os or x11
  VkWaylandSurfaceCreateInfoKHR surfacecreateInfo = {
      VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
  surfacecreateInfo.display = glfwGetWaylandDisplay();
  surfacecreateInfo.surface = glfwGetWaylandWindow(window);
  VkSurfaceKHR surface = 0;
  VK_CHECK(
      vkCreateWaylandSurfaceKHR(instance, &surfacecreateInfo, 0, &surface));
  VkBool32 presentSupported = 0;
  VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(
      selectedPhysicalDevice, queuefamilyIndex, surface, &presentSupported));
  assert(presentSupported);
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      selectedPhysicalDevice, surface, &surfaceCapabilities));
  u32 formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(selectedPhysicalDevice, surface,
                                       &formatCount, NULL);
  VkSurfaceFormatKHR *formats =
      malloc(formatCount * sizeof(VkSurfaceFormatKHR));
  vkGetPhysicalDeviceSurfaceFormatsKHR(selectedPhysicalDevice, surface,
                                       &formatCount, formats);
  VkSwapchainKHR swapchain;
  VkSwapchainCreateInfoKHR swapchaincreateinfo = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = surfaceCapabilities.minImageCount,
      .imageFormat = formats[0].format,
      .imageColorSpace = formats[0].colorSpace,
      .imageExtent = {.width = windowWidth, .height = windowHeight},
      .imageArrayLayers = 1,
      .imageUsage =
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queuefamilyIndex,
  };
  VK_CHECK(vkCreateSwapchainKHR(device, &swapchaincreateinfo, 0, &swapchain));
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderCompleteSemaphore;
  VkSemaphoreCreateInfo semInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VK_CHECK(vkCreateSemaphore(device, &semInfo, 0, &imageAvailableSemaphore));
  VK_CHECK(vkCreateSemaphore(device, &semInfo, 0, &renderCompleteSemaphore));
  VkQueue queue;
  vkGetDeviceQueue(device, queuefamilyIndex, 0, &queue);
  VkCommandPoolCreateInfo commandPoolInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = queuefamilyIndex,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
  };
  VkCommandPool commandpool;
  VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, NULL, &commandpool));
  VkRenderPass renderPass = 0;
  VkAttachmentDescription attachmentsrp[1] = {
      {
          .format = swapchaincreateinfo.imageFormat,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      },

  };
  VkAttachmentReference colorAttachments = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  VkSubpassDescription subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachments,
  };
  VkRenderPassCreateInfo rpcreateInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = ARRAYSIZE(attachmentsrp),
      .pAttachments = attachmentsrp,
      .subpassCount = 1,
      .pSubpasses = &subpass,
  };
  VK_CHECK(vkCreateRenderPass(device, &rpcreateInfo, 0, &renderPass));
  u32 swapchainimageCount = 0;
  VK_CHECK(
      vkGetSwapchainImagesKHR(device, swapchain, &swapchainimageCount, NULL));
  VkImage *swapchainImages = malloc(swapchainimageCount * sizeof(VkImage));
  VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainimageCount,
                                   swapchainImages));
  VkImageView *swapchainImageViews =
      malloc(swapchainimageCount * sizeof(VkImageView));
  for (u32 i = 0; i < swapchainimageCount; ++i) {
    VkImageViewCreateInfo imageViewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapchainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchaincreateinfo.imageFormat,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    VK_CHECK(vkCreateImageView(device, &imageViewInfo, NULL,
                               &swapchainImageViews[i]));
  }
  // swapview for gui
  VkImageView *uiImageViews = malloc(swapchainimageCount * sizeof(VkImageView));
  for (u32 i = 0; i < swapchainimageCount; ++i) {
    VkImageViewCreateInfo uiViewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapchainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchaincreateinfo.imageFormat,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    VK_CHECK(vkCreateImageView(device, &uiViewInfo, NULL,
                               &uiImageViews[i]));
  }

  VkFramebuffer *framebuffers = malloc(swapchainimageCount * sizeof(VkFramebuffer));
  for (u32 i = 0; i < swapchainimageCount; ++i) {
    VkImageView attachments[1] = {swapchainImageViews[i]};
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = ARRAYSIZE(attachments),
        .pAttachments = attachments,
        .width = windowWidth,
        .height = windowHeight,
        .layers = 1,
    };
    VK_CHECK(
        vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]));
  }
  VkShaderModule triangleVS;
  {
    FILE *file = fopen("shaders/tri.vert.spv", "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    assert(length >= 0);
    fseek(file, 0, SEEK_SET);
    char *buffer = (char *)malloc(length);
    assert(buffer);
    size_t rc = fread(buffer, 1, length, file);
    assert(rc == (size_t)length);
    fclose(file);
    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = length;
    createInfo.pCode = (const uint32_t *)buffer;
    VK_CHECK(vkCreateShaderModule(device, &createInfo, NULL, &triangleVS));
    free(buffer);
  }
  VkShaderModule triangleFS;
  {
    FILE *file = fopen("shaders/tri.frag.spv", "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    assert(length >= 0);
    fseek(file, 0, SEEK_SET);
    char *buffer = (char *)malloc(length);
    assert(buffer);
    size_t rc = fread(buffer, 1, length, file);
    assert(rc == (size_t)length);
    fclose(file);
    VkShaderModuleCreateInfo createInfo = {1};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = length;
    createInfo.pCode = (const uint32_t *)buffer;
    VK_CHECK(vkCreateShaderModule(device, &createInfo, NULL, &triangleFS));
    free(buffer);
  }
  VkPipelineLayout pipelinelayout;
  VkPipelineLayoutCreateInfo pipelinecreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
  };
  VK_CHECK(
      vkCreatePipelineLayout(device, &pipelinecreateInfo, 0, &pipelinelayout));
  VkGraphicsPipelineCreateInfo pipelineinfo = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
  };
  VkPipelineShaderStageCreateInfo stages[2] = {
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = triangleVS,
          .pName = "main",
      },
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = triangleFS,
          .pName = "main",
      },
  };
  pipelineinfo.stageCount = ARRAYSIZE(stages);
  pipelineinfo.pStages = stages;
  VkPipelineVertexInputStateCreateInfo vertexInput = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };
  pipelineinfo.pVertexInputState = &vertexInput;
  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
  };
  pipelineinfo.pInputAssemblyState = &inputAssembly;
  VkPipelineViewportStateCreateInfo viewportState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
  };
  pipelineinfo.pViewportState = &viewportState;
  VkPipelineRasterizationStateCreateInfo rasterizationState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .lineWidth = 1.f,
  };
  pipelineinfo.pRasterizationState = &rasterizationState;
  VkPipelineMultisampleStateCreateInfo multisampleState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };
  pipelineinfo.pMultisampleState = &multisampleState;
  VkPipelineDepthStencilStateCreateInfo depthStencilState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
  };
  pipelineinfo.pDepthStencilState = &depthStencilState;
  VkPipelineColorBlendAttachmentState colorAttachmentState = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo colorBlendState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &colorAttachmentState,
  };
  pipelineinfo.pColorBlendState = &colorBlendState;
  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]),
      .pDynamicStates = dynamicStates,
  };
  pipelineinfo.pDynamicState = &dynamicState;
  pipelineinfo.layout = pipelinelayout;
  pipelineinfo.renderPass = renderPass;
  VkPipeline pipeline = 0;
  VkPipelineCache pipelineCache = 0;
  VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineinfo, 0,
                                     &pipeline));
  VkCommandBuffer commandBuffer;

  VkSemaphore sceneFinishedSemaphore;
    VK_CHECK(vkCreateSemaphore(device, &semInfo, NULL, &sceneFinishedSemaphore));
    
  struct nk_context *ctx;
  ctx = nk_glfw3_init(window, device, selectedPhysicalDevice, queuefamilyIndex,
                      uiImageViews, swapchainimageCount,
                      swapchaincreateinfo.imageFormat,
                      0, MAX_VERTEX_BUFFER,
                      MAX_ELEMENT_BUFFER);
  {
    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&atlas);
    nk_glfw3_font_stash_end(queue);
  }

  float r = 1.0f, g = 0.0f, b = 0.0f;

    // VkSemaphore sceneFinishedSemaphore;
    // VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    // vkCreateSemaphore(device, &semInfo, NULL, &sceneFinishedSemaphore);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    
    // Handle window resize
    if (framebufferResized) {
      framebufferResized = false;
      
      // Wait for device to be idle
      vkDeviceWaitIdle(device);
      
      // Get new window size
      glfwGetWindowSize(window, &windowWidth, &windowHeight);
      
      // Skip if minimized
      if (windowWidth == 0 || windowHeight == 0) {
        continue;
      }
      
      // Cleanup old resources
      for (uint32_t i = 0; i < swapchainimageCount; ++i) {
        vkDestroyFramebuffer(device, framebuffers[i], 0);
        vkDestroyImageView(device, swapchainImageViews[i], 0);
        vkDestroyImageView(device, uiImageViews[i], 0);
      }
      vkDestroySwapchainKHR(device, swapchain, 0);
      
      // Recreate swapchain with new size
      swapchaincreateinfo.imageExtent.width = windowWidth;
      swapchaincreateinfo.imageExtent.height = windowHeight;
      VK_CHECK(vkCreateSwapchainKHR(device, &swapchaincreateinfo, 0, &swapchain));
      
      // Recreate swapchain images and views
      VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainimageCount, NULL));
      swapchainImages = realloc(swapchainImages, swapchainimageCount * sizeof(VkImage));
      VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainimageCount, swapchainImages));
      
      swapchainImageViews = realloc(swapchainImageViews, swapchainimageCount * sizeof(VkImageView));
      uiImageViews = realloc(uiImageViews, swapchainimageCount * sizeof(VkImageView));
      
      for (u32 i = 0; i < swapchainimageCount; ++i) {
        VkImageViewCreateInfo imageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchaincreateinfo.imageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        VK_CHECK(vkCreateImageView(device, &imageViewInfo, NULL, &swapchainImageViews[i]));
        
        VkImageViewCreateInfo uiViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchaincreateinfo.imageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        VK_CHECK(vkCreateImageView(device, &uiViewInfo, NULL, &uiImageViews[i]));
      }
      
      // Recreate framebuffers
      framebuffers = realloc(framebuffers, swapchainimageCount * sizeof(VkFramebuffer));
      for (u32 i = 0; i < swapchainimageCount; ++i) {
        VkImageView attachments[1] = {swapchainImageViews[i]};
        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = ARRAYSIZE(attachments),
            .pAttachments = attachments,
            .width = windowWidth,
            .height = windowHeight,
            .layers = 1,
        };
        VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]));
      }
      
      // Notify nuklear about resize
      nk_glfw3_resize(windowWidth, windowHeight);
    }
    
    nk_glfw3_new_frame();

   if (show_gui) {
        nk_flags windowFlags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
            NK_WINDOW_CLOSABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE;

        if (nk_begin(ctx, "bad code", nk_rect(50, 50, 200, 200), windowFlags)) {
            if (nk_button_label(ctx, "Change Color")) {
                r = (float)rand() / (float)RAND_MAX;
                g = (float)rand() / (float)RAND_MAX;
                b = (float)rand() / (float)RAND_MAX;
            }
        } else {
            // The window was closed by the user, so hide it
            show_gui = false;
        }
        nk_end(ctx);
    }


    u32 imageIndex = 0;

    VkResult result = vkAcquireNextImageKHR(device, swapchain, ~0ull,
                                           imageAvailableSemaphore, VK_NULL_HANDLE,
                                           &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      framebufferResized = true;
      continue;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      assert(false && "Failed to acquire swapchain image");
    }
    VK_CHECK(vkResetCommandPool(device, commandpool, 0));
    VkCommandBufferAllocateInfo commandBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandpool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VK_CHECK(
        vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer));
    VkCommandBufferBeginInfo begininfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &begininfo));
    VkClearValue clearValue = {.color = {{r, g, b, 1.0f}}};
    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = framebuffers[imageIndex],
        .renderArea = {.offset = {0, 0}, .extent = {windowWidth, windowHeight}},
        .clearValueCount = 1,
        .pClearValues = &clearValue,
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)windowWidth,
        .height = (float)windowHeight,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {windowWidth, windowHeight},
    };
    vkCmdSetScissor(commandBuffer, 0, 2, &scissor);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    // Draw full rectangle (4 vertices) instead of triangle (3 vertices)
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    // nk_glfw3_render(queue, imageIndex, renderCompleteSemaphore, NK_ANTI_ALIASING_ON);
    // end the render pass
    vkCmdEndRenderPass(commandBuffer);
    VK_CHECK(vkEndCommandBuffer(commandBuffer));
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &imageAvailableSemaphore,
        .pWaitDstStageMask =
            (VkPipelineStageFlags[]){
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &sceneFinishedSemaphore, // signal when scene is done
    };
    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

    VkSemaphore uiFinishedSemaphore = nk_glfw3_render(queue, imageIndex, sceneFinishedSemaphore, NK_ANTI_ALIASING_ON);


    // // wait for the scene to finish rendering
    // VK_CHECK(vkQueueWaitIdle(queue));
    // // render the UI
    // VkSemaphore uiFinishedSemaphore = nk_glfw3_render(queue, imageIndex, renderCompleteSemaphore, NK_ANTI_ALIASING_ON);

    // present the swapchain image
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &uiFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex,
    };
    result = vkQueuePresentKHR(queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
      framebufferResized = true;
    } else if (result != VK_SUCCESS) {
      assert(false && "Failed to present swapchain image");
    }
    // VK_CHECK(vkDeviceWaitIdle(device));

  }
  
  nk_glfw3_shutdown();
  vkDestroySemaphore(device, sceneFinishedSemaphore, 0);

  vkDeviceWaitIdle(device);
  vkFreeCommandBuffers(device, commandpool, 1, &commandBuffer);
  vkDestroyCommandPool(device, commandpool, 0);
  for (uint32_t i = 0; i < swapchainimageCount; ++i)
    vkDestroyFramebuffer(device, framebuffers[i], 0);
  for (uint32_t i = 0; i < swapchainimageCount; ++i)
    vkDestroyImageView(device, swapchainImageViews[i], 0);
  vkDestroyPipeline(device, pipeline, 0);
  vkDestroyPipelineLayout(device, pipelinelayout, 0);
  vkDestroyShaderModule(device, triangleFS, 0);
  vkDestroyShaderModule(device, triangleVS, 0);
  vkDestroyRenderPass(device, renderPass, 0);
vkDestroySemaphore(device, renderCompleteSemaphore, 0);
vkDestroySemaphore(device, imageAvailableSemaphore, 0);
  vkDestroySwapchainKHR(device, swapchain, 0);
  vkDestroySurfaceKHR(instance, surface, 0);
  glfwDestroyWindow(window);
  
  // Free allocated arrays
  free(swapchainImages);
  free(swapchainImageViews);
  free(uiImageViews);
  free(framebuffers);
  
  vkDestroyDevice(device, 0);
  vkDestroyInstance(instance, 0);
  return 0;
}
