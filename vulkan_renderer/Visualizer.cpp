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

// Estrutura interna para não vazar dependências Vulkan para a main
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
    
    // Sincronização
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    // Vertex Buffer dinâmico (alocado no init com tamanho fixo grande, ex: 10MB)
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
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

// 1. Criação única do Render Pass, Framebuffers e Layout
void Visualizer::init_render_resources() {
    // Render Pass
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

    // Framebuffers
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

    // Pipeline Layout
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

// 2. Função auxiliar limpa apenas para o Pipeline
VkPipeline create_pipeline(VkPrimitiveTopology topology, VulkanState *vkState) {
    auto vertShaderCode = read_file("vulkan_renderer/shaders/vert.spv");
    auto fragShaderCode = read_file("vulkan_renderer/shaders/frag.spv");

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

    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

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
    colorBlendAttachment.blendEnable = VK_FALSE;

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
    pipelineInfo.layout = vkState->pipelineLayout;
    pipelineInfo.renderPass = vkState->renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline newPipeline;
    vkCreateGraphicsPipelines(vkState->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline);

    vkDestroyShaderModule(vkState->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkState->device, vertShaderModule, nullptr);
    return newPipeline;
}

void Visualizer::init_pipelines() {
    init_render_resources(); // Cria render pass, framebuffers e layout uma única vez
    
    vkState->graphicsPipelineTriangles = create_pipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vkState);
    vkState->graphicsPipelineLines     = create_pipeline(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, vkState);
    vkState->graphicsPipelinePoints    = create_pipeline(VK_PRIMITIVE_TOPOLOGY_POINT_LIST, vkState);
}

