// flags
//#define GLFW_INCLUDE_VULKAN
//#define GLFW_EXPOSE_NATIVE_WAYLAND

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

const char* WIN_TITLE = "Vulkan";
const u32 WIN_WIDTH = 800;
const u32 WIN_HEIGHT = 600;
typedef struct {
    uint32_t graphics_index;
    uint32_t present_index;
    VkBool32 separate_queues;
} Queue_Family_Indices;


typedef struct App {
	int width, height;
	GLFWwindow *window;
	VkInstance instance;
	VkSurfaceKHR  surface;
	VkPhysicalDevice gpu_device;
	Queue_Family_Indices queue_family_index;
	VkDevice gpu_thread;// one handler to gpu for indirect convo with gpu instead of directly interacting the monster(i am imagining it as thread connecting us to gpu and many threads we can create)
	
	//swapchain 
	VkSwapchainKHR swapchain;
	VkFormat swapchain_format;
	VkColorSpaceKHR swapchain_color_space;
	VkImage* swapchain_images;
	VkImageView* swapchain_image_views;
	u32 swapchain_image_count;
	

	VkSemaphore *image_available_semaphore;
	VkSemaphore *render_complete_semaphore;

	VkSemaphore *present_semaphores; 
	VkQueue graphics_queue;
	VkCommandPool command_pool;
	VkCommandBuffer* command_buffers;
	PFN_vkCmdBeginRendering vkCmdBeginRendering;
	PFN_vkCmdEndRendering vkCmdEndRendering;

} App;

void init_window(App *pApp);
void init_vulkan(App *pApp);
void main_loop(App *pApp);
void cleanup(App *pApp);

VkInstance create_instance(App *pApp);
VkSurfaceKHR create_surface(App *pApp);
VkPhysicalDevice select_gpu_device(VkInstance instance);
Queue_Family_Indices find_gpu_queue_family_index(VkPhysicalDevice selected_gpu_device, VkSurfaceKHR surface);
VkDevice create_gpu_thread(VkPhysicalDevice selected_physical_device, u32 queue_family_index);
VkQueue create_graphics_queue(App *pApp);
VkSwapchainKHR create_swapchain(App *pApp);
void sync(App *pApp);
VkImage *create_swapchain_images(App *pApp, VkSwapchainKHR swapchain);
VkCommandPool create_command_pool(App *pApp);
VkCommandBuffer *create_command_buffers(App *pApp, VkCommandPool command_pool);
VkCommandBuffer *record_command_buffers(App *pApp, VkCommandBuffer* command_buffers);
VkSemaphore create_semaphore(VkDevice device) {
    VkSemaphoreCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkSemaphore semaphore;
    VK_CHECK(vkCreateSemaphore(device, &info, NULL, &semaphore));
    return semaphore;
}
VkImageView create_image_view(App *pApp);


int main() {
	 App app = {0};

	init_window(&app);
	init_vulkan(&app);
	main_loop(&app);
	cleanup(&app);
    return 0;
}


void init_window(App *pApp){
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	pApp->window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE, NULL, NULL);
	if(pApp->window){
		printf("windpw created....\n");
	}
	assert(pApp->window);
    glfwGetWindowSize(pApp->window, &pApp->width, &pApp->height);

}

VkInstance create_instance(App *pApp){
	VkInstance instance;
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = WIN_TITLE,
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3,
		.pNext = NULL,
	};
	u32 extension_count = 0;
	const char **glfw_ext = glfwGetRequiredInstanceExtensions(&extension_count);
        #ifndef NDEBUG
	const char* extra_exts[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
	const char** all_exts = malloc((extension_count + 1) * sizeof(char*));
	memcpy(all_exts, glfw_ext, extension_count * sizeof(char*));
	all_exts[extension_count] = extra_exts[0];
	extension_count++;
        #else
	const char** all_exts = glfw_ext;
        #endif
	/**
	const char *extensions[] ={ 
		VK_KHR_SURFACE_EXTENSION_NAME
                #ifdef VK_USE_PLATFORM_WAYLAND_KHR
		VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
		#endif
                #ifndef NDEBUG
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		#endif
	};
**/ 
	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = extension_count,
		.ppEnabledExtensionNames = all_exts,
		.enabledLayerCount = 0,

	};
	#ifdef _DEBUG
	const char* debugLayers[] = {"VK_LAYER_KHRONOS_validation"};
	create_info.ppEnabledLayerNames = debugLayers;
	create_info.enabledLayerCount = ARRAYSIZE(debugLayers);
