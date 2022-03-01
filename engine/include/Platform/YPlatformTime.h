#pragma once

#if WIN32
#include "Platform/Windows/YWindowsPlatformTLS.h"
#elif PLATFORM_PS4
#include "PS4/PS4TLS.h"
#elif PLATFORM_XBOXONE
#include "XboxOne/XboxOneTLS.h"
#elif PLATFORM_MAC
#include "Apple/ApplePlatformTLS.h"
#elif PLATFORM_IOS
#include "Apple/ApplePlatformTLS.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidTLS.h"
#elif PLATFORM_HTML5
#include "HTML5/HTML5PlatformTLS.h"
#elif PLATFORM_UNIX
#include "Unix/UnixPlatformTLS.h"
#elif PLATFORM_SWITCH
#include "Switch/SwitchPlatformTLS.h"
#else
#error Unknown platform
#endif