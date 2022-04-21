// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D11Query.cpp: D3D query RHI implementation.
=============================================================================*/

#include "RHI/D3D11RHI/D3D11RHI.h"
#include "RHI/D3D11RHI/D3D11Util.h"
#include "Math/YMath.h"
#include "RHI/GPUProfiler.h"


FGPUFenceRHIRef FD3D11DynamicRHI::RHICreateGPUFence(const std::string &Name)
{
	return new FD3D11GPUFence(Name);
}

void FD3D11DynamicRHI::RHIBeginOcclusionQueryBatch(uint32 NumQueriesInBatch)
{
	check(RequestedOcclusionQueriesInBatch == 0);
	ActualOcclusionQueriesInBatch = 0;
	RequestedOcclusionQueriesInBatch = NumQueriesInBatch;
}

void FD3D11DynamicRHI::RHIEndOcclusionQueryBatch()
{
	//checkf(ActualOcclusionQueriesInBatch <= RequestedOcclusionQueriesInBatch, TEXT("Expected %d total occlusion queries per RHIBeginOcclusionQueryBatch(); got %d total; this will break newer APIs"), RequestedOcclusionQueriesInBatch, ActualOcclusionQueriesInBatch);
	assert(ActualOcclusionQueriesInBatch <= ResourceTableFrameCounter);
	RequestedOcclusionQueriesInBatch = 0;
}

FRenderQueryRHIRef FD3D11DynamicRHI::RHICreateRenderQuery(ERenderQueryType QueryType)
{
	TRefCountPtr<ID3D11Query> Query;
	D3D11_QUERY_DESC Desc;

	if(QueryType == RQT_Occlusion)
	{
		Desc.Query = D3D11_QUERY_OCCLUSION;
	}
	else if(QueryType == RQT_AbsoluteTime)
	{
		Desc.Query = D3D11_QUERY_TIMESTAMP;
	}
	else
	{
		check(0);
	}

	Desc.MiscFlags = 0;
	VERIFYD3D11RESULT_EX(d3d_device_->CreateQuery(&Desc,Query.GetInitReference()), d3d_device_);
	return new FD3D11RenderQuery(Query, QueryType);
}

bool FD3D11DynamicRHI::RHIGetRenderQueryResult(FRenderQueryRHIParamRef QueryRHI,uint64& OutResult,bool bWait)
{
	check(IsInRenderingThread());
	FD3D11RenderQuery* Query = ResourceCast(QueryRHI);

	bool bSuccess = true;
	if(!Query->bResultIsCached)
	{
		bSuccess = GetQueryData(Query->Resource,&Query->Result,sizeof(Query->Result),bWait, Query->QueryType);

		Query->bResultIsCached = bSuccess;
	}

	if(Query->QueryType == RQT_AbsoluteTime)
	{
		// GetTimingFrequency is the number of ticks per second
		uint64 Div = YMath::Max(1llu, FGPUTiming::GetTimingFrequency() / (1000 * 1000));

		// convert from GPU specific timestamp to micro sec (1 / 1 000 000 s) which seems a reasonable resolution
		OutResult = Query->Result / Div;
	}
	else
	{
		OutResult = Query->Result;
	}
	return bSuccess;
}


// Occlusion/Timer queries.
void FD3D11DynamicRHI::RHIBeginRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	FD3D11RenderQuery* Query = ResourceCast(QueryRHI);

	if(Query->QueryType == RQT_Occlusion)
	{
		++ActualOcclusionQueriesInBatch;
		Query->bResultIsCached = false;
		d3d_dc_->Begin(Query->Resource);
	}
	else
	{
		// not supported/needed for RQT_AbsoluteTime
		check(0);
	}
}

void FD3D11DynamicRHI::RHIEndRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	FD3D11RenderQuery* Query = ResourceCast(QueryRHI);
	Query->bResultIsCached = false; // for occlusion queries, this is redundant with the one in begin
	d3d_dc_->End(Query->Resource);

	//@todo - d3d debug spews warnings about OQ's that are being issued but not polled, need to investigate
}

