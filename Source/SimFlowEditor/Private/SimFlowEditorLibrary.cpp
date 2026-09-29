// Copyright SimFlow. All Rights Reserved.

#include "SimFlowEditorLibrary.h"
#include "SimFlowGraph.h"
#include "SimFlowGraphNode.h"
#include "SimFlowAsset.h"
#include "SimFlowNode.h"
#include "SimFlowNodes.h"
#include "SimFlowTask.h"
#include "SimFlowCondition.h"
#include "SimFlowComponent.h"
#include "SimFlowInstance.h"
#include "SimFlowBlackboard.h"
#include "SimFlowScenarioRecord.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "GameplayTagContainer.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	FString ToJsonString(const TSharedRef<FJsonObject>& Json)
	{
		FString Out;
		FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Out));
		return Out;
	}

	/** True for the slots that hold a sub-object owned by the node: tasks and conditions, or anything marked Instanced. */
	bool HoldsInstancedObject(const FObjectProperty* Property)
	{
		return Property->HasAnyPropertyFlags(CPF_PersistentInstance)
			|| Property->PropertyClass->IsChildOf(USimFlowTask::StaticClass())
			|| Property->PropertyClass->IsChildOf(USimFlowCondition::StaticClass());
	}

	/**
	 * Every editable property as JSON. The converter would write an Instanced object as its base
	 * class and lose the subclass, so those are written by hand as {class, classPath, properties}.
	 * The callback reaches them inside arrays and structs too, which is where Branch keeps its conditions.
	 */
	TSharedRef<FJsonObject> ObjectToJson(const UObject* Object, int32 Depth = 0)
	{
		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		FJsonObjectConverter::CustomExportCallback Callback;
		Callback.BindLambda([Depth](FProperty* Property, const void* Value) -> TSharedPtr<FJsonValue>
		{
			const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
			if (!ObjectProperty || !HoldsInstancedObject(ObjectProperty))
			{
				return nullptr;
			}
			const UObject* Child = ObjectProperty->GetObjectPropertyValue(Value);
			if (!Child || Depth >= 8)
			{
				return MakeShared<FJsonValueNull>();
			}
			const TSharedRef<FJsonObject> ChildJson = MakeShared<FJsonObject>();
			ChildJson->SetStringField(TEXT("class"), Child->GetClass()->GetName());
			ChildJson->SetStringField(TEXT("classPath"), Child->GetClass()->GetPathName());
			ChildJson->SetObjectField(TEXT("properties"), ObjectToJson(Child, Depth + 1));
			return MakeShared<FJsonValueObject>(ChildJson);
		});
		FJsonObjectConverter::UStructToJsonObject(Object->GetClass(), Object, Json, CPF_Edit, 0, &Callback);
		return Json;
	}

	/** Every loaded or on-disk subclass of Base, Blueprint ones included. */
	TArray<UClass*> GatherClasses(UClass* Base)
	{
		TArray<UClass*> Classes;
		Classes.Add(Base);

		IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		TSet<FTopLevelAssetPath> Derived;
		Registry.GetDerivedClassNames({ Base->GetClassPathName() }, {}, Derived);
		for (const FTopLevelAssetPath& Path : Derived)
		{
			const FString Name = Path.GetAssetName().ToString();
			if (Name.StartsWith(TEXT("SKEL_")) || Name.StartsWith(TEXT("REINST_")) || Name.StartsWith(TEXT("HOTRELOADED_")))
			{
				continue;
			}
			UClass* Class = LoadObject<UClass>(nullptr, *Path.ToString());
			if (Class && !Class->HasAnyClassFlags(CLASS_Deprecated | CLASS_NewerVersionExists))
			{
				Classes.AddUnique(Class);
			}
		}
		return Classes;
	}
}

bool USimFlowEditorLibrary::SyncGraphFromAsset(USimFlowAsset* Asset)
{
	if (!Asset)
	{
		return false;
	}

	USimFlowGraph* Graph = Cast<USimFlowGraph>(Asset->EdGraph);
	if (!Graph)
	{
		Graph = USimFlowGraph::CreateGraphForAsset(Asset);
		Asset->EdGraph = Graph;
	}
	if (!Graph)
	{
		return false;
	}

	// RebuildFromAsset starts from an empty graph. Comment boxes exist only in the
	// graph, never in the runtime nodes, so carry them across by hand.
	TArray<UEdGraphNode*> Comments;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node && !Cast<USimFlowGraphNode>(Node))
		{
			Comments.Add(Node);
		}
	}

	Graph->RebuildFromAsset();
	for (UEdGraphNode* Comment : Comments)
	{
		Graph->AddNode(Comment, /*bFromUI*/ false, /*bSelectNewNode*/ false);
	}

	Asset->MarkPackageDirty();
	return true;
}

