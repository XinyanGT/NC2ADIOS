#include "group.h"

using namespace std;
using namespace RoseHelper;
using namespace SageBuilder;
using namespace SageInterface;


/**********************************************
 * Constructor
 **********************************************/
Group::Group(vector<SgFunctionCallExp*> vec, 
		const map<string, FUNC> &nameIndMap, int id) 
	: Name("DefaultGroup"), FileName("DefaultFile"), GroupID(id),
		CommExp(NULL), FuncNameIndMap(&nameIndMap)
{
	cout << "FuncNameIndMap size: " << FuncNameIndMap->size() << endl;

	char groupIDStr[30];
	snprintf(groupIDStr, 30, "%d", GroupID); 
	GroupIDVar = string("adios_group") + groupIDStr;
	FileVar = string("adios_file") + groupIDStr;

	CallVV.resize(FuncNameIndMap->size());
	cout << "Initializing CallVV..." << endl;
	InitCallVV(vec); 
	FillIsPara();

//	cout << setw(80) << setfill('*')<< '*' << endl;
//	cout << "Extracting info from nc_create..." << endl;
//	Extract_nc_create(CallVV[NC_CREATE]);	


//	cout << setw(80) << setfill('*')<< '*' << endl;
//	cout << "Extracting info from nc_def_dim..." << endl;
//	Extract_nc_def_dim(CallVV[NC_DEF_DIM]);	


}



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
Group::InitCallVV(vector<SgFunctionCallExp*> vec)
{
	map<string, FUNC>::const_iterator mapItr;
	string funcName;

	for (vector<SgFunctionCallExp*>::size_type i = 0;
			i != vec.size(); ++i) {
		funcName = GetCallName(vec[i]);
		if ( (mapItr = FuncNameIndMap->find(funcName)) 
				!= FuncNameIndMap->end() ) 
			CallVV[mapItr->second].push_back(vec[i]);
	}
}


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
Group::GetNames(const string &path)
{
	size_t len, posSlash, posLast;

	FileName = path;
	len = FileName.length();
	posLast = len - 1;

	if ( (len > 3) && (FileName.substr(len-3) == ".nc") ) {
		FileName.replace(len-3, 3, ".bp");
		posLast = len - 4;
	}

	if ( (posSlash = FileName.rfind('/')) == string::npos )
		posSlash = 0;
	else
		posSlash++;

	Name = FileName.substr(posSlash, posLast-posSlash+1);
}


/*********************************************
  * Process nc_create_par function calls
  * There should only one nc_create_par call
  * Output:
  *		string Name: group name
  *		string FileName: output file name
  *		int Cmode: nc create par mode
  *******************************************/
