//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include <iostream>
#include <string>
#include <regex>
#include <map>

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Fix SetupTree Items"), fixSetupTreeItems, NULL, 0);
    setCommand(1, TEXT("GIVE FUNNY"), showFunny, NULL, 0);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void fixSetupTreeItems() 
{
    // Get the current scintilla
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);


    if (which == -1)
        return;
    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;


    // Get the length of the current document.
    const unsigned length = ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);
    const std::string test("Length: " + std::to_string(length));

    ::MessageBoxA(NULL, test.c_str(), "TEST", MB_OKCANCEL);

    std::string treeItems;
    treeItems.resize(length + 1);
    ::SendMessage(curScintilla, SCI_GETTEXT, length + 1, (LPARAM)treeItems.data());

    // Define the regex pattern to match tags like <P1=...> within sections
    const std::regex pattern("\\[([^\\]]+)\\]|(<P\\d+=)(.*?)\r\n|([^\\[<]+)");

    // Create a regex iterator to iterate through matches
    std::sregex_iterator it(treeItems.begin(), treeItems.end(), pattern);
    std::sregex_iterator end;

    std::string result;
    int tagNumber = 1;

    // Iterate through matches
    while (it != end) {
        std::smatch match = *it;

        if (match[1].matched) {
            // Matched a section header, reset the tag number
            tagNumber = 1;
            result += match.str();
        }
        else if (match[2].matched) {
            // Matched the beginning of a tag, update the tag number
            const std::string tagContent = match[3].str();
            result += "<P" + std::to_string(tagNumber++) + "=" + tagContent + "\r\n";
        }
        else if (match[4].matched) {
            // Matched text that is not a header or a tag, such as blank lines
            result += match[4].str();
        }

        ++it;
    }

    
    ::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)result.c_str());
}

void showFunny()
{
    ::MessageBoxA(NULL, "This is a joke.", "FUNNY", MB_OK);
}

