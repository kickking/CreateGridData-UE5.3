// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class CREATEGRIDDATA_API FlowControlUtility
{
public:
	FlowControlUtility();
	~FlowControlUtility();

	static void InitLoopData(struct FStructLoopData& InOut_Data);
	static void SaveLoopData(AActor* Owner, struct FStructLoopData& InOut_Data, int32 Count, const TArray<int32>& Indices,
		const FTimerDynamicDelegate TimerDelegate, bool& Out_Success);
};
