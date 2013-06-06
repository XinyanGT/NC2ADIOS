#ifndef GROUP_H
#define GROUP_H

#include <map>
#include <vector>
#include <cstdio>
#include "rose.h"
#include "roseHelper.h"
#include "func.h"
#include "utils.h"

class VarSec;

class Group
{

public:
	/**********************************************
	 * Constructor
	 **********************************************/
	Group(std::vector<SgFunctionCallExp*>, 
			const std::map<std::string, FUNC> &,
			int id);

	void
	InsertAdiosInit();

	void
	Process_nc_create_par();

	void
	Process_nc_def_dim();

	void
	Process_nc_def_var();

	void 
	Process_nc_enddef();

	void
	Extract_nc_def_var();

	void
	Extract_nc_put_vara_int();


	// void
	// Process_nc_enddef();

private:
	enum IDTYPE {
		SINGLE,
		ARRAY_FIRST,
		ARRAY_OTHER
	};

	std::string Name;
	std::string FileName;
	std::string GroupIDVar;
	std::string FileVar;
	int Cmode;
	int GroupID;
	SgExpression *CommExp;
	bool IsPara;
	const std::map<std::string, FUNC> *FuncNameIndMap;
	std::map<SgInitializedName*, std::vector<std::string> > DimMap;
	std::map<SgInitializedName*, VarSec> VarMap;
	std::vector< std::vector<SgFunctionCallExp*> > CallVV;
	std::vector<SgInitializedName*> PutVarVec;


	void 
	FillIsPara();

	SgExprStatement *
	BuildAdInit();

	SgExprStatement *
	BuildAdAllocBuf();

	SgExprStatement *
	BuildAdDeclGroup();

	SgExprStatement *
	BuildAdSelMod();

	SgExprStatement *
	BuildAdOpen();


	SgExprStatement *
	BuildGroupSizeAssign(const std::string &groupVarName);

	SgExprStatement *
	BuildAdGroupSize(const std::string &groupSizeVarName, 
						const std::string &totalSizeVarName);


	SgExprStatement *
	BuildAdDefVar(const std::string &varName, const std::string &typeName,
		const std::string &count = std::string(), const std::string &global = std::string(), 
		const std::string &offset = std::string());


	
	/*************************************************
	 * Distribute calls of supported NetCDF functions 
	 * of one group to their respective vectors
	 * (data member CallVV)
	 * Input:
	 *		vector <SgFunctionCallExp *vec:
	 *			supported NetCDF functions calls vector
	 * Output:
	 *		vector< vector<SgFunctionCallExp*> > CallVV
	 **************************************************/
	void 
	InitCallVV(std::vector<SgFunctionCallExp*>);

	/*********************************************
	  * Extract info from nc_create function calls
	  * There should only one nc_create call
	  * Input:
	  *		vector<SgFunctionCallExp*> vec:
	  *			nc_create calls vector
	  * Output:
	  *		string Name: group name
	  *		string FileName: output file name
	  *		int Cmode: nc create mode
	  *******************************************/
	void
	Extract_nc_create(std::vector<SgFunctionCallExp*>);



	/**********************************************
	 * Extract info from nc_def_dim function calls
	 * Input:
	 *		vector<SgFunctionCallExp*> vec:
	 *			nc_def_dim call vector
	 * Output:
	 *		map<SgInitializedName*, string> DimMap:
	 *			dim info 
	 **********************************************/
	void
	Extract_nc_def_dim(std::vector<SgFunctionCallExp*>);

	void 
	ProcessOne_nc_def_dim(SgFunctionCallExp *callExp);

	void 
	ProcessOne_nc_def_var(SgFunctionCallExp *callExp);

	void 
	ExtractOne_nc_def_var(SgFunctionCallExp *callExp);


	/************************************************
	 * Get adios group name and output file name
	 * based on NetCDF output file name
	 * Input:
	 *		const string &path: NetCDF output file name
	 * Output:
	 *		string Name: adios group name
	 *		string FileName: adios output file name
	 ***************************************************/
	void
	GetNames(const std::string &path);

};
	

class VarSec
{
public:
	VarSec(SgInitializedName *initName, std::vector<std::string> vec, 
		std::string name, std::string typeStr, int unitSize):
			InitName(initName), CountInit(NULL), OffsetInit(NULL),
			StrVec(vec), Name(name), TypeStr(typeStr), UnitSize(unitSize), 
			IterNum(0), IsGlobal(false)  {}

	SgInitializedName *InitName;		// for dimids
	SgInitializedName *CountInit;
	SgInitializedName *OffsetInit;
	std::vector<std::string> StrVec;
	std::string Name;
	std::string TypeStr;
	int UnitSize;
	int IterNum;
	bool IsGlobal;


};



#endif	
