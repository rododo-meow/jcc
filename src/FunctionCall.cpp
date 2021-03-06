#include "FunctionCall.h"
#include "exception.h"
#include "Context.h"
#include "Identifier.h"
#include "Function.h"
#include "Symbol.h"
#include "Type.h"
#include "Class.h"
#include "DebugInfo.h"
#include <iostream>

FunctionCall::FunctionCall(Expression *target, Identifier *identifier, std::list<Expression*> *arg_list) :
	target(target), identifier(identifier), arg_list(*arg_list) {
}

Expression* FunctionCall::clone() const {
	std::list<Expression*> *arg = new std::list<Expression*>();
	for (std::list<Expression*>::const_iterator it = arg_list.begin(); it != arg_list.end(); it++)
		arg->push_back((*it)->clone());
	if (target)
		return new FunctionCall(target->clone(), identifier->clone(), arg);
	else
		return new FunctionCall(NULL, identifier->clone(), arg);
}

FunctionCall::~FunctionCall() {
	if (target)
		delete target;
	delete identifier;
	delete &arg_list;
}

Json::Value FunctionCall::json() {
	Json::Value root;
	root["name"] = "call";
	if (target)
		root["target"] = target->json();
	root["identifier"] = identifier->json();
	root["arg_list"] = Json::Value(Json::arrayValue);
	int i = 0;
	for (std::list<Expression*>::iterator it = arg_list.begin(); it != arg_list.end(); it++, i++)
		root["arg_list"][i] = (*it)->json();
	return root;
}

llvm::Value* FunctionCall::load(Context &context) {
	std::vector<Type*> actual_type;
	for (std::list<Expression*>::iterator it = arg_list.begin(); it != arg_list.end(); it++)
		actual_type.push_back((*it)->getType(context));

	Symbol *symbol = NULL;
	Class *targetClass = NULL;
	if (target == NULL) {
		// static call or self call
		if (identifier->getName().rfind("::") != std::string::npos) {
			// static call

			// find class
			targetClass = context.findClass(identifier->getName().substr(0, identifier->getName().rfind("::")));
			if (!targetClass)
				throw SymbolNotFound("Class '" + identifier->getName().substr(0, identifier->getName().rfind("::")) + "'");

			// find function
			symbol = targetClass->findSymbol(identifier->getName().substr(identifier->getName().rfind("::") + 2));
			if (!symbol)
				throw SymbolNotFound("Function '" + identifier->getName() + "'");

			// match the best override
			symbol = bestMatch(symbol, actual_type, context);
			if (!symbol)
				throw SymbolNotFound("No matching call for '" + Function::genName(identifier->getName(), actual_type) + "'");

			// check symbol is static function
			if (symbol->type != Symbol::STATIC_FUNCTION)
				throw InvalidType("'" + symbol->data.function.function->getName() + "' is not a static function");

			// check symbol's permission
			if (symbol->data.static_function.isPrivate && targetClass != context.currentClass)
				throw CompileException("function '" + symbol->data.static_function.function->getName() + "' is private");
			if (symbol->data.static_function.isProtected && !Class::isA(context.currentClass, targetClass))
				throw CompileException("function '" + symbol->data.static_function.function->getName() + "' is protected");
		} else {
			// self call
			targetClass = context.currentClass;

			// find function
			symbol = targetClass->findSymbol(identifier->getName());
			if (!symbol)
				throw SymbolNotFound("'" + identifier->getName() + "'");

			symbol = bestMatch(symbol, actual_type, context);
			if (!symbol)
				throw SymbolNotFound("No matching call for '" + Function::genName(identifier->getName(), actual_type) + "'");
		}
	} else {
		// member function call
		targetClass = target->getType(context)->getClass();
		
		// find function
		symbol = targetClass->findSymbol(identifier->getName());
		if (!symbol)
			throw SymbolNotFound("Function '" + identifier->getName() + "'");

		symbol = bestMatch(symbol, actual_type, context);
		if (!symbol)
			throw SymbolNotFound("No matching call for '" + Function::genName(identifier->getName(), actual_type) + "'");

		// check symbol is normal function
		if (symbol->type != Symbol::FUNCTION)
			throw InvalidType("function '" + symbol->data.static_function.function->getName() + "' is not a member function");

		// check symbol's permission
		if (symbol->data.static_function.isPrivate && targetClass != context.currentClass)
			throw CompileException("function '" + symbol->data.static_function.function->getName() + "' is private");
		if (symbol->data.static_function.isProtected && !Class::isA(context.currentClass, targetClass))
			throw CompileException("function '" + symbol->data.static_function.function->getName() + "' is protected");
	}

	llvm::Value *function;
	std::vector<llvm::Value*> arg_code;
	switch (symbol->type) {
	case Symbol::FUNCTION: {
		Expression *tmpTarget = target ? target : new Identifier("this");
		if (!tmpTarget->getType(context)->isObject())
			throw InvalidType(std::string("calling a function of ") + tmpTarget->getType(context)->getName());
		llvm::Value *thisval = tmpTarget->load(context);
		if (!target)
			delete tmpTarget;

		if (targetClass->getMangleName()[0] == 'J') {
			// locate vtable
			function = addDebugLoc(
					context,
					context.getBuilder().CreateLoad(
							thisval
					),
					loc
			);
			// for interface call, we have to recalculate the object base address
			// load offset from vtable
			llvm::Value *baseOffset = addDebugLoc(
					context,
					context.getBuilder().CreateLoad(
							addDebugLoc(
									context,
									context.getBuilder().CreateStructGEP(
											nullptr,
											function,
											0
									),
									loc
							)
					),
					loc
			);
			// calculate the base address
			thisval = addDebugLoc(
					context,
					context.getBuilder().CreateIntToPtr(
							addDebugLoc(
									context,
									context.getBuilder().CreateSub(
											addDebugLoc(
													context,
													context.getBuilder().CreatePtrToInt(
															thisval,
															context.getBuilder().getInt32Ty()
													),
													loc
											),
											baseOffset
									),
									loc
							),
							context.getBuilder().getInt8PtrTy(0)
					),
					loc
			);
		} else {
			// locate vtable
			function = addDebugLoc(
					context,
					context.getBuilder().CreateLoad(
							addDebugLoc(
									context,
									context.getBuilder().CreateStructGEP(
											nullptr,
											thisval,
											symbol->data.function.vtableOffset
									),
									loc
							)
					),
					loc
			);

			// for object call, we only need type cast
			thisval = context.getBuilder().CreatePointerCast(
					thisval,
					context.getBuilder().getInt8PtrTy(0)
			);
		}
		// add thisval as the first argument
		arg_code.push_back(thisval);
		// load function pointer from vtable
		function = addDebugLoc(
				context,
				context.getBuilder().CreateLoad(
						addDebugLoc(
								context,
								context.getBuilder().CreateStructGEP(
										nullptr,
										function,
										symbol->data.function.funcPtrOffset
								),
								loc
						)
				),
				loc
		);
		break; }
	case Symbol::STATIC_FUNCTION:
		function = symbol->data.static_function.function->getLLVMFunction(context);
		if (target)
			throw CompileException("Calling a static function of an object");
		break;
	default:
		throw InvalidType("calling a symbol which is not a function");
	}

	// load remaining arguments
	{
		Function *func = symbol->type == Symbol::FUNCTION ? symbol->data.function.function : symbol->data.static_function.function;
		Function::arg_iterator it2 = func->arg_begin();
		for (std::list<Expression*>::iterator it = arg_list.begin(); it != arg_list.end(); it++, it2++)
			arg_code.push_back(Type::cast(context, (*it)->getType(context), (*it)->load(context), it2->first));
	}

	// do the call
	llvm::Value *ans = addDebugLoc(
			context,
			context.getBuilder().CreateCall(function, llvm::ArrayRef<llvm::Value*>(arg_code)),
			loc);
	return ans;
}

