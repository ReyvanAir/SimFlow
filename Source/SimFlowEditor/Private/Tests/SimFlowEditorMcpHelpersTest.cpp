// Copyright SimFlow. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SimFlowEditorLibrary.h"
#include "SimFlowAsset.h"
#include "SimFlowComponent.h"
#include "SimFlowConditions.h"
#include "SimFlowNodes.h"
#include "SimFlowTask.h"
#include "SimFlowTasks.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSimFlowEditorMcpHelpersTest,
	"SimFlow.Editor.McpHelpers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimFlowEditorMcpHelpersTest::RunTest(const FString&)
{
	USimFlowAsset* Asset = NewObject<USimFlowAsset>();
	USimFlowNode_Task* TaskNode = Cast<USimFlowNode_Task>(Asset->AddNode(USimFlowNode_Task::StaticClass()));
	USimFlowNode_Branch* Branch = Cast<USimFlowNode_Branch>(Asset->AddNode(USimFlowNode_Branch::StaticClass()));
	if (!TestNotNull(TEXT("nodes were added"), TaskNode) || !TestNotNull(TEXT("branch was added"), Branch))
	{
		return false;
	}

	// A node is found by the guid DescribeFlow prints.
	const FString Guid = USimFlowEditorLibrary::GetNodeGuidString(TaskNode);
	TestTrue(TEXT("find by guid string"), USimFlowEditorLibrary::FindNodeByGuidString(Asset, Guid) == TaskNode);
	TestNull(TEXT("a bad guid finds nothing"), USimFlowEditorLibrary::FindNodeByGuidString(Asset, TEXT("nonsense")));

	// Plain properties: applied together, or refused together with the reason.
	TestEqual(TEXT("valid settings apply"),
		USimFlowEditorLibrary::SetPropertiesFromJson(TaskNode, TEXT("{\"TimeLimit\": 12, \"bAutoRetryOnFailure\": true}")), FString());
	TestEqual(TEXT("TimeLimit took"), TaskNode->TimeLimit, 12.f);
	TestTrue(TEXT("bool took"), TaskNode->bAutoRetryOnFailure);

	const FString Refused = USimFlowEditorLibrary::SetPropertiesFromJson(TaskNode, TEXT("{\"TimeLimit\": 99, \"NoSuchThing\": 1}"));
	TestTrue(TEXT("an unknown name is refused"), Refused.Contains(TEXT("NoSuchThing")));
	TestEqual(TEXT("and nothing changed"), TaskNode->TimeLimit, 12.f);
	TestFalse(TEXT("an Instanced slot is refused here"),
		USimFlowEditorLibrary::SetPropertiesFromJson(TaskNode, TEXT("{\"Task\": {}}")).IsEmpty());
	TestFalse(TEXT("bad JSON is refused"), USimFlowEditorLibrary::SetPropertiesFromJson(TaskNode, TEXT("not json")).IsEmpty());

	// A task in the Task slot, with its own settings, owned by the node.
	TestEqual(TEXT("task is created"),
		USimFlowEditorLibrary::SetInstancedProperty(TaskNode, TEXT("Task"), USimFlowTask_Delay::StaticClass(), TEXT("{\"Duration\": 4}")), FString());
	USimFlowTask_Delay* Delay = Cast<USimFlowTask_Delay>(TaskNode->Task);
	if (!TestNotNull(TEXT("the slot holds a Delay"), Delay))
	{
		return false;
	}
	TestEqual(TEXT("the task got its settings"), Delay->Duration, 4.f);
	TestTrue(TEXT("the node owns the task"), Delay->GetOuter() == TaskNode);
	TestFalse(TEXT("an abstract class is refused"),
		USimFlowEditorLibrary::SetInstancedProperty(TaskNode, TEXT("Task"), USimFlowTask::StaticClass(), FString()).IsEmpty());
	TestFalse(TEXT("a class the slot cannot hold is refused"),
		USimFlowEditorLibrary::SetInstancedProperty(TaskNode, TEXT("Task"), USimFlowCondition_Constant::StaticClass(), FString()).IsEmpty());
	TestTrue(TEXT("a refused call leaves the task alone"), TaskNode->Task == Delay);
	TestEqual(TEXT("a null class clears the slot"), USimFlowEditorLibrary::SetInstancedProperty(TaskNode, TEXT("Task"), nullptr, FString()), FString());
	TestNull(TEXT("the slot is empty"), TaskNode->Task.Get());
	USimFlowEditorLibrary::SetInstancedProperty(TaskNode, TEXT("Task"), USimFlowTask_Delay::StaticClass(), TEXT("{\"Duration\": 4}"));

	// A condition inside a struct inside an array, which is where Branch keeps them.
	TestEqual(TEXT("cases are set"),
		USimFlowEditorLibrary::SetPropertiesFromJson(Branch, TEXT("{\"Cases\": [{\"Label\": \"Yes\"}, {\"Label\": \"No\"}]}")), FString());
	TestEqual(TEXT("the branch has two cases"), Branch->Cases.Num(), 2);
	TestEqual(TEXT("and rebuilt its pins: two cases and Default"), Branch->OutputPins.Num(), 3);
	TestEqual(TEXT("a condition goes into Cases[1]"),
		USimFlowEditorLibrary::SetInstancedProperty(Branch, TEXT("Cases[1].Condition"), USimFlowCondition_Constant::StaticClass(), TEXT("{\"bValue\": false}")), FString());
	const USimFlowCondition_Constant* Constant = Cast<USimFlowCondition_Constant>(Branch->Cases[1].Condition);
	if (TestNotNull(TEXT("Cases[1] holds a Constant"), Constant))
	{
		TestFalse(TEXT("it got its setting"), Constant->bValue);
		TestTrue(TEXT("the branch owns it"), Constant->GetOuter() == Branch);
	}
	TestFalse(TEXT("an index past the end is refused"),
		USimFlowEditorLibrary::SetInstancedProperty(Branch, TEXT("Cases[5].Condition"), USimFlowCondition_Constant::StaticClass(), FString()).IsEmpty());
	TestFalse(TEXT("an array step without an index is refused"),
		USimFlowEditorLibrary::SetInstancedProperty(Branch, TEXT("Cases.Condition"), USimFlowCondition_Constant::StaticClass(), FString()).IsEmpty());

	// DescribeFlow shows the settings, including the task and the condition by subclass.
	TSharedPtr<FJsonObject> Flow;
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(USimFlowEditorLibrary::DescribeFlow(Asset)), Flow);
	if (!TestTrue(TEXT("DescribeFlow parses"), Flow.IsValid()))
	{
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Value : Flow->GetArrayField(TEXT("nodes")))
	{
		const TSharedPtr<FJsonObject> Node = Value->AsObject();
		const TSharedPtr<FJsonObject> Properties = Node->GetObjectField(TEXT("properties"));
		if (Node->GetStringField(TEXT("guid")) == Guid)
		{
			const TSharedPtr<FJsonObject> Task = Properties->GetObjectField(TEXT("task"));
			TestEqual(TEXT("the task shows by its own class"), Task->GetStringField(TEXT("class")), FString(TEXT("SimFlowTask_Delay")));
			TestEqual(TEXT("with its settings"), Task->GetObjectField(TEXT("properties"))->GetNumberField(TEXT("duration")), 4.0);
		}
		else if (Node->GetStringField(TEXT("class")) == TEXT("SimFlowNode_Branch"))
		{
			const TSharedPtr<FJsonObject> Case = Properties->GetArrayField(TEXT("cases"))[1]->AsObject();
			TestEqual(TEXT("the condition inside the case shows by its class"),
				Case->GetObjectField(TEXT("condition"))->GetStringField(TEXT("class")), FString(TEXT("SimFlowCondition_Constant")));
		}
	}

	// Finding and describing types.
	UClass* TaskBase = USimFlowTask::StaticClass();
	TestTrue(TEXT("a type is found by class name"), USimFlowEditorLibrary::FindClass(TaskBase, TEXT("SimFlowTask_Delay")) == USimFlowTask_Delay::StaticClass());
	TestTrue(TEXT("and without the prefix"), USimFlowEditorLibrary::FindClass(TaskBase, TEXT("Task_Delay")) == USimFlowTask_Delay::StaticClass());
	TestTrue(TEXT("and by path"), USimFlowEditorLibrary::FindClass(TaskBase, USimFlowTask_Delay::StaticClass()->GetPathName()) == USimFlowTask_Delay::StaticClass());
	TestNull(TEXT("an unknown name finds nothing"), USimFlowEditorLibrary::FindClass(TaskBase, TEXT("Task_NoSuchTask")));
	TestNull(TEXT("a class outside the base finds nothing"), USimFlowEditorLibrary::FindClass(TaskBase, TEXT("SimFlowNode_Task")));

	TArray<TSharedPtr<FJsonValue>> Listed;
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(USimFlowEditorLibrary::ListClasses(TaskBase)), Listed);
	bool bDelayListed = false;
	bool bBaseAbstract = false;
	for (const TSharedPtr<FJsonValue>& Value : Listed)
	{
		const FString Name = Value->AsObject()->GetStringField(TEXT("name"));
		bDelayListed |= Name == TEXT("SimFlowTask_Delay");
		bBaseAbstract |= Name == TEXT("SimFlowTask") && Value->AsObject()->GetBoolField(TEXT("abstract"));
	}
	TestTrue(TEXT("ListClasses lists Delay"), bDelayListed);
	TestTrue(TEXT("and marks the base abstract"), bBaseAbstract);

	TSharedPtr<FJsonObject> Described;
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(USimFlowEditorLibrary::DescribeClass(USimFlowNode_Task::StaticClass())), Described);
	if (TestTrue(TEXT("DescribeClass parses"), Described.IsValid()))
	{
		bool bAbortResultHasOptions = false;
		bool bTimeLimitHasDefault = false;
		for (const TSharedPtr<FJsonValue>& Value : Described->GetArrayField(TEXT("properties")))
		{
			const TSharedPtr<FJsonObject> Property = Value->AsObject();
			const FString Name = Property->GetStringField(TEXT("name"));
			bAbortResultHasOptions |= Name == TEXT("AbortResult") && Property->HasField(TEXT("options"));
			bTimeLimitHasDefault |= Name == TEXT("TimeLimit") && Property->HasField(TEXT("default"));
		}
		TestTrue(TEXT("an enum property lists its options"), bAbortResultHasOptions);
		TestTrue(TEXT("a number property carries its default"), bTimeLimitHasDefault);
	}

	// Sending an event by name needs a registered tag.
	USimFlowComponent* Component = NewObject<USimFlowComponent>();
	TestFalse(TEXT("an unregistered tag is refused"), USimFlowEditorLibrary::SendEventByName(Component, TEXT("Not.A.Registered.Tag")));
	TestFalse(TEXT("no component is refused"), USimFlowEditorLibrary::SendEventByName(nullptr, TEXT("Not.A.Registered.Tag")));

	return true;
}

#endif
