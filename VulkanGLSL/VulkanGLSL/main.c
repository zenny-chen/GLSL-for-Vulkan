// main.c : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <vulkan/vulkan.h>

#ifdef _WIN32

static inline FILE* OpenFileWithRead(const char* filePath)
{
    FILE* fp = NULL;
    if (fopen_s(&fp, filePath, "rb") != 0)
    {
        if (fp != NULL)
        {
            fclose(fp);
            fp = NULL;
        }
    }
    return fp;
}

#else

#define sprintf_s(buffer, maxBufferSize, fmt, ...)      sprintf((buffer), fmt, ## __VA_ARGS__)

static inline FILE* OpenFileWithRead(const char* filePath)
{
    return fopen(filePath, "r");
}

#endif // _WIN32

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif // !max


enum MY_CONSTANTS
{
    MAX_VULKAN_LAYER_COUNT = 64,
    MAX_VULKAN_GLOBAL_EXT_PROPS = 256,
    MAX_EXEC_PROPS_COUNT = 32,

    MAX_GPU_COUNT = 8,
    MAX_QUEUE_FAMILY_PROPERTY_COUNT = 8
};

static VkLayerProperties s_layerProperties[MAX_VULKAN_LAYER_COUNT];
static const char* s_layerNames[MAX_VULKAN_LAYER_COUNT];
static VkExtensionProperties s_instanceExtensions[MAX_VULKAN_LAYER_COUNT][MAX_VULKAN_GLOBAL_EXT_PROPS];
static uint32_t s_layerCount;
static uint32_t s_instanceExtensionCounts[MAX_VULKAN_LAYER_COUNT];

static VkInstance s_instance = VK_NULL_HANDLE;
static VkDevice s_specDevice = VK_NULL_HANDLE;
static uint32_t s_specQueueFamilyIndex = 0;
static VkPhysicalDeviceMemoryProperties s_memoryProperties = { 0 };

static PFN_vkGetPipelineExecutablePropertiesKHR dyn_vkGetPipelineExecutablePropertiesKHR = NULL;
static PFN_vkGetPipelineExecutableInternalRepresentationsKHR dyn_vkGetPipelineExecutableInternalRepresentationsKHR = NULL;
static PFN_vkGetPipelineExecutableStatisticsKHR dyn_vkGetPipelineExecutableStatisticsKHR = NULL;

static uint32_t s_maxWorkgroupSize = 1;
static bool s_supportBufferDeviceAddress = true;
static bool s_supportPipelineExecutableProperties = false;

static const char* const s_deviceTypes[] = {
    "Other",
    "Integrated GPU",
    "Discrete GPU",
    "Virtual GPU",
    "CPU"
};

static VkResult init_global_extension_properties(uint32_t layerIndex)
{
    uint32_t instance_extension_count;
    VkResult res;
    VkLayerProperties* currLayer = &s_layerProperties[layerIndex];
    char const* const layer_name = currLayer->layerName;
    s_layerNames[layerIndex] = layer_name;

    do {
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, NULL);
        if (res != VK_SUCCESS) {
            return res;
        }

        if (instance_extension_count == 0) {
            return VK_SUCCESS;
        }
        if (instance_extension_count > MAX_VULKAN_GLOBAL_EXT_PROPS) {
            instance_extension_count = MAX_VULKAN_GLOBAL_EXT_PROPS;
        }

        s_instanceExtensionCounts[layerIndex] = instance_extension_count;
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, s_instanceExtensions[layerIndex]);
    } while (res == VK_INCOMPLETE);

    return res;
}

static VkResult init_global_layer_properties(void)
{
    uint32_t instance_layer_count;
    VkResult res;

    /*
     * It's possible, though very rare, that the number of
     * instance layers could change. For example, installing something
     * could include new layers that the loader would pick up
     * between the initial query for the count and the
     * request for VkLayerProperties. The loader indicates that
     * by returning a VK_INCOMPLETE status and will update the
     * the count parameter.
     * The count parameter will be updated with the number of
     * entries loaded into the data pointer - in case the number
     * of layers went down or is smaller than the size given.
    */
    do
    {
        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        if (res != VK_SUCCESS) {
            return res;
        }

        if (instance_layer_count == 0) {
            return VK_SUCCESS;
        }

        if (instance_layer_count > MAX_VULKAN_LAYER_COUNT) {
            instance_layer_count = MAX_VULKAN_LAYER_COUNT;
        }

        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, s_layerProperties);
    } while (res == VK_INCOMPLETE);

    /*
     * Now gather the extension list for each instance layer.
    */
    s_layerCount = instance_layer_count;
    for (uint32_t i = 0; i < instance_layer_count; i++)
    {
        res = init_global_extension_properties(i);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "Query global extension properties error: %d\n", res);
            break;
        }
    }

    return res;
}

static VkResult InitializeInstance(void)
{
    VkResult result = init_global_layer_properties();
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "init_global_layer_properties failed: %d\n", result);
        return result;
    }
    printf("Found %u layer(s)...\n", s_layerCount);

    // Check whether a validation layer exists
    for (uint32_t i = 0; i < s_layerCount; ++i)
    {
        if (strstr(s_layerNames[i], "validation") != NULL)
        {
            printf("Contains %s!\n", s_layerNames[i]);
            break;
        }
    }

    // Query the API version
    uint32_t apiVersion = VK_API_VERSION_1_0;
    vkEnumerateInstanceVersion(&apiVersion);
    printf("Current API version: %u.%u.%u\n", VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));

    // initialize the VkApplicationInfo structure
    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "Vulkan Test",
        .applicationVersion = 1,
        .pEngineName = "My Engine",
        .engineVersion = 1,
        .apiVersion = apiVersion
    };

    // initialize the VkInstanceCreateInfo structure
    const VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0, // s_layerCount,
        .ppEnabledLayerNames = s_layerNames
    };

    result = vkCreateInstance(&inst_info, NULL, &s_instance);
    if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        puts("cannot find a compatible Vulkan ICD");
    }
    else if (result != VK_SUCCESS) {
        fprintf(stderr, "vkCreateInstance failed: %d\n", result);
    }

    return result;
}

