#include "pch.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <optional>
#include <algorithm>
#include <set>


const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
	"VK_KHR_SWAPCHAIN_EXTENSION_NAME"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


bool checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<const char*> getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
	uint32_t count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

	std::vector<VkExtensionProperties> available(count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());

	std::set required(deviceExtensions.begin(), deviceExtensions.end());

	for (auto &ext : available)
		required.erase(ext.extensionName);

	return required.empty();
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	
	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};


void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, callback, pAllocator);
}

class HelloTriangleApplication {
public:
	void run() {
		mainLoop();
	}

	HelloTriangleApplication() {
		initVulkan();
	}

	virtual ~HelloTriangleApplication() {
		cleanup();
	}

private:
	void initVulkan() {
		createWindow();
		createInstance();
		setupDebugCallback();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
	}

	void createWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		this->window = glfwCreateWindow(800, 600, "Vulkan Test", nullptr, nullptr);
	}

	void createInstance() {

		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine, Yet";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
		appInfo.pNext = nullptr;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}
		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		std::cout << "available extensions:" << std::endl;
		for (const auto &extension : extensions)
			std::cout << "\t" << extension << std::endl;

		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	}

	void setupDebugCallback() {
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug callback!");
		}
	}

	void pickPhysicalDevice() {
		uint32_t count = 0;
		vkEnumeratePhysicalDevices(instance, &count, nullptr);

		if (!count)
			throw std::runtime_error("failed to find GPUs with Vulkan support.");

		std::vector<VkPhysicalDevice> devices(count);
		std::vector<VkPhysicalDeviceProperties> deviceProperties;
		vkEnumeratePhysicalDevices(instance, &count, devices.data());

		std::cout << count << " device(s) available:" << std::endl;
		std::optional<VkPhysicalDevice> selected;
		for (const VkPhysicalDevice &device : devices) {
			VkPhysicalDeviceProperties p = {};
			vkGetPhysicalDeviceProperties(device, &p);
			std::cout << "Name: " << p.deviceName << std::endl;
			deviceProperties.push_back(p);
			if (suitable(device))
				selected = device;
		}

		if (selected.has_value())
			physicalDevice = selected.value();
		else
			throw std::runtime_error("No suitable GPU found.");

		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		std::cout << "Picked " << properties.deviceName << std::endl;
		std::cout << "ID: " << properties.deviceID << std::endl;
		std::cout << "Driver version: " << properties.driverVersion << std::endl;
		std::cout << "API version: " << properties.apiVersion << std::endl;

	}

	QueueFamilyIndices findQueueFamilyIndices(VkPhysicalDevice device) {
		QueueFamilyIndices indices;
		uint32_t count;

		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

		std::vector<VkQueueFamilyProperties> properties(count);

		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());

		for (uint32_t i = 0; i < count; i++) {
			if (properties[i].queueCount > 0 ) {
				if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indices.graphicsFamily = i;

				uint32_t presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, this->surface, &presentSupport);

				if (presentSupport)
					indices.presentFamily = i;
			}
			if (indices.isComplete())
				break;
		}

		if (!indices.isComplete())
			throw std::runtime_error("Couldn't find all required queue families!");

		return indices;
	}

	void createLogicalDevice() {
		auto indices = this->findQueueFamilyIndices(physicalDevice);
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		for (auto i : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueInfo = {};
			queueInfo.queueFamilyIndex = i;
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueCount = 1;
			float priority = 1.0;
			queueInfo.pQueuePriorities = &priority;
			queueCreateInfos.push_back(queueInfo);
		}

		VkPhysicalDeviceFeatures features = {};
		
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &features;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Logical device creation failed!");

		std::cout << "Logical device successfully created." << std::endl;

		uint32_t index = indices.graphicsFamily.value();
		vkGetDeviceQueue(device, index, 0, &this->graphicsQueue);

		index = indices.presentFamily.value();
		vkGetDeviceQueue(device, index, 0, &this->presentQueue);
	}

	void createSurface() {
		VkResult result = glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Window surface creation failed!");
		std::cout << "Window surface successfully created." << std::endl;
	}

	SwapChainSupportDetails querySwapChainSupport() {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

		uint32_t pmCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &pmCount, nullptr);
		details.presentModes.resize(pmCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &pmCount, details.presentModes.data());

		uint32_t formatsCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr);
		details.formats.resize(formatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, details.formats.data());

		return details;
	}

	bool suitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilyIndices(device);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		return indices.isComplete() && extensionsSupported;
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		vkDestroyDevice(device, nullptr);
		std::cout << "Logical device destroyed." << std::endl;
		vkDestroySurfaceKHR(instance, surface, nullptr);
		std::cout << "Window surface destroyed." << std::endl;

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
			std::cout << "Debug layer destroyed." << std::endl;
		}

		vkDestroyInstance(instance, nullptr);
		std::cout << "Vulkan instance destroyed." << std::endl;
		glfwDestroyWindow(window);
		std::cout << "GLFW window destroyed." << std::endl;
		glfwTerminate();
		std::cout << "GLFW terminated." << std::endl;
	}

	GLFWwindow *window;
	VkInstance instance;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT callback;
};


int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
