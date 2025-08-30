#ifndef LAVA
#define LAVA

// where does Vertex struct belong?
#include "misc.h"


#define VK_CHECK(expr) \
    do {if (expr != VK_SUCCESS) {fprintf(stderr, "%s: unexpected error while calling Vulkan function", __FUNCTION__); exit(1);}} while(0);


uint32_t uint32Clamp(uint32_t val, uint32_t min, uint32_t max) {
    val = val > min ? val : min;
    val = val < max ? val : max; 
    return val;
}



#define QueueFamilyIndex int64_t
#define NO_QUEUE_FAMILY -1
typedef struct {
    VkPhysicalDevice device;
    QueueFamilyIndex graphicsFamilyIndex;
    QueueFamilyIndex presentFamilyIndex; 
    VkSampleCountFlagBits multisampling;
} GPU;

typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
} VulkanImage;

typedef struct {
    VkSurfaceFormatKHR *data;
    uint32_t len;
} SwapChainFormats;

typedef struct {
    VkPresentModeKHR *data;
    uint32_t len;
} SwapChainPresentModes;


/* Should this struct even exist? */
typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;     
    SwapChainFormats formats;
    SwapChainPresentModes presentModes;
} SwapChainDetails;

typedef struct {
    VkSwapchainKHR swapchain;
    VkPresentModeKHR presentMode;
    VkSurfaceFormatKHR surfaceFormat;
    VkFormat zBufferFormat;
    VkExtent2D extent;
    VkImage *images;
    VkImageView *views;
    size_t imageCount;
    VulkanImage z_buffer;
    VkFramebuffer *framebuffers;
    size_t framebufferCount; 
    VulkanImage MSAAbuffer;
    VkRenderPass renderPass;
} Swapchain;

typedef struct {
    VkShaderModule module;
    char *code;
    size_t code_size;
} Shader;

typedef struct {
    VkPipelineLayout layout;
} VulkanPipelineLayout;

typedef struct {
    VkBuffer buffer;
    VkDeviceMemory memory; 
} VulkanBuffer;

typedef struct {
    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
    VkFence inFlight;
} SyncObjects;

typedef struct {
    VulkanBuffer buffer; 
    void *mapped_memory;
    VkDescriptorPool pool;
    VkDescriptorSet set; 
    VkDescriptorSetLayout layout;
} UniformBuffer;

typedef struct {
    VkCommandPool *pool;
    VkCommandBuffer cmd;
    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
} Frame;

typedef struct {
    alignas(16) mat4t model;
    alignas(16) mat4t view;
    alignas(16) mat4t projection;
} Ubo;

typedef struct {
    VkInstance instance;
    VkSurfaceKHR surface;
    GPU gpu;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    Swapchain swapchain;
    VulkanPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    SyncObjects sync;
    UniformBuffer uniform_buffer;
    Shader frag;
    Shader vert;
    VulkanBuffer staging_buffer;
    VulkanBuffer vertex_buffer; 
} Vulkan;


void vulkanFillMemory(VkDevice device, VkDeviceMemory mem, void *data, size_t data_size) {
    void *mapped_memory;
    vkMapMemory(device, mem, 0, data_size, 0, &mapped_memory);
    memcpy(mapped_memory, data, data_size);
    vkUnmapMemory(device, mem);
}