FString USimFlowEditorLibrary::DescribeFlow(const USimFlowAsset* Asset)
{
	if (!Asset)
	{
		return FString();
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("asset"), Asset->GetPathName());
	Root->SetStringField(TEXT("displayName"), Asset->GetDisplayNameText().ToString());

	TArray<TSharedPtr<FJsonValue>> Entries;
	for (const FName Entry : Asset->GetEntryNames())
	{
		Entries.Add(MakeShared<FJsonValueString>(Entry.ToString()));
	}
	Root->SetArrayField(TEXT("entries"), Entries);

	TArray<TSharedPtr<FJsonValue>> Nodes;
	for (const TObjectPtr<USimFlowNode>& Node : Asset->Nodes)
	{
		if (!Node)
		{
			continue;
		}

		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
		Json->SetStringField(TEXT("class"), Node->GetClass()->GetName());
		Json->SetStringField(TEXT("classPath"), Node->GetClass()->GetPathName());
		Json->SetObjectField(TEXT("properties"), ObjectToJson(Node));
		Json->SetStringField(TEXT("title"), Node->GetNodeTitle().ToString());
		Json->SetStringField(TEXT("subtitle"), Node->GetNodeSubtitle().ToString());
		if (!Node->NodeComment.IsEmpty())
		{
			Json->SetStringField(TEXT("comment"), Node->NodeComment);
		}
#if WITH_EDITORONLY_DATA
		Json->SetArrayField(TEXT("position"), {
			MakeShared<FJsonValueNumber>(Node->GraphPosition.X),
			MakeShared<FJsonValueNumber>(Node->GraphPosition.Y) });
#endif

		TArray<TSharedPtr<FJsonValue>> Inputs;
		for (const FSimFlowInputPin& Pin : Node->InputPins)
		{
			Inputs.Add(MakeShared<FJsonValueString>(Pin.PinName.ToString()));
		}
		Json->SetArrayField(TEXT("inputs"), Inputs);

		TArray<TSharedPtr<FJsonValue>> Outputs;
		for (const FSimFlowOutputPin& Pin : Node->OutputPins)
		{
			const TSharedRef<FJsonObject> PinJson = MakeShared<FJsonObject>();
			PinJson->SetStringField(TEXT("pin"), Pin.PinName.ToString());

			TArray<TSharedPtr<FJsonValue>> Links;
			for (const FSimFlowPinLink& Link : Pin.Links)
			{
				const TSharedRef<FJsonObject> LinkJson = MakeShared<FJsonObject>();
				LinkJson->SetStringField(TEXT("node"), Link.NodeGuid.ToString());
				LinkJson->SetStringField(TEXT("pin"), Link.PinName.ToString());
				Links.Add(MakeShared<FJsonValueObject>(LinkJson));
			}
			PinJson->SetArrayField(TEXT("links"), Links);
			Outputs.Add(MakeShared<FJsonValueObject>(PinJson));
		}
		Json->SetArrayField(TEXT("outputs"), Outputs);

		if (const USimFlowNode_Task* TaskNode = Cast<USimFlowNode_Task>(Node))
		{
			if (const USimFlowTask* Task = TaskNode->Task)
			{
				const TSharedRef<FJsonObject> TaskJson = MakeShared<FJsonObject>();
				TaskJson->SetStringField(TEXT("class"), Task->GetClass()->GetName());
				TaskJson->SetStringField(TEXT("taskId"), Task->TaskId.ToString());
				TaskJson->SetStringField(TEXT("displayName"), Task->GetDisplayNameText().ToString());
				TaskJson->SetStringField(TEXT("instruction"), Task->Instruction.ToString());
				Json->SetObjectField(TEXT("task"), TaskJson);
			}
		}
		else if (const USimFlowNode_SubFlow* SubNode = Cast<USimFlowNode_SubFlow>(Node))
		{
			Json->SetStringField(TEXT("subFlow"), SubNode->SubFlow ? SubNode->SubFlow->GetPathName() : FString());
			Json->SetStringField(TEXT("entryName"), SubNode->EntryName.ToString());
		}

		Nodes.Add(MakeShared<FJsonValueObject>(Json));
	}
	Root->SetArrayField(TEXT("nodes"), Nodes);

	TArray<FString> Errors;
	TArray<FString> Warnings;
	Asset->ValidateFlow(Errors, Warnings);
	TArray<TSharedPtr<FJsonValue>> ErrorValues;
	for (const FString& Error : Errors)
	{
		ErrorValues.Add(MakeShared<FJsonValueString>(Error));
	}
	TArray<TSharedPtr<FJsonValue>> WarningValues;
	for (const FString& Warning : Warnings)
	{
		WarningValues.Add(MakeShared<FJsonValueString>(Warning));
	}
	Root->SetArrayField(TEXT("errors"), ErrorValues);
	Root->SetArrayField(TEXT("warnings"), WarningValues);

	FString Out;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

void USimFlowEditorLibrary::SetNodePosition(USimFlowNode* Node, FVector2D Position)
{
#if WITH_EDITORONLY_DATA
	if (Node)
	{
		Node->Modify();
		Node->GraphPosition = Position;
	}
#endif
}

USimFlowNode* USimFlowEditorLibrary::FindNodeByGuidString(const USimFlowAsset* Asset, const FString& Guid)
{
	FGuid Parsed;
	return Asset && FGuid::Parse(Guid, Parsed) ? Asset->FindNodeByGuid(Parsed) : nullptr;
}

FString USimFlowEditorLibrary::GetNodeGuidString(const USimFlowNode* Node)
{
	return Node ? Node->NodeGuid.ToString() : FString();
}

namespace
{
	/** What the Details panel does after an edit: the object, and the node around it, rebuild. */
	void NotifyEdited(UObject* Object)
	{
		Object->PostEditChange();
		USimFlowNode* Node = Object->GetTypedOuter<USimFlowNode>();
		if (Node && Node != Object)
		{
			Node->PostEditChange();
		}
	}
}

FString USimFlowEditorLibrary::SetPropertiesFromJson(UObject* Object, const FString& Json)
{
	if (!Object)
	{
		return TEXT("Object is null.");
	}

	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid())
	{
		return TEXT("Properties must be a JSON object, e.g. {\"TimeLimit\": 30}.");
	}

	// The converter skips a name it does not know without saying so. Say so, before touching anything.
	TArray<FString> Rejected;
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Root->Values)
	{
		const FProperty* Property = Object->GetClass()->FindPropertyByName(*Pair.Key);
		if (!Property || !Property->HasAllPropertyFlags(CPF_Edit) || Property->HasAnyPropertyFlags(CPF_PersistentInstance))
		{
			Rejected.Add(Pair.Key);
		}
	}
	if (Rejected.Num() > 0)
	{
		return FString::Printf(
			TEXT("Not editable plain properties of %s: %s. Nothing was changed. Instanced objects (a task, a condition) are set with SetInstancedProperty; DescribeClass lists the rest."),
			*Object->GetClass()->GetName(), *FString::Join(Rejected, TEXT(", ")));
	}

	Object->Modify();
	if (!FJsonObjectConverter::JsonObjectToUStruct(Root.ToSharedRef(), Object->GetClass(), Object, CPF_Edit, CPF_PersistentInstance))
	{
		NotifyEdited(Object);
		return TEXT("A value did not fit its property's type, and earlier ones may already be set. Read the object back to see what took.");
	}

	NotifyEdited(Object);
	return FString();
}

