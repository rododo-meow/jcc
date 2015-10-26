#ifndef _TYPE_H_
#define _TYPE_H_

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <vector>

#include "ASTNode.h"

class ArrayDefinator;
class Identifier;
class Class;
class Context;
struct Type : public ASTNode {
	enum BaseType {
		BYTE,
		SHORT,
		INT,
		CHAR,
		FLOAT,
		DOUBLE,
		ARRAY,
		BOOL,
		STRING,
		OBJECT
	} baseType;
	static const char *BaseTypeNames[];
	static Type Bool;
	static Type String;
	static Type Int32;
	static Type UInt32;
	bool isUnsigned;
	std::vector<std::pair<int, int> > arrayDim;
	Type *internal;
	Class *cls;
	Identifier *identifier;
	Type(BaseType baseType, bool isUnsigned = false);
	Type(BaseType array, Type *baseType, ArrayDefinator *definator);
	Type(Class *cls);
	Type(Identifier *identifier);
	~Type();
	Json::Value json() override;
	llvm::Type *getType(Context &context);
	size_t getSize();
	bool isInt() { return baseType == BYTE || baseType == SHORT || baseType == INT || baseType == BOOL; }
	bool isBool() { return baseType == BOOL; }
	bool isFloat() { return baseType == FLOAT || baseType == DOUBLE; }
	bool isNumber() { return isInt() || isFloat(); }
	bool isChar() { return baseType == CHAR; }
	bool isArray() { return baseType == ARRAY; }
	bool isString() { return baseType == STRING; }
	bool isObject() { return baseType == OBJECT; }
	Class* getClass() { return cls; }
	static Type* higherType(Type *a, Type *b);
	const char *getName() { return BaseTypeNames[baseType]; }
	static llvm::Value* cast(Context &context, Type *otype, llvm::Value *val, Type *dtype);
};

#endif
