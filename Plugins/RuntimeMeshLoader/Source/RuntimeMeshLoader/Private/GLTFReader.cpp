#include "GLTFReader.h"
#include "Materials/MaterialInstanceDynamic.h"

static uint32 ArraySize(const FJsonObject * Object, const FString& Name)
{
	if (Object)
		if (Object->HasTypedField<EJson::Array>(Name))
		{
			return Object->GetArrayField(Name).Num();
		}

	return 0; // for empty arrays & if array does not exist
}

bool GLTFReader::IsLoaded(FString Name)
{
	for (auto Material : GLTFAsset->Materials)
	{
		if (Material->GetName().Contains(Name))
			return true;
	}
	return false;
}

static EBlendMode AlphaModeFromString(const FString& S)
{
	// case sensitive comparison
	if (S.Equals("OPAQUE")) return EBlendMode::BLEND_Opaque;
	if (S.Equals("MASK")) return EBlendMode::BLEND_Masked;
    if (S.Equals("BLEND")) return EBlendMode::BLEND_Translucent;
	return EBlendMode::BLEND_Opaque;
}

static FString GetString(const FJsonObject& Object, const char* Name, const FString& DefaultValue = FString())
{
	if (Object.HasTypedField<EJson::String>(Name))
	{
		return Object.GetStringField(Name);
	}

	return DefaultValue;
}

bool GetBool(const FJsonObject& Object, const char* Name, bool DefaultValue = false)
{
	if (Object.HasTypedField<EJson::Boolean>(Name))
	{
		return Object.GetBoolField(Name);
	}
	return DefaultValue;
}

static uint32 GetInt(const FJsonObject& Object, const char* Name, int32 DefaultValue = 0)
{
	if (Object.HasTypedField<EJson::Number>(Name))
	{
		int32 SignedValue = Object.GetIntegerField(Name);
		if (SignedValue >= 0)
		{
			return SignedValue;
		}
	}
	return DefaultValue;
}

static float GetScalar(const FJsonObject& Object, const char* Name, float DefaultValue = 0.0f)
{
	if (Object.HasTypedField<EJson::Number>(Name))
	{
		return Object.GetNumberField(Name);
	}

	return DefaultValue;
}

static FVector GetVec3(const FJsonObject& Object, const char* Name, const FVector& DefaultValue = FVector::ZeroVector)
{
	if (Object.HasTypedField<EJson::Array>(Name))
	{
		const TArray<TSharedPtr<FJsonValue>>& Array = Object.GetArrayField(Name);
		if (Array.Num() == 3)
		{
			float X = Array[0]->AsNumber();
			float Y = Array[1]->AsNumber();
			float Z = Array[2]->AsNumber();
			return FVector(X, Y, Z);
		}
	}
	return DefaultValue;
}

static FVector4 GetVec2(const FJsonObject& Object, const char* Name, const FVector4& DefaultValue = FVector4())
{
    if (Object.HasTypedField<EJson::Array>(Name))
    {
        const TArray<TSharedPtr<FJsonValue>>& Array = Object.GetArrayField(Name);
        if (Array.Num() == 2)
        {
            float X = Array[0]->AsNumber();
            float Y = Array[1]->AsNumber();
            return FVector4(X, Y, 0, 0);
        }
    }
    return DefaultValue;
}

static FVector4 GetVec4(const FJsonObject& Object, const char* Name, const FVector4& DefaultValue = FVector4())
{
	if (Object.HasTypedField<EJson::Array>(Name))
	{
		const TArray<TSharedPtr<FJsonValue>>& Array = Object.GetArrayField(Name);
		if (Array.Num() == 4)
		{
			float X = Array[0]->AsNumber();
			float Y = Array[1]->AsNumber();
			float Z = Array[2]->AsNumber();
			float W = Array[3]->AsNumber();
			return FVector4(X, Y, Z, W);
		}
	}
	return DefaultValue;
}

