#!/bin/bash

mkdir -p vulkan-profile/vulkan

python3 ~/.local/share/vulkan/registry/gen_profiles_solution.py \
	--registry ~/git/Vulkan-Headers/registry/vk.xml \
	--input . \
	--output-library-inc vulkan-profile/vulkan \
	--output-library-src vulkan-profile/vulkan \
	--validate
