#pragma once

#include "CoreMinimal.h"
#include "GLTFRuntimeAsset.h"
#include "Json.h"

class GLTFReader
{
	uint8 ImageCount{ 0 };
	uint8 SamplerCount{ 0 };
	uint8 TextureCount{ 0 };
	uint8 MaterialCount{ 0 };

	FGLTFRuntimeAsset *GLTFAsset;
	FString FilePath;
	FMaterialData MaterialData;

	bool IsLoaded(FString Name);

	void SetupImage(const FJsonObject& Object);
	void SetupSampler(const FJsonObject& Object);
	void SetupTexture(const FJsonObject& Object);
	void SetupMaterial(const FJsonObject& Object);
	void SetupAddinionalMaterials(const FJsonObject& Object);

	// Returns scale factor if JSON has it, 1.0 by default.
    float SetupMaterialTexture(int8 & TextureIndex, const FJsonObject& Object, const char* TexName, const char* ScaleName,FMaterialInfo& MatInfo);

public:

	GLTFReader(FGLTFRuntimeAsset * GLTFAsset, FString FilePath);

	FMaterialData GetMaterialData() { return MaterialData; };
};
