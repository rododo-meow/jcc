#include <llvm/IR/Function.h>

#include "Function.h"
#include "Identifier.h"
#include "Type.h"
#include "Context.h"
#include "Symbol.h"
#include "Class.h"
#include "DebugInfo.h"
#include "Qualifier.h"
#include "util.h"
#include "exception.h"

Function::Function(Qualifier *qualifier, Type *return_type, Identifier *identifier, std::list<std::pair<Type*, Identifier*> > *arg_list, ASTNode *body) :
		qualifier(qualifier), return_type(return_type), identifier(identifier), arg_list(*arg_list), body(body), llvmFunction(NULL) {
}

Function::~Function() {
	if (return_type)
		delete return_type;
	delete qualifier;
	delete identifier;
	for (std::list<std::pair<Type*, Identifier*> >::iterator it = arg_list.begin(); it != arg_list.end(); it++) {
		delete it->first;
		if (it->second)
			delete it->second;
	}
	delete &arg_list;
	if (body)
		delete body;
}

Json::Value Function::json() {
	Json::Value root;
	root["name"] = "function";
	root["qualifier"] = qualifier->json();
	root["identifier"] = identifier->json();
	root["arg_list"] = Json::Value(Json::arrayValue);
	int i = 0;
	for (arg_iterator it = arg_list.begin(); it != arg_list.end(); i++, it++) {
		root["arg_list"][i] = Json::Value();
		root["arg_list"][i]["type"] = it->first->json();
		root["arg_list"][i]["identifier"] = it->second->json();
	}
	if (body)
		root["body"] = body->json();
	return root;
}

void Function::gen(Context &context) {
	if (body == NULL && !isNative())
		throw CompileException("empty non-static function defination");
	std::vector<llvm::Type*> arg_type;
	if (!isStatic())
		arg_type.push_back(context.getBuilder().getInt8PtrTy(0));
	for (arg_iterator it = arg_list.begin(); it != arg_list.end(); it++)
		arg_type.push_back(it->first->getType(context));
	llvmFunction = context.createFunction(
			getMangleName(context),
			llvm::FunctionType::get(
					return_type ? return_type->getType(context) : context.getBuilder().getVoidTy(),
					llvm::ArrayRef<llvm::Type*>(arg_type),
					false),
			this);
	if (!isStatic())
		cls->addFunction(getSignature(context), llvmFunction);
	*const_cast<llvm::DISubprogram**>(&context.DIfunction) = getDIFunction(context, this, loc);
	context.pushContext();
	context.pushDIScope(context.DIfunction);
	arg_iterator it = arg_list.begin();
	llvm::Function::arg_iterator it2 = llvmFunction->arg_begin();
	if (!isStatic()) {
		llvm::Value *thisval = context.getBuilder().CreatePointerCast(&(*it2), llvm::PointerType::get(cls->getLLVMType(), 0), "this");
		insertDeclareDebugInfo(context, cls->getType(), "this", thisval, loc, true);
		context.addSymbol(new Symbol("this", Symbol::ARGUMENT, cls->getType(), thisval));
		it2++;
	}
	for (; it != arg_list.end(); it++, it2++) {
		assert(it2 != llvmFunction->arg_end());
		insertDeclareDebugInfo(context, it->first, it->second->getName(), &(*it2), it->second->loc, true);
		context.addSymbol(new Symbol(it->second->getName(), Symbol::ARGUMENT, it->first, &(*it2)));
		it2->setName(it->second->getName());
	}
	if (!isNative())
		body->gen(context);
	context.popDIScope();
	context.popContext();
	*const_cast<llvm::DISubprogram**>(&context.DIfunction) = NULL;
	context.endFunction();
}

llvm::Function* Function::getLLVMFunction(Context &context) {
	if (!llvmFunction && !body && !isNative()) {
		std::vector<llvm::Type*> arg_type;
		if (!isStatic())
			arg_type.push_back(context.getBuilder().getInt8PtrTy(0));
		for (arg_iterator it = arg_list.begin(); it != arg_list.end(); it++)
			arg_type.push_back(it->first->getType(context));
		llvmFunction = context.createFunction(
				getMangleName(context),
				llvm::FunctionType::get(
						return_type ? return_type->getType(context) : context.getBuilder().getVoidTy(),
						llvm::ArrayRef<llvm::Type*>(arg_type),
						false),
				this);
		return llvmFunction;
	} else if (!llvmFunction)
		gen(context);
	return llvmFunction;
}

const std::string& Function::getName() {
	return identifier->getName();
}

const std::string Function::getMangleName(Context &context) {
	std::string tmp = cls->getMangleName() + "F" + itos(identifier->getName().length()) + identifier->getName() + itos(arg_list.size());
	for (arg_iterator it = arg_list.begin(); it != arg_list.end(); it++)
		tmp += it->first->getMangleName(context);
	return tmp;
}

const std::string Function::getSignature(Context &context) {
	std::string tmp = "F" + itos(identifier->getName().length()) + identifier->getName() + itos(arg_list.size());
	for (arg_iterator it = arg_list.begin(); it != arg_list.end(); it++)
		tmp += it->first->getMangleName(context);
	return tmp;
}

llvm::FunctionType* Function::getLLVMType(Context &context) {
	std::vector<llvm::Type*> arg_type;
	if (!isStatic())
		arg_type.push_back(context.getBuilder().getInt8PtrTy(0));
	for (arg_iterator it = arg_list.begin(); it != arg_list.end(); it++)
		arg_type.push_back(it->first->getType(context));
	return llvm::FunctionType::get(
			return_type ? return_type->getType(context) : context.getBuilder().getVoidTy(),
			llvm::ArrayRef<llvm::Type*>(arg_type),
			false);
}

void Function::genStruct(Context &context) {
	if (!qualifier->isStatic())
		cls->addFunctionStruct(getSignature(context), new Symbol(getName(), qualifier, this, getLLVMType(context), 0, 0));
	else
		cls->addFunctionStruct(getSignature(context), new Symbol(getName(), qualifier, this));
}

const std::string Function::genName(const std::string &name, const std::vector<Type*> &arg_type) {
	if (arg_type.size() == 0)
		return name + "()";
	std::string ans = name + "(";
	for (std::vector<Type*>::const_iterator it = arg_type.begin(); it != arg_type.end(); it++)
		ans = ans + (*it)->getName() + ", ";
	return ans.substr(0, ans.size() - 2) + ")";
}
