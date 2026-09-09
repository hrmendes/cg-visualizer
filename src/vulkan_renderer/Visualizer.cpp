#include "Visualizer.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <fstream>
#include <array>
#include "VkBootstrap.h"
#include <iostream>

#include "../geom/triangulation.hpp"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../utils/stb_truetype.h"

struct VulkanState {
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger; 
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipelineLines, graphicsPipelineTriangles, graphicsPipelinePoints;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    // Recursos de Texto e UTF-8
    VkImage fontImage;
    VkDeviceMemory fontMemory;
    VkImageView fontImageView;
    VkSampler fontSampler;
    VkPipeline graphicsPipelineText;
    VkPipelineLayout textPipelineLayout;
    VkDescriptorSetLayout textDescriptorSetLayout;
    VkDescriptorPool textDescriptorPool;
    VkDescriptorSet textDescriptorSet;

    // Buffers Persistentes para atualização do Atlas na GPU
    VkBuffer fontStagingBuffer;
    VkDeviceMemory fontStagingMemory;
    void* fontStagingMapped;
    bool fontAtlasDirty;

    stbtt_fontinfo fontInfo;
    std::vector<unsigned char> fontBufferStorage;
    std::vector<unsigned char> atlasBitmap;
    stbtt_pack_context spc;
    stbtt_packedchar packedChars[65536];
    bool charBaked[65536];
};

bool Visualizer::is_key_pressed(int key){
    return (glfwGetKey(vkState->window, key) == GLFW_PRESS);
}

static std::vector<char> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Falha ao abrir " + filename);
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

static VkShaderModule create_shader_module(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    return shaderModule;
}

void Visualizer::init_render_resources() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vkState->swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    vkCreateRenderPass(vkState->device, &renderPassInfo, nullptr, &vkState->renderPass);

    vkState->swapChainFramebuffers.resize(vkState->swapChainImageViews.size());
    for (size_t i = 0; i < vkState->swapChainImageViews.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vkState->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vkState->swapChainImageViews[i];
        framebufferInfo.width = vkState->swapChainExtent.width;
        framebufferInfo.height = vkState->swapChainExtent.height;
        framebufferInfo.layers = 1;
        vkCreateFramebuffer(vkState->device, &framebufferInfo, nullptr, &vkState->swapChainFramebuffers[i]);
    }

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    vkCreatePipelineLayout(vkState->device, &pipelineLayoutInfo, nullptr, &vkState->pipelineLayout);
}

VkPipeline create_pipeline(VkPrimitiveTopology topology, VulkanState *vkState, const std::string& vertPath, const std::string& fragPath, VkPipelineLayout layout) {
    auto vertShaderCode = read_file(vertPath);
    auto fragShaderCode = read_file(fragPath);

    VkShaderModule vertShaderModule = create_shader_module(vkState->device, vertShaderCode);
    VkShaderModule fragShaderModule = create_shader_module(vkState->device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, uv);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology; 
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = vkState->renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline newPipeline;
    vkCreateGraphicsPipelines(vkState->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline);

    vkDestroyShaderModule(vkState->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkState->device, vertShaderModule, nullptr);
    return newPipeline;
}

void Visualizer::init_pipelines() {
    init_render_resources();
    
    std::string vert = "src/vulkan_renderer/shaders/vert.spv";
    std::string frag = "src/vulkan_renderer/shaders/frag.spv";

    vkState->graphicsPipelineTriangles = create_pipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vkState, vert, frag, vkState->pipelineLayout);
    vkState->graphicsPipelineLines     = create_pipeline(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, vkState, vert, frag, vkState->pipelineLayout);
    vkState->graphicsPipelinePoints    = create_pipeline(VK_PRIMITIVE_TOPOLOGY_POINT_LIST, vkState, vert, frag, vkState->pipelineLayout);
    
    init_text_pipeline();
}

