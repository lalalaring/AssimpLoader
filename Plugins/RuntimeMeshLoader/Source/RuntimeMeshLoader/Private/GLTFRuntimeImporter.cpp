// Fill out your copyright notice in the Description page of Project Settings.
#include "GLTFRuntimeImporter.h"
#include "RuntimeMeshLoader.h"
#include "GLTFRuntimeMaterial.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#include <jni.h>

static jmethodID GetExternalDirMethodId;
#endif

//assimp
//#if PLATFORM_ANDROID || PLATFORM_IOS
#include "assimp/Importer.hpp"  // C++ importer interface
#include "assimp/scene.h"       // Output data structure
#include "assimp/postprocess.h" // Post processing flags
//#endif

FAssimpImport* FAssimpImport::Instance = nullptr;

static TArray<FVector> GenerateFlatNormals(const TArray<FVector>& Positions, const TArray<uint32>& Indices)
{
    TArray<FVector> Normals;
    const uint32 N = Indices.Num();
    check(N % 3 == 0);
    Normals.AddUninitialized(N);

    for (uint32 i = 0; i < N; i += 3)
    {
        const FVector& A = Positions[Indices[i]];
        const FVector& B = Positions[Indices[i + 1]];
        const FVector& C = Positions[Indices[i + 2]];

        const FVector Normal = FVector::CrossProduct(A - B, A - C).GetSafeNormal();

        // Same for each corner of the triangle.
        Normals[i] = Normal;
        Normals[i + 1] = Normal;
        Normals[i + 2] = Normal;
    }

    return Normals;
}

