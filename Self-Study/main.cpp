
#include <iostream>
#include <list>


#include "ThreadPool.h"


//************************************************* ThreadPool *********************************//
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

int main()
{
    TestThreadPoolSort();


    return 0;
}