void Visualizer::init(int width, int height) {
    vkState = new VulkanState();
    
    // 1. Inicializar GLFW
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    vkState->window = glfwCreateWindow(width, height, "GeomVulkan", nullptr, nullptr);

    // 2. Criar Instance (vk-bootstrap)
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("GeomVulkan")
        .request_validation_layers(true)
        .use_default_debug_messenger()
        .build();
    vkb::Instance vkb_inst = inst_ret.value();
    vkState->instance = vkb_inst.instance;
    vkState->debug_messenger = vkb_inst.debug_messenger;

    // 3. Criar Surface
    glfwCreateWindowSurface(vkState->instance, vkState->window, nullptr, &vkState->surface);

    // 4. Selecionar GPU Física
    vkb::PhysicalDeviceSelector selector{vkb_inst};
    auto phys_ret = selector.set_surface(vkState->surface)
        .set_minimum_version(1, 3)
        .prefer_gpu_device_type(vkb::PreferredDeviceType::integrated) // Força a Intel iGPU
        .select();
    
    vkb::PhysicalDevice vkb_phys_dev = phys_ret.value();
    vkState->physicalDevice = vkb_phys_dev.physical_device;

    // Imprime a GPU para confirmar
    std::cout << ">> GPU Selecionada: " << vkb_phys_dev.name << std::endl;

    // 5. Criar Logical Device
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extDynamicState{};
    extDynamicState.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extDynamicState.extendedDynamicState = VK_TRUE;

    VkPhysicalDeviceFeatures features{};
    features.largePoints = VK_TRUE;

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

    // 6. Criar Swapchain
    vkb::SwapchainBuilder swapchain_builder{vkb_device};
    auto swap_ret = swapchain_builder.build();
    vkb::Swapchain vkb_swapchain = swap_ret.value();
    vkState->swapChain = vkb_swapchain.swapchain;
    vkState->swapChainImages = vkb_swapchain.get_images().value();
    vkState->swapChainImageViews = vkb_swapchain.get_image_views().value();
    vkState->swapChainImageFormat = vkb_swapchain.image_format;
    vkState->swapChainExtent = vkb_swapchain.extent;

    // 7. Sincronização (Resolve o Segfault)
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Crucial: Inicia sinalizada
    vkCreateFence(vkState->device, &fenceInfo, nullptr, &vkState->inFlightFence);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vkCreateSemaphore(vkState->device, &semaphoreInfo, nullptr, &vkState->imageAvailableSemaphore);
    vkCreateSemaphore(vkState->device, &semaphoreInfo, nullptr, &vkState->renderFinishedSemaphore);

    // 8. Command Pool e Command Buffer
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

    // 9. Alocar Vertex Buffer de 10MB
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = 10 * 1024 * 1024; // 10 MB
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(vkState->device, &bufferInfo, nullptr, &vkState->vertexBuffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkState->device, vkState->vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocMemInfo{};
    allocMemInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocMemInfo.allocationSize = memRequirements.size;

    // Encontrar tipo de memória visível para a CPU (Host Visible | Host Coherent)
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
    if (!vkState) {
        return;
    }

    if (vkState->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(vkState->device);
    }

    // 1. Framebuffers
    for (auto framebuffer : vkState->swapChainFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(vkState->device, framebuffer, nullptr);
        }
    }
    vkState->swapChainFramebuffers.clear();

    // 2. Pipelines
    if (vkState->graphicsPipelineTriangles != VK_NULL_HANDLE) {
        vkDestroyPipeline(vkState->device, vkState->graphicsPipelineTriangles, nullptr);
        vkState->graphicsPipelineTriangles = VK_NULL_HANDLE;
    }
    if (vkState->graphicsPipelineLines != VK_NULL_HANDLE) {
        vkDestroyPipeline(vkState->device, vkState->graphicsPipelineLines, nullptr);
        vkState->graphicsPipelineLines = VK_NULL_HANDLE;
    }
    if (vkState->graphicsPipelinePoints != VK_NULL_HANDLE) {
        vkDestroyPipeline(vkState->device, vkState->graphicsPipelinePoints, nullptr);
        vkState->graphicsPipelinePoints = VK_NULL_HANDLE;
    }

    // 3. Pipeline Layout
    if (vkState->pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vkState->device, vkState->pipelineLayout, nullptr);
        vkState->pipelineLayout = VK_NULL_HANDLE;
    }

    // 4. Render Pass
    if (vkState->renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(vkState->device, vkState->renderPass, nullptr);
        vkState->renderPass = VK_NULL_HANDLE;
    }

    // 5. Vertex Buffers e Memória (com proteção contra double-free / lixo)
    if (vkState && vkState->device != VK_NULL_HANDLE) {
        if (vkState->vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(vkState->device, vkState->vertexBuffer, nullptr);
            vkState->vertexBuffer = VK_NULL_HANDLE;
        }
        if (vkState->vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(vkState->device, vkState->vertexBufferMemory, nullptr);
            vkState->vertexBufferMemory = VK_NULL_HANDLE;
        }
    }

    // 6. Destruir Sincronizações e Command Pool
    if (vkState && vkState->device != VK_NULL_HANDLE) {
        if (vkState->imageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(vkState->device, vkState->imageAvailableSemaphore, nullptr);
            vkState->imageAvailableSemaphore = VK_NULL_HANDLE;
        }
        if (vkState->renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(vkState->device, vkState->renderFinishedSemaphore, nullptr);
            vkState->renderFinishedSemaphore = VK_NULL_HANDLE;
        }
        if (vkState->inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(vkState->device, vkState->inFlightFence, nullptr);
            vkState->inFlightFence = VK_NULL_HANDLE;
        }
        if (vkState->commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(vkState->device, vkState->commandPool, nullptr);
            vkState->commandPool = VK_NULL_HANDLE;
        }
    }

    // 7. Destruir Image Views e Swapchain
    if (vkState && vkState->device != VK_NULL_HANDLE) {
        for (auto imageView : vkState->swapChainImageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(vkState->device, imageView, nullptr);
            }
        }
        vkState->swapChainImageViews.clear();

        if (vkState->swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(vkState->device, vkState->swapChain, nullptr);
            vkState->swapChain = VK_NULL_HANDLE;
        }
    }

    // 8. Destruir Dispositivo Lógico
    if (vkState && vkState->device != VK_NULL_HANDLE) {
        vkDestroyDevice(vkState->device, nullptr);
        vkState->device = VK_NULL_HANDLE;
    }

    // 9. Debug Messenger, Superfície e Instância
    if (vkState->instance != VK_NULL_HANDLE) {
        auto destroyFn = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkState->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyFn && vkState->debug_messenger != VK_NULL_HANDLE) {
            destroyFn(vkState->instance, vkState->debug_messenger, nullptr);
            vkState->debug_messenger = VK_NULL_HANDLE;
        }

        if (vkState->surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(vkState->instance, vkState->surface, nullptr);
            vkState->surface = VK_NULL_HANDLE;
        }

        vkDestroyInstance(vkState->instance, nullptr);
        vkState->instance = VK_NULL_HANDLE;
    }

    // 10. Janela e GLFW
    if (vkState->window != nullptr) {
        glfwDestroyWindow(vkState->window);
        vkState->window = nullptr;
    }
    glfwTerminate();

    // 11. Deletar estrutura de estado
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
}