bool FD3D11DynamicRHI::GetQueryData(ID3D11Query* Query,void* Data,SIZE_T DataSize,bool bWait, ERenderQueryType QueryType)
{
	// Request the data from the query.
	HRESULT Result = d3d_dc_->GetData(Query,Data,DataSize,0);

	// Isn't the query finished yet, and can we wait for it?
	if ( Result == S_FALSE && bWait )
	{
		SCOPE_CYCLE_COUNTER( STAT_RenderQueryResultTime );
		uint32 IdleStart = FPlatformTime::Cycles();
		double StartTime = FPlatformTime::Seconds();
		do 
		{
			Result = d3d_dc_->GetData(Query,Data,DataSize,0);

			// timer queries are used for Benchmarks which can stall a bit more
			double TimeoutValue = (QueryType == RQT_AbsoluteTime) ? 2.0 : 0.5;

			if((FPlatformTime::Seconds() - StartTime) > TimeoutValue)
			{
				//UE_LOG(LogD3D11RHI, Log, TEXT("Timed out while waiting for GPU to catch up. (%.1f s)"), TimeoutValue);
				LOG_INFO(StringFormat("Timed out while waiting for GPU to catch up. (%.1f s)",TimeoutValue));
				return false;
			}
		} while ( Result == S_FALSE );
		GRenderThreadIdle[ERenderThreadIdleTypes::WaitingForGPUQuery] += FPlatformTime::Cycles() - IdleStart;
		GRenderThreadNumIdle[ERenderThreadIdleTypes::WaitingForGPUQuery]++;
	}

	if( Result == S_OK )
	{
		return true;
	}
	else if(Result == S_FALSE && !bWait)
	{
		// Return failure if the query isn't complete, and waiting wasn't requested.
		return false;
	}
	else
	{
		VERIFYD3D11RESULT_EX(Result, d3d_device_);
		return false;
	}
}

void FD3D11EventQuery::IssueEvent()
{
	D3DRHI->GetDeviceContext()->End(Query);
}

void FD3D11EventQuery::WaitForCompletion()
{
	BOOL bRenderingIsFinished = false;
	while(
		D3DRHI->GetQueryData(Query,&bRenderingIsFinished,sizeof(bRenderingIsFinished),true,RQT_Undefined) &&
		!bRenderingIsFinished
		)
	{};
}

void FD3D11EventQuery::InitDynamicRHI()
{
	D3D11_QUERY_DESC QueryDesc;
	QueryDesc.Query = D3D11_QUERY_EVENT;
	QueryDesc.MiscFlags = 0;
	VERIFYD3D11RESULT_EX(D3DRHI->GetDevice()->CreateQuery(&QueryDesc,Query.GetInitReference()), D3DRHI->GetDevice());

	// Initialize the query by issuing an initial event.
	IssueEvent();
}
void FD3D11EventQuery::ReleaseDynamicRHI()
{
	Query = NULL;
}

/*=============================================================================
 * class FD3D11BufferedGPUTiming
 *=============================================================================*/

/**
 * Constructor.
 *
 * @param InD3DRHI			RHI interface
 * @param InBufferSize		Number of buffered measurements
 */
FD3D11BufferedGPUTiming::FD3D11BufferedGPUTiming( FD3D11DynamicRHI* InD3DRHI, int32 InBufferSize )
:	D3DRHI( InD3DRHI )
,	BufferSize( InBufferSize )
,	CurrentTimestamp( -1 )
,	NumIssuedTimestamps( 0 )
,	StartTimestamps( NULL )
,	EndTimestamps( NULL )
,	bIsTiming( false )
{
}

/**
 * Initializes the static variables, if necessary.
 */
