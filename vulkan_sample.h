/* Copyright (c) 2019-2025, Arm Limited and Contributors
 * Copyright (c) 2021-2025, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Volk/volk.h"

#include "scene/utils.h"
#include "common/gltf_loader.h"
#include "common/gui.h"
#include "common/strings.h"
#include "platform/application.h"
#include "platform/configuration.h"
#include "rendering/render_pipeline.h"
#include "scene/components/camera/camera.h"
#include "scene/scene.h"
#include "scene/scripts/animation.h"

#if defined(PLATFORM__MACOS)
#	include <TargetConditionals.h>
#endif

namespace frame {
	namespace gui {
		class Gui;
	}

	namespace core {
		class CommandBuffer;
		class DebugUtils;
		class Device;
		class Instance;
		class PhysicalDevice;
	}

	namespace rendering {
		class RenderContext;
		class RenderTarget;
		class RenderPipeline;
	}

	namespace stats {
		class Stats;
	}

	class VulkanSample : public platform::Application {
		using Parent = Application;

	public:
		VulkanSample() = default;
		~VulkanSample() override;

		platform::Configuration& getConfiguration();
		rendering::RenderContext& getRenderContext();
		rendering::RenderContext const& getRenderContext() const;
		bool hasRenderContext() const;

	protected:
		void inputEvent(const platform::InputEvent& input_event) override;
		void finish() override;
		bool resize(uint32_t width, uint32_t height) override;

		virtual std::unique_ptr<core::Device> createDevice(core::PhysicalDevice& gpu);
		virtual std::unique_ptr<core::Instance> createInstance();
		virtual void createRenderContext();
		virtual void draw(core::CommandBuffer& command_buffer, rendering::RenderTarget& render_target);
		virtual void drawGui();
		virtual void drawRenderpass(core::CommandBuffer& command_buffer, rendering::RenderTarget& render_target);
		virtual void prepareRenderContext();
		virtual void render(core::CommandBuffer& command_buffer);
		virtual void requestGpuFeatures(core::PhysicalDevice& gpu);
		virtual void resetStatsView();
		virtual void updateDebugWindow();

		void addDeviceExtension(const char* extension, bool optional = false);
		void addInstanceExtension(const char* extension, bool optional = false);
		void addInstanceLayer(const char* layer, bool optional = false);
		void addLayerSetting(vk::LayerSettingEXT const& layerSetting);
		void createGui(const platform::Window& m_window, stats::Stats const* stats = nullptr, const float font_size = 21.0f, bool explicit_update = false);
		void createRenderContext(const std::vector<vk::SurfaceFormatKHR>& surface_priority_list);

		core::Device& getDevice();
		core::Device const& getDevice() const;
		gui::Gui& getGui();
		gui::Gui const& getGui() const;
		core::Instance& getInstance();
		core::Instance const& getInstance() const;
		rendering::RenderPipeline& getRenderPipeline();
		rendering::RenderPipeline const& getRenderPipeline() const;
		scene::Scene& getScene();
		stats::Stats& getStats();
		vk::SurfaceKHR getSurface() const;
		std::vector<vk::SurfaceFormatKHR>& getSurfacePriorityList();
		std::vector<vk::SurfaceFormatKHR> const& getSurfacePriorityList() const;
		bool hasDevice() const;
		bool hasInstance() const;
		bool hasGui() const;
		bool hasRenderPipeline() const;
		bool hasScene();

		void loadScene(const std::string& path);
		bool prepare(const platform::ApplicationOptions& options) override;
		void setApiVersion(uint32_t requested_api_version);
		void setHighPriorityGraphicsQueueEnable(bool enable);
		void setRenderContext(std::unique_ptr<rendering::RenderContext>&& render_context);
		void setRenderPipeline(std::unique_ptr<rendering::RenderPipeline>&& render_pipeline);
		void update(float delta_time) override;
		void updateGui(float delta_time);
		void updateScene(float delta_time);
		void updateStats(float delta_time);
		static void setViewportAndScissor(core::CommandBuffer const& command_buffer, vk::Extent2D const& extent);

	private:
		std::unordered_map<const char*, bool> const& getDeviceExtensions() const;
		std::unordered_map<const char*, bool> const& getInstanceExtensions() const;
		std::unordered_map<const char*, bool> const& getInstanceLayers() const;
		std::vector<vk::LayerSettingEXT> const& getLayerSettings() const;

	private:
		std::unique_ptr<core::Instance> m_instance;
		std::unique_ptr<core::Device> m_device;
		std::unique_ptr<rendering::RenderContext> m_render_context;
		std::unique_ptr<rendering::RenderPipeline> m_render_pipeline;
		std::unique_ptr<scene::Scene> m_scene;
		std::unique_ptr<gui::Gui> m_gui;
		std::unique_ptr<stats::Stats> m_stats;

		static constexpr float STATS_VIEW_RESET_TIME{ 10.0f };

		vk::SurfaceKHR m_surface;
		std::vector<vk::SurfaceFormatKHR> m_surface_priority_list = {
			{vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
			{vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear} };

		platform::Configuration m_configuration{};
		std::unordered_map<const char*, bool> m_device_extensions;
		std::unordered_map<const char*, bool> m_instance_extensions;
		std::unordered_map<const char*, bool> m_instance_layers;
		std::vector<vk::LayerSettingEXT> m_layer_settings;
		uint32_t m_api_version = VK_API_VERSION_1_3;
		bool m_high_priority_graphics_queue{ false };

		std::unique_ptr<core::DebugUtils> m_debug_utils;
	};

	inline VulkanSample::~VulkanSample() {
		if(m_device) {
			m_device->getHandle().waitIdle();
		}

		m_scene.reset();
		m_stats.reset();
		m_gui.reset();
		m_render_context.reset();
		m_device.reset();

		if(m_surface) {
			m_instance->getHandle().destroySurfaceKHR(m_surface);
		}

		m_instance.reset();
	}

	inline void VulkanSample::addDeviceExtension(const char* extension, bool optional) {
		m_device_extensions[extension] = optional;
	}

	inline void VulkanSample::addInstanceExtension(const char* extension, bool optional) {
		m_instance_extensions[extension] = optional;
	}

	inline void VulkanSample::addLayerSetting(vk::LayerSettingEXT const& layerSetting) {
		m_layer_settings.push_back(layerSetting);
	}

	inline std::unique_ptr<core::Device> VulkanSample::createDevice(core::PhysicalDevice& gpu) {
		return std::make_unique<core::Device>(gpu, m_surface, std::move(m_debug_utils), getDeviceExtensions());
	}

	inline std::unique_ptr<core::Instance> VulkanSample::createInstance() {
		return std::make_unique<core::Instance>(getName(), getInstanceExtensions(), getInstanceLayers(), getLayerSettings(), m_api_version);
	}

	inline void VulkanSample::createRenderContext() {
		createRenderContext(m_surface_priority_list);
	}

	inline void VulkanSample::createRenderContext(const std::vector<vk::SurfaceFormatKHR>& surface_priority_list) {
#ifdef VK_USE_PLATFORM_ANDROID_KHR
		vk::PresentModeKHR present_mode = (m_window->getProperties().vsync == Window::Vsync::OFF) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
		std::vector<vk::PresentModeKHR> present_mode_priority_list{ vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate };
#else
		vk::PresentModeKHR present_mode = (m_window->getProperties().vsync == platform::Window::Vsync::On) ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox;
		std::vector<vk::PresentModeKHR> present_mode_priority_list{ vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate, vk::PresentModeKHR::eFifo };
#endif

		m_render_context = std::make_unique<rendering::RenderContext>(*m_device, m_surface, *m_window, present_mode, present_mode_priority_list, surface_priority_list);
	}

	inline void VulkanSample::draw(core::CommandBuffer& command_buffer, rendering::RenderTarget& render_target) {
		auto& views = render_target.getViews();
		
		{
			common::ImageMemoryBarrier color_barrier{};
			color_barrier.m_old_layout = vk::ImageLayout::eUndefined;
			color_barrier.m_new_layout = vk::ImageLayout::eColorAttachmentOptimal;
			color_barrier.m_src_access_mask = {};
			color_barrier.m_dst_access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
			color_barrier.m_src_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			color_barrier.m_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			
			command_buffer.imageMemoryBarrier(views[1], color_barrier);
			render_target.setLayout(1, color_barrier.m_new_layout);
			
			for (size_t i = 2; i < views.size(); ++i) {
				command_buffer.imageMemoryBarrier(views[i], color_barrier);
				render_target.setLayout(static_cast<size_t>(i), color_barrier.m_new_layout);
			}
		}
		
		{
			common::ImageMemoryBarrier depth_barrier{};
			depth_barrier.m_old_layout = vk::ImageLayout::eUndefined;
			depth_barrier.m_new_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			depth_barrier.m_src_access_mask = {};
			depth_barrier.m_dst_access_mask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
				vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			depth_barrier.m_src_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
			depth_barrier.m_dst_stage_mask = vk::PipelineStageFlagBits::eEarlyFragmentTests |
				vk::PipelineStageFlagBits::eLateFragmentTests;
			
			command_buffer.imageMemoryBarrier(views[0], depth_barrier);
			render_target.setLayout(0, depth_barrier.m_new_layout);
		}
		
		drawRenderpass(command_buffer, render_target);
		
		{
			common::ImageMemoryBarrier present_barrier{};
			present_barrier.m_old_layout = vk::ImageLayout::eColorAttachmentOptimal;
			present_barrier.m_new_layout = vk::ImageLayout::ePresentSrcKHR;
			present_barrier.m_src_access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
			present_barrier.m_src_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			present_barrier.m_dst_stage_mask = vk::PipelineStageFlagBits::eBottomOfPipe;
			
			command_buffer.imageMemoryBarrier(views[1], present_barrier);
			render_target.setLayout(1, present_barrier.m_new_layout);
		}
	}

	inline void VulkanSample::drawGui() {}

	inline void VulkanSample::drawRenderpass(core::CommandBuffer& command_buffer, rendering::RenderTarget& render_target) {
		setViewportAndScissor(command_buffer, render_target.getExtent());
		render(command_buffer);

		if(m_gui) {
			m_gui->draw(command_buffer);
		}

		command_buffer.getHandle().endRenderPass();
	}

	inline void VulkanSample::finish() {
		Parent::finish();

		if(m_device) {
			m_device->getHandle().waitIdle();
		}
	}

	inline platform::Configuration& VulkanSample::getConfiguration() {
		return m_configuration;
	}

	inline core::Device const& VulkanSample::getDevice() const {
		return *m_device;
	}

	inline core::Device& VulkanSample::getDevice() {
		return *m_device;
	}

	inline std::unordered_map<const char*, bool> const& VulkanSample::getDeviceExtensions() const {
		return m_device_extensions;
	}

	inline gui::Gui& VulkanSample::getGui() {
		return *m_gui;
	}

	inline gui::Gui const& VulkanSample::getGui() const {
		return *m_gui;
	}

	inline std::vector<vk::SurfaceFormatKHR>& VulkanSample::getSurfacePriorityList() {
		return m_surface_priority_list;
	}

	inline std::vector<vk::SurfaceFormatKHR> const& VulkanSample::getSurfacePriorityList() const {
		return m_surface_priority_list;
	}

	inline core::Instance& VulkanSample::getInstance() {
		return *m_instance;
	}

	inline core::Instance const& VulkanSample::getInstance() const {
		return *m_instance;
	}

	inline std::unordered_map<const char*, bool> const& VulkanSample::getInstanceExtensions() const {
		return m_instance_extensions;
	}

	inline std::unordered_map<const char*, bool> const& VulkanSample::getInstanceLayers() const {
		return m_instance_layers;
	}

	inline void VulkanSample::addInstanceLayer(const char* layer, bool optional) {
		m_instance_layers[layer] = optional;
	}

	inline std::vector<vk::LayerSettingEXT> const& VulkanSample::getLayerSettings() const {
		return m_layer_settings;
	}

	inline rendering::RenderContext const& VulkanSample::getRenderContext() const {
		assert(m_render_context && "Render context is not valid");
		return *m_render_context;
	}

	inline rendering::RenderContext& VulkanSample::getRenderContext() {
		assert(m_render_context && "Render context is not valid");
		return *m_render_context;
	}

	inline rendering::RenderPipeline const& VulkanSample::getRenderPipeline() const {
		assert(m_render_pipeline && "Render pipeline was not created");
		return *m_render_pipeline;
	}

	inline rendering::RenderPipeline& VulkanSample::getRenderPipeline() {
		assert(m_render_pipeline && "Render pipeline was not created");
		return *m_render_pipeline;
	}

	inline scene::Scene& VulkanSample::getScene() {
		assert(m_scene && "Scene not loaded");
		return *m_scene;
	}

	inline stats::Stats& VulkanSample::getStats() {
		return *m_stats;
	}

	inline vk::SurfaceKHR VulkanSample::getSurface() const {
		return m_surface;
	}

	inline bool VulkanSample::hasDevice() const {
		return m_device != nullptr;
	}

	inline bool VulkanSample::hasInstance() const {
		return m_instance != nullptr;
	}

	inline bool VulkanSample::hasGui() const {
		return m_gui != nullptr;
	}

	inline bool VulkanSample::hasRenderContext() const {
		return m_render_context != nullptr;
	}

	inline bool VulkanSample::hasRenderPipeline() const {
		return m_render_pipeline != nullptr;
	}

	inline bool VulkanSample::hasScene() {
		return m_scene != nullptr;
	}

	inline void VulkanSample::inputEvent(const platform::InputEvent& input_event) {
		Parent::inputEvent(input_event);

		bool gui_captures_event = false;

		if(m_gui) {
			gui_captures_event = m_gui->inputEvent(input_event);
		}

		if(!gui_captures_event) {
			if(m_scene && m_scene->hasComponent<scene::Script>()) {
				auto scripts = m_scene->getComponents<scene::Script>();

				for (auto script : scripts) {
					script->inputEvent(input_event);
				}
			}
		}

		if(input_event.getSource() == platform::EventSource::Keyboard) {
			const auto& key_event = static_cast<const platform::KeyInputEvent&>(input_event);
			if(key_event.getAction() == platform::KeyAction::Down &&
				(key_event.getCode() == platform::KeyCode::PrintScreen || key_event.getCode() == platform::KeyCode::F12))
			{
				scene::screenShot(*m_render_context, "screenshot-" + getName());
			}
		}
	}

	inline void VulkanSample::loadScene(const std::string& path) {
		common::GltfLoader loader(*m_device);
		m_scene = loader.readSceneFromFile(path);

		if(!m_scene) {
			LOGE("Cannot load scene: {}", path.c_str());
			throw std::runtime_error("Cannot load scene: " + path);
		}

		m_scene->addNode(std::make_unique<scene::Node>(0, "root node"));
		m_scene->setRootNode(*m_scene->findNode("root node"));
	}

	inline bool VulkanSample::prepare(const platform::ApplicationOptions& options) {
		if(!Parent::prepare(options)) {
			return false;
		}

		LOGI("Initializing Vulkan sample");

#if defined(_HPP_VULKAN_LIBRARY)
		static vk::DynamicLoader dl(_HPP_VULKAN_LIBRARY);
#else
		static vk::DynamicLoader dl;
#endif
		VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
		
		VkResult result = volkInitialize();
		if(result) {
			throw std::runtime_error("Failed to initialize volk");
		}

		for (const char* extension_name : m_window->getRequiredSurfaceExtensions()) {
			addInstanceExtension(extension_name);
		}

#ifdef VK_DEBUG
		{
			std::vector<vk::ExtensionProperties> available_instance_extensions = vk::enumerateInstanceExtensionProperties();
			auto debugExtensionIt = std::find_if(available_instance_extensions.begin(),
				available_instance_extensions.end(),
				[](vk::ExtensionProperties const& ep) { return strcmp(ep.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0; });
			if(debugExtensionIt != available_instance_extensions.end())
			{
				LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

				m_debug_utils = std::make_unique<core::DebugUtilsExtDebugUtils>();
				addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
		}
#endif

		m_instance = createInstance();

		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance->getHandle());

		m_surface = m_window->createSurface(*m_instance);

		if(!m_surface) {
			throw std::runtime_error("Failed to create m_window surface.");
		}

		auto& gpu = m_instance->getSuitablePhysicalDevice(m_surface);
		gpu.setHighPriorityGraphicsQueueEnable(m_high_priority_graphics_queue);

		if(gpu.getFeatures().textureCompressionASTC_LDR) {
			gpu.getMutableRequestedFeatures().textureCompressionASTC_LDR = true;
		}

		requestGpuFeatures(gpu);

		{
			addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

			if(m_instance_extensions.find(VK_KHR_DISPLAY_EXTENSION_NAME) != m_instance_extensions.end()) {
				addDeviceExtension(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME, /*optional=*/true);
			}
		}

