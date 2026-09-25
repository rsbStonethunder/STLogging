#include "STSmokeCommandlet.h"
#include "STSmokeObjects.h"

int32 USTSmokeCommandlet::Main(const FString& Params)
{
	USTSmokeRoot* Root = NewObject<USTSmokeRoot>(GetTransientPackage());
	Root->AddToRoot();
	Root->Frames = 3;

	Root->Describe(); // Leaf is null

	Root->Leaf = NewObject<USTSmokeLeaf>(GetTransientPackage());
	Root->Leaf->Count = 7;
	Root->Leaf->Label = TEXT("L");
	Root->Describe();

	Root->Leaf->Count = 8;
	ST_LOG_CONTEXT(Ctx, Root);
	ST_LOG(LogTemp, Display, "main done", Ctx);

	USTSmokeRoot::StaticDescribe();
	STSmokeFreeFunction();

	Root->RemoveFromRoot();
	return 0;
}
