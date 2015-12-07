#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>

#include "ArrayAccess.h"
#include "Context.h"
#include "exception.h"
#include "ArrayAccessor.h"
#include "Type.h"
#include "DebugInfo.h"

ArrayAccess::ArrayAccess(Expression *array, ArrayAccessor *accessor) :
	array(array), accessor(accessor) {
}

Json::Value ArrayAccess::json() {
	Json::Value root;
	root["name"] = "array_access";
	root["array"] = array->json();
	root["accessor"] = accessor->json();
	return root;
}

ArrayAccess::~ArrayAccess() {
	if (array)
		delete array;
	if (accessor)
		delete accessor;
}

llvm::Value* ArrayAccess::getPointer(Context &context) {
	const std::vector<std::pair<Expression*, Expression*> > &dim = array->getType(context)->arrayDim;
	llvm::Value *offset = NULL;
	for (size_t i = 0; i < dim.size(); i++) {
		if (!dim[i].second) {
			// TODO: Dynamic array
			throw NotImplemented("Dynamic array");
		} else if (!dim[i].first->isConstant() || !dim[i].second->isConstant()) {
			throw NotImplemented("dynamic dim expression");
		} else if (!dim[i].first->getTypeConstant()->isInt() || !dim[i].first->getTypeConstant()->isInt()) {
			throw InvalidType("dim expression must be integer");
		} else {
			if (offset == NULL)
				offset = addDebugLoc(
						context,
						llvm::BinaryOperator::CreateSub(
								accessor->list[i]->load(context),
								context.getBuilder().getInt32(dim[i].first->loadConstant()._int64), "", context.currentBlock()),
						loc);
			else {
				offset = addDebugLoc(
						context,
						context.getBuilder().CreateMul(
								offset,
								llvm::ConstantInt::get(context.getBuilder().getInt32Ty(), dim[i].second->loadConstant()._int64 - dim[i].first->loadConstant()._int64 + 1, false)),
						loc);
				offset = addDebugLoc(
						context,
						context.getBuilder().CreateAdd(
								offset,
								addDebugLoc(
										context,
										llvm::BinaryOperator::CreateSub(
												accessor->list[i]->load(context),
												context.getBuilder().getInt32(dim[i].first->loadConstant()._int64),
												"",
												context.currentBlock()),
										loc)),
						loc);
			}
		}
	}
	llvm::Value *index[2];
	index[0] = context.getBuilder().getInt32(0);
	index[1] = offset;
	llvm::Value *array_ptr = array->load(context);
	return addDebugLoc(
			context,
			llvm::GetElementPtrInst::Create(
					nullptr,
					array_ptr,
					llvm::ArrayRef<llvm::Value*>(index, 2),
					"",
					context.currentBlock()),
			loc);
}

llvm::Value* ArrayAccess::load(Context &context) {
	return addDebugLoc(
			context,
			context.getBuilder().CreateLoad(
					getPointer(context)),
			loc);
}

llvm::Instruction* ArrayAccess::store(Context &context, llvm::Value *value) {
	return context.getBuilder().CreateStore(
			value,
			getPointer(context));
}

Type* ArrayAccess::getType(Context &context) {
	return array->getType(context)->internal;
}

bool ArrayAccess::isConstant() {
	throw NotImplemented("const array");
}

Expression::Constant ArrayAccess::loadConstant() {
	throw NotImplemented("const array");
}

Type* ArrayAccess::getTypeConstant() {
	throw NotImplemented("const array");
}
