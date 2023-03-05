#ifndef COMPONENT_HYPNOS_CONFIG_HYPNOS_CONFIG_H
#define COMPONENT_HYPNOS_CONFIG_HYPNOS_CONFIG_H

#include <memory>
#include <nvs_handle.hpp>

namespace hypnos_config {

class HypnosConfig {
    std::unique_ptr<nvs::NVSHandle> handle;
public:
    HypnosConfig();
    bool hasConfig();
};

}

#endif