// Fill out your copyright notice in the Description page of Project Settings.

#include "Loader.h"
#include "Engine.h"
#include "Async.h"

// Sets default values
ALoader::ALoader()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    
    ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GeneratedMesh"));
    ProceduralMesh->bUseAsyncCooking = false;
}

// Called when the game starts or when spawned
void ALoader::BeginPlay()
{
	Super::BeginPlay();
	if(!Importer)
        Importer = NewObject<UGLTFRuntimeImporter>();
}

// Called every frame
void ALoader::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ALoader::LoadAssimpModel(FString Filepath)
{
    if(Importer)
    {
        if(!Importer->OnImportComplete.IsBound())
            Importer->OnImportComplete.AddUObject(this, &ALoader::OnLoadComplete);
        if(Importer->OnImportComplete.IsBound())
            Importer->LoadAsset(Filepath);
    }
}

void ALoader::OnLoadComplete(FGLTFRuntimeAsset * LoadedAsset)
{
    int32 Index = 0;
    if(LoadedAsset)
    {
        if(LoadedAsset->bSuccess)
        {
            for (auto MeshInfo : LoadedAsset->MeshInfo)
            {
                ProceduralMesh->CreateMeshSection_LinearColor(Index, MeshInfo.Vertices, MeshInfo.Triangles,
                                                              MeshInfo.Normals, MeshInfo.UV0, MeshInfo.VertexColors, MeshInfo.Tangents, false);
                {
                    if (LoadedAsset->Materials.IsValidIndex(LoadedAsset->MeshInfo[Index].MaterialIndex))
                        ProceduralMesh->SetMaterial(Index, LoadedAsset->Materials[LoadedAsset->MeshInfo[Index].MaterialIndex]);
                }
                Index++;
                
            }
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Magenta, "Success");
        }
        else
            GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Magenta, "Successn't");
    }
    else
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Magenta, "no loadedAsset");
}