#endif
	VK_CHECK(vkCreateInstance(&create_info, NULL, &instance));

        #ifndef NDEBUG
	free(all_exts);
        #endif
if(instance!=NULL){
		printf("instance created.....\n");
	}
	return instance;	
}

VkSurfaceKHR create_surface(App *pApp){
	VkSurfaceKHR surface;
/**
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
	VkWaylandSurfaceCreateInfoKHR surfacecreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
	    .display = glfwGetWaylandDisplay(),
	    .surface = glfwGetWaylandWindow(pApp->window),
	};
	VK_CHECK(vkCreateWaylandSurfaceKHR(pApp->instance, &surfacecreateInfo, 0, &surface));
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
	VkXlibSurfaceCreateInfoKHR surfacecreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
	    .dpy = glfwGetX11Display(),
	    .window = glfwGetX11Window(pApp->window),
	};
	VK_CHECK(vkCreateXlibSurfaceKHR(pApp->instance, &surfacecreateInfo, 0, &surface));
#else
	fprintf(stderr, "No supported platform defined for Vulkan surface creation\n");
	exit(1);
#endif
**/ 
      VK_CHECK(glfwCreateWindowSurface(pApp->instance, pApp->window, NULL, &surface));

	if(surface !=NULL){
		printf("surface created.........\n");
	}
	return surface;

	
}

VkPhysicalDevice select_gpu_device(VkInstance instance){

	VkPhysicalDevice gpu_device = VK_NULL_HANDLE;
	VkPhysicalDevice gpu_devices[8];
	u32 count = ARRAYSIZE(gpu_devices);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &count, gpu_devices));

	    VkPhysicalDevice selected_gpu_device = VK_NULL_HANDLE,
                     discrete = VK_NULL_HANDLE, fallback = VK_NULL_HANDLE;
    
    for (u32 i = 0; i < count; ++i) {
      VkPhysicalDeviceProperties props = {0};
      vkGetPhysicalDeviceProperties(gpu_devices[i], &props);
      printf("GPU%d: %s\n", i, props.deviceName);
      discrete = (!discrete && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
              ? gpu_devices[i] : discrete;
      fallback = (!fallback) ? gpu_devices[i] : fallback;
    }
    
    selected_gpu_device = discrete ? discrete : fallback;
    if (selected_gpu_device) {
      VkPhysicalDeviceProperties props = {0};
      vkGetPhysicalDeviceProperties(selected_gpu_device, &props);
      printf("Selected GPU: %s\n", props.deviceName);
    } else {
      printf("No suitable GPU found\n");
      exit(1);
    }
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(selected_gpu_device, &props);

		printf("\n=== SELECTED GPU ===\n");
	printf("Name: %s\n", props.deviceName);
	printf("Type: %s\n", props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "Discrete GPU" : "Integrated GPU");
	printf("Vendor ID: 0x%X\n", props.vendorID);
	printf("Device ID: 0x%X\n", props.deviceID);
	printf("Vulkan API: %d.%d.%d\n",
	    VK_VERSION_MAJOR(props.apiVersion),
	    VK_VERSION_MINOR(props.apiVersion),
	    VK_VERSION_PATCH(props.apiVersion));
	printf("Driver: %d.%d.%d\n",
	    VK_VERSION_MAJOR(props.driverVersion),
	    VK_VERSION_MINOR(props.driverVersion),
	    VK_VERSION_PATCH(props.driverVersion));
	printf("Max Texture Size: %d x %d\n",
	    props.limits.maxImageDimension2D,
	    props.limits.maxImageDimension2D);
	printf("Max Uniform Buffer Size: %u MB\n",
	    props.limits.maxUniformBufferRange / (1024 * 1024));
	printf("====================\n\n");

	return selected_gpu_device;

}

