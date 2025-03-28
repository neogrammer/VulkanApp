#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <vector>
#include "Utilities.h"
#include <cstring>


const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG	
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


class VRenderer
{
	GLFWwindow*  window;

	VkInstance instance;
	VkDebugUtilsMessengerEXT msnger;
	struct {
		VkPhysicalDevice gpu;
		VkDevice device;
	} mainDevice;
	VkQueue gfxQ;


	PFN_vkDebugUtilsMessengerCallbackEXT __stdcall debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagBitsEXT messageTypes, VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);


	// Vulkan Functions
	//Create Functions
	void createInstance();
	void createLogicalDevice();

	//Getters
	VkPhysicalDevice getGPU();
	QueueFamilyIndices getQFams(VkPhysicalDevice gpu_);
	

	//Support Functions
	bool checkInstanceExtensionSupport(std::vector<const char*> checkExtensions);
	bool checkValidationLayerSupport(std::vector<const char*> checkLayers);
	bool checkDeviceCompat(VkPhysicalDevice gpu_);
	void initVulkan(); 

	void setupDebugMessenger();
	std::vector<const char*> getRequiredExtensions();
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMsnger);

public:
	VRenderer();
	~VRenderer();

	int init(GLFWwindow* window_);
	void cleanup();
};