#include "STSmokeObjects.h"

void USTSmokeRoot::Describe()
{
	ST_LOG_CONTEXT(Ctx, Frames, Leaf);
	ST_LOG(LogTemp, Display, "describe frames={Frames}", Ctx);
}

void USTSmokeRoot::StaticDescribe()
{
	ST_LOG(LogTemp, Display, "static");
}

void STSmokeFreeFunction()
{
	ST_LOG(LogTemp, Display, "free");
}
