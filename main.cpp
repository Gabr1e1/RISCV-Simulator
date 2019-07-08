#include <iostream>
#include "executor.h"

int main()
{
	freopen("pi.data", "r", stdin);
	auto *exec = new Executor();
	exec->read();
	std::cout << exec->execute() << std::endl;
	printf("PREDICTION ACC: %.2f%% %d misses in %d total\n", 100 * (double) (exec->total - exec->miss) / (double) exec->total,
		   exec->miss, exec->total);
	delete exec;
	return 0;
}