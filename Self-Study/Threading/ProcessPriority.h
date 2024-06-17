

#ifndef _PROCESSPRIO_H
#define _PROCESSPRIO_H


#define FMT_HEADER_ONLY // (#define FMT_HEADER_ONLY)是强制性的，它告诉编译器也要编译fmt头文件
#include <fmt/core.h>

#include <string>

#define CONFIG_PROCESSOR_AFFINITY "UseProcessors"
#define CONFIG_HIGH_PRIORITY "ProcessPriority"

void SetProcessPriority(std::string const& logChannel, uint32_t affinity, bool highPriority);

#endif