void FD3D11BufferedGPUTiming::PlatformStaticInitialize(void* UserData)
{
	// Are the static variables initialized?
	check( !GAreGlobalsInitialized );

	// Get the GPU timestamp frequency.
	GTimingFrequency = 0;
	TRefCountPtr<ID3D11Query> FreqQuery;
	FD3D11DynamicRHI* D3DRHI = (FD3D11DynamicRHI*)UserData;
	ID3D11DeviceContext *D3D11DeviceContext = D3DRHI->GetDeviceContext();
	HRESULT D3DResult;

	D3D11_QUERY_DESC QueryDesc;
	QueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	QueryDesc.MiscFlags = 0;

	{
		// to track down some rare event where GTimingFrequency is 0 or <1000*1000
		uint32 DebugState = 0;
		uint32 DebugCounter = 0;

		D3DResult = D3DRHI->GetDevice()->CreateQuery(&QueryDesc, FreqQuery.GetInitReference());
		if (D3DResult == S_OK)
		{
			DebugState = 1;
			D3D11DeviceContext->Begin(FreqQuery);
			D3D11DeviceContext->End(FreqQuery);

			D3D11_QUERY_DATA_TIMESTAMP_DISJOINT FreqQueryData;

			D3DResult = D3D11DeviceContext->GetData(FreqQuery, &FreqQueryData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);
			double StartTime = FPlatformTime::Seconds();
			while (D3DResult == S_FALSE && (FPlatformTime::Seconds() - StartTime) < 0.5f)
			{
				++DebugCounter;
				FPlatformProcess::Sleep(0.005f);
				D3DResult = D3D11DeviceContext->GetData(FreqQuery, &FreqQueryData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);
			}

			if (D3DResult == S_OK)
			{
				DebugState = 2;
				GTimingFrequency = FreqQueryData.Frequency;
				checkSlow(!FreqQueryData.Disjoint);

				if (FreqQueryData.Disjoint)
				{
					DebugState = 3;
				}
			}
		}

		//UE_LOG(LogD3D11RHI, Log, TEXT("GPU Timing Frequency: %f (Debug: %d %d)"), GTimingFrequency / (double)(1000 * 1000), DebugState, DebugCounter);
		LOG_INFO(StringFormat("GPU Timing Frequency: %f (Debug: %d %d)", GTimingFrequency / (double)(1000 * 1000), DebugState, DebugCounter));
	}

	FreqQuery = NULL;
}

