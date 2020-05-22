#!/bin/bash

./Granite/tools/create_android_build.py \
	--output-gradle android \
	--application-id net.themaister.rdp_conformance \
	--granite-dir Granite \
	--native-target rdp-conformance \
	--cmake-lists-toplevel CMakeLists.txt \
	--assets parallel-rdp/shaders \
	--abis arm64-v8a \
	--app-name "RDP Conformance"
