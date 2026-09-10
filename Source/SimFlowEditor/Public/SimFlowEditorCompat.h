// Copyright SimFlow. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"

/**
 * Slate is migrating its 2D vector APIs from double precision (FVector2D) to single (FVector2f),
 * and FEdGraphSchemaAction::PerformAction is caught up in it. The parameter type we need in order
 * to override that virtual depends on how far along the engine we compile against has got:
 *
 *   0 - const FVector2D                                  the original; still present through 5.8
 *   1 - const UE::Slate::FDeprecateVector2DParameter&    transitional
 *   2 - const UE::Slate::FDeprecateVector2DParameter     transitional, by value
 *   3 - const FVector2f&                                 the post-migration signature
 *
 * Mode 0 is correct for 5.6 through 5.8 alike: 5.8 still declares all four PerformAction overloads
 * and does not mark the FVector2D one deprecated.
 *
 * Two things to watch for on a future engine bump:
 *
 *  - If the FVector2D overload is removed, this stops compiling with "method with override
 *    specifier did not override any base class methods". Switch to mode 3.
 *  - EdGraphSchema.h tags that overload with UE_SLATE_DEPRECATED_VECTOR_VIRTUAL_FUNCTION, which
 *    expands to `final` when a module both opts into UE_REPORT_SLATE_VECTOR_DEPRECATION and builds
 *    with warnings-as-errors. SimFlowEditor does not opt in, so the overload stays overridable. If
 *    you ever add PrivateDefinitions.Add("UE_REPORT_SLATE_VECTOR_DEPRECATION=1") to
 *    SimFlowEditor.Build.cs, move to mode 3 in the same change.
 *
 * Set it from outside rather than editing here if you prefer - in SimFlowEditor.Build.cs:
 *     PrivateDefinitions.Add("SIMFLOW_PERFORMACTION_LOCATION_MODE=3");
 */
#ifndef SIMFLOW_PERFORMACTION_LOCATION_MODE
	#define SIMFLOW_PERFORMACTION_LOCATION_MODE 0
#endif

#if SIMFLOW_PERFORMACTION_LOCATION_MODE == 0
using FSimFlowGraphLocation = const FVector2D;
#elif SIMFLOW_PERFORMACTION_LOCATION_MODE == 1
using FSimFlowGraphLocation = const UE::Slate::FDeprecateVector2DParameter&;
#elif SIMFLOW_PERFORMACTION_LOCATION_MODE == 2
using FSimFlowGraphLocation = const UE::Slate::FDeprecateVector2DParameter;
#elif SIMFLOW_PERFORMACTION_LOCATION_MODE == 3
using FSimFlowGraphLocation = const FVector2f&;
#else
#error "SIMFLOW_PERFORMACTION_LOCATION_MODE must be 0, 1, 2 or 3 - see the comment above."
#endif

 /** Normalises whatever that type is into a plain FVector2D for our own code. */
#define SIMFLOW_GRAPH_LOCATION_TO_VECTOR2D(Loc) FVector2D((Loc).X, (Loc).Y)