// Better error hadling, maybe?
void vulkanCopyBuffer(VkDevice device, VkQueue graphics_queue, VkCommandPool pool, VulkanBuffer src, VulkanBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandPool = pool;
    info.commandBufferCount = 1;

    VkResult res;
    VkCommandBuffer command_buffer;
    if ((res = vkAllocateCommandBuffers(device, &info, &command_buffer)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to allocate command buffer. Code: %d", res);
        exit(1);
    }
    
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
    if ((res = vkBeginCommandBuffer(command_buffer, &begin_info)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to allocate command buffer. Code: %d", res);
        exit(1);
    }

    VkBufferCopy mem_region = {0};
    mem_region.srcOffset = 0;
    mem_region.dstOffset = 0;
    mem_region.size = size;

    vkCmdCopyBuffer(command_buffer, src.buffer, dst.buffer, 1, &mem_region);

    if ((res = vkEndCommandBuffer(command_buffer)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to end command buffer. Error code: %d", res);
        exit(1);
    }

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    if ((res = vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to submit command buffer to graphics queue. Error code: %d", res);
        exit(1);
    }    
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, pool, 1, &command_buffer);
}


VulkanImage vulkanCreateImage(
    GPU gpu,
    VkDevice device,
    VkImageCreateInfo info,
    VkImageViewCreateInfo view_info,
    VkMemoryPropertyFlags req_mem_props
) {
    VkImage image;
    VK_CHECK(vkCreateImage(device, &info, NULL, &image));

    VkPhysicalDeviceMemoryProperties device_props;
    vkGetPhysicalDeviceMemoryProperties(gpu.device, &device_props); 
    VkMemoryRequirements reqs;
    vkGetImageMemoryRequirements(device, image, &reqs);
    int64_t memory_type_index = -1;
    for (uint32_t i=0; i<device_props.memoryTypeCount; ++i) {
        if (!(reqs.memoryTypeBits & (1 << i))) {
            continue;
        }

        if (!((device_props.memoryTypes[i].propertyFlags & req_mem_props) == req_mem_props)) {
            continue;
        }
        memory_type_index = i;
        break;
    }

    if (memory_type_index == -1) {
        fprintf(stderr, "ERROR: Failed to find suitable memory for image");
        exit(1);
    }

    VkMemoryAllocateInfo mem_info = {0};
    mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_info.allocationSize = reqs.size; 
    mem_info.memoryTypeIndex = memory_type_index;

    VkDeviceMemory image_memory;
    VK_CHECK(vkAllocateMemory(device, &mem_info, NULL, &image_memory));
    vkBindImageMemory(device, image, image_memory, 0);

    view_info.image = image;
    view_info.format = info.format; 
    VkImageView image_view;
    VK_CHECK(vkCreateImageView(device, &view_info, NULL, &image_view));

    return (VulkanImage) {image, image_memory, image_view};
};


VulkanBuffer vulkanCreateBuffer(GPU gpu, VkDevice device, VkBufferUsageFlags usage, VkMemoryPropertyFlags required_props, size_t size) {
    VkBufferCreateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; 
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult res;
    VkBuffer vertex_buffer;
    if ((res = vkCreateBuffer(device, &buffer_info, NULL, &vertex_buffer)) != VK_SUCCESS) {
        fprintf(stderr, "Error: Failed to allocate vertex buffer. Code: %d", res);
        exit(1);
    }

    VkPhysicalDeviceMemoryProperties device_props;
    vkGetPhysicalDeviceMemoryProperties(gpu.device, &device_props); 

    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(device, vertex_buffer, &reqs);
         
    int64_t memory_type_index = -1;
    for (uint32_t i=0; i<device_props.memoryTypeCount; ++i) {
        if (!(reqs.memoryTypeBits & (1 << i))) {
            continue;
        }

        if (!((device_props.memoryTypes[i].propertyFlags & required_props) == required_props)) {
            continue;
        }
        memory_type_index = i;
        break;
    }

    if (memory_type_index == -1) {
        fprintf(stderr, "ERROR: Failed to find suitable memory for vertex buffer");
        exit(1);
    }
 
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = reqs.size;
    alloc_info.memoryTypeIndex = memory_type_index;
    
    VkDeviceMemory vulkan_buffer_memory;
    if ((res=vkAllocateMemory(device, &alloc_info, NULL, &vulkan_buffer_memory)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to allocate vertex buffer memory. Code: %d", res);
        exit(1);
    }

    vkBindBufferMemory(device, vertex_buffer, vulkan_buffer_memory, 0);

    return (VulkanBuffer) {
        vertex_buffer, 
        vulkan_buffer_memory
    };
}


VkCommandPool vulkanCreateCommandPool(VkDevice device, GPU gpu) {
    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = gpu.graphicsFamilyIndex;

    VkResult res;
    VkCommandPool pool;
    if ((res = vkCreateCommandPool(device, &pool_info, NULL, &pool)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to create command pool. Error code: %d", res);
        exit(1);
    }    

    fprintf(stderr, "INFO: Command pool created successfully\n");
    return pool;
}


VkCommandBuffer vulkanCreateCommandBuffer(VkDevice device, VkCommandPool pool) {
    VkCommandBufferAllocateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_info.commandPool = pool;
    buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_info.commandBufferCount = 1;

    VkResult res;
    VkCommandBuffer buffer;
    if ((res = vkAllocateCommandBuffers(device, &buffer_info, &buffer)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create command buffer. Error code: %d", res);
        exit(1);
    }

    fprintf(stderr, "INFO: Command buffer created successfully\n");
    return buffer;
}


bool vulkanTryGetSampleCount(VkPhysicalDevice device, VkSampleCountFlagBits count) {
    VkPhysicalDeviceProperties props; 
    vkGetPhysicalDeviceProperties(device, &props);

    return (props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts) & count;
}


GPU vulkanChooseGpu(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);

    if (device_count == 0) {
        fprintf(stderr, "ERROR: Failed to find GPU that supports Vulkan");
        exit(1);
    }

    GPU target_gpu = (GPU) {
        VK_NULL_HANDLE,
        NO_QUEUE_FAMILY,
        NO_QUEUE_FAMILY,
        (VkSampleCountFlagBits)0
    };

    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    // picking the first available device, in the real world i should choose the most powerful one or something 
    for (uint32_t i=0;i<device_count;++i) {
        VkPhysicalDevice device = devices[i];
        
        /* Check if device is a gpu */ { 
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(device, &features);
    
            if (!((props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) && features.geometryShader)) {
                continue;
            }
        }
        
         
        int64_t graphics_family_index = NO_QUEUE_FAMILY;
        int64_t present_family_index = NO_QUEUE_FAMILY;
        /* Graphics queue support */ {
            uint32_t queue_family_count;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
            VkQueueFamilyProperties queue_families[queue_family_count]; 
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
    
            // Probably a suboptimal way to do that, so
            // @TODO: Learn how to find queueFamilies properly
            for (uint32_t i=0;i<queue_family_count;++i) {
                VkQueueFamilyProperties queue_family = queue_families[i];  
                if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphics_family_index = i;
                }
                
                VkBool32 supports_present = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present);
                if(supports_present) {
                    present_family_index = i;
                }
                
                if (graphics_family_index != NO_QUEUE_FAMILY &&
                    present_family_index != NO_QUEUE_FAMILY) {
                    break;
                }
            }
            
            if (graphics_family_index == NO_QUEUE_FAMILY ||
                present_family_index == NO_QUEUE_FAMILY) {
                continue;
            }
        } 

        /* Check if we can use swap chain extensions */ {
            uint32_t extension_count;
            vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
            VkExtensionProperties extensions[extension_count];
            vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, extensions);
    
            bool khr_swap_extension_found = false;
            for (uint32_t i=0;i<extension_count;++i) {
                VkExtensionProperties extension = extensions[i];
                if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                    khr_swap_extension_found = true;
                    break;
                }
            }
            if (!khr_swap_extension_found) {
                continue;  
            } 
        }

        /* Check if gpu support 8xMSAA, @TODO: unhardcode */ {
            if (!vulkanTryGetSampleCount(device, VK_SAMPLE_COUNT_8_BIT)) {
                continue;
            }
        }

        target_gpu.device = device;
        target_gpu.graphicsFamilyIndex = graphics_family_index; 
        target_gpu.presentFamilyIndex = present_family_index; 
        target_gpu.multisampling = VK_SAMPLE_COUNT_8_BIT;
    }

    if (target_gpu.device == VK_NULL_HANDLE) {
        fprintf(stderr, "ERROR: Supported GPU was found, but it not suitable for rendering");
        exit(1);
    }

    fprintf(stderr, "INFO: Target gpu is found\n");

    fprintf(stderr, "INFO: Graphics queue index(%lld), Present queue index(%lld)\n", target_gpu.graphicsFamilyIndex, target_gpu.presentFamilyIndex);
    return target_gpu;
}


VkSurfaceFormatKHR chooseSwapSurfaceFormat(SwapChainFormats formats) {
    for (uint32_t i=0; i<formats.len; ++i) {
        VkSurfaceFormatKHR format = formats.data[i];

        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format; 
        }
    }
    
    fprintf(stderr, "Unable to found sufficient surface format");
    exit(1);
}


