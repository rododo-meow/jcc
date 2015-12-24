#include <llvm/IR/DataLayout.h>

#include "New.h"
#include "Type.h"
#include "Context.h"
#include "exception.h"
#include "DebugInfo.h"
#include "Class.h"

New::New(Type *type) : type(type) {
}

New::~New() {
	delete type;
}

Json::Value New::json() {
	Json::Value root;
	root["name"] = "new";
	root["type"] = type->json();
	return root;
}

llvm::Value *New::load(Context &context) {
	llvm::Value *arg[] = { context.getBuilder().getInt64(type->getSize(context)) };
	llvm::Value *newObj = addDebugLoc(
			context,
			context.getBuilder().CreateBitCast(
					addDebugLoc(
							context,
							llvm::CallInst::Create(
									context.mallocFunc,
									llvm::ArrayRef<llvm::Value*>(arg, 1),
									"",
									context.currentBlock()
							),
							loc),
					type->getType(context)),
			loc);
	if (type->isObject()) {
		Class *cls = type->getClass();
		std::vector<llvm::Value*> arg;
		arg.push_back(newObj);
		addDebugLoc(
				context,
				context.getBuilder().CreateCall(cls->getConstructor(), llvm::ArrayRef<llvm::Value*>(arg)),
				loc
		);
	}
	return newObj;
}

llvm::Instruction* New::store(Context &context, llvm::Value *value) {
	throw NotAssignable("object create");
}

Type* New::getType(Context &context) {
	return type;
}

Expression::Constant New::loadConstant() {
	throw NotConstant("object create");
}

Type* New::getTypeConstant() {
	throw NotConstant("object create");
}