void GLTFReader::SetupImage(const FJsonObject& Object)
{
	MaterialData.Images.Emplace();
	FImageInfo &ImageInfo = MaterialData.Images.Last();
	ImageInfo.Name = GetString(Object, "name");

	if (Object.HasTypedField<EJson::String>("uri"))
	{
		ImageInfo.URI = Object.GetStringField("uri");

		if (ImageInfo.URI.EndsWith(".png"))
		{
			ImageInfo.ImageFormat = FImageInfo::EExtension::PNG;
		}
		else if (ImageInfo.URI.EndsWith(".jpg") || ImageInfo.URI.EndsWith(".jpeg"))
		{
			ImageInfo.ImageFormat = FImageInfo::EExtension::JPEG;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Unknown file format, could not be read."));
		}
	}
}

void GLTFReader::SetupTexture(const FJsonObject& Object)
{
	MaterialData.Textures.Emplace();
	FTextureInfo &ITextureInfo = MaterialData.Textures.Last();
	if (Object.HasTypedField<EJson::String>("name"))
		ITextureInfo.Name = GetString(Object, "name");
    if (Object.HasTypedField<EJson::Number>("source"))
		ITextureInfo.Source = GetInt(Object, "source");
}

void GLTFReader::SetupMaterial(const FJsonObject& Object)
{
	MaterialData.Materials.Emplace(GetString(Object, "name"));
	FMaterialInfo& Mat = MaterialData.Materials.Last();

	SetupMaterialTexture(Mat.EmissiveIndex, Object, "emissiveTexture",nullptr,Mat);
	Mat.EmissiveFactor = GetVec3(Object, "emissiveFactor");

	Mat.NormalScale = SetupMaterialTexture(Mat.NormalIndex, Object, "normalTexture", "scale",Mat);
    
	Mat.OcclusionStrength = SetupMaterialTexture(Mat.OcclusionIndex, Object, "occlusionTexture", "strength",Mat);

	if (Object.HasTypedField<EJson::Object>("pbrMetallicRoughness"))
	{
		const FJsonObject& PBR = *Object.GetObjectField("pbrMetallicRoughness");

		SetupMaterialTexture(Mat.BaseColorIndex, PBR, "baseColorTexture",nullptr,Mat);
		Mat.BaseColorFactor = GetVec4(PBR, "baseColorFactor", FVector4(-1.0f, -1.0f, -1.0f, -1.0f));

		SetupMaterialTexture(Mat.MetallicRoughness, PBR,"metallicRoughnessTexture",nullptr,Mat);
		Mat.MetallicFactor = GetScalar(PBR, "metallicFactor", -1.0f);
        UE_LOG(LogTemp, Error, TEXT("Metallic factor %s"), *FString::SanitizeFloat(Mat.MetallicFactor));
		Mat.RoughnessFactor = GetScalar(PBR, "roughnessFactor", -1.0f);
        UE_LOG(LogTemp, Error, TEXT("roughnessFactor %s"), *FString::SanitizeFloat(Mat.RoughnessFactor));
	}

	if (Object.HasTypedField<EJson::String>("alphaMode"))
	{
		Mat.AlphaMode = AlphaModeFromString(Object.GetStringField("alphaMode"));
	}

	Mat.DoubleSided = GetBool(Object, "doubleSided");
}

void GLTFReader::SetupAddinionalMaterials(const FJsonObject& Object)
{
	const FJsonObject& Extras = *Object.GetObjectField("extras");
    auto MaterialSetsArray = Extras.GetArrayField("MaterialSets");
    if(MaterialSetsArray.Num() > 0)
    {
        for (TSharedPtr<FJsonValue> MaterialSets : MaterialSetsArray)
        {
            if(MaterialSets.IsValid())
            {
                const FJsonObject& MaterialSet = *MaterialSets->AsObject();
                GLTFAsset->AdditonalMaterials.Emplace();
                FAdditionalMaterial & AddMaterial = GLTFAsset->AdditonalMaterials.Last();
                AddMaterial.Name = MaterialSet.GetStringField("Name");
                for (TSharedPtr<FJsonValue> Geos : MaterialSet.GetArrayField("Geo"))
                {
                    const FJsonObject& Geo = *Geos->AsObject();
                    FString MeshName = Geo.GetStringField("GName");
                    FString MaterialName = Geo.GetStringField("MName");
                    int32 MeshIndex = -1;
                    int32 MaterialIndex = -1;
                    for (int32 i = 0; i < GLTFAsset->MeshInfo.Num(); i++)
                    {
                        if(MeshName.Equals(GLTFAsset->MeshInfo[i].Name))
                        {
                            MeshIndex = i;
                            break;
                        }
                    }
                    for (int32 i = 0; i < MaterialData.Materials.Num(); i++)
                    {
                        if (MaterialName.Equals(MaterialData.Materials[i].Name))
                        {
                            MaterialIndex = i;
                            break;
                        }
                    }
                    if(MeshIndex != -1 && MaterialIndex != -1)
                        AddMaterial.MaterialMesh.Add(MeshIndex, MaterialIndex);
                }
            }
        }
    }
}