void FD3D11BufferedGPUTiming::CalibrateTimers(FD3D11DynamicRHI* InD3DRHI)
{
	// Attempt to generate a timestamp on GPU and CPU as closely to each other as possible.
	// This works by first flushing any pending GPU work, then writing a GPU timestamp and waiting for GPU to finish.
	// CPU timestamp is continuously captured while we are waiting on GPU.

	HRESULT D3DResult = E_FAIL;

	TRefCountPtr<ID3D11Query> DisjointQuery;
	{
		D3D11_QUERY_DESC QueryDesc;
		QueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		QueryDesc.MiscFlags = 0;
		D3DResult = InD3DRHI->GetDevice()->CreateQuery(&QueryDesc, DisjointQuery.GetInitReference());

		if (D3DResult != S_OK) return;
	}

	TRefCountPtr<ID3D11Query> TimestampQuery;
	{
		D3D11_QUERY_DESC QueryDesc;
		QueryDesc.Query = D3D11_QUERY_TIMESTAMP;
		QueryDesc.MiscFlags = 0;
		D3DResult = InD3DRHI->GetDevice()->CreateQuery(&QueryDesc, TimestampQuery.GetInitReference());

		if (D3DResult != S_OK) return;
	}

	TRefCountPtr<ID3D11Query> PendingWorkDoneQuery;
	TRefCountPtr<ID3D11Query> TimestampDoneQuery;
	{
		D3D11_QUERY_DESC QueryDesc;
		QueryDesc.Query = D3D11_QUERY_EVENT;
		QueryDesc.MiscFlags = 0;

		D3DResult = InD3DRHI->GetDevice()->CreateQuery(&QueryDesc, PendingWorkDoneQuery.GetInitReference());
		if (D3DResult != S_OK) return;

		D3DResult = InD3DRHI->GetDevice()->CreateQuery(&QueryDesc, TimestampDoneQuery.GetInitReference());
		if (D3DResult != S_OK) return;
	}

	ID3D11DeviceContext* D3D11DeviceContext = InD3DRHI->GetDeviceContext();

	// Flush any currently pending GPU work and wait for it to finish
	D3D11DeviceContext->End(PendingWorkDoneQuery);
	D3D11DeviceContext->Flush();

	for(;;)
	{
		BOOL EventComplete = false;
		D3D11DeviceContext->GetData(PendingWorkDoneQuery, &EventComplete, sizeof(EventComplete), 0);
		if (EventComplete) break;
		FPlatformProcess::Sleep(0.001f);
	}

	const uint32 MaxCalibrationAttempts = 10;
	for (uint32 CalibrationAttempt = 0; CalibrationAttempt < MaxCalibrationAttempts; ++CalibrationAttempt)
	{
		D3D11DeviceContext->Begin(DisjointQuery);
		D3D11DeviceContext->End(TimestampQuery);
		D3D11DeviceContext->End(DisjointQuery);
		D3D11DeviceContext->End(TimestampDoneQuery);

		D3D11DeviceContext->Flush();

		uint64 CPUTimestamp = 0;
		uint64 GPUTimestamp = 0;

		// Busy-wait for GPU to finish and capture CPU timestamp approximately when GPU work is done
		for (;;)
		{
			BOOL EventComplete = false;
			CPUTimestamp = FPlatformTime::Cycles64();
			D3D11DeviceContext->GetData(TimestampDoneQuery, &EventComplete, sizeof(EventComplete), 0);
			if (EventComplete) break;
		}

		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointQueryData = {};
		D3DResult = D3D11DeviceContext->GetData(DisjointQuery, &DisjointQueryData, sizeof(DisjointQueryData), 0);

		// If timestamp was unreliable, try again
		if (D3DResult != S_OK || DisjointQueryData.Disjoint)
		{
			continue;
		}

		D3DResult = D3D11DeviceContext->GetData(TimestampQuery, &GPUTimestamp, sizeof(GPUTimestamp), 0);

		// If we managed to get valid timestamps, save both of them (CPU & GPU) and return
		if (D3DResult == S_OK && GPUTimestamp)
		{
			GCalibrationTimestamp.CPUMicroseconds = uint64(FPlatformTime::ToSeconds64(CPUTimestamp) * 1e6);
			GCalibrationTimestamp.GPUMicroseconds = uint64(GPUTimestamp * (1e6 / GTimingFrequency));
			break;
		}
		else
		{
			continue;
		}
	}
}

void FD3D11DynamicRHI::RHICalibrateTimers()
{
	check(IsInRenderingThread());

	FScopedRHIThreadStaller StallRHIThread(FRHICommandListExecutor::GetImmediateCommandList());

	FD3D11BufferedGPUTiming::CalibrateTimers(this);
}

/**
 * Initializes all D3D resources and if necessary, the static variables.
 */
void FD3D11BufferedGPUTiming::InitDynamicRHI()
{
	StaticInitialize(D3DRHI, PlatformStaticInitialize);

	CurrentTimestamp = 0;
	NumIssuedTimestamps = 0;
	bIsTiming = false;

	// Now initialize the queries for this timing object.
	if ( GIsSupported )
	{
		StartTimestamps = new TRefCountPtr<ID3D11Query>[ BufferSize ];
		EndTimestamps = new TRefCountPtr<ID3D11Query>[ BufferSize ];
		for ( int32 TimestampIndex = 0; TimestampIndex < BufferSize; ++TimestampIndex )
		{
			HRESULT D3DResult;

			D3D11_QUERY_DESC QueryDesc;
			QueryDesc.Query = D3D11_QUERY_TIMESTAMP;
			QueryDesc.MiscFlags = 0;

			D3DResult = D3DRHI->GetDevice()->CreateQuery(&QueryDesc,StartTimestamps[TimestampIndex].GetInitReference());
			GIsSupported = GIsSupported && (D3DResult == S_OK);
			D3DResult = D3DRHI->GetDevice()->CreateQuery(&QueryDesc,EndTimestamps[TimestampIndex].GetInitReference());
			GIsSupported = GIsSupported && (D3DResult == S_OK);
		}
	}
}

