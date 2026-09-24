// Copyright SimFlow. All Rights Reserved.

#include "SimFlowEditorLibrary.h"
#include "SimFlowGraph.h"
#include "SimFlowGraphNode.h"
#include "SimFlowAsset.h"
#include "SimFlowNode.h"
#include "SimFlowNodes.h"
#include "SimFlowTask.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

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
