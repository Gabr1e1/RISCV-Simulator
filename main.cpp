#include <iostream>
#include "executor.hpp"

int main()
{
	auto *exec = new Executor();
	exec->read();
	std::cout << exec->execute() << std::endl;
	delete exec;

	return 0;
}