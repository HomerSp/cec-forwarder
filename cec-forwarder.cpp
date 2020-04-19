/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2015 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#include "env.h"
#include <libcec/cec.h>

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

using namespace CEC;
using namespace P8PLATFORM;

#include <libcec/cecloader.h>

#include "irsend.h"
#include "config.h"

static void FindPort();

ICECCallbacks        g_callbacks;
libcec_configuration g_config;
std::string          g_strPort;
bool                 g_bExit(false);
bool                 g_bHardExit(false);
CMutex               g_outputMutex;
ICECAdapter*         g_parser;
HueConfig*           g_iniconfig;

struct CECIRKeys {

};

struct KeyRepeat {
  cec_user_control_code keycode;
  uint64_t lastpress;
  uint64_t repeatstart;
};

KeyRepeat g_keyRepeat;

class CReconnect : public P8PLATFORM::CThread
{
public:
  static CReconnect& Get(void)
  {
    static CReconnect _instance;
    return _instance;
  }

  virtual ~CReconnect(void) {}

  void* Process(void)
  {
    if (g_parser)
    {
      g_parser->Close();
      FindPort();
      while (true)
      {
        FindPort();
        std::cerr << "Attempting to reconnect to " <<  g_strPort.c_str() << "\n";
        if (g_parser->Open(g_strPort.c_str())) {
            break;
        }

        std::cerr << "Failed to reconnect, retrying in a bit\n";
        sleep(500);
      }
    }
    return NULL;
  }

private:
  CReconnect(void) {}
};

static void FindPort()
{
    cec_adapter_descriptor devices[10];
    uint8_t iDevicesFound = g_parser->DetectAdapters(devices, 10, NULL, true);
    if (iDevicesFound > 0)
    {
      g_strPort = devices[0].strComName;
    } else {
      g_strPort = "";
    }
}

void CecKeyPress(void *UNUSED(cbParam), const cec_keypress* key)
{
  if(key->duration != 0) {
    return;
  }

  std::string basedir = "";
  if(g_iniconfig->getSection("Main") != NULL) {
    basedir = g_iniconfig->getSection("Main")->value("irbase");
  }

  if (basedir.length() == 0) {
    std::cerr << "Required irbase parameter missing\n";
    return;
  }

  timeval tv;
  gettimeofday(&tv, NULL);

  uint64_t timenow = (tv.tv_sec * 1000) + (tv.tv_usec / 1000UL);

  if(g_keyRepeat.keycode == key->keycode) {
    uint64_t diff = timenow - g_keyRepeat.lastpress;

    int repeatDelay = (g_keyRepeat.repeatstart == 0) ? 850 : 50;
    if (g_keyRepeat.repeatstart == 0) {
      if (g_iniconfig->contains("Main", "repeatdelay")) {
        repeatDelay = g_iniconfig->getSection("Main")->intValue("repeatdelay");
      }
    } else if (g_iniconfig->contains("Main", "repeatrate")) {
      repeatDelay = g_iniconfig->getSection("Main")->intValue("repeatrate");
    }

    if(diff < repeatDelay) {
      return;
    }

    std::cerr << "Key repeat " <<  key->keycode << ", diff " << diff << ", delay " << repeatDelay << "\n";

    if (g_keyRepeat.repeatstart == 0) {
      g_keyRepeat.repeatstart = timenow;
    }
  }

  g_keyRepeat.keycode = key->keycode;
  g_keyRepeat.lastpress = timenow;

  std::cerr << "Key " << key->keycode << "\n";

  if(g_iniconfig->getSection("Keys") != NULL) {
    std::string cmd = g_iniconfig->getSection("Keys")->value(std::to_string(key->keycode));
    if(cmd.size() > 0) {
      irSend(basedir + "/" + cmd);
    }
  }
}

void CecCommand(void *UNUSED(cbParam), const cec_command* command)
{
  switch(command->opcode) {
  case CEC_OPCODE_USER_CONTROL_PRESSED:
    if(command->parameters.size > 0) {
      cec_keypress key = {(cec_user_control_code) command->parameters.data[0], 0};
      CecKeyPress(nullptr, &key);
    }

    break;
  case CEC_OPCODE_USER_CONTROL_RELEASE:
    memset(&g_keyRepeat, 0, sizeof(KeyRepeat));
    g_keyRepeat.keycode = CEC_USER_CONTROL_CODE_UNKNOWN;
    break;
  }
}

void CecAlert(void *UNUSED(cbParam), const libcec_alert type, const libcec_parameter UNUSED(param))
{
  switch (type)
  {
  case CEC_ALERT_CONNECTION_LOST:
    if (!CReconnect::Get().IsRunning())
    {
      std::cerr << "Connection lost - trying to reconnect\n";
      CReconnect::Get().CreateThread(false);
    }
    break;
  default:
    break;
  }
}

void sighandler(int iSignal)
{
  std::cerr << "signal caught: " << iSignal << " - exiting\n";
  g_bExit = true;
  g_bHardExit = true;
}

int main (int argc, char *argv[])
{
  if (signal(SIGINT, sighandler) == SIG_ERR)
  {
    std::cerr << "can't register sighandler\n";
    return -1;
  }

  bool ok = false;
  g_iniconfig = new HueConfig("/etc/cec-forwarder");
  g_iniconfig->parse(ok);

  memset(&g_keyRepeat, 0, sizeof(KeyRepeat));
  g_keyRepeat.keycode = CEC_USER_CONTROL_CODE_UNKNOWN;

  g_config.Clear();
  g_callbacks.Clear();
  snprintf(g_config.strDeviceName, 13, "CECForwarder");
  g_config.clientVersion      = LIBCEC_VERSION_CURRENT;
  g_config.bActivateSource    = 0;
  g_callbacks.keyPress        = &CecKeyPress;
  g_callbacks.commandReceived = &CecCommand;
  g_callbacks.alert           = &CecAlert;
  g_config.callbacks          = &g_callbacks;

  g_config.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);

  g_parser = LibCecInitialise(&g_config);
  if (!g_parser)
  {
    if (g_parser) {
      UnloadLibCec(g_parser);
    }

    return 1;
  }

  // init video on targets that need this
  g_parser->InitVideoStandalone();

  while (!g_bHardExit) {
    cec_adapter_descriptor devices[10];
    uint8_t iDevicesFound = g_parser->DetectAdapters(devices, 10, NULL, true);
    if (iDevicesFound > 0)
    {
      g_strPort = devices[0].strComName;
    } else {
      g_strPort = "";
    }

    if(!g_strPort.empty()) {
      std::cerr << "opening a connection to the CEC adapter at " << g_strPort.c_str() << "...\n";
      if (!g_parser->Open(g_strPort.c_str()))
      {
        std::cerr << "unable to open the device on port " << g_strPort.c_str() << "!\n";
        UnloadLibCec(g_parser);
        return 1;
      }

      while (!g_bHardExit && !g_bExit)
      {
        CEvent::Sleep(1);
      }

      g_bExit = false;
    }

    CEvent::Sleep(1000);
  }

  g_parser->Close();
  UnloadLibCec(g_parser);

  delete g_iniconfig;

  return (g_bHardExit) ? -1 : 0;
}