float GLTFReader::SetupMaterialTexture(int8 & TextureIndex, const FJsonObject& Object, const char* TexName/* = nullptr*/,const char* ScaleName,FMaterialInfo& MatInfo)
{
    float Scale = 1.0f;
    
    if (Object.HasTypedField<EJson::Object>(TexName))
    {
        FString myString(TexName);
        UE_LOG(LogTemp, Error, TEXT("Texture name is %s"), *myString);
        const FJsonObject& TexObj = *Object.GetObjectField(TexName);
        int8 TexIndex = GetInt(TexObj, "index", -1);
    
        
        if (TexObj.HasTypedField<EJson::Object>("extensions"))
        {
            UE_LOG(LogTemp, Error, TEXT("Texture name is %s has an EXT"), *myString);
            const FJsonObject& Ext = *TexObj.GetObjectField("extensions");
            const FJsonObject& Ext_T = *Ext.GetObjectField("EXT_texture_transform");
            MatInfo.AllTextureParams = GetVec2(Ext_T, "offset");
            FVector4 temp = GetVec2(Ext_T, "scale");
            MatInfo.AllTextureParams.Z = temp.X;
            MatInfo.AllTextureParams.W = temp.Y;
            UE_LOG(LogTemp, Error, TEXT("Texture name is %s has an EXT transform"), *MatInfo.AllTextureParams.ToString());
            if(Ext_T.HasField("texCoord"))
            {
                MatInfo.HasTexture = true;
            }
            else
            {
                MatInfo.HasTexture = false;
            }
        }
        
        if (MaterialData.Textures.IsValidIndex(TexIndex))
        {
            TextureIndex = TexIndex;
            if (ScaleName)
            {
                if (TexObj.HasTypedField<EJson::Number>(ScaleName))
                {
                    Scale = TexObj.GetNumberField(ScaleName);
                }
            }
        }
    }
    
    return Scale;
}

GLTFReader::GLTFReader(FGLTFRuntimeAsset * GLTFAsset, FString FilePath)
{
	this->GLTFAsset = GLTFAsset;
	this->FilePath = FilePath;
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Could not load file %s"), *FilePath);
		return;
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		const TSharedPtr<FJsonObject>& AssetInfo = JsonObject->GetObjectField("asset");
		if (AssetInfo->HasTypedField<EJson::Number>("minVersion"))
		{
			const double MinVersion = AssetInfo->GetNumberField("minVersion");
			if (MinVersion > 2.0)
			{
				UE_LOG(LogTemp, Error, TEXT("This importer supports glTF version 2.0 (or compatible) assets."));
				return;
			}
		}
		else
		{
			const double Version = AssetInfo->GetNumberField("version");
			if (Version < 2.0)
			{
				UE_LOG(LogTemp, Error, TEXT("This importer supports glTF asset version 2.0 or later."));
				return;
			}
		}
	}

	ImageCount = ArraySize(JsonObject.Get(), "images");
	SamplerCount = ArraySize(JsonObject.Get(), "samplers");
	TextureCount = ArraySize(JsonObject.Get(), "textures");
	MaterialCount = ArraySize(JsonObject.Get(), "materials");

	if (ImageCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : JsonObject->GetArrayField("images"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupImage(Object);
		}
	}

	if (TextureCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : JsonObject->GetArrayField("textures"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupTexture(Object);
		}
	}

	if (MaterialCount > 0)
	{
		for (TSharedPtr<FJsonValue> Value : JsonObject->GetArrayField("materials"))
		{
			const FJsonObject& Object = *Value->AsObject();
			SetupMaterial(Object);
		}
	}
	for (TSharedPtr<FJsonValue> Value : JsonObject->GetArrayField("scenes"))
	{
		const FJsonObject& Object = *Value->AsObject();
		SetupAddinionalMaterials(Object);
	}
}
