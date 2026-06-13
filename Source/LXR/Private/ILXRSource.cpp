// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "ILXRSource.h"

#include "LXR.h"


// Add default functionality here for any ILightSource functions that are not pure virtual.

bool ILXRSource::IsEnabled_Implementation()
{
	UE_LOG(LogLXR, Warning, TEXT("This is Interface Default..."))
	return false;
}
