#include "STLogTestObjects.h"

DEFINE_LOG_CATEGORY(LogSTLoggingTest);

void USTTestNode::Describe() const
{
	ST_LOG_CONTEXT(Ctx, Id, Next);
	ST_LOG(LogSTLoggingTest, Log, "node {Id}", Ctx);
}

void USTTestNode::DescribeStatic()
{
	ST_LOG(LogSTLoggingTest, Log, "static");
}

void STLogTestFreeFunction()
{
	ST_LOG(LogSTLoggingTest, Log, "free");
}
