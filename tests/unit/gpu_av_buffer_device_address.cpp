/*
 * Copyright (c) 2020-2023 The Khronos Group Inc.
 * Copyright (c) 2020-2023 Valve Corporation
 * Copyright (c) 2020-2023 LunarG, Inc.
 * Copyright (c) 2020-2023 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "../framework/layer_validation_tests.h"
#include "../framework/pipeline_helper.h"
#include "../framework/descriptor_helper.h"
#include "../framework/gpu_av_helper.h"

TEST_F(NegativeGpuAVBufferDeviceAddress, ReadBeforePointerPushConstant) {
    TEST_DESCRIPTION("Read before the valid pointer - use Push Constants to set the value");
    RETURN_IF_SKIP(InitGpuVUBufferDeviceAddress());
    InitRenderTarget();

    VkMemoryPropertyFlags mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkMemoryAllocateFlagsInfo allocate_flag_info = vku::InitStructHelper();
    allocate_flag_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    vkt::Buffer buffer(*m_device, 64, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, mem_props, &allocate_flag_info);

    VkPushConstantRange push_constant_ranges = {VK_SHADER_STAGE_VERTEX_BIT, 0, 2 * sizeof(VkDeviceAddress)};
    VkPipelineLayoutCreateInfo plci = vku::InitStructHelper();
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &push_constant_ranges;
    vkt::PipelineLayout pipeline_layout(*m_device, plci);

    char const *shader_source = R"glsl(
            #version 450
            #extension GL_EXT_buffer_reference : enable
            layout(buffer_reference, buffer_reference_align = 16) buffer bufStruct;
            layout(push_constant) uniform ufoo {
                bufStruct data;
                int nWrites;
            } u_info;
            layout(buffer_reference, std140) buffer bufStruct {
                int a[4];
            };
            void main() {
                for (int i=0; i < u_info.nWrites; ++i) {
                    u_info.data.a[i] = 0xdeadca71;
                }
            }
        )glsl";
    VkShaderObj vs(this, shader_source, VK_SHADER_STAGE_VERTEX_BIT);

    CreatePipelineHelper pipe(*this);
    pipe.InitState();
    pipe.shader_stages_[0] = vs.GetStageCreateInfo();
    pipe.gp_ci_.layout = pipeline_layout.handle();
    pipe.CreateGraphicsPipeline();

    m_commandBuffer->begin();
    m_commandBuffer->BeginRenderPass(m_renderPassBeginInfo);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.Handle());

    VkDeviceAddress u_info_ptr = buffer.address();
    // Will dereference the wrong ptr address
    VkDeviceAddress push_constants[2] = {u_info_ptr - 16, 4};
    vk::CmdPushConstants(m_commandBuffer->handle(), pipeline_layout.handle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants),
                         push_constants);

    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_commandBuffer->EndRenderPass();
    m_commandBuffer->end();

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "UNASSIGNED-Device address out of bounds");
    m_default_queue->submit(*m_commandBuffer, false);
    m_default_queue->wait();
    m_errorMonitor->VerifyFound();
}

// https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/7132
TEST_F(NegativeGpuAVBufferDeviceAddress, DISABLED_ReadAfterPointerPushConstant) {
    TEST_DESCRIPTION("Read after the valid pointer - use Push Constants to set the value");
    RETURN_IF_SKIP(InitGpuVUBufferDeviceAddress());
    InitRenderTarget();

    VkMemoryPropertyFlags mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkMemoryAllocateFlagsInfo allocate_flag_info = vku::InitStructHelper();
    allocate_flag_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    vkt::Buffer buffer(*m_device, 64, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, mem_props, &allocate_flag_info);

    VkPushConstantRange push_constant_ranges = {VK_SHADER_STAGE_VERTEX_BIT, 0, 2 * sizeof(VkDeviceAddress)};
    VkPipelineLayoutCreateInfo plci = vku::InitStructHelper();
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &push_constant_ranges;
    vkt::PipelineLayout pipeline_layout(*m_device, plci);

    char const *shader_source = R"glsl(
            #version 450
            #extension GL_EXT_buffer_reference : enable
            layout(buffer_reference, buffer_reference_align = 16) buffer bufStruct;
            layout(push_constant) uniform ufoo {
                bufStruct data;
                int nWrites;
            } u_info;
            layout(buffer_reference, std140) buffer bufStruct {
                int a[4];
            };
            void main() {
                for (int i=0; i < u_info.nWrites; ++i) {
                    u_info.data.a[i] = 0xdeadca71;
                }
            }
        )glsl";
    VkShaderObj vs(this, shader_source, VK_SHADER_STAGE_VERTEX_BIT);

    CreatePipelineHelper pipe(*this);
    pipe.InitState();
    pipe.shader_stages_[0] = vs.GetStageCreateInfo();
    pipe.gp_ci_.layout = pipeline_layout.handle();
    pipe.CreateGraphicsPipeline();

    m_commandBuffer->begin();
    m_commandBuffer->BeginRenderPass(m_renderPassBeginInfo);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.Handle());

    VkDeviceAddress u_info_ptr = buffer.address();
    // will go over a[4] by one
    VkDeviceAddress push_constants[2] = {u_info_ptr, 5};
    vk::CmdPushConstants(m_commandBuffer->handle(), pipeline_layout.handle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants),
                         push_constants);

    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_commandBuffer->EndRenderPass();
    m_commandBuffer->end();

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "UNASSIGNED-Device address out of bounds");
    m_default_queue->submit(*m_commandBuffer, false);
    m_default_queue->wait();
    m_errorMonitor->VerifyFound();
}

TEST_F(NegativeGpuAVBufferDeviceAddress, ReadBeforePointerDescriptor) {
    TEST_DESCRIPTION("Read before the valid pointer - use Descriptor to set the value");
    RETURN_IF_SKIP(InitGpuVUBufferDeviceAddress());
    InitRenderTarget();

    VkMemoryPropertyFlags mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkMemoryAllocateFlagsInfo allocate_flag_info = vku::InitStructHelper();
    allocate_flag_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    vkt::Buffer buffer(*m_device, 64, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, mem_props, &allocate_flag_info);

    OneOffDescriptorSet descriptor_set(m_device, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr}});
    const vkt::PipelineLayout pipeline_layout(*m_device, {&descriptor_set.layout_});

    vkt::Buffer uniform_buffer(*m_device, 12, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, mem_props);
    descriptor_set.WriteDescriptorBufferInfo(0, uniform_buffer.handle(), 0, VK_WHOLE_SIZE);
    descriptor_set.UpdateDescriptorSets();

    VkDeviceAddress u_info_ptr = buffer.address();
    // Will dereference the wrong ptr address
    VkDeviceAddress data = u_info_ptr - 16;
    uint32_t n_writes = 4;

    uint8_t *uniform_buffer_ptr = (uint8_t *)uniform_buffer.memory().map();
    memcpy(uniform_buffer_ptr, &data, sizeof(VkDeviceAddress));
    memcpy(uniform_buffer_ptr + sizeof(VkDeviceAddress), &n_writes, sizeof(uint32_t));
    uniform_buffer.memory().unmap();

    char const *shader_source = R"glsl(
        #version 450
        #extension GL_EXT_buffer_reference : enable
        layout(buffer_reference, buffer_reference_align = 16) buffer bufStruct;
        layout(set = 0, binding = 0) uniform ufoo {
            bufStruct data;
            int nWrites;
        } u_info;
        layout(buffer_reference, std140) buffer bufStruct {
            int a[4];
        };
        void main() {
            for (int i=0; i < u_info.nWrites; ++i) {
                u_info.data.a[i] = 0xdeadca71;
            }
        }
    )glsl";
    VkShaderObj vs(this, shader_source, VK_SHADER_STAGE_VERTEX_BIT);

    CreatePipelineHelper pipe(*this);
    pipe.InitState();
    pipe.shader_stages_[0] = vs.GetStageCreateInfo();
    pipe.gp_ci_.layout = pipeline_layout.handle();
    pipe.CreateGraphicsPipeline();

    m_commandBuffer->begin();
    m_commandBuffer->BeginRenderPass(m_renderPassBeginInfo);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.Handle());
    vk::CmdBindDescriptorSets(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout.handle(), 0, 1,
                              &descriptor_set.set_, 0, nullptr);
    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_commandBuffer->EndRenderPass();
    m_commandBuffer->end();

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "UNASSIGNED-Device address out of bounds");
    m_default_queue->submit(*m_commandBuffer, false);
    m_default_queue->wait();
    m_errorMonitor->VerifyFound();
}

// https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/7132
TEST_F(NegativeGpuAVBufferDeviceAddress, DISABLED_ReadAfterPointerDescriptor) {
    TEST_DESCRIPTION("Read after the valid pointer - use Descriptor to set the value");
    RETURN_IF_SKIP(InitGpuVUBufferDeviceAddress());
    InitRenderTarget();

    VkMemoryPropertyFlags mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    VkMemoryAllocateFlagsInfo allocate_flag_info = vku::InitStructHelper();
    allocate_flag_info.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    vkt::Buffer buffer(*m_device, 64, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, mem_props, &allocate_flag_info);

    OneOffDescriptorSet descriptor_set(m_device, {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr}});
    const vkt::PipelineLayout pipeline_layout(*m_device, {&descriptor_set.layout_});

    vkt::Buffer uniform_buffer(*m_device, 12, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, mem_props);
    descriptor_set.WriteDescriptorBufferInfo(0, uniform_buffer.handle(), 0, VK_WHOLE_SIZE);
    descriptor_set.UpdateDescriptorSets();

    VkDeviceAddress u_info_ptr = buffer.address();
    // will go over a[4] by one
    uint32_t n_writes = 5;

    uint8_t *uniform_buffer_ptr = (uint8_t *)uniform_buffer.memory().map();
    memcpy(uniform_buffer_ptr, &u_info_ptr, sizeof(VkDeviceAddress));
    memcpy(uniform_buffer_ptr + sizeof(VkDeviceAddress), &n_writes, sizeof(uint32_t));
    uniform_buffer.memory().unmap();

    char const *shader_source = R"glsl(
        #version 450
        #extension GL_EXT_buffer_reference : enable
        layout(buffer_reference, buffer_reference_align = 16) buffer bufStruct;
        layout(set = 0, binding = 0) uniform ufoo {
            bufStruct data;
            int nWrites;
        } u_info;
        layout(buffer_reference, std140) buffer bufStruct {
            int a[4];
        };
        void main() {
            for (int i=0; i < u_info.nWrites; ++i) {
                u_info.data.a[i] = 0xdeadca71;
            }
        }
    )glsl";
    VkShaderObj vs(this, shader_source, VK_SHADER_STAGE_VERTEX_BIT);

    CreatePipelineHelper pipe(*this);
    pipe.InitState();
    pipe.shader_stages_[0] = vs.GetStageCreateInfo();
    pipe.gp_ci_.layout = pipeline_layout.handle();
    pipe.CreateGraphicsPipeline();

    m_commandBuffer->begin();
    m_commandBuffer->BeginRenderPass(m_renderPassBeginInfo);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.Handle());
    vk::CmdBindDescriptorSets(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout.handle(), 0, 1,
                              &descriptor_set.set_, 0, nullptr);
    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_commandBuffer->EndRenderPass();
    m_commandBuffer->end();

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "UNASSIGNED-Device address out of bounds");
    m_default_queue->submit(*m_commandBuffer, false);
    m_default_queue->wait();
    m_errorMonitor->VerifyFound();
}