static VkResult InitializeDevice(VkQueueFlagBits queueFlag, VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    VkPhysicalDevice physicalDevices[MAX_GPU_COUNT] = { VK_NULL_HANDLE };
    uint32_t gpu_count = 0;
    VkResult res = vkEnumeratePhysicalDevices(s_instance, &gpu_count, NULL);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumeratePhysicalDevices failed: %d\n", res);
        return res;
    }

    if (gpu_count > MAX_GPU_COUNT) {
        gpu_count = MAX_GPU_COUNT;
    }

    res = vkEnumeratePhysicalDevices(s_instance, &gpu_count, physicalDevices);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumeratePhysicalDevices failed: %d\n", res);
        return res;
    }

    // TODO: The following code is used to choose the working device and may be not necessary to other projects...
    const bool isSingle = gpu_count == 1;
    printf("This application has detected there %s %u Vulkan capable device%s installed: \n",
        isSingle ? "is" : "are",
        gpu_count,
        isSingle ? "" : "s");

    VkPhysicalDeviceProperties props = { 0 };
    for (uint32_t i = 0; i < gpu_count; i++)
    {
        vkGetPhysicalDeviceProperties(physicalDevices[i], &props);
        printf("\n======== Device %u info ========\n", i);
        printf("Device name: %s\n", props.deviceName);
        printf("Device type: %s\n", s_deviceTypes[props.deviceType]);
        printf("Vulkan API version: %u.%u.%u\n", VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
        printf("Driver version: %08X\n", props.driverVersion);
    }
    puts("\nPlease choose which device to use...");

#ifdef _WIN32
    char inputBuffer[8] = { '\0' };
    const char* input = gets_s(inputBuffer, sizeof(inputBuffer));
    if (input == NULL) {
        input = "0";
    }
    const uint32_t deviceIndex = atoi(input);
#else
    char* input = NULL;
    ssize_t initLen = 0;
    const ssize_t len = getline(&input, &initLen, stdin);
    input[len - 1] = '\0';
    errno = 0;
    const uint32_t deviceIndex = (uint32_t)strtoul(input, NULL, 10);
    if (errno != 0)
    {
        printf("Input error: %d! Invalid integer input!!\n", errno);
        return VK_ERROR_DEVICE_LOST;
    }
#endif // WIN32

    if (deviceIndex >= gpu_count)
    {
        fprintf(stderr, "Your input (%u) exceeds the max number of available devices (%u)\n", deviceIndex, gpu_count);
        return VK_ERROR_DEVICE_LOST;
    }
    printf("You have chosen device[%u]...\n", deviceIndex);

    // Query Vulkan extensions the current selected physical device supports
    uint32_t extPropCount = 0U;
    res = vkEnumerateDeviceExtensionProperties(physicalDevices[deviceIndex], NULL, &extPropCount, NULL);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumerateDeviceExtensionProperties for count failed: %d\n", res);
        return res;
    }
    printf("The current selected physical device supports %u Vulkan extensions!\n", extPropCount);
    if (extPropCount > MAX_VULKAN_GLOBAL_EXT_PROPS) {
        extPropCount = MAX_VULKAN_GLOBAL_EXT_PROPS;
    }

    VkExtensionProperties extProps[MAX_VULKAN_GLOBAL_EXT_PROPS];
    res = vkEnumerateDeviceExtensionProperties(physicalDevices[deviceIndex], NULL, &extPropCount, extProps);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkEnumerateDeviceExtensionProperties for content failed: %d\n", res);
        return res;
    }

    bool support8BitStorage = false;
    bool supportShaderFloat16Int8 = false;
    bool supportBufferDeviceAddress = false;
    for (uint32_t i = 0; i < extPropCount; ++i)
    {
        if (strcmp(extProps[i].extensionName, VK_KHR_8BIT_STORAGE_EXTENSION_NAME) == 0)
        {
            support8BitStorage = true;
            puts("The current device supports `VK_KHR_8bit_storage` extension!");
        }
        if (strcmp(extProps[i].extensionName, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME) == 0)
        {
            supportShaderFloat16Int8 = true;
            puts("The current device supports `VK_KHR_shader_float16_int8` extension!");
        }
        if (strcmp(extProps[i].extensionName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0)
        {
            supportBufferDeviceAddress = true;
            puts("The current device supports `VK_KHR_buffer_device_address` extension!");
        }
        if (strcmp(extProps[i].extensionName, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME) == 0)
        {
            s_supportPipelineExecutableProperties = true;
            puts("The current device supports `VK_KHR_pipeline_executable_properties` extension!");
        }
    }

    if (!support8BitStorage) {
        puts("The current device does not support `VK_KHR_8bit_storag` feature which is required for the test!");
    }
    if (!supportShaderFloat16Int8) {
        puts("The current device does not support `VK_KHR_shader_float16_int8` feature which may be required for the test!");
    }
    if (!supportBufferDeviceAddress)
    {
        puts("The current device does not support `VK_KHR_buffer_device_address` extension feature which may be required for the test!");
        s_supportBufferDeviceAddress = false;
    }
    if (!s_supportPipelineExecutableProperties) {
        puts("The current device does not support `VK_KHR_pipeline_executable_properties` extension!");
    }
    else
    {
        dyn_vkGetPipelineExecutablePropertiesKHR = (PFN_vkGetPipelineExecutablePropertiesKHR)vkGetInstanceProcAddr(s_instance, "vkGetPipelineExecutablePropertiesKHR");
        dyn_vkGetPipelineExecutableInternalRepresentationsKHR = (PFN_vkGetPipelineExecutableInternalRepresentationsKHR)vkGetInstanceProcAddr(s_instance, "vkGetPipelineExecutableInternalRepresentationsKHR");
        dyn_vkGetPipelineExecutableStatisticsKHR = (PFN_vkGetPipelineExecutableStatisticsKHR)vkGetInstanceProcAddr(s_instance, "vkGetPipelineExecutableStatisticsKHR");
    }

    VkPhysicalDevice8BitStorageFeatures shader8BitStorageFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES,
        .pNext = NULL
    };

    VkPhysicalDeviceShaderFloat16Int8Features shaderFloat16Int8Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES,
        // link to shader8BitStorageFeatures node
        .pNext = &shader8BitStorageFeatures
    };

    VkPhysicalDeviceBufferDeviceAddressFeatures deviceBufferAddresFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        // link to shaderFloat16Int8Features node
        .pNext = &shaderFloat16Int8Features
    };

    VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR pipelineExecutablePropertiesFeature = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR,
        // link to deviceBufferAddresFeatures node
        .pNext = &deviceBufferAddresFeatures
    };

    // physical device feature 2
    VkPhysicalDeviceFeatures2 features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        // link to pipelineExecutablePropertiesFeature node
        .pNext = &pipelineExecutablePropertiesFeature
    };

    // Query all above features
    vkGetPhysicalDeviceFeatures2(physicalDevices[deviceIndex], &features2);

    if (features2.features.shaderInt64 == VK_FALSE)
    {
        puts("ShaderInt64 feature is not enabled by default... The test will enable it automatically!");
        features2.features.shaderInt64 = VK_TRUE;
    }

    if (shader8BitStorageFeatures.storageBuffer8BitAccess != VK_FALSE) {
        puts("Support storageBuffer8BitAccess!");
    }
    if (shader8BitStorageFeatures.uniformAndStorageBuffer8BitAccess != VK_FALSE) {
        puts("Support uniformAndStorageBuffer8BitAccess!");
    }
    if (shader8BitStorageFeatures.storagePushConstant8 != VK_FALSE) {
        puts("Support storagePushConstant8!");
    }

    if (shaderFloat16Int8Features.shaderFloat16 != VK_FALSE) {
        puts("Support shaderFloat16!");
    }
    if (shaderFloat16Int8Features.shaderInt8 != VK_FALSE) {
        puts("Support shaderInt8!");
    }

    if (deviceBufferAddresFeatures.bufferDeviceAddress != VK_FALSE) {
        puts("Support bufferDeviceAddress!");
    }
    else
    {
        fprintf(stderr, "The current device does not support VK_KHR_buffer_device_address feature! The demo cannot be run...\n");
        s_supportBufferDeviceAddress = false;
    }
    if (deviceBufferAddresFeatures.bufferDeviceAddressCaptureReplay != VK_FALSE) {
        puts("Support bufferDeviceAddressCaptureReplay!");
    }
    if (deviceBufferAddresFeatures.bufferDeviceAddressMultiDevice != VK_FALSE) {
        puts("Support bufferDeviceAddressMultiDevice!");
    }

    if (dyn_vkGetPipelineExecutablePropertiesKHR != NULL) {
        pipelineExecutablePropertiesFeature.pipelineExecutableInfo = VK_TRUE;
    }

    // ==== Query the current selected device properties corresponding the above features ====
    
    // SubgroupSize properties
    VkPhysicalDeviceSubgroupProperties subgroupSizeProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES,
        .pNext = NULL
    };

    VkPhysicalDeviceDriverProperties driverProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
        // link to subgroupSizeProps
        .pNext = &subgroupSizeProps
    };

    VkPhysicalDeviceProperties2 properties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        // link to driverProps
        .pNext = &driverProps
    };

    // Query all above properties
    vkGetPhysicalDeviceProperties2(physicalDevices[deviceIndex], &properties2);

    s_maxWorkgroupSize = properties2.properties.limits.maxComputeWorkGroupInvocations;

    printf("Detail driver info: %s %s\n", driverProps.driverName, driverProps.driverInfo);
    printf("Current device max workgroup size: %u\n", s_maxWorkgroupSize);
    printf("Current device subgroup size: %u\n", subgroupSizeProps.subgroupSize);

    // Get device memory properties
    vkGetPhysicalDeviceMemoryProperties(physicalDevices[deviceIndex], pMemoryProperties);

    const float queue_priorities[1] = { 0.0f };
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities
    };

    uint32_t queueFamilyPropertyCount = 0;
    VkQueueFamilyProperties queueFamilyProperties[MAX_QUEUE_FAMILY_PROPERTY_COUNT];

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[deviceIndex], &queueFamilyPropertyCount, NULL);
    if (queueFamilyPropertyCount > MAX_QUEUE_FAMILY_PROPERTY_COUNT) {
        queueFamilyPropertyCount = MAX_QUEUE_FAMILY_PROPERTY_COUNT;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[deviceIndex], &queueFamilyPropertyCount, queueFamilyProperties);

    bool found = false;
    for (uint32_t i = 0; i < queueFamilyPropertyCount; i++)
    {
        if ((queueFamilyProperties[i].queueFlags & queueFlag) != 0)
        {
            queue_info.queueFamilyIndex = i;
            found = true;
            break;
        }
    }

    s_specQueueFamilyIndex = queue_info.queueFamilyIndex;

    uint32_t extCount = 0;
    const char* extensionNames[4] = { NULL };
    if (support8BitStorage) {
        extensionNames[extCount++] = VK_KHR_8BIT_STORAGE_EXTENSION_NAME;
    }
    if (supportShaderFloat16Int8) {
        extensionNames[extCount++] = VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME;
    }
    if (supportBufferDeviceAddress) {
        extensionNames[extCount++] = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
    }
    if (s_supportPipelineExecutableProperties) {
        extensionNames[extCount++] = VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME;
    }

    // There are two ways to enable features:
    // (1) Set pNext to a VkPhysicalDeviceFeatures2 structure and set pEnabledFeatures to NULL;
    // (2) or set pNext to NULL and set pEnabledFeatures to a VkPhysicalDeviceFeatures structure.
    // Here uses the first way
    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = extCount,
        .ppEnabledExtensionNames = extensionNames,
        .pEnabledFeatures = NULL
    };

    res = vkCreateDevice(physicalDevices[deviceIndex], &device_info, NULL, &s_specDevice);
    if (res != VK_SUCCESS)
    {
        if (res == VK_ERROR_FEATURE_NOT_PRESENT)
        {
            puts("ShaderInt64 feature is not supported on this device! It will be disabled...");
            features2.features.shaderInt64 = VK_FALSE;
            res = vkCreateDevice(physicalDevices[deviceIndex], &device_info, NULL, &s_specDevice);
        }

        if (res != VK_SUCCESS) {
            fprintf(stderr, "vkCreateDevice failed: %d\n", res);
        }
    }

    return res;
}

