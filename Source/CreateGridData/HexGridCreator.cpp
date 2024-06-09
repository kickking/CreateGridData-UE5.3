// Fill out your copyright notice in the Description page of Project Settings.


#include "HexGridCreator.h"
#include "FlowControlUtility.h"

#include <Kismet/KismetTextLibrary.h>
#include <Math/UnrealMathUtility.h>
#include <TimerManager.h>

#include <iostream>
#include <fstream>
#include <filesystem>

DEFINE_LOG_CATEGORY(HexGridCreator);

// Sets default values
AHexGridCreator::AHexGridCreator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

	BindDelegate();
}

// Called when the game starts or when spawned
void AHexGridCreator::BeginPlay()
{
	Super::BeginPlay();

	WorkflowState = Enum_HexGridWorkflowState::InitWorkflow;
	CreateHexGridFlow();
	
}

// Called every frame
void AHexGridCreator::Tick(float DeltaTime)
{
	//Super::Tick(DeltaTime);

}

void AHexGridCreator::BindDelegate()
{
	WorkflowDelegate.BindUFunction(Cast<UObject>(this), TEXT("CreateHexGridFlow"));
}

void AHexGridCreator::InitWorkflow()
{
	InitDirection();
	InitTileParams();
	InitLoopData();
	InitAxialDirections();

	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::SpiralCreateCenter;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
	UE_LOG(HexGridCreator, Log, TEXT("Init workflow done."));
}

void AHexGridCreator::InitDirection()
{
	//Init Q,R,S
	QDirection.Set(1.0, 0.0, 0.0);
	FVector ZAxis(0.0, 0.0, 1.0);
	FVector Vec = QDirection.RotateAngleAxis(120.0, ZAxis);
	RDirection.Set(Vec.X, Vec.Y, Vec.Z);
	Vec = RDirection.RotateAngleAxis(120.0, ZAxis);
	SDirection.Set(Vec.X, Vec.Y, Vec.Z);

	//Init Neighbor Directions
	Vec.Set(1.0, 0.0, 0.0);
	Vec = Vec.RotateAngleAxis(30.0, ZAxis);
	float Angle;
	FVector NDir;
	for (int32 i = 0; i <= 5; i++)
	{
		Angle = i * (-60.0);
		NDir = Vec.RotateAngleAxis(Angle, ZAxis);
		NeighborDirVectors.Add(FVector2D(NDir.X, NDir.Y));
	}
}

void AHexGridCreator::InitTileParams()
{
	TileWidth = TileSize * 2.0;
	TileHeight = TileSize * FMath::Sqrt(3.0);

}

void AHexGridCreator::InitLoopData()
{
	FlowControlUtility::InitLoopData(SpiralCreateCenterLoopData);
	SpiralCreateCenterLoopData.IndexSaved[0] = 1;
	FlowControlUtility::InitLoopData(SpiralCreateNeighborsLoopData);
	SpiralCreateNeighborsLoopData.IndexSaved[1] = 1;
	FlowControlUtility::InitLoopData(CreateVerticesLoopData);
	FlowControlUtility::InitLoopData(CreateTrianglesLoopData);
	FlowControlUtility::InitLoopData(WriteTilesLoopData);
	FlowControlUtility::InitLoopData(WriteTileIndicesLoopData);
	FlowControlUtility::InitLoopData(WriteVerticesLoopData);
	FlowControlUtility::InitLoopData(WriteTrianglesLoopData);
}

