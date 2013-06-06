// #include <algorithm>
#include <numeric>
#include <map>
#include <vector>
#include "rose.h"
#include "roseHelper.h"
#include "func.h"
#include "utils.h"
#include "group.h"

using namespace std;
using namespace RoseHelper;
using namespace SageBuilder;
using namespace SageInterface;



/**********************
 * main
 **********************/
int
main(int argc, char *argv[])
{


	/***** Build AST *****/
	SgProject *project = frontend(argc, argv);
	ROSE_ASSERT(project != NULL);

	/***** In C ? *****/
	if (!is_C_language()) {
		cout << "NOT in C. Quit." << endl;
		exit(1);
	} else 
		cout << "In C. " << endl;


	/***** Absolute path file names of source files *****/
	SgStringList srcNameList;
//	srcNameList = project->get_sourceFileNameList();
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

	/***** Query *****/ 
	Rose_STL_Container<SgNode*>callList =
		NodeQuery::querySubTree(project, V_SgFunctionCallExp);
//	Rose_STL_Container<SgNode*>funcDeclList =
//		NodeQuery::querySubTree(project, V_SgFunctionDeclaration);

	/***** Print *****/
//	for (Rose_STL_Container<SgNode*>::iterator itr = callList.begin();
//			itr != callList.end(); ++itr) 
//			PrintFuncCallInfo(*itr);
//
//	for (Rose_STL_Container<SgNode*>::iterator itr = funcDeclList.begin();
//			itr != funcDeclList.end(); ++itr) 
//			PrintFuncDeclInfo(*itr, srcNameList);


	/***** Print Funk info *****/
//	for (vector<Funk>::iterator itr = funkVec.begin();
//			itr != funkVec.end(); ++itr) 
//		cout << *itr;

	vector<SgFunctionCallExp*> ncCall;
	vector< vector<SgFunctionCallExp*> > callGroupVec;

	FilterCall(callList, ncCall, funcNameIndMap);
	GroupNcCall(ncCall, callGroupVec);

	vector<Group*> groupPtrVec(callGroupVec.size());
	cout << "total group num: " << groupPtrVec.size() << endl;

	for (vector< vector<SgFunctionCallExp*> >::size_type i = 0;
			i < callGroupVec.size(); ++i) {
		cout << "group " << i << endl;
		groupPtrVec[i] = new Group(callGroupVec[i], funcNameIndMap, i);
//		for (vector<SgFunctionCallExp*>::size_type j = 0; 
//				j < callGroupVec[i].size(); ++j) {
//			cout << "\t" << GetCallName(callGroupVec[i][j]) << endl;
//		}
	}

//	InsertMPI(project);
//	groupPtrVec[0]->InsertAdiosInitFuncs();
	cout << setw(80) << setfill('*')<< '*' << endl;
	cout << "Processing nc_create_par..." << endl;
	groupPtrVec[0]->Process_nc_create_par();
	cout << setw(80) << setfill('*')<< '*' << endl;
	cout << "Processing nc_def_dim..." << endl;
	groupPtrVec[0]->Process_nc_def_dim();
	cout << setw(80) << setfill('*')<< '*' << endl;
	cout << "Extracting nc_def_var..." << endl;
	groupPtrVec[0]->Extract_nc_def_var();
	cout << "Extracting nc_put_vara_int..." << endl;
	groupPtrVec[0]->Extract_nc_put_vara_int();
	cout << "Processing nc_def_var..." << endl;
	groupPtrVec[0]->Process_nc_def_var();
	cout << "Process nc_enddef..." << endl;
	groupPtrVec[0]->Process_nc_enddef();


	AstTests::runAllTests(project);
	AstPostProcessing(project);
	project->unparse();


	return 0;
}


