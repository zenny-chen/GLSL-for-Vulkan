#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <vulkan/vulkan.h>

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif // !max

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>

#define Sleep(duration)    usleep(duration)

#endif // !WIN32


enum MY_CONSTANTS
{
    // additional 64 bytes to store enough adresses (up to 8 addresses)
    ADDITIONAL_ADDRESS_BUFFER_SIZE = 64
};

// deviceMemories[0] as host visible memory;
// deviceMemories[1] as device local memory for src and dst device buffers;
// deviceMemories[2] as device local memory to store up to 8 device buffer addresses and up to 16 uint atomic values;
// deviceBuffers[0] as host temporal buffer;
// deviceBuffers[1] as dst device buffer;
// deviceBuffers[2] as src device buffer;
// deviceBuffers[3] as atomic value device buffer;
// deviceBuffers[4] as address storage device buffer;
static VkResult AllocateMemoryAndBuffers(VkDevice device, const VkPhysicalDeviceMemoryProperties* pMemoryProperties, VkDeviceMemory deviceMemories[2],
    VkBuffer deviceBuffers[5], VkDeviceSize bufferSize, uint32_t queueFamilyIndex)
{
    const VkDeviceSize hostBufferSize = bufferSize + ADDITIONAL_ADDRESS_BUFFER_SIZE * 2;

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

    // ATTENTION: `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT` usage MUST be specified in order to invoke `vkGetBufferDeviceAddress` API
    VkBufferCreateInfo deviceBufCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = bufferSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
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

    deviceBufCreateInfo.size = ADDITIONAL_ADDRESS_BUFFER_SIZE;
    res = vkCreateBuffer(device, &deviceBufCreateInfo, NULL, &deviceBuffers[3]);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer failed: %d\n", res);
        return res;
    }

    VkMemoryRequirements deviceMemBufRequirements = { 0 };
    vkGetBufferMemoryRequirements(device, deviceBuffers[1], &deviceMemBufRequirements);

    // dst memory buffer, src memory buffer and the atomic memory buffer share one device local memory.
    const VkDeviceSize deviceMemTotalSize = max(bufferSize * 2 + ADDITIONAL_ADDRESS_BUFFER_SIZE, deviceMemBufRequirements.size);
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

    // If buffer was created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT bit set,
    // memory must have been allocated with the VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT bit set.
    const VkMemoryAllocateFlagsInfo memAllocFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = NULL,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
    };

    VkMemoryAllocateInfo deviceMemAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &memAllocFlagsInfo,
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

    res = vkBindBufferMemory(device, deviceBuffers[3], deviceMemories[1], bufferSize * 2);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkBindBufferMemory failed: %d\n", res);
        return res;
    }

    const VkBufferCreateInfo addressWrapperDeviceBufCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .size = ADDITIONAL_ADDRESS_BUFFER_SIZE,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = (uint32_t[]){ queueFamilyIndex }
    };

    res = vkCreateBuffer(device, &deviceBufCreateInfo, NULL, &deviceBuffers[4]);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkCreateBuffer failed: %d\n", res);
        return res;
    }

    memset(&deviceMemBufRequirements, 0, sizeof(deviceMemBufRequirements));
    vkGetBufferMemoryRequirements(device, deviceBuffers[4], &deviceMemBufRequirements);

    // Find device local property memory type index
    for (memoryTypeIndex = 0; memoryTypeIndex < pMemoryProperties->memoryTypeCount; memoryTypeIndex++)
    {
        if ((deviceMemBufRequirements.memoryTypeBits & (1U << memoryTypeIndex)) == 0U) {
            continue;
        }
        const VkMemoryType memoryType = pMemoryProperties->memoryTypes[memoryTypeIndex];
        if ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 &&
            pMemoryProperties->memoryHeaps[memoryType.heapIndex].size >= deviceMemBufRequirements.size) {
            // found our memory type!
            break;
        }
    }
    deviceMemAllocInfo.allocationSize = deviceMemBufRequirements.size;
    deviceMemAllocInfo.memoryTypeIndex = memoryTypeIndex;

    res = vkAllocateMemory(device, &deviceMemAllocInfo, NULL, &deviceMemories[2]);
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "vkAllocateMemory for deviceMemories[2] failed: %d\n", res);
        return res;
    }

    res = vkBindBufferMemory(device, deviceBuffers[4], deviceMemories[2], 0);
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

    // Initialize the host buffer for addresses
    VkDeviceAddress* addrMem = (VkDeviceAddress*)((uint8_t*)hostBuffer + bufferSize);
    memset(addrMem, 0, ADDITIONAL_ADDRESS_BUFFER_SIZE);

    // Store dst device buffer address
    VkBufferDeviceAddressInfo addressInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = NULL,
        .buffer = deviceBuffers[1]
    };
    addrMem[0] = vkGetBufferDeviceAddress(device, &addressInfo);

    // Store src device buffer address
    addressInfo.buffer = deviceBuffers[2];
    addrMem[1] = vkGetBufferDeviceAddress(device, &addressInfo);

    // store atomic value buffer address
    addressInfo.buffer = deviceBuffers[3];
    addrMem[2] = vkGetBufferDeviceAddress(device, &addressInfo);

    vkUnmapMemory(device, deviceMemories[0]);

    return res;
}

