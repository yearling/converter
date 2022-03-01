// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Event.h"

/**
 * This class is allows a simple one-shot scoped event.
 *
 * Usage:
 * {
 *		FScopedEvent MyEvent;
 *		SendReferenceOrPointerToSomeOtherThread(&MyEvent); // Other thread calls MyEvent->Trigger();
 *		// MyEvent destructor is here, we wait here.
 * }
 */
class FScopedEvent
{
public:

	/** Default constructor. */
	 FScopedEvent();

	/** Destructor. */
	 ~FScopedEvent();

	/** Triggers the event. */
	void Trigger()
	{
		Event->Trigger();
	}

	/**
	 * Retrieve the event, usually for passing around.
	 *
	 * @return The event.
	 */
	FEvent* Get()
	{
		return Event;
	}

private:

	/** Holds the event. */
	FEvent* Event;
};