void
Group::Process_nc_create_par()
{
	vector<SgFunctionCallExp*> vec = CallVV[NC_CREATE_PAR];
	assert(vec.size() == 1);

	string path, pathVar;

	SgExpression *pathExp, *cmodeExp;

	/***** path *****/
	pathExp = GetCallArgs(vec[0])[0];
	path = ArgCharPtr(pathExp, pathVar);

	if (!path.empty()) {
		cout << "\tFile name before: " << path << endl;
		GetNames(path);
		cout << "\tFile name after: " << FileName << endl;
		cout << "\tGroup name: " << Name << endl;

	} else {
		cout << "\tFile name is in var: " << pathVar << endl;
		cout << "\tCan NOT deal with it yet" << endl;
		cout << "\tSet file name to default value:" << FileName << endl;
	}

	/**** cmode *****/
	cmodeExp = GetCallArgs(vec[0])[1];
	if (cmodeExp->variantT() == V_SgBitOrOp) {
		Cmode = ArgBitOrOp(cmodeExp);
	} else if (cmodeExp->variantT() == V_SgIntVal) {
		Cmode = ArgEnum(cmodeExp);
	} else {
		cout << "ERROR: unsupported cmodeExp class name: "
				<< cmodeExp->class_name() << endl;
		exit(1);
	}
	cout << "\tcmode value: " << Cmode << endl;

	/***** comm *****/
	CommExp = GetCallArgs(vec[0])[2];


	/***** Insert adios calls, some var decls, remove nc call *****/
	InsertAdiosInit();

}

	
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
Group::Extract_nc_create(vector<SgFunctionCallExp*> vec)
{
	assert(vec.size() == 1);

	string path, pathVar;

	SgExpression *pathExp, *cmodeExp;

	/***** path *****/
	pathExp = GetCallArgs(vec[0])[0];
	path = ArgCharPtr(pathExp, pathVar);

	if (!path.empty()) {
		cout << "\tFile name before: " << path << endl;
		GetNames(path);
		cout << "\tFile name after: " << FileName << endl;
		cout << "\tGroup name: " << Name << endl;

	} else {
		cout << "\tFile name is in var: " << pathVar << endl;
		cout << "\tCan NOT deal with it yet" << endl;
		cout << "\tSet file name to default value:" << FileName << endl;
	}

	/**** cmode *****/
	cmodeExp = GetCallArgs(vec[0])[1];
	Cmode = ArgEnum(cmodeExp);
	cout << "\tcmode value: " << Cmode << endl;
}

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
Group::Extract_nc_def_dim(vector<SgFunctionCallExp*> vec)
{

	string name, nameVar, lenVar, idpVar;
	size_t len;
	
	SgExpression *nameExp, *lenExp, *idpExp;

	cout << "nc_def_dim is called " << vec.size() 
			<< " times"  << endl;

	for (vector<SgFunctionCallExp*>::size_type i = 0; i != vec.size(); ++i) {
		cout << "    time " << i << endl;

		/**** name *****/
		nameExp = GetCallArgs(vec[i])[1];
		name = ArgCharPtr(nameExp, nameVar);

		if (!name.empty()) {
			cout << "\tdim name: " << name << endl;
		} else {
			cout << "\tdim name is in var: " << nameVar << endl;
		}

		/***** len *****/
		lenExp = GetCallArgs(vec[i])[2];
		len = ArgSize_t(lenExp, lenVar);

		if (len > 0) {
			cout << "\tlen: " << len << endl;
		} else {
			cout << "\tlen is in var: " << lenVar << endl;
		}

		/***** idp *****/
		idpExp =  GetCallArgs(vec[i])[3];
		idpVar = ArgIntPtr(idpExp);
		cout << "\tidp is in var: " << idpVar << endl;
	}
	
}

void 
Group::ProcessOne_nc_def_dim(SgFunctionCallExp *callExp)
{

	SgStatement *orginStmt = getEnclosingStatement(callExp);
	SgScopeStatement *scope = getScope(orginStmt);
	pushScopeStack(scope);

	SgExpression *nameExp, *lenExp, *idpExp;
	SgInitializedName *idpInitName;
	string name, nameVar, adName;
	IDTYPE idType;
	int idOffset = 0;			// in case of ARRAY_OTHER

	nameExp = GetCallArgs(callExp)[1];
	lenExp = GetCallArgs(callExp)[2];
	idpExp = GetCallArgs(callExp)[3];

	/***** name *****/
	name = ArgCharPtr(nameExp, nameVar);
	if (!name.empty()) {
		cout << "\tdim name: " << name << endl;
		adName = name;
	} else {
		cout << "\tdim name is in var: " << nameVar << endl;
		adName = nameVar;
	}

	/***** idp*****/
	if (idpExp->variantT() == V_SgAddressOfOp) {
		idpInitName = ArgIntPtr_InitName(idpExp);
		idType = SINGLE;

	} else if (idpExp->variantT() == V_SgVarRefExp) {
		idpInitName = ArgVarRef_InitName(idpExp);
		idType = ARRAY_FIRST;
		
	} else if (idpExp->variantT() == V_SgAddOp) {
		idpInitName = ArgIntPtr_AddOp_Left(idpExp);
		idOffset = ArgIntPtr_AddOp_Right(idpExp);
		idType = ARRAY_OTHER;
	} else {
		cout << "ERROR: unsupported idpExp class name: "
				<< idpExp->class_name() << endl;
		exit(1);
	}


	/***** unsigned long long adName = len *****/
	SgVariableDeclaration *adVarDecl = 
		buildVariableDeclaration(adName,
			buildUnsignedLongLongType(),
			buildAssignInitializer(copyExpression(lenExp))
			);


	/***** adios_define_var (GroupIDVar, adName ,"", 
			adios_unsigned_long, "", "", "") *****/
	SgExprStatement *adDefVarCall = 
		BuildAdDefVar(adName, "adios_unsigned_long");


	/***** Insert, remove calls and pop scope *****/
	insertStatementAfter(orginStmt, adVarDecl);
	insertStatementAfter(adVarDecl, adDefVarCall);
	removeStatement(orginStmt);
	popScopeStack();
	
	/***** SINGLE, then append to DimMap *****/
	if (idType == SINGLE) {
		DimMap.insert(make_pair(idpInitName, vector<string>(1, adName))); 

	/***** Related to ARRAY *****/
	} else if (idType == ARRAY_FIRST || idType == ARRAY_OTHER) {
		map<SgInitializedName*, vector<string> >::iterator itr = 
				DimMap.find(idpInitName);
		/***** Have not existed yet, then insert *****/
		if (itr == DimMap.end()) {
			vector<string> strVec;
			strVec.resize(idOffset+1);
			strVec[idOffset] = adName;
			DimMap.insert(make_pair(idpInitName, strVec)); 
		/***** Already existed, then update *****/
		} else {
			if ( (itr->second.size()) < idOffset+1 ) {
				itr->second.resize(idOffset+1);
			}
			(itr->second)[idOffset] = adName;
		}
	}

}


