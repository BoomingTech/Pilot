#include "runtime/function/render/include/render/vulkan_manager/vulkan_common.h"
#include "runtime/function/render/include/render/vulkan_manager/vulkan_mesh.h"
#include "runtime/function/render/include/render/vulkan_manager/vulkan_misc.h"
#include "runtime/function/render/include/render/vulkan_manager/vulkan_point_light_pass.h"
#include "runtime/function/render/include/render/vulkan_manager/vulkan_util.h"

#include <mesh_point_light_shadow_frag.h>
#include <mesh_point_light_shadow_geom.h>
#include <mesh_point_light_shadow_vert.h>

#include <map>

namespace Pilot
{
    void PPointLightShadowPass::initialize()
    {
        createImage();
        setupRenderPass();
        setupFramebuffer();
        setupDescriptorSetLayout();
    }
    void PPointLightShadowPass::postInitialize()
    {
        setupPipelines();
        setupDescriptorSet();
    }
    void PPointLightShadowPass::draw()
    {
        auto& command_buffer = m_command_info._current_command_buffer;
        if (m_render_config._enable_debug_untils_label)
        {
            VkDebugUtilsLabelEXT label_info = {
                VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, NULL, "Point Light Shadow", {1.0f, 1.0f, 1.0f, 1.0f}};
            m_p_vulkan_context->_vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label_info);
        }

        drawModel();

