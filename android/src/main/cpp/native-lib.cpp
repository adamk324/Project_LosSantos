#include <jni.h>
#include <android/log.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <exception>
#include <mutex>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "runtime/memory/MemoryArena.h"

namespace {

constexpr char kLogTag[] = "ProjectLosSantos";
std::mutex g_mutex;
VkInstance g_instance = VK_NULL_HANDLE;
bool g_initialized = false;
bool g_runtimeCoreInitialized = false;
std::string g_vulkanJson = R"({"available":false,"error":"Not initialized"})";

std::string jsonEscape(const std::string& value) {
    std::string escaped;
    for (const unsigned char ch : value) {
        switch (ch) {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                if (ch >= 0x20u) {
                    escaped += static_cast<char>(ch);
                }
                break;
        }
    }
    return escaped;
}

std::string vkResultMessage(const char* operation, VkResult result) {
    return std::string(operation) + " failed with VkResult " + std::to_string(result);
}

std::string inspectVulkan() {
    uint32_t instanceVersion = VK_API_VERSION_1_0;
    const VkResult versionResult = vkEnumerateInstanceVersion(&instanceVersion);
    if (versionResult != VK_SUCCESS) {
        return "{\"available\":false,\"error\":\"" +
               jsonEscape(vkResultMessage("vkEnumerateInstanceVersion", versionResult)) + "\"}";
    }

    VkApplicationInfo applicationInfo{};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "Project_Los Santos";
    applicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    applicationInfo.pEngineName = "Project_Los Santos bring-up";
    applicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    applicationInfo.apiVersion = instanceVersion;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &applicationInfo;

    const VkResult createResult = vkCreateInstance(&createInfo, nullptr, &g_instance);
    if (createResult != VK_SUCCESS) {
        g_instance = VK_NULL_HANDLE;
        return "{\"available\":false,\"apiVersion\":\"" +
               std::to_string(VK_API_VERSION_MAJOR(instanceVersion)) + "." +
               std::to_string(VK_API_VERSION_MINOR(instanceVersion)) + "." +
               std::to_string(VK_API_VERSION_PATCH(instanceVersion)) +
               "\",\"error\":\"" + jsonEscape(vkResultMessage("vkCreateInstance", createResult)) + "\"}";
    }

    uint32_t physicalDeviceCount = 0;
    const VkResult countResult = vkEnumeratePhysicalDevices(g_instance, &physicalDeviceCount, nullptr);
    if (countResult != VK_SUCCESS || physicalDeviceCount == 0u) {
        const std::string error = countResult == VK_SUCCESS
                                      ? "No Vulkan physical devices were reported"
                                      : vkResultMessage("vkEnumeratePhysicalDevices", countResult);
        vkDestroyInstance(g_instance, nullptr);
        g_instance = VK_NULL_HANDLE;
        return "{\"available\":false,\"apiVersion\":\"" +
               std::to_string(VK_API_VERSION_MAJOR(instanceVersion)) + "." +
               std::to_string(VK_API_VERSION_MINOR(instanceVersion)) + "." +
               std::to_string(VK_API_VERSION_PATCH(instanceVersion)) +
               "\",\"error\":\"" + jsonEscape(error) + "\"}";
    }

    std::vector<VkPhysicalDevice> devices(physicalDeviceCount);
    const VkResult devicesResult =
        vkEnumeratePhysicalDevices(g_instance, &physicalDeviceCount, devices.data());
    if (devicesResult != VK_SUCCESS) {
        vkDestroyInstance(g_instance, nullptr);
        g_instance = VK_NULL_HANDLE;
        return "{\"available\":false,\"error\":\"" +
               jsonEscape(vkResultMessage("vkEnumeratePhysicalDevices", devicesResult)) + "\"}";
    }

    std::ostringstream json;
    json << "{\"available\":true,\"apiVersion\":\""
         << VK_API_VERSION_MAJOR(instanceVersion) << '.'
         << VK_API_VERSION_MINOR(instanceVersion) << '.'
         << VK_API_VERSION_PATCH(instanceVersion) << "\",\"physicalDevices\":[";
    for (uint32_t deviceIndex = 0; deviceIndex < physicalDeviceCount; ++deviceIndex) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(devices[deviceIndex], &properties);

        uint32_t extensionCount = 0;
        VkResult extensionResult =
            vkEnumerateDeviceExtensionProperties(devices[deviceIndex], nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions;
        std::string extensionError;
        if (extensionResult != VK_SUCCESS) {
            extensionError = vkResultMessage("vkEnumerateDeviceExtensionProperties", extensionResult);
        }
        if (extensionResult == VK_SUCCESS && extensionCount > 0u) {
            extensions.resize(extensionCount);
            extensionResult = vkEnumerateDeviceExtensionProperties(
                devices[deviceIndex], nullptr, &extensionCount, extensions.data());
            if (extensionResult != VK_SUCCESS) {
                extensions.clear();
                extensionError =
                    vkResultMessage("vkEnumerateDeviceExtensionProperties", extensionResult);
            } else {
                extensions.resize(extensionCount);
            }
        }

        if (deviceIndex != 0u) {
            json << ',';
        }
        json << "{\"name\":\"" << jsonEscape(properties.deviceName)
             << "\",\"apiVersion\":\"" << VK_API_VERSION_MAJOR(properties.apiVersion)
             << '.' << VK_API_VERSION_MINOR(properties.apiVersion)
             << '.' << VK_API_VERSION_PATCH(properties.apiVersion)
             << "\",\"vendorId\":" << properties.vendorID
             << ",\"deviceId\":" << properties.deviceID
             << ",\"extensionCount\":" << extensions.size()
             << ",\"extensionError\":";
        if (extensionError.empty()) {
            json << "null";
        } else {
            json << '"' << jsonEscape(extensionError) << '"';
        }
        json
             << ",\"extensions\":[";
        for (std::size_t extensionIndex = 0; extensionIndex < extensions.size(); ++extensionIndex) {
            if (extensionIndex != 0u) {
                json << ',';
            }
            json << '"' << jsonEscape(extensions[extensionIndex].extensionName) << '"';
        }
        json << "]}";
    }
    json << "]}";
    return json.str();
}

jstring toJavaString(JNIEnv* env, const std::string& value) {
    return env->NewStringUTF(value.c_str());
}

}  // namespace

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    JNIEnv* env = nullptr;
    if (vm == nullptr || vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK ||
        env == nullptr) {
        return JNI_ERR;
    }
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "JNI_OnLoad completed");
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_project_los_1santos_NativeBridge_nativeInitialize(JNIEnv* env, jclass) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_initialized) {
        __android_log_print(ANDROID_LOG_INFO, kLogTag, "Initializing native diagnostics");
        try {
            project_los_santos::runtime::memory::MemoryArena arena(64u);
            g_runtimeCoreInitialized = arena.allocate(16u) != nullptr;
            if (!g_runtimeCoreInitialized) {
                return toJavaString(env, "{\"initialized\":false,\"error\":"
                                         "\"Runtime memory initialization failed\"}");
            }
            g_vulkanJson = inspectVulkan();
            g_initialized = true;
        } catch (const std::exception& exception) {
            if (g_instance != VK_NULL_HANDLE) {
                vkDestroyInstance(g_instance, nullptr);
                g_instance = VK_NULL_HANDLE;
            }
            return toJavaString(env, "{\"initialized\":false,\"error\":\"" +
                                         jsonEscape(exception.what()) + "\"}");
        } catch (...) {
            if (g_instance != VK_NULL_HANDLE) {
                vkDestroyInstance(g_instance, nullptr);
                g_instance = VK_NULL_HANDLE;
            }
            return toJavaString(env, "{\"initialized\":false,\"error\":"
                                     "\"Unknown native initialization failure\"}");
        }
    }
    return toJavaString(env, "{\"initialized\":true,\"library\":\"native_lib\","
                             "\"version\":\"0.1.0\",\"runtimeCoreInitialized\":" +
                             std::string(g_runtimeCoreInitialized ? "true" : "false") +
                             ",\"vulkan\":" +
                                 g_vulkanJson + "}");
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_project_los_1santos_NativeBridge_nativeRunDiagnostic(JNIEnv* env, jclass) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::ostringstream result;
    result << "Native C++ diagnostic executed; process_id=" << getpid()
           << "; initialized=" << (g_initialized ? "true" : "false");
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "%s", result.str().c_str());
    return toJavaString(env, result.str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_project_los_1santos_NativeBridge_nativeShutdown(JNIEnv*, jclass) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(g_instance, nullptr);
        g_instance = VK_NULL_HANDLE;
    }
    g_initialized = false;
    g_runtimeCoreInitialized = false;
    g_vulkanJson = R"({"available":false,"error":"Not initialized"})";
    __android_log_print(ANDROID_LOG_INFO, kLogTag, "Native diagnostics shut down");
}