static void WriteBufferAndSync(VkCommandBuffer commandBuffer, uint32_t queueFamilyIndex, VkBuffer dataDeviceBuffer, VkBuffer addressDeviceBuffer,
    VkBuffer atomicDeviceBuffer, VkBuffer srcHostBuffer, VkDeviceSize dataBufferSize)
{
    // Copy src device buffer from the host buffer
    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = dataBufferSize
    };
    vkCmdCopyBuffer(commandBuffer, srcHostBuffer, dataDeviceBuffer, 1, &copyRegion);

    // Copy address device buffer from the host buffer
    copyRegion.srcOffset = dataBufferSize;
    copyRegion.size = ADDITIONAL_ADDRESS_BUFFER_SIZE;
    vkCmdCopyBuffer(commandBuffer, srcHostBuffer, addressDeviceBuffer, 1, &copyRegion);

    // memset atomic device buffer
    vkCmdFillBuffer(commandBuffer, atomicDeviceBuffer, 0, sizeof(unsigned), 0);

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
            .size = dataBufferSize
        },
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = queueFamilyIndex,
            .dstQueueFamilyIndex = queueFamilyIndex,
            .buffer = addressDeviceBuffer,
            .offset = 0,
            .size = ADDITIONAL_ADDRESS_BUFFER_SIZE
        },
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = queueFamilyIndex,
            .dstQueueFamilyIndex = queueFamilyIndex,
            .buffer = atomicDeviceBuffer,
            .offset = 0,
            .size = sizeof(unsigned)
        }
    };

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL,
        (uint32_t)(sizeof(bufferBarriers) / sizeof(bufferBarriers[0])), bufferBarriers, 0, NULL);
}

static void SyncAndReadBuffer(VkCommandBuffer commandBuffer, uint32_t queueFamilyIndex, VkBuffer dstHostBuffer, VkBuffer srcDeviceDataBuffer,
    VkBuffer srcDeviceAtomicBuffer, VkDeviceSize dataBufferSize)
{
    const VkBufferMemoryBarrier bufferBarriers[] = {
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .srcQueueFamilyIndex = queueFamilyIndex,
            .dstQueueFamilyIndex = queueFamilyIndex,
            .buffer = srcDeviceDataBuffer,
            .offset = 0,
            .size = dataBufferSize
        },
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = NULL,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .srcQueueFamilyIndex = queueFamilyIndex,
            .dstQueueFamilyIndex = queueFamilyIndex,
            .buffer = srcDeviceAtomicBuffer,
            .offset = 0,
            .size = sizeof(unsigned)
        }
    };
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, NULL, sizeof(bufferBarriers) / sizeof(bufferBarriers[0]), bufferBarriers, 0, NULL);

    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = dataBufferSize
    };
    vkCmdCopyBuffer(commandBuffer, srcDeviceDataBuffer, dstHostBuffer, 1, &copyRegion);

    copyRegion.dstOffset = dataBufferSize + ADDITIONAL_ADDRESS_BUFFER_SIZE;
    copyRegion.size = sizeof(unsigned);
    vkCmdCopyBuffer(commandBuffer, srcDeviceAtomicBuffer, dstHostBuffer, 1, &copyRegion);

    const VkBufferMemoryBarrier bufferBarrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .dstAccessMask = VK_ACCESS_NONE,
        .srcQueueFamilyIndex = queueFamilyIndex,
        .dstQueueFamilyIndex = queueFamilyIndex,
        .buffer = dstHostBuffer,
        .offset = 0,
        .size = dataBufferSize + sizeof(unsigned)
    };

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
        0, NULL, 1U, &bufferBarrier, 0, NULL);
}

