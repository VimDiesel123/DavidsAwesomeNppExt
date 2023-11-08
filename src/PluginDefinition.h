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

#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

//
// All difinitions of plugin interface
//
#include "PluginInterface.h"
#include <string>
#include "../lib/json.hpp"

//-------------------------------------//
//-- STEP 1. DEFINE YOUR PLUGIN NAME --//
//-------------------------------------//
// Here define your plugin name
//
const TCHAR NPP_PLUGIN_NAME[] = TEXT("David's Awesome Tools");

//-----------------------------------------------//
//-- STEP 2. DEFINE YOUR PLUGIN COMMAND NUMBER --//
//-----------------------------------------------//
//
// Here define the number of your plugin commands
//
const int nbFunc = 2;


//
// Initialization of your plugin data
// It will be called while plugin loading
//
void pluginInit(HANDLE hModule);

//
// Cleaning of your plugin
// It will be called while plugin unloading
//
void pluginCleanUp();

//
//Initialization of your plugin commands
//
void commandMenuInit();

//
//Initialization of calltip extionsion
//
void callTipInit();

//
//Clean up your plugin commands allocation (if any)
//
void commandMenuCleanUp();

//
// Function which sets your command 
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);


//
// Your plugin command functions
//
void fixSetupTreeItems();
void showFunny();

void onDwellStart(SCNotification* pNotify);
void onDwellEnd(SCNotification* pNotify);
std::string buildCallTip(const std::string& word);
std::string extractCommand(const std::string& word);
std::string extractDataFromJson(const nlohmann::json& entry);
int lighten_color(int color);
std::vector<std::string> toLines(std::istringstream rawCode);
std::map<std::string, std::string> extractLabelDetails(const std::vector<std::string>& lines);
bool startsWith(const std::string& bigString, const std::string& smallString);
std::string extractLabelDescription(const std::vector<std::string>& lines, const size_t labelIndex);
std::map<std::string, std::string> parseLabels();

void handleError();


#endif //PLUGINDEFINITION_H