VkResult InitializeCommandBuffer(uint32_t queueFamilyIndex, VkDevice device, VkCommandPool* pCommandPool,
    VkCommandBuffer commandBuffers[], uint32_t commandBufferCount)
{
    const VkCommandPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueFamilyIndex = queueFamilyIndex
    };

    VkResult res = vkCreateCommandPool(device, &cmd_pool_info, NULL, pCommandPool);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateCommandPool failed: %d\n", res);
        return res;
    }

    // Create the command buffer from the command pool
    const VkCommandBufferAllocateInfo cmdInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = *pCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = commandBufferCount
    };

    res = vkAllocateCommandBuffers(device, &cmdInfo, commandBuffers);
    return res;
}

// deviceMemories[0] as host visible memory;
// deviceMemories[1] as device local memory for src and dst device buffers;
// deviceBuffers[0] as host temporal buffer;
// deviceBuffers[1] as dst device buffer;
// deviceBuffers[2] as src device buffer;
static VkResult AllocateMemoryAndBuffers(VkDevice device, const VkPhysicalDeviceMemoryProperties* pMemoryProperties, VkDeviceMemory deviceMemories[2],
    VkBuffer deviceBuffers[3], VkDeviceSize bufferSize, uint32_t queueFamilyIndex)
{
    const VkDeviceSize hostBufferSize = bufferSize;

    const VkBufferCreateInfo hostBufCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = hostBufferSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = (uint32_t[]){ queueFamilyIndex }
    };

    VkResult res = vkCreateBuffer(device, &hostBufCreateInfo, NULL, &deviceBuffers[0]);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer failed: %d\n", res);
        return res;
    }

    VkMemoryRequirements hostMemBufRequirements = { 0 };
    vkGetBufferMemoryRequirements(device, deviceBuffers[0], &hostMemBufRequirements);

    uint32_t memoryTypeIndex;
    // Find host visible property memory type index
    for (memoryTypeIndex = 0; memoryTypeIndex < pMemoryProperties->memoryTypeCount; memoryTypeIndex++)
    {
        if ((hostMemBufRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
            continue;
        }
        const VkMemoryType memoryType = pMemoryProperties->memoryTypes[memoryTypeIndex];
        if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0 &&
            (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0 &&
            pMemoryProperties->memoryHeaps[memoryType.heapIndex].size >= hostMemBufRequirements.size)
        {
            // found our memory type!
            printf("Host visible memory size: %zuMB\n", pMemoryProperties->memoryHeaps[memoryType.heapIndex].size / (1024 * 1024));
            break;
        }
    }

    const VkMemoryAllocateInfo hostMemAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = hostMemBufRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    res = vkAllocateMemory(device, &hostMemAllocInfo, NULL, &deviceMemories[0]);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateMemory for deviceMemories[0] failed: %d\n", res);
        return res;
    }

    res = vkBindBufferMemory(device, deviceBuffers[0], deviceMemories[0], 0);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindBufferMemory failed: %d\n", res);
        return res;
    }

    const VkBufferCreateInfo deviceBufCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = (uint32_t[]){ queueFamilyIndex }
    };

    res = vkCreateBuffer(device, &deviceBufCreateInfo, NULL, &deviceBuffers[1]);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer failed: %d\n", res);
        return res;
    }

    res = vkCreateBuffer(device, &deviceBufCreateInfo, NULL, &deviceBuffers[2]);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer failed: %d\n", res);
        return res;
    }

    VkMemoryRequirements deviceMemBufRequirements = { 0 };
    vkGetBufferMemoryRequirements(device, deviceBuffers[1], &deviceMemBufRequirements);

    // two memory buffers share one device local memory.
    const VkDeviceSize deviceMemTotalSize = max(bufferSize * 2, deviceMemBufRequirements.size);
    // Find device local property memory type index
    for (memoryTypeIndex = 0; memoryTypeIndex < pMemoryProperties->memoryTypeCount; memoryTypeIndex++)
    {
        if ((deviceMemBufRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
            continue;
        }
        const VkMemoryType memoryType = pMemoryProperties->memoryTypes[memoryTypeIndex];
        if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 &&
            pMemoryProperties->memoryHeaps[memoryType.heapIndex].size >= deviceMemTotalSize)
        {
            // found our memory type!
            printf("Device local VRAM size: %zuMB\n", pMemoryProperties->memoryHeaps[memoryType.heapIndex].size / (1024 * 1024));
            break;
        }
    }

    const VkMemoryAllocateInfo deviceMemAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = NULL,
        .allocationSize = deviceMemTotalSize,
        .memoryTypeIndex = memoryTypeIndex
    };

    res = vkAllocateMemory(device, &deviceMemAllocInfo, NULL, &deviceMemories[1]);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateMemory for deviceMemories[1] failed: %d\n", res);
        return res;
    }

    res = vkBindBufferMemory(device, deviceBuffers[1], deviceMemories[1], 0);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindBufferMemory failed: %d\n", res);
        return res;
    }

    res = vkBindBufferMemory(device, deviceBuffers[2], deviceMemories[1], bufferSize);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindBufferMemory failed: %d\n", res);
        return res;
    }

    // Initialize the source host visible temporal buffer
    const int elemCount = (int)(bufferSize / sizeof(int));
    void* hostBuffer = NULL;
    res = vkMapMemory(device, deviceMemories[0], 0, bufferSize, 0, &hostBuffer);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkMapMemory failed: %d\n", res);
        return res;
    }

    // Initialize the host buffer for buffer data
    int* srcMem = hostBuffer;
    for (int i = 0; i < elemCount; i++) {
        srcMem[i] = i;
    }

    vkUnmapMemory(device, deviceMemories[0]);

    return res;
}