void AHexGridCreator::CreateHexGridFlow()
{
	switch (WorkflowState)
	{
	case Enum_HexGridWorkflowState::InitWorkflow:
		InitWorkflow();
		break;
	case Enum_HexGridWorkflowState::SpiralCreateCenter:
		SpiralCreateCenter();
		break;
	case Enum_HexGridWorkflowState::SpiralCreateNeighbors:
		SpiralCreateNeighbors();
		break;
	case Enum_HexGridWorkflowState::CreateVertices:
		CreateVertices();
		break;
	case Enum_HexGridWorkflowState::CreateTriangles:
		CreateTriangles();
		break;
	case Enum_HexGridWorkflowState::WriteTiles:
		WriteTilesToFile();
		break;
	case Enum_HexGridWorkflowState::WriteTileIndices:
		WriteTileIndicesToFile();
		break;
	case Enum_HexGridWorkflowState::WriteVertices:
		WriteVerticesToFile();
		break;
	case Enum_HexGridWorkflowState::WriteTriangles:
		WriteTrianglesToFile();
		break;
	case Enum_HexGridWorkflowState::WriteParams:
		WriteParamsToFile();
		break;
	case Enum_HexGridWorkflowState::Done:
		break;
	case Enum_HexGridWorkflowState::Error:
		UE_LOG(HexGridCreator, Warning, TEXT("CreateHexGridFlow Error!"));
		break;
	default:
		break;
	}

}

void AHexGridCreator::InitAxialDirections()
{
	AxialDirectionVectors.Add(FIntPoint(1.0, 0.0));
	AxialDirectionVectors.Add(FIntPoint(1.0, -1.0));
	AxialDirectionVectors.Add(FIntPoint(0.0, -1.0));
	AxialDirectionVectors.Add(FIntPoint(-1.0, 0.0));
	AxialDirectionVectors.Add(FIntPoint(-1.0, 1.0));
	AxialDirectionVectors.Add(FIntPoint(0.0, 1.0));
}

FIntPoint AHexGridCreator::AxialAdd(const FIntPoint& Hex, const FIntPoint& Vec)
{
	return Hex + Vec;
}

FIntPoint AHexGridCreator::AxialDirection(const int32 Direction)
{
	return AxialDirectionVectors[Direction];
}

FIntPoint AHexGridCreator::AxialNeighbor(const FIntPoint& Hex, const int32 Direction)
{
	return AxialAdd(Hex, AxialDirection(Direction));
}

FIntPoint AHexGridCreator::AxialScale(const FIntPoint& Hex, const int32 Factor)
{
	return Hex * Factor;
}

void AHexGridCreator::ResetProgress()
{
	ProgressTarget = 0;
	ProgressCurrent = 0;
}

void AHexGridCreator::GetProgress(float& Out_Progress)
{
	float Rate;
	if (ProgressTarget == 0) {
		Out_Progress = 0;
	}
	else {
		Rate = float(ProgressCurrent) / float(ProgressTarget);
		Rate = Rate > 1.0 ? 1.0 : Rate;
		Out_Progress = Rate;
	}
}

void AHexGridCreator::SpiralCreateCenter()
{
	bool OnceLoop0 = true;
	bool OnceLoop1 = true;
	int32 Count = 0;
	TArray<int32> Indices = { 0, 0, 0 };
	bool SaveLoopFlag = false;

	if (!SpiralCreateCenterLoopData.IsInitialized) {
		SpiralCreateCenterLoopData.IsInitialized = true;
		RingInitFlag = false;
		InitGridCenter();
		ProgressTarget = 6 * (1 + GridRange) * GridRange / 2;
	}

	int32 i = SpiralCreateCenterLoopData.IndexSaved[0];
	i = i < 1 ? 1 : i;
	int32 j, k;

	for (; i <= GridRange; i++)
	{
		Indices[0] = i;
		if (!RingInitFlag) {
			//Init position2D
			RingInitFlag = true;
			FVector2D Vec = i * TileHeight * NeighborDirVectors[RING_START_DIRECTION_INDEX];
			TmpPosition2D.Set(Vec.X, Vec.Y);

			//Init hex Axial
			FIntPoint Point = AxialAdd(AxialScale(AxialDirection(RING_START_DIRECTION_INDEX), i), FIntPoint(0, 0));
			TmpHex.X = Point.X;
			TmpHex.Y = Point.Y;
		}

		j = OnceLoop0 ? SpiralCreateCenterLoopData.IndexSaved[1] : 0;
		for (; j <= 5; j++) {
			Indices[1] = j;
			k = OnceLoop1 ? SpiralCreateCenterLoopData.IndexSaved[2] : 0;
			for (; k <= i - 1; k++)
			{
				Indices[2] = k;
				FlowControlUtility::SaveLoopData(this, SpiralCreateCenterLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
				if (SaveLoopFlag) {
					return;
				}

				AddRingTileAndIndex();
				FindNeighborTileOfRing(j);

				ProgressCurrent = SpiralCreateCenterLoopData.Count;
				Count++;
			}
			OnceLoop1 = false;
		}
		RingInitFlag = false;
		OnceLoop0 = false;
	}

	ResetProgress();

	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::SpiralCreateNeighbors;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, SpiralCreateCenterLoopData.Rate, false);
	UE_LOG(HexGridCreator, Log, TEXT("Spiral create center done."));
}

