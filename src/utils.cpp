#include "utils.h"

using namespace std;
using namespace RoseHelper;
using namespace SageBuilder;
using namespace SageInterface;




/**************************************
 * Decide whether a node is the type 
 * define declaration for MPI_Comm.
 * If it is, push it back to a vector.
 *************************************/
static vector<SgNode*>
Is_MPI_Comm_Declaration(SgNode *node)
{
	SgTypedefDeclaration *decl;
	vector<SgNode*> vec;

	decl = isSgTypedefDeclaration(node);
	if ( (decl != NULL) &&
//			(decl->get_file_info()->get_filenameString() == "mpi.h") &&
			(decl->get_name().getString() == "MPI_Comm") 
		)
		vec.push_back(decl);

	return vec;

}

/*************************************
 * Return the type define declaration
 * for MPI_Comm
 ************************************/
SgTypedefDeclaration *
Get_MPI_Comm_Declaration(SgNode *node)
{
	static SgTypedefDeclaration *decl = NULL;

	if (decl == NULL) {
		assert(node != NULL);
		vector<SgNode*> vec = 
			NodeQuery::querySubTree(node, Is_MPI_Comm_Declaration);
		cout << "vector size: " << vec.size() << endl;
		assert(vec.size() == 1);
		decl = isSgTypedefDeclaration(vec[0]);
	}

	return decl;
}


/******************************************
 * Insert MPI related code 
 *****************************************/
void 
InsertMPI(SgProject *project)
{

	/***** main body *****/
	SgFunctionDeclaration *mainFunc = findMain(project);
	SgBasicBlock *body = mainFunc->get_definition()->get_body();


	/***** Use main body scope *****/
	pushScopeStack(body);


	/***** int rank *****/
	SgVariableDeclaration *rankVarDecl = 
		buildVariableDeclaration("rank", buildIntType());


	/***** int size *****/
	SgVariableDeclaration *sizeVarDecl = 
		buildVariableDeclaration("size", buildIntType());


	/***** MPI_Comm comm = MPI_WORLD_COMM *****/
	SgTypedefDeclaration *commTypeDecl;
	commTypeDecl= Get_MPI_Comm_Declaration(project);
	SgType *MPICommType = SgTypedefType::createType(commTypeDecl);
	SgType *intType = buildIntType();

	SgExpression *initExp = 
			buildCastExp(buildIntVal(0x44000000), 
				MPICommType
			);
	SgVariableDeclaration *commVarDecl =
			buildVariableDeclaration("comm", 
				MPICommType,
				buildAssignInitializer(initExp)
			);


	/***** Insert var declarations for MPI *****/
	prependStatement(rankVarDecl);
	insertStatementAfter(rankVarDecl, sizeVarDecl);
	insertStatementAfter(sizeVarDecl, commVarDecl);


	/***** MPI_Init(&argc, &argv) *****/
	SgAddressOfOp *arg11 = buildAddressOfOp(buildVarRefExp(SgName("argc")));
	SgAddressOfOp *arg12 = buildAddressOfOp(buildVarRefExp(SgName("argv")));
	SgExprListExp *argList1 = buildExprListExp();
	appendExpression(argList1, arg11);
	appendExpression(argList1, arg12);
	SgExprStatement *MPIInitCall = 
		buildFunctionCallStmt(SgName("MPI_Init"), intType, argList1);


	/***** MPI_Comm_rank(comm, &rank) *****/
	SgVarRefExp *arg21 = buildVarRefExp(SgName("comm")); 
	SgAddressOfOp *arg22 = buildAddressOfOp(buildVarRefExp(SgName("rank")));
	SgExprListExp *argList2 = buildExprListExp();
	appendExpression(argList2, arg21);
	appendExpression(argList2, arg22);
	SgExprStatement *MPICommRankCall = 
		buildFunctionCallStmt(SgName("MPI_Comm_rank"), intType, argList2);

	/***** MPI_Comm_size(comm, &size) *****/ 
	SgVarRefExp *arg31 = buildVarRefExp(SgName("comm")); 
	SgAddressOfOp *arg32 = buildAddressOfOp(buildVarRefExp(SgName("size")));
	SgExprListExp *argList3 = buildExprListExp();
	appendExpression(argList3, arg31);
	appendExpression(argList3, arg32);
	SgExprStatement *MPICommSizeCall = 
		buildFunctionCallStmt(SgName("MPI_Comm_size"), intType, argList3);

	/***** MPI_Finalize() *****/
	SgExprListExp *argList4 = buildExprListExp();
	SgExprStatement *MPIFinalizeCall = 
		buildFunctionCallStmt(SgName("MPI_Finalize"), intType, argList4);


	/***** Insert MPI calls *****/
 	insertStatementAfter(commVarDecl, MPIInitCall);
	insertStatementAfter(MPIInitCall, MPICommRankCall);
	insertStatementAfter(MPICommRankCall, MPICommSizeCall);
	instrumentEndOfFunction(mainFunc, MPIFinalizeCall);


	/***** End work *****/
	popScopeStack();
}