Queue_Family_Indices find_gpu_queue_family_index(VkPhysicalDevice selected_gpu_device,VkSurfaceKHR surface){
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(selected_gpu_device, &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(selected_gpu_device, &queue_family_count, queue_families);
   
    Queue_Family_Indices queue_family_index = {UINT32_MAX, UINT32_MAX, VK_FALSE};
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queue_family_index.graphics_index = i;
        }

        VkBool32 present_supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(selected_gpu_device, i, surface, &present_supported);
        if (present_supported) {
            queue_family_index.present_index = i;
        }

        if (queue_family_index.graphics_index != UINT32_MAX && queue_family_index.present_index != UINT32_MAX) {
            break;
        }
    }

    free(queue_families);
	
	queue_family_index.separate_queues = (queue_family_index.graphics_index != queue_family_index.present_index);
    assert(queue_family_index.graphics_index != UINT32_MAX && "No graphics queue found!");
    assert(queue_family_index.present_index != UINT32_MAX && "No present queue found!"); 

	return queue_family_index;
}

VkDevice create_gpu_thread(VkPhysicalDevice selected_gpu_device, u32 queue_family_index){
	float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_create_info = {
	    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	    .queueFamilyIndex = queue_family_index,
	    .queueCount = 1,
	    .pQueuePriorities = &queue_priority,
	};

	const char* thread_extensions[] = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
	    VK_KHR_MAINTENANCE2_EXTENSION_NAME,
	    VK_KHR_MULTIVIEW_EXTENSION_NAME,
	    VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
	    VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME};

	VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
	    .dynamicRendering = VK_TRUE,
	};

	VkPhysicalDeviceFeatures2 features2 = {
	    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
	    .features = {
	        .samplerAnisotropy = VK_TRUE,
	    },
	    .pNext = &dynamic_rendering_features,
	};

	VkDeviceCreateInfo thread_create_info = {
	    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext = &features2,
	    .queueCreateInfoCount = 1,
	    .pQueueCreateInfos = &queue_create_info,
	    .enabledExtensionCount = ARRAYSIZE(thread_extensions),
	    .ppEnabledExtensionNames = thread_extensions,
	};

	VkDevice thread;
	VK_CHECK(vkCreateDevice(selected_gpu_device, &thread_create_info, 0, &thread));

	if(thread!=NULL){
		printf("created gpu thread/device.............\n");
	}
	return thread;

}
VkQueue create_graphics_queue(App *pApp){
	VkQueue queue;
	vkGetDeviceQueue(pApp->gpu_thread, pApp->queue_family_index.graphics_index, 0, &queue);
	return queue;
}
VkSwapchainKHR create_swapchain(App *pApp){
	Queue_Family_Indices queue_family_index = find_gpu_queue_family_index(pApp->gpu_device, pApp->surface);
  VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pApp->gpu_device, pApp->surface, &surface_capabilities));
	
	u32 format_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(pApp->gpu_device, pApp->surface, &format_count, 0));
    VkSurfaceFormatKHR *formats = malloc(format_count * sizeof(VkSurfaceFormatKHR));
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(pApp->gpu_device, pApp->surface, &format_count, formats));

    pApp->swapchain_format = formats[0].format;
    pApp->swapchain_color_space = formats[0].colorSpace;
    for(u32 i = 0; i< format_count; ++i){
        if(formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            pApp->swapchain_format = formats[i].format;
            pApp->swapchain_color_space = formats[i].colorSpace;
            break;
        }
    }
    free(formats);