llvm::Instruction* FunctionCall::store(Context &context, llvm::Value *value) {
	throw NotAssignable("function call");
}

Type* FunctionCall::getType(Context &context) {
	Symbol *symbol = NULL;
	if (target)
		symbol = target->getType(context)->getClass()->findSymbol(identifier->getName());
	else if (identifier->getName().rfind("::") != std::string::npos) {
		Class *targetClass = context.findClass(identifier->getName().substr(0, identifier->getName().rfind("::")));
		if (!targetClass)
			throw SymbolNotFound(identifier->getName().substr(0, identifier->getName().rfind("::")));
		symbol = targetClass->findSymbol(identifier->getName().substr(identifier->getName().rfind("::") + 2));
	} else
		symbol = context.currentClass->findSymbol(identifier->getName());
	if (!symbol)
		throw SymbolNotFound(identifier->getName());
	switch (symbol->type) {
	case Symbol::STATIC_FUNCTION:
		return symbol->data.static_function.function->getReturnType();
	case Symbol::FUNCTION:
		return symbol->data.function.function->getReturnType();
	default:
		throw InvalidType("calling a symbol which is not a function");
	}
}

Expression::Constant FunctionCall::loadConstant() {
	throw NotConstant("function call");
}

Type* FunctionCall::getTypeConstant() {
	throw NotConstant("function call");
}

Symbol* FunctionCall::bestMatch(Symbol *symbol, const std::vector<Type*> &actual_type, Context &context) {
	int minDis = 0x7fffffff;
	Symbol *ans = NULL;
	while (symbol) {
		int nowDis = 0;
		std::vector<Type*>::const_iterator it = actual_type.begin();
		Function *func;
		if (symbol->type == Symbol::FUNCTION)
			func = symbol->data.function.function;
		else if (symbol->type == Symbol::STATIC_FUNCTION)
			func = symbol->data.static_function.function;
		else
			throw CompileException("Shouldn't reach here");
		Function::arg_iterator it2 = func->arg_begin();
		while (it != actual_type.end() && it2 != func->arg_end()) {
			int dis = Type::distance(*it, it2->first, context);
			if (dis < 0)
				break;
			nowDis += dis;
			it++, it2++;
		}
		if (it == actual_type.end() && it2 == func->arg_end() && nowDis < minDis) {
			minDis = nowDis;
			ans = symbol;
		}
		if (symbol->type == Symbol::FUNCTION)
			symbol = symbol->data.function.next;
		else if (symbol->type == Symbol::STATIC_FUNCTION)
			symbol = symbol->data.static_function.next;
	}
	return ans;
}