/**
 * Releases all D3D resources.
 */
void FD3D11BufferedGPUTiming::ReleaseDynamicRHI()
{
	if ( StartTimestamps && EndTimestamps )
	{
		for ( int32 TimestampIndex = 0; TimestampIndex < BufferSize; ++TimestampIndex )
		{
			StartTimestamps[TimestampIndex] = NULL;
			EndTimestamps[TimestampIndex] = NULL;
		}
		delete [] StartTimestamps;
		delete [] EndTimestamps;
		StartTimestamps = NULL;
		EndTimestamps = NULL;
	}
}

/**
 * Start a GPU timing measurement.
 */
void FD3D11BufferedGPUTiming::StartTiming()
{
	// Issue a timestamp query for the 'start' time.
	if ( GIsSupported && !bIsTiming )
	{
		int32 NewTimestampIndex = (CurrentTimestamp + 1) % BufferSize;
		D3DRHI->GetDeviceContext()->End(StartTimestamps[NewTimestampIndex]);
		CurrentTimestamp = NewTimestampIndex;
		bIsTiming = true;
	}
}

/**
 * End a GPU timing measurement.
 * The timing for this particular measurement will be resolved at a later time by the GPU.
 */
void FD3D11BufferedGPUTiming::EndTiming()
{
	// Issue a timestamp query for the 'end' time.
	if ( GIsSupported && bIsTiming )
	{
		checkSlow( CurrentTimestamp >= 0 && CurrentTimestamp < BufferSize );
		D3DRHI->GetDeviceContext()->End(EndTimestamps[CurrentTimestamp]);
		NumIssuedTimestamps = YMath::Min<int32>(NumIssuedTimestamps + 1, BufferSize);
		bIsTiming = false;
	}
}

/**
 * Retrieves the most recently resolved timing measurement.
 * The unit is the same as for FPlatformTime::Cycles(). Returns 0 if there are no resolved measurements.
 *
 * @return	Value of the most recently resolved timing, or 0 if no measurements have been resolved by the GPU yet.
 */