FString USimFlowEditorLibrary::SetInstancedProperty(UObject* Owner, const FString& PropertyPath, UClass* ObjectClass, const FString& PropertiesJson)
{
	if (!Owner)
	{
		return TEXT("Owner is null.");
	}

	// Walk "Cases[0].Condition" down to the slot. Structs, array elements and existing Instanced
	// objects are stepped into; the last step must be the object slot itself, or an array to append to.
	// Holder is whichever object owns the slot, so the new sub-object gets the right outer.
	UObject* Holder = Owner;
	const UStruct* Struct = Owner->GetClass();
	void* Container = Owner;
	TArray<FString> Steps;
	PropertyPath.ParseIntoArray(Steps, TEXT("."));
	if (Steps.IsEmpty())
	{
		return TEXT("PropertyPath is empty.");
	}

	for (int32 StepIndex = 0; StepIndex < Steps.Num(); ++StepIndex)
	{
		FString Name = Steps[StepIndex];
		int32 ElementIndex = INDEX_NONE;
		int32 Bracket = INDEX_NONE;
		if (Name.FindChar(TEXT('['), Bracket))
		{
			ElementIndex = FCString::Atoi(*Name.Mid(Bracket + 1));
			Name.LeftInline(Bracket);
		}

		const FProperty* Property = Struct->FindPropertyByName(*Name);
		if (!Property || !Property->HasAllPropertyFlags(CPF_Edit))
		{
			return FString::Printf(TEXT("'%s' is not an editable property of %s."), *Name, *Struct->GetName());
		}
		void* Value = Property->ContainerPtrToValuePtr<void>(Container);
		const bool bLastStep = StepIndex == Steps.Num() - 1;
		bool bAppend = false;

		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Array(ArrayProperty, Value);
			if (ElementIndex == INDEX_NONE)
			{
				if (!bLastStep)
				{
					return FString::Printf(TEXT("'%s' is an array; say which element, e.g. %s[0]."), *Name, *Name);
				}
				bAppend = true;
			}
			else if (!Array.IsValidIndex(ElementIndex))
			{
				return FString::Printf(TEXT("'%s' has %d elements; index %d is out of range."), *Name, Array.Num(), ElementIndex);
			}
			else
			{
				Value = Array.GetRawPtr(ElementIndex);
			}
			Property = ArrayProperty->Inner;
		}
		else if (ElementIndex != INDEX_NONE)
		{
			return FString::Printf(TEXT("'%s' is not an array."), *Name);
		}

		if (!bLastStep)
		{
			if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
			{
				Struct = StructProperty->Struct;
				Container = Value;
			}
			else if (const FObjectProperty* StepObject = CastField<FObjectProperty>(Property))
			{
				UObject* Child = StepObject->GetObjectPropertyValue(Value);
				if (!Child)
				{
					return FString::Printf(TEXT("'%s' is empty; create it first."), *Name);
				}
				Holder = Child;
				Struct = Child->GetClass();
				Container = Child;
			}
			else
			{
				return FString::Printf(TEXT("'%s' holds no fields to step into."), *Name);
			}
			continue;
		}

		const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
		if (!ObjectProperty || !HoldsInstancedObject(ObjectProperty))
		{
			return FString::Printf(TEXT("'%s' is not an Instanced object slot; use SetPropertiesFromJson for plain values."), *PropertyPath);
		}
		if (ObjectClass && !ObjectClass->IsChildOf(ObjectProperty->PropertyClass))
		{
			return FString::Printf(TEXT("%s does not derive from %s, which '%s' takes."),
				*ObjectClass->GetName(), *ObjectProperty->PropertyClass->GetName(), *PropertyPath);
		}
		if (ObjectClass && ObjectClass->HasAnyClassFlags(CLASS_Abstract))
		{
			return FString::Printf(TEXT("%s is abstract; pick a concrete subclass."), *ObjectClass->GetName());
		}

		Holder->Modify();
		if (bAppend)
		{
			if (!ObjectClass)
			{
				return TEXT("Appending needs a class; to empty an array element give its index.");
			}
			FScriptArrayHelper Array(CastField<FArrayProperty>(Struct->FindPropertyByName(*Name)), Value);
			Value = Array.GetRawPtr(Array.AddValue());
		}
		UObject* Created = ObjectClass ? NewObject<UObject>(Holder, ObjectClass, NAME_None, RF_Transactional) : nullptr;
		ObjectProperty->SetObjectPropertyValue(Value, Created);

		NotifyEdited(Holder);
		return Created && !PropertiesJson.IsEmpty() ? SetPropertiesFromJson(Created, PropertiesJson) : FString();
	}
	return FString();
}

