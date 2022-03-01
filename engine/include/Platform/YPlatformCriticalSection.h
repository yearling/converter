#pragma once

#if WIN32
#include "Platform/Windows/WindowsCriticalSection.h"
#elif PLATFORM_PS4
#include "PS4/PS4CriticalSection.h"
#elif PLATFORM_XBOXONE
#include "XboxOne/XboxOneCriticalSection.h"
#elif PLATFORM_MAC
#include "Apple/ApplePlatformCriticalSection.h"
#elif PLATFORM_IOS
#include "Apple/ApplePlatformCriticalSection.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidCriticalSection.h"
#elif PLATFORM_HTML5
#include "HTML5/HTML5PlatformCriticalSection.h"
#elif PLATFORM_UNIX
#include "Unix/UnixPlatformCriticalSection.h"
#elif PLATFORM_SWITCH
#include "Switch/SwitchPlatformCriticalSection.h"
#else
#error Unknown platform
#endif