void
Group::Process_nc_def_dim()
{
	vector<SgFunctionCallExp*> vec = CallVV[NC_DEF_DIM];

	for (vector<SgFunctionCallExp*>::size_type i = 0; 
			i != vec.size(); ++i) {
		ProcessOne_nc_def_dim(vec[i]);
	}
	
}

void 
Group::ProcessOne_nc_def_var(SgFunctionCallExp *callExp)
{

	SgStatement *orginStmt = getEnclosingStatement(callExp);
	SgScopeStatement *scope = getScope(orginStmt);
	pushScopeStack(scope);
	SgExprStatement *adDefVarCall;

	/***** varidp *****/
	SgExpression *varidpExp;
	varidpExp = GetCallArgs(callExp)[5];
	SgInitializedName *varidInitName = 
		ArgIntPtr_InitName(varidpExp);

	/***** Get strVec *****/
	map<SgInitializedName*, VarSec>::iterator itr 
		= VarMap.find(varidInitName);
	assert(itr != VarMap.end());
	vector<string> strVec = itr->second.StrVec;

	assert(itr->second.IsGlobal);

	/* adios_define_var() for
	 * Count, Offset var declarations */
	/***** Count var declarations *****/
	SgStatement *prevStmt = orginStmt;
	for (vector<string>::size_type i = 0; 
			i < strVec.size(); ++i) {
		// Build
		adDefVarCall = 
			BuildAdDefVar("c"+strVec[i], "adios_unsigned_integer");
		// Insert
		insertStatementAfter(prevStmt, adDefVarCall);
		prevStmt = adDefVarCall;
	}

	/***** Offset var declarations(Use a For Statement) *****/
	SgForStatement *forStmt =
		BuildCanonicalForStmt(0, itr->second.IterNum);
	SgBasicBlock *forBody = 
		isSgBasicBlock(forStmt->get_loop_body());
	assert(forBody != NULL);
	insertStatementAfter(prevStmt, forStmt);
	pushScopeStack(forBody);

	for (vector<string>::size_type i = 0; 
			i < strVec.size(); ++i) {
		// Build
		adDefVarCall = 
			BuildAdDefVar("o"+strVec[i], "adios_unsigned_integer");
		// Insert
		appendStatement(adDefVarCall);
	}

	popScopeStack();



 	/***** adios_define_var (GroupIDVar, adName, "", 
 		adType, dims, "", "") ****/ 
// 
	adDefVarCall = 
		BuildAdDefVar(itr->second.Name, itr->second.TypeStr, 
			MakeStr(strVec, "c"), MakeStr(strVec, ""),
			MakeStr(strVec, "o")
		);
		

	/***** Insert, remove and pop scope *****/
	insertStatementAfter(forStmt, adDefVarCall);
//	insertStatementAfter(orginStmt, adDefVarCall);
	removeStatement(orginStmt);
	popScopeStack();
	

}