static void WriteBufferAndSync(VkCommandBuffer commandBuffer, uint32_t queueFamilyIndex, VkBuffer dataDeviceBuffer,
    VkBuffer srcHostBuffer, size_t size)
{
    const VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(commandBuffer, srcHostBuffer, dataDeviceBuffer, 1, &copyRegion);

    const VkBufferMemoryBarrier bufferBarriers[] = {
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = queueFamilyIndex,
            .dstQueueFamilyIndex = queueFamilyIndex,
            .buffer = dataDeviceBuffer,
            .offset = 0,
            .size = size
        }
    };

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL,
        (uint32_t)(sizeof(bufferBarriers) / sizeof(bufferBarriers[0])), bufferBarriers, 0, NULL);
}

static void SyncAndReadBuffer(VkCommandBuffer commandBuffer, uint32_t queueFamilyIndex, VkBuffer dstHostBuffer, VkBuffer srcDeviceBuffer, size_t size)
{
    const VkBufferMemoryBarrier bufferBarrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .srcQueueFamilyIndex = queueFamilyIndex,
        .dstQueueFamilyIndex = queueFamilyIndex,
        .buffer = srcDeviceBuffer,
        .offset = 0,
        .size = size
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, NULL, 1, &bufferBarrier, 0, NULL);

    const VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(commandBuffer, srcDeviceBuffer, dstHostBuffer, 1, &copyRegion);
}

