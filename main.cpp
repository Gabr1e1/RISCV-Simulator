#include <iostream>
#include "executor.h"

int main()
{
//	freopen("pi.data", "r", stdin);
//	freopen("myans.out","w",stdout);
	std::ios::sync_with_stdio(false);
	auto *exec = new Executor();
	exec->read();
	std::cout << exec->execute() << std::endl;
	delete exec;
	return 0;
}