void
Group::Process_nc_def_var()
{
	vector<SgFunctionCallExp*> vec = CallVV[NC_DEF_VAR];

	for (vector<SgFunctionCallExp*>::size_type i = 0; 
			i != vec.size(); ++i) {
		ProcessOne_nc_def_var(vec[i]);
	}
	
}


void 
Group::ExtractOne_nc_def_var(SgFunctionCallExp *callExp)
{

	SgExpression *nameExp, *xtypeExp, 
			*ndimsExp, *dimidspExp, *varidpExp;

	nameExp = GetCallArgs(callExp)[1];
	xtypeExp = GetCallArgs(callExp)[2];
	ndimsExp = GetCallArgs(callExp)[3];
	dimidspExp = GetCallArgs(callExp)[4];
	varidpExp = GetCallArgs(callExp)[5];

	string name, nameVar, adName, adType;

	/***** name *****/
	name = ArgCharPtr(nameExp, nameVar);
	if (!name.empty()) {
		cout << "\tvar name: " << name << endl;
		adName = name;
	} else {
		cout << "\tvar name is in var: " << nameVar << endl;
		adName = nameVar;
	}

	/***** xtype *****/	
	int xtypeInt = ArgCastInt(xtypeExp);
	int unitSize;
	if (xtypeInt == 4) {
		adType = "adios_integer";
		unitSize = 4;	
	} else if (xtypeInt == 5) {
		adType = "adios_real";
		unitSize = 4;
	} else {
		cout << "ERROR: unsupported NetCDF xtype: " 
			<< xtypeInt << " .Quit. " << endl;
		exit(1);
	}

	// string xtypeMacroName;
	// xtypeMacroName = ArgCastMacro(xtypeExp);
	// int unitSize;
	// if (xtypeMacroName == "NC_FLOAT") {
		// adType = "adios_real";
		// unitSize = 4;
	// } else if (xtypeMacroName == "NC_INT") {
		// adType = "adios_integer";
		// unitSize = 4;
	// } else {
		// cout << "ERROR: unsupported NetCDF xtype: " 
			// << xtypeMacroName << " .Quit. " << endl;
		// exit(1);
	// }


	/***** ndims *****/
	int ndims = ArgInt_Val(ndimsExp);


	/***** dimidsp *****/
	vector<string> strVec;
	SgInitializedName *dimidspInitName = 
		ArgIntPtr_InitName(dimidspExp);
	map<SgInitializedName*, vector<string> >::iterator itr 
		= DimMap.find(dimidspInitName);
	assert(itr != DimMap.end());
	strVec = itr->second;
	assert(strVec.size() == ndims);

	/***** varidp *****/
	SgInitializedName *varidInitName = 
		ArgIntPtr_InitName(varidpExp);

	/***** Append to VarMap *****/
	VarMap.insert( make_pair(varidInitName, 
				VarSec(dimidspInitName, strVec, adName, adType, unitSize)) ); 

}

void
Group::Extract_nc_def_var()
{
	vector<SgFunctionCallExp*> vec = CallVV[NC_DEF_VAR];

	for (vector<SgFunctionCallExp*>::size_type i = 0; 
			i != vec.size(); ++i) {
		ExtractOne_nc_def_var(vec[i]);
	}
	
}

/*****************************************
 * Update VarMap: Update IsGlobal, and 
 * fill CountInit, OffsetInit, IterNum
 * if necessary
 * Also, update PutVarVec
 ****************************************/