UClass* USimFlowEditorLibrary::FindClass(UClass* BaseClass, const FString& NameOrPath)
{
	if (!BaseClass || NameOrPath.IsEmpty())
	{
		return nullptr;
	}

	if (NameOrPath.Contains(TEXT("/")) || NameOrPath.Contains(TEXT(".")))
	{
		UClass* Class = LoadObject<UClass>(nullptr, *NameOrPath);
		return Class && Class->IsChildOf(BaseClass) ? Class : nullptr;
	}

	UClass* Match = nullptr;
	for (UClass* Class : GatherClasses(BaseClass))
	{
		const FString Name = Class->GetName();
		const bool bHit = Name.Equals(NameOrPath, ESearchCase::IgnoreCase)
			|| Name.Equals(TEXT("SimFlow") + NameOrPath, ESearchCase::IgnoreCase)
			|| Name.Equals(NameOrPath + TEXT("_C"), ESearchCase::IgnoreCase);
		if (bHit)
		{
			if (Match)
			{
				return nullptr; // ambiguous; the caller should pass the full path
			}
			Match = Class;
		}
	}
	return Match;
}

FString USimFlowEditorLibrary::ListClasses(UClass* BaseClass)
{
	if (!BaseClass)
	{
		return FString();
	}

	TArray<TSharedPtr<FJsonValue>> Entries;
	for (UClass* Class : GatherClasses(BaseClass))
	{
		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("name"), Class->GetName());
		Json->SetStringField(TEXT("path"), Class->GetPathName());
		Json->SetStringField(TEXT("description"), Class->GetToolTipText().ToString());
		Json->SetBoolField(TEXT("abstract"), Class->HasAnyClassFlags(CLASS_Abstract));
		Json->SetBoolField(TEXT("blueprint"), !Class->IsNative());
		if (const USimFlowNode* Node = Cast<USimFlowNode>(Class->GetDefaultObject()))
		{
			Json->SetBoolField(TEXT("placeable"), Node->IsPlaceableInGraph());
			Json->SetStringField(TEXT("category"), Node->GetNodeCategory().ToString());
		}
		Entries.Add(MakeShared<FJsonValueObject>(Json));
	}

	FString Out;
	FJsonSerializer::Serialize(Entries, TJsonWriterFactory<>::Create(&Out));
	return Out;
}

