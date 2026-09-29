// Copyright SimFlow. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimFlowEditorLibrary.generated.h"

class USimFlowAsset;
class USimFlowComponent;
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

	// ---- Helpers for the MCP toolset (Content/Python/simflow_toolset). Also fine to call from any script.

	/** The node whose guid prints as Guid in DescribeFlow, or null. A script has no easy way to build an FGuid. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static USimFlowNode* FindNodeByGuidString(const USimFlowAsset* Asset, const FString& Guid);

	/** The guid as DescribeFlow prints it. The property itself is not visible to scripts. */
	UFUNCTION(BlueprintPure, Category = "SimFlow|Editor Scripting")
	static FString GetNodeGuidString(const USimFlowNode* Node);

	/**
	 * Sets editable properties on a node, task, condition or the asset from a JSON object,
	 * e.g. {"TimeLimit": 30, "ScoreOnSuccess": 5}. Names are checked first: one that is not an
	 * editable property, or is an Instanced object (use SetInstancedProperty), rejects the whole
	 * call before anything is touched. Sends the same change notification the Details panel does,
	 * so a node rebuilds its pins. Returns an empty string on success, otherwise the reason.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static FString SetPropertiesFromJson(UObject* Object, const FString& Json);

	/**
	 * Creates an Instanced sub-object (a task, a condition) of ObjectClass and stores it at
	 * PropertyPath on Owner: "Task", "AbortCondition", "Cases[0].Condition", "Task.Condition"
	 * (a path steps through structs, array elements and existing Instanced objects). A path ending
	 * at an array without an index ("Task.Conditions") appends. A null ObjectClass clears the slot.
	 * PropertiesJson, when not empty, is applied to the new object as SetPropertiesFromJson would.
	 * Returns an empty string on success, otherwise the reason.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static FString SetInstancedProperty(UObject* Owner, const FString& PropertyPath, UClass* ObjectClass, const FString& PropertiesJson);

	/**
	 * Finds a subclass of BaseClass by short name ("SimFlowTask_Delay"), by the name without
	 * the SimFlow prefix ("Task_Delay"), or by full path (needed for Blueprint subclasses).
	 * Returns null when nothing matches or a short name matches more than one class.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static UClass* FindClass(UClass* BaseClass, const FString& NameOrPath);

	/** BaseClass and every subclass, native and Blueprint, as a JSON array of name, path, description, abstract. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static FString ListClasses(UClass* BaseClass);

	/** One class as JSON: description and every editable property with its type, tooltip, default and enum values. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static FString DescribeClass(UClass* Class);

	/**
	 * A running flow as JSON: state, current task, progress, score, active nodes, blackboard,
	 * mistakes and the scenario record. Empty when there is no component.
	 */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static FString DescribeRunningFlow(const USimFlowComponent* Component);

	/** Send Event with the tag given by name. False when the tag is not registered. */
	UFUNCTION(BlueprintCallable, Category = "SimFlow|Editor Scripting")
	static bool SendEventByName(USimFlowComponent* Component, const FString& TagName);
};
