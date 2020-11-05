// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "BRPlugins.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#define LOCTEXT_NAMESPACE "FBRPluginsModule"

void FBRPluginsModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("BRPlugins"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/BRPlugins"), PluginShaderDir);
}

void FBRPluginsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBRPluginsModule, BRPlugins)