void
Group::Extract_nc_put_vara_int()
{
	assert(CallVV[NC_PUT_VARA_INT].size() == 1);
	PutVarVec.clear();
	PutVarVec.resize(CallVV[NC_PUT_VARA_INT].size());
	SgFunctionCallExp *callExp = CallVV[NC_PUT_VARA_INT][0];

	SgExpression *varidExp, *startpExp, *countpExp;
	SgInitializedName *varidInitName;

	varidExp = GetCallArgs(callExp)[1];
	startpExp = GetCallArgs(callExp)[2];
	countpExp = GetCallArgs(callExp)[3];

	varidInitName = ArgVarRef_InitName(varidExp);
	map<SgInitializedName*, VarSec>::iterator itr 
		= VarMap.find(varidInitName);
	assert(itr != VarMap.end());
	vector<string>::size_type ndims = itr->second.StrVec.size();


	/***** IsGlobal *****/
	itr->second.IsGlobal = true;


	/***** CountInit and OffsetInt *****/
	SgType *startpType = ArgVarRef_Type(startpExp);
	SgType *countpType = ArgVarRef_Type(countpExp);
	assert(getDimensionCount(startpType) == 1);
	assert(getDimensionCount(countpType) == 1);
	assert(startpType->variantT() == V_SgArrayType);
	assert(countpType->variantT() == V_SgArrayType);
	assert(getArrayElementCount(static_cast<SgArrayType*>(startpType))
		== ndims);
	assert(getArrayElementCount(static_cast<SgArrayType*>(countpType))
		== ndims);
	itr->second.OffsetInit = ArgVarRef_InitName(startpExp);
	itr->second.CountInit = ArgVarRef_InitName(countpExp);


	/***** IterNum *****/
	int low, high;
	SgStatement *orginStmt;
	SgScopeStatement *scopeStmt;

	orginStmt = getEnclosingStatement(callExp);
	// scopeStmt = getScope(orginStmt);
	// cout << "class name of scope of put function is " << 
	// 	scopeStmt->class_name() << endl;;
	scopeStmt = findEnclosingLoop(orginStmt);
	SgForStatement *forStmt = isSgForStatement(scopeStmt);
	assert(forStmt != NULL);

	ExtractForStmtBounds(forStmt, low, high);
	itr->second.IterNum = high - low;
	cout << "Iteration number is: " << itr->second.IterNum << endl;

	/***** Update PutVarVec *****/
	PutVarVec[0] = itr->first;

}


void 
Group::Process_nc_enddef()
{
	SgStatement *orginStmt;
	assert(CallVV[NC_ENDDEF].size() == 1);
	orginStmt = getEnclosingStatement(CallVV[NC_ENDDEF][0]);

	SgScopeStatement *scope = getScope(orginStmt);
	pushScopeStack(scope);

	/***** adios_open(&FileVar, Name, FileName, "w", CommExp) *****/
	SgExprStatement *adOpenCall= BuildAdOpen();


	/***** Var declarations *****/
	char groupIDStr[30];
	snprintf(groupIDStr, 30, "%d", GroupID); 
	string adios_groupsize_str =
		string("adios_groupsize") + groupIDStr;
	string adios_totalsize_str = 
		string("adios_totalsize") + groupIDStr;
	/***** unsigned long long adios_groupsizeXX *****/
	SgVariableDeclaration *adios_groupsize_decl = 
		buildVariableDeclaration(adios_groupsize_str,
			buildUnsignedLongLongType());
	/***** unsigned long long adios_totalsizeXX *****/
	SgVariableDeclaration *adios_totalsize_decl = 
		buildVariableDeclaration(adios_totalsize_str,
			buildUnsignedLongLongType());


	/* adios_groupsizeXX = ndims*8(g) + ndims*4(c) + ndims*4(o)*IterNum
	 * + count * UnitSize(data) * IterNum */
	SgExprStatement *assignGroupSizeStmt = 
		BuildGroupSizeAssign(adios_groupsize_str);


	/* adios_group_size(FileVar, adios_groupsize_str, 
	 *	&adios_totalsize_str
	 */
	 SgExprStatement *adGroupSizeStmt = 
	 	BuildAdGroupSize(adios_groupsize_str, adios_totalsize_str);


//	/***** Insert *****/
	insertStatementAfter(orginStmt, adOpenCall);
	insertStatementAfter(adOpenCall, adios_groupsize_decl);
	insertStatementAfter(adios_groupsize_decl, adios_totalsize_decl);
	insertStatementAfter(adios_totalsize_decl, assignGroupSizeStmt);
	insertStatementAfter(assignGroupSizeStmt, adGroupSizeStmt);




	/***** remove original statement *****/
	removeStatement(orginStmt);

	popScopeStack();

}