VkPresentModeKHR chooseSwapPresentMode(SwapChainPresentModes modes) {
    for (uint32_t i=0;i<modes.len;++i) {
        VkPresentModeKHR mode = modes.data[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, GLFWwindow *window) {
    if (capabilities.currentExtent.width == UINT32_MAX) {
        return capabilities.currentExtent;
    }
        
    int32_t width, height;
    glfwGetFramebufferSize(window, &width, &height);
    
    VkExtent2D extent;

    // clamping values
    extent.width = uint32Clamp (
        (uint32_t)width, 
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );

    extent.height = uint32Clamp (
        (uint32_t)height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return extent;
}



VkInstance vulkanInit(const char *validation_layers[], size_t validation_layer_count) {
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; 
    app_info.pApplicationName = "Game?";
    app_info.pEngineName = "No Engine";
    // @LEAK?
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    // i'm not going to bother myself with callbacks now, so no VK_EXT_debug_utils for now
    uint32_t glfw_extension_count = {0};
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    
    VkInstanceCreateInfo instance_info = {0};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = glfw_extension_count;
    instance_info.ppEnabledExtensionNames = glfw_extensions;

    /* Check if all validation layers are found */ {
        uint32_t available_layer_count;
        vkEnumerateInstanceLayerProperties(&available_layer_count, NULL);
        
        VkLayerProperties available_layers[available_layer_count];
        vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers);
    
        // why on Earth vulkan tutorial tells us to do linear search to find supported layers?
        for (size_t i=0; i<validation_layer_count; ++i) {
            bool found = false;
            for (size_t j=0; j<available_layer_count; ++j) {
                found = (strcmp(validation_layers[i], available_layers[j].layerName) == 0);
                if (found) break;
            }

            if (!found) {
                    fprintf(stderr, "Required validataion layer(%s) is not found", validation_layers[i]);
                    exit(1);
            }
        }
    }
    instance_info.enabledLayerCount = validation_layer_count;
    instance_info.ppEnabledLayerNames = validation_layers;
 
    VkInstance instance;
    VkResult res = vkCreateInstance(&instance_info, NULL, &instance);

    if (res != VK_SUCCESS) {
        fprintf(stderr, "ERROR: Unable to instantiate Vulkan, error=%d", res);
        exit(1);
    }

    fprintf(stderr, "INFO: Vulkan is initialized\n");
    return instance;
}


VkDevice createLogicalDevice(GPU gpu) {
    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = gpu.graphicsFamilyIndex;
    queue_create_info.queueCount = 1;
    float queue_priority = 1;
    queue_create_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo device_create_info = {0};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = &queue_create_info;
    device_create_info.queueCreateInfoCount = 1;
    // @TODO: learn what values it should be filles with
    VkPhysicalDeviceFeatures device_features = {0};
    // @SPEED: this thing slows down 
    device_features.sampleRateShading = VK_TRUE;
    device_create_info.pEnabledFeatures = &device_features;
    device_create_info.enabledLayerCount = 0;
    // @TODO: unhardcode extenstions here and in PickGpu functions
    const char *extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME; 
    device_create_info.ppEnabledExtensionNames = &extension;
    device_create_info.enabledExtensionCount = 1;

    VkDevice device;
    int32_t err;
    if ((err = vkCreateDevice(gpu.device, &device_create_info, NULL, &device)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to create logical device. Code: %d", err);
        exit(1); 
    }

    fprintf(stderr, "INFO: logical device created successfully\n");
    return device;
}


// Maybe i should perform some checks when choosing gpu but whatever
Swapchain vulkanInitSwapchain(GPU gpu, VkDevice logical_device, VkSurfaceKHR surface, GLFWwindow *window) { 
    VkPhysicalDevice device = gpu.device;

    SwapChainDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);


    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formats.len, NULL);
    details.formats.data = malloc(details.formats.len * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formats.len, details.formats.data);

#if 0
    fprintf(stderr, "INFO: available surfaces:\n");
    for (uint32_t i=0; i<details.formats.len; ++i) {
        VkSurfaceFormatKHR surface = details.formats.data[i];
        fprintf(stderr, "\tformat(%d), colorspace(%d)\n", surface.format, surface.colorSpace);
    } 
#endif


    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModes.len, NULL);
    details.presentModes.data = malloc(details.presentModes.len * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModes.len, details.presentModes.data);

#if 0
    fprintf(stderr, "INFO: available present modes:\n");
    for (uint32_t i=0; i<details.presentModes.len; ++i) {
        VkPresentModeKHR present_mode = details.presentModes.data[i];
        fprintf(stderr, "\t(%d)\n", present_mode);
    }
#endif
    
    if (details.presentModes.len < 1 || details.formats.len < 1) {
        fprintf(stderr, "Error creating swapchain: no present modes or formats\n");
        exit(1);
    }
 
    VkPresentModeKHR mode = chooseSwapPresentMode(details.presentModes);
    uint32_t image_count = uint32Clamp (
            details.capabilities.minImageCount + 1,
            details.capabilities.minImageCount,
            details.capabilities.maxImageCount
    );

    VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(details.formats);
   
    // @TODO: support sharing between queues
    if (gpu.graphicsFamilyIndex != gpu.presentFamilyIndex) {
        fprintf(stderr, "Graphics and Present queues differ and sharing of swap chain images is not supported");
        exit(1);
    }

    VkExtent2D extent = chooseSwapExtent(details.capabilities, window);

    VkSwapchainCreateInfoKHR swapchain_info = {0};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = image_count; 
    swapchain_info.imageFormat = format.format;
    swapchain_info.imageColorSpace = format.colorSpace;
    swapchain_info.imageExtent = extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.preTransform = details.capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = mode;
    swapchain_info.clipped = VK_FALSE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;
    
    VkSwapchainKHR swapchain;
    VkResult err;
    VK_CHECK(vkCreateSwapchainKHR(logical_device, &swapchain_info, NULL, &swapchain));

    vkGetSwapchainImagesKHR(logical_device, swapchain, &image_count, NULL); 
    VkImage *images = malloc(image_count * sizeof(VkImageView));
    if (images == NULL) {
        fprintf(stderr, "ERROR: failed to allocate memory for swapchain images");
        exit(1);
    }
    vkGetSwapchainImagesKHR(logical_device, swapchain, &image_count, images);
    
    VkImageView *views = malloc(image_count * sizeof(VkImageView));  
    if (views == NULL) {
        fprintf(stderr, "ERROR: failed to allocate memory for swapchain image views");
        exit(1);
    }


    for (size_t i=0;i<image_count;++i) {
        VkImageViewCreateInfo view_info = {0};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format.format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
  
        if ((err = vkCreateImageView(logical_device, &view_info, NULL, &views[i])) != VK_SUCCESS) {
            fprintf(stderr, "ERROR: failed to allocate swapchain image. Error code: %d", err);
            exit(1);
        }
    }

    VulkanImage z_buffer;
    VkFormat found_z_format;
    /* z buffer */ {
        VkFormat desired_z_formats[] = {VK_FORMAT_D24_UNORM_S8_UINT};

        bool found = false;
        for (size_t i=0; i<sizeof desired_z_formats / sizeof(VkFormat); ++i) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(device, desired_z_formats[i], &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                found_z_format = desired_z_formats[i];
                found = true;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "ERROR: Failed to find desired z buffer format");
            exit(1);
        }

        VkImageCreateInfo image_info = {0};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = extent.width;
        image_info.extent.height = extent.height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = found_z_format;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.samples = gpu.multisampling;

        VkImageViewCreateInfo view_info = {0};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = found_z_format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        
        z_buffer = vulkanCreateImage(
            gpu,
            logical_device,
            image_info,
            view_info,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );    
    }

    VulkanImage MSAAbuffer;
    /* MSAA */ {
        VkImageCreateInfo MSAA_buffer_info = {0};
        MSAA_buffer_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        MSAA_buffer_info.imageType = VK_IMAGE_TYPE_2D;
        MSAA_buffer_info.extent.width = extent.width;
        MSAA_buffer_info.extent.height = extent.height;
        MSAA_buffer_info.extent.depth = 1;
        MSAA_buffer_info.mipLevels = 1;
        MSAA_buffer_info.arrayLayers = 1;
        MSAA_buffer_info.format = format.format;
        MSAA_buffer_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        MSAA_buffer_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        MSAA_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; 
        MSAA_buffer_info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        MSAA_buffer_info.samples = gpu.multisampling;

        VkImageViewCreateInfo MSAA_buffer_view_info = {0};
        MSAA_buffer_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        MSAA_buffer_view_info.format = format.format;
        MSAA_buffer_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        MSAA_buffer_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        MSAA_buffer_view_info.subresourceRange.layerCount = 1;
        MSAA_buffer_view_info.subresourceRange.levelCount = 1;
        MSAA_buffer_view_info.subresourceRange.baseArrayLayer = 0;
        MSAA_buffer_view_info.subresourceRange.baseMipLevel = 0;

        MSAAbuffer = vulkanCreateImage(
                gpu, 
                logical_device, 
                MSAA_buffer_info,
                MSAA_buffer_view_info,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    } 

    /* cleanup, @TODO: probaly allocations shouldn't be here in the first place */ {
        free(details.formats.data); 
        free(details.presentModes.data);
    }

    //fprintf(stderr, "INFO: Swapchain created successfully\n");
    return (Swapchain) {
        swapchain,
        mode, 
        format,
        found_z_format,
        extent,
        images,
        views,
        image_count,
        z_buffer,
        NULL,
        0,
        MSAAbuffer,
        NULL
    };
}


