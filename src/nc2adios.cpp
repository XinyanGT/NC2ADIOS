#include <algorithm>
#include <numeric>
#include <map>
#include <vector>
#include "rose.h"
#include "callInst.h"
#include "funk.h"
#include "nc2adios.h"

using namespace std;


/****************************************
 * Decide whether it is a NetCDF funcion 
 * or not, based on its name
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
	nameIndMap.insert(make_pair("nc_def_dim", NC_DEF_DIM));
	nameIndMap.insert(make_pair("nc_def_var", NC_DEF_VAR));
	nameIndMap.insert(make_pair("nc_enddef", NC_ENDDEF));
	nameIndMap.insert(make_pair("nc_put_var_float", NC_PUT_VAR_FLOAT));
	nameIndMap.insert(make_pair("nc_close", NC_CLOSE));
}

/*************************
 * Initialize Funk vector
 ************************/
void
InitFunkVec(vector<Funk> &funkVec, const map<string, FUNC> &nameIndMap)
{
	for (map<string, FUNC>::const_iterator itr = nameIndMap.begin();
			itr != nameIndMap.end(); ++itr)
		funkVec[itr->second].SetName(itr->first);
}



/*************************************
 * Print info about function call 
 ************************************/
void
PrintFuncCallInfo(SgNode *node)
{
	SgFunctionCallExp *funcCallExp;

// 	funcCallExp = dynamic_cast<SgFunctionCallExp*>(node);
 	funcCallExp = isSgFunctionCallExp(node);
	ROSE_ASSERT(funcCallExp != NULL);
	ROSE_ASSERT(funcCallExp->getAssociatedFunctionSymbol() != NULL);
	SgExpressionPtrList &paraList = 
				funcCallExp->get_args()->get_expressions();


	cout << "Function call found in " << 
			funcCallExp->get_file_info()->get_filenameString() << 
			"(line:" << funcCallExp->get_file_info()->get_line() << ")" << endl;
	cout << "\t" << funcCallExp->getAssociatedFunctionSymbol() ->
				get_name().getString() << endl;
}
	

/*************************************************
 * Update Funk vector
 * based on a SgFunctionCallExp
 *************************************************/
void
UpdateFunkVec(SgNode *node, 
				vector<Funk> &funkVec,
				const map<string, FUNC> &nameIndMap, 
				const SgStringList &srcNameList)
{
	/***** Cast *****/
	SgFunctionCallExp *callExp;
 	callExp = isSgFunctionCallExp(node);
	ROSE_ASSERT(callExp != NULL);
	ROSE_ASSERT(callExp->getAssociatedFunctionSymbol() != NULL);

	
	/***** Update Funk vector *****/
	string funcName = callExp->getAssociatedFunctionSymbol() -> 
			get_name().getString();
	map<string, FUNC>::const_iterator mapItr;

	if ( (mapItr = nameIndMap.find(funcName)) != nameIndMap.end()) {
		funkVec[mapItr->second].callInstVec.push_back(
			CallInst(callExp->get_file_info(), 
				callExp, callExp->get_args()->get_expressions())
		);
	} else {
		if (HasNcPrefix(funcName)) {
			cout << "ERROR: unsupport NetCDF function: " << funcName << endl;
			exit(1);
		}

	}

//	SgExpressionPtrList paraPtrList;
//	praPtrList = funCallExp->get_args->get_expressions();
	
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
	if (find(srcNameList.begin(), srcNameList.end(), fileName)
			!= srcNameList.end()) {
		cout << "Function declaration found in " << 
			funcDecl->get_file_info()->get_filenameString() << 
			"(line:" << funcDecl->get_file_info()->get_line() << ")" << endl;
		cout << "\t" << funcDecl ->get_name().getString() << endl;
	}
}



/**********************
 * main
 **********************/
int
main(int argc, char *argv[])
{

	/***** Build AST *****/
	SgProject *project = frontend(argc, argv);
	ROSE_ASSERT(project != NULL);

	/***** Absolute path file names of source files *****/
//	srcNameList = project->get_sourceFileNameList();
	SgStringList srcNameList;
	srcNameList = project->getAbsolutePathFileNames();
	cout << "Absolute source files:" << endl;
	ostream_iterator<string> coutStrItr(cout, "\n");
	copy(srcNameList.begin(), srcNameList.end(), coutStrItr);
//	for(SgStringList::iterator itr = srcNameList.begin();
//			itr != srcNameList.end(); ++itr)
//			cout << *itr << endl;

	
	/***** Initialize Function name to index(FUNC) map *****/
	map<string, FUNC> funcNameIndMap;	
	InitFuncNameIndMap(funcNameIndMap);
	/***** Initialize Funk vector *****/
	vector<Funk> funkVec(FUNC_SIZE);
	InitFunkVec(funkVec, funcNameIndMap);


	/***** Query *****/ 
	Rose_STL_Container<SgNode*>funcCallList =
		NodeQuery::querySubTree(project, V_SgFunctionCallExp);
	Rose_STL_Container<SgNode*>funcDeclList =
		NodeQuery::querySubTree(project, V_SgFunctionDeclaration);

	/***** Print *****/
//	for (Rose_STL_Container<SgNode*>::iterator itr = funcCallList.begin();
//			itr != funcCallList.end(); ++itr) 
//			PrintFuncCallInfo(*itr);
//
//	for (Rose_STL_Container<SgNode*>::iterator itr = funcDeclList.begin();
//			itr != funcDeclList.end(); ++itr) 
//			PrintFuncDeclInfo(*itr, srcNameList);

	for (Rose_STL_Container<SgNode*>::iterator itr = funcCallList.begin();
			itr != funcCallList.end(); ++itr) 
			UpdateFunkVec(*itr, funkVec, funcNameIndMap, srcNameList);

	for (vector<Funk>::iterator itr = funkVec.begin();
			itr != funkVec.end(); ++itr) 
		cout << *itr;


	return 0;
}


