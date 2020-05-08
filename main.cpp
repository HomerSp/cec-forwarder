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
#include "irreader.h"

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

    bool argVerbose = false, argTest = false;
    for (int i = 1; i < argc; i++) {
        std::string a = std::string(argv[i]);
        if (a == "-v") {
            argVerbose = true;
        } else if (a == "-t") {
            argTest = true;
        }
    }

    HueConfig config("/etc/cec-forwarder/cec-forwarder.config");
    if (!config.parse()) {
        std::cerr << "Failed parsing config\n";
        return -1;
    }

    if (!config.contains("Main", "keyname")) {
        std::cerr << "Config: Required Main.keyname parameter missing\n";
        return -1;
    }

    HueConfigSection* mainSection = config.getSection("Main");

    std::string cecname = "CECForwarder";
    if (mainSection->hasKey("cecname")) {
        cecname = mainSection->value("cecname");
    }

    IRReader irReader("/etc/cec-forwarder", mainSection->value("irname"));

    CecForwarder forwarder("/etc/cec-forwarder", mainSection->value("keyname"), cecname, argVerbose);
    forwarder.setRepeat(mainSection->intValue("repeatdelay"), mainSection->intValue("repeatrate"));

    HueConfigSection* keySection = config.getSection("Keys");
    if (keySection != nullptr) {
        for (auto it = keySection->begin(); it != keySection->end(); it++) {
            forwarder.addKey(std::atoi(it->first.c_str()), it->second);
        }
    }

    irReader.addCallback(&forwarder);
    irReader.CreateThread(false);

    while (!g_bHardExit) {
        if (!forwarder.ensureOpen()) {
            CEvent::Sleep(1000);
            continue;
        }

        CEvent::Sleep(1);
    }

    std::cerr << "All done\n";

    forwarder.close();
    irReader.cancel();

    return (g_bHardExit) ? -1 : 0;
}
