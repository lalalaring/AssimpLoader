// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h" //Include for FProcMeshTangent. Should probably avoid this dependence.
#include "ImageCore.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"


/*
	Meshes and materials has 1 to 1 dependencies.
	Additional material has index that point to what mesh it is related. 
*/

struct FMeshInfo
{
	TArray<FVector> Vertices;

	TArray<int32> Triangles;

	TArray<FVector> Normals;

	TArray<FVector2D> UV0;
    
    TArray<FVector2D> UV1;
    
	TArray<FLinearColor> VertexColors;

	TArray<FProcMeshTangent> Tangents;

	FTransform RelativeTransform;

	uint32 MaterialIndex;
	
	FString Name;
};

struct FImageInfo
{
	FString URI;
	FString Name;

	enum class EExtension
	{
		PNG,
		JPEG
	};
	EExtension ImageFormat;
};

struct FTextureInfo
{
	FString Name;
	int32 Source;
};

struct FMaterialInfo
{
	const FString Name;

	FMaterialInfo(const FString& InName)
		: Name(InName)
	{}

	// PBR material inputs
	int8 BaseColorIndex;
	int8 MetallicRoughness;
	FVector4 BaseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
	float MetallicFactor{ 1.0f };
	float RoughnessFactor{ 1.0f };

	// base material inputs
	int8 NormalIndex;
	int8 OcclusionIndex;
	int8 EmissiveIndex;
	float NormalScale{ -1.0f };
	float OcclusionStrength{ -1.0f };
	FVector EmissiveFactor{ FVector::ZeroVector };

	// material properties
	bool DoubleSided{ false };
	EBlendMode AlphaMode{ EBlendMode::BLEND_Opaque };
	float AlphaCutoff{ 0.5f }; // only meaningful when AlphaMode == Mask
    
    // ext_extension prams we are putting in FVector4 (offset,offset,scale,scale)
    FVector4 AllTextureParams {0,0,1,1};
    bool HasTexture {false};
};

struct FMaterialData
{
	TArray<FImageInfo> Images;
	TArray<FTextureInfo> Textures;
	TArray<FMaterialInfo> Materials;
};


struct FAdditionalMaterial
{
	FString Name;

	//Mesh key, material value
	TMap<int8, int8> MaterialMesh;
};

struct FGLTFRuntimeAsset
{

	TArray<FMeshInfo> MeshInfo;
	TArray<UMaterialInstanceDynamic *> Materials;
	TArray<UTexture2D*> Textures;
	TArray<FAdditionalMaterial> AdditonalMaterials;

	FString Name; //Name is equivalent to folder path from where asset was loaded
	bool bSuccess = false;
    
    ~FGLTFRuntimeAsset()
    {

        for (auto Material : Materials)
        {
            if(Material->IsValidLowLevel())
            {
				Material->ClearParameterValues();
                Material->ConditionalBeginDestroy();
                Material = nullptr;
            }
        }
		UE_LOG(LogTemp, Warning, TEXT("MLARALOG: Materials objects are empty"))
        Materials.Empty();
        for (auto Texture : Textures)
        {
            if(Texture->IsValidLowLevel() && !Texture->IsPendingKillOrUnreachable())
            {

                Texture->ConditionalBeginDestroy();
                Texture = nullptr;
            }
        }
        Textures.Empty();
		UE_LOG(LogTemp, Warning, TEXT("MLARALOG: Textures objects are empty"))
    }
};