FString USimFlowEditorLibrary::DescribeClass(UClass* Class)
{
	if (!Class)
	{
		return FString();
	}

	const UObject* Defaults = Class->GetDefaultObject();
	const TSharedRef<FJsonObject> DefaultValues = ObjectToJson(Defaults);

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("name"), Class->GetName());
	Root->SetStringField(TEXT("path"), Class->GetPathName());
	Root->SetStringField(TEXT("parent"), Class->GetSuperClass() ? Class->GetSuperClass()->GetName() : FString());
	Root->SetStringField(TEXT("description"), Class->GetToolTipText().ToString());
	Root->SetBoolField(TEXT("abstract"), Class->HasAnyClassFlags(CLASS_Abstract));
	if (const USimFlowNode* Node = Cast<USimFlowNode>(Defaults))
	{
		Root->SetStringField(TEXT("category"), Node->GetNodeCategory().ToString());
		Root->SetStringField(TEXT("tooltip"), Node->GetNodeTooltip().ToString());
	}

	TArray<TSharedPtr<FJsonValue>> Properties;
	for (TFieldIterator<FProperty> It(Class); It; ++It)
	{
		const FProperty* Property = *It;
		if (!Property->HasAllPropertyFlags(CPF_Edit))
		{
			continue;
		}

		const TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("name"), Property->GetName());
		Json->SetStringField(TEXT("type"), Property->GetCPPType());
		Json->SetStringField(TEXT("category"), Property->GetMetaData(TEXT("Category")));
		Json->SetStringField(TEXT("tooltip"), Property->GetToolTipText().ToString());

		const FProperty* Element = Property;
		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			Element = ArrayProperty->Inner;
		}
		if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Element))
		{
			if (HoldsInstancedObject(ObjectProperty))
			{
				Json->SetStringField(TEXT("instancedOf"), ObjectProperty->PropertyClass->GetName());
			}
		}
		const UEnum* Enum = nullptr;
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Element))
		{
			Enum = EnumProperty->GetEnum();
		}
		else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Element))
		{
			Enum = ByteProperty->Enum;
		}
		if (Enum)
		{
			TArray<TSharedPtr<FJsonValue>> Options;
			for (int32 Index = 0; Index < Enum->NumEnums() - 1; ++Index)
			{
				Options.Add(MakeShared<FJsonValueString>(Enum->GetNameStringByIndex(Index)));
			}
			Json->SetArrayField(TEXT("options"), Options);
		}

		if (const TSharedPtr<FJsonValue> Default = DefaultValues->TryGetField(FJsonObjectConverter::StandardizeCase(Property->GetName())))
		{
			Json->SetField(TEXT("default"), Default);
		}
		Properties.Add(MakeShared<FJsonValueObject>(Json));
	}
	Root->SetArrayField(TEXT("properties"), Properties);
	return ToJsonString(Root);
}

