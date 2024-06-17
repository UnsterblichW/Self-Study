

#ifndef _PROCESSPRIO_H
#define _PROCESSPRIO_H


#define FMT_HEADER_ONLY // (#define FMT_HEADER_ONLY)��ǿ���Եģ������߱�����ҲҪ����fmtͷ�ļ�
#include <fmt/core.h>

#include <string>

#define CONFIG_PROCESSOR_AFFINITY "UseProcessors"
#define CONFIG_HIGH_PRIORITY "ProcessPriority"

void SetProcessPriority(std::string const& logChannel, uint32_t affinity, bool highPriority);

#endif
