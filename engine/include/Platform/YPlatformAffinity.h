#pragma once

#if WIN32
#include "Windows/YWindowsPlatformAffinity.h"
#elif PLATFORM_PS4
#include "PS4/PS4Affinity.h"
#elif PLATFORM_XBOXONE
#include "XboxOne/XboxOneAffinity.h"
#elif PLATFORM_MAC
#include "Apple/ApplePlatformAffinity.h"
#elif PLATFORM_IOS
#include "Apple/ApplePlatformAffinity.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidAffinity.h"
#elif PLATFORM_HTML5
#include "HTML5/HTML5PlatformAffinity.h"
#elif PLATFORM_UNIX
#include "Unix/UnixPlatformAffinity.h"
#elif PLATFORM_SWITCH
#include "Switch/SwitchPlatformAffinity.h"
#else
#error Unknown platform
#endif