void AHexGridCreator::InitGridCenter()
{
	FStructHexTileData Data;
	Data.AxialCoord.X = 0;
	Data.AxialCoord.Y = 0;
	Data.Position2D.Set(0.0, 0.0);
	Tiles.Empty();
	Tiles.Add(Data);

	TileIndices.Empty();
	TileIndices.Add(FIntPoint(0, 0), 0.0);

}

void AHexGridCreator::AddRingTileAndIndex()
{
	FStructHexTileData Data;
	Data.AxialCoord.X = TmpHex.X;
	Data.AxialCoord.Y = TmpHex.Y;
	Data.Position2D.Set(TmpPosition2D.X, TmpPosition2D.Y);
	int32 Index = Tiles.Add(Data);

	TileIndices.Add(FIntPoint(TmpHex.X, TmpHex.Y), Index);
}

void AHexGridCreator::FindNeighborTileOfRing(int32 DirIndex)
{
	FVector2D Pos = NeighborDirVectors[DirIndex] * TileHeight + TmpPosition2D;
	TmpPosition2D.Set(Pos.X, Pos.Y);

	FIntPoint Hex = AxialNeighbor(TmpHex, DirIndex);
	TmpHex.X = Hex.X;
	TmpHex.Y = Hex.Y;
}

void AHexGridCreator::SpiralCreateNeighbors()
{
	bool OnceLoop0 = true;
	bool OnceLoop1 = true;
	bool OnceLoop2 = true;
	int32 Count = 0;
	TArray<int32> Indices = { 0, 0, 0, 0 };
	bool SaveLoopFlag = false;

	if (!SpiralCreateNeighborsLoopData.IsInitialized) {
		SpiralCreateNeighborsLoopData.IsInitialized = true;
		RingInitFlag = false;
		ProgressTarget = Tiles.Num() * 6 * (1 + NeighborRange) * NeighborRange / 2;
	}

	int32 TileIndex = SpiralCreateNeighborsLoopData.IndexSaved[0];
	int32 i, j, k;

	FIntPoint center;

	for (; TileIndex < Tiles.Num(); TileIndex++)
	{
		Indices[0] = TileIndex;
		center = Tiles[TileIndex].AxialCoord;
		i = OnceLoop0 ? SpiralCreateNeighborsLoopData.IndexSaved[1] : 1;
		i = i < 1 ? 1 : i;
		for (; i <= NeighborRange; i++)
		{
			Indices[1] = i;
			if (!RingInitFlag) {
				RingInitFlag = true;

				AddTileNeighbor(TileIndex, i);

				FIntPoint Point = AxialAdd(AxialScale(AxialDirection(RING_START_DIRECTION_INDEX), i), center);
				TmpHex.X = Point.X;
				TmpHex.Y = Point.Y;
			}

			j = OnceLoop1 ? SpiralCreateNeighborsLoopData.IndexSaved[2] : 0;
			for (; j <= 5; j++) {
				Indices[2] = j;
				k = OnceLoop2 ? SpiralCreateNeighborsLoopData.IndexSaved[3] : 0;
				for (; k <= i - 1; k++) {
					Indices[3] = k;
					FlowControlUtility::SaveLoopData(this, SpiralCreateNeighborsLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
					if (SaveLoopFlag) {
						return;
					}

					SetTileNeighbor(TileIndex, i, j);
					ProgressCurrent = SpiralCreateNeighborsLoopData.Count;
					Count++;
				}
				OnceLoop2 = false;
			}
			RingInitFlag = false;
			OnceLoop1 = false;
		}
		OnceLoop0 = false;
	}

	ResetProgress();

	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::CreateVertices;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, SpiralCreateNeighborsLoopData.Rate, false);
	UE_LOG(HexGridCreator, Log, TEXT("Spiral create neighbors done."));
}