// void
// Group::Process_nc_put_vara_int()
// {
// 	assert(CallVV[NC_PUT_VARA_INT].size() == 1);
// 	SgFunctionCallExp *callExp = CallVV[NC_PUT_VARA_INT][0];

// 	SgStatement *orginStmt;
// 	orginStmt = getEnclosingStatement(callExp);
// 	SgScopeStatement *scope = getScope(orginStmt);
// 	pushScopeStack(scope);
	
// 	SgExpression *varidExp, *startpExp, *countpExp, *opExp;
// 	SgInitializedName *varidInitName;

// 	/***** Extract info from arguments *****/
// 	varidExp = GetCallArgs(callExp)[1];
// 	startpExp = GetCallArgs(callExp)[2];
// 	countpExp = GetCallArgs(callExp)[3];
// 	opExp = GetCallArgs(callExp)[4];


// 	varidInitName = ArgVarRef_InitName(varidExp);
// 	map<SgInitializedName*, VarSec>::iterator itr 
// 		= VarMap.find(varidInitName);
// 	assert(itr != VarMap.end());
// 	vector<string> strVec = itr->second.StrVec;


// 	SgType *startpType = ArgVarRef_Type(startpExp);
// 	SgType *countpType = ArgVarRef_Type(countpExp);
// 	assert(getDimensionCount(startpType) == 1);
// 	assert(getDimensionCount(countpType) == 1);
// 	assert(startpType->variantT() == V_SgArrayType);
// 	assert(countpType->variantT() == V_SgArrayType);
// 	assert(getArrayElementCount(static_cast<SgArrayType*>(startpType))
// 		== strVec.size());
// 	assert(getArrayElementCount(static_cast<SgArrayType*>(countpType))
// 		== strVec.size());


// 	/***** For Statement *****/
// 	assert(scope->variantT() = V_SgForStatement);
// 	SgForStatement *forStmt = static_cast<SgForStatement*>(scope);
// 	pushScopeStack(scope);


// 	/***** Declare offset dims *****/
// 	SgVariableDeclaration *adGDecl[strVec.size()];
// 	for (vector<string>::size_type i = 0; i < strVec.size(); i++) {
// 		adGDecl[i] = buildVariableDeclaration("g"+strVec[i],
// 						buildUnsignedIntType()
// 					)
// 	}

// 	/*****




// }



SgExprStatement *
Group::BuildAdInit()
{
	/***** adios_init_noxml(comm) *****/
	SgExpression *argArg1;
	if (CommExp != NULL)
		argArg1 = copyExpression(CommExp);
	else
		argArg1 = buildVarRefExp(SgName("comm"));

	SgExprListExp *argArgList = buildExprListExp();
	appendExpression(argArgList, argArg1);
	
	SgExprStatement *call = 
		buildFunctionCallStmt(SgName("adios_init_noxml"), 
			buildIntType(), argArgList);
	return call;
}

SgExprStatement *
Group::BuildAdAllocBuf()
{
	/***** adios_allocate_buffer(ADIOS_BUFFER_ALLOC_NOW, 10) *****/
	SgExpression *arg1 =  
			GetEnumExpr("ADIOS_BUFFER_ALLOC_WHEN", "ADIOS_BUFFER_ALLOC_NOW");

	SgUnsignedLongLongIntVal *arg2 = 
			buildUnsignedLongLongIntVal(10);
	SgExprListExp *argList = buildExprListExp();
	appendExpression(argList, arg1);
	appendExpression(argList, arg2);
	SgExprStatement *call = 
		buildFunctionCallStmt(SgName("adios_allocate_buffer"),
			buildIntType(), argList);

	return call;
}