VkResult CreateShaderModule(VkDevice device, const char* fileName, VkShaderModule* pShaderModule)
{
    FILE* fp = OpenFileWithRead(fileName);
    if (fp == NULL)
    {
        fprintf(stderr, "Shader file %s not found!\n", fileName);
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    fseek(fp, 0, SEEK_END);
    size_t fileLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint32_t* codeBuffer = malloc(fileLen);
    if (codeBuffer != NULL) {
        fread(codeBuffer, 1, fileLen, fp);
    }
    fclose(fp);

    const VkShaderModuleCreateInfo moduleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = fileLen,
        .pCode = codeBuffer
    };

    VkResult res = vkCreateShaderModule(device, &moduleCreateInfo, NULL, pShaderModule);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateShaderModule failed: %d\n", res);
    }

    free(codeBuffer);

    return res;
}

struct CustomPushConstants
{
    int a;
    unsigned b;
    unsigned paddings[2];
    int secondConstant[4];
};

static VkResult CreateComputePipeline(VkDevice device, VkShaderModule computeShaderModule, VkPipeline* pComputePipeline,
    VkPipelineLayout* pPipelineLayout, VkDescriptorSetLayout* pDescLayout, int specConstantValue)
{
    const VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0},
    };
    const uint32_t bindingCount = (uint32_t)(sizeof(descriptorSetLayoutBindings) / sizeof(descriptorSetLayoutBindings[0]));
    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        NULL, 0, bindingCount, descriptorSetLayoutBindings
    };

    VkResult res = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, NULL, pDescLayout);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateDescriptorSetLayout failed: %d\n", res);
        return res;
    }

    const VkPushConstantRange pushConstRanges[1] = {
        {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = sizeof(struct CustomPushConstants)
        }
    };

    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = pDescLayout,
        .pushConstantRangeCount = sizeof(pushConstRanges) / sizeof(pushConstRanges[0]),
        .pPushConstantRanges = pushConstRanges
    };

    res = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, NULL, pPipelineLayout);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreatePipelineLayout failed: %d\n", res);
        return res;
    }

    const struct SpecializationConstants
    {
        uint32_t local_size_x;
        uint32_t local_size_y;
        uint32_t local_size_z;
        int spec_constant;
    } specConsts = { s_maxWorkgroupSize, 1U, 1U, specConstantValue };

    const VkSpecializationMapEntry mapEntries[4] = {
        {
            .constantID = 0,
            .offset = (uint32_t)offsetof(struct SpecializationConstants, local_size_x),
            .size = sizeof(specConsts.local_size_x)
        },
        {
            .constantID = 1,
            .offset = (uint32_t)offsetof(struct SpecializationConstants, local_size_y),
            .size = sizeof(specConsts.local_size_y)
        },
        {
            .constantID = 2,
            .offset = (uint32_t)offsetof(struct SpecializationConstants, local_size_z),
            .size = sizeof(specConsts.local_size_z)
        },
        {
            .constantID = 3,
            .offset = (uint32_t)offsetof(struct SpecializationConstants, spec_constant),
            .size = sizeof(specConsts.spec_constant)
        }
    };

    const VkSpecializationInfo specializationInfo = {
        .mapEntryCount = (uint32_t)(sizeof(mapEntries) / sizeof(mapEntries[0])),
        .pMapEntries = mapEntries,
        .dataSize = sizeof(specConsts),
        .pData = &specConsts
    };

    const VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = computeShaderModule,
        .pName = "main",
        .pSpecializationInfo = &specializationInfo
    };

    VkPipelineCreateFlags pipelineCreateFlags = 0;
    if (dyn_vkGetPipelineExecutablePropertiesKHR != NULL) {
        pipelineCreateFlags = VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR | VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
    }

    const VkComputePipelineCreateInfo computePipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = pipelineCreateFlags,
        .stage = shaderStageCreateInfo,
        .layout = *pPipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0
    };

    res = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, NULL, pComputePipeline);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateComputePipelines failed: %d\n", res);
        return res;
    }

    if (dyn_vkGetPipelineExecutablePropertiesKHR != NULL)
    {
        VkPipelineExecutablePropertiesKHR executableProperties[MAX_EXEC_PROPS_COUNT] = { 0 };
        VkPipelineExecutableInternalRepresentationKHR internalRepresentations[MAX_EXEC_PROPS_COUNT] = { 0 };
        VkPipelineExecutableStatisticKHR statistics[MAX_EXEC_PROPS_COUNT] = { 0 };
        char strBuf[64] = { '\0' };

        const VkPipelineInfoKHR pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR,
            .pNext = NULL,
            .pipeline = *pComputePipeline
        };

        uint32_t executableCount = 0;
        res = dyn_vkGetPipelineExecutablePropertiesKHR(s_specDevice, &pipelineInfo, &executableCount, NULL);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkGetPipelineExecutablePropertiesKHR for count failed: %d\n", res);
            return res;
        }
        if (executableCount > MAX_EXEC_PROPS_COUNT) {
            executableCount = MAX_EXEC_PROPS_COUNT;
        }
        for (uint32_t i = 0; i < executableCount; ++i) {
            executableProperties[i].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR;
        }

        res = dyn_vkGetPipelineExecutablePropertiesKHR(s_specDevice, &pipelineInfo, &executableCount, executableProperties);
        if (res != VK_SUCCESS)
        {
            fprintf(stderr, "vkGetPipelineExecutablePropertiesKHR for data failed: %d\n", res);
            return res;
        }

        for (uint32_t i = 0; i < executableCount; ++i)
        {
            printf("pipeline[%u]: %s(%s), subgroup size: %u\n", i, executableProperties[i].name, executableProperties[i].description, executableProperties[i].subgroupSize);

            const VkPipelineExecutableInfoKHR executableInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR,
                .pNext = NULL,
                .pipeline = *pComputePipeline,
                .executableIndex = i
            };

            uint32_t internalRepresentationCount = 0;
            res = dyn_vkGetPipelineExecutableInternalRepresentationsKHR(s_specDevice, &executableInfo, &internalRepresentationCount, NULL);
            if (res != VK_SUCCESS)
            {
                fprintf(stderr, "vkGetPipelineExecutableInternalRepresentationsKHR for count failed: %d\n", res);
                return res;
            }
            if (internalRepresentationCount > MAX_EXEC_PROPS_COUNT) {
                internalRepresentationCount = MAX_EXEC_PROPS_COUNT;
            }

            for (uint32_t j = 0; j < internalRepresentationCount; ++j) {
                internalRepresentations[j].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INTERNAL_REPRESENTATION_KHR;
            }
            res = dyn_vkGetPipelineExecutableInternalRepresentationsKHR(s_specDevice, &executableInfo, &internalRepresentationCount, internalRepresentations);
            if (res != VK_SUCCESS)
            {
                fprintf(stderr, "vkGetPipelineExecutableInternalRepresentationsKHR for data failed: %d\n", res);
                return res;
            }

            uint32_t statisticCount = 0;
            res = dyn_vkGetPipelineExecutableStatisticsKHR(s_specDevice, &executableInfo, &statisticCount, NULL);
            if (res != VK_SUCCESS)
            {
                fprintf(stderr, "vkGetPipelineExecutableStatisticsKHR for count failed: %d\n", res);
                return res;
            }
            if (statisticCount > MAX_EXEC_PROPS_COUNT) {
                statisticCount = MAX_EXEC_PROPS_COUNT;
            }

            for (uint32_t j = 0; j < statisticCount; ++j) {
                statistics[j].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
            }

            res = dyn_vkGetPipelineExecutableStatisticsKHR(s_specDevice, &executableInfo, &statisticCount, statistics);
            if (res != VK_SUCCESS)
            {
                fprintf(stderr, "vkGetPipelineExecutableStatisticsKHR for data failed: %d\n", res);
                return res;
            }

            for (uint32_t j = 0; j < statisticCount; ++j)
            {
                switch (statistics[j].format)
                {
                case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
                    sprintf_s(strBuf, sizeof(strBuf), "%s", statistics[j].value.b32 != 0 ? "TRUE" : "FALSE");
                    break;

                case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
                    sprintf_s(strBuf, sizeof(strBuf), "%lld", (long long)statistics[j].value.i64);
                    break;

                case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
                    sprintf_s(strBuf, sizeof(strBuf), "%llu", (unsigned long long)statistics[j].value.u64);
                    break;

                case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
                    sprintf_s(strBuf, sizeof(strBuf), "%f", statistics[j].value.f64);
                    break;

                default:
                    break;
                }

                printf("%s(%s): %s\n", statistics[j].name, statistics[j].description, strBuf);
            }
        }
    }

    return res;
}

