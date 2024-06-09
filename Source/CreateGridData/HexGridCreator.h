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
	CreateVertices,
	CreateTriangles,
	WriteTiles,
	WriteTileIndices,
	WriteVertices,
	WriteTriangles,
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Params", meta = (ClampMin = "0.0", ClampMax = "1.0"))
		float GridLineRatio = 0.1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Params", meta = (ClampMin = "1"))
		int32 NeighborRange = 10;

	//Path
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
		FString TilesDataPath = FString(TEXT("Data/Tiles.data"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
		FString TileIndicesDataPath = FString(TEXT("Data/TileIndices.data"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
		FString VerticesDataPath = FString(TEXT("Data/Vertices.data"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Path")
		FString TrianglesDataPath = FString(TEXT("Data/Triangles.data"));

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
		FStructLoopData CreateVerticesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
		FStructLoopData CreateTrianglesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
		FStructLoopData WriteTilesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
		FStructLoopData WriteTileIndicesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
		FStructLoopData WriteVerticesLoopData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom|Loop")
		FStructLoopData WriteTrianglesLoopData;

	//Render variables
	UPROPERTY(BlueprintReadOnly)
		TArray<FVector> Vertices;
	UPROPERTY(BlueprintReadOnly)
		TArray<int32> Triangles;

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

	//Create vertices
	void CreateVertices();
	void InitCreateVertices();
	void CreateTileLineVertices(int32 TileIndex);

	//Create triangles
	void CreateTriangles();
	void InitCreateTriangles(TArray<int32>& Arr0, TArray<int32>& Arr1, TArray<int32>& Arr2, TArray<int32>& Arr3);
	void CreateTileTriangles(int32 VIndexStart, const TArray<int32>& Arr0, const TArray<int32>& Arr1, const TArray<int32>& Arr2, const TArray<int32>& Arr3);

	//For write data
	void CreateFilePath(const FString& RelPath, FString& FullPath);
	void WritePipeDelimiter(std::ofstream& ofs);
	void WriteColonDelimiter(std::ofstream& ofs);
	void WriteLineEnd(std::ofstream& ofs);

	//Write hex tiles data to file
	void WriteTilesToFile();
	void WriteTiles(std::ofstream& ofs);
	void WriteTileLine(std::ofstream& ofs, int32 Index);
	void WriteAxialCoord(std::ofstream& ofs, const FStructHexTileData& Data);
	void WritePosition2D(std::ofstream& ofs, const FStructHexTileData& Data);
	void WriteNeighbors(std::ofstream& ofs, const FStructHexTileData& Data);
	void WriteNeighborsInfo(std::ofstream& ofs, const FStructHexTileNeighbors& Neighbors);

	//Write tile indices data to file
	void WriteTileIndicesToFile();
	void WriteTileIndices(std::ofstream& ofs);
	void WriteTileIndicesLine(std::ofstream& ofs, int32 Index);
	void WriteIndicesKey(std::ofstream& ofs, const FIntPoint& key);
	void WriteIndicesValue(std::ofstream& ofs, int32 Index);

	//Write vertices data to file
	void WriteVerticesToFile();
	void WriteVertices(std::ofstream& ofs);
	void WriteVerticesLine(std::ofstream& ofs, int32 Index);
	void WriteVertex(std::ofstream& ofs, const FVector& Vertex);

	//Write triangles data to file
	void WriteTrianglesToFile();
	void WriteTriangles(std::ofstream& ofs);
	void WriteTrianglesLine(std::ofstream& ofs, int32 Index);
	void WriteTriangleVertex(std::ofstream& ofs, int32 Data);

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