        if (m_render_config._enable_debug_untils_label)
        {
            m_p_vulkan_context->_vkCmdEndDebugUtilsLabelEXT(command_buffer);
        }
    }
    void PPointLightShadowPass::createImage()
    {
        m_p_vulkan_context->CreateRenderImage2D(std::hash<std::string>()("point shadow color"),
                                                VK_FORMAT_R32_SFLOAT,
                                                m_point_light_shadow_map_dimension,
                                                m_point_light_shadow_map_dimension,
                                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                VK_IMAGE_ASPECT_COLOR_BIT,
                                                2 * m_max_point_light_count,
                                                1);

        m_p_vulkan_context->CreateRenderImage2D(std::hash<std::string>()("point shadow depth"),
                                                m_p_vulkan_context->_depth_image_format,
                                                m_point_light_shadow_map_dimension,
                                                m_point_light_shadow_map_dimension,
                                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                                                VK_IMAGE_ASPECT_DEPTH_BIT,
                                                2 * m_max_point_light_count,
                                                1);
    }
    void PPointLightShadowPass::setupRenderPass()
    {
        auto&                   _device        = m_p_vulkan_context->_device;
        VkAttachmentDescription attachments[2] = {};

        VkAttachmentDescription& point_light_shadow_color_attachment_description = attachments[0];
        point_light_shadow_color_attachment_description.format =
            m_p_vulkan_context->GetImageFormat(std::hash<std::string>()("point shadow color"));
        point_light_shadow_color_attachment_description.samples        = VK_SAMPLE_COUNT_1_BIT;
        point_light_shadow_color_attachment_description.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        point_light_shadow_color_attachment_description.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        point_light_shadow_color_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        point_light_shadow_color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        point_light_shadow_color_attachment_description.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        point_light_shadow_color_attachment_description.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentDescription& point_light_shadow_depth_attachment_description = attachments[1];
        point_light_shadow_depth_attachment_description.format =
            m_p_vulkan_context->GetImageFormat(std::hash<std::string>()("point shadow depth"));
        point_light_shadow_depth_attachment_description.samples        = VK_SAMPLE_COUNT_1_BIT;
        point_light_shadow_depth_attachment_description.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        point_light_shadow_depth_attachment_description.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        point_light_shadow_depth_attachment_description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        point_light_shadow_depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        point_light_shadow_depth_attachment_description.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        point_light_shadow_depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpasses[1] = {};

        VkAttachmentReference shadow_pass_color_attachment_reference {};
        shadow_pass_color_attachment_reference.attachment =
            &point_light_shadow_color_attachment_description - attachments;
        shadow_pass_color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference shadow_pass_depth_attachment_reference {};
        shadow_pass_depth_attachment_reference.attachment =
            &point_light_shadow_depth_attachment_description - attachments;
        shadow_pass_depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription& shadow_pass   = subpasses[0];
        shadow_pass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        shadow_pass.colorAttachmentCount    = 1;
        shadow_pass.pColorAttachments       = &shadow_pass_color_attachment_reference;
        shadow_pass.pDepthStencilAttachment = &shadow_pass_depth_attachment_reference;

        VkSubpassDependency dependencies[1] = {};

        VkSubpassDependency& lighting_pass_dependency = dependencies[0];
        lighting_pass_dependency.srcSubpass           = 0;
        lighting_pass_dependency.dstSubpass           = VK_SUBPASS_EXTERNAL;
        lighting_pass_dependency.srcStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        lighting_pass_dependency.dstStageMask         = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        lighting_pass_dependency.srcAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // STORE_OP_STORE
        lighting_pass_dependency.dstAccessMask        = 0;
        lighting_pass_dependency.dependencyFlags      = 0; // NOT BY REGION

        VkRenderPassCreateInfo renderpass_create_info {};
        renderpass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpass_create_info.attachmentCount = std::size(attachments);
        renderpass_create_info.pAttachments    = attachments;
        renderpass_create_info.subpassCount    = std::size(subpasses);
        renderpass_create_info.pSubpasses      = subpasses;
        renderpass_create_info.dependencyCount = std::size(dependencies);
        renderpass_create_info.pDependencies   = dependencies;

        if (vkCreateRenderPass(_device, &renderpass_create_info, nullptr, &_framebuffer.render_pass) != VK_SUCCESS)
        {
            throw std::runtime_error("create point light shadow render pass");
        }
    }
    void PPointLightShadowPass::setupFramebuffer()
    {
        auto&       _device        = m_p_vulkan_context->_device;
        VkImageView attachments[2] = {m_p_vulkan_context->GetImageView(std::hash<std::string>()("point shadow color")),
                                      m_p_vulkan_context->GetImageView(std::hash<std::string>()("point shadow depth"))};

        VkFramebufferCreateInfo framebuffer_create_info {};
        framebuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.flags           = 0U;
        framebuffer_create_info.renderPass      = _framebuffer.render_pass;
        framebuffer_create_info.attachmentCount = std::size(attachments);
        framebuffer_create_info.pAttachments    = attachments;
        framebuffer_create_info.width           = m_point_light_shadow_map_dimension;
        framebuffer_create_info.height          = m_point_light_shadow_map_dimension;
        framebuffer_create_info.layers          = 2 * m_max_point_light_count;

        if (vkCreateFramebuffer(_device, &framebuffer_create_info, nullptr, &_framebuffer.framebuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("create point light shadow framebuffer");
        }
    }
    void PPointLightShadowPass::setupDescriptorSetLayout()
    {
        auto& _device = m_p_vulkan_context->_device;
        _descriptor_infos.resize(1);

        VkDescriptorSetLayoutBinding mesh_point_light_shadow_global_layout_bindings[3];

        VkDescriptorSetLayoutBinding& mesh_point_light_shadow_global_layout_perframe_storage_buffer_binding =
            mesh_point_light_shadow_global_layout_bindings[0];
        mesh_point_light_shadow_global_layout_perframe_storage_buffer_binding.binding = 0;
        mesh_point_light_shadow_global_layout_perframe_storage_buffer_binding.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_point_light_shadow_global_layout_perframe_storage_buffer_binding.descriptorCount = 1;
        mesh_point_light_shadow_global_layout_perframe_storage_buffer_binding.stageFlags =
            VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding& mesh_point_light_shadow_global_layout_perdrawcall_storage_buffer_binding =
            mesh_point_light_shadow_global_layout_bindings[1];
        mesh_point_light_shadow_global_layout_perdrawcall_storage_buffer_binding.binding = 1;
        mesh_point_light_shadow_global_layout_perdrawcall_storage_buffer_binding.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_point_light_shadow_global_layout_perdrawcall_storage_buffer_binding.descriptorCount = 1;
        mesh_point_light_shadow_global_layout_perdrawcall_storage_buffer_binding.stageFlags =
            VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding&
            mesh_point_light_shadow_global_layout_per_drawcall_vertex_blending_storage_buffer_binding =
                mesh_point_light_shadow_global_layout_bindings[2];
        mesh_point_light_shadow_global_layout_per_drawcall_vertex_blending_storage_buffer_binding.binding = 2;
        mesh_point_light_shadow_global_layout_per_drawcall_vertex_blending_storage_buffer_binding.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_point_light_shadow_global_layout_per_drawcall_vertex_blending_storage_buffer_binding.descriptorCount = 1;
        mesh_point_light_shadow_global_layout_per_drawcall_vertex_blending_storage_buffer_binding.stageFlags =
            VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo mesh_point_light_shadow_global_layout_create_info;
        mesh_point_light_shadow_global_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        mesh_point_light_shadow_global_layout_create_info.pNext = NULL;
        mesh_point_light_shadow_global_layout_create_info.flags = 0;
        mesh_point_light_shadow_global_layout_create_info.bindingCount =
            (sizeof(mesh_point_light_shadow_global_layout_bindings) /
             sizeof(mesh_point_light_shadow_global_layout_bindings[0]));
        mesh_point_light_shadow_global_layout_create_info.pBindings = mesh_point_light_shadow_global_layout_bindings;

        if (VK_SUCCESS !=
            vkCreateDescriptorSetLayout(
                _device, &mesh_point_light_shadow_global_layout_create_info, NULL, &_descriptor_infos[0].layout))
        {
            throw std::runtime_error("create mesh point light shadow global layout");
        }
    }
    void PPointLightShadowPass::setupPipelines()
    {
        auto& _device = m_p_vulkan_context->_device;
        if (!m_render_config._enable_point_light_shadow)
            return;

        _render_pipelines.resize(1);

        VkDescriptorSetLayout      descriptorset_layouts[] = {_descriptor_infos[0].layout, _per_mesh_layout};
        VkPipelineLayoutCreateInfo pipeline_layout_create_info {};
        pipeline_layout_create_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_create_info.setLayoutCount = std::size(descriptorset_layouts);
        pipeline_layout_create_info.pSetLayouts    = descriptorset_layouts;

        if (vkCreatePipelineLayout(_device, &pipeline_layout_create_info, nullptr, &_render_pipelines[0].layout) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("create mesh point light shadow pipeline layout");
        }

        VkPipelineShaderStageCreateInfo shader_stages[3] = {};
        FillShaderStageCreateInfo(shader_stages,
                                  _device,
                                  MESH_POINT_LIGHT_SHADOW_VERT,
                                  MESH_POINT_LIGHT_SHADOW_GEOM,
                                  MESH_POINT_LIGHT_SHADOW_FRAG);

        auto                                 vertex_binding_descriptions   = PMeshVertex::getBindingDescriptions();
        auto                                 vertex_attribute_descriptions = PMeshVertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info {};
        vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_state_create_info.vertexBindingDescriptionCount   = 1;
        vertex_input_state_create_info.pVertexBindingDescriptions      = &vertex_binding_descriptions[0];
        vertex_input_state_create_info.vertexAttributeDescriptionCount = 1;
        vertex_input_state_create_info.pVertexAttributeDescriptions    = &vertex_attribute_descriptions[0];

        VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info {};
        input_assembly_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly_create_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewport_state_create_info {};
        viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports    = NULL;
        viewport_state_create_info.scissorCount  = 1;
        viewport_state_create_info.pScissors     = NULL;

        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info {};
        rasterization_state_create_info.sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterization_state_create_info.depthClampEnable = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode             = VK_POLYGON_MODE_FILL;
        rasterization_state_create_info.lineWidth               = 1.0f;
        // TODO : test more to verify
        rasterization_state_create_info.cullMode                = VK_CULL_MODE_BACK_BIT;
        rasterization_state_create_info.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization_state_create_info.depthBiasEnable         = VK_FALSE;
        rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
        rasterization_state_create_info.depthBiasClamp          = 0.0f;
        rasterization_state_create_info.depthBiasSlopeFactor    = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisample_state_create_info {};
        multisample_state_create_info.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_state_create_info.sampleShadingEnable  = VK_FALSE;
        multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState color_blend_attachment_state {};
        color_blend_attachment_state.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment_state.blendEnable         = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo color_blend_state_create_info {};
        color_blend_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blend_state_create_info.logicOpEnable     = VK_FALSE;
        color_blend_state_create_info.logicOp           = VK_LOGIC_OP_COPY;
        color_blend_state_create_info.attachmentCount   = 1;
        color_blend_state_create_info.pAttachments      = &color_blend_attachment_state;
        color_blend_state_create_info.blendConstants[0] = 0.0f;
        color_blend_state_create_info.blendConstants[1] = 0.0f;
        color_blend_state_create_info.blendConstants[2] = 0.0f;
        color_blend_state_create_info.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info {};
        depth_stencil_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil_create_info.depthTestEnable       = VK_TRUE;
        depth_stencil_create_info.depthWriteEnable      = VK_TRUE;
        depth_stencil_create_info.depthCompareOp        = VK_COMPARE_OP_LESS;
        depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_create_info.stencilTestEnable     = VK_FALSE;

        VkDynamicState                   dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic_state_create_info {};
        dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state_create_info.dynamicStateCount = std::size(dynamic_states);
        dynamic_state_create_info.pDynamicStates    = dynamic_states;

        VkGraphicsPipelineCreateInfo pipelineInfo {};
        pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = std::size(shader_stages);
        pipelineInfo.pStages             = shader_stages;
        pipelineInfo.pVertexInputState   = &vertex_input_state_create_info;
        pipelineInfo.pInputAssemblyState = &input_assembly_create_info;
        pipelineInfo.pViewportState      = &viewport_state_create_info;
        pipelineInfo.pRasterizationState = &rasterization_state_create_info;
        pipelineInfo.pMultisampleState   = &multisample_state_create_info;
        pipelineInfo.pColorBlendState    = &color_blend_state_create_info;
        pipelineInfo.pDepthStencilState  = &depth_stencil_create_info;
        pipelineInfo.layout              = _render_pipelines[0].layout;
        pipelineInfo.renderPass          = _framebuffer.render_pass;
        pipelineInfo.subpass             = 0;
        pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
        pipelineInfo.pDynamicState       = &dynamic_state_create_info;

        if (vkCreateGraphicsPipelines(
                _device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_render_pipelines[0].pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("create mesh point light shadow graphics pipeline");
        }

        ModuleGC();
    }
    void PPointLightShadowPass::setupDescriptorSet()
    {
        auto& _device         = m_p_vulkan_context->_device;
        auto& _storage_buffer = m_p_global_render_resource->_storage_buffer;

        VkDescriptorSetAllocateInfo mesh_point_light_shadow_global_descriptor_set_alloc_info;
        mesh_point_light_shadow_global_descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        mesh_point_light_shadow_global_descriptor_set_alloc_info.pNext = NULL;
        mesh_point_light_shadow_global_descriptor_set_alloc_info.descriptorPool     = m_descriptor_pool;
        mesh_point_light_shadow_global_descriptor_set_alloc_info.descriptorSetCount = 1;
        mesh_point_light_shadow_global_descriptor_set_alloc_info.pSetLayouts        = &_descriptor_infos[0].layout;

        if (VK_SUCCESS != vkAllocateDescriptorSets(_device,
                                                   &mesh_point_light_shadow_global_descriptor_set_alloc_info,
                                                   &_descriptor_infos[0].descriptor_set))
        {
            throw std::runtime_error("allocate mesh point light shadow global descriptor set");
        }

        VkDescriptorBufferInfo mesh_point_light_shadow_perframe_storage_buffer_info = {};
        // this offset plus dynamic_offset should not be greater than the size of the buffer
        mesh_point_light_shadow_perframe_storage_buffer_info.offset = 0;
        // the range means the size actually used by the shader per draw call
        mesh_point_light_shadow_perframe_storage_buffer_info.range =
            sizeof(MeshPointLightShadowPerframeStorageBufferObject);
        mesh_point_light_shadow_perframe_storage_buffer_info.buffer = _storage_buffer._global_upload_ringbuffer;
        assert(mesh_point_light_shadow_perframe_storage_buffer_info.range < _storage_buffer._max_storage_buffer_range);

        VkDescriptorBufferInfo mesh_point_light_shadow_perdrawcall_storage_buffer_info = {};
        mesh_point_light_shadow_perdrawcall_storage_buffer_info.offset                 = 0;
        mesh_point_light_shadow_perdrawcall_storage_buffer_info.range =
            sizeof(MeshPointLightShadowPerdrawcallStorageBufferObject);
        mesh_point_light_shadow_perdrawcall_storage_buffer_info.buffer = _storage_buffer._global_upload_ringbuffer;
        assert(mesh_point_light_shadow_perdrawcall_storage_buffer_info.range <
               _storage_buffer._max_storage_buffer_range);

        VkDescriptorBufferInfo mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_info = {};
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_info.offset                 = 0;
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_info.range =
            sizeof(MeshPointLightShadowPerdrawcallVertexBlendingStorageBufferObject);
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_info.buffer =
            _storage_buffer._global_upload_ringbuffer;
        assert(mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_info.range <
               _storage_buffer._max_storage_buffer_range);

        VkDescriptorSet descriptor_set_to_write = _descriptor_infos[0].descriptor_set;

        VkWriteDescriptorSet descriptor_writes[3];

        VkWriteDescriptorSet& mesh_point_light_shadow_perframe_storage_buffer_write_info = descriptor_writes[0];
        mesh_point_light_shadow_perframe_storage_buffer_write_info.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mesh_point_light_shadow_perframe_storage_buffer_write_info.pNext      = NULL;
        mesh_point_light_shadow_perframe_storage_buffer_write_info.dstSet     = descriptor_set_to_write;
        mesh_point_light_shadow_perframe_storage_buffer_write_info.dstBinding = 0;
        mesh_point_light_shadow_perframe_storage_buffer_write_info.dstArrayElement = 0;
        mesh_point_light_shadow_perframe_storage_buffer_write_info.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_point_light_shadow_perframe_storage_buffer_write_info.descriptorCount = 1;
        mesh_point_light_shadow_perframe_storage_buffer_write_info.pBufferInfo =
            &mesh_point_light_shadow_perframe_storage_buffer_info;

        VkWriteDescriptorSet& mesh_point_light_shadow_perdrawcall_storage_buffer_write_info = descriptor_writes[1];
        mesh_point_light_shadow_perdrawcall_storage_buffer_write_info.sType  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mesh_point_light_shadow_perdrawcall_storage_buffer_write_info.pNext  = NULL;
        mesh_point_light_shadow_perdrawcall_storage_buffer_write_info.dstSet = descriptor_set_to_write;
        mesh_point_light_shadow_perdrawcall_storage_buffer_write_info.dstBinding      = 1;
        mesh_point_light_shadow_perdrawcall_storage_buffer_write_info.dstArrayElement = 0;
        mesh_point_light_shadow_perdrawcall_storage_buffer_write_info.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_point_light_shadow_perdrawcall_storage_buffer_write_info.descriptorCount = 1;
        mesh_point_light_shadow_perdrawcall_storage_buffer_write_info.pBufferInfo =
            &mesh_point_light_shadow_perdrawcall_storage_buffer_info;

        VkWriteDescriptorSet& mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_write_info =
            descriptor_writes[2];
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_write_info.sType =
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_write_info.pNext  = NULL;
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_write_info.dstSet = descriptor_set_to_write;
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_write_info.dstBinding      = 2;
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_write_info.dstArrayElement = 0;
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_write_info.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_write_info.descriptorCount = 1;
        mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_write_info.pBufferInfo =
            &mesh_point_light_shadow_per_drawcall_vertex_blending_storage_buffer_info;

        vkUpdateDescriptorSets(_device, std::size(descriptor_writes), descriptor_writes, 0, NULL);
    }
    void Pilot::PPointLightShadowPass::drawModel()
    {
        auto& command_buffer  = m_command_info._current_command_buffer;
        auto& _storage_buffer = m_p_global_render_resource->_storage_buffer;
        struct PMeshNode
        {
            glm::mat4 model_matrix;
            glm::mat4 joint_matrices[m_mesh_vertex_blending_max_joint_count];
            bool      enable_vertex_blending;
        };

        std::map<VulkanPBRMaterial*, std::map<VulkanMesh*, std::vector<PMeshNode>>> point_lights_mesh_drawcall_batch;

        // reorganize mesh
        for (PVulkanMeshNode& node : *(m_visiable_nodes.p_point_lights_visible_mesh_nodes))
        {
            auto& mesh_instanced = point_lights_mesh_drawcall_batch[node.ref_material];
            auto& mesh_nodes     = mesh_instanced[node.ref_mesh];

            PMeshNode temp;
            temp.model_matrix           = node.model_matrix;
            temp.enable_vertex_blending = node.enable_vertex_blending;
            if (node.enable_vertex_blending)
            {
                for (uint32_t i = 0; i < m_mesh_vertex_blending_max_joint_count; ++i)
                {
                    temp.joint_matrices[i] = node.joint_matrices[i];
                }
            }

            mesh_nodes.push_back(temp);
        }

        VkRenderPassBeginInfo renderpass_begin_info {};
        renderpass_begin_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderpass_begin_info.renderPass        = _framebuffer.render_pass;
        renderpass_begin_info.framebuffer       = _framebuffer.framebuffer;
        renderpass_begin_info.renderArea.offset = {0, 0};
        renderpass_begin_info.renderArea.extent = {m_point_light_shadow_map_dimension,
                                                   m_point_light_shadow_map_dimension};

        VkClearValue clear_values[2];
        clear_values[0].color                 = {1.0f};
        clear_values[1].depthStencil          = {1.0f, 0};
        renderpass_begin_info.clearValueCount = std::size(clear_values);
        renderpass_begin_info.pClearValues    = clear_values;

        m_p_vulkan_context->_vkCmdBeginRenderPass(command_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        if (m_render_config._enable_point_light_shadow)
        {
            if (m_render_config._enable_debug_untils_label)
            {
                VkDebugUtilsLabelEXT label_info = {
                    VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, NULL, "Mesh", {1.0f, 1.0f, 1.0f, 1.0f}};
                m_p_vulkan_context->_vkCmdBeginDebugUtilsLabelEXT(command_buffer, &label_info);
            }

            m_p_vulkan_context->_vkCmdBindPipeline(
                command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _render_pipelines[0].pipeline);

            VkViewport viewport = {
                0, 0, m_point_light_shadow_map_dimension, m_point_light_shadow_map_dimension, 0.0, 1.0};
            VkRect2D scissor = {{0, 0}, {m_point_light_shadow_map_dimension, m_point_light_shadow_map_dimension}};
            m_p_vulkan_context->_vkCmdSetViewport(command_buffer, 0, 1, &viewport);
            m_p_vulkan_context->_vkCmdSetScissor(command_buffer, 0, 1, &scissor);

            // perframe storage buffer
            uint32_t perframe_dynamic_offset =
                roundUp(_storage_buffer._global_upload_ringbuffers_end[m_command_info._current_frame_index],
                        _storage_buffer._min_storage_buffer_offset_alignment);
            _storage_buffer._global_upload_ringbuffers_end[m_command_info._current_frame_index] =
                perframe_dynamic_offset + sizeof(MeshPerframeStorageBufferObject);
            assert(_storage_buffer._global_upload_ringbuffers_end[m_command_info._current_frame_index] <=
                   (_storage_buffer._global_upload_ringbuffers_begin[m_command_info._current_frame_index] +
                    _storage_buffer._global_upload_ringbuffers_size[m_command_info._current_frame_index]));

            MeshPointLightShadowPerframeStorageBufferObject& perframe_storage_buffer_object =
                (*reinterpret_cast<MeshPointLightShadowPerframeStorageBufferObject*>(
                    reinterpret_cast<uintptr_t>(_storage_buffer._global_upload_ringbuffer_memory_pointer) +
                    perframe_dynamic_offset));
            perframe_storage_buffer_object = _mesh_point_light_shadow_perframe_storage_buffer_object;

            for (auto& pair1 : point_lights_mesh_drawcall_batch)
            {
                VulkanPBRMaterial& material       = (*pair1.first);
                auto&              mesh_instanced = pair1.second;

                // TODO: render from near to far

                for (auto& pair2 : mesh_instanced)
                {
                    VulkanMesh& mesh       = (*pair2.first);
                    auto&       mesh_nodes = pair2.second;

                    uint32_t total_instance_count = static_cast<uint32_t>(mesh_nodes.size());
                    if (total_instance_count == 0)
                    {
                        continue;
                    }
                    // bind per mesh
                    m_p_vulkan_context->_vkCmdBindDescriptorSets(command_buffer,
                                                                 VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                 _render_pipelines[0].layout,
                                                                 1,
                                                                 1,
                                                                 &mesh.mesh_vertex_blending_descriptor_set,
                                                                 0,
                                                                 NULL);

                    VkBuffer     vertex_buffers[] = {mesh.mesh_vertex_position_buffer};
                    VkDeviceSize offsets[]        = {0};
                    m_p_vulkan_context->_vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
                    m_p_vulkan_context->_vkCmdBindIndexBuffer(
                        command_buffer, mesh.mesh_index_buffer, 0, VK_INDEX_TYPE_UINT16);

                    uint32_t drawcall_max_instance_count =
                        (sizeof(MeshPointLightShadowPerdrawcallStorageBufferObject::mesh_instances) /
                         sizeof(MeshPointLightShadowPerdrawcallStorageBufferObject::mesh_instances[0]));
                    uint32_t drawcall_count =
                        roundUp(total_instance_count, drawcall_max_instance_count) / drawcall_max_instance_count;

                    for (uint32_t drawcall_index = 0; drawcall_index < drawcall_count; ++drawcall_index)
                    {
                        uint32_t current_instance_count =
                            ((total_instance_count - drawcall_max_instance_count * drawcall_index) <
                             drawcall_max_instance_count) ?
                                (total_instance_count - drawcall_max_instance_count * drawcall_index) :
                                drawcall_max_instance_count;

                        // perdrawcall storage buffer
                        uint32_t perdrawcall_dynamic_offset =
                            roundUp(_storage_buffer._global_upload_ringbuffers_end[m_command_info._current_frame_index],
                                    _storage_buffer._min_storage_buffer_offset_alignment);
                        _storage_buffer._global_upload_ringbuffers_end[m_command_info._current_frame_index] =
                            perdrawcall_dynamic_offset + sizeof(MeshPointLightShadowPerdrawcallStorageBufferObject);
                        assert(_storage_buffer._global_upload_ringbuffers_end[m_command_info._current_frame_index] <=
                               (_storage_buffer._global_upload_ringbuffers_begin[m_command_info._current_frame_index] +
                                _storage_buffer._global_upload_ringbuffers_size[m_command_info._current_frame_index]));

                        MeshPointLightShadowPerdrawcallStorageBufferObject& perdrawcall_storage_buffer_object =
                            (*reinterpret_cast<MeshPointLightShadowPerdrawcallStorageBufferObject*>(
                                reinterpret_cast<uintptr_t>(_storage_buffer._global_upload_ringbuffer_memory_pointer) +
                                perdrawcall_dynamic_offset));
                        for (uint32_t i = 0; i < current_instance_count; ++i)
                        {
                            perdrawcall_storage_buffer_object.mesh_instances[i].model_matrix =
                                mesh_nodes[drawcall_max_instance_count * drawcall_index + i].model_matrix;
                            perdrawcall_storage_buffer_object.mesh_instances[i].enable_vertex_blending =
                                mesh_nodes[drawcall_max_instance_count * drawcall_index + i].enable_vertex_blending ?
                                    1.0 :
                                    -1.0;
                        }

                        // per drawcall vertex blending storage buffer
                        uint32_t per_drawcall_vertex_blending_dynamic_offset;
                        bool     least_one_enable_vertex_blending = true;
                        for (uint32_t i = 0; i < current_instance_count; ++i)
                        {
                            if (!mesh_nodes[drawcall_max_instance_count * drawcall_index + i].enable_vertex_blending)
                            {
                                least_one_enable_vertex_blending = false;
                                break;
                            }
                        }
                        if (mesh.enable_vertex_blending)
                        {
                            per_drawcall_vertex_blending_dynamic_offset = roundUp(
                                _storage_buffer._global_upload_ringbuffers_end[m_command_info._current_frame_index],
                                _storage_buffer._min_storage_buffer_offset_alignment);
                            _storage_buffer._global_upload_ringbuffers_end[m_command_info._current_frame_index] =
                                per_drawcall_vertex_blending_dynamic_offset +
                                sizeof(MeshPointLightShadowPerdrawcallVertexBlendingStorageBufferObject);
                            assert(
                                _storage_buffer._global_upload_ringbuffers_end[m_command_info._current_frame_index] <=
                                (_storage_buffer._global_upload_ringbuffers_begin[m_command_info._current_frame_index] +
                                 _storage_buffer._global_upload_ringbuffers_size[m_command_info._current_frame_index]));

                            MeshPointLightShadowPerdrawcallVertexBlendingStorageBufferObject&
                                per_drawcall_vertex_blending_storage_buffer_object =
                                    (*reinterpret_cast<
                                        MeshPointLightShadowPerdrawcallVertexBlendingStorageBufferObject*>(
                                        reinterpret_cast<uintptr_t>(
                                            _storage_buffer._global_upload_ringbuffer_memory_pointer) +
                                        per_drawcall_vertex_blending_dynamic_offset));
                            for (uint32_t i = 0; i < current_instance_count; ++i)
                            {
                                if (mesh_nodes[drawcall_max_instance_count * drawcall_index + i].enable_vertex_blending)
                                {
                                    for (uint32_t j = 0; j < m_mesh_vertex_blending_max_joint_count; ++j)
                                    {
                                        per_drawcall_vertex_blending_storage_buffer_object
                                            .joint_matrices[m_mesh_vertex_blending_max_joint_count * i + j] =
                                            mesh_nodes[drawcall_max_instance_count * drawcall_index + i]
                                                .joint_matrices[j];
                                    }
                                }
                            }
                        }
                        else
                        {
                            per_drawcall_vertex_blending_dynamic_offset = 0;
                        }

                        // bind perdrawcall
                        uint32_t dynamic_offsets[3] = {perframe_dynamic_offset,
                                                       perdrawcall_dynamic_offset,
                                                       per_drawcall_vertex_blending_dynamic_offset};
                        m_p_vulkan_context->_vkCmdBindDescriptorSets(command_buffer,
                                                                     VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                                     _render_pipelines[0].layout,
                                                                     0,
                                                                     1,
                                                                     &_descriptor_infos[0].descriptor_set,
                                                                     std::size(dynamic_offsets),
                                                                     dynamic_offsets);

                        m_p_vulkan_context->_vkCmdDrawIndexed(
                            command_buffer, mesh.mesh_index_count, current_instance_count, 0, 0, 0);
                    }
                }
            }

            if (m_render_config._enable_debug_untils_label)
            {
                m_p_vulkan_context->_vkCmdEndDebugUtilsLabelEXT(command_buffer);
            }
        }

        m_p_vulkan_context->_vkCmdEndRenderPass(command_buffer);
    }

} // namespace Pilot