void vulkanSwapchainCreateFramebuffers(VkDevice device, Swapchain *swapchain) {
    // swapchain framebufferCount is probably redundant 
    swapchain->framebufferCount = swapchain->imageCount;
    swapchain->framebuffers = malloc(swapchain->framebufferCount * sizeof(VkFramebuffer));
    if (swapchain->framebuffers == NULL) {
        fprintf(stderr, "ERROR: failed to allocate memory for framebuffers");
        exit(1);
    }

    for (size_t i = 0; i<swapchain->framebufferCount; ++i) {
        VkImageView all_attachments[] = {swapchain->MSAAbuffer.view, swapchain->z_buffer.view, swapchain->views[i]};
        size_t all_attachments_size = sizeof all_attachments / sizeof(VkImageView); 

        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = swapchain->renderPass;
        framebuffer_info.attachmentCount = all_attachments_size;
        framebuffer_info.pAttachments = all_attachments;
        framebuffer_info.width = swapchain->extent.width;
        framebuffer_info.height = swapchain->extent.height;
        framebuffer_info.layers = 1;

        VK_CHECK(vkCreateFramebuffer(device, &framebuffer_info, NULL, &(swapchain->framebuffers[i])));
    }

    fprintf(stderr, "INFO: Framebuffers created successfully\n");
}