/************************************************
 * Fill in a function name to FUNC enum map 
 ***********************************************/
void 
InitFuncNameIndMap(map<string, FUNC> &nameIndMap)
{
	/* Clear the map first */
	nameIndMap.clear();

	/* fill in the map */
	nameIndMap.insert(make_pair("nc_create", NC_CREATE));
	nameIndMap.insert(make_pair("nc_create_par", NC_CREATE_PAR));
	nameIndMap.insert(make_pair("nc_def_dim", NC_DEF_DIM));
	nameIndMap.insert(make_pair("nc_def_var", NC_DEF_VAR));
	nameIndMap.insert(make_pair("nc_enddef", NC_ENDDEF));
	nameIndMap.insert(make_pair("nc_put_var_float", NC_PUT_VAR_FLOAT));
	nameIndMap.insert(make_pair("nc_put_vara_int", NC_PUT_VARA_INT));
	nameIndMap.insert(make_pair("nc_close", NC_CLOSE));
}



/****************************************
 * Decide whether the func name has nc 
 * functions' prefix "nc_" or not
 ****************************************/
inline bool 
HasNcPrefix(const string &funcName)
{
	if ( (funcName.length() > 3) && 
			(funcName[0] == 'n') &&
			(funcName[1] == 'c') &&
			(funcName[2] == '_') )
		return true;
	else 
		return false;
}
	
/****************************************
 * Only leave supported NetCDF calls
 ****************************************/
void
FilterCall(Rose_STL_Container<SgNode *>&callList,
			vector<SgFunctionCallExp *> &ncCall, 
			const map<string, FUNC> &nameIndMap)
{

	SgFunctionCallExp *callExp;
	string funcName;
	map<string, FUNC>::const_iterator mapItr;

	/***** Clear nc calls vector first *****/
	ncCall.clear();


	/***** Iterate through func call *****/
	for (Rose_STL_Container<SgNode*>::iterator itr = callList.begin();
			itr != callList.end(); ++itr) {

		/***** Cast *****/
	 	callExp = isSgFunctionCallExp(*itr);
		ROSE_ASSERT(callExp != NULL);


		/***** Supported nc function? *****/
		funcName = GetCallName(callExp);
		if ( (mapItr = nameIndMap.find(funcName)) != nameIndMap.end()) {
			ncCall.push_back(callExp);
		} else {
			if (HasNcPrefix(funcName)) {
				cout << "ERROR: unsupport NetCDF function: " << 
					funcName << endl;
				exit(1);
			}
		}
	}
}



	

/************************************
 * Group nc functions related to one 
 * ncid together
 ************************************/
void 
GroupNcCall(vector<SgFunctionCallExp*> &callVec, 
			vector< vector<SgFunctionCallExp*> > &callGroupVec)
{
	map<SgInitializedName*, int> ncidMap;

	/* Fill a map between declaration of ncid, which occurs in 
	 * nc file calls, such as nc_create and nc_create_par 
	 * and index in callGroupVec */
	InitNcidMap(callVec, ncidMap);

	/* Group NetCDF functions based on the map generated 
	 * in the above step */
	GroupNcCallNow(callVec, callGroupVec, ncidMap);
}


/******************************************
 * Group NetCDF functions based on a ncid 
 * declaration to index map
 ******************************************/
void
GroupNcCallNow(vector<SgFunctionCallExp*> &callVec, 
			vector< vector<SgFunctionCallExp*> > &callGroupVec,
			map<SgInitializedName*, int>ncidMap)
{
	SgFunctionCallExp *callExp;
	SgInitializedName *varInitName;
	string funcName;


	/***** Clear and resize callGroupVec *****/
	callGroupVec.clear();
	callGroupVec.resize(ncidMap.size());

	/***** Iterate through nc call Vec *****/
	for (vector<SgFunctionCallExp*>::size_type i = 0;
			i != callVec.size(); ++i) {

		callExp = callVec[i];
		funcName = GetCallName(callExp);
		if (funcName == string("nc_create")) {
			varInitName = ArgIntPtr_InitName(GetCallArgs(callExp)[2]); 
		} else if (funcName == string("nc_create_par")) {
			varInitName = ArgIntPtr_InitName(GetCallArgs(callExp)[4]); 
		} else {
			varInitName = ArgVarRef_InitName(GetCallArgs(callExp)[0]);
		}

		callGroupVec[ncidMap[varInitName]].push_back(callExp);
	}
}




