#include "renderer.h"

#include "core.h"
#include "shaders/precompiled.h"

#include <stdio.h>
#include <stdlib.h>


static void CreateSampler(ERenderer renderer, EContext context);
static void CreateDescriptorSetLayout(ERenderer renderer, EContext context);
static void CreateDescriptorPool(ERenderer renderer, EContext context);
static void CreatePipelineLayout(ERenderer renderer, EContext context);
static void CreatePipeline(ERenderer renderer,
  EContext context,
  ERendererCreateInfo* infoIn);

E_EXTERN void eCreateRenderer(ERenderer* rendererOut,
  ERendererCreateInfo* infoIn) {
    if (!rendererOut) {
        return;
    }

    EContext context = infoIn->context;
    ERenderer renderer = malloc(sizeof(*context));

    if (!renderer) {
        *rendererOut = NULL;
        return;
    }
    *rendererOut = renderer;
    *renderer = (struct ERenderer_t){ 0 };

    CreateSampler(renderer, context);
    CreateDescriptorSetLayout(renderer, context);
    CreateDescriptorPool(renderer, context);
    CreatePipelineLayout(renderer, context);
    CreatePipeline(renderer, context, infoIn);
}

E_EXTERN void eDestroyRenderer(ERenderer renderer, EContext context) {
    vkDestroyPipeline(context->device, renderer->pipeline, NULL);
    vkDestroyShaderModule(context->device, renderer->fragShader, NULL);
    vkDestroyShaderModule(context->device, renderer->vertShader, NULL);
    vkDestroyPipelineLayout(context->device, renderer->pipelineLayout, NULL);
    vkDestroyDescriptorPool(context->device, renderer->descPool, NULL);
    vkDestroyDescriptorSetLayout(
      context->device, renderer->descSetLayout, NULL);
    vkDestroySampler(context->device, renderer->sampler, NULL);
    free(renderer);
}

E_EXTERN void eDestroyTexture(ERenderer renderer, EContext context);
E_EXTERN void eCreateTexture(ERenderer renderer, EContext context) {
    if (renderer->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    if (renderer->texture->descriptorSet) {
        vkQueueWaitIdle(context->queue);
        eDestroyTexture(renderer, context);
    }
}


static void CreatePipeline(ERenderer renderer,
  EContext context,
  ERendererCreateInfo* infoIn) {
    if (renderer->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    // uint32_t* vert = { NULL };
    // uint32_t* frag = { NULL };
    // uint32_t fileSize = { 0 };

    // FILE* file = { NULL };
    // errno_t ferr = { 0 };

    // (void)fopen_s(&file,
    //   "C:"
    //   "\\Users\\danie\\Desktop\\Sources\\projects\\EldenSheet\\src\\core\\shade"
    //   "rs\\shader.vert.in",
    //   "rb");

    // (void)fseek(file, 0, SEEK_END);
    // fileSize = ftell(file);
    // (void)fseek(file, 0, SEEK_SET);

    // vert = malloc(fileSize);
    // (void)fread_s(vert, fileSize, 1, fileSize, file);
    // (void)fclose(file);

    VkShaderModuleCreateInfo vsmci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(__glsl_shader_vert_spv),
        .pCode = __glsl_shader_vert_spv,
    };
    vkCreateShaderModule(context->device, &vsmci, NULL, &renderer->vertShader);

    // (void)fopen_s(&file,
    //   "\\Users\\danie\\Desktop\\Sources\\projects\\EldenSheet\\src\\core\\shade"
    //   "rs\\shader.frag.in",
    //   "rb");

    // (void)fseek(file, 0, SEEK_END);
    // fileSize = ftell(file);
    // (void)fseek(file, 0, SEEK_SET);

    // frag = malloc(fileSize);
    // (void)fread_s(frag, fileSize, 1, fileSize, file);
    // (void)fclose(file);

    VkShaderModuleCreateInfo fsmci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(__glsl_shader_frag_spv),
        .pCode = __glsl_shader_frag_spv,
    };
    vkCreateShaderModule(context->device, &fsmci, NULL, &renderer->fragShader);

    VkPipelineShaderStageCreateInfo pssci[2] = {
        (VkPipelineShaderStageCreateInfo){
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pName = "main",
          .module = renderer->vertShader,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
        },
        (VkPipelineShaderStageCreateInfo){
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pName = "main",
          .module = renderer->fragShader,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };
    VkVertexInputBindingDescription vertBindDesc = {
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        .stride = infoIn->imguiVertData.inputAttrSize,
    };
    VkVertexInputAttributeDescription vertAttrDesc[3] = {
        (VkVertexInputAttributeDescription){
          .binding = vertBindDesc.binding,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .location = 0,
          .offset = infoIn->imguiVertData.inputAttrOffsets[0],
        },
        (VkVertexInputAttributeDescription){
          .binding = vertBindDesc.binding,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .location = 1,
          .offset = infoIn->imguiVertData.inputAttrOffsets[1],
        },
        (VkVertexInputAttributeDescription){
          .binding = vertBindDesc.binding,
          .format = VK_FORMAT_R8G8B8A8_UNORM,
          .location = 2,
          .offset = infoIn->imguiVertData.inputAttrOffsets[2],
        },
    };
    VkPipelineVertexInputStateCreateInfo pvisci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pVertexAttributeDescriptions = vertAttrDesc,
        .vertexAttributeDescriptionCount = 3,
        .pVertexBindingDescriptions = &vertBindDesc,
        .vertexBindingDescriptionCount = 1,
    };
    VkPipelineInputAssemblyStateCreateInfo piasci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    VkPipelineViewportStateCreateInfo pvsci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };
    VkPipelineRasterizationStateCreateInfo prsci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .cullMode = VK_POLYGON_MODE_FILL,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    };
    VkPipelineMultisampleStateCreateInfo pmsci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    VkPipelineColorBlendAttachmentState colorAttach = {
        .colorBlendOp = VK_BLEND_OP_ADD,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_TRUE,
    };
    VkPipelineColorBlendStateCreateInfo pcbsci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pAttachments = &colorAttach,
        .attachmentCount = 1,
    };
    VkPipelineDepthStencilStateCreateInfo pdssci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };
    VkDynamicState dynStates[2] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo pdsci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynStates,
    };
    VkGraphicsPipelineCreateInfo gpci = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .layout = renderer->pipelineLayout,
        .renderPass = infoIn->display->renderPass,
        .stageCount = 2,
        .pStages = pssci,
        .pVertexInputState = &pvisci,
        .pInputAssemblyState = &piasci,
        .pViewportState = &pvsci,
        .pRasterizationState = &prsci,
        .pMultisampleState = &pmsci,
        .pDepthStencilState = &pdssci,
        .pColorBlendState = &pcbsci,
        .pDynamicState = &pdsci,
    };

    err = vkCreateGraphicsPipelines(
      context->device, VK_NULL_HANDLE, 1, &gpci, NULL, &renderer->pipeline);
    if (err != VK_SUCCESS) {
        renderer->result = E_CREATE_PIPELINE_FAILURE;
    }
}