uint32_t image_count = surface_capabilities.minImageCount + 1;
if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
    image_count = surface_capabilities.maxImageCount;
}

	VkSwapchainCreateInfoKHR swapchain_info = {
	    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	    .surface = pApp->surface,
	    .minImageCount = image_count,
	    .imageFormat = pApp->swapchain_format,
	    .imageColorSpace = pApp->swapchain_color_space,
	    .imageExtent = surface_capabilities.currentExtent.width != UINT32_MAX ? surface_capabilities.currentExtent : (VkExtent2D){pApp->width, pApp->height},
	    .imageArrayLayers = 1,
	    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
	    .imageSharingMode = queue_family_index.separate_queues ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
	    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
	    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
	    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
	    .clipped = VK_TRUE,
	    .queueFamilyIndexCount = queue_family_index.separate_queues ? 2 : 0,
	    .pQueueFamilyIndices = queue_family_index.separate_queues ? (uint32_t[]){queue_family_index.graphics_index, queue_family_index.present_index} : NULL,
	};
	VkSwapchainKHR swapchain;
	VK_CHECK(vkCreateSwapchainKHR(pApp->gpu_thread, &swapchain_info, 0, &swapchain));

	if(swapchain!=NULL){
		printf("created swapchain..........\n");
	}
	return swapchain;
	

}
VkImage *create_swapchain_images(App *pApp,  VkSwapchainKHR swapchain) {
	vkGetSwapchainImagesKHR(pApp->gpu_thread, swapchain, &pApp->swapchain_image_count, NULL);

	printf("[Swapchain] Image count: %u\n", pApp->swapchain_image_count);

	pApp->swapchain_images = malloc(pApp->swapchain_image_count * sizeof(VkImage));
	pApp->present_semaphores = malloc(pApp->swapchain_image_count * sizeof(VkSemaphore));

	vkGetSwapchainImagesKHR(pApp->gpu_thread, swapchain, &pApp->swapchain_image_count, pApp->swapchain_images);

	for (u32 i = 0; i < pApp->swapchain_image_count; ++i)
	{
		// Create a per-image semaphore to be signaled when rendering that image completes
		pApp->present_semaphores[i] = create_semaphore(pApp->gpu_thread);
	}

	printf("[Swapchain] Image count: %u\n", pApp->swapchain_image_count);

	return pApp->swapchain_images; 

}


VkImageView *create_swapchain_views(App *pApp){
    u32 swapchain_image_count = pApp->swapchain_image_count;

    VkImageView *swapchain_image_views = malloc(swapchain_image_count * sizeof(VkImageView));
    assert(swapchain_image_views && "Failed to allocate swapchain image views");
	
    for (u32 i = 0; i < swapchain_image_count; ++i) {
      VkImageViewCreateInfo image_view_info = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = pApp->swapchain_images[i],
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = pApp->swapchain_format,
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
      VK_CHECK(vkCreateImageView(pApp->gpu_thread, &image_view_info, NULL, &swapchain_image_views[i]));
    }

    printf("[Swapchain] Created %u image views!\n", swapchain_image_count);
    return swapchain_image_views;
}

void sync(App *pApp){
    VkSemaphoreCreateInfo semaphore_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
pApp->image_available_semaphore = malloc(sizeof(VkSemaphore) * pApp->swapchain_image_count);
pApp->render_complete_semaphore = malloc(sizeof(VkSemaphore) * pApp->swapchain_image_count);

for (u32 i = 0; i < pApp->swapchain_image_count; i++) {
    VK_CHECK(vkCreateSemaphore(pApp->gpu_thread, &semaphore_info, NULL, &pApp->image_available_semaphore[i]));
    VK_CHECK(vkCreateSemaphore(pApp->gpu_thread, &semaphore_info, NULL, &pApp->render_complete_semaphore[i]));
}
	printf("sync is on.........\n");
}
VkCommandPool create_command_pool(App *pApp) {
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = pApp->queue_family_index.graphics_index,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    };

    VkCommandPool command_pool;
    VK_CHECK(vkCreateCommandPool(pApp->gpu_thread, &pool_info, NULL, &command_pool));
	if(command_pool!=NULL){
		printf("command pull successfully created.........\n");
	}
    return command_pool;
}

VkCommandBuffer* create_command_buffers(App *pApp, VkCommandPool command_pool) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = pApp->swapchain_image_count,
    };

    VkCommandBuffer* buffers = malloc(sizeof(VkCommandBuffer) * pApp->swapchain_image_count);
    VK_CHECK(vkAllocateCommandBuffers(pApp->gpu_thread, &alloc_info, buffers));
	if(buffers!=NULL){
		printf("command buffer successfully created.........\n");
	}
    return buffers;
}
VkCommandBuffer *record_command_buffers(App *pApp, VkCommandBuffer* command_buffers) {	
    for (u32 i = 0; i < pApp->swapchain_image_count; i++) {
        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };
        VK_CHECK(vkBeginCommandBuffer(command_buffers[i], &begin_info));
		//transition layoiut 
 VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = pApp->swapchain_images[i],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        };

        vkCmdPipelineBarrier(
            command_buffers[i],
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &barrier);
		
        // Dynamic Rendering setup
        VkRenderingAttachmentInfo color_attachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pApp->swapchain_image_views[i],
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,			
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue.color = {{0.1f, 0.2f, 0.4f, 1.0f}}, // blueish background
        };

        VkRenderingInfo render_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea.offset = {0, 0},
            .renderArea.extent = {pApp->width, pApp->height},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment,
        };

