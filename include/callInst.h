#ifndef CALLINST_H
#define CALLINST_H

#include "rose.h"

class CallInst
{
public:
//	string funcName;
	Sg_File_Info *fileInfo;
	SgFunctionCallExp *callExp;
	SgExpressionPtrList paraList;

	CallInst(Sg_File_Info *fInfo, SgFunctionCallExp *cExp, SgExpressionPtrList pList)
		:fileInfo(fInfo), callExp(cExp), paraList(pList) {}
		

};


#endif
