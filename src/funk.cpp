#include "funk.h"
#include <iomanip>

using namespace std;

ostream& 
operator<< (ostream &out, const Funk &funk)
{
	cout <<  "Function Name: " << setw(30) << left << 
		funk.name << "Occurs: " << 
		funk.callInstVec.size() << " times" << endl;
}

void
Funk::SetName(const string &str)
{
	name = str;
}

string 
Funk::GetName() const
{
	return name;
}