void AHexGridCreator::AddTileNeighbor(int32 TileIndex, int32 Radius)
{
	FStructHexTileNeighbors neighbors;
	neighbors.Radius = Radius;
	Tiles[TileIndex].Neighbors.Add(neighbors);
}

void AHexGridCreator::SetTileNeighbor(int32 TileIndex, int32 Radius, int32 DirIndex)
{
	Tiles[TileIndex].Neighbors[Radius - 1].Tiles.Add(FIntPoint(TmpHex.X, TmpHex.Y));

	FIntPoint Hex = AxialNeighbor(TmpHex, DirIndex);
	TmpHex.X = Hex.X;
	TmpHex.Y = Hex.Y;
}

void AHexGridCreator::CreateVertices()
{
	if (!CreateVerticesLoopData.IsInitialized) {
		CreateVerticesLoopData.IsInitialized = true;
		InitCreateVertices();
		ProgressTarget = Tiles.Num();
	}
	
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	int32 i = CreateVerticesLoopData.IndexSaved[0];
	for (; i <= Tiles.Num() - 1; i++)
	{
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, CreateVerticesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
		CreateTileLineVertices(i);

		ProgressCurrent = CreateVerticesLoopData.Count;
		Count++;
	}
	ResetProgress();

	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::CreateTriangles;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, CreateVerticesLoopData.Rate, false);
	UE_LOG(HexGridCreator, Log, TEXT("Create vertices done."));

}

void AHexGridCreator::InitCreateVertices()
{
	float Ratio = 1.0 - GridLineRatio;

	FVector Vec(1.0, 0.0, 0.0);
	FVector ZAxis(0.0, 0.0, 1.0);
	for (int32 i = 0; i <= 5; i++)
	{
		FVector OuterVec = Vec.RotateAngleAxis(i * 60, ZAxis) * TileSize;
		FVector InnerVec = OuterVec * Ratio;
		OuterVectors.Add(OuterVec);
		InnerVectors.Add(InnerVec);
	}
}

void AHexGridCreator::CreateTileLineVertices(int32 TileIndex)
{
	for (int32 i = 0; i <= 5; i++)
	{
		FVector Vec(Tiles[TileIndex].Position2D.X, Tiles[TileIndex].Position2D.Y, 0);
		FVector OuterPoint = Vec + OuterVectors[i];
		FVector InnerPoint = Vec + InnerVectors[i];
		Vertices.Add(OuterPoint);
		Vertices.Add(InnerPoint);
	}
}

void AHexGridCreator::CreateTriangles()
{
	if (!CreateTrianglesLoopData.IsInitialized) {
		CreateTrianglesLoopData.IsInitialized = true;
		InitCreateTriangles(TriArr0, TriArr1, TriArr2, TriArr3);
		ProgressTarget = Tiles.Num();
	}
	

	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;
	int32 VIndexStart;

	int32 i = CreateTrianglesLoopData.IndexSaved[0];
	for (; i <= Tiles.Num() - 1; i++)
	{
		Indices[0] = i;
		VIndexStart = i * 12;
		FlowControlUtility::SaveLoopData(this, CreateTrianglesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			return;
		}
		CreateTileTriangles(VIndexStart, TriArr0, TriArr1, TriArr2, TriArr3);
		ProgressCurrent = CreateTrianglesLoopData.Count;
		Count++;
	}
	ResetProgress();

	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::WriteTiles;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, CreateTrianglesLoopData.Rate, false);
	UE_LOG(HexGridCreator, Log, TEXT("Create triangles done."));
}

