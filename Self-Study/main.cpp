
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


//************************************************* fmt ****************************************//
#define FMT_HEADER_ONLY // (#define FMT_HEADER_ONLY)是强制性的，它告诉编译器也要编译fmt头文件
#include <fmt/core.h>
// 如果不写 FMT_HEADER_ONLY ，那么就得在项目设置里面把编译出来的dll或者lib链到当前需要调试的项目里面
// 或者像下面这样写
//
//#ifndef _WIN64
//    #ifdef _DEBUG
//        #pragma comment(lib, "../lib32/debug/fmtd.lib")  路径得改
//    #else
//        #pragma comment(lib, "../lib32/release/fmt.lib")
//    #endif
//#else
//    #ifdef _DEBUG
//        #pragma comment(lib, "../lib64/debug/fmtd.lib")
//    #else
//        #pragma comment(lib, "../lib64/release/fmt.lib")
//    #endif
//#endif

void TestFormat() {
    std::string s = fmt::format("The answer is {}.", 42);

    s = fmt::format("{0}{1}{0}", "abra", "cad");

    std::cout << s << std::endl;

    fmt::print("{0:-^10}\n{1:-^10}", "hello", "world");

}

//************************************************* fmt ****************************************//



//************************************************* iocp ***************************************//
#include "test_iocp.h"



//************************************************* iocp ***************************************//

int main()
{
    //TestThreadPoolSort();

    //TestFormat();

    test_iocp_demo();


    return 0;
}

