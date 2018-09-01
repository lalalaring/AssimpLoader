// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GLTFRuntimeImporter.h"
#include "GLTFRuntimeAsset.h"

#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstance.h"

#include "Loader.generated.h"

UCLASS()
class ASSIMPLOADER_API ALoader : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALoader();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UProceduralMeshComponent * ProceduralMesh;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
    void LoadAssimpModel(FString Filepath);
    
    //UFUNCTION()
    void OnLoadComplete(FGLTFRuntimeAsset * LoadedAsset);
	
    UGLTFRuntimeImporter* Importer;
};