void AHexGridCreator::InitCreateTriangles(TArray<int32>& Arr0, TArray<int32>& Arr1, TArray<int32>& Arr2, TArray<int32>& Arr3)
{
	for (int32 i = 0; i <= 5; i++)
	{
		Arr0.Add(i * 2);
		Arr1.Add(i * 2 + 1);
		int32 a = ((i + 1) % 6) * 2;
		Arr2.Add(a);
		Arr3.Add(a + 1);
	}
}

void AHexGridCreator::CreateTileTriangles(int32 VIndexStart, 
	const TArray<int32>& Arr0, const TArray<int32>& Arr1, const TArray<int32>& Arr2, const TArray<int32>& Arr3)
{
	TArray<int32> Indices = { 0, 0, 0, 0, 0, 0 };
	for (int32 i = 0; i <= 5; i++)
	{
		Indices[0] = Arr0[i] + VIndexStart;
		Indices[1] = Arr1[i] + VIndexStart;
		Indices[2] = Arr3[i] + VIndexStart;
		Indices[3] = Arr0[i] + VIndexStart;
		Indices[4] = Arr3[i] + VIndexStart;
		Indices[5] = Arr2[i] + VIndexStart;

		Triangles.Append(Indices);
	}
}

void AHexGridCreator::CreateFilePath(const FString& RelPath, FString& FullPath)
{
	FullPath = FPaths::ProjectDir().Append(RelPath);
	FString Path = FPaths::GetPath(FullPath);
	if (!FPaths::DirectoryExists(Path)) {
		if (std::filesystem::create_directories(*Path)) {
			UE_LOG(HexGridCreator, Log, TEXT("Create directory %s success."), *Path);
		}
	}
}

void AHexGridCreator::WriteTilesToFile()
{
	FString FullPath;
	CreateFilePath(TilesDataPath, FullPath);

	FTimerHandle TimerHandle;
	std::ofstream ofs;
	if (!WriteTilesLoopData.IsInitialized) {
		WriteTilesLoopData.IsInitialized = true;
		ofs.open(*FullPath, std::ios::out | std::ios::trunc);
		ProgressTarget = Tiles.Num();
	}
	else {
		ofs.open(*FullPath, std::ios::out | std::ios::app);
	}
	
	if (!ofs || !ofs.is_open()) {
		UE_LOG(HexGridCreator, Warning, TEXT("Open file %s failed!"), *FullPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, WriteTilesLoopData.Rate, false);
		return;
	}

	WriteTiles(ofs);
}

void AHexGridCreator::WriteTiles(std::ofstream& ofs)
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	int32 i = WriteTilesLoopData.IndexSaved[0];
	for ( ; i <= Tiles.Num() - 1; i++)
	{
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, WriteTilesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			ofs.close();
			return;
		}
		WriteTileLine(ofs, i);
		ProgressCurrent = WriteTilesLoopData.Count;
		Count++;
	}
	ofs.close();
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::WriteTileIndices;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, WriteTilesLoopData.Rate, false);
	UE_LOG(HexGridCreator, Log, TEXT("Write tiles done."));

}

void AHexGridCreator::WriteTileLine(std::ofstream& ofs, int32 Index)
{
	FStructHexTileData Data = Tiles[Index];
	WriteAxialCoord(ofs, Data);
	WritePipeDelimiter(ofs);
	WritePosition2D(ofs, Data);
	WritePipeDelimiter(ofs);
	WriteNeighbors(ofs, Data);
	WriteLineEnd(ofs);
}

void AHexGridCreator::WritePipeDelimiter(std::ofstream& ofs)
{
	ofs << TCHAR_TO_ANSI(*PipeDelim);
}

void AHexGridCreator::WriteColonDelimiter(std::ofstream& ofs)
{
	ofs << TCHAR_TO_ANSI(*ColonDelim);
}

