
#include <iostream>
#include <list>

//************************************************* ThreadPool *********************************//
#include "ThreadPool.h"

template<typename T>
std::list<T>pool_thread_quick_sort(std::list<T> input) {
    if (input.empty())
    {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& partition_val = *result.begin();
    typename std::list<T>::iterator divide_point =
        std::partition(input.begin(), input.end(),
            [&](T const& val) {return val < partition_val; });
    std::list<T> new_lower_chunk;
    new_lower_chunk.splice(new_lower_chunk.end(),
        input, input.begin(),
        divide_point);
    std::future<std::list<T> > new_lower = ThreadPool::instance().commit(pool_thread_quick_sort<T>, new_lower_chunk);
    std::list<T> new_higher(pool_thread_quick_sort(input));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

void TestThreadPoolSort() {
    std::list<int> nlist = { 6,1,0,5,2,9,11 };
    auto sortlist = pool_thread_quick_sort<int>(nlist);
    for (auto& value : sortlist) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
}

//************************************************* ThreadPool *********************************//



#define FMT_HEADER_ONLY // (#define FMT_HEADER_ONLY)是强制性的，它告诉编译器也要编译fmt头文件
#include <fmt/core.h>




int main()
{
    //TestThreadPoolSort();

    std::string s = fmt::format("The answer is {}.", 42);

    std::cout << s << std::endl;

    return 0;
}