static void CreatePipelineLayout(ERenderer renderer, EContext context) {
    if (renderer->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    VkPushConstantRange pushConstant = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(float) * 4,
    };
    VkPipelineLayoutCreateInfo plci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pSetLayouts = &renderer->descSetLayout,
        .setLayoutCount = 1,
        .pPushConstantRanges = &pushConstant,
        .pushConstantRangeCount = 1,
    };
    err = vkCreatePipelineLayout(
      context->device, &plci, NULL, &renderer->pipelineLayout);
    if (err != VK_SUCCESS) {
        renderer->result = E_CREATE_PIPELINE_LAYOUT_FAILURE;
    }
}

static void CreateDescriptorPool(ERenderer renderer, EContext context) {
    if (renderer->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    };
    renderer->descPoolSize = sizeof(poolSizes) / sizeof(*poolSizes);

    uint32_t maxSets = { 0 };
    for (int i = 0; i < sizeof(poolSizes) / sizeof(*poolSizes); ++i) {
        maxSets += poolSizes[i].descriptorCount;
    }
    VkDescriptorPoolCreateInfo dpci = (VkDescriptorPoolCreateInfo){
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .pPoolSizes = poolSizes,
        .poolSizeCount = renderer->descPoolSize,
        .maxSets = maxSets,
    };
    err =
      vkCreateDescriptorPool(context->device, &dpci, NULL, &renderer->descPool);
    if (err != VK_SUCCESS) {
        context->result = E_CREATE_DESCRIPTOR_POOL_FAILURE;
    }
}

static void CreateDescriptorSetLayout(ERenderer renderer, EContext context) {
    if (renderer->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    VkDescriptorSetLayoutBinding dscBinding = {
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .descriptorCount = 1,
    };
    VkDescriptorSetLayoutCreateInfo dslci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &dscBinding,
    };
    err = vkCreateDescriptorSetLayout(
      context->device, &dslci, NULL, &renderer->descSetLayout);
    if (err != VK_SUCCESS) {
        renderer->result = E_CREATE_DESCRIPTOR_POOL_FAILURE;
    }
}

static void CreateSampler(ERenderer renderer, EContext context) {
    if (renderer->result != E_SUCCESS) {
        return;
    }
    VkResult err = { 0 };

    VkSamplerCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .magFilter = VK_FILTER_LINEAR,
        .minLod = -1000,
        .maxLod = -1000,
        .maxAnisotropy = 1.0f,
    };
    err = vkCreateSampler(context->device, &sci, NULL, &renderer->sampler);
    if (err != VK_SUCCESS) {
        renderer->result = E_CREATE_SAMPLER_FAILURE;
    }
}