/********************************************
 * Fill a mapping between SgInitializedName*
 * for ncid and index 
 ********************************************/
void 
InitNcidMap(const vector<SgFunctionCallExp*>&callVec, 
				map<SgInitializedName*, int> &ncidMap)
{
	SgFunctionCallExp *callExp;
	SgExpression *exp;
	SgInitializedName *varInitName;
	string funcName;
	bool toInsert;

	int index = 0;;

	ncidMap.clear(); // clear the map first

	for (vector<SgFunctionCallExp*>::size_type i = 0;
		i != callVec.size(); ++i) {

		callExp = callVec[i];
		funcName = GetCallName(callExp);

		/***** Is it nc_create()? *****/
		if ( funcName == string("nc_create") ) {
			exp= GetCallArgs(callExp)[2];
			toInsert = true;

		/***** Is it nc_create_par()? *****/
		} else if ( funcName == string("nc_create_par") ) {
			exp= GetCallArgs(callExp)[4];
			toInsert = true;
		}
		/**** Update the map *****/
		if (toInsert) {
			varInitName = ArgIntPtr_InitName(exp);
			ncidMap.insert(make_pair(varInitName, index++));
		}
	}
}



/*************************************
 * Print info about function call 
 ************************************/
void
PrintFuncCallInfo(SgNode *node)
{
	SgFunctionCallExp *callExp;

// 	funcCallExp = dynamic_cast<SgFunctionCallExp*>(node);
 	callExp = isSgFunctionCallExp(node);
	ROSE_ASSERT(callExp != NULL);

//	const vector<SgExpression*> &argVec = GetCallArgs(callExp);

	cout << "Function call found in " << GetCallFileName(callExp) <<
			"(line:" << GetCallFileLine(callExp) << ")" << endl;
	cout << "\t" << GetCallName(callExp) << endl;
}
	

/****************************************
 * Print info about function declaration
 ****************************************/
void
PrintFuncDeclInfo(SgNode *node, const SgStringList &srcNameList)
{
	SgFunctionDeclaration *funcDecl;
// 	funcDecl = dynamic_cast<SgFunctionDeclaration*>(node);
 	funcDecl = isSgFunctionDeclaration(node);
	ROSE_ASSERT(funcDecl != NULL);
	string fileName = funcDecl->get_file_info()->get_filenameString();


//	if (funcDecl->get_file_info()->isCompilerGenerated() == true) {

	/***** Only function declarations in input files *****/
	if (find(srcNameList.begin(), srcNameList.end(), fileName)
			!= srcNameList.end()) {
		cout << "Function declaration found in " << 
			funcDecl->get_file_info()->get_filenameString() << 
			"(line:" << funcDecl->get_file_info()->get_line() << ")" << endl;
		cout << "\t" << funcDecl ->get_name().getString() << endl;
	}
}

static int
GetInitializerIntVal(SgInitializer *initptr)
{
	assert(initptr->variantT() == V_SgAssignInitializer);
	SgExpression *exp;
	exp = (static_cast<SgAssignInitializer*>(initptr))->get_operand();
	assert(exp->variantT() == V_SgIntVal);
	return (static_cast<SgIntVal*>(exp))->get_value();
}


/****************************************
 * Get the int value of an enum constatnt
 ****************************************/
int
GetEnumIntVal(SgEnumDeclaration *decl, const string &name)
{
	vector<SgInitializedName*> vec = decl->get_enumerators();
	int val = 0;

	for (vector<SgInitializedName*>::size_type i = 0;
			i != vec.size(); ++i) {
		if (vec[i]->get_initializer() != NULL)
			val = GetInitializerIntVal(vec[i]->get_initializer());
		if (vec[i]->get_name() == name) 
			return val;
		val++;
	}
	return INT_MAX;
}


/***************************************
 * Get the declaration for an enum type
 ***************************************/
SgEnumDeclaration *
GetEnumDecl(const string &name)
{
	static vector<SgNode*> vec;
	SgEnumDeclaration *decl;

	if (vec.empty()) {
		SgProject *project = getProject();
		vec = NodeQuery::querySubTree(project, V_SgEnumDeclaration);
	}

	for (vector<SgNode*>::size_type i = 0; 
			i < vec.size(); ++i) {
		decl = isSgEnumDeclaration(vec[i]);
		if (decl->get_name().getString() == name)
			return decl;
	}
	return NULL;
}

/*****************************************
 * Get the expression of an enum constant
 ******************************************/
