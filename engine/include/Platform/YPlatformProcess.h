#pragma once

#if WIN32
#include "Windows/WindowsPlatformProcess.h"
#elif PLATFORM_PS4
#include "PS4/PS4Process.h"
#elif PLATFORM_XBOXONE
#include "XboxOne/XboxOneProcess.h"
#elif PLATFORM_MAC
#include "Apple/ApplePlatformProcess.h"
#elif PLATFORM_IOS
#include "Apple/ApplePlatformProcess.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidProcess.h"
#elif PLATFORM_HTML5
#include "HTML5/HTML5PlatformProcess.h"
#elif PLATFORM_UNIX
#include "Unix/UnixPlatformProcess.h"
#elif PLATFORM_SWITCH
#include "Switch/SwitchPlatformProcess.h"
#else
#error Unknown platform
#endif