pApp->vkCmdBeginRendering(command_buffers[i], &render_info);

        // TODO: pipeline + vkCmdDraw here later
pApp->vkCmdEndRendering(command_buffers[i]);


        VK_CHECK(vkEndCommandBuffer(command_buffers[i]));
    }
	printf("command record buffer created ...........\n");

	return command_buffers;
}
void draw_frame(App *pApp, VkCommandBuffer* command_buffers) {
    u32 image_index ;
    VK_CHECK(vkAcquireNextImageKHR(
        pApp->gpu_thread,
        pApp->swapchain,
        UINT64_MAX,
        pApp->image_available_semaphore[image_index],
        VK_NULL_HANDLE,
        &image_index));

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &pApp->image_available_semaphore[image_index],
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffers[image_index],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &pApp->render_complete_semaphore[image_index],
    };

    VK_CHECK(vkQueueSubmit(pApp->graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &pApp->render_complete_semaphore[image_index],
        .swapchainCount = 1,
        .pSwapchains = &pApp->swapchain,
        .pImageIndices = &image_index,
    };

    VK_CHECK(vkQueuePresentKHR(pApp->graphics_queue, &present_info));

   // vkQueueWaitIdle(pApp->graphics_queue);
}

void init_vulkan(App *pApp){
	VK_CHECK(volkInitialize());
	pApp->instance = create_instance(pApp);
	volkLoadInstance(pApp->instance);
	pApp->surface  = create_surface(pApp);
	pApp->gpu_device = select_gpu_device(pApp->instance);
	pApp->queue_family_index = find_gpu_queue_family_index(pApp->gpu_device, pApp->surface);
	pApp->gpu_thread = create_gpu_thread(pApp->gpu_device, pApp->queue_family_index.graphics_index);
	volkLoadDevice(pApp->gpu_thread);
	if(pApp->gpu_thread==NULL){
		printf("[warning]gpu thread(logical device) is null\n");
	}

	pApp->vkCmdBeginRendering =  
		(PFN_vkCmdBeginRendering)vkGetDeviceProcAddr(pApp->gpu_thread, "vkCmdBeginRenderingKHR");
	if(pApp->vkCmdBeginRendering==NULL){
		printf("[warning]begin rendering is null\n");
	}

	pApp->vkCmdEndRendering =
		(PFN_vkCmdEndRendering)vkGetDeviceProcAddr(pApp->gpu_thread, "vkCmdEndRenderingKHR");
	pApp->vkCmdEndRendering = (PFN_vkCmdEndRendering)vkGetDeviceProcAddr(pApp->gpu_thread, "vkCmdEndRenderingKHR");
	if(pApp->vkCmdEndRendering==NULL){
		printf("[warning]end rendering is null\n");
	}
	if (pApp->vkCmdBeginRendering == NULL || pApp->vkCmdEndRendering == NULL) {
		printf("Failed to load dynamic rendering function pointers\n");
		exit(1);
	}
	pApp->graphics_queue = create_graphics_queue(pApp);	
	pApp->swapchain = create_swapchain(pApp);
	pApp->swapchain_images=create_swapchain_images(pApp, pApp->swapchain);
	pApp->swapchain_image_views = create_swapchain_views(pApp);	
	sync(pApp);
	pApp->command_pool = create_command_pool(pApp);
	pApp->command_buffers = create_command_buffers(pApp, pApp->command_pool);
	pApp->command_buffers = record_command_buffers(pApp, pApp->command_buffers);
}
void main_loop(App *pApp){
		while (!glfwWindowShouldClose(pApp->window)) {
			glfwPollEvents();
		draw_frame(pApp, pApp->command_buffers);
		
		}
}
void cleanup(App *pApp){
		glfwDestroyWindow(pApp->window);
   if (pApp->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(pApp->gpu_thread, pApp->swapchain, NULL);
    }

    if (pApp->gpu_thread != VK_NULL_HANDLE) {
        vkDestroyDevice(pApp->gpu_thread, NULL);
    }

    if (pApp->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(pApp->instance, pApp->surface, NULL);
    }

    if (pApp->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(pApp->instance, NULL);
    }

    if (pApp->window) {
        glfwDestroyWindow(pApp->window);
    }
		glfwTerminate();
}