//#if PLATFORM_ANDROID || PLATFORM_IOS
void FindMeshInfo(const aiScene* Scene, aiNode* Node, FGLTFRuntimeAsset * Result)
{
	if (!Result) return;
	for (uint32 i = 0; i < Node->mNumMeshes; i++)
	{
		int32 MeshIndex = *Node->mMeshes;
		aiMesh *Mesh = Scene->mMeshes[MeshIndex];
		FMeshInfo &MeshInfo = Result->MeshInfo[MeshIndex];

		//transform.
		aiMatrix4x4 TransformMatrix = Node->mTransformation;
        //Node-
		FMatrix Matrix;
        Matrix.M[0][0] = TransformMatrix.a1; Matrix.M[0][1] = TransformMatrix.b1; Matrix.M[0][2] = TransformMatrix.c1; Matrix.M[0][3] = TransformMatrix.d1;
		Matrix.M[1][0] = TransformMatrix.a2; Matrix.M[1][1] = TransformMatrix.b2; Matrix.M[1][2] = TransformMatrix.c2; Matrix.M[1][3] = TransformMatrix.d2;
		Matrix.M[2][0] = TransformMatrix.a3; Matrix.M[2][1] = TransformMatrix.b3; Matrix.M[2][2] = TransformMatrix.c3; Matrix.M[2][3] = TransformMatrix.d3;
		Matrix.M[3][0] = TransformMatrix.a4; Matrix.M[3][1] = TransformMatrix.b4; Matrix.M[3][2] = TransformMatrix.c4; Matrix.M[3][3] = TransformMatrix.d4;
		
		//UE_LOG(LogTemp, Warning, TEXT("Name node : %s"), *MeshInfo.Name);
		//	Matrix.M[0][0], Matrix.M[0][1], Matrix.M[0][2], Matrix.M[0][3],
		//	Matrix.M[1][0], Matrix.M[1][1], Matrix.M[1][2], Matrix.M[1][3],
		//	Matrix.M[2][0], Matrix.M[2][1], Matrix.M[2][2], Matrix.M[2][3],
		//	Matrix.M[3][0], Matrix.M[3][1], Matrix.M[3][2], Matrix.M[3][3]);

		MeshInfo.RelativeTransform = FTransform(Matrix);
        MeshInfo.RelativeTransform *= FTransform(FRotator(0.f, -90.f, -90.f), FVector(0.f), FVector(100.f));
        TArray<FVector> Vertices;
        TArray<uint32> indexes;
        int32 counter = 0;
		//vet
		for (uint32 j = 0; j < Mesh->mNumVertices; ++j)
		{
			FVector Vertex = FVector(
				Mesh->mVertices[j].x,
                Mesh->mVertices[j].y,
				Mesh->mVertices[j].z);
                Vertex = MeshInfo.RelativeTransform.TransformPosition(Vertex);
			    MeshInfo.Vertices.Push(Vertex);
            
            Vertices.Add(Vertex);
            ++counter;
            
			//Normal
            if (Mesh->HasNormals())
            {
                FVector Normal = FVector(
                    Mesh->mNormals[j].x,
                    Mesh->mNormals[j].y,
                    Mesh->mNormals[j].z);
                MeshInfo.Normals.Push(Normal);
            }
            else
            {
                //TODO Generate Flat normals
                UE_LOG(LogTemp, Warning, TEXT("NO NORMALS"));
            }
			//UV Coordinates - inconsistent coordinates
            int32 numUVChannels  = Mesh->GetNumUVChannels();
            if(numUVChannels > 1)
            {
                FVector2D UV0 = FVector2D(Mesh->mTextureCoords[0][j].x, -Mesh->mTextureCoords[0][j].y);
                MeshInfo.UV0.Add(UV0);
                FVector2D UV1 = FVector2D(Mesh->mTextureCoords[1][j].x, -Mesh->mTextureCoords[1][j].y);
                MeshInfo.UV1.Add(UV1);
            }
            else
            {
                if (Mesh->HasTextureCoords(0))
                {
                    FVector2D UV = FVector2D(Mesh->mTextureCoords[0][j].x, -Mesh->mTextureCoords[0][j].y);
                    MeshInfo.UV0.Add(UV);
                    MeshInfo.UV1.Add(UV);
                }
            }
            
			//Tangent
            if (Mesh->HasTangentsAndBitangents())
            {
                FProcMeshTangent MeshTangent = FProcMeshTangent(
                    Mesh->mTangents[j].x,
                    Mesh->mTangents[j].y,
                    Mesh->mTangents[j].z
                );
                MeshInfo.Tangents.Push(MeshTangent);
            }else
            {
                UE_LOG(LogTemp, Warning, TEXT("NO Tangents"));
            }
		}
	}
}
//#endif
//#if PLATFORM_ANDROID || PLATFORM_IOS
void FindMesh(const aiScene* Scene, aiNode* Node, FGLTFRuntimeAsset * OutData)
{
	FindMeshInfo(Scene, Node, OutData);

	for (uint32 i = 0; i < Node->mNumChildren; ++i)
	{
		FindMesh(Scene, Node->mChildren[i], OutData);
	}
}
//#endif
//#if PLATFORM_ANDROID || PLATFORM_IOS
void ImportMeshes(FGLTFRuntimeAsset * MeshData, const struct aiScene * ImportedScene)
{
	if (!MeshData) return;
	MeshData->MeshInfo.SetNum(ImportedScene->mNumMeshes, false);

	for (uint32 i = 0; i < ImportedScene->mNumMeshes; ++i)
	{
		MeshData->MeshInfo[i].MaterialIndex = ImportedScene->mMeshes[i]->mMaterialIndex;
		MeshData->MeshInfo[i].Name = FString(UTF8_TO_TCHAR(ImportedScene->mMeshes[i]->mName.C_Str()));
	}

	FindMesh(ImportedScene, ImportedScene->mRootNode, MeshData);

	for (uint32 i = 0; i < ImportedScene->mNumMeshes; ++i)
	{
		//Triangle number
		for (uint32 l = 0; l < ImportedScene->mMeshes[i]->mNumFaces; ++l)
		{
			for (uint32 m = 0; m < ImportedScene->mMeshes[i]->mFaces[l].mNumIndices; ++m)
			{
				MeshData->MeshInfo[i].Triangles.Push(ImportedScene->mMeshes[i]->mFaces[l].mIndices[m]);
			}
		}

	}
	MeshData->bSuccess = true;
}
//#endif