static VkResult CreateComputePipeline(VkDevice device, VkShaderModule computeShaderModule, VkPipeline* pComputePipeline,
    VkPipelineLayout* pPipelineLayout, VkDescriptorSetLayout* pDescLayout, uint32_t maxWorkgroupSize, uint32_t elemCount)
{
    const VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0}
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

    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = pDescLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
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
        uint32_t total_data_elem_count;
    } specConsts = { maxWorkgroupSize, 1U, 1U, elemCount };

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
            .offset = (uint32_t)offsetof(struct SpecializationConstants, total_data_elem_count),
            .size = sizeof(specConsts.total_data_elem_count)
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

    const VkComputePipelineCreateInfo computePipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .stage = shaderStageCreateInfo,
        .layout = *pPipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0
    };
    res = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, NULL, pComputePipeline);
    if (res != VK_SUCCESS) {
        fprintf(stderr, "vkCreateComputePipelines failed: %d\n", res);
    }

    return res;
}

static VkResult CreateDescriptorSets(VkDevice device, VkBuffer srcAddressDeviceBuffer, VkDeviceSize bufferSize, VkDescriptorSetLayout descLayout,
    VkDescriptorPool* pDescriptorPool, VkDescriptorSet* pDescSets)
{
    const VkDescriptorPoolCreateInfo descriptorPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .maxSets = 2,
        .poolSizeCount = 1,
        .pPoolSizes = (VkDescriptorPoolSize[]) {
            // Just one binding buffer, so there's only one descriptor.
            {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1}
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

    const VkDescriptorBufferInfo srcAddrDescBufferInfo = {
        .buffer = srcAddressDeviceBuffer,
        .offset = 0,
        .range = bufferSize
    };

    const VkWriteDescriptorSet writeDescSet[] = {
        // srcAddressDeviceBuffer
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = NULL,
            .dstSet = *pDescSets,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = NULL,
            .pBufferInfo = (VkDescriptorBufferInfo[]) { srcAddrDescBufferInfo },
            .pTexelBufferView = NULL
        }
    };

    vkUpdateDescriptorSets(device, (uint32_t)(sizeof(writeDescSet) / sizeof(writeDescSet[0])), writeDescSet, 0, NULL);

    return res;
}

extern VkResult CreateShaderModule(VkDevice device, const char* fileName, VkShaderModule* pShaderModule);

extern VkResult InitializeCommandBuffer(uint32_t queueFamilyIndex, VkDevice device, VkCommandPool* pCommandPool,
    VkCommandBuffer commandBuffers[], uint32_t commandBufferCount);

