#include "Platform/Windows/YSysUtility.h"
#include <stdio.h>
#include <locale.h>
#include <iostream>

void YSysUtility::AllocWindowsConsole()
{
	AllocConsole();
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONIN$", "w+t", stdin);
	freopen_s(&stream, "CONOUT$", "w+t", stderr);
	setlocale(LC_ALL, "chs");
	std::ios::sync_with_stdio();
	std::cout.clear(); 
}