void AHexGridCreator::WriteLineEnd(std::ofstream& ofs)
{
	ofs << std::endl;
}

void AHexGridCreator::WriteAxialCoord(std::ofstream& ofs, const FStructHexTileData& Data)
{
	FIntPoint AC = Data.AxialCoord;
	FString Str = FString::FromInt(AC.X);
	Str.Append(*CommaDelim);
	Str.Append(FString::FromInt(AC.Y));
	ofs << TCHAR_TO_ANSI(*Str);
}

void AHexGridCreator::WritePosition2D(std::ofstream& ofs, const FStructHexTileData& Data)
{
	FVector2D Pos2D = Data.Position2D;
	FText Txt = UKismetTextLibrary::Conv_FloatToText(Pos2D.X, ERoundingMode::HalfFromZero, false, false, 1, 324, 0, 2);
	FString Str = UKismetTextLibrary::Conv_TextToString(Txt);
	Str.Append(*CommaDelim);
	Txt = UKismetTextLibrary::Conv_FloatToText(Pos2D.Y, ERoundingMode::HalfFromZero, false, false, 1, 324, 0, 2);
	Str.Append(UKismetTextLibrary::Conv_TextToString(Txt));
	ofs << TCHAR_TO_ANSI(*Str);
}

void AHexGridCreator::WriteNeighbors(std::ofstream& ofs, const FStructHexTileData& Data)
{
	for (int32 i = 0; i < Data.Neighbors.Num(); i++)
	{
		WriteNeighborsInfo(ofs, Data.Neighbors[i]);
		if (i != Data.Neighbors.Num() - 1) {
			WriteColonDelimiter(ofs);
		}
	}
}

void AHexGridCreator::WriteNeighborsInfo(std::ofstream& ofs, const FStructHexTileNeighbors& Neighbors)
{
	for (int32 i = 0; i < Neighbors.Tiles.Num(); i++)
	{
		FString Str = FString::FromInt(Neighbors.Tiles[i].X);
		Str.Append(*CommaDelim);
		Str.Append(FString::FromInt(Neighbors.Tiles[i].Y));
		if (i != Neighbors.Tiles.Num() - 1) {
			Str.Append(*SpaceDelim);
		}
		ofs << TCHAR_TO_ANSI(*Str);
	}
}

void AHexGridCreator::WriteTileIndicesToFile()
{
	FString FullPath;
	CreateFilePath(TileIndicesDataPath, FullPath);

	FTimerHandle TimerHandle;
	std::ofstream ofs;
	if (!WriteTileIndicesLoopData.IsInitialized) {
		WriteTileIndicesLoopData.IsInitialized = true;
		ofs.open(*FullPath, std::ios::out | std::ios::trunc);
		ProgressTarget = Tiles.Num();
	}
	else {
		ofs.open(*FullPath, std::ios::out | std::ios::app);
	}

	if (!ofs || !ofs.is_open()) {
		UE_LOG(HexGridCreator, Warning, TEXT("Open file %s failed!"), *FullPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, WriteTileIndicesLoopData.Rate, false);
		return;
	}

	WriteTileIndices(ofs);
}

void AHexGridCreator::WriteTileIndices(std::ofstream& ofs)
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	int32 i = WriteTileIndicesLoopData.IndexSaved[0];
	for (; i <= Tiles.Num() - 1; i++)
	{
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, WriteTileIndicesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			ofs.close();
			return;
		}
		WriteTileIndicesLine(ofs, i);
		ProgressCurrent = WriteTileIndicesLoopData.Count;
		Count++;
	}
	ofs.close();
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::WriteVertices;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, WriteTileIndicesLoopData.Rate, false);
	UE_LOG(HexGridCreator, Log, TEXT("Write tiles indices done."));
}

void AHexGridCreator::WriteTileIndicesLine(std::ofstream& ofs, int32 Index)
{
	FIntPoint key = Tiles[Index].AxialCoord;
	WriteIndicesKey(ofs, key);
	WritePipeDelimiter(ofs);
	WriteIndicesValue(ofs, Index);
	WriteLineEnd(ofs);
}