SgExpression *
GetEnumExpr(const string &enumType, const string &enumConstant)
{
	SgEnumDeclaration *decl = GetEnumDecl(enumType);
	assert(decl != NULL);
	int val = GetEnumIntVal(decl, enumConstant);
	return BuildEnumVal(val, decl, enumConstant);
}
	

SgEnumVal* 
BuildEnumVal(int value, SgEnumDeclaration* decl, SgName name)
{
  SgEnumVal* enumVal= new SgEnumVal(value,decl,name);
  ROSE_ASSERT(enumVal);
  SageInterface::setOneSourcePositionForTransformation(enumVal);
  return enumVal;
}

SgForStatement *
BuildCanonicalForStmt(int low, int high, const string &indexName)
{
	if (indexName.empty())
		return BuildCanonicalForStmtDeclIn(low, high);
	else
		return BuildCanonicalForStmtDeclOut(low, high, indexName);
}


/***************************************
 * Create a For Statement with a empty
 * basic block body
 * Note: the index "i" is declared in 
 *		the for init statement
 ***************************************/
SgForStatement *
BuildCanonicalForStmtDeclIn(int low, int high)
{
	SgStatement *initStmt = 
		buildVariableDeclaration( "i", buildIntType(), 
			buildAssignInitializer(buildIntVal(low)), NULL);

	SgStatement *testStmt = 
		buildExprStatement(buildLessThanOp(
			buildVarRefExp("i", NULL),
			buildIntVal(high)
			)
		);

	SgExpression *incrementExp = 
		buildPlusPlusOp(buildVarRefExp("i", NULL),
			SgUnaryOp::postfix);

	SgStatement *loopStmt = buildBasicBlock();

	SgForStatement *forStmt = 
		buildForStatement(initStmt, testStmt, incrementExp, loopStmt);

	return forStmt;
}

/***************************************
 * Create a For Statement with a empty
 * basic block body
 * Note: the index is declared outside 
 *		the For Statement
 ***************************************/
SgForStatement *
BuildCanonicalForStmtDeclOut(int low, int high, string indexName)
{

	SgStatement *initStmt = 
		buildAssignStatement(buildVarRefExp(indexName), buildIntVal(low));

	SgStatement *testStmt =
		buildExprStatement(buildLessThanOp(
			buildVarRefExp(indexName), buildIntVal(high) 
			)
		);

	SgExpression *incrementExp = 
		buildPlusPlusOp(buildVarRefExp(indexName));

	SgStatement *loopStmt = buildBasicBlock();

	SgForStatement *forStmt = 
		buildForStatement(initStmt, testStmt, incrementExp, loopStmt);

	return forStmt;
}

/*****************************************
 * Extract low bound and high bound from 
 * a For Statement
 *****************************************/
void
ExtractForStmtBounds(SgForStatement *forStmt, int &low, int &high)
{

	/***** Get index low bound *****/
	vector<SgStatement*> initStmtVec = forStmt->get_init_stmt();
	assert(initStmtVec.size() == 1);
	assert(initStmtVec[0]->variantT() == V_SgExprStatement);
	SgAssignOp *assOp= 
		isSgAssignOp( (static_cast<SgExprStatement*>(initStmtVec[0]))->
				get_expression());
	assert(assOp != NULL);

	SgCastExp *r;
	r = isSgCastExp(assOp->get_rhs_operand());
	assert(r != NULL);

	SgIntVal *intVal;
	intVal = isSgIntVal(r->get_operand());
	assert(intVal != NULL);
	low = intVal->get_value();
//	cout << "Low bound is " << low << endl;


	/***** Get index high bound *****/
	SgExprStatement *testStmt;
	testStmt = isSgExprStatement(forStmt->get_test());
	assert(testStmt != NULL);

	SgLessThanOp *lessOp;
	lessOp = isSgLessThanOp(testStmt->get_expression());
	assert(lessOp != NULL);

	r = isSgCastExp(lessOp->get_rhs_operand());
	assert(r != NULL);
	intVal = isSgIntVal(r->get_operand());
	assert(intVal != NULL);
	high = intVal->get_value();
//	cout << "High bound is " << high << endl;


	/***** Ensure that the index increments 1 per iter *****/
	SgPlusPlusOp *ppOP;
	ppOP = isSgPlusPlusOp(forStmt->get_increment());
	assert(ppOP != NULL);

}

string
MakeStr(vector<string> strVec, string prefix)
{
	string str;
	for (vector<string>::size_type i = 0; i < strVec.size()-1; i++) { 
		str += prefix + strVec[i] + ",";
	}
	str += strVec[strVec.size()-1];

	return str;
}


