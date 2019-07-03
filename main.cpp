#include <iostream>
#include "executor.h"

int main()
{
	freopen("sample.data","r",stdin);
	auto *exec = new Executor();
	exec->read();
	std::cout << exec->execute() << std::endl;
	delete exec;
	return 0;
}