SgExprStatement *
Group::BuildAdDeclGroup()
{

	/***** adios_declare_group(&GroupIDVar, Name, 
			"", adios_flag_yes) *****/
	SgAddressOfOp *arg1 = 
		buildAddressOfOp(buildVarRefExp(SgName(GroupIDVar)) );
	SgStringVal *arg2 = buildStringVal(Name);
	SgStringVal *arg3 = buildStringVal("");
	SgExpression *arg4 =  
			GetEnumExpr("ADIOS_FLAG", "adios_flag_yes");

	SgExprListExp *argList = buildExprListExp();
	appendExpression(argList, arg1);
	appendExpression(argList, arg2);
	appendExpression(argList, arg3);
	appendExpression(argList, arg4);

	SgExprStatement *call = 
		buildFunctionCallStmt(SgName("adios_declare_group"),
			buildIntType(), argList);


	return call;
}

SgExprStatement *
Group::BuildAdSelMod()
{
	/***** 	adios_select_method (GroupIDVar, "MPI", "", "") *****/
	SgVarRefExp *arg1 = buildVarRefExp(SgName(GroupIDVar));
	SgStringVal *arg2 = buildStringVal("MPI");
	SgStringVal *arg3 = buildStringVal("");
	SgStringVal *arg4 = buildStringVal("");

	SgExprListExp *argList = buildExprListExp();
	appendExpression(argList, arg1);
	appendExpression(argList, arg2);
	appendExpression(argList, arg3);
	appendExpression(argList, arg4);

	SgExprStatement *call= 
		buildFunctionCallStmt(SgName("adios_select_method"),
			buildIntType(), argList);

	return call;
}
	
SgExprStatement *
Group::BuildAdDefVar(const string &varName, const string &typeName,
	const string &count, const string &global, const string &offset)
{
	/***** Args *****/
	SgExpression *arg1, *arg2, *arg3,
			*arg4, *arg5, *arg6, *arg7;
	arg1 = buildVarRefExp(GroupIDVar);
	arg2 = buildStringVal(varName);
	arg3 = buildStringVal("");
	arg4 = GetEnumExpr("ADIOS_DATATYPES", typeName);
	arg5 = buildStringVal(count);
	arg6 = buildStringVal(global);
	arg7 = buildStringVal(offset);

	/***** Arg list *****/
	SgExprListExp *argList = buildExprListExp();
	appendExpression(argList, arg1);
	appendExpression(argList, arg2);
	appendExpression(argList, arg3);
	appendExpression(argList, arg4);
	appendExpression(argList, arg5);
	appendExpression(argList, arg6);
	appendExpression(argList, arg7);

	/***** Call *****/
	SgExprStatement *call = 
		buildFunctionCallStmt(SgName("adios_define_var"),
			buildLongLongType(), argList);

	return call;
}

SgExprStatement *
Group::BuildAdOpen()
{
	SgExpression *arg1 = 
		buildAddressOfOp(buildVarRefExp(SgName(FileVar)) );
	SgExpression *arg2 = buildStringVal(Name);
	SgExpression *arg3 = buildStringVal(FileName);
	SgExpression *arg4 = buildStringVal("w");
	SgExpression *arg5;
	if (CommExp != NULL)
		arg5 = copyExpression(CommExp);
	else 
		arg5 = buildVarRefExp("comm");
	
	SgExprListExp *argList = buildExprListExp();
	appendExpression(argList, arg1);
	appendExpression(argList, arg2);
	appendExpression(argList, arg3);
	appendExpression(argList, arg4);
	appendExpression(argList, arg5);

	SgExprStatement *call= 
		buildFunctionCallStmt(SgName("adios_open"),
			buildIntType(), argList);

	return call;
}