void RunAdvancedComputeTest(VkDevice specDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties, uint32_t specQueueFamilyIndex, unsigned maxWorkgroupSize)
{
    puts("\n================ Begin Advanced Compute Test ================\n");

    VkDeviceMemory deviceMemories[3] = { VK_NULL_HANDLE };
    // deviceBuffers[0] as host temporal buffer, deviceBuffers[1] as device dst buffer, deviceBuffers[2] as device src buffer
    // deviceBuffers[3] as device atomic buffer, deviceBuffers[4] as device address buffer
    VkBuffer deviceBuffers[5] = { VK_NULL_HANDLE };
    VkShaderModule computeShaderModule = VK_NULL_HANDLE;
    VkPipeline computePipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffers[1] = { VK_NULL_HANDLE };
    VkFence fence = VK_NULL_HANDLE;
    VkEvent event = VK_NULL_HANDLE;
    uint32_t const commandBufferCount = (uint32_t)(sizeof(commandBuffers) / sizeof(commandBuffers[0]));

    do
    {
        const uint32_t elemCount = 64 * 1024;
        const VkDeviceSize bufferSize = elemCount * sizeof(int);

        VkResult result = AllocateMemoryAndBuffers(specDevice, pMemoryProperties, deviceMemories, deviceBuffers, bufferSize, specQueueFamilyIndex);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "AllocateMemoryAndBuffers failed!\n");
            break;
        }

        result = CreateShaderModule(specDevice, "shaders/advanced.spv", &computeShaderModule);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "CreateShaderModule failed!\n");
            break;
        }

        result = CreateComputePipeline(specDevice, computeShaderModule, &computePipeline, &pipelineLayout, &descriptorSetLayout,
            maxWorkgroupSize, elemCount);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "CreateComputePipeline failed!\n");
            break;
        }

        // There's no need to destroy `descriptorSet`, since VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT flag is not set
        // in `flags` in `VkDescriptorPoolCreateInfo`
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        result = CreateDescriptorSets(specDevice, deviceBuffers[4], ADDITIONAL_ADDRESS_BUFFER_SIZE,
            descriptorSetLayout, &descriptorPool, &descriptorSet);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "CreateDescriptorSets failed!\n");
            break;
        }

        result = InitializeCommandBuffer(specQueueFamilyIndex, specDevice, &commandPool, commandBuffers, commandBufferCount);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "InitializeCommandBuffer failed!\n");
            break;
        }

        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(specDevice, specQueueFamilyIndex, 0, &queue);

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

        WriteBufferAndSync(commandBuffers[0], specQueueFamilyIndex, deviceBuffers[2], deviceBuffers[4],
            deviceBuffers[3], deviceBuffers[0], bufferSize);

        vkCmdDispatch(commandBuffers[0], elemCount / maxWorkgroupSize, 1, 1);

        SyncAndReadBuffer(commandBuffers[0], specQueueFamilyIndex, deviceBuffers[0], deviceBuffers[1], deviceBuffers[3], bufferSize);

        const VkEventCreateInfo eventCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
            .pNext = NULL,
            .flags = 0
        };
        result = vkCreateEvent(specDevice, &eventCreateInfo, NULL, &event);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "vkCreateEvent failed: %d\n", result);
            break;
        }

        vkCmdSetEvent(commandBuffers[0], event, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

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
        result = vkCreateFence(specDevice, &fenceCreateInfo, NULL, &fence);
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

        while (vkGetEventStatus(specDevice, event) != VK_EVENT_SET) {
            Sleep(1);
        }

        // Verify the result
        void* hostBuffer = NULL;
        result = vkMapMemory(specDevice, deviceMemories[0], 0, bufferSize, 0, &hostBuffer);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "vkMapMemory failed: %d\n", result);
            break;
        }

        int sumAtomicValue = 0;
        for (int i = 0; i < (int)elemCount; ++i) {
            sumAtomicValue += i;
        }

        unsigned* dstMem = hostBuffer;
        // Verify low 16-bit values
        for (unsigned i = 0; i < elemCount; i++)
        {
            if ((dstMem[i] & 0xffffU) != i)
            {
                fprintf(stderr, "Low 16-bit result error @ %d, result is: %d\n", i, dstMem[i] & 0xffffU);
                break;
            }
        }
        // Verify high 16-bit values
        int dstAtomicSum = 0;
        for (int i = 0; i < (int)elemCount; ++i) {
            dstAtomicSum += dstMem[i] >> 16;
        }
        if (dstAtomicSum != sumAtomicValue) {
            fprintf(stderr, "High 16-bit result is not correct: %u\n", dstAtomicSum);
        }

        dstMem = (unsigned*)((uint8_t*)hostBuffer + bufferSize + ADDITIONAL_ADDRESS_BUFFER_SIZE);
        const unsigned atomValue = dstMem[0];
        if (dstMem[0] != elemCount) {
            fprintf(stderr, "The atomic value is not correct!\n");
        }

        printf("The final atomic value is: %u\n", atomValue);

        vkUnmapMemory(specDevice, deviceMemories[0]);

        result = vkWaitForFences(specDevice, 1, &fence, VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "vkWaitForFences failed: %d\n", result);
            break;
        }
    }
    while (false);

    if (event != VK_NULL_HANDLE) {
        vkDestroyEvent(specDevice, event, NULL);
    }
    if (fence != VK_NULL_HANDLE) {
        vkDestroyFence(specDevice, fence, NULL);
    }
    if (commandPool != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(specDevice, commandPool, commandBufferCount, commandBuffers);
        vkDestroyCommandPool(specDevice, commandPool, NULL);
    }
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(specDevice, descriptorPool, NULL);
    }
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(specDevice, descriptorSetLayout, NULL);
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(specDevice, pipelineLayout, NULL);
    }
    if (computePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(specDevice, computePipeline, NULL);
    }
    if (computeShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(specDevice, computeShaderModule, NULL);
    }

    for (size_t i = 0; i < sizeof(deviceBuffers) / sizeof(deviceBuffers[0]); i++)
    {
        if (deviceBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(specDevice, deviceBuffers[i], NULL);
        }
    }
    for (size_t i = 0; i < sizeof(deviceMemories) / sizeof(deviceMemories[0]); i++)
    {
        if (deviceMemories[i] != VK_NULL_HANDLE) {
            vkFreeMemory(specDevice, deviceMemories[i], NULL);
        }
    }

    puts("\n================ Complete Advanced Compute Test ================\n");
}

