#include "Platform/YGenericPlatformAtomics.h"

#if WIN32
#include "Windows/WindowsPlatformAtomics.h"
#elif PLATFORM_PS4
#include "PS4/PS4Atomics.h"
#elif PLATFORM_XBOXONE
#include "XboxOne/XboxOneAtomics.h"
#elif PLATFORM_MAC
#include "Apple/ApplePlatformAtomics.h"
#elif PLATFORM_IOS
#include "Apple/ApplePlatformAtomics.h"
#elif PLATFORM_ANDROID
#include "Android/AndroidAtomics.h"
#elif PLATFORM_HTML5
#include "HTML5/HTML5PlatformAtomics.h"
#elif PLATFORM_UNIX
#include "Unix/UnixPlatformAtomics.h"
#elif PLATFORM_SWITCH
#include "Switch/SwitchPlatformAtomics.h"
#else
#error Unknown platform
#endif