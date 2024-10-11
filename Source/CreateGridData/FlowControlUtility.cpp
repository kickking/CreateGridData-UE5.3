// Fill out your copyright notice in the Description page of Project Settings.


#include "FlowControlUtility.h"
#include "StructDefine.h"
#include <GameFramework/Actor.h>
#include <TimerManager.h>
#include <kismet/KismetSystemLibrary.h>
#include <Containers/UnrealString.h>

FlowControlUtility::FlowControlUtility()
{
}

FlowControlUtility::~FlowControlUtility()
{
}

void FlowControlUtility::InitLoopData(FStructLoopData& InOut_Data)
{
	InOut_Data.IndexSaved.Empty();
	for (int32 i = 0; i < InOut_Data.LoopDepthLimit; i++)
	{
		InOut_Data.IndexSaved.Add(0);
	}
	InOut_Data.IsInitialized = false;
	InOut_Data.Count = 0;
}

void FlowControlUtility::SaveLoopData(AActor* Owner, FStructLoopData& InOut_Data, int32 Count, const TArray<int32>& Indices,
	const FTimerDynamicDelegate TimerDelegate, bool& Out_Success)
{
	if (Count > InOut_Data.LoopCountLimit) {
		for (int32 i = 0; i < Indices.Num() && i < InOut_Data.LoopDepthLimit; i++)
		{
			InOut_Data.IndexSaved[i] = Indices[i];
		}
		FTimerHandle TimerHandle;
		Owner->GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, InOut_Data.Rate, false);
		Out_Success = true;
	}
	else {
		InOut_Data.Count++;
		Out_Success = false;

	}
}