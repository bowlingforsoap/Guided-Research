#pragma once

// Helper measurement macros. To be used together. Place before and after the code which you want to measure execution time for. 
// iterations - the number of times to execute code;
// result - total and mean time output in cout.
#define measure_start(iterations) double timeDelta = glfwGetTime(); \
	const int n = iterations; \
	for (int i = 0; i < n; i++) {

#define measure_end } \
	timeDelta = (glfwGetTime() - timeDelta); \
	std::cout << "Total time for n = " << n << " executions: " << timeDelta << '\n'; \
	timeDelta /= n; \
	std::cout << "Mean time per execution: " << timeDelta << '\n';