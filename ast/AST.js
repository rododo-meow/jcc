var AST;
(function () {
	function CompileUnit(data, parent) {
		this.parent = parent;
		this.name = data.name;
		this.body = AST(data.body, this);
		this.getChildren = function() {
			return [ this.body ];
		}
		this.getTips = function() {
			return null;
		}
		return this;
	};
	function Op2(data, parent) {
		this.parent = parent;
		this.name = data.op;
		var children = [ AST(data.left, this), AST(data.right, this) ];
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
		return this;
	}
	function Literal(data, parent) {
		this.parent = parent;
		this.name = "literal";
		this.type = data.type;
		this.literal = AST(data.literal, this);
		this.getChildren = function() {
			return [ this.literal ];
		}
		this.getTips = function() {
			return null;
		}
	}
	function LiteralInt(data, parent) {
		this.parent = parent;
		this.name = "literal_int";
		this.value = data.val;
		this.getChildren = function() {
			return null;
		}
		this.getTips = function() {
			return { "Value": this.value };
		}
	}
	function LiteralString(data, parent) {
		this.parent = parent;
		this.name = "literal_string";
		this.text = data.text;
		this.getChildren = function() {
			return null;
		}
		this.getTips = function() {
			return { "Text": this.text }
		}
	}
	function Identifier(data, parent) {
		this.parent = parent;
		this.name = "identifier";
		this.text = data.text;
		this.getChildren = function() {
			return null;
		}
		this.getTips = function() {
			return { "Name": this.text }
		}
	}
	function Statements(data, parent) {
		this.parent = parent;
		this.name = "statements";
		var children = data.statements;
		for (i in children)
			children[i] = AST(children[i], this);
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function IfStatement(data, parent) {
		this.parent = parent;
		this.name = "if";
		var children = [ AST(data.test, this), AST(data.then, this) ];
		if (data["else"])
			children.push(AST(data["else"], this));
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function VariableDefination(data, parent) {
		this.parent = parent;
		this.name = "variable defination";
		var children = [ AST(data.type, this) ];
		for (i in data.list)
			children.push(AST({
				name: "variable_defination_entry",
				identifier: data.list[i]["identifier"],
				init_value: data.list[i]["init_value"]
			}, this));
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function VariableDefinationEntry(data, parent) {
		this.parent = parent;
		this.name = "entry";
		var children = [ AST(data.identifier, this) ];
		if (data.init_value)
			children.push(AST(data.init_value, this));
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function WhileStatement(data, parent) {
		this.parent = parent;
		this.name = "while";
		var children = [ AST(data.test, this), AST(data.body, this) ];
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function Type(data, parent) {
		this.parent = parent;
		this.name = "type";
		this.getChildren = function() {
			return null;
		}
		this.getTips = function() {
			return {
				"Type": data.base_type,
				"Is unsigned": data.is_unsigned
			}
		}
	}
	function Function(data, parent) {
		this.parent = parent;
		this.name = "function";
		this.visibility = data.visibility;
		var children = [ AST(data.identifier, this), AST(data.arg_list, this), AST(data.body, this) ];
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return {
				"Visibility": this.visibility
			}
		}
	}
	function ArgumentList(data, parent) {
		this.parent = parent;
		this.name = "arg_list";
		var children = [];
		for (i in data.list)
			children.push(AST({
				name: "arg_list_entry",
				type: data.list[i].type,
				identifier: data.list[i].identifier
			}, this));
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function ArgumentListEntry(data, parent) {
		this.parent = parent;
		this.name = "entry";
		var children = [ AST(data.type, this), AST(data.identifier, this) ];
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function Block(data, parent) {
		this.parent = parent;
		this.name = "block";
		var children = [ AST(data.body, this) ];
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function Call(data, parent) {
		this.parent = parent;
		this.name = "call";
		var children = [ AST(data.identifier, this), AST(data.arg_list, this) ];
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function CallArgumentList(data, parent) {
		this.parent = parent;
		this.name = "call_arg_list";
		var children = [];
		for (i in data.list)
			children.push(AST(data.list[i], this));
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function Return(data, parent) {
		this.parent = parent;
		this.name = "return";
		var children = null;
		if (data.value)
			children = [ AST(data.value, this) ];
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function ArrayAccess(data, parent) {
		this.parent = parent;
		this.name = "array";
		var children = [ AST(data.array, this), AST(data.accessor, this) ];
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function ArrayAccessor(data, parent) {
		this.parent = parent;
		this.name = "accessor";
		var children = [];
		for (i in data.list)
			children.push(AST(data.list[i], this));
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function Repeat(data, parent) {
		this.parent = parent;
		this.name = "repeat";
		var children = [ AST(data.body, this), AST(data.until, this) ];
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
	}
	function Op1(data, parent) {
		this.parent = parent;
		this.name = data.op;
		var children = [ AST(data.operand, this) ];
		this.getChildren = function() {
			return children;
		}
		this.getTips = function() {
			return null;
		}
		return this;
	}
	var ASTtypes = {
		"compile_unit": CompileUnit,
		"op2": Op2,
		"literal": Literal,
		"literal_int": LiteralInt,
		"literal_string": LiteralString,
		"identifier": Identifier,
		"statements": Statements,
		"if_statement": IfStatement,
		"variable_defination": VariableDefination,
		"variable_defination_entry": VariableDefinationEntry,
		"while_statement": WhileStatement,
		"type": Type,
		"function": Function,
		"arg_list": ArgumentList,
		"arg_list_entry": ArgumentListEntry,
		"block": Block,
		"call": Call,
		"call_arg_list": CallArgumentList,
		"return": Return,
		"array_access": ArrayAccess,
		"array_accessor": ArrayAccessor,
		"repeat_statement": Repeat,
		"op1": Op1
	};
	AST = function(data, parent) {
		if (!parent)
			return new ASTtypes[data.name](data, null);
		else
			return new ASTtypes[data.name](data, parent);
	}
})();