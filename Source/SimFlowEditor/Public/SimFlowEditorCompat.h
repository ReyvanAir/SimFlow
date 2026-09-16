// Copyright SimFlow. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "Misc/EngineVersionComparison.h"

/**
 * Slate is moving its 2D vector APIs from double precision (FVector2D) to single (FVector2f), and
 * FEdGraphSchemaAction::PerformAction got caught up in it. The parameter type we need in order to
 * override that virtual depends on which engine we compile against:
 *
 *   0 - const FVector2D     the only signature 5.4 declares
 *   3 - const FVector2f&    added in 5.6, alongside UE_DEPRECATED(5.6) on the FVector2D overload
 *
 * The default picks by engine version, so an ordinary build needs nothing here. Mode 0 on 5.6 or
 * later does still compile - the engine's FVector2f& overload forwards to the deprecated one we
 * override - but each override warns, and a target that promotes deprecation warnings to errors
 * (Fab submission does) will not build.
 *
 * Modes 1 and 2 used to offer UE::Slate::FDeprecateVector2DParameter. No release declares a
 * PerformAction overload taking it, so they are gone. 0 and 3 keep their old numbers.
 *
 * To force one from SimFlowEditor.Build.cs, use PublicDefinitions rather than PrivateDefinitions.
 * The type below lands in the signature of SIMFLOWEDITOR_API structs in SimFlowGraphSchema.h, so
 * every module that includes that header has to agree with us on it - a private define would give
 * consumers a different signature and a link error:
 *
 *     PublicDefinitions.Add("SIMFLOW_PERFORMACTION_LOCATION_MODE=3");
 *
 * Get it wrong and the compiler says "method with override specifier did not override any base
 * class methods". Open EdGraphSchema.h, find the PerformAction overload taking a single
 * UEdGraphPin* FromPin, and match its third parameter.
 *
 * Checked against the 5.4, 5.6 and 5.8 headers.
 */
#ifndef SIMFLOW_PERFORMACTION_LOCATION_MODE
	#if UE_VERSION_OLDER_THAN(5, 6, 0)
		#define SIMFLOW_PERFORMACTION_LOCATION_MODE 0
	#else
		#define SIMFLOW_PERFORMACTION_LOCATION_MODE 3
	#endif
#endif

#if SIMFLOW_PERFORMACTION_LOCATION_MODE == 0
using FSimFlowGraphLocation = const FVector2D;
#elif SIMFLOW_PERFORMACTION_LOCATION_MODE == 3
using FSimFlowGraphLocation = const FVector2f&;
#else
#error "SIMFLOW_PERFORMACTION_LOCATION_MODE must be 0 or 3 - see the comment above."
#endif

 /** Normalises whatever that type is into a plain FVector2D for our own code. */
#define SIMFLOW_GRAPH_LOCATION_TO_VECTOR2D(Loc) FVector2D((Loc).X, (Loc).Y)