Shader vulkanCreateShaderModule(VkDevice device, const char *filename) {
    // I don't really know if VkShaderModule uses our code pointer or if it copies its contents?
    // Also, is mallocs result properly alligned? 
    
    size_t shader_code_size;
    char *code = readEntireFile(filename, &shader_code_size);
    if (code == NULL) {
        fprintf(stderr, "ERROR: failed to load shader: %s", filename);
        exit(1);
    }
    
    VkShaderModuleCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = shader_code_size;
    create_info.pCode = (uint32_t*)code;

    VkResult res;
    VkShaderModule module;
    if ((res = vkCreateShaderModule(device, &create_info, NULL, &module)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to crate shader module. Error code: %d", res);
        exit(1);
    }

    fprintf(stderr, "INFO: Successfully loaded shader: %s\n", filename);

    return (Shader) {
        module,
        code,
        shader_code_size
    }; 
}


void vulkanSwapchainCreateRenderPass(GPU gpu, VkDevice device, Swapchain *swapchain) {
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = swapchain->surfaceFormat.format;
    // NOTE: this field is connected with multisampling
    color_attachment.samples = gpu.multisampling;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
   

    VkAttachmentDescription depth_attachment = {0};
    depth_attachment.format = swapchain->zBufferFormat;
    depth_attachment.samples = gpu.multisampling;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;    

    VkAttachmentReference depth_attachment_ref = {0};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkAttachmentDescription multisampling_color_resolve_attachment = {0};
    multisampling_color_resolve_attachment.format = swapchain->surfaceFormat.format;
    multisampling_color_resolve_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_color_resolve_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    multisampling_color_resolve_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; 
    multisampling_color_resolve_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    multisampling_color_resolve_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    multisampling_color_resolve_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    multisampling_color_resolve_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference multisampling_color_resolve_attachment_reference= {0};
    multisampling_color_resolve_attachment_reference.attachment = 2;
    multisampling_color_resolve_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;  
    subpass.pResolveAttachments = &multisampling_color_resolve_attachment_reference;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; 
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; 


    VkAttachmentDescription all_attachments[] = {color_attachment, depth_attachment, multisampling_color_resolve_attachment};
    size_t all_attachments_size = sizeof all_attachments / sizeof(VkAttachmentDescription);

    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = all_attachments_size;
    render_pass_info.pAttachments = all_attachments;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;


    VK_CHECK(vkCreateRenderPass(device, &render_pass_info, NULL, &swapchain->renderPass));

    fprintf(stderr, "INFO: Render pass created successfully\n");
}


VulkanPipelineLayout vulkanCreatePipelineLayout(VkDevice device, VkDescriptorSetLayout layout) {
    VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &layout;

    VkPipelineLayout pipeline_layout;
    VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &pipeline_layout));
        
    return (VulkanPipelineLayout) {
        pipeline_layout,
    };
}


