#ifndef UTILS_H
#define UTILS_H

#include <map>
#include <vector>
#include "rose.h"
#include "roseHelper.h"
#include "func.h"


/***************************************
 * Get the declaration for an enum type
 ***************************************/
SgEnumDeclaration *
GetEnumDecl(const std::string &name);


/****************************************
 * Get the int value of an enum constatnt
 ****************************************/
int
GetEnumIntVal(SgEnumDeclaration *decl, const std::string &name);


/*****************************************
 * Get the expression of an enum constant
 ******************************************/
SgExpression *
GetEnumExpr(const std::string &enumType, const std::string &enumConstant);


/*************************************
 * Return the type define declaration
 * for MPI_Comm
 ************************************/
SgTypedefDeclaration *
Get_MPI_Comm_Declaration(SgNode *node = NULL);


/******************************************
 * Insert MPI related code 
 *****************************************/
void 
InsertMPI(SgProject *project);

/************************************************
 * Fill in a function name to FUNC enum map 
 ***********************************************/
void 
InitFuncNameIndMap(std::map<std::string, FUNC> &nameIndMap);



/****************************************
 * Decide whether the func name has nc 
 * functions' prefix "nc_" or not
 ****************************************/
inline bool 
HasNcPrefix(const std::string &funcName);



/****************************************
 * Only leave supported NetCDF calls
 ****************************************/
void
FilterCall(Rose_STL_Container<SgNode *>&callList,
			std::vector<SgFunctionCallExp *> &ncCall, 
			const std::map<std::string, FUNC> &nameIndMap);


/************************************
 * Group nc functions related to one 
 * ncid together
 ************************************/
void 
GroupNcCall(std::vector<SgFunctionCallExp*> &callVec, 
			std::vector< std::vector<SgFunctionCallExp*> > &callGroupVec);


/******************************************
 * Group NetCDF functions based on a ncid 
 * declaration to index map
 ******************************************/
void
GroupNcCallNow(std::vector<SgFunctionCallExp*> &callVec, 
			std::vector< std::vector<SgFunctionCallExp*> > &callGroupVec,
			std::map<SgInitializedName*, int>ncidMap);


/********************************************
 * Fill a mapping between SgInitializedName*
 * for ncid and index 
 ********************************************/
void 
InitNcidMap(const std::vector<SgFunctionCallExp*> &callVec,
				std::map<SgInitializedName*, int> &ncidMap);


/*************************************
 * Print function call info
 ************************************/
void
PrintFuncCallInfo(SgNode *node);


/****************************************
 * Print function declaration info
 ****************************************/
void
PrintFuncDeclInfo(SgNode *node, const SgStringList &srcNameList);

SgEnumVal* 
BuildEnumVal(int value, SgEnumDeclaration* decl, SgName name);


SgForStatement *
BuildCanonicalForStmt(int start, int end, 
			const std::string &indexName = std::string());

SgForStatement *
BuildCanonicalForStmtDeclIn(int start, int end);

SgForStatement *
BuildCanonicalForStmtDeclOut(int start, int end, std::string indexName);

void
ExtractForStmtBounds(SgForStatement *forStmt, int &low, int &high);

std::string
MakeStr(std::vector<std::string> strVec, std::string delimiter);


#endif