static VkResult CreateDescriptorSets(VkDevice device, VkBuffer dstDeviceBuffer, VkBuffer srcDeviceBuffer, VkDeviceSize bufferSize, VkDescriptorSetLayout descLayout,
    VkDescriptorPool* pDescriptorPool, VkDescriptorSet* pDescSets)
{
    const VkDescriptorPoolCreateInfo descriptorPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .maxSets = 2,
        .poolSizeCount = 1,
        .pPoolSizes = (VkDescriptorPoolSize[]) {
            {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 2}
        }
    };
    VkResult res = vkCreateDescriptorPool(device, &descriptorPoolInfo, NULL, pDescriptorPool);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateDescriptorPool failed: %d\n", res);
        return res;
    }

    const VkDescriptorSetAllocateInfo descAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = *pDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descLayout
    };
    res = vkAllocateDescriptorSets(device, &descAllocInfo, pDescSets);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateDescriptorSets failed: %d\n", res);
        return res;
    }

    const VkDescriptorBufferInfo dstDescBufferInfo = {
        .buffer = dstDeviceBuffer,
        .offset = 0,
        .range = bufferSize
    };

    const VkDescriptorBufferInfo srcDescBufferInfo = {
        .buffer = srcDeviceBuffer,
        .offset = 0,
        .range = bufferSize
    };

    const VkWriteDescriptorSet writeDescSet[] = {
        // dstDeviceBuffer
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = NULL,
            .dstSet = *pDescSets,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = NULL,
            .pBufferInfo = (VkDescriptorBufferInfo[]) { dstDescBufferInfo },
            .pTexelBufferView = NULL
        },
        // srcDeviceBuffer
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = NULL,
            .dstSet = *pDescSets,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = NULL,
            .pBufferInfo = (VkDescriptorBufferInfo[]) { srcDescBufferInfo },
            .pTexelBufferView = NULL
        }
    };

    vkUpdateDescriptorSets(device, (uint32_t)(sizeof(writeDescSet) / sizeof(writeDescSet[0])), writeDescSet, 0, NULL);

    return res;
}

