// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "GLTFRuntimeAsset.h"
#include "RunnableThread.h"
#include "ThreadSafeBool.h"
#include "Runnable.h"
#include "Async.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GLTFRuntimeImporter.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnImportComplete, FGLTFRuntimeAsset *)

class FAssimpImport : FRunnable
{
public:

	~FAssimpImport()
	{
		bRunning = false;
		if (Thread){
			delete Thread;
			Thread = nullptr;
		}
	}

	// Begin FRunnable interface

	virtual uint32 Run() override;

	virtual void Stop() override
	{
		bRunning = false;
		if (Thread) {
			Thread->WaitForCompletion();
		}
	}

	virtual bool Init() override { return OnImportComplete.IsBound() && !FilePath.IsEmpty(); }

	virtual void Exit() override
	{
		UE_LOG(LogTemp, Warning, TEXT("Exit geometry load."));

		if (OnImportComplete.IsBound())
			AsyncTask(ENamedThreads::GameThread, [&]()
		{
			OnImportComplete.Broadcast(GLTFAsset);
			OnImportComplete.Clear();
			Stop();
		}
		);
	}

	// End FRunnable interface

	bool IsRunning() const { return bRunning; }

	FOnImportComplete GetOnImportComplete() const { return OnImportComplete; }

	void SetReadPath(FString NewFilePath) { FilePath = NewFilePath; }

	static void StartImport(FString NewFilePath, FOnImportComplete NewOnImportComplete, FGLTFRuntimeAsset * Asset = nullptr)
	{
		if(!Instance)
			Instance = new FAssimpImport(NewFilePath, NewOnImportComplete);
 		else
		{	
			Instance->FilePath = NewFilePath;
			Instance->OnImportComplete = NewOnImportComplete;
			Instance->Run();
		}
	}

private:

	FAssimpImport(FString NewFilePath, FOnImportComplete NewOnImportComplete, FGLTFRuntimeAsset * Asset) :
		bRunning(true),
		FilePath(NewFilePath),
		OnImportComplete(NewOnImportComplete),
		GLTFAsset(Asset)
	{
		Thread = FRunnableThread::Create(this, TEXT("AssimpImport"));
	}

	FAssimpImport(FString NewFilePath, FOnImportComplete NewOnImportComplete)
		: FAssimpImport(NewFilePath, NewOnImportComplete, nullptr) {}

	FOnImportComplete OnImportComplete;

	// Singleton instance
	static FAssimpImport * Instance;

	FGLTFRuntimeAsset * GLTFAsset;

	FString FilePath;

	FThreadSafeBool bRunning;

	FRunnableThread* Thread;

	FCriticalSection ImportCriticalSection;
};

UCLASS()
class RUNTIMEMESHLOADER_API UGLTFRuntimeImporter : public UObject
{
	GENERATED_BODY()
private:

protected:

	FString GetAbsolutePathToSaved();

	FString AssetFilePath;

	void OnGeometryLoaded(FGLTFRuntimeAsset * Asset);

	UPROPERTY()
	TArray<UMaterialInstanceDynamic *> Materials;

public:

	FOnImportComplete OnImportComplete;

	bool LoadAsset(FString Filepath);
    
    void DestroyMaterials()
    {
        Materials.Empty();
    }



};