void AHexGridCreator::WriteIndicesKey(std::ofstream& ofs, const FIntPoint& key)
{
	FString Str = FString::FromInt(key.X);
	Str.Append(*CommaDelim);
	Str.Append(FString::FromInt(key.Y));
	ofs << TCHAR_TO_ANSI(*Str);
}

void AHexGridCreator::WriteIndicesValue(std::ofstream& ofs, int32 Index)
{
	FString Str = FString::FromInt(Index);
	ofs << TCHAR_TO_ANSI(*Str);
}

void AHexGridCreator::WriteVerticesToFile()
{
	FString FullPath;
	CreateFilePath(VerticesDataPath, FullPath);

	FTimerHandle TimerHandle;
	std::ofstream ofs;
	if (!WriteVerticesLoopData.IsInitialized) {
		WriteVerticesLoopData.IsInitialized = true;
		ofs.open(*FullPath, std::ios::out | std::ios::trunc);
		ProgressTarget = Vertices.Num() / 12;
	}
	else {
		ofs.open(*FullPath, std::ios::out | std::ios::app);
	}

	if (!ofs || !ofs.is_open()) {
		UE_LOG(HexGridCreator, Warning, TEXT("Open file %s failed!"), *FullPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, WriteVerticesLoopData.Rate, false);
		return;
	}

	WriteVertices(ofs);
}

void AHexGridCreator::WriteVertices(std::ofstream& ofs)
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	int32 i = WriteVerticesLoopData.IndexSaved[0];
	for (; i <= Vertices.Num() - 1; i += 12) {
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, WriteVerticesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			ofs.close();
			return;
		}

		WriteVerticesLine(ofs, i);
		ProgressCurrent = WriteVerticesLoopData.Count;
		Count++;
	}
	ofs.close();
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::WriteTriangles;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, WriteVerticesLoopData.Rate, false);
	UE_LOG(HexGridCreator, Log, TEXT("Write vertices done."));
}

void AHexGridCreator::WriteVerticesLine(std::ofstream& ofs, int32 Index)
{
	for (int32 i = 0; i <= 11; i++)
	{
		FVector Vertex = Vertices[Index + i];
		WriteVertex(ofs, Vertex);
		WritePipeDelimiter(ofs);
	}
	WriteLineEnd(ofs);
}

void AHexGridCreator::WriteVertex(std::ofstream& ofs, const FVector& Vertex)
{
	FText Txt = UKismetTextLibrary::Conv_FloatToText(Vertex.X, ERoundingMode::HalfFromZero, false, false, 1, 324, 0, 2);
	FString Str = UKismetTextLibrary::Conv_TextToString(Txt);
	Str.Append(*CommaDelim);
	Txt = UKismetTextLibrary::Conv_FloatToText(Vertex.Y, ERoundingMode::HalfFromZero, false, false, 1, 324, 0, 2);
	Str.Append(UKismetTextLibrary::Conv_TextToString(Txt));
	Str.Append(*CommaDelim);
	Txt = UKismetTextLibrary::Conv_FloatToText(Vertex.Z, ERoundingMode::HalfFromZero, false, false, 1, 324, 0, 2);
	Str.Append(UKismetTextLibrary::Conv_TextToString(Txt));
	ofs << TCHAR_TO_ANSI(*Str);
}

void AHexGridCreator::WriteTrianglesToFile()
{
	FString FullPath;
	CreateFilePath(TrianglesDataPath, FullPath);

	FTimerHandle TimerHandle;
	std::ofstream ofs;
	if (!WriteTrianglesLoopData.IsInitialized) {
		WriteTrianglesLoopData.IsInitialized = true;
		ofs.open(*FullPath, std::ios::out | std::ios::trunc);
		ProgressTarget = Triangles.Num() / (6 * 2 * 3); // Each tile has 6 sides. Each side has 2 triangles. Each triangles has 3 vertices.
	}
	else {
		ofs.open(*FullPath, std::ios::out | std::ios::app);
	}

	if (!ofs || !ofs.is_open()) {
		UE_LOG(HexGridCreator, Warning, TEXT("Open file %s failed!"), *FullPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, WriteTrianglesLoopData.Rate, false);
		return;
	}

	WriteTriangles(ofs);
}

