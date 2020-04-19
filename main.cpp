#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <p8-platform/os.h>
#include <p8-platform/util/StringUtils.h>
#include <p8-platform/threads/threads.h>

#include "cecforwarder.h"

using namespace P8PLATFORM;

bool g_bHardExit(false);

void sighandler(int iSignal)
{
    std::cerr << "signal caught: " << iSignal << " - exiting\n";
    g_bHardExit = true;
}

int main (int argc, char *argv[])
{
    if (signal(SIGINT, sighandler) == SIG_ERR) {
        std::cerr << "can't register sighandler\n";
        return -1;
    }

    std::cerr << "CecForwarder\n";
    CecForwarder forwarder;
    std::cerr << "Starting main loop\n";
    while (!g_bHardExit) {
        if (!forwarder.process()) {
            std::cerr << "!process\n";
            CEvent::Sleep(1000);
            continue;
        }

        CEvent::Sleep(1);
    }

    std::cerr << "All done\n";

    forwarder.close();
    return (g_bHardExit) ? -1 : 0;
}