uint64 FD3D11BufferedGPUTiming::GetTiming(bool bGetCurrentResultsAndBlock)
{
	if ( GIsSupported )
	{
		checkSlow( CurrentTimestamp >= 0 && CurrentTimestamp < BufferSize );
		uint64 StartTime, EndTime;
		HRESULT D3DResult;

		int32 TimestampIndex = CurrentTimestamp;
		if (!bGetCurrentResultsAndBlock)
		{
			// Quickly check the most recent measurements to see if any of them has been resolved.  Do not flush these queries.
			for ( int32 IssueIndex = 1; IssueIndex < NumIssuedTimestamps; ++IssueIndex )
			{
				D3DResult = D3DRHI->GetDeviceContext()->GetData(EndTimestamps[TimestampIndex],&EndTime,sizeof(EndTime),D3D11_ASYNC_GETDATA_DONOTFLUSH);
				if ( D3DResult == S_OK )
				{
					D3DResult = D3DRHI->GetDeviceContext()->GetData(StartTimestamps[TimestampIndex],&StartTime,sizeof(StartTime),D3D11_ASYNC_GETDATA_DONOTFLUSH);
					if ( D3DResult == S_OK && EndTime > StartTime)
					{
						return EndTime - StartTime;
					}
				}

				TimestampIndex = (TimestampIndex + BufferSize - 1) % BufferSize;
			}
		}
		
		if ( NumIssuedTimestamps > 0 || bGetCurrentResultsAndBlock )
		{
			// None of the (NumIssuedTimestamps - 1) measurements were ready yet,
			// so check the oldest measurement more thoroughly.
			// This really only happens if occlusion and frame sync event queries are disabled, otherwise those will block until the GPU catches up to 1 frame behind
			const bool bBlocking = ( NumIssuedTimestamps == BufferSize ) || bGetCurrentResultsAndBlock;
			const uint32 AsyncFlags = bBlocking ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH;
			uint32 IdleStart = FPlatformTime::Cycles();
			double StartTimeoutTime = FPlatformTime::Seconds();

			SCOPE_CYCLE_COUNTER( STAT_RenderQueryResultTime );
			// If we are blocking, retry until the GPU processes the time stamp command
			do 
			{
				D3DResult = D3DRHI->GetDeviceContext()->GetData( EndTimestamps[TimestampIndex], &EndTime, sizeof(EndTime), AsyncFlags );

				if ((FPlatformTime::Seconds() - StartTimeoutTime) > 0.5)
				{
					//UE_LOG(LogD3D11RHI, Log, TEXT("Timed out while waiting for GPU to catch up. (500 ms)"));
					LOG_INFO("Timed out while waiting for GPU to catch up. (500 ms)");
					return 0;
				}
			} while ( D3DResult == S_FALSE && bBlocking );
			GRenderThreadIdle[ERenderThreadIdleTypes::WaitingForGPUQuery] += FPlatformTime::Cycles() - IdleStart;
			GRenderThreadNumIdle[ERenderThreadIdleTypes::WaitingForGPUQuery]++;
			if ( D3DResult == S_OK )
			{
				IdleStart = FPlatformTime::Cycles();
				StartTimeoutTime = FPlatformTime::Seconds();
				do 
				{
					D3DResult = D3DRHI->GetDeviceContext()->GetData( StartTimestamps[TimestampIndex], &StartTime, sizeof(StartTime), AsyncFlags );

					if ((FPlatformTime::Seconds() - StartTimeoutTime) > 0.5)
					{
						LOG_INFO("Timed out while waiting for GPU to catch up. (500 ms)");
						return 0;
					}
				} while ( D3DResult == S_FALSE && bBlocking );
				GRenderThreadIdle[ERenderThreadIdleTypes::WaitingForGPUQuery] += FPlatformTime::Cycles() - IdleStart;

				if ( D3DResult == S_OK && EndTime > StartTime )
				{
					return EndTime - StartTime;
				}
			}
		}
	}
	return 0;
}

FD3D11DisjointTimeStampQuery::FD3D11DisjointTimeStampQuery(class FD3D11DynamicRHI* InD3DRHI) :
	D3DRHI(InD3DRHI)
{

}

void FD3D11DisjointTimeStampQuery::StartTracking()
{
	ID3D11DeviceContext* D3D11DeviceContext = D3DRHI->GetDeviceContext();
	D3D11DeviceContext->Begin(DisjointQuery);
}

void FD3D11DisjointTimeStampQuery::EndTracking()
{
	ID3D11DeviceContext* D3D11DeviceContext = D3DRHI->GetDeviceContext();
	D3D11DeviceContext->End(DisjointQuery);
}

bool FD3D11DisjointTimeStampQuery::IsResultValid()
{
	return GetResult().Disjoint == 0;
}

D3D11_QUERY_DATA_TIMESTAMP_DISJOINT FD3D11DisjointTimeStampQuery::GetResult()
{
	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointQueryData;

	ID3D11DeviceContext* D3D11DeviceContext = D3DRHI->GetDeviceContext();
	HRESULT D3DResult = D3D11DeviceContext->GetData(DisjointQuery, &DisjointQueryData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);

	const double StartTime = FPlatformTime::Seconds();
	while (D3DResult == S_FALSE && (FPlatformTime::Seconds() - StartTime) < 0.5)
	{
		FPlatformProcess::Sleep(0.005f);
		D3DResult = D3D11DeviceContext->GetData(DisjointQuery, &DisjointQueryData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0);
	}

	return DisjointQueryData;
}

void FD3D11DisjointTimeStampQuery::InitDynamicRHI()
{
	D3D11_QUERY_DESC QueryDesc;
	QueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	QueryDesc.MiscFlags = 0;

	VERIFYD3D11RESULT_EX(D3DRHI->GetDevice()->CreateQuery(&QueryDesc, DisjointQuery.GetInitReference()), D3DRHI->GetDevice());
}

void FD3D11DisjointTimeStampQuery::ReleaseDynamicRHI()
{

}