VkPipeline vulkanCreatePipeline(GPU gpu, VkDevice device, Swapchain swapchain, Shader frag, Shader vert, VkPipelineLayout pipeline_layout) {
    VkResult err;

    VkPipelineShaderStageCreateInfo vert_shader_info = {0};
    vert_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_info.module = vert.module;
    vert_shader_info.pName = "main";    


    VkPipelineShaderStageCreateInfo frag_shader_info = {0};
    frag_shader_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_info.module = frag.module;
    frag_shader_info.pName = "main";


    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {0};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;


    VkVertexInputBindingDescription vertex_binding_description = {0};
    vertex_binding_description.binding = 0;
    vertex_binding_description.stride = sizeof(Vertex);
    vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertex_position_description = {0};
    vertex_position_description.binding = 0;
    vertex_position_description.location = 0;
    vertex_position_description.format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_position_description.offset = offsetof(Vertex, pos);

    VkVertexInputAttributeDescription vertex_color_description = {0};
    vertex_color_description.binding = 0;
    vertex_color_description.location = 1;
    vertex_color_description.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertex_color_description.offset = offsetof(Vertex, color);

    VkVertexInputAttributeDescription descriptions[] = { 
        vertex_position_description,
        vertex_color_description
    };
    size_t descriptions_size = sizeof descriptions / sizeof(VkVertexInputAttributeDescription);

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.pVertexBindingDescriptions = &vertex_binding_description;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexAttributeDescriptions = descriptions;
    vertex_input_info.vertexAttributeDescriptionCount = descriptions_size;

   
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {0};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = sizeof dynamic_states / sizeof(VkDynamicState);
    dynamic_state.pDynamicStates = dynamic_states;


    VkViewport viewport = {0};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float) swapchain.extent.width;
    viewport.height = (float) swapchain.extent.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;

    VkRect2D scissors = {0};
    scissors.offset = (VkOffset2D){0, 0};
    scissors.extent = swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_state_info = {0};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.pViewports = &viewport;
    viewport_state_info.scissorCount = 1;
    viewport_state_info.pScissors = &scissors;

    
    VkPipelineRasterizationStateCreateInfo rasterization_info = {0};
    rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // @TODO: try to change it to VK_TRUE. This thing requires a feature, and i don't wanna bother myself implementing it right now.
    rasterization_info.depthClampEnable = VK_FALSE;
    rasterization_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_info.lineWidth = 1.0f;
    rasterization_info.cullMode = VK_CULL_MODE_NONE;
    rasterization_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_info.depthBiasEnable = VK_FALSE;

    // @TODO: check out multisampling. As of for now it is disabled
    VkPipelineMultisampleStateCreateInfo multisampling_info = {0};
    multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_info.rasterizationSamples = gpu.multisampling;   
    // @SPEED: same as in createLogicalDevice
    multisampling_info.sampleShadingEnable = VK_TRUE;
    multisampling_info.sampleShadingEnable = VK_TRUE;


    // @TODO: It is also empty for the same reason
    VkPipelineColorBlendAttachmentState colorblend_attachment = {0};
    colorblend_attachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorblend_attachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorblend_info = {0};
    colorblend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

    // @TODO: logic op is an extension, so i have to check if is is available
    colorblend_info.logicOpEnable = VK_FALSE;
    colorblend_info.logicOp = VK_LOGIC_OP_AND; // useless, as  logicOpEnable is set to VK_FALSE;
    colorblend_info.attachmentCount = 1;
    colorblend_info.pAttachments = &colorblend_attachment;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {0};
    depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_info.depthTestEnable = VK_TRUE;
    depth_stencil_info.depthWriteEnable = VK_TRUE;
    depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_info.stencilTestEnable = VK_FALSE;

    VkPipelineShaderStageCreateInfo stages[] = {vert_shader_info, frag_shader_info}; 

    VkGraphicsPipelineCreateInfo pipeline_info = {0};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = sizeof stages / sizeof (VkPipelineShaderStageCreateInfo);
    pipeline_info.pStages = stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_state_info;
    pipeline_info.pRasterizationState = &rasterization_info;
    pipeline_info.pMultisampleState = &multisampling_info;
    pipeline_info.pColorBlendState = &colorblend_info;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.pDepthStencilState = &depth_stencil_info;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = swapchain.renderPass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    VkPipeline pipeline;
    if ((err = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to create graphics pipeline. Error code: %d",err);
        exit(1);
    }

    fprintf(stderr, "INFO: Pipeline created successfully\n");
    return pipeline;
}