void AHexGridCreator::WriteTriangles(std::ofstream& ofs)
{
	int32 Count = 0;
	TArray<int32> Indices = { 0 };
	bool SaveLoopFlag = false;

	int32 i = WriteTrianglesLoopData.IndexSaved[0];
	for (;  i <= Triangles.Num() - 1; i += 36)
	{
		Indices[0] = i;
		FlowControlUtility::SaveLoopData(this, WriteTrianglesLoopData, Count, Indices, WorkflowDelegate, SaveLoopFlag);
		if (SaveLoopFlag) {
			ofs.close();
			return;
		}
		WriteTrianglesLine(ofs, i);
		ProgressCurrent = WriteTrianglesLoopData.Count;
		Count++;
	}
	ofs.close();
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::WriteParams;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, WriteTrianglesLoopData.Rate, false);
	UE_LOG(HexGridCreator, Log, TEXT("Write triangles done."));
}

void AHexGridCreator::WriteTrianglesLine(std::ofstream& ofs, int32 Index)
{
	for (int32 i = 0; i <= 35; i++)
	{
		WriteTriangleVertex(ofs, Triangles[Index + i]);
	}
	WriteLineEnd(ofs);
}

void AHexGridCreator::WriteTriangleVertex(std::ofstream& ofs, int32 Data)
{
	FString Str = FString::FromInt(Data);
	Str.Append(*CommaDelim);
	ofs << TCHAR_TO_ANSI(*Str);
}

void AHexGridCreator::WriteParamsToFile()
{
	FString FullPath;
	CreateFilePath(ParamsDataPath, FullPath);

	FTimerHandle TimerHandle;
	std::ofstream ofs;
	ofs.open(*FullPath, std::ios::out | std::ios::trunc);
	ProgressTarget = 1;

	if (!ofs || !ofs.is_open()) {
		UE_LOG(HexGridCreator, Warning, TEXT("Open file %s failed!"), *FullPath);
		WorkflowState = Enum_HexGridWorkflowState::Error;
		GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
		return;
	}

	WriteParams(ofs);
}

void AHexGridCreator::WriteParams(std::ofstream& ofs)
{
	WriteParamsContent(ofs);
	ProgressCurrent = 1;
	ofs.close();
	FTimerHandle TimerHandle;
	WorkflowState = Enum_HexGridWorkflowState::Done;
	GetWorldTimerManager().SetTimer(TimerHandle, WorkflowDelegate, DefaultTimerRate, false);
	UE_LOG(HexGridCreator, Log, TEXT("Write params done."));
}

void AHexGridCreator::WriteParamsContent(std::ofstream& ofs)
{
	FText Txt = UKismetTextLibrary::Conv_FloatToText(TileSize, ERoundingMode::HalfFromZero, false, false, 1, 324, 0, 2);
	FString Str = UKismetTextLibrary::Conv_TextToString(Txt);
	ofs << TCHAR_TO_ANSI(*Str);
	WritePipeDelimiter(ofs);
	Str = FString::FromInt(GridRange);
	ofs << TCHAR_TO_ANSI(*Str);
	WritePipeDelimiter(ofs);
	Txt = UKismetTextLibrary::Conv_FloatToText(GridLineRatio, ERoundingMode::HalfFromZero, false, false, 1, 324, 0, 3);
	Str = UKismetTextLibrary::Conv_TextToString(Txt);
	ofs << TCHAR_TO_ANSI(*Str);
	WritePipeDelimiter(ofs);
	Str = FString::FromInt(NeighborRange);
	ofs << TCHAR_TO_ANSI(*Str);
	WriteLineEnd(ofs);
}