FString USimFlowEditorLibrary::DescribeRunningFlow(const USimFlowComponent* Component)
{
	if (!Component)
	{
		return FString();
	}

	const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("flowSaveId"), Component->GetEffectiveSaveId().ToString());
	Root->SetStringField(TEXT("owner"), Component->GetOwner() ? Component->GetOwner()->GetPathName() : FString());
	Root->SetStringField(TEXT("flowAsset"), Component->FlowAsset ? Component->FlowAsset->GetPathName() : FString());
	Root->SetStringField(TEXT("runState"), StaticEnum<ESimFlowRunState>()->GetNameStringByValue(static_cast<int64>(Component->GetRunState())));
	Root->SetNumberField(TEXT("progress"), Component->GetProgress());
	Root->SetNumberField(TEXT("score"), Component->GetScore());
	Root->SetStringField(TEXT("currentTask"), Component->GetCurrentTaskName().ToString());
	Root->SetStringField(TEXT("currentInstruction"), Component->GetCurrentInstruction().ToString());
	Root->SetNumberField(TEXT("currentTaskRemainingSeconds"), Component->GetCurrentTaskRemainingTime());

	const FSimFlowScenarioRecord ScenarioRecord = Component->GetScenarioRecord();
	const TSharedRef<FJsonObject> Record = MakeShared<FJsonObject>();
	FJsonObjectConverter::UStructToJsonObject(FSimFlowScenarioRecord::StaticStruct(), &ScenarioRecord, Record);
	Root->SetObjectField(TEXT("scenarioRecord"), Record);

	if (const USimFlowInstance* Instance = Component->GetFlowInstance())
	{
		Root->SetNumberField(TEXT("elapsedSeconds"), Instance->GetElapsedTime());
		Root->SetStringField(TEXT("entry"), Instance->GetEntryName().ToString());

		TArray<TSharedPtr<FJsonValue>> Active;
		for (const USimFlowNode* Node : Instance->GetActiveNodes())
		{
			const TSharedRef<FJsonObject> NodeJson = MakeShared<FJsonObject>();
			NodeJson->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
			NodeJson->SetStringField(TEXT("class"), Node->GetClass()->GetName());
			NodeJson->SetStringField(TEXT("title"), Node->GetNodeTitle().ToString());
			NodeJson->SetStringField(TEXT("status"), Node->GetDebugStatus());
			Active.Add(MakeShared<FJsonValueObject>(NodeJson));
		}
		Root->SetArrayField(TEXT("activeNodes"), Active);

		TArray<TSharedPtr<FJsonValue>> Mistakes;
		for (const FSimFlowMistake& Mistake : Instance->GetMistakes())
		{
			const TSharedRef<FJsonObject> MistakeJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSimFlowMistake::StaticStruct(), &Mistake, MistakeJson);
			Mistakes.Add(MakeShared<FJsonValueObject>(MistakeJson));
		}
		Root->SetArrayField(TEXT("mistakes"), Mistakes);
	}

	TArray<TSharedPtr<FJsonValue>> Blackboard;
	if (const USimFlowBlackboard* Board = Component->GetBlackboard())
	{
		for (const FSimFlowBlackboardEntry& Entry : Board->ToEntries(true))
		{
			const TSharedRef<FJsonObject> EntryJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSimFlowBlackboardEntry::StaticStruct(), &Entry, EntryJson);
			Blackboard.Add(MakeShared<FJsonValueObject>(EntryJson));
		}
	}
	Root->SetArrayField(TEXT("blackboard"), Blackboard);

	return ToJsonString(Root);
}

bool USimFlowEditorLibrary::SendEventByName(USimFlowComponent* Component, const FString& TagName)
{
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
	if (!Component || !Tag.IsValid())
	{
		return false;
	}
	Component->SendEvent(Tag, nullptr);
	return true;
}
