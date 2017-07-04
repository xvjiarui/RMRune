#include <iostream>
#include "Voter.hpp"
#include <vector>
using namespace std;
int main()
{
	VoteSetting vs;
	vs.saveTime = 7;
	Voter<vector<int> > test(vs);
	vector<int> t = {3,6,7,5,8,9,4,2,1};
	vector<int> t1 = {3,6,7,5,8,9,4,2,1};
	vector<int> t2= {7,4,6,2,1,8,9,3,0};
	vector<int> res;
	test.PushElement(t);
	test.PushElement(t);
	test.PushElement(t1);
	test.PushElement(t1);
	test.PushElement(t2);
	test.PushElement(t2);
	test.PushElement(t2);
	if(test.GetBestElement(res))
	{
	for(auto i:res)
	{
		cout << i;
	}
	cout << endl;
	}
	else
	{
		cout <<"fuck"<<endl;
	}
	//test.RemoveOldElements(6);
	test.PushElement(t);
	test.PushElement(t);
	test.PushElement(t2);
	test.PushElement(t2);
	test.PushElement(t2);
	test.PushElement(t2);
	test.GetBestElement(res);
	for(auto i:res)
	{
		cout << i;
	}
	cout << endl;
	return 0;
}
