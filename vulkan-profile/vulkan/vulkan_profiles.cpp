/**
 * Copyright (c) 2021-2023 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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
 *
 * DO NOT EDIT: This file is generated.
 */

#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <vulkan/vulkan_profiles.h>

namespace detail {


VPAPI_ATTR bool isMultiple(double source, double multiple) {
    double mod = std::fmod(source, multiple);
    return std::abs(mod) < 0.0001; 
}

VPAPI_ATTR bool isPowerOfTwo(double source) {
    double mod = std::fmod(source, 1.0);
    if (std::abs(mod) >= 0.0001) return false;

    std::uint64_t value = static_cast<std::uint64_t>(std::abs(source));
    return !(value & (value - static_cast<std::uint64_t>(1)));
}

using PFN_vpStructFiller = void(*)(VkBaseOutStructure* p);
using PFN_vpStructComparator = bool(*)(VkBaseOutStructure* p);
using PFN_vpStructChainerCb =  void(*)(VkBaseOutStructure* p, void* pUser);
using PFN_vpStructChainer = void(*)(VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb);

struct VpFeatureDesc {
    PFN_vpStructFiller              pfnFiller;
    PFN_vpStructComparator          pfnComparator;
    PFN_vpStructChainer             pfnChainer;
};

struct VpPropertyDesc {
    PFN_vpStructFiller              pfnFiller;
    PFN_vpStructComparator          pfnComparator;
    PFN_vpStructChainer             pfnChainer;
};

struct VpQueueFamilyDesc {
    PFN_vpStructFiller              pfnFiller;
    PFN_vpStructComparator          pfnComparator;
};

struct VpFormatDesc {
    VkFormat                        format;
    PFN_vpStructFiller              pfnFiller;
    PFN_vpStructComparator          pfnComparator;
};

struct VpStructChainerDesc {
    PFN_vpStructChainer             pfnFeature;
    PFN_vpStructChainer             pfnProperty;
    PFN_vpStructChainer             pfnQueueFamily;
    PFN_vpStructChainer             pfnFormat;
};

struct VpProfileDesc {
    VpProfileProperties             props;
    uint32_t                        minApiVersion;

    const VkExtensionProperties*    pInstanceExtensions;
    uint32_t                        instanceExtensionCount;

    const VkExtensionProperties*    pDeviceExtensions;
    uint32_t                        deviceExtensionCount;

    const VpProfileProperties*      pFallbacks;
    uint32_t                        fallbackCount;

    const VkStructureType*          pFeatureStructTypes;
    uint32_t                        featureStructTypeCount;
    VpFeatureDesc                   feature;

    const VkStructureType*          pPropertyStructTypes;
    uint32_t                        propertyStructTypeCount;
    VpPropertyDesc                  property;

    const VkStructureType*          pQueueFamilyStructTypes;
    uint32_t                        queueFamilyStructTypeCount;
    const VpQueueFamilyDesc*        pQueueFamilies;
    uint32_t                        queueFamilyCount;

    const VkStructureType*          pFormatStructTypes;
    uint32_t                        formatStructTypeCount;
    const VpFormatDesc*             pFormats;
    uint32_t                        formatCount;

