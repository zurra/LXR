// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "LXRFlux.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FLXRFluxModule"

DEFINE_LOG_CATEGORY(LogLXRFlux);


void FLXRFluxModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin("LXR")->GetBaseDir(), TEXT("Shaders/LXRFlux"));
	AddShaderSourceDirectoryMapping(TEXT("/LXRFlux_Shaders"), PluginShaderDir);
}

void FLXRFluxModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLXRFluxModule, LXRFlux)
