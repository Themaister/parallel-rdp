#!/bin/bash

adb shell am start -n net.themaister.vi_conformance/net.themaister.granite.GraniteActivity -e granite "\"--range 0 1000 --verbose\""
