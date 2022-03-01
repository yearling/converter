#pragma once

#if WIN32
#include "Platform/Windows/YWindowsPlatform.h"
#elif PLATFORM_PS4
#include "PS4/PS4.h"
#elif PLATFORM_XBOXONE
#include "XboxOne/XboxOne.h"
#elif PLATFORM_MAC
#include "Apple/ApplePlatform.h"
#elif PLATFORM_IOS
#include "Apple/ApplePlatform.h"
#elif PLATFORM_ANDROID
#include "Android/Android.h"
#elif PLATFORM_HTML5
#include "HTML5/HTML5Platform.h"
#elif PLATFORM_UNIX
#include "Unix/UnixPlatform.h"
#elif PLATFORM_SWITCH
#include "Switch/SwitchPlatform.h"
#else
#error Unknown platform
#endif