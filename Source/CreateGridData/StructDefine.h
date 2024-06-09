#pragma once

#include "StructDefine.generated.h"

USTRUCT(BlueprintType)
struct FStructHexTileNeighbors
{
	GENERATED_BODY()

	UPROPERTY()
		int32 Radius;

	UPROPERTY()
		TArray<FIntPoint> Tiles;
};

USTRUCT(BlueprintType)
struct FStructHexTileData
{
	GENERATED_BODY()

	UPROPERTY()
		FIntPoint AxialCoord;

	UPROPERTY()
		FVector2D Position2D;

	UPROPERTY()
		TArray<FStructHexTileNeighbors> Neighbors;

};

USTRUCT(BlueprintType)
struct FStructLoopData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		int32 LoopCountLimit = 3000;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float Rate = 0.01f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "1"))
		int32 LoopDepthLimit = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		TArray<int32> IndexSaved = {};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		bool IsInitialized = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0"))
		int32 Count = 0;

};
