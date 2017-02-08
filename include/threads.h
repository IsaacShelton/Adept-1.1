
#ifndef THREADS_H_INCLUDED
#define THREADS_H_INCLUDED

#include <future>
#include <thread>
#include <vector>

int threads_int_result(std::vector<std::future<int>>& threads);

#endif // THREADS_H_INCLUDED