uint32 FAssimpImport::Run()
{
	//FPlatformProcess::Sleep(0.03f);
    //#if PLATFORM_ANDROID || PLATFORM_IOS
	{
		FScopeLock Lock(&ImportCriticalSection);
     
		aiString CFilePath;
		CFilePath = TCHAR_TO_UTF8(*FilePath);
  
		Assimp::Importer Importer;
		const aiScene* ImportedScene = Importer.ReadFile(CFilePath.C_Str(), 
			aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_MakeLeftHanded | aiProcess_GenUVCoords | aiProcess_CalcTangentSpace | aiProcess_OptimizeMeshes);

		if (ImportedScene == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("ImportError: %s."), UTF8_TO_TCHAR(Importer.GetErrorString()));
			return -1;
		}
		UE_LOG(LogTemp, Warning, TEXT("Geometry imported."));
		GLTFAsset = new FGLTFRuntimeAsset();

		if (ImportedScene->HasMeshes())
		{
			ImportMeshes(GLTFAsset, ImportedScene);
			Exit();
		}
		else
			return -1;
	}
//#endif
	return 0;
}

bool UGLTFRuntimeImporter::LoadAsset(FString Filepath)
{
	if (Filepath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Runtime Mesh Loader: filepath is empty."));
		return false;
	}

	AssetFilePath = Filepath;

#if PLATFORM_ANDROID
	Filepath.RemoveAt(0, 8, false);
	Filepath = GetAbsolutePathToSaved() + Filepath;
	FPaths::NormalizeFilename(Filepath);
	UE_LOG(LogTemp, Warning, TEXT("MLARALOG: Full path: %s"), *Filepath);
#endif
	FOnImportComplete OnImportComplete;
	OnImportComplete.AddUObject(this, &UGLTFRuntimeImporter::OnGeometryLoaded);
	UE_LOG(LogTemp, Warning, TEXT("Starting importing geometry."));
	FAssimpImport::StartImport(Filepath, OnImportComplete);

	return true;
}

void UGLTFRuntimeImporter::OnGeometryLoaded(FGLTFRuntimeAsset * Asset)
{
	if (Asset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Geometry loaded, starting to load materials."));
		FString FoderPath = FPaths::GetPath(AssetFilePath);
		Asset->Name = FoderPath;

#if PLATFORM_IOS
		int32 posDoc = AssetFilePath.Find(TEXT("Documents/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		AssetFilePath = AssetFilePath.Mid(posDoc + 10);
#endif

		GLTFRuntimeMaterials::ImportMaterials(Asset, AssetFilePath);
		for (auto Material : Asset->Materials)
			Materials.AddUnique(Material);
		if (OnImportComplete.IsBound())
		{
			OnImportComplete.Broadcast(Asset);
			OnImportComplete.Clear();
		}
	}
	else
		UE_LOG(LogTemp, Warning, TEXT("Failed to load geometry, aborting."));

}

FString UGLTFRuntimeImporter::GetAbsolutePathToSaved()
{
	FString ExternalDirPath;
#if PLATFORM_ANDROID
	JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	GetExternalDirMethodId = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "getExternalDir", "()Ljava/lang/String;", false);
	jstring externalDirPath = (jstring)Env->CallObjectMethod(FJavaWrapper::GameActivityThis, GetExternalDirMethodId);
	if (externalDirPath != NULL)
	{
		const char* JavaChars = Env->GetStringUTFChars(externalDirPath, 0);
		ExternalDirPath = FString(UTF8_TO_TCHAR(JavaChars));
		Env->ReleaseStringUTFChars(externalDirPath, JavaChars);
		Env->DeleteLocalRef(externalDirPath);
	}
#endif

	return ExternalDirPath + "UE4Game/" + FApp::GetProjectName();
}
