// Copyright SimFlow. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimFlowEditorLibrary.generated.h"

class USimFlowAsset;
class USimFlowNode;

/**
 * Editor-only helpers for building and reading flows from a script (Editor Utility
 * Blueprint, or Python as unreal.SimFlowEditorLibrary). Pairs with the authoring
 * functions on USimFlowAsset: AddNode, ConnectNodes, RemoveNode, ValidateFlow.
 *
 * Once an asset has been opened, its editor graph is the source of truth and is
 * written back over the runtime nodes on every graph edit. A script that edits the
 * nodes must call SyncGraphFromAsset afterwards, or the edit never shows in the
 * editor and the next graph change erases it.
 */
UCLASS()
class SIMFLOWEDITOR_API USimFlowEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Rebuilds the editor graph from the runtime nodes, creating the graph if the
	 * asset has none. Comment boxes are kept. Marks the asset dirty; save it yourself.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static bool SyncGraphFromAsset(USimFlowAsset* Asset);

	/**
	 * The whole flow as JSON: every node with its guid, class, title, pins and links,
	 * task or sub flow details, plus the ValidateFlow errors and warnings.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static FString DescribeFlow(const USimFlowAsset* Asset);

	/** Where the node sits in the graph. Scripted nodes otherwise all land at 0,0. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static void SetNodePosition(USimFlowNode* Node, FVector2D Position);
};
