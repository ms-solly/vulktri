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
const uint32_t WIN_WIDTH = 800;
const uint32_t WIN_HEIGHT = 600;

typedef struct App {
	GLFWwindow *window;
	VkInstance instance;
	VkSurfaceKHR  *surface;

} App;

void init_window(App *pApp);
void init_vulkan(App *pApp);
void main_loop(App *pApp);
void cleanup(App *pApp);

void create_instance(App *pApp);


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

}

void create_instance(App *pApp){
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
	VK_CHECK(vkCreateInstance(&create_info, NULL, &pApp->instance));

        #ifndef NDEBUG
	free(all_exts);
        #endif

VK_CHECK(glfwCreateWindowSurface(pApp->instance, pApp->window, NULL, pApp->surface));
	
}
void init_vulkan(App *pApp){
	create_instance(pApp);

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


