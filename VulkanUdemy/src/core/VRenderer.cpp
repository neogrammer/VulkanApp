#include "VRenderer.h"


VkResult VRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMsnger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMsnger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

PFN_vkDebugUtilsMessengerCallbackEXT __stdcall VRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagBitsEXT messageTypes, VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	return PFN_vkDebugUtilsMessengerCallbackEXT();
}

void VRenderer::createInstance()
{
	std::vector<const char*>* supportLayers = { };
	supportLayers->push_back("VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT");


	if (enableValidationLayers && !checkValidationLayerSupport(*supportLayers)) {
		throw std::runtime_error("Validation Layers requested, no fucks given");
	}

	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pEngineName = "Cidgine";
	appInfo.apiVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
	appInfo.pApplicationName = "Vulkan Test App";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);


	VkInstanceCreateInfo iinfo;
	iinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	iinfo.pNext = nullptr;
	iinfo.pApplicationInfo = &appInfo;

	//std::vector<const char*> instanceExtensions = std::vector<const char*>();

	//std::vector<const char*> instanceLayers = std::vector<const char*>();
	////uint32_t glfwExtensionCount = 0;



	//const char** glfwExtensions; // Extensions passed as array of cstrings, so need pointer




	//glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	

	// Add glfw extensions to list
	//for (size_t i = 0; i < glfwExtensionCount; i++)
	//{
	//	instanceExtensions.push_back(glfwExtensions[i]);
	//}
	//
	

	uint32_t extenCount = 0;
	std::vector<const char*> exts = getRequiredExtensions();
	if (!checkInstanceExtensionSupport(exts))
	{
		throw std::runtime_error("vkInstance unable");
	}
	iinfo.enabledExtensionCount = static_cast<uint32_t>(exts.size());
	iinfo.ppEnabledExtensionNames = exts.data();
	//TODO//
	// Set up validation layers


	if (!checkValidationLayerSupport(validationLayers))
	{
		throw std::runtime_error("No layers");
	}

	iinfo.enabledLayerCount = 1;
	iinfo.ppEnabledLayerNames = validationLayers.data();

	VkResult res;

	res = vkCreateInstance(&iinfo, nullptr, &instance);

	if (res != VK_SUCCESS)
	{	
		throw std::runtime_error("Failed to create a Vulkan Instance!");
	}

	createLogicalDevice();
}

void VRenderer::createLogicalDevice()
{
	getGPU();
	QueueFamilyIndices indices = getQFams(mainDevice.gpu);

	VkDeviceQueueCreateInfo qi = {};
	qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	qi.queueFamilyIndex = static_cast<uint32_t>(indices[QFam::Gfx]);
	qi.queueCount = 1;
	float priority = 1.0f;
	qi.pQueuePriorities = &priority;



	VkDeviceCreateInfo di = { };
	di.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	di.queueCreateInfoCount = 1;
	di.pQueueCreateInfos = &qi;
	di.enabledExtensionCount = 0;
	di.ppEnabledExtensionNames = nullptr;

	VkPhysicalDeviceFeatures features = {};
	di.pEnabledFeatures = &features;

	VkResult result = vkCreateDevice(mainDevice.gpu, &di, nullptr, &mainDevice.device);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a logical Device!");
	}

	
	vkGetDeviceQueue(mainDevice.device, static_cast<uint32_t>(indices[QFam::Gfx]), 0, &gfxQ);


}

VkPhysicalDevice VRenderer::getGPU()
{
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(instance, &count, nullptr);

	if (count == 0)
	{
		throw std::runtime_error("Can't find a GPU that supports Vulkan nowheres!");
	}

	std::vector<VkPhysicalDevice> devices(count);
	vkEnumeratePhysicalDevices(instance, &count, devices.data());

	for (const auto& dev : devices)
	{

		if (checkDeviceCompat(dev))
		{
			mainDevice.gpu = dev;
			break;
		}
	}

}

QueueFamilyIndices VRenderer::getQFams(VkPhysicalDevice gpu_)
{
	QueueFamilyIndices indices;

	uint32_t qFamCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu_, &qFamCount, nullptr);

	std::vector<VkQueueFamilyProperties> qFamList(qFamCount);
	vkGetPhysicalDeviceQueueFamilyProperties(gpu_, &qFamCount, qFamList.data());

	int i = 0;
	for (const auto& qFam : qFamList)
	{
		if (qFam.queueCount > 0 && qFam.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			QFamilies famBucket;
			famBucket.gfxIdx = i;
			famBucket.gfxName = QFam::Gfx;

			indices[famBucket.gfxName] = famBucket.gfxIdx;
			if (famBucket.isValid())
			{
				break;
			}
		}

		i++;
	}
	return indices;
}

bool VRenderer::checkInstanceExtensionSupport(std::vector<const char*> checkExtensions)
{
	
	//uint32_t glfwExtensionCount = 0;
	//const char** glfwExtensions;
	//glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	//std::vector<const char*> exts(glfwExtensions, glfwExtensions + glfwExtensionCount);
	//if (enableValidationLayers)
	//{
	//	exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	//}

	


	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());


	bool hasExtension = false;

	// check if given extensions are in list of available ones
	for (const auto& checkExtension : checkExtensions)
	{

		for (const auto& extension : extensions)
		{
			if (strcmp(checkExtension, extension.extensionName))
			{
				hasExtension = true;
				break;
			}
		}

		
	}

	if (!hasExtension)
	{
		return false;
	}


	return true;
}

bool VRenderer::checkValidationLayerSupport(std::vector<const char*> checkLayers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers{ layerCount };
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


	for (const auto& checkLayer : checkLayers)
	{
		bool layerFound = false;

		for (const auto& layer : availableLayers)
		{
			if (strcmp(checkLayer, layer.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}

	}

	return true;
}

bool VRenderer::checkDeviceCompat(VkPhysicalDevice gpu_)
{
	//VkPhysicalDeviceProperties props;
	//vkGetPhysicalDeviceProperties(gpu_, &props);

	//VkPhysicalDeviceFeatures features;
	//vkGetPhysicalDeviceFeatures(gpu_, &features);



	return true;
}

void VRenderer::initVulkan()
{
	createInstance();
	setupDebugMessenger();
}
void VRenderer::setupDebugMessenger()
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT ci{};
	ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
	ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	//ci.pfnUserCallback = VRenderer::debugCallback;
	ci.pUserData = nullptr;
}
std::vector<const char*> VRenderer::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> exts(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers)
	{
		exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return exts;
}

VRenderer::VRenderer()
{
}

VRenderer::~VRenderer()
{
}

int VRenderer::init(GLFWwindow* window_)
{
	window = window_;

	try
	{
		createInstance();
	}
	catch (const std::runtime_error& e) {
		printf("Error: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return 0;
}

void VRenderer::cleanup()
{
	vkDestroyInstance(instance, nullptr);
}
