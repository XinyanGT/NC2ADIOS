#ifndef FUNC_H
#define FUNC_H

#include <string>
#include <vector>
#include "callInst.h"

class Funk
{
public:
	friend std::ostream& operator<< (std::ostream &, const Funk &);
	std::string GetName() const;
	void SetName(const std::string &);
	

	std::vector<CallInst> callInstVec;
private:
	std::string name;

};

#endif
