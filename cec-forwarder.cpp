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

static void PrintToStdOut(const char *strFormat, ...);

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
      while (!g_parser->Open(g_strPort.c_str()))
      {
        PrintToStdOut("Failed to reconnect, retrying in a bit\n");
        sleep(500);
      }
    }
    return NULL;
  }

private:
  CReconnect(void) {}
};

static void PrintToStdOut(const char *strFormat, ...)
{
  std::string strLog;

  va_list argList;
  va_start(argList, strFormat);
  strLog = StringUtils::FormatV(strFormat, argList);
  va_end(argList);

  CLockObject lock(g_outputMutex);
  std::cout << strLog << std::endl;
}

void CecLogMessage(void *UNUSED(cbParam), const cec_log_message* message)
{
  //PrintToStdOut("CecLogMessage: %s\n", message->message);
}

void CecKeyPress(void *UNUSED(cbParam), const cec_keypress* key)
{
  //PrintToStdOut("CecKeyPress: key %d, duration %d, last %d\n", key->keycode, key->duration, g_keyRepeat.keycode);
  if(key->duration != 0) {
    return;
  }

  if(g_keyRepeat.keycode == key->keycode) {
    timeval tv;
    gettimeofday(&tv, NULL);
    
    uint64_t timenow = (tv.tv_sec * 1000) + (tv.tv_usec / 1000UL);
    uint64_t diff = timenow - g_keyRepeat.lastpress;

    int repeatDelay = 700;
    if(g_iniconfig->getSection("Main") != NULL) {
      repeatDelay = g_iniconfig->getSection("Main")->intValue("repeatdelay");
    }

    if(diff < repeatDelay) {
      return;
    }
  }

  timeval tv;
  gettimeofday(&tv, NULL);

  g_keyRepeat.keycode = key->keycode;
  g_keyRepeat.lastpress = (tv.tv_sec * 1000) + (tv.tv_usec / 1000UL);

  if(g_iniconfig->getSection("Keys") != NULL) {
    std::string cmd = g_iniconfig->getSection("Keys")->value(std::to_string(key->keycode));
    if(cmd.size() > 0) {
      lircSendOnce(cmd.c_str());
    }
  }

  /*switch(key->keycode) {
  case 0x1:
    lircSendOnce("KEY_UP");
    break;
  case 0x2:
    lircSendOnce("KEY_DOWN");
    break;
  case 0x3:
    lircSendOnce("KEY_LEFT");
    break;
  case 0x4:
    lircSendOnce("KEY_RIGHT");
    break;
  case 0xd:
    lircSendOnce("KEY_BACK");
    break;
  case 0x20:
    lircSendOnce("KEY_0");
    break;
  case 0x21:
    lircSendOnce("KEY_1");
    break;
  case 0x22:
    lircSendOnce("KEY_2");
    break;
  case 0x23:
    lircSendOnce("KEY_3");
    break;
  case 0x24:
    lircSendOnce("KEY_4");
    break;
  case 0x25:
    lircSendOnce("KEY_5");
    break;
  case 0x26:
    lircSendOnce("KEY_6");
    break;
  case 0x27:
    lircSendOnce("KEY_7");
    break;
  case 0x28:
    lircSendOnce("KEY_8");
    break;
  case 0x29:
    lircSendOnce("KEY_9");
    break;
  case 0x30:
    lircSendOnce("KEY_PAGEUP");
    break;
  case 0x31:
    lircSendOnce("KEY_PAGEDOWN");
    break;
  case 0x35:
    lircSendOnce("KEY_INFO");
    break;
  case 0x44:
    lircSendOnce("KEY_PLAYPAUSE");
    break;
  case 0x45:
    lircSendOnce("KEY_STOP");
    break;
  case 0x46:
    lircSendOnce("KEY_PLAYPAUSE");
    break;
  case 0x47:
    lircSendOnce("KEY_RECORD");
    break;
  case 0x48:
    lircSendOnce("KEY_REWIND");
    break;
  case 0x49:
    lircSendOnce("KEY_FASTFORWARD");
    break;
  case 0x51:
    lircSendOnce("KEY_SUBTITLE");
    break;
  case 0x53:
    lircSendOnce("KEY_EPG");
    break;
  case 0x76:
    lircSendOnce("KEY_TEXT");
    break;
  }*/
}

void CecCommand(void *UNUSED(cbParam), const cec_command* command)
{
  //PrintToStdOut("CecLogMessage: %d\n", command->opcode);
  switch(command->opcode) {
  case CEC_OPCODE_USER_CONTROL_PRESSED:
    if(command->parameters.size > 0) {
      cec_keypress key = {(cec_user_control_code) command->parameters.data[0], 0};
      CecKeyPress(nullptr, &key);
    }

    break;
  case CEC_OPCODE_USER_CONTROL_RELEASE:
    g_keyRepeat.keycode = CEC_USER_CONTROL_CODE_UNKNOWN;
    g_keyRepeat.lastpress = 0;
    break;
  }
}

void CecAlert(void *UNUSED(cbParam), const libcec_alert type, const libcec_parameter UNUSED(param))
{
  PrintToStdOut("CecAlert %d\n", type);

  switch (type)
  {
  case CEC_ALERT_CONNECTION_LOST:
    if (!CReconnect::Get().IsRunning())
    {
      PrintToStdOut("Connection lost - trying to reconnect\n");
      CReconnect::Get().CreateThread(false);
    }
    break;
  default:
    break;
  }
}

void sighandler(int iSignal)
{
  PrintToStdOut("signal caught: %d - exiting", iSignal);
  g_bExit = true;
  g_bHardExit = true;
}

int main (int argc, char *argv[])
{
  if (signal(SIGINT, sighandler) == SIG_ERR)
  {
    PrintToStdOut("can't register sighandler");
    return -1;
  }

  bool ok = false;
  g_iniconfig = new HueConfig("/etc/cec-forwarder");
  g_iniconfig->parse(ok);

  g_keyRepeat.keycode = CEC_USER_CONTROL_CODE_UNKNOWN;
  g_keyRepeat.lastpress = 0;

  g_config.Clear();
  g_callbacks.Clear();
  snprintf(g_config.strDeviceName, 13, "CECForwarder");
  g_config.clientVersion      = LIBCEC_VERSION_CURRENT;
  g_config.bActivateSource    = 0;
  g_callbacks.logMessage      = &CecLogMessage;
  g_callbacks.keyPress        = &CecKeyPress;
  g_callbacks.commandReceived = &CecCommand;
  g_callbacks.alert           = &CecAlert;
  g_config.callbacks          = &g_callbacks;

  g_config.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);

  g_parser = LibCecInitialise(&g_config);
  if (!g_parser)
  {
#ifdef __WINDOWS__
    std::cout << "Cannot load cec.dll" << std::endl;
#else
    std::cout << "Cannot load libcec.so" << std::endl;
#endif

    if (g_parser)
      UnloadLibCec(g_parser);

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
      PrintToStdOut("opening a connection to the CEC adapter %s...", g_strPort.c_str());
      if (!g_parser->Open(g_strPort.c_str()))
      {
        PrintToStdOut("unable to open the device on port %s", g_strPort.c_str());
        UnloadLibCec(g_parser);
        return 1;
      }

      while (!g_bHardExit && !g_bExit)
      {
        CEvent::Sleep(50);
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
