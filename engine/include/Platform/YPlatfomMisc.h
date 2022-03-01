#pragma once

#if WIN32
#include "Platform/Windows/YWindowsPlatformMisc.h"
#elif PLATFORM_PS4
#include "PS4/PS4Misc.h"
#elif PLATFORM_XBOXONE
#include "XboxOne/XboxOneMisc.h"
#elif PLATFORM_MAC
#include "Apple/ApplePlatformMisc.h"
#elif PLATFORM_IOS
#include "Apple/ApplePlatformMisc.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidMisc.h"
#elif PLATFORM_HTML5
#include "HTML5/HTML5PlatformMisc.h"
#elif PLATFORM_UNIX
#include "Unix/UnixPlatformMisc.h"
#elif PLATFORM_SWITCH
#include "Switch/SwitchPlatformMisc.h"
#else
#error Unknown platform
#endif