    VpStructChainerDesc             chainers;
};

template <typename T>
VPAPI_ATTR bool vpCheckFlags(const T& actual, const uint64_t expected) {
    return (actual & expected) == expected;
}

#ifdef VP_PARALLEL_RDP_baseline
namespace VP_PARALLEL_RDP_BASELINE {

static const VkExtensionProperties deviceExtensions[] = {
    VkExtensionProperties{ VK_KHR_16BIT_STORAGE_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_KHR_8BIT_STORAGE_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, 1 },
};

static const VkStructureType featureStructTypes[] = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR,
};

static const VpFeatureDesc featureDesc = {
    [](VkBaseOutStructure* p) {
            switch (p->sType) {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR: {
                    VkPhysicalDevice16BitStorageFeaturesKHR* s = static_cast<VkPhysicalDevice16BitStorageFeaturesKHR*>(static_cast<void*>(p));
                    s->storageBuffer16BitAccess = VK_TRUE;
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR: {
                    VkPhysicalDevice8BitStorageFeaturesKHR* s = static_cast<VkPhysicalDevice8BitStorageFeaturesKHR*>(static_cast<void*>(p));
                    s->storageBuffer8BitAccess = VK_TRUE;
                } break;
                default: break;
            }
    },
    [](VkBaseOutStructure* p) -> bool {
        bool ret = true;
            switch (p->sType) {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR: {
                    VkPhysicalDevice16BitStorageFeaturesKHR* s = static_cast<VkPhysicalDevice16BitStorageFeaturesKHR*>(static_cast<void*>(p));
                    ret = ret && (s->storageBuffer16BitAccess == VK_TRUE);
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR: {
                    VkPhysicalDevice8BitStorageFeaturesKHR* s = static_cast<VkPhysicalDevice8BitStorageFeaturesKHR*>(static_cast<void*>(p));
                    ret = ret && (s->storageBuffer8BitAccess == VK_TRUE);
                } break;
                default: break;
            }
        return ret;
    }
};

static const VpPropertyDesc propertyDesc = {
    [](VkBaseOutStructure* p) {
    },
    [](VkBaseOutStructure* p) -> bool {
        bool ret = true;
        return ret;
    }
};

static const VpStructChainerDesc chainerDesc = {
    [](VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb) {
        VkPhysicalDevice16BitStorageFeaturesKHR physicalDevice16BitStorageFeaturesKHR{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR, nullptr };
        VkPhysicalDevice8BitStorageFeaturesKHR physicalDevice8BitStorageFeaturesKHR{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR, &physicalDevice16BitStorageFeaturesKHR };
        p->pNext = static_cast<VkBaseOutStructure*>(static_cast<void*>(&physicalDevice8BitStorageFeaturesKHR));
        pfnCb(p, pUser);
    },
    [](VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb) {
        pfnCb(p, pUser);
    },
    [](VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb) {
        pfnCb(p, pUser);
    },
    [](VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb) {
        pfnCb(p, pUser);
    },
};

} // namespace VP_PARALLEL_RDP_BASELINE
#endif

#ifdef VP_PARALLEL_RDP_optimal
namespace VP_PARALLEL_RDP_OPTIMAL {

static const VkExtensionProperties deviceExtensions[] = {
    VkExtensionProperties{ VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_KHR_16BIT_STORAGE_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_KHR_8BIT_STORAGE_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_KHR_SWAPCHAIN_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, 1 },
    VkExtensionProperties{ VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, 1 },
};

static const VkStructureType featureStructTypes[] = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR,
};

static const VkStructureType propertyStructTypes[] = {
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES_EXT,
    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES,
};

static const VpFeatureDesc featureDesc = {
    [](VkBaseOutStructure* p) {
            switch (p->sType) {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR: {
                    VkPhysicalDevice16BitStorageFeaturesKHR* s = static_cast<VkPhysicalDevice16BitStorageFeaturesKHR*>(static_cast<void*>(p));
                    s->storageBuffer16BitAccess = VK_TRUE;
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR: {
                    VkPhysicalDevice8BitStorageFeaturesKHR* s = static_cast<VkPhysicalDevice8BitStorageFeaturesKHR*>(static_cast<void*>(p));
                    s->storageBuffer8BitAccess = VK_TRUE;
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR: {
                    VkPhysicalDeviceFeatures2KHR* s = static_cast<VkPhysicalDeviceFeatures2KHR*>(static_cast<void*>(p));
                    s->features.shaderInt16 = VK_TRUE;
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR: {
                    VkPhysicalDeviceShaderFloat16Int8FeaturesKHR* s = static_cast<VkPhysicalDeviceShaderFloat16Int8FeaturesKHR*>(static_cast<void*>(p));
                    s->shaderInt8 = VK_TRUE;
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT: {
                    VkPhysicalDeviceSubgroupSizeControlFeaturesEXT* s = static_cast<VkPhysicalDeviceSubgroupSizeControlFeaturesEXT*>(static_cast<void*>(p));
                    s->computeFullSubgroups = VK_TRUE;
                    s->subgroupSizeControl = VK_TRUE;
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR: {
                    VkPhysicalDeviceSynchronization2FeaturesKHR* s = static_cast<VkPhysicalDeviceSynchronization2FeaturesKHR*>(static_cast<void*>(p));
                    s->synchronization2 = VK_TRUE;
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR: {
                    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR* s = static_cast<VkPhysicalDeviceTimelineSemaphoreFeaturesKHR*>(static_cast<void*>(p));
                    s->timelineSemaphore = VK_TRUE;
                } break;
                default: break;
            }
    },
    [](VkBaseOutStructure* p) -> bool {
        bool ret = true;
            switch (p->sType) {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR: {
                    VkPhysicalDevice16BitStorageFeaturesKHR* s = static_cast<VkPhysicalDevice16BitStorageFeaturesKHR*>(static_cast<void*>(p));
                    ret = ret && (s->storageBuffer16BitAccess == VK_TRUE);
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR: {
                    VkPhysicalDevice8BitStorageFeaturesKHR* s = static_cast<VkPhysicalDevice8BitStorageFeaturesKHR*>(static_cast<void*>(p));
                    ret = ret && (s->storageBuffer8BitAccess == VK_TRUE);
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR: {
                    VkPhysicalDeviceFeatures2KHR* s = static_cast<VkPhysicalDeviceFeatures2KHR*>(static_cast<void*>(p));
                    ret = ret && (s->features.shaderInt16 == VK_TRUE);
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR: {
                    VkPhysicalDeviceShaderFloat16Int8FeaturesKHR* s = static_cast<VkPhysicalDeviceShaderFloat16Int8FeaturesKHR*>(static_cast<void*>(p));
                    ret = ret && (s->shaderInt8 == VK_TRUE);
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT: {
                    VkPhysicalDeviceSubgroupSizeControlFeaturesEXT* s = static_cast<VkPhysicalDeviceSubgroupSizeControlFeaturesEXT*>(static_cast<void*>(p));
                    ret = ret && (s->computeFullSubgroups == VK_TRUE);
                    ret = ret && (s->subgroupSizeControl == VK_TRUE);
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR: {
                    VkPhysicalDeviceSynchronization2FeaturesKHR* s = static_cast<VkPhysicalDeviceSynchronization2FeaturesKHR*>(static_cast<void*>(p));
                    ret = ret && (s->synchronization2 == VK_TRUE);
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR: {
                    VkPhysicalDeviceTimelineSemaphoreFeaturesKHR* s = static_cast<VkPhysicalDeviceTimelineSemaphoreFeaturesKHR*>(static_cast<void*>(p));
                    ret = ret && (s->timelineSemaphore == VK_TRUE);
                } break;
                default: break;
            }
        return ret;
    }
};

static const VpPropertyDesc propertyDesc = {
    [](VkBaseOutStructure* p) {
            switch (p->sType) {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT: {
                    VkPhysicalDeviceExternalMemoryHostPropertiesEXT* s = static_cast<VkPhysicalDeviceExternalMemoryHostPropertiesEXT*>(static_cast<void*>(p));
                    s->minImportedHostPointerAlignment = 65536;
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES_EXT: {
                    VkPhysicalDeviceSubgroupSizeControlPropertiesEXT* s = static_cast<VkPhysicalDeviceSubgroupSizeControlPropertiesEXT*>(static_cast<void*>(p));
                    s->requiredSubgroupSizeStages = (VK_SHADER_STAGE_COMPUTE_BIT);
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES: {
                    VkPhysicalDeviceSubgroupProperties* s = static_cast<VkPhysicalDeviceSubgroupProperties*>(static_cast<void*>(p));
                    s->supportedOperations = (VK_SUBGROUP_FEATURE_BALLOT_BIT | VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_VOTE_BIT | VK_SUBGROUP_FEATURE_ARITHMETIC_BIT);
                    s->supportedStages = (VK_SHADER_STAGE_COMPUTE_BIT);
                } break;
                default: break;
            }
    },
    [](VkBaseOutStructure* p) -> bool {
        bool ret = true;
            switch (p->sType) {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT: {
                    VkPhysicalDeviceExternalMemoryHostPropertiesEXT* s = static_cast<VkPhysicalDeviceExternalMemoryHostPropertiesEXT*>(static_cast<void*>(p));
                    ret = ret && (s->minImportedHostPointerAlignment <= 65536);
                    ret = ret && ((s->minImportedHostPointerAlignment & (s->minImportedHostPointerAlignment - 1)) == 0);
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES_EXT: {
                    VkPhysicalDeviceSubgroupSizeControlPropertiesEXT* s = static_cast<VkPhysicalDeviceSubgroupSizeControlPropertiesEXT*>(static_cast<void*>(p));
                    ret = ret && (vpCheckFlags(s->requiredSubgroupSizeStages, (VK_SHADER_STAGE_COMPUTE_BIT)));
                } break;
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES: {
                    VkPhysicalDeviceSubgroupProperties* s = static_cast<VkPhysicalDeviceSubgroupProperties*>(static_cast<void*>(p));
                    ret = ret && (vpCheckFlags(s->supportedOperations, (VK_SUBGROUP_FEATURE_BALLOT_BIT | VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_VOTE_BIT | VK_SUBGROUP_FEATURE_ARITHMETIC_BIT)));
                    ret = ret && (vpCheckFlags(s->supportedStages, (VK_SHADER_STAGE_COMPUTE_BIT)));
                } break;
                default: break;
            }
        return ret;
    }
};

static const VpStructChainerDesc chainerDesc = {
    [](VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb) {
        VkPhysicalDevice16BitStorageFeaturesKHR physicalDevice16BitStorageFeaturesKHR{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR, nullptr };
        VkPhysicalDevice8BitStorageFeaturesKHR physicalDevice8BitStorageFeaturesKHR{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR, &physicalDevice16BitStorageFeaturesKHR };
        VkPhysicalDeviceShaderFloat16Int8FeaturesKHR physicalDeviceShaderFloat16Int8FeaturesKHR{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR, &physicalDevice8BitStorageFeaturesKHR };
        VkPhysicalDeviceSubgroupSizeControlFeaturesEXT physicalDeviceSubgroupSizeControlFeaturesEXT{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT, &physicalDeviceShaderFloat16Int8FeaturesKHR };
        VkPhysicalDeviceSynchronization2FeaturesKHR physicalDeviceSynchronization2FeaturesKHR{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR, &physicalDeviceSubgroupSizeControlFeaturesEXT };
        VkPhysicalDeviceTimelineSemaphoreFeaturesKHR physicalDeviceTimelineSemaphoreFeaturesKHR{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR, &physicalDeviceSynchronization2FeaturesKHR };
        p->pNext = static_cast<VkBaseOutStructure*>(static_cast<void*>(&physicalDeviceTimelineSemaphoreFeaturesKHR));
        pfnCb(p, pUser);
    },
    [](VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb) {
        VkPhysicalDeviceExternalMemoryHostPropertiesEXT physicalDeviceExternalMemoryHostPropertiesEXT{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT, nullptr };
        VkPhysicalDeviceSubgroupSizeControlPropertiesEXT physicalDeviceSubgroupSizeControlPropertiesEXT{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES_EXT, &physicalDeviceExternalMemoryHostPropertiesEXT };
        VkPhysicalDeviceSubgroupProperties physicalDeviceSubgroupProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES, &physicalDeviceSubgroupSizeControlPropertiesEXT };
        p->pNext = static_cast<VkBaseOutStructure*>(static_cast<void*>(&physicalDeviceSubgroupProperties));
        pfnCb(p, pUser);
    },
    [](VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb) {
        pfnCb(p, pUser);
    },
    [](VkBaseOutStructure* p, void* pUser, PFN_vpStructChainerCb pfnCb) {
        pfnCb(p, pUser);
    },
};

} // namespace VP_PARALLEL_RDP_OPTIMAL
#endif

static const VpProfileDesc vpProfiles[] = {
#ifdef VP_PARALLEL_RDP_baseline
    VpProfileDesc{
        VpProfileProperties{ VP_PARALLEL_RDP_BASELINE_NAME, VP_PARALLEL_RDP_BASELINE_SPEC_VERSION },
        VP_PARALLEL_RDP_BASELINE_MIN_API_VERSION,
        nullptr, 0,
        &VP_PARALLEL_RDP_BASELINE::deviceExtensions[0], static_cast<uint32_t>(sizeof(VP_PARALLEL_RDP_BASELINE::deviceExtensions) / sizeof(VP_PARALLEL_RDP_BASELINE::deviceExtensions[0])),
        nullptr, 0,
        &VP_PARALLEL_RDP_BASELINE::featureStructTypes[0], static_cast<uint32_t>(sizeof(VP_PARALLEL_RDP_BASELINE::featureStructTypes) / sizeof(VP_PARALLEL_RDP_BASELINE::featureStructTypes[0])),
        VP_PARALLEL_RDP_BASELINE::featureDesc,
        nullptr, 0,
        VP_PARALLEL_RDP_BASELINE::propertyDesc,
        nullptr, 0,
        nullptr, 0,
        nullptr, 0,
        nullptr, 0,
        VP_PARALLEL_RDP_BASELINE::chainerDesc,
    },
#endif
#ifdef VP_PARALLEL_RDP_optimal
    VpProfileDesc{
        VpProfileProperties{ VP_PARALLEL_RDP_OPTIMAL_NAME, VP_PARALLEL_RDP_OPTIMAL_SPEC_VERSION },
        VP_PARALLEL_RDP_OPTIMAL_MIN_API_VERSION,
        nullptr, 0,
        &VP_PARALLEL_RDP_OPTIMAL::deviceExtensions[0], static_cast<uint32_t>(sizeof(VP_PARALLEL_RDP_OPTIMAL::deviceExtensions) / sizeof(VP_PARALLEL_RDP_OPTIMAL::deviceExtensions[0])),
        nullptr, 0,
        &VP_PARALLEL_RDP_OPTIMAL::featureStructTypes[0], static_cast<uint32_t>(sizeof(VP_PARALLEL_RDP_OPTIMAL::featureStructTypes) / sizeof(VP_PARALLEL_RDP_OPTIMAL::featureStructTypes[0])),
        VP_PARALLEL_RDP_OPTIMAL::featureDesc,
        &VP_PARALLEL_RDP_OPTIMAL::propertyStructTypes[0], static_cast<uint32_t>(sizeof(VP_PARALLEL_RDP_OPTIMAL::propertyStructTypes) / sizeof(VP_PARALLEL_RDP_OPTIMAL::propertyStructTypes[0])),
        VP_PARALLEL_RDP_OPTIMAL::propertyDesc,
        nullptr, 0,
        nullptr, 0,
        nullptr, 0,
        nullptr, 0,
        VP_PARALLEL_RDP_OPTIMAL::chainerDesc,
    },
#endif
};
static const uint32_t vpProfileCount = static_cast<uint32_t>(sizeof(vpProfiles) / sizeof(vpProfiles[0]));

VPAPI_ATTR const VpProfileDesc* vpGetProfileDesc(const char profileName[VP_MAX_PROFILE_NAME_SIZE]) {
    for (uint32_t i = 0; i < vpProfileCount; ++i) {
        if (strncmp(vpProfiles[i].props.profileName, profileName, VP_MAX_PROFILE_NAME_SIZE) == 0) return &vpProfiles[i];
    }
    return nullptr;
}

VPAPI_ATTR bool vpCheckVersion(uint32_t actual, uint32_t expected) {
    uint32_t actualMajor = VK_API_VERSION_MAJOR(actual);
    uint32_t actualMinor = VK_API_VERSION_MINOR(actual);
    uint32_t expectedMajor = VK_API_VERSION_MAJOR(expected);
    uint32_t expectedMinor = VK_API_VERSION_MINOR(expected);
    return actualMajor > expectedMajor || (actualMajor == expectedMajor && actualMinor >= expectedMinor);
}

VPAPI_ATTR bool vpCheckExtension(const VkExtensionProperties *supportedProperties, size_t supportedSize,
                                 const char *requestedExtension) {
    bool found = false;
    for (size_t i = 0, n = supportedSize; i < n; ++i) {
        if (strcmp(supportedProperties[i].extensionName, requestedExtension) == 0) {
            found = true;
            // Drivers don't actually update their spec version, so we cannot rely on this
            // if (supportedProperties[i].specVersion >= expectedVersion) found = true;
        }
    }
    return found;
}

VPAPI_ATTR void vpGetExtensions(uint32_t requestedExtensionCount, const char *const *ppRequestedExtensionNames,
                                uint32_t profileExtensionCount, const VkExtensionProperties *pProfileExtensionProperties,
                                std::vector<const char *> &extensions, bool merge, bool override) {
    if (override) {
        for (uint32_t i = 0; i < requestedExtensionCount; ++i) {
            extensions.push_back(ppRequestedExtensionNames[i]);
        }
    } else {
        for (uint32_t i = 0; i < profileExtensionCount; ++i) {
            extensions.push_back(pProfileExtensionProperties[i].extensionName);
        }

        if (merge) {
            for (uint32_t i = 0; i < requestedExtensionCount; ++i) {
                if (vpCheckExtension(pProfileExtensionProperties, profileExtensionCount, ppRequestedExtensionNames[i])) {
                    continue;
                }
                extensions.push_back(ppRequestedExtensionNames[i]);
            }
        }
    }
}

VPAPI_ATTR const void* vpGetStructure(const void* pNext, VkStructureType type) {
    const VkBaseOutStructure *p = static_cast<const VkBaseOutStructure*>(pNext);
    while (p != nullptr) {
        if (p->sType == type) return p;
        p = p->pNext;
    }
    return nullptr;
}

VPAPI_ATTR void* vpGetStructure(void* pNext, VkStructureType type) {
    VkBaseOutStructure *p = static_cast<VkBaseOutStructure*>(pNext);
    while (p != nullptr) {
        if (p->sType == type) return p;
        p = p->pNext;
    }
    return nullptr;
}

} // namespace detail

VPAPI_ATTR VkResult vpGetProfiles(uint32_t *pPropertyCount, VpProfileProperties *pProperties) {
    VkResult result = VK_SUCCESS;

    if (pProperties == nullptr) {
        *pPropertyCount = detail::vpProfileCount;
    } else {
        if (*pPropertyCount < detail::vpProfileCount) {
            result = VK_INCOMPLETE;
        } else {
            *pPropertyCount = detail::vpProfileCount;
        }
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            pProperties[i] = detail::vpProfiles[i].props;
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetProfileFallbacks(const VpProfileProperties *pProfile, uint32_t *pPropertyCount, VpProfileProperties *pProperties) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pProperties == nullptr) {
        *pPropertyCount = pDesc->fallbackCount;
    } else {
        if (*pPropertyCount < pDesc->fallbackCount) {
            result = VK_INCOMPLETE;
        } else {
            *pPropertyCount = pDesc->fallbackCount;
        }
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            pProperties[i] = pDesc->pFallbacks[i];
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetInstanceProfileSupport(const char *pLayerName, const VpProfileProperties *pProfile, VkBool32 *pSupported) {
    VkResult result = VK_SUCCESS;

    uint32_t apiVersion = VK_MAKE_VERSION(1, 0, 0);
    static PFN_vkEnumerateInstanceVersion pfnEnumerateInstanceVersion =
        (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion");
    if (pfnEnumerateInstanceVersion != nullptr) {
        result = pfnEnumerateInstanceVersion(&apiVersion);
        if (result != VK_SUCCESS) {
            return result;
        }
    }

    uint32_t extCount = 0;
    result = vkEnumerateInstanceExtensionProperties(pLayerName, &extCount, nullptr);
    if (result != VK_SUCCESS) {
        return result;
    }
    std::vector<VkExtensionProperties> ext(extCount);
    result = vkEnumerateInstanceExtensionProperties(pLayerName, &extCount, ext.data());
    if (result != VK_SUCCESS) {
        return result;
    }

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    *pSupported = VK_TRUE;

    if (pDesc->props.specVersion < pProfile->specVersion) {
        *pSupported = VK_FALSE;
    }

    if (!detail::vpCheckVersion(apiVersion, pDesc->minApiVersion)) {
        *pSupported = VK_FALSE;
    }

    for (uint32_t i = 0; i < pDesc->instanceExtensionCount; ++i) {
        if (!detail::vpCheckExtension(ext.data(), ext.size(),
            pDesc->pInstanceExtensions[i].extensionName)) {
            *pSupported = VK_FALSE;
        }
    }

    // We require VK_KHR_get_physical_device_properties2 if we are on Vulkan 1.0
    if (apiVersion < VK_API_VERSION_1_1) {
        bool foundGPDP2 = false;
        for (size_t i = 0; i < ext.size(); ++i) {
            if (strcmp(ext[i].extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0) {
                foundGPDP2 = true;
                break;
            }
        }
        if (!foundGPDP2) {
            *pSupported = VK_FALSE;
        }
    }

    return result;
}

VPAPI_ATTR VkResult vpCreateInstance(const VpInstanceCreateInfo *pCreateInfo,
                                     const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    std::vector<const char*> extensions;
    VkInstanceCreateInfo* pInstanceCreateInfo = nullptr;

    if (pCreateInfo != nullptr && pCreateInfo->pCreateInfo != nullptr) {
        createInfo = *pCreateInfo->pCreateInfo;
        pInstanceCreateInfo = &createInfo;

        const detail::VpProfileDesc* pDesc = nullptr;
        if (pCreateInfo->pProfile != nullptr) {
            pDesc = detail::vpGetProfileDesc(pCreateInfo->pProfile->profileName);
            if (pDesc == nullptr) return VK_ERROR_UNKNOWN;
        }

        if (createInfo.pApplicationInfo == nullptr) {
            appInfo.apiVersion = pDesc->minApiVersion;
            createInfo.pApplicationInfo = &appInfo;
        }

        if (pDesc != nullptr && pDesc->pInstanceExtensions != nullptr) {
            bool merge = (pCreateInfo->flags & VP_INSTANCE_CREATE_MERGE_EXTENSIONS_BIT) != 0;
            bool override = (pCreateInfo->flags & VP_INSTANCE_CREATE_OVERRIDE_EXTENSIONS_BIT) != 0;

            if (!merge && !override && pCreateInfo->pCreateInfo->enabledExtensionCount > 0) {
                // If neither merge nor override is used then the application must not specify its
                // own extensions
                return VK_ERROR_UNKNOWN;
            }

            detail::vpGetExtensions(pCreateInfo->pCreateInfo->enabledExtensionCount,
                                    pCreateInfo->pCreateInfo->ppEnabledExtensionNames,
                                    pDesc->instanceExtensionCount,
                                    pDesc->pInstanceExtensions,
                                    extensions, merge, override);
            {
                bool foundPortEnum = false;
                for (size_t i = 0; i < extensions.size(); ++i) {
                    if (strcmp(extensions[i], VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
                        foundPortEnum = true;
                        break;
                    }
                }
                if (foundPortEnum) {
                    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
                }
            }

            // Need to include VK_KHR_get_physical_device_properties2 if we are on Vulkan 1.0
            if (createInfo.pApplicationInfo->apiVersion < VK_API_VERSION_1_1) {
                bool foundGPDP2 = false;
                for (size_t i = 0; i < extensions.size(); ++i) {
                    if (strcmp(extensions[i], VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0) {
                        foundGPDP2 = true;
                        break;
                    }
                }
                if (!foundGPDP2) {
                    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
                }
            }

            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
        }
    }

    return vkCreateInstance(pInstanceCreateInfo, pAllocator, pInstance);
}

VPAPI_ATTR VkResult vpGetPhysicalDeviceProfileSupport(VkInstance instance, VkPhysicalDevice physicalDevice,
                                                      const VpProfileProperties *pProfile, VkBool32 *pSupported) {
    VkResult result = VK_SUCCESS;

    uint32_t extCount = 0;
    result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
    if (result != VK_SUCCESS) {
        return result;
    }
    std::vector<VkExtensionProperties> ext;
    if (extCount > 0) {
        ext.resize(extCount);
    }
    result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, ext.data());
    if (result != VK_SUCCESS) {
        return result;
    }

    // Workaround old loader bug where count could be smaller on the second call to vkEnumerateDeviceExtensionProperties
    if (extCount > 0) {
        ext.resize(extCount);
    }

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    struct GPDP2EntryPoints {
        PFN_vkGetPhysicalDeviceFeatures2KHR                 pfnGetPhysicalDeviceFeatures2;
        PFN_vkGetPhysicalDeviceProperties2KHR               pfnGetPhysicalDeviceProperties2;
        PFN_vkGetPhysicalDeviceFormatProperties2KHR         pfnGetPhysicalDeviceFormatProperties2;
        PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR    pfnGetPhysicalDeviceQueueFamilyProperties2;
    };

    struct UserData {
        VkPhysicalDevice                    physicalDevice;
        const detail::VpProfileDesc*        pDesc;
        GPDP2EntryPoints                    gpdp2;
        uint32_t                            index;
        uint32_t                            count;
        detail::PFN_vpStructChainerCb       pfnCb;
        bool                                supported;
    } userData{ physicalDevice, pDesc };

    // Attempt to load core versions of the GPDP2 entry points
    userData.gpdp2.pfnGetPhysicalDeviceFeatures2 =
        (PFN_vkGetPhysicalDeviceFeatures2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2");
    userData.gpdp2.pfnGetPhysicalDeviceProperties2 =
        (PFN_vkGetPhysicalDeviceProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2");
    userData.gpdp2.pfnGetPhysicalDeviceFormatProperties2 =
        (PFN_vkGetPhysicalDeviceFormatProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties2");
    userData.gpdp2.pfnGetPhysicalDeviceQueueFamilyProperties2 =
        (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2");

    // If not successful, try to load KHR variant
    if (userData.gpdp2.pfnGetPhysicalDeviceFeatures2 == nullptr) {
        userData.gpdp2.pfnGetPhysicalDeviceFeatures2 =
            (PFN_vkGetPhysicalDeviceFeatures2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR");
        userData.gpdp2.pfnGetPhysicalDeviceProperties2 =
            (PFN_vkGetPhysicalDeviceProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR");
        userData.gpdp2.pfnGetPhysicalDeviceFormatProperties2 =
            (PFN_vkGetPhysicalDeviceFormatProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties2KHR");
        userData.gpdp2.pfnGetPhysicalDeviceQueueFamilyProperties2 =
            (PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties2KHR");
    }

    if (userData.gpdp2.pfnGetPhysicalDeviceFeatures2 == nullptr ||
        userData.gpdp2.pfnGetPhysicalDeviceProperties2 == nullptr ||
        userData.gpdp2.pfnGetPhysicalDeviceFormatProperties2 == nullptr ||
        userData.gpdp2.pfnGetPhysicalDeviceQueueFamilyProperties2 == nullptr) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    *pSupported = VK_TRUE;

    if (pDesc->props.specVersion < pProfile->specVersion) {
        *pSupported = VK_FALSE;
    }

    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(physicalDevice, &props);
        if (!detail::vpCheckVersion(props.apiVersion, pDesc->minApiVersion)) {
            *pSupported = VK_FALSE;
        }
    }

    for (uint32_t i = 0; i < pDesc->deviceExtensionCount; ++i) {
        if (!detail::vpCheckExtension(ext.data(), ext.size(),
            pDesc->pDeviceExtensions[i].extensionName)) {
            *pSupported = VK_FALSE;
        }
    }

    {
        VkPhysicalDeviceFeatures2KHR features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
        pDesc->chainers.pfnFeature(static_cast<VkBaseOutStructure*>(static_cast<void*>(&features)), &userData,
            [](VkBaseOutStructure* p, void* pUser) {
                UserData* pUserData = static_cast<UserData*>(pUser);
                pUserData->gpdp2.pfnGetPhysicalDeviceFeatures2(pUserData->physicalDevice,
                                                               static_cast<VkPhysicalDeviceFeatures2KHR*>(static_cast<void*>(p)));
                pUserData->supported = true;
                while (p != nullptr) {
                    if (!pUserData->pDesc->feature.pfnComparator(p)) {
                        pUserData->supported = false;
                    }
                    p = p->pNext;
                }
            }
        );
        if (!userData.supported) {
            *pSupported = VK_FALSE;
        }
    }

    {
        VkPhysicalDeviceProperties2KHR props{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR };
        pDesc->chainers.pfnProperty(static_cast<VkBaseOutStructure*>(static_cast<void*>(&props)), &userData,
            [](VkBaseOutStructure* p, void* pUser) {
                UserData* pUserData = static_cast<UserData*>(pUser);
                pUserData->gpdp2.pfnGetPhysicalDeviceProperties2(pUserData->physicalDevice,
                                                                 static_cast<VkPhysicalDeviceProperties2KHR*>(static_cast<void*>(p)));
                pUserData->supported = true;
                while (p != nullptr) {
                    if (!pUserData->pDesc->property.pfnComparator(p)) {
                        pUserData->supported = false;
                    }
                    p = p->pNext;
                }
            }
        );
        if (!userData.supported) {
            *pSupported = VK_FALSE;
        }
    }

    for (uint32_t i = 0; i < pDesc->formatCount; ++i) {
        userData.index = i;
        VkFormatProperties2KHR props{ VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR };
        pDesc->chainers.pfnFormat(static_cast<VkBaseOutStructure*>(static_cast<void*>(&props)), &userData,
            [](VkBaseOutStructure* p, void* pUser) {
                UserData* pUserData = static_cast<UserData*>(pUser);
                pUserData->gpdp2.pfnGetPhysicalDeviceFormatProperties2(pUserData->physicalDevice,
                                                                       pUserData->pDesc->pFormats[pUserData->index].format,
                                                                       static_cast<VkFormatProperties2KHR*>(static_cast<void*>(p)));
                pUserData->supported = true;
                while (p != nullptr) {
                    if (!pUserData->pDesc->pFormats[pUserData->index].pfnComparator(p)) {
                        pUserData->supported = false;
                    }
                    p = p->pNext;
                }
            }
        );
        if (!userData.supported) {
            *pSupported = VK_FALSE;
        }
    }

    {
        userData.gpdp2.pfnGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &userData.count, nullptr);
        std::vector<VkQueueFamilyProperties2KHR> props(userData.count, { VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR });
        userData.index = 0;

        detail::PFN_vpStructChainerCb callback = [](VkBaseOutStructure* p, void* pUser) {
            UserData* pUserData = static_cast<UserData*>(pUser);
            VkQueueFamilyProperties2KHR* pProps = static_cast<VkQueueFamilyProperties2KHR*>(static_cast<void*>(p));
            if (++pUserData->index < pUserData->count) {
                pUserData->pDesc->chainers.pfnQueueFamily(static_cast<VkBaseOutStructure*>(static_cast<void*>(++pProps)),
                                                          pUser, pUserData->pfnCb);
            } else {
                pProps -= pUserData->count - 1;
                pUserData->gpdp2.pfnGetPhysicalDeviceQueueFamilyProperties2(pUserData->physicalDevice,
                                                                            &pUserData->count,
                                                                            pProps);
                pUserData->supported = true;

                // Check first that each queue family defined is supported by the device
                for (uint32_t i = 0; i < pUserData->pDesc->queueFamilyCount; ++i) {
                    bool found = false;
                    for (uint32_t j = 0; j < pUserData->count; ++j) {
                        bool propsMatch = true;
                        p = static_cast<VkBaseOutStructure*>(static_cast<void*>(&pProps[j]));
                        while (p != nullptr) {
                            if (!pUserData->pDesc->pQueueFamilies[i].pfnComparator(p)) {
                                propsMatch = false;
                                break;
                            }
                            p = p->pNext;
                        }
                        if (propsMatch) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        pUserData->supported = false;
                        return;
                    }
                }

                // Then check each permutation to ensure that while order of the queue families
                // doesn't matter, each queue family property criteria is matched with a separate
                // queue family of the actual device
                std::vector<uint32_t> permutation(pUserData->count);
                for (uint32_t i = 0; i < pUserData->count; ++i) {
                    permutation[i] = i;
                }
                bool found = false;
                do {
                    bool propsMatch = true;
                    for (uint32_t i = 0; i < pUserData->pDesc->queueFamilyCount && propsMatch; ++i) {
                        p = static_cast<VkBaseOutStructure*>(static_cast<void*>(&pProps[permutation[i]]));
                        while (p != nullptr) {
                            if (!pUserData->pDesc->pQueueFamilies[i].pfnComparator(p)) {
                                propsMatch = false;
                                break;
                            }
                            p = p->pNext;
                        }
                    }
                    if (propsMatch) {
                        found = true;
                        break;
                    }
                } while (std::next_permutation(permutation.begin(), permutation.end()));

                if (!found) {
                    pUserData->supported = false;
                }
            }
        };
        userData.pfnCb = callback;

        if (userData.count >= userData.pDesc->queueFamilyCount) {
            pDesc->chainers.pfnQueueFamily(static_cast<VkBaseOutStructure*>(static_cast<void*>(props.data())), &userData, callback);
            if (!userData.supported) {
                *pSupported = VK_FALSE;
            }
        } else {
            *pSupported = VK_FALSE;
        }
    }

    return result;
}

VPAPI_ATTR VkResult vpCreateDevice(VkPhysicalDevice physicalDevice, const VpDeviceCreateInfo *pCreateInfo,
                                   const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    if (physicalDevice == VK_NULL_HANDLE || pCreateInfo == nullptr || pDevice == nullptr) {
        return vkCreateDevice(physicalDevice, pCreateInfo == nullptr ? nullptr : pCreateInfo->pCreateInfo, pAllocator, pDevice);
    }

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pCreateInfo->pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    struct UserData {
        VkPhysicalDevice                physicalDevice;
        const detail::VpProfileDesc*    pDesc;
        const VpDeviceCreateInfo*       pCreateInfo;
        const VkAllocationCallbacks*    pAllocator;
        VkDevice*                       pDevice;
        VkResult                        result;
    } userData{ physicalDevice, pDesc, pCreateInfo, pAllocator, pDevice };

    VkPhysicalDeviceFeatures2KHR features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
    pDesc->chainers.pfnFeature(static_cast<VkBaseOutStructure*>(static_cast<void*>(&features)), &userData,
        [](VkBaseOutStructure* p, void* pUser) {
            UserData* pUserData = static_cast<UserData*>(pUser);
            const detail::VpProfileDesc* pDesc = pUserData->pDesc;
            const VpDeviceCreateInfo* pCreateInfo = pUserData->pCreateInfo;

            bool merge = (pCreateInfo->flags & VP_DEVICE_CREATE_MERGE_EXTENSIONS_BIT) != 0;
            bool override = (pCreateInfo->flags & VP_DEVICE_CREATE_OVERRIDE_EXTENSIONS_BIT) != 0;

            if (!merge && !override && pCreateInfo->pCreateInfo->enabledExtensionCount > 0) {
                // If neither merge nor override is used then the application must not specify its
                // own extensions
                pUserData->result = VK_ERROR_UNKNOWN;
                return;
            }

            std::vector<const char*> extensions;
            detail::vpGetExtensions(pCreateInfo->pCreateInfo->enabledExtensionCount,
                                    pCreateInfo->pCreateInfo->ppEnabledExtensionNames,
                                    pDesc->deviceExtensionCount,
                                    pDesc->pDeviceExtensions,
                                    extensions, merge, override);

            VkBaseOutStructure profileStructList;
            profileStructList.pNext = p;
            VkPhysicalDeviceFeatures2KHR* pFeatures = static_cast<VkPhysicalDeviceFeatures2KHR*>(static_cast<void*>(p));
            if (pDesc->feature.pfnFiller != nullptr) {
                while (p != nullptr) {
                    pDesc->feature.pfnFiller(p);
                    p = p->pNext;
                }
            }

            if (pCreateInfo->pCreateInfo->pEnabledFeatures != nullptr) {
                pFeatures->features = *pCreateInfo->pCreateInfo->pEnabledFeatures;
            }

            if (pCreateInfo->flags & VP_DEVICE_CREATE_DISABLE_ROBUST_BUFFER_ACCESS_BIT) {
                pFeatures->features.robustBufferAccess = VK_FALSE;
            }

#ifdef VK_EXT_robustness2
            VkPhysicalDeviceRobustness2FeaturesEXT* pRobustness2FeaturesEXT = static_cast<VkPhysicalDeviceRobustness2FeaturesEXT*>(
                detail::vpGetStructure(pFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT));
            if (pRobustness2FeaturesEXT != nullptr) {
                if (pCreateInfo->flags & VP_DEVICE_CREATE_DISABLE_ROBUST_BUFFER_ACCESS_BIT) {
                    pRobustness2FeaturesEXT->robustBufferAccess2 = VK_FALSE;
                }
                if (pCreateInfo->flags & VP_DEVICE_CREATE_DISABLE_ROBUST_IMAGE_ACCESS_BIT) {
                    pRobustness2FeaturesEXT->robustImageAccess2 = VK_FALSE;
                }
            }
#endif

#ifdef VK_EXT_image_robustness
            VkPhysicalDeviceImageRobustnessFeaturesEXT* pImageRobustnessFeaturesEXT = static_cast<VkPhysicalDeviceImageRobustnessFeaturesEXT*>(
                detail::vpGetStructure(pFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES_EXT));
            if (pImageRobustnessFeaturesEXT != nullptr && (pCreateInfo->flags & VP_DEVICE_CREATE_DISABLE_ROBUST_IMAGE_ACCESS_BIT)) {
                pImageRobustnessFeaturesEXT->robustImageAccess = VK_FALSE;
            }
#endif

#ifdef VK_VERSION_1_3
            VkPhysicalDeviceVulkan13Features* pVulkan13Features = static_cast<VkPhysicalDeviceVulkan13Features*>(
                detail::vpGetStructure(pFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES));
            if (pVulkan13Features != nullptr && (pCreateInfo->flags & VP_DEVICE_CREATE_DISABLE_ROBUST_IMAGE_ACCESS_BIT)) {
                pVulkan13Features->robustImageAccess = VK_FALSE;
            }
#endif

            VkBaseOutStructure* pNext = static_cast<VkBaseOutStructure*>(const_cast<void*>(pCreateInfo->pCreateInfo->pNext));
            if ((pCreateInfo->flags & VP_DEVICE_CREATE_OVERRIDE_ALL_FEATURES_BIT) == 0) {
                for (uint32_t i = 0; i < pDesc->featureStructTypeCount; ++i) {
                    const void* pRequested = detail::vpGetStructure(pNext, pDesc->pFeatureStructTypes[i]);
                    if (pRequested == nullptr) {
                        VkBaseOutStructure* pPrevStruct = &profileStructList;
                        VkBaseOutStructure* pCurrStruct = pPrevStruct->pNext;
                        while (pCurrStruct->sType != pDesc->pFeatureStructTypes[i]) {
                            pPrevStruct = pCurrStruct;
                            pCurrStruct = pCurrStruct->pNext;
                        }
                        pPrevStruct->pNext = pCurrStruct->pNext;
                        pCurrStruct->pNext = pNext;
                        pNext = pCurrStruct;
                    } else
                    if ((pCreateInfo->flags & VP_DEVICE_CREATE_OVERRIDE_FEATURES_BIT) == 0) {
                        // If override is not used then the application must not specify its
                        // own feature structure for anything that the profile defines
                        pUserData->result = VK_ERROR_UNKNOWN;
                        return;
                    }
                }
            }

            VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
            createInfo.pNext = pNext;
            createInfo.queueCreateInfoCount = pCreateInfo->pCreateInfo->queueCreateInfoCount;
            createInfo.pQueueCreateInfos = pCreateInfo->pCreateInfo->pQueueCreateInfos;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();
            if (pCreateInfo->flags & VP_DEVICE_CREATE_OVERRIDE_ALL_FEATURES_BIT) {
                createInfo.pEnabledFeatures = pCreateInfo->pCreateInfo->pEnabledFeatures;
            }
            pUserData->result = vkCreateDevice(pUserData->physicalDevice, &createInfo, pUserData->pAllocator, pUserData->pDevice);
        }
    );

    return userData.result;
}

VPAPI_ATTR VkResult vpGetProfileInstanceExtensionProperties(const VpProfileProperties *pProfile, uint32_t *pPropertyCount,
                                                            VkExtensionProperties *pProperties) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pProperties == nullptr) {
        *pPropertyCount = pDesc->instanceExtensionCount;
    } else {
        if (*pPropertyCount < pDesc->instanceExtensionCount) {
            result = VK_INCOMPLETE;
        } else {
            *pPropertyCount = pDesc->instanceExtensionCount;
        }
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            pProperties[i] = pDesc->pInstanceExtensions[i];
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetProfileDeviceExtensionProperties(const VpProfileProperties *pProfile, uint32_t *pPropertyCount,
                                                          VkExtensionProperties *pProperties) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pProperties == nullptr) {
        *pPropertyCount = pDesc->deviceExtensionCount;
    } else {
        if (*pPropertyCount < pDesc->deviceExtensionCount) {
            result = VK_INCOMPLETE;
        } else {
            *pPropertyCount = pDesc->deviceExtensionCount;
        }
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            pProperties[i] = pDesc->pDeviceExtensions[i];
        }
    }
    return result;
}

VPAPI_ATTR void vpGetProfileFeatures(const VpProfileProperties *pProfile, void *pNext) {
    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc != nullptr && pDesc->feature.pfnFiller != nullptr) {
        VkBaseOutStructure* p = static_cast<VkBaseOutStructure*>(pNext);
        while (p != nullptr) {
            pDesc->feature.pfnFiller(p);
            p = p->pNext;
        }
    }
}

VPAPI_ATTR VkResult vpGetProfileFeatureStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                      VkStructureType *pStructureTypes) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pStructureTypes == nullptr) {
        *pStructureTypeCount = pDesc->featureStructTypeCount;
    } else {
        if (*pStructureTypeCount < pDesc->featureStructTypeCount) {
            result = VK_INCOMPLETE;
        } else {
            *pStructureTypeCount = pDesc->featureStructTypeCount;
        }
        for (uint32_t i = 0; i < *pStructureTypeCount; ++i) {
            pStructureTypes[i] = pDesc->pFeatureStructTypes[i];
        }
    }
    return result;
}

VPAPI_ATTR void vpGetProfileProperties(const VpProfileProperties *pProfile, void *pNext) {
    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc != nullptr && pDesc->property.pfnFiller != nullptr) {
        VkBaseOutStructure* p = static_cast<VkBaseOutStructure*>(pNext);
        while (p != nullptr) {
            pDesc->property.pfnFiller(p);
            p = p->pNext;
        }
    }
}

VPAPI_ATTR VkResult vpGetProfilePropertyStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                       VkStructureType *pStructureTypes) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pStructureTypes == nullptr) {
        *pStructureTypeCount = pDesc->propertyStructTypeCount;
    } else {
        if (*pStructureTypeCount < pDesc->propertyStructTypeCount) {
            result = VK_INCOMPLETE;
        } else {
            *pStructureTypeCount = pDesc->propertyStructTypeCount;
        }
        for (uint32_t i = 0; i < *pStructureTypeCount; ++i) {
            pStructureTypes[i] = pDesc->pPropertyStructTypes[i];
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetProfileQueueFamilyProperties(const VpProfileProperties *pProfile, uint32_t *pPropertyCount,
                                                      VkQueueFamilyProperties2KHR *pProperties) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pProperties == nullptr) {
        *pPropertyCount = pDesc->queueFamilyCount;
    } else {
        if (*pPropertyCount < pDesc->queueFamilyCount) {
            result = VK_INCOMPLETE;
        } else {
            *pPropertyCount = pDesc->queueFamilyCount;
        }
        for (uint32_t i = 0; i < *pPropertyCount; ++i) {
            VkBaseOutStructure* p = static_cast<VkBaseOutStructure*>(static_cast<void*>(&pProperties[i]));
            while (p != nullptr) {
                pDesc->pQueueFamilies[i].pfnFiller(p);
                p = p->pNext;
            }
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetProfileQueueFamilyStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                          VkStructureType *pStructureTypes) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pStructureTypes == nullptr) {
        *pStructureTypeCount = pDesc->queueFamilyStructTypeCount;
    } else {
        if (*pStructureTypeCount < pDesc->queueFamilyStructTypeCount) {
            result = VK_INCOMPLETE;
        } else {
            *pStructureTypeCount = pDesc->queueFamilyStructTypeCount;
        }
        for (uint32_t i = 0; i < *pStructureTypeCount; ++i) {
            pStructureTypes[i] = pDesc->pQueueFamilyStructTypes[i];
        }
    }
    return result;
}

VPAPI_ATTR VkResult vpGetProfileFormats(const VpProfileProperties *pProfile, uint32_t *pFormatCount, VkFormat *pFormats) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pFormats == nullptr) {
        *pFormatCount = pDesc->formatCount;
    } else {
        if (*pFormatCount < pDesc->formatCount) {
            result = VK_INCOMPLETE;
        } else {
            *pFormatCount = pDesc->formatCount;
        }
        for (uint32_t i = 0; i < *pFormatCount; ++i) {
            pFormats[i] = pDesc->pFormats[i].format;
        }
    }
    return result;
}

VPAPI_ATTR void vpGetProfileFormatProperties(const VpProfileProperties *pProfile, VkFormat format, void *pNext) {
    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return;

    for (uint32_t i = 0; i < pDesc->formatCount; ++i) {
        if (pDesc->pFormats[i].format == format) {
            VkBaseOutStructure* p = static_cast<VkBaseOutStructure*>(static_cast<void*>(pNext));
            while (p != nullptr) {
                pDesc->pFormats[i].pfnFiller(p);
                p = p->pNext;
            }
#if defined(VK_VERSION_1_3) || defined(VK_KHR_format_feature_flags2)
            VkFormatProperties2KHR* fp2 = static_cast<VkFormatProperties2KHR*>(
                detail::vpGetStructure(pNext, VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR));
            VkFormatProperties3KHR* fp3 = static_cast<VkFormatProperties3KHR*>(
                detail::vpGetStructure(pNext, VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3_KHR));
            if (fp3 != nullptr) {
                VkFormatProperties2KHR fp{ VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR };
                pDesc->pFormats[i].pfnFiller(static_cast<VkBaseOutStructure*>(static_cast<void*>(&fp)));
                fp3->linearTilingFeatures = static_cast<VkFormatFeatureFlags2KHR>(fp3->linearTilingFeatures | fp.formatProperties.linearTilingFeatures);
                fp3->optimalTilingFeatures = static_cast<VkFormatFeatureFlags2KHR>(fp3->optimalTilingFeatures | fp.formatProperties.optimalTilingFeatures);
                fp3->bufferFeatures = static_cast<VkFormatFeatureFlags2KHR>(fp3->bufferFeatures | fp.formatProperties.bufferFeatures);
            }
            if (fp2 != nullptr) {
                VkFormatProperties3KHR fp{ VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3_KHR };
                pDesc->pFormats[i].pfnFiller(static_cast<VkBaseOutStructure*>(static_cast<void*>(&fp)));
                fp2->formatProperties.linearTilingFeatures = static_cast<VkFormatFeatureFlags>(fp2->formatProperties.linearTilingFeatures | fp.linearTilingFeatures);
                fp2->formatProperties.optimalTilingFeatures = static_cast<VkFormatFeatureFlags>(fp2->formatProperties.optimalTilingFeatures | fp.optimalTilingFeatures);
                fp2->formatProperties.bufferFeatures = static_cast<VkFormatFeatureFlags>(fp2->formatProperties.bufferFeatures | fp.bufferFeatures);
            }
#endif
        }
    }
}

VPAPI_ATTR VkResult vpGetProfileFormatStructureTypes(const VpProfileProperties *pProfile, uint32_t *pStructureTypeCount,
                                                     VkStructureType *pStructureTypes) {
    VkResult result = VK_SUCCESS;

    const detail::VpProfileDesc* pDesc = detail::vpGetProfileDesc(pProfile->profileName);
    if (pDesc == nullptr) return VK_ERROR_UNKNOWN;

    if (pStructureTypes == nullptr) {
        *pStructureTypeCount = pDesc->formatStructTypeCount;
    } else {
        if (*pStructureTypeCount < pDesc->formatStructTypeCount) {
            result = VK_INCOMPLETE;
        } else {
            *pStructureTypeCount = pDesc->formatStructTypeCount;
        }
        for (uint32_t i = 0; i < *pStructureTypeCount; ++i) {
            pStructureTypes[i] = pDesc->pFormatStructTypes[i];
        }
    }
    return result;
}