void vulkanRecordCommandBuffer(VkCommandBuffer command_buffer, Swapchain swapchain, VkPipeline pipeline, VkPipelineLayout pipeline_layout, VulkanBuffer vertex_buffer, UniformBuffer uniform, uint32_t image_index, uint32_t vertices_count) {
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult res;
    if ((res = vkBeginCommandBuffer(command_buffer, &begin_info)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: failed to begin recording command buffer\nIt is probably an application bug");
        exit(1);
    } 

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkClearValue clear_z_buffer = {{{1.0f, 0}}};
    
    VkClearValue all_clear_values[] = {clear_color, clear_z_buffer};
    size_t all_clear_values_size = sizeof all_clear_values / sizeof(VkClearValue);

    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = swapchain.renderPass;
    render_pass_info.framebuffer = swapchain.framebuffers[image_index];
    render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
    render_pass_info.renderArea.extent = swapchain.extent;
    render_pass_info.clearValueCount = all_clear_values_size;
    render_pass_info.pClearValues = all_clear_values;


    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        vkCmdBindDescriptorSets(
            command_buffer, 
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout,
            0,
            1,
            &uniform.set,
            0, 
            NULL
        );

        VkDeviceSize vertex_buffer_offset = 0;
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer.buffer, &vertex_buffer_offset);
         
        VkViewport viewport = {0};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapchain.extent.width;
        viewport.height = (float)swapchain.extent.height;
        viewport.minDepth = 0.0f;
        // ...
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
 
        VkRect2D scissors = {0};
        scissors.offset = (VkOffset2D){0, 0};
        scissors.extent = swapchain.extent;
        vkCmdSetScissor(command_buffer, 0, 1, &scissors);
    
        vkCmdDraw(command_buffer, vertices_count, 1, 0, 0);
    
    vkCmdEndRenderPass(command_buffer);
    if ((res = vkEndCommandBuffer(command_buffer)) != VK_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to record command buffer. Error code: %d", res);
    }
}


SyncObjects createSyncObjects(VkDevice device) {
    SyncObjects objs;

    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    bool res = true;
    res = res && (vkCreateSemaphore(device, &semaphore_info, NULL, &objs.imageAvailable) == VK_SUCCESS); 
    res = res && (vkCreateSemaphore(device, &semaphore_info, NULL, &objs.renderFinished) == VK_SUCCESS);
    res = res && (vkCreateFence(device, &fence_info, NULL, &objs.inFlight) == VK_SUCCESS);

    if (res != true) {
        fprintf(stderr, "ERROR: Failed to allocate synchronisation primitives");
        exit(1);
    }

    fprintf(stderr, "INFO: Synchronisation primitives allocated successfully\n");
    return objs; 
}


UniformBuffer vulkanCreateUniformBuffer(GPU gpu, VkDevice device) {
    size_t buffer_size = sizeof(Ubo);
    VulkanBuffer buffer = vulkanCreateBuffer (
        gpu, 
        device,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer_size
    );
    
    void *mapped_memory;
    VK_CHECK(vkMapMemory (
        device, 
        buffer.memory,
        0,
        buffer_size, 
        0,
        &mapped_memory
    ));

    VkDescriptorPoolSize pool_size = {0};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = 1;
   
    VkDescriptorPool descriptor_pool;
    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, NULL, &descriptor_pool));

    VkDescriptorSetLayout layout;
    /* Creating layout */ { 
        VkDescriptorSetLayoutBinding ubo_binding = {0};
        ubo_binding.binding = 0;
        ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_binding.descriptorCount = 1;
        ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        ubo_binding.pImmutableSamplers = NULL;
    
        VkDescriptorSetLayoutCreateInfo layout_info = {0};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = 1;
        layout_info.pBindings = &ubo_binding;
            
    VK_CHECK(vkCreateDescriptorSetLayout(device, &layout_info, NULL, &layout));
    }
    
    VkDescriptorSetAllocateInfo info = {0};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = descriptor_pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet set;

    VK_CHECK(vkAllocateDescriptorSets(device, &info, &set));

    VkDescriptorBufferInfo buffer_info = {0};
    buffer_info.buffer = buffer.buffer;
    buffer_info.offset = 0;
    buffer_info.range = buffer_size;

    VkWriteDescriptorSet descriptor_write = {0};
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet = set;
    descriptor_write.dstBinding = 0; 
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, NULL);

    fprintf(stderr, "INFO: Uniform buffer allocated successfully\n");
    return (UniformBuffer) {
        buffer,
        mapped_memory,
        descriptor_pool,
        set,
        layout
    };   
}



void freeVulkanImage(VkDevice device, VulkanImage image) {
    vkDestroyImage(device, image.image, NULL);
    vkFreeMemory(device, image.memory, NULL);
    vkDestroyImageView(device, image.view, NULL);
}

void freeSyncObjects(VkDevice device, SyncObjects objects) {
    vkDestroySemaphore(device, objects.imageAvailable, NULL);
    vkDestroySemaphore(device, objects.renderFinished, NULL);
    vkDestroyFence(device, objects.inFlight, NULL);
}

void freeVulkanBuffer(VkDevice device, VulkanBuffer buffer) {
    vkDestroyBuffer(device, buffer.buffer, NULL);
    vkFreeMemory(device, buffer.memory, NULL);
}

void freePipelineLayout(VkDevice device, VulkanPipelineLayout layout) {
    vkDestroyPipelineLayout(device, layout.layout, NULL);
}

void freeShader(VkDevice device, Shader shader) {
    vkDestroyShaderModule(device, shader.module, NULL);
    free(shader.code);
}