void Visualizer::init_text_pipeline() {
    int bitmap_w = 1024;
    int bitmap_h = 1024;
    vkState->atlasBitmap.resize(bitmap_w * bitmap_h, 0);
    std::memset(vkState->charBaked, 0, sizeof(vkState->charBaked));
    vkState->fontAtlasDirty = false;

    std::string font_path = "assets/fonts/JetBrainsMono.ttf";
    std::ifstream file(font_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "AVISO: Nao foi possivel abrir a fonte em " << font_path << std::endl;
        return;
    }
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    vkState->fontBufferStorage.resize(file_size);
    file.read((char*)vkState->fontBufferStorage.data(), file_size);

    // Inicializa o packer
    if (!stbtt_PackBegin(&vkState->spc, vkState->atlasBitmap.data(), bitmap_w, bitmap_h, 0, 1, nullptr)) {
        std::cerr << "ERRO: Falha ao iniciar stbtt_PackBegin" << std::endl;
        return;
    }
    stbtt_PackSetOversampling(&vkState->spc, 2, 2);

    // Pré-carrega ASCII
    stbtt_pack_range range;
    range.font_size = 32.0f;
    range.first_unicode_codepoint_in_range = 32;
    range.array_of_unicode_codepoints = nullptr;
    range.num_chars = 95;
    range.chardata_for_range = &vkState->packedChars[32];
    stbtt_PackFontRanges(&vkState->spc, vkState->fontBufferStorage.data(), 0, &range, 1);
    
    for (int i = 32; i < 127; i++) {
        vkState->charBaked[i] = true;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = bitmap_w;
    imageInfo.extent.height = bitmap_h;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateImage(vkState->device, &imageInfo, nullptr, &vkState->fontImage);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vkState->device, vkState->fontImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkState->physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }
    vkAllocateMemory(vkState->device, &allocInfo, nullptr, &vkState->fontMemory);
    vkBindImageMemory(vkState->device, vkState->fontImage, vkState->fontMemory, 0);

    // Staging buffer PERSISTENTE para atualizações dinâmicas da textura
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bitmap_w * bitmap_h;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(vkState->device, &bufferInfo, nullptr, &vkState->fontStagingBuffer);

    vkGetBufferMemoryRequirements(vkState->device, vkState->fontStagingBuffer, &memRequirements);
    allocInfo.allocationSize = memRequirements.size;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }
    vkAllocateMemory(vkState->device, &allocInfo, nullptr, &vkState->fontStagingMemory);
    vkBindBufferMemory(vkState->device, vkState->fontStagingBuffer, vkState->fontStagingMemory, 0);
    
    // Mapeia permanentemente para evitar overhead de mapeamento em tempo real
    vkMapMemory(vkState->device, vkState->fontStagingMemory, 0, VK_WHOLE_SIZE, 0, &vkState->fontStagingMapped);
    memcpy(vkState->fontStagingMapped, vkState->atlasBitmap.data(), bitmap_w * bitmap_h);

    // Carga inicial
    VkCommandBufferAllocateInfo allocInfoCmd{};
    allocInfoCmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfoCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfoCmd.commandPool = vkState->commandPool;
    allocInfoCmd.commandBufferCount = 1;

    VkCommandBuffer tempCmdBuffer;
    vkAllocateCommandBuffers(vkState->device, &allocInfoCmd, &tempCmdBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(tempCmdBuffer, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vkState->fontImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(tempCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { (uint32_t)bitmap_w, (uint32_t)bitmap_h, 1 };
    vkCmdCopyBufferToImage(tempCmdBuffer, vkState->fontStagingBuffer, vkState->fontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(tempCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkEndCommandBuffer(tempCmdBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &tempCmdBuffer;

    vkQueueSubmit(vkState->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkState->graphicsQueue);
    vkFreeCommandBuffers(vkState->device, vkState->commandPool, 1, &tempCmdBuffer);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vkState->fontImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(vkState->device, &viewInfo, nullptr, &vkState->fontImageView);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    vkCreateSampler(vkState->device, &samplerInfo, nullptr, &vkState->fontSampler);

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;
    vkCreateDescriptorSetLayout(vkState->device, &layoutInfo, nullptr, &vkState->textDescriptorSetLayout);

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    vkCreateDescriptorPool(vkState->device, &poolInfo, nullptr, &vkState->textDescriptorPool);

    VkDescriptorSetAllocateInfo allocSetInfo{};
    allocSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocSetInfo.descriptorPool = vkState->textDescriptorPool;
    allocSetInfo.descriptorSetCount = 1;
    allocSetInfo.pSetLayouts = &vkState->textDescriptorSetLayout;
    vkAllocateDescriptorSets(vkState->device, &allocSetInfo, &vkState->textDescriptorSet);

    VkDescriptorImageInfo imageDescInfo{};
    imageDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageDescInfo.imageView = vkState->fontImageView;
    imageDescInfo.sampler = vkState->fontSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = vkState->textDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageDescInfo;
    vkUpdateDescriptorSets(vkState->device, 1, &descriptorWrite, 0, nullptr);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vkState->textDescriptorSetLayout;
    
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    vkCreatePipelineLayout(vkState->device, &pipelineLayoutInfo, nullptr, &vkState->textPipelineLayout);

    std::string vert = "src/vulkan_renderer/shaders/vert.spv";
    std::string text_frag = "src/vulkan_renderer/shaders/text_frag.spv";
    vkState->graphicsPipelineText = create_pipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vkState, vert, text_frag, vkState->textPipelineLayout);
}

void Visualizer::init(int width, int height) {
    vkState = new VulkanState();
    
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    vkState->window = glfwCreateWindow(width, height, "GeomVulkan", nullptr, nullptr);

    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("GeomVulkan")
        .request_validation_layers(true)
        .use_default_debug_messenger()
        .build();
    vkb::Instance vkb_inst = inst_ret.value();
    vkState->instance = vkb_inst.instance;
    vkState->debug_messenger = vkb_inst.debug_messenger;

    glfwCreateWindowSurface(vkState->instance, vkState->window, nullptr, &vkState->surface);

    vkb::PhysicalDeviceSelector selector{vkb_inst};
    auto phys_ret = selector.set_surface(vkState->surface)
        .set_minimum_version(1, 3)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::integrated)
        .select();
    
    vkb::PhysicalDevice vkb_phys_dev = phys_ret.value();
    vkState->physicalDevice = vkb_phys_dev.physical_device;

    std::cout << ">> GPU Selecionada: " << vkb_phys_dev.name << std::endl;

    vkb::DeviceBuilder dev_builder{vkb_phys_dev};
    auto dev_ret = dev_builder.build();
    
    if (!dev_ret) {
        std::cerr << "Falha ao criar o dispositivo lógico: " << dev_ret.error().message() << std::endl;
        return;
    }

    vkb::Device vkb_device = dev_ret.value();
    vkState->device = vkb_device.device;

    vkState->graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    vkState->presentQueue = vkb_device.get_queue(vkb::QueueType::present).value();

    vkb::SwapchainBuilder swapchain_builder{vkb_device};
    auto swap_ret = swapchain_builder.build();
    vkb::Swapchain vkb_swapchain = swap_ret.value();
    vkState->swapChain = vkb_swapchain.swapchain;
    vkState->swapChainImages = vkb_swapchain.get_images().value();
    vkState->swapChainImageViews = vkb_swapchain.get_image_views().value();
    vkState->swapChainImageFormat = vkb_swapchain.image_format;
    vkState->swapChainExtent = vkb_swapchain.extent;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 
    vkCreateFence(vkState->device, &fenceInfo, nullptr, &vkState->inFlightFence);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(vkState->device, &semaphoreInfo, nullptr, &vkState->imageAvailableSemaphore);
    vkCreateSemaphore(vkState->device, &semaphoreInfo, nullptr, &vkState->renderFinishedSemaphore);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    vkCreateCommandPool(vkState->device, &poolInfo, nullptr, &vkState->commandPool);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vkState->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(vkState->device, &allocInfo, &vkState->commandBuffer);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = 10 * 1024 * 1024;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(vkState->device, &bufferInfo, nullptr, &vkState->vertexBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkState->device, vkState->vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocMemInfo{};
    allocMemInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocMemInfo.allocationSize = memRequirements.size;

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkState->physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            allocMemInfo.memoryTypeIndex = i;
            break;
        }
    }
    
    vkAllocateMemory(vkState->device, &allocMemInfo, nullptr, &vkState->vertexBufferMemory);
    vkBindBufferMemory(vkState->device, vkState->vertexBuffer, vkState->vertexBufferMemory, 0);

    init_pipelines();
}

void Visualizer::cleanup() {
    if (!vkState) return;

    // Resolve o memory leak principal da árvore do packer
    stbtt_PackEnd(&vkState->spc);

    if (vkState->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(vkState->device);

        // Desaloca staging buffer do font atlas
        if (vkState->fontStagingBuffer != VK_NULL_HANDLE) {
            vkUnmapMemory(vkState->device, vkState->fontStagingMemory);
            vkDestroyBuffer(vkState->device, vkState->fontStagingBuffer, nullptr);
            vkFreeMemory(vkState->device, vkState->fontStagingMemory, nullptr);
        }

        if (vkState->graphicsPipelineText != VK_NULL_HANDLE) vkDestroyPipeline(vkState->device, vkState->graphicsPipelineText, nullptr);
        if (vkState->textPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(vkState->device, vkState->textPipelineLayout, nullptr);
        if (vkState->textDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(vkState->device, vkState->textDescriptorSetLayout, nullptr);
        if (vkState->textDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(vkState->device, vkState->textDescriptorPool, nullptr);
        
        if (vkState->fontSampler != VK_NULL_HANDLE) vkDestroySampler(vkState->device, vkState->fontSampler, nullptr);
        if (vkState->fontImageView != VK_NULL_HANDLE) vkDestroyImageView(vkState->device, vkState->fontImageView, nullptr);
        if (vkState->fontImage != VK_NULL_HANDLE) vkDestroyImage(vkState->device, vkState->fontImage, nullptr);
        if (vkState->fontMemory != VK_NULL_HANDLE) vkFreeMemory(vkState->device, vkState->fontMemory, nullptr);
    }

    for (auto framebuffer : vkState->swapChainFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(vkState->device, framebuffer, nullptr);
    }
    vkState->swapChainFramebuffers.clear();

    if (vkState->graphicsPipelineTriangles != VK_NULL_HANDLE) vkDestroyPipeline(vkState->device, vkState->graphicsPipelineTriangles, nullptr);
    if (vkState->graphicsPipelineLines != VK_NULL_HANDLE) vkDestroyPipeline(vkState->device, vkState->graphicsPipelineLines, nullptr);
    if (vkState->graphicsPipelinePoints != VK_NULL_HANDLE) vkDestroyPipeline(vkState->device, vkState->graphicsPipelinePoints, nullptr);
    if (vkState->pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(vkState->device, vkState->pipelineLayout, nullptr);
    if (vkState->renderPass != VK_NULL_HANDLE) vkDestroyRenderPass(vkState->device, vkState->renderPass, nullptr);

    if (vkState && vkState->device != VK_NULL_HANDLE) {
        if (vkState->vertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(vkState->device, vkState->vertexBuffer, nullptr);
        if (vkState->vertexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(vkState->device, vkState->vertexBufferMemory, nullptr);
    }

    if (vkState && vkState->device != VK_NULL_HANDLE) {
        if (vkState->imageAvailableSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(vkState->device, vkState->imageAvailableSemaphore, nullptr);
        if (vkState->renderFinishedSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(vkState->device, vkState->renderFinishedSemaphore, nullptr);
        if (vkState->inFlightFence != VK_NULL_HANDLE) vkDestroyFence(vkState->device, vkState->inFlightFence, nullptr);
        if (vkState->commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(vkState->device, vkState->commandPool, nullptr);
    }

    if (vkState && vkState->device != VK_NULL_HANDLE) {
        for (auto imageView : vkState->swapChainImageViews) {
            if (imageView != VK_NULL_HANDLE) vkDestroyImageView(vkState->device, imageView, nullptr);
        }
        vkState->swapChainImageViews.clear();
        if (vkState->swapChain != VK_NULL_HANDLE) vkDestroySwapchainKHR(vkState->device, vkState->swapChain, nullptr);
    }

    if (vkState && vkState->device != VK_NULL_HANDLE) {
        vkDestroyDevice(vkState->device, nullptr);
        vkState->device = VK_NULL_HANDLE;
    }

    if (vkState->instance != VK_NULL_HANDLE) {
        auto destroyFn = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkState->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyFn && vkState->debug_messenger != VK_NULL_HANDLE) destroyFn(vkState->instance, vkState->debug_messenger, nullptr);
        if (vkState->surface != VK_NULL_HANDLE) vkDestroySurfaceKHR(vkState->instance, vkState->surface, nullptr);
        vkDestroyInstance(vkState->instance, nullptr);
    }

    if (vkState->window != nullptr) glfwDestroyWindow(vkState->window);
    glfwTerminate();

    delete vkState;
    vkState = nullptr;
}

bool Visualizer::is_running() {
    glfwPollEvents();
    return !glfwWindowShouldClose(vkState->window);
}

void Visualizer::clear_buffers() {
    points.clear();
    lines.clear();
    triangles.clear();
    text_vertices.clear();
}

void Visualizer::render_frame(glm::mat4 proj) {
    vkWaitForFences(vkState->device, 1, &vkState->inFlightFence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vkState->device, vkState->swapChain, UINT64_MAX, vkState->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return; 
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cout << "ERRO: vkAcquireNextImageKHR falhou." << std::endl;
        return;
    }

    vkResetFences(vkState->device, 1, &vkState->inFlightFence);
    vkResetCommandBuffer(vkState->commandBuffer, 0);

    std::vector<Vertex> all_vertices;
    all_vertices.insert(all_vertices.end(), triangles.begin(), triangles.end());

    size_t lines_offset = all_vertices.size();
    all_vertices.insert(all_vertices.end(), lines.begin(), lines.end());

    size_t points_offset = all_vertices.size();
    all_vertices.insert(all_vertices.end(), points.begin(), points.end());

    size_t text_offset = all_vertices.size();
    all_vertices.insert(all_vertices.end(), text_vertices.begin(), text_vertices.end());

    VkDeviceSize bufferSize = sizeof(Vertex) * all_vertices.size();

    if (bufferSize > 0){
        void* data;
        vkMapMemory(vkState->device, vkState->vertexBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);
        memcpy(data, all_vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkState->device, vkState->vertexBufferMemory);
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(vkState->commandBuffer, &beginInfo);

    // --- SICRONIZAÇÃO DINÂMICA CPU -> GPU PARA CARACTERES UTF-8 ---
    if (vkState->fontAtlasDirty) {
        memcpy(vkState->fontStagingMapped, vkState->atlasBitmap.data(), 1024 * 1024);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = vkState->fontImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(vkState->commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = { 1024, 1024, 1 };
        vkCmdCopyBufferToImage(vkState->commandBuffer, vkState->fontStagingBuffer, vkState->fontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(vkState->commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        vkState->fontAtlasDirty = false;
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vkState->renderPass;
    renderPassInfo.framebuffer = vkState->swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vkState->swapChainExtent;

    VkClearValue clearColor = {{{1.0f, 1.0f, 1.0f, 1.0f}}}; 
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(vkState->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vkState->swapChainExtent.width;
    viewport.height = (float)vkState->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vkState->commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vkState->swapChainExtent;
    vkCmdSetScissor(vkState->commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {vkState->vertexBuffer};
    VkDeviceSize offsets[] = {0};
    
    vkCmdBindVertexBuffers(vkState->commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdPushConstants(vkState->commandBuffer, vkState->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &proj[0][0]);

    if (!triangles.empty()) {
        vkCmdBindPipeline(vkState->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkState->graphicsPipelineTriangles);
        vkCmdDraw(vkState->commandBuffer, static_cast<uint32_t>(triangles.size()), 1, 0, 0);
    }

    if (!lines.empty()) {
        vkCmdBindPipeline(vkState->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkState->graphicsPipelineLines);
        vkCmdDraw(vkState->commandBuffer, static_cast<uint32_t>(lines.size()), 1, static_cast<uint32_t>(lines_offset), 0);
    }

    if (!points.empty()) {
        vkCmdBindPipeline(vkState->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkState->graphicsPipelinePoints);
        vkCmdDraw(vkState->commandBuffer, static_cast<uint32_t>(points.size()), 1, static_cast<uint32_t>(points_offset), 0);
    }

    if (!text_vertices.empty()) {
        vkCmdBindPipeline(vkState->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkState->graphicsPipelineText);
        vkCmdBindDescriptorSets(vkState->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkState->textPipelineLayout, 0, 1, &vkState->textDescriptorSet, 0, nullptr);
        vkCmdDraw(vkState->commandBuffer, static_cast<uint32_t>(text_vertices.size()), 1, static_cast<uint32_t>(text_offset), 0);
    }

    vkCmdEndRenderPass(vkState->commandBuffer);
    vkEndCommandBuffer(vkState->commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {vkState->imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkState->commandBuffer;

    VkSemaphore signalSemaphores[] = {vkState->renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(vkState->graphicsQueue, 1, &submitInfo, vkState->inFlightFence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {vkState->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(vkState->presentQueue, &presentInfo);

    glfwPollEvents();
}

void Visualizer::draw_point(pt p, glm::vec4 color) {
    points.push_back({glm::vec2((float)p.x, (float)p.y), color, glm::vec2(0.0f)});
}

void Visualizer::draw_line(pt a, pt b, glm::vec4 color) {
    lines.push_back({glm::vec2((float)a.x, (float)a.y), color, glm::vec2(0.0f)});
    lines.push_back({glm::vec2((float)b.x, (float)b.y), color, glm::vec2(0.0f)});
}

void Visualizer::draw_polygon(const std::vector<pt>& poly, glm::vec4 color) {
    if (poly.size() < 3) return;
    for (auto t : triangulate(poly)) {
        auto [p1,p2,p3] = t;
        triangles.push_back({glm::vec2(p1.x, p1.y), color, glm::vec2(0.0f)});
        triangles.push_back({glm::vec2(p2.x, p2.y), color, glm::vec2(0.0f)});
        triangles.push_back({glm::vec2(p3.x, p3.y), color, glm::vec2(0.0f)});
    }
}

void Visualizer::draw_text(const std::string& text, pt pos, float font_size, glm::vec4 color) {
    float scale = font_size / 32.0f;
    float cursor_x = 0.0f;
    float cursor_y = 0.0f;

    size_t i = 0;
    while (i < text.length()) {
        unsigned int codepoint = 0;
        unsigned char c = text[i];
        int bytes_to_read = 0;

        if (c < 0x80) {
            codepoint = c;
            bytes_to_read = 1;
        } else if ((c & 0xE0) == 0xC0) {
            codepoint = c & 0x1F;
            bytes_to_read = 2;
        } else if ((c & 0xF0) == 0xE0) {
            codepoint = c & 0x0F;
            bytes_to_read = 3;
        } else if ((c & 0xF8) == 0xF0) {
            codepoint = c & 0x07;
            bytes_to_read = 4;
        } else {
            i++;
            continue;
        }

        if (i + bytes_to_read > text.length()) break;

        for (int b = 1; b < bytes_to_read; b++) {
            codepoint = (codepoint << 6) | (text[i + b] & 0x3F);
        }
        i += bytes_to_read;

        // Limite de segurança para não explodir os arrays caso passem Emojis (codepoint > 65535)
        if (codepoint >= 65536) codepoint = '?'; 

        if (!vkState->charBaked[codepoint]) {
            stbtt_pack_range range;
            range.font_size = 32.0f;
            range.first_unicode_codepoint_in_range = codepoint;
            range.array_of_unicode_codepoints = nullptr;
            range.num_chars = 1;
            range.chardata_for_range = &vkState->packedChars[codepoint]; 

            stbtt_PackFontRanges(&vkState->spc, vkState->fontBufferStorage.data(), 0, &range, 1);
            vkState->charBaked[codepoint] = true;
            vkState->fontAtlasDirty = true; // Avisa a GPU para atualizar a textura antes de renderizar
        }

        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(vkState->packedChars, 1024, 1024, codepoint, &cursor_x, &cursor_y, &q, 1);

        float x0 = pos.x + q.x0 * scale;
        float y0 = pos.y - q.y0 * scale;
        float x1 = pos.x + q.x1 * scale;
        float y1 = pos.y - q.y1 * scale;

        text_vertices.push_back({{x0, y0}, color, {q.s0, q.t0}});
        text_vertices.push_back({{x0, y1}, color, {q.s0, q.t1}});
        text_vertices.push_back({{x1, y1}, color, {q.s1, q.t1}});

        text_vertices.push_back({{x0, y0}, color, {q.s0, q.t0}});
        text_vertices.push_back({{x1, y1}, color, {q.s1, q.t1}});
        text_vertices.push_back({{x1, y0}, color, {q.s1, q.t0}});
    }
}