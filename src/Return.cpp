#include "Return.h"
#include "Context.h"
#include "Expression.h"

Return::Return() :
	expression(NULL) {
}

Return::Return(Expression *expression) :
	expression(expression) {
}

Return::~Return() {
	if (expression)
		delete expression;
}

Json::Value Return::json() {
	Json::Value root;
	root["name"] = "return";
	if (expression)
		root["value"] = expression->json();
	return root;
}

void Return::gen(Context &context) {
	if (expression)
		context.getBuilder().CreateRet(expression->load(context));
	else
		context.getBuilder().CreateRetVoid();
	context.newBlock();
}