static VkResult InitializeInstanceAndeDevice(void)
{
    VkResult result = InitializeInstance();
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "InitializeInstance failed!\n");
        return result;
    }

    result = InitializeDevice(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, &s_memoryProperties);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "InitializeDevice failed!\n");
    }

    return result;
}

static void DestroyInstanceAndDevice(void)
{
    if (s_specDevice != VK_NULL_HANDLE) {
        vkDestroyDevice(s_specDevice, NULL);
    }
    if (s_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(s_instance, NULL);
    }
}

static void RunSimpleComputeTest(void)
{
    puts("\n================ Begin Simple Compute Test ================\n");

    VkDeviceMemory deviceMemories[2] = { VK_NULL_HANDLE };
    // deviceBuffers[0] as host temporal buffer, deviceBuffers[1] as device dst buffer, deviceBuffers[2] as device src buffer
    VkBuffer deviceBuffers[3] = { VK_NULL_HANDLE };
    VkShaderModule computeShaderModule = VK_NULL_HANDLE;
    VkPipeline computePipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffers[1] = { VK_NULL_HANDLE };
    VkFence fence = VK_NULL_HANDLE;
    uint32_t const commandBufferCount = (uint32_t)(sizeof(commandBuffers) / sizeof(commandBuffers[0]));

    do
    {
        const uint32_t elemCount = 25 * 1024 * 1024;
        const VkDeviceSize bufferSize = elemCount * sizeof(int);

        VkResult result = AllocateMemoryAndBuffers(s_specDevice, &s_memoryProperties, deviceMemories, deviceBuffers, bufferSize, s_specQueueFamilyIndex);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "AllocateMemoryAndBuffers failed!\n");
            break;
        }

        result = CreateShaderModule(s_specDevice, "shaders/basic.spv", &computeShaderModule);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "CreateShaderModule failed!\n");
            break;
        }

        result = CreateComputePipeline(s_specDevice, computeShaderModule, &computePipeline, &pipelineLayout, &descriptorSetLayout, 1);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "CreateComputePipeline failed!\n");
            break;
        }

        // There's no need to destroy `descriptorSet`, since VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT flag is not set
        // in `flags` in `VkDescriptorPoolCreateInfo`
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        result = CreateDescriptorSets(s_specDevice, deviceBuffers[1], deviceBuffers[2], bufferSize,
            descriptorSetLayout, &descriptorPool, &descriptorSet);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "CreateDescriptorSets failed!\n");
            break;
        }

        result = InitializeCommandBuffer(s_specQueueFamilyIndex, s_specDevice, &commandPool, commandBuffers, commandBufferCount);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "InitializeCommandBuffer failed!\n");
            break;
        }

        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(s_specDevice, s_specQueueFamilyIndex, 0, &queue);

        const VkCommandBufferBeginInfo cmdBufBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = NULL
        };
        result = vkBeginCommandBuffer(commandBuffers[0], &cmdBufBeginInfo);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "vkBeginCommandBuffer failed: %d\n", result);
            break;
        }

        vkCmdBindPipeline(commandBuffers[0], VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(commandBuffers[0], VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);

        const struct CustomPushConstants pushConstants = { 100, 10U, .secondConstant = {1, 2, 3, 4} };
        vkCmdPushConstants(commandBuffers[0], pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
            0, sizeof(pushConstants), &pushConstants);

        WriteBufferAndSync(commandBuffers[0], s_specQueueFamilyIndex, deviceBuffers[2], deviceBuffers[0], bufferSize);

        vkCmdDispatch(commandBuffers[0], elemCount / s_maxWorkgroupSize, 1, 1);

        SyncAndReadBuffer(commandBuffers[0], s_specQueueFamilyIndex, deviceBuffers[0], deviceBuffers[1], bufferSize);

        result = vkEndCommandBuffer(commandBuffers[0]);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "vkEndCommandBuffer failed: %d\n", result);
            break;
        }

        const VkFenceCreateInfo fenceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0
        };
        result = vkCreateFence(s_specDevice, &fenceCreateInfo, NULL, &fence);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateFence failed: %d\n", result);
            break;
        }

        const VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = NULL,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = NULL,
            .pWaitDstStageMask = NULL,
            .commandBufferCount = commandBufferCount,
            .pCommandBuffers = commandBuffers,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = NULL
        };
        result = vkQueueSubmit(queue, 1, &submit_info, fence);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "vkQueueSubmit failed: %d\n", result);
            break;
        }

        result = vkWaitForFences(s_specDevice, 1, &fence, VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "vkWaitForFences failed: %d\n", result);
            break;
        }

        // Verify the result
        void* hostBuffer = NULL;
        result = vkMapMemory(s_specDevice, deviceMemories[0], 0, bufferSize, 0, &hostBuffer);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "vkMapMemory failed: %d\n", result);
            break;
        }
        int* dstMem = hostBuffer;

        // verification assist buffer
        for (int i = 0; i < (int)elemCount; i++)
        {
            if (dstMem[i] != i + 100)
            {
                fprintf(stderr, "Result error @ %d, result is: %d\n", i, dstMem[i]);
                break;
            }
        }

        printf("The first 5 elements sum = %d\n", dstMem[0] + dstMem[1] + dstMem[2] + dstMem[3] + dstMem[4]);

        vkUnmapMemory(s_specDevice, deviceMemories[0]);
    }
    while (false);

    if (fence != VK_NULL_HANDLE) {
        vkDestroyFence(s_specDevice, fence, NULL);
    }
    if (commandPool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(s_specDevice, commandPool, commandBufferCount, commandBuffers);
        vkDestroyCommandPool(s_specDevice, commandPool, NULL);
    }
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(s_specDevice, descriptorPool, NULL);
    }
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(s_specDevice, descriptorSetLayout, NULL);
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(s_specDevice, pipelineLayout, NULL);
    }
    if (computePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(s_specDevice, computePipeline, NULL);
    }
    if (computeShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(s_specDevice, computeShaderModule, NULL);
    }

    for (size_t i = 0; i < sizeof(deviceBuffers) / sizeof(deviceBuffers[0]); i++)
    {
        if (deviceBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(s_specDevice, deviceBuffers[i], NULL);
        }
    }
    for (size_t i = 0; i < sizeof(deviceMemories) / sizeof(deviceMemories[0]); i++)
    {
        if (deviceMemories[i] != VK_NULL_HANDLE) {
            vkFreeMemory(s_specDevice, deviceMemories[i], NULL);
        }
    }

    puts("\n================ Complete Basic Compute Test ================\n");
}

extern void RunAdvancedComputeTest(VkDevice specDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties, uint32_t specQueueFamilyIndex, unsigned maxWorkgroupSize);

int main(int argc, const char* argv[])
{
    if (InitializeInstanceAndeDevice() == VK_SUCCESS)
    {
        RunSimpleComputeTest();

        if (s_supportBufferDeviceAddress) {
            RunAdvancedComputeTest(s_specDevice, &s_memoryProperties, s_specQueueFamilyIndex, s_maxWorkgroupSize);
        }
        else {
            fprintf(stderr, "The current device does not support `VK_KHR_buffer_device_address` extension feature, so RunAdvancedComputeTest will not run!\n");
        }
    }

    DestroyInstanceAndDevice();
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件