void
Group::InsertAdiosInit()
{
	SgStatement *orginStmt;

	if (IsPara)
		orginStmt = getEnclosingStatement(CallVV[NC_CREATE_PAR][0]);
	else
		orginStmt = getEnclosingStatement(CallVV[NC_CREATE][0]);

	SgScopeStatement *scope = getScope(orginStmt);
	pushScopeStack(scope);

//	SgTypedefDeclaration *decl = Get_MPI_Comm_Declaration();

	/***** long long adios_groupXX *****/
	SgVariableDeclaration *adios_group_VarDecl = 
		buildVariableDeclaration(GroupIDVar, buildLongLongType());

	/***** long long adios_fileXX *****/
	SgVariableDeclaration *adios_file_VarDecl = 
		buildVariableDeclaration(FileVar, buildLongLongType());


	SgExprStatement *adInitCall	= BuildAdInit();
	SgExprStatement *adAllocBufCall = BuildAdAllocBuf();
	SgExprStatement *adDeclGroupCall = BuildAdDeclGroup();
	SgExprStatement *adSelModCall = BuildAdSelMod();

	/***** Insert adios code *****/
	insertStatementAfter(orginStmt, adios_group_VarDecl);
	insertStatementAfter(adios_group_VarDecl, adios_file_VarDecl);

	insertStatementAfter(adios_file_VarDecl, adInitCall);
	cout << "Inserting adios_init_noxml" << endl;

	insertStatementAfter(adInitCall, adAllocBufCall);
	cout << "Inserting adios_allocate_buffer" << endl;

	insertStatementAfter(adAllocBufCall, adDeclGroupCall);
	cout << "Inserting adios_declare_group" << endl;

	insertStatementAfter(adDeclGroupCall, adSelModCall);
	cout << "Inserting adios_select_method" << endl;


	/***** remove original statement *****/
	removeStatement(orginStmt);


	popScopeStack();


}

SgExprStatement *
Group::BuildGroupSizeAssign(const string &groupSizeVarName)
{

	/***** Build group size assign statement *****/
	int ndims;
	SgExpression *rhs = buildUnsignedLongLongIntVal(0);
	SgExpression *countCal = buildUnsignedLongLongIntVal(1);
	SgExpression *dataSize;
	SgName countVar;	

	/***** Iterate through vars in VarMap *****/
	for (map<SgInitializedName*, VarSec>::iterator itr = VarMap.begin();
			itr != VarMap.end(); ++itr) {

		assert(itr->second.IsGlobal == true);
		ndims = itr->second.StrVec.size();
		assert(ndims > 0);
		countVar = itr->second.CountInit->get_name();

		for (vector<string>::size_type i = 0;
				i < ndims; i++) {
			countCal = 
				buildMultiplyOp(
					countCal, 
					buildPntrArrRefExp(
						buildVarRefExp(countVar),
						buildIntVal(i)
					)
				);
		}

	 	dataSize = 
			buildMultiplyOp(
				buildMultiplyOp(
					countCal,
					buildIntVal(itr->second.UnitSize)
				),
				buildIntVal(itr->second.IterNum)
			);


		rhs = 
			buildAddOp(
				rhs,
				buildAddOp(
					buildAddOp(
						buildAddOp(
							buildMultiplyOp(
								buildIntVal(ndims),
								buildIntVal(8)
							),
							buildMultiplyOp(
								buildIntVal(ndims),
								buildIntVal(4)
							)
						),
						buildMultiplyOp(
							buildMultiplyOp(
								buildIntVal(ndims),
								buildIntVal(4)
							),
							buildIntVal(itr->second.IterNum)
						)
					),
					dataSize
				)
			);
	}

	SgExprStatement *groupSizeAssignStmt = 
		buildExprStatement(
			buildAssignOp(
				buildVarRefExp(groupSizeVarName),
				rhs
			)
		);


	return groupSizeAssignStmt;

}



SgExprStatement *
Group::BuildAdGroupSize(const string &groupSizeVarName, 
						const string &totalSizeVarName)
{
	SgExpression *arg1 = buildVarRefExp(FileVar);
	SgExpression *arg2 = buildVarRefExp(groupSizeVarName);
	SgExpression *arg3 = 
		buildAddressOfOp(buildVarRefExp(totalSizeVarName));

	SgExprListExp *argList = buildExprListExp();
	appendExpression(argList, arg1);
	appendExpression(argList, arg2);
	appendExpression(argList, arg3);

	SgExprStatement *call= 
		buildFunctionCallStmt(SgName("adios_group_size"),
			buildIntType(), argList);

	return call;

}
/************************************
 * Fill IsPara data member
 ************************************/
void 
Group::FillIsPara()
{
	if (!CallVV[NC_CREATE].empty())
		IsPara = false;
	else if (!CallVV[NC_CREATE_PAR].empty())
		IsPara = true;
	else {
		cout << "ERROR: no &ncid call!" << endl;
		exit(1);
	}
}