void freeSwapchain(VkDevice device, Swapchain swapchain) {
    for (size_t i=0;i<swapchain.imageCount; ++i) {
        vkDestroyImageView(device, swapchain.views[i], NULL);
    }
    vkDestroySwapchainKHR(device, swapchain.swapchain, NULL);
    free(swapchain.images);
    free(swapchain.views); 

    for (size_t i=0;i<swapchain.framebufferCount;++i) {
        vkDestroyFramebuffer(device, swapchain.framebuffers[i], NULL);
    }
    free(swapchain.framebuffers);

    vkDestroyRenderPass(device, swapchain.renderPass, NULL);
    freeVulkanImage(device, swapchain.z_buffer);
    freeVulkanImage(device, swapchain.MSAAbuffer);
}

void freeUniformBuffer(VkDevice device, UniformBuffer uniform) {
    vkDestroyDescriptorSetLayout(device, uniform.layout, NULL);
    vkDestroyDescriptorPool(device, uniform.pool, NULL);
    vkUnmapMemory(device, uniform.buffer.memory);    
    freeVulkanBuffer(device, uniform.buffer);
}



Vulkan vulkanCompleteInit(GLFWwindow *window, const char *validation_layers[], size_t validation_layer_count, const char *vertex_shader_path, const char *fragment_shader_path) {
    VkInstance instance = vulkanInit(validation_layers, validation_layer_count);

    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window, NULL, &surface));
    
    GPU gpu = vulkanChooseGpu(instance, surface);
    VkDevice device = createLogicalDevice(gpu);

    VkQueue graphics_queue;
    VkQueue present_queue;
    vkGetDeviceQueue (
            device,
            gpu.graphicsFamilyIndex,
            0,
            &graphics_queue
    );
    vkGetDeviceQueue (
            device,
            gpu.presentFamilyIndex,
            0,
            &present_queue
    );

    Shader vert = vulkanCreateShaderModule(device, vertex_shader_path);
    Shader frag = vulkanCreateShaderModule(device, fragment_shader_path);

    Swapchain swapchain = vulkanInitSwapchain(gpu, device, surface, window); 
    vulkanSwapchainCreateRenderPass(gpu, device, &swapchain);
    vulkanSwapchainCreateFramebuffers(device, &swapchain);

    UniformBuffer uniform_buffer = vulkanCreateUniformBuffer(gpu, device);

    VulkanPipelineLayout pipeline_layout = vulkanCreatePipelineLayout(device, uniform_buffer.layout);
    VkPipeline pipeline = vulkanCreatePipeline(gpu, device, swapchain, frag, vert, pipeline_layout.layout); 
    VkCommandPool command_pool = vulkanCreateCommandPool(device, gpu);
    VkCommandBuffer command_buffer = vulkanCreateCommandBuffer(device, command_pool);

    SyncObjects sync_objects = createSyncObjects(device);  
    
    return (Vulkan) {
        instance,
        surface,
        gpu,
        device,
        graphics_queue,
        present_queue,
        swapchain,
        pipeline_layout,
        pipeline,
        command_pool,
        command_buffer,
        sync_objects,
        uniform_buffer,
        vert,
        frag,
        (VulkanBuffer) {0},
        (VulkanBuffer) {0},
    };
}


void vulkanCreateVertexBuffer(Vulkan *vulkan, Vertex *vertices, size_t vertices_size_bytes) {
    vulkan->staging_buffer = vulkanCreateBuffer(vulkan->gpu, vulkan->device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertices_size_bytes);
    vulkanFillMemory(vulkan->device, vulkan->staging_buffer.memory, vertices, vertices_size_bytes); 
    vulkan->vertex_buffer = vulkanCreateBuffer(vulkan->gpu, vulkan->device, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertices_size_bytes);
    vulkanCopyBuffer(vulkan->device, vulkan->graphics_queue, vulkan->command_pool, vulkan->staging_buffer, vulkan->vertex_buffer, vertices_size_bytes);
}


void vulkanFree(Vulkan *vulkan) {
    vkDeviceWaitIdle(vulkan->device);

    freeShader(vulkan->device, vulkan->frag);
    freeShader(vulkan->device, vulkan->vert);
    freeSyncObjects(vulkan->device, vulkan->sync);
    vkDestroyCommandPool(vulkan->device, vulkan->command_pool, NULL);
    freePipelineLayout(vulkan->device, vulkan->pipeline_layout);

    freeUniformBuffer(vulkan->device, vulkan->uniform_buffer);
    freeVulkanBuffer(vulkan->device, vulkan->vertex_buffer); 
    freeVulkanBuffer(vulkan->device, vulkan->staging_buffer);
    vkDestroyPipeline(vulkan->device, vulkan->pipeline, NULL);
    freeSwapchain(vulkan->device, vulkan->swapchain);
    vkDestroyDevice(vulkan->device, NULL);
    vkDestroySurfaceKHR(vulkan->instance, vulkan->surface, NULL); 
    vkDestroyInstance(vulkan->instance, NULL);
}

void vulkanRecreateSwapchain(Vulkan *vulkan, GLFWwindow *window) {
    vkDeviceWaitIdle(vulkan->device);

    freeSwapchain(vulkan->device, vulkan->swapchain);

    vulkan->swapchain = vulkanInitSwapchain(vulkan->gpu, vulkan->device, vulkan->surface, window);
    vulkanSwapchainCreateRenderPass(vulkan->gpu, vulkan->device, &vulkan->swapchain);
    vulkanSwapchainCreateFramebuffers(vulkan->device, &vulkan->swapchain);
}

#endif
