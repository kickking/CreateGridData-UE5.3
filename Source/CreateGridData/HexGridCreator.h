// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "StructDefine.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexGridCreator.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(HexGridCreator, Log, All);

UENUM(BlueprintType)
enum class Enum_HexGridWorkflowState : uint8
{
	InitWorkflow,
	SpiralCreateCenter,
	SpiralCreateNeighbors,
	WriteTiles,
	WriteTilesNeighbor,
	WriteTileIndices,
	WriteParams,
	Done,
	Error
};

#define RING_START_DIRECTION_INDEX	4

UCLASS()
class CREATEGRIDDATA_API AHexGridCreator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHexGridCreator();

private:
	//Delimiter
	FString PipeDelim = FString(TEXT("|"));
	FString CommaDelim = FString(TEXT(","));
	FString SpaceDelim = FString(TEXT(" "));
	FString ColonDelim = FString(TEXT(":"));

	FVector QDirection;
	FVector RDirection;
	FVector SDirection;

	TArray<FVector2D> NeighborDirVectors;

	float TileWidth;
	float TileHeight;

	//delegate
	FTimerDynamicDelegate WorkflowDelegate;

	TArray<FStructHexTileData> Tiles;
	TMap<FIntPoint, int32> TileIndices;

	//Flag for spiral ring
	bool RingInitFlag = false;

	TArray<FIntPoint> AxialDirectionVectors;

	//Save temp data for SpiralCreateCenter and SpiralCreateNeighbors
	FVector2D TmpPosition2D;
	FIntPoint TmpHex;

	//Temp data for create vertices
	TArray<FVector> OuterVectors;
	TArray<FVector> InnerVectors;

	//Temp data for create triangles
	TArray<int32> TriArr0, TriArr1, TriArr2, TriArr3;

protected:
	//Params
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Params", meta = (ClampMin = "0.0"))
		float TileSize = 500.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Params", meta = (ClampMin = "1"))
		int32 GridRange = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Params", meta = (ClampMin = "1"))
		int32 NeighborRange = 5;

	//Path
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
		FString TilesDataPath = FString(TEXT("Data/Tiles.data"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
		FString TilesNeighborPathPrefix = FString(TEXT("Data/N"));
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
		FString TileIndicesDataPath = FString(TEXT("Data/TileIndices.data"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
		FString ParamsDataPath = FString(TEXT("Data/Params.data"));

	//Timer
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Timer")
		float DefaultTimerRate = 0.01f;

	//Loop BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
		FStructLoopData SpiralCreateCenterLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
		FStructLoopData SpiralCreateNeighborsLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
		FStructLoopData WriteTilesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
		FStructLoopData WriteNeighborsLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
		FStructLoopData WriteTileIndicesLoopData;

	//Workflow
	UPROPERTY(BlueprintReadOnly)
		Enum_HexGridWorkflowState WorkflowState = Enum_HexGridWorkflowState::InitWorkflow;


	UPROPERTY(BlueprintReadOnly)
		int32 ProgressTarget = 0;
	UPROPERTY(BlueprintReadOnly)
		int32 ProgressCurrent = 0;

private:
	//Timer delegate
	void BindDelegate();

	//Init
	void InitDirection();
	void InitTileParams();
	void InitLoopData();

	//Workflow
	UFUNCTION()
	void CreateHexGridFlow();

	//Init workflow
	void InitWorkflow();

	//Hex axial coordinate function
	void InitAxialDirections();
	FIntPoint AxialAdd(const FIntPoint& Hex, const FIntPoint& Vec);
	FIntPoint AxialDirection(const int32 Direction);
	FIntPoint AxialNeighbor(const FIntPoint& Hex, const int32 Direction);
	FIntPoint AxialScale(const FIntPoint& Hex, const int32 Factor);

	void ResetProgress();

	//Create center
	void InitGridCenter();
	void SpiralCreateCenter();
	void AddRingTileAndIndex();
	void FindNeighborTileOfRing(int32 DirIndex);

	//Create neighbors
	void SpiralCreateNeighbors();
	void AddTileNeighbor(int32 TileIndex, int32 Radius);
	void SetTileNeighbor(int32 TileIndex, int32 Radius, int32 DirIndex);

	//For write data
	void CreateFilePath(const FString& RelPath, FString& FullPath);
	void WritePipeDelimiter(std::ofstream& ofs);
	void WriteColonDelimiter(std::ofstream& ofs);
	void WriteLineEnd(std::ofstream& ofs);

	//Write hex tiles data to file
	void WriteTilesToFile();
	void WriteTiles(std::ofstream& ofs);
	void WriteTileLine(std::ofstream& ofs, int32 Index);
	void WriteIndices(std::ofstream& ofs, int32 Index);
	void WriteAxialCoord(std::ofstream& ofs, const FStructHexTileData& Data);
	void WritePosition2D(std::ofstream& ofs, const FStructHexTileData& Data);
	//void WriteNeighbors(std::ofstream& ofs, const FStructHexTileData& Data);
	//void WriteNeighborsInfo(std::ofstream& ofs, const FStructHexTileNeighbors& Neighbors);

	//Write neighbors to file
	void WriteNeighborsToFile();
	void CreateNeighborPath(FString& NeighborPath, int32 Radius);
	int32 CalNeighborsWeight(int32 Range);
	bool WriteNeighbors(std::ofstream& ofs, int32 Radius);
	void WriteNeighborLine(std::ofstream& ofs, int32 Index, int32 Radius);

	//Write tile indices data to file
	void WriteTileIndicesToFile();
	void WriteTileIndices(std::ofstream& ofs);
	void WriteTileIndicesLine(std::ofstream& ofs, int32 Index);
	void WriteIndicesKey(std::ofstream& ofs, const FIntPoint& key);
	void WriteIndicesValue(std::ofstream& ofs, int32 Index);

	//Write info data to file
	void WriteParamsToFile();
	void WriteParams(std::ofstream& ofs);
	void WriteParamsContent(std::ofstream& ofs);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void GetProgress(float& Out_Progress);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
