#pragma once

class YGenericPlatformProcess
{
public:
	/** Allow the platform to do anything it needs for game thread */
	static void SetupGameThread() { }

	/** Allow the platform to do anything it needs for render thread */
	static void SetupRenderThread() { }

	/** Allow the platform to do anything it needs for audio thread */
	static void SetupAudioThread() { }
};