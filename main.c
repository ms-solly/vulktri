// flags
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WAYLAND

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
const uint32_t WIN_WIDTH = 800;
const uint32_t WIN_HEIGHT = 600;

typedef struct App {
	GLFWwindow *window;
	VkInstance instance;
	VkSurfaceKHR  surface;
	VkPhysicalDevice gpu_device;
	u32 queue_family_index;
	VkDevice gpu_thread;// one handler to gpu for indirect convo with gpu instead of directly interacting the monster(i am imagining it as thread connecting us to gpu and many threads we can create)

} App;

void init_window(App *pApp);
void init_vulkan(App *pApp);
void main_loop(App *pApp);
void cleanup(App *pApp);

VkInstance create_instance(App *pApp);
VkSurfaceKHR create_surface(App *pApp);
VkPhysicalDevice select_gpu_device(VkInstance instance);
u_short find_gpu_queue_family_index(VkPhysicalDevice selected_gpu_device);
VkDevice create_gpu_thread(VkPhysicalDevice selected_physical_device, u32 queue_family_index);


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
		printf("windpw created");
	}
	assert(pApp->window);

}

VkInstance create_instance(App *pApp){
	VkInstance instance;
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3,
	};
	uint32_t extension_count = 0;
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

u_short find_gpu_queue_family_index(VkPhysicalDevice selected_gpu_device){
    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(selected_gpu_device, &queue_family_count, NULL);
    VkQueueFamilyProperties *queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(selected_gpu_device, &queue_family_count, queue_families);
    
    u32 queue_family_index = UINT32_MAX;
    for (u32 i = 0; i < queue_family_count; ++i) {
      if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        queue_family_index = i;
        break;
      }
    }
    
	assert(queue_family_index != UINT32_MAX && "No suitable queue family found");
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
	return thread;


}
void init_vulkan(App *pApp){
	pApp->instance = create_instance(pApp);
	pApp->surface  = create_surface(pApp);
	pApp->gpu_device = select_gpu_device(pApp->instance);
	pApp->queue_family_index = find_gpu_queue_family_index(pApp->gpu_device);
	pApp->gpu_thread = create_gpu_thread(pApp->gpu_device, pApp->queue_family_index);
}
void main_loop(App *pApp){
		while (!glfwWindowShouldClose(pApp->window)) {
			glfwPollEvents();
		}
}
void cleanup(App *pApp){
		glfwDestroyWindow(pApp->window);

		glfwTerminate();
}


