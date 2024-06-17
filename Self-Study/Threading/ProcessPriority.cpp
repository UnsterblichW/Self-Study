
#include "ProcessPriority.h"

#ifdef _WIN32 // Windows
#include <Windows.h>
#elif defined(__linux__)
#include <sched.h>
#include <sys/resource.h>
#define PROCESS_HIGH_PRIORITY -15 // [-20, 19], default is 0
#endif

void SetProcessPriority(std::string const& logChannel, uint32_t affinity, bool highPriority)
{
    ///- Handle affinity for multiple processors and process priority
#ifdef _WIN32 // Windows

    HANDLE hProcess = GetCurrentProcess();
    if (affinity > 0)
    {
        ULONG_PTR appAff;
        ULONG_PTR sysAff;

        if (GetProcessAffinityMask(hProcess, &appAff, &sysAff))
        {
            // remove non accessible processors
            ULONG_PTR currentAffinity = affinity & appAff;

            if (!currentAffinity)
                fmt::print(logChannel, "Processors marked in UseProcessors bitmask (hex) {:x} are not accessible. Accessible processors bitmask (hex): {:x}", affinity, appAff);
            else if (SetProcessAffinityMask(hProcess, currentAffinity))
                fmt::print(logChannel, "Using processors (bitmask, hex): {:x}", currentAffinity);
            else
                fmt::print(logChannel, "Can't set used processors (hex): {:x}", currentAffinity);
        }
    }

    if (highPriority)
    {
        if (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS))
            fmt::print(logChannel, "Process priority class set to HIGH");
        else
            fmt::print(logChannel, "Can't set process priority class.");
    }

#elif defined(__linux__) // Linux

    if (affinity > 0)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);

        for (unsigned int i = 0; i < sizeof(affinity) * 8; ++i)
            if (affinity & (1 << i))
                CPU_SET(i, &mask);

        if (sched_setaffinity(0, sizeof(mask), &mask))
            fmt::print(logChannel, "Can't set used processors (hex): {:x}, error: {}", affinity, strerror(errno));
        else
        {
            CPU_ZERO(&mask);
            sched_getaffinity(0, sizeof(mask), &mask);
            fmt::print(logChannel, "Using processors (bitmask, hex): {:x}", *(__cpu_mask*)(&mask));
        }
    }

    if (highPriority)
    {
        if (setpriority(PRIO_PROCESS, 0, PROCESS_HIGH_PRIORITY))
            fmt::print(logChannel, "Can't set process priority class, error: {}", strerror(errno));
        else
            fmt::print(logChannel, "Process priority class set to {}", getpriority(PRIO_PROCESS, 0));
    }

#else
    // Suppresses unused argument warning for all other platforms
    (void)logChannel;
    (void)affinity;
    (void)highPriority;
#endif
}