void Visualizer::render_frame(glm::mat4 proj) {
    // 1. Aguardar a GPU terminar o frame anterior
    vkWaitForFences(vkState->device, 1, &vkState->inFlightFence, VK_TRUE, UINT64_MAX);

    // 2. Adquirir a próxima imagem da Swapchain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vkState->device, vkState->swapChain, UINT64_MAX, vkState->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        std::cout << "ALERTA: Janela desatualizada (Wayland/Resize). Frame pulado." << std::endl;
        return; 
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cout << "ERRO: vkAcquireNextImageKHR falhou." << std::endl;
        return;
    }

    vkResetFences(vkState->device, 1, &vkState->inFlightFence);
    vkResetCommandBuffer(vkState->commandBuffer, 0);

    // 3. Consolidar todos os vértices na CPU
    std::vector<Vertex> all_vertices;
    all_vertices.insert(all_vertices.end(), triangles.begin(), triangles.end());
    size_t lines_offset = all_vertices.size();
    
    all_vertices.insert(all_vertices.end(), lines.begin(), lines.end());
    size_t points_offset = all_vertices.size();
    
    all_vertices.insert(all_vertices.end(), points.begin(), points.end());

    VkDeviceSize bufferSize = sizeof(Vertex) * all_vertices.size();

    // 4. Copiar para a memória da GPU
    if (bufferSize > 0){
    void* data;
        // Usa VK_WHOLE_SIZE para evitar violação de alinhamento na Intel
        vkMapMemory(vkState->device, vkState->vertexBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);
        memcpy(data, all_vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(vkState->device, vkState->vertexBufferMemory);
    }

    // 5. Iniciar gravação de comandos
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(vkState->commandBuffer, &beginInfo);

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

    // vkCmdBindPipeline(vkState->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkState->graphicsPipeline);
    // vkCmdBindPipeline(vkState->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkState->graphicsPipeline);

    // 6. Viewport dinâmico (necessário se VK_DYNAMIC_STATE_VIEWPORT foi ativado)
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

    // 7. Bind Vertex Buffer e Push Constants
    VkBuffer vertexBuffers[] = {vkState->vertexBuffer};
    VkDeviceSize offsets[] = {0};
    
    // CORREÇÃO CRUCIAL PARA INTEL: Garanta que o binding aponta para o slot 0 com offset 0 exato
    vkCmdBindVertexBuffers(vkState->commandBuffer, 0, 1, vertexBuffers, offsets);

    vkCmdPushConstants(vkState->commandBuffer, vkState->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &proj[0][0]);

    // 8. Calls de desenho dinâmico alterando a topologia
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

    vkCmdEndRenderPass(vkState->commandBuffer);
    vkEndCommandBuffer(vkState->commandBuffer);

    // 9. Submeter para a Fila (Queue)
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

    // 10. Apresentar na tela
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