#ifdef VK_ENABLE_PORTABILITY
		addDeviceExtension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, /*optional=*/true);
#endif

#ifdef VK_DEBUG
		if(!m_debug_utils) {

			std::vector<vk::ExtensionProperties> available_device_extensions = gpu.getHandle().enumerateDeviceExtensionProperties();

			auto debugExtensionIt = std::find_if(available_device_extensions.begin(), available_device_extensions.end(),
				[](vk::ExtensionProperties const& ep) { return strcmp(ep.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0; });

			if(debugExtensionIt != available_device_extensions.end()) {
				LOGI("Vulkan debug utils enabled ({})", VK_EXT_DEBUG_MARKER_EXTENSION_NAME);

				m_debug_utils = std::make_unique<core::DebugMarkerExtDebugUtils>();
				addDeviceExtension(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
			}
		}

		if(!m_debug_utils) {
			LOGW("Vulkan debug utils were requested, but no extension that provides them was found");
		}
#endif

		if(!m_debug_utils) {
			m_debug_utils = std::make_unique<core::DummyDebugUtils>();
		}
		
		m_device = createDevice(gpu);

		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device->getHandle());
		
		createRenderContext();
		prepareRenderContext();

		m_stats = std::make_unique<stats::Stats>(*m_render_context);

		m_configuration.reset();

		return true;
	}

	inline void VulkanSample::createGui(const platform::Window& window, stats::Stats const* stats, const float font_size, bool explicit_update) {
		m_gui = std::make_unique<gui::Gui>(*this, window, stats, font_size, explicit_update);
	}

	inline void VulkanSample::prepareRenderContext() {
		m_render_context->prepare();
	}

	inline void VulkanSample::render(core::CommandBuffer& command_buffer) {
		if(m_render_pipeline) {
			m_render_pipeline->draw(command_buffer, m_render_context->getActiveFrame().getRenderTarget());
		}
	}

	inline void VulkanSample::requestGpuFeatures(core::PhysicalDevice& gpu) {}

	inline void VulkanSample::resetStatsView() {}

	inline bool VulkanSample::resize(uint32_t width, uint32_t height) {
		if (m_device) {
			m_device->getHandle().waitIdle();
		}

		if (m_render_context && m_render_context->hasSwapchain()) {
			m_render_context->updateSwapchain(vk::Extent2D{ width, height });
		}
		
		if(m_gui) {
			m_gui->resize(width, height);
		}

		if(m_scene && m_scene->hasComponent<scene::Script>()) {
			auto scripts = m_scene->getComponents<scene::Script>();

			for (auto script : scripts) {
				script->resize(width, height);
			}
		}

		if(m_stats) {
			m_stats->resize(width);
		}

		if (m_scene && m_scene->hasComponent<scene::Camera>()) {
			auto cameras = m_scene->getComponents<scene::Camera>();

			for (auto camera : cameras) {
				float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
				camera->setAspectRatio(aspect_ratio);
			}
		}

		getDebugInfo().insert<common::field::Static, std::string>("resolution",
			common::toString(width) + "x" + common::toString(height));

		return true;
	}

	inline void VulkanSample::setApiVersion(uint32_t requested_api_version) {
		m_api_version = requested_api_version;
	}

	inline void VulkanSample::setHighPriorityGraphicsQueueEnable(bool enable) {
		m_high_priority_graphics_queue = enable;
	}

	inline void VulkanSample::setRenderContext(std::unique_ptr<rendering::RenderContext>&& rc) {
		m_render_context = std::move(rc);
	}

	inline void VulkanSample::setRenderPipeline(std::unique_ptr<rendering::RenderPipeline>&& rp) {
		m_render_pipeline = std::move(rp);
	}

	inline void VulkanSample::setViewportAndScissor(core::CommandBuffer const& command_buffer, vk::Extent2D const& extent) {
		command_buffer.getHandle().setViewport(0, { {0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f} });
		command_buffer.getHandle().setScissor(0, vk::Rect2D({}, extent));
	}

	inline void VulkanSample::update(float delta_time) {
		Application::update(delta_time);

		updateScene(delta_time);
		updateGui(delta_time);

		auto& command_buffer = m_render_context->begin();
		updateStats(delta_time);

		command_buffer.begin(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		m_stats->beginSampling(command_buffer);

		draw(command_buffer, m_render_context->getActiveFrame().getRenderTarget());

		m_stats->endSampling(command_buffer);
		command_buffer.end();

		m_render_context->submit(command_buffer);
	}

	inline void VulkanSample::updateDebugWindow() {
		auto driver_version = m_device->getPhysicalDevice().getDriverVersion();
		std::string driver_version_str = fmt::format("major: {} minor: {} patch: {}", driver_version.major, driver_version.minor, driver_version.patch);

		getDebugInfo().insert<common::field::Static, std::string>("driver_version", driver_version_str);
		getDebugInfo().insert<common::field::Static, std::string>("resolution",
			common::toString(m_render_context->getSwapchain().getExtent()));
		getDebugInfo().insert<common::field::Static, std::string>("surface_format",
			common::toString(m_render_context->getSwapchain().getFormat()) + " (" +
			common::toString(common::getBitsPerPixel(m_render_context->getSwapchain().getFormat())) + "bpp)");

		if(m_scene != nullptr) {
			getDebugInfo().insert<common::field::Static, uint32_t>("mesh_count", common::toU32(m_scene->getComponents<scene::SubMesh>().size()));
			getDebugInfo().insert<common::field::Static, uint32_t>("texture_count", common::toU32(m_scene->getComponents<scene::Texture>().size()));

			if(auto camera = m_scene->getComponents<scene::Camera>()[0]) {
				if(auto camera_node = camera->getNode()) {
					const glm::vec3& pos = camera_node->getTransform().getTranslation();
					getDebugInfo().insert<common::field::Vector, float>("camera_pos", pos.x, pos.y, pos.z);
				}
			}
		}
	}

	inline void VulkanSample::updateGui(float delta_time) {
		if(m_gui) {
			if(m_gui->isDebugViewActive()) {
				updateDebugWindow();
			}

			m_gui->newFrame();
			m_gui->showTopWindow(getName(), m_stats.get(), &getDebugInfo());
			drawGui();
			m_gui->update(delta_time);
		}
	}

	inline void VulkanSample::updateScene(float delta_time) {
		if(m_scene) {
			if(m_scene->hasComponent<scene::Script>()) {
				auto scripts = m_scene->getComponents<scene::Script>();

				for (auto script : scripts) {
					script->update(delta_time);
				}
			}

			if(m_scene->hasComponent<scene::Animation>()) {
				auto animations = m_scene->getComponents<scene::Animation>();

				for (auto animation : animations) {
					animation->update(delta_time);
				}
			}
		}
	}

	inline void VulkanSample::updateStats(float delta_time) {
		if(m_stats) {
			m_stats->update(delta_time);

			static float stats_view_count = 0.0f;
			stats_view_count += delta_time;

			if(stats_view_count > STATS_VIEW_RESET_TIME) {
				resetStatsView();
				stats_view_count = 0.0f;
			}
		}
	}

}