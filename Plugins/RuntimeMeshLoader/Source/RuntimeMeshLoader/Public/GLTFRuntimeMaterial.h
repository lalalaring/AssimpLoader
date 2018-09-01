#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "RenderUtils.h"
#include "Engine/Texture2D.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Misc/FileHelper.h"
#include "GLTFReader.h"
#include "GLTFRuntimeAsset.h"
#include "ModuleManager.h"

namespace GLTFRuntimeMaterials
{
// 	UCLASS()
// 	class UBaseMaterial
// 	{
// 
// 	};
	static UMaterial * BaseMaterial = nullptr;
    static UMaterial* BaseTranslusentMaterial = nullptr;

	//static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

	bool VerifyOrLoadBaseMaterial()
    {
        FString BaseMaterialPath =  "/RuntimeMeshLoader/BaseMaterial/M_GLTFBase.M_GLTFBase";
        BaseMaterial = LoadObject<UMaterial>(nullptr, *BaseMaterialPath);
        
        FString BaseTranslusentMaterialPath = "/RuntimeMeshLoader/BaseMaterial/M_GLTFBaseTrans.M_GLTFBaseTrans";
        BaseTranslusentMaterial =  LoadObject<UMaterial>(nullptr, *BaseTranslusentMaterialPath);
        
		if (BaseMaterial && BaseTranslusentMaterial) return true;
		return false;
	}

	UTexture2D* CreateTexture(UObject* Outer, const TArray<uint8>& PixelData, EImageFormat Format, int32 InSizeX, int32 InSizeY, EPixelFormat InFormat, FName BaseName)
	{
		UTexture2D* NewTexture = NULL;

		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(Format);

		if (ImageWrapper.IsValid())
		{
			NewTexture = UTexture2D::CreateTransient(InSizeX, InSizeY, InFormat);
			NewTexture->Rename(*(BaseName.ToString() + FString::SanitizeFloat(FMath::RandRange(0, 9))));

			if (!NewTexture) return NULL;

			void* TextureData = NewTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memmove(TextureData, PixelData.GetData(), PixelData.Num() * sizeof(uint8));
			//FMemory::Memzero((void *)PixelData.GetData(), PixelData.Num() * sizeof(uint8));
			NewTexture->PlatformData->Mips[0].BulkData.Unlock();

			NewTexture->UpdateResource();
		}
		return NewTexture;
	}

	UTexture2D * ImportTexture(UObject * Outer, FString ImageName)
	{
		if (!FPaths::FileExists(ImageName))
		{
			UE_LOG(LogTemp, Error, TEXT("File not found: %s"), *ImageName);
			return nullptr;
		}

		// Load the compressed byte data from the file
		TArray<uint8> FileData;
		if (!FFileHelper::LoadFileToArray(FileData, *ImageName))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *ImageName);
			return nullptr;
		}

		// Detect the image type using the ImageWrapper module
        IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(FileData.GetData(), FileData.Num());
		if (ImageFormat == EImageFormat::Invalid)
		{
			UE_LOG(LogTemp, Error, TEXT("Unrecognized image file format: %s"), *ImageName);
			return nullptr;
		}

		// Create an image wrapper for the detected image format
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
		if (!ImageWrapper.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create image wrapper for file: %s"), *ImageName);
			return nullptr;
		}

		// Decompress the image data
		const TArray<uint8>* RawData = nullptr;
		ImageWrapper->SetCompressed(FileData.GetData(), FileData.Num() * sizeof(uint8));

		ImageWrapper->GetRaw(ERGBFormat::RGBA, 8, RawData);
		if (RawData == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to decompress image file: %s"), *ImageName);
			return nullptr;
		}

		// Create the texture and upload the uncompressed image data
		FString TextureBaseName = TEXT("T_") + FPaths::GetBaseFilename(ImageName);
        auto NewTexture =  CreateTexture(Outer, *RawData, ImageFormat, ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), EPixelFormat::PF_B8G8R8A8 /*EPixelFormat::PF_R8G8B8A8*/, FName(*TextureBaseName));
        return NewTexture;
	}

	UMaterialInstanceDynamic * CreateNewMaterial(FString Name,bool Translusent)
	{
		// Create an unreal material asset
		UMaterialInstanceDynamic * DynamicMaterial = nullptr;
		if(VerifyOrLoadBaseMaterial())
		{
			//Add unique salt to avoid name collision
			
			Name += FString::SanitizeFloat(FMath::Rand());
            if(Translusent)
                DynamicMaterial = UMaterialInstanceDynamic::Create(BaseTranslusentMaterial, nullptr, FName(*Name));
            else
                DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, nullptr, FName(*Name));
		}

		return DynamicMaterial;
	}

	UMaterialInstanceDynamic * ImportMaterial(const FMaterialInfo& MaterialData, FGLTFRuntimeAsset * Asset)
	{
		if (!Asset) return nullptr;
		UMaterialInstanceDynamic * NewMaterial = nullptr;
		FString Name;

		Name = MaterialData.Name;
		UE_LOG(LogTemp, Log, TEXT("MLARALOG: Importing material %s"), *Name);
        
        if(MaterialData.AlphaMode == EBlendMode::BLEND_Translucent)
        {
            NewMaterial = CreateNewMaterial(Name,true);
            if (!NewMaterial) return NewMaterial;
        }
        else
        {
            NewMaterial = CreateNewMaterial(Name,false);
            if (!NewMaterial) return NewMaterial;
        }
        
		
		UE_LOG(LogTemp, Log, TEXT("MLARALOG: Material %s created, filling parameters."), *Name);

        
		//Blend mode
		NewMaterial->BlendMode = MaterialData.AlphaMode;
        

		//Two sided
		NewMaterial->TwoSided = MaterialData.DoubleSided;

		//Diffuse factor
        FVector BaseColorFactor = MaterialData.BaseColorFactor;
        
        UE_LOG(LogTemp, Log, TEXT("MLARALOG: Material has base Factor %s"), *BaseColorFactor.ToString());
        
       
        if(BaseColorFactor.X >= 0.0f)
        {
            NewMaterial->SetVectorParameterValue("DiffuseFactor", FLinearColor(BaseColorFactor));
            NewMaterial->SetScalarParameterValue("isDifFactorExist",true);
            UE_LOG(LogTemp, Log, TEXT("MLARALOG: Material has base Factor"));
            
        }
        else
        {
            NewMaterial->SetScalarParameterValue("isDifFactorExist",false);
            UE_LOG(LogTemp, Log, TEXT("MLARALOG: Material doesnt have base Factor"));
        }
       
		//Diffuse texture 
		if (Asset->Textures.IsValidIndex(MaterialData.BaseColorIndex) && MaterialData.HasTexture)
		{
			UTexture2D * DiffuseTexture = Asset->Textures[MaterialData.BaseColorIndex];
			if (DiffuseTexture)
			{
				NewMaterial->SetTextureParameterValue("DiffuseTexture", DiffuseTexture);
                NewMaterial->SetScalarParameterValue("isDifExist",true);
                UE_LOG(LogTemp, Log, TEXT("MLARALOG: Material have Diffuse Texture"));
			}
            else
            {
                 NewMaterial->SetScalarParameterValue("isDifExist",false);
                 UE_LOG(LogTemp, Log, TEXT("MLARALOG: Material doesnt have Diffuse Texture"));
            }
            
		}
        
        //Params for ext tiling
        NewMaterial->SetScalarParameterValue("UScale",MaterialData.AllTextureParams.Z);
        NewMaterial->SetScalarParameterValue("VScale",MaterialData.AllTextureParams.W);
        NewMaterial->SetScalarParameterValue("UOffset",MaterialData.AllTextureParams.X);
        NewMaterial->SetScalarParameterValue("VOffset",MaterialData.AllTextureParams.Y);
        
        //return if have Translucent mat
        if(MaterialData.AlphaMode == EBlendMode::BLEND_Translucent)
        {
            return NewMaterial;
        }
        
       
        
        
		//Roughness factor
		float RoughnessFactor = MaterialData.RoughnessFactor;
        
        UE_LOG(LogTemp, Log, TEXT("MLARALOG: RoughnessFactor %s"), *FString::SanitizeFloat(RoughnessFactor));
        
        
        if (RoughnessFactor >= 0)
        {
			NewMaterial->SetScalarParameterValue("RoughnessFactor", RoughnessFactor);
            NewMaterial->SetScalarParameterValue("isRougnessExist",1.0f);
        }
        else
        {
            NewMaterial->SetScalarParameterValue("isRougnessExist",1.0f);
            NewMaterial->SetScalarParameterValue("RoughnessFactor", 0.0f);
        }

		//Metallic factor
		float MetallicFactor = MaterialData.MetallicFactor;
        
        UE_LOG(LogTemp, Log, TEXT("MLARALOG: MetallicFactor %s"), *FString::SanitizeFloat(MetallicFactor));
        
        if (MetallicFactor >= 0)
        {
			NewMaterial->SetScalarParameterValue("MetallicFactor", MetallicFactor);
            NewMaterial->SetScalarParameterValue("isMetallicExist",1.0f);
        }
        else
        {
            NewMaterial->SetScalarParameterValue("MetallicFactor", 1.0f);
            NewMaterial->SetScalarParameterValue("isMetallicExist",1.0f);
        }
        
		//Roughness-metallic texture
		if (Asset->Textures.IsValidIndex(MaterialData.MetallicRoughness) && MaterialData.HasTexture)
		{
			UTexture2D * RoughnessTexture = Asset->Textures[MaterialData.MetallicRoughness];
			if (RoughnessTexture)
			{
				NewMaterial->SetTextureParameterValue("MetallicRoughnessTexture", RoughnessTexture);
                NewMaterial->SetScalarParameterValue("isMRTExist",1.0f);
                UE_LOG(LogTemp, Log, TEXT("MLARALOG: Metallic Texture exist"));
			}
            else
            {
                UE_LOG(LogTemp, Log, TEXT("MLARALOG: Metallic Texture doesnt exist"));
            }
		}

		//Normal texture
		if (Asset->Textures.IsValidIndex(MaterialData.NormalIndex) && MaterialData.HasTexture)
		{
			UTexture2D * NormalTexture = Asset->Textures[MaterialData.NormalIndex];
			if (NormalTexture)
			{
				NewMaterial->SetTextureParameterValue("NormalTexture", NormalTexture);
                NewMaterial->SetScalarParameterValue("isValidNormalTexture", 1.0f);
                UE_LOG(LogTemp, Log, TEXT("MLARALOG: Normal texture exist"));
			}
            else
            {
                NewMaterial->SetScalarParameterValue("isValidNormalTexture", 0.0f);
            }
		}
		//Normal factor
		float NormalFactor = MaterialData.NormalScale;
        if(NormalFactor >= 0)
        {
			NewMaterial->SetScalarParameterValue("NormalFactor", NormalFactor);
            NewMaterial->SetScalarParameterValue("isValidNormalFactor", 1.0f);
        }
        else
        {
            NewMaterial->SetScalarParameterValue("NormalFactor", 1.0f);
            NewMaterial->SetScalarParameterValue("isValidNormalFactor", 0.0f);
        }

		//Ambient texture
		if (Asset->Textures.IsValidIndex(MaterialData.OcclusionIndex) && MaterialData.HasTexture)
		{
			UTexture2D * AmbientTexture = Asset->Textures[MaterialData.OcclusionIndex];
			if (AmbientTexture)
			{
				NewMaterial->SetTextureParameterValue("AmbientTexture", AmbientTexture);
                NewMaterial->SetScalarParameterValue("isATextureExist",1.0f);
                UE_LOG(LogTemp, Log, TEXT("MLARALOG: Ambient texture exist"));
			}
            else
            {
                NewMaterial->SetScalarParameterValue("isATextureExist",0.0f);
            }
		}
		//Ambient factor
//        float AmbientFactor = MaterialData.OcclusionStrength;
//        if (AmbientFactor >= 0)
//        {
//            NewMaterial->SetScalarParameterValue("AmbientFactor", AmbientFactor);
//            NewMaterial->SetScalarParameterValue("isAmbientFExist", true);
//        }
//        else
//        {
//             NewMaterial->SetScalarParameterValue("isAmbientFExist", false);
//        }
//
//        //Emissive texture
//        if(Asset->Textures.IsValidIndex(MaterialData.EmissiveIndex))
//        {
//            UTexture2D * EmissiveTexture = Asset->Textures[MaterialData.EmissiveIndex];
//            if (EmissiveTexture)
//            {
//                NewMaterial->SetTextureParameterValue("EmissiveTexture", EmissiveTexture);
//            }
//        }
		//Emissive factor
		FVector EmissiveFactor = MaterialData.EmissiveFactor;
		NewMaterial->SetVectorParameterValue("EmissiveFactor", EmissiveFactor);
        
        
		return NewMaterial;
	}
    
    //Importing the Asset materials parsing Asset
	bool ImportMaterials(FGLTFRuntimeAsset * Asset, FString FilePath)
	{
		GLTFReader Reader(Asset, FilePath);
		FMaterialData MaterialData = Reader.GetMaterialData();
		Asset->Textures.Reserve(MaterialData.Textures.Num());
		Asset->Materials.Reserve(MaterialData.Materials.Num());
		UE_LOG(LogTemp, Log, TEXT("MLARALOG: Materials json data read."));
        for (auto Texture : MaterialData.Textures)
		{
			FString FolderPath = FPaths::GetPath(FilePath);
			FString FileName = "/" + MaterialData.Images[Texture.Source].URI;
            UE_LOG(LogTemp, Log, TEXT("Iterating"));
            
            Asset->Textures.Add(ImportTexture(BaseMaterial, FolderPath + FileName));
		}
		UE_LOG(LogTemp, Log, TEXT("MLARALOG: Textures created."));
		for (auto Material : MaterialData.Materials)
			Asset->Materials.Add(ImportMaterial(Material, Asset));
		UE_LOG(LogTemp, Log, TEXT("MLARALOG: Materials created."));
		return true;
	}
};
