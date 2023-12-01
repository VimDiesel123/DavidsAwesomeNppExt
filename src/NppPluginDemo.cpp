// this file is part of notepad++
// Copyright (C)2022 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "Intellinonsense/Intellinonsense.h"
#include "PluginDefinition.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall,
                      LPVOID /*lpReserved*/) {
  try {
    switch (reasonForCall) {
      case DLL_PROCESS_ATTACH:
        pluginInit(hModule);
        break;

      case DLL_PROCESS_DETACH:
        pluginCleanUp();
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }
  } catch (...) {
    return FALSE;
  }

  return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData) {
  nppData = notpadPlusData;
  commandMenuInit();
  // callTipInit();
}

extern "C" __declspec(dllexport) const TCHAR *getName() {
  return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF) {
  *nbF = nbFunc;
  return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
  switch (notifyCode->nmhdr.code) {
    case NPPN_SHUTDOWN: {
      return commandMenuCleanUp();
    }
    case SCN_DWELLSTART: {
      return onDwellStart(notifyCode);
    }
    case SCN_DWELLEND: {
      return onDwellEnd(notifyCode);
    }
    case NPPN_READY: {
      return callTipInit();
    }
    case SCN_CHARADDED: {
      return onCharacterAdded(notifyCode);
    }
    case SCN_MODIFIED: {
      // TODO: (David) Idk where SETMODEVENTMASK is meant to be called.
      // Everything I tried didn't work, so I'm doing it here. Which is dumb.

      // TODO: (David) Also, this is completely ridiculous. Why does SCI have a
      // CHARADDED event and not a CHARDELETED event? This is just a mess.
      if (!(notifyCode->modificationType & SC_MOD_DELETETEXT)) {
        ::SendMessage(nppData._scintillaMainHandle, SCI_SETMODEVENTMASK,
                      SC_MOD_DELETETEXT, 0);
        return;
      }
      assert(notifyCode->modificationType & SC_MOD_DELETETEXT);
      return onTextDeleted(notifyCode);
    } break;

    default:
      return;
  }
}

// Here you can process the Npp Messages
// I will make the messages accessible little by little, according to the need
// of plugin development. Please let me know if you need to access to some
// messages : http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(
    UINT /*Message*/, WPARAM /*wParam*/,
    LPARAM /*lParam*/) { /*
                                if (Message == WM_MOVE)
                                {
                                        ::MessageBox(NULL, "move", "", MB_OK);
                                }
                        */
  return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() { return TRUE; }
#endif  // UNICODE
