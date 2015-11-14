#ifndef _QUALIFIER_H_
#define _QUALIFIER_H_

#include "StructNode.h"

class Qualifier : public StructNode {
	bool _isConst, _isStatic, _isPrivate, _isPublic, _isProtected;
public:
	Qualifier();
	~Qualifier();
	void setConst();
	void setStatic();
	void setPrivate();
	void setPublic();
	void setProtected();
	inline bool isConst() const { return _isConst; }
	inline bool isStatic() const { return _isStatic; }
	inline bool isPrivate() const { return _isPrivate; }
	inline bool isPublic() const { return _isPublic; }
	inline bool isProtected() const { return _isProtected; }
	Json::Value json() override;
	void writeJsymFile(std::ostream &os) override;
	void genStruct(Context &context) override {};
};

#endif
