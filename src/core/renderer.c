#include "renderer.h"

#include "core.h"

#include <stdio.h>
#include <stdlib.h>


static void CreateSampler(ERenderer renderer, EContext context);
static void CreateDescriptorSetLayout(ERenderer renderer, EContext context);
static void CreateDescriptorPool(ERenderer renderer, EContext context);
static void CreatePipelineLayout(ERenderer renderer, EContext context);

E_EXTERN void eCreateRenderer(ERenderer* rendererOut, EContext context) {
    if (!rendererOut) {
        return;
    }
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
}

E_EXTERN void eDestroyRenderer(ERenderer renderer, EContext context) {
    vkDestroyPipelineLayout(context->device, renderer->pipelineLayout, NULL);
    vkDestroyDescriptorPool(context->device, renderer->descPool, NULL);
    vkDestroyDescriptorSetLayout(
      context->device, renderer->descSetLayout, NULL);
    vkDestroySampler(context->device, renderer->sampler, NULL);
    free(renderer);
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
