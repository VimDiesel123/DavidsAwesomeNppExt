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
#include <Windows.h>
#include <wininet.h>
#include <fstream>
#include <sstream>
#include "../lib/tabulate.hpp"
#include <thread>

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

// JSON data with commands
nlohmann::json commandData;

const std::string PATH_TO_MANUAL_DATA = "plugins\\DavidsAwesomeTools\\manual.json";

std::map<std::string, std::string> labelCalltips;

struct CurrentCalltipInfo {
	int argumentNumber;
};

CurrentCalltipInfo currentCallTipInfo;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
	std::ifstream f(PATH_TO_MANUAL_DATA);
	f >> commandData;
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

void callTipInit()
{
	// Set the dwell time (in milliseconds) and the styler for dwell events
	::SendMessage(nppData._scintillaMainHandle, SCI_SETMOUSEDWELLTIME, 1000, 0); // Set a 1-second dwell time

	// labelCalltips = parseLabels();
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk, bool check0nInit)
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

	// Get filename of current document
	std::wstring filename;
	filename.resize(MAX_PATH);
	::SendMessage(nppData._nppHandle, NPPM_GETFILENAME, MAX_PATH, (LPARAM)filename.data());

	const std::wregex treeItemsPattern(L"TreeItems.*\\.ini.*");

	const auto isTreeItemsFile = std::regex_match(filename.c_str(), treeItemsPattern);

	if (!isTreeItemsFile) {
		std::wstring errormessage = std::wstring(filename.c_str()) + std::wstring(L" is not a TreeItems file.");
		::MessageBox(NULL, errormessage.c_str(), L"Wrong file!", MB_OK);
		return;
	}

	// Get the length of the current document.
	const unsigned length = ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);

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
	HINTERNET hInternet = InternetOpen(L"David's Awesome Tools", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (hInternet == NULL) {
		OutputDebugStringA("Failed to intialize internet handle.");
		handleError();
		return;
	}

	HINTERNET hRequest = InternetOpenUrl(hInternet, L"https://v2.jokeapi.dev/joke/Any?blacklistFlags=nsfw,religious,political,racist,sexist,explicit", NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (hRequest == NULL) {
		OutputDebugStringA("Failed to create request.");
		handleError();
		return;
	}

	DWORD bytesRead;
	char buffer[1024];
	InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead);
	buffer[bytesRead] = '\0';
	const auto JSONResponse = std::string(buffer);

	// Parse the JSON response using JSON for Modern C++
	try {
		nlohmann::json jsonData = nlohmann::json::parse(JSONResponse);

		std::string category = jsonData["category"];
		std::string type = jsonData["type"];
		std::string joke = type == "twopart" ? std::string(jsonData["setup"]) + "\n\n" + std::string(jsonData["delivery"]) : jsonData["joke"];

		::MessageBoxA(NULL, joke.c_str(), "FUNNY", MB_OK);

	}
	catch (const nlohmann::json::parse_error& e) {
		OutputDebugStringA(e.what());
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hInternet);

}


std::string extractCommand(const std::string& word) {
	auto commandRegex = std::regex("(?:^|[;=])_?(@?[A-Z]{2})(?:[A-HW-Z0-9]+=?|#|$)");
	std::smatch match;
	std::regex_search(word, match, commandRegex);
	if (!match.ready()) {
		return "";
	}
	return match[1];
}

std::string buildCallTip(const std::string& word) {
	try {
		return labelCalltips.at(word) == "" ? "" : word + "\n" + labelCalltips.at(word);
	}
	catch (std::out_of_range)
	{
		for (const auto& entry : commandData) {
			if (entry["Command"] == word) {
				return
					extractDataFromJson(entry);
			}
		}
	}

	return "";
}

std::string extractDataFromJson(const nlohmann::json& entry) {
	std::string result;

	// Extract the "Command" and "Description" fields
	result += entry["Command"].get<std::string>() + "\n";
	result += "Description:\n" + entry["Description"].get<std::string>() + "\n";


	tabulate::Table usageAndArgumentsTable;

	// Extract the "Usage" array
	if (!entry["Usage"].empty()) {
		for (const auto& usageEntry : entry["Usage"]) {
			usageAndArgumentsTable.add_row({ "Usage", usageEntry["Example"].get<std::string>(), usageEntry["Explanation"].get<std::string>() });
		}
	}

	// Extract the "Operands" object
	if (!entry["Operands"].empty()) {
		std::string operands;
		for (const std::string operand : entry["Operands"]["Operands"]) {
			operands += operand;
		}
		usageAndArgumentsTable.add_row({ "Operands", operands, entry["Operands"]["Explanation"].get<std::string>() });
	}

	result += usageAndArgumentsTable.str() + "\n";

	// Extract the "Arguments" array
	if (!entry["Arguments"].empty()) {
		result += "Arguments:\n";
		tabulate::Table argumentsTable;
		argumentsTable.add_row({ "Argument", "Description" });
		argumentsTable[0].format().font_style({ tabulate::FontStyle::bold });
		for (const auto& argEntry : entry["Arguments"]) {
			argumentsTable.add_row({ argEntry["Argument"].get<std::string>(), argEntry["Description"].get<std::string>() });
		}
		result += argumentsTable.str();
	}

	return result;
}

void onDwellStart(SCNotification* pNotify) {
	labelCalltips = parseLabels();
	const HWND handle = (HWND)pNotify->nmhdr.hwndFrom;
	int wordStart = ::SendMessage(handle, SCI_WORDSTARTPOSITION, pNotify->position, true);

	const auto word = wordAt(pNotify->position);

	const auto prevChar = charAt(wordStart - 1);
	const auto label = prevChar == '#';

	const auto calltip = label ? buildCallTip(word) : buildCallTip(extractCommand(word));

	::SendMessage(handle, SCI_CALLTIPSETBACK, RGB(68, 70, 84), 0);
	::SendMessage(handle, SCI_CALLTIPSETFORE, RGB(209, 213, 219), 0);
	::SendMessage(handle, SCI_CALLTIPUSESTYLE, 0, 0);
	::SendMessage(handle, SCI_CALLTIPSHOW, pNotify->position, (LPARAM)calltip.c_str());
	::SendMessage(handle, SCI_CALLTIPSETFOREHLT, RGB(3, 128, 226), 0);
	const auto endOfFirstLine = calltip.find_first_of('\n');
	::SendMessage(handle, SCI_CALLTIPSETHLT, 0, endOfFirstLine);

}


void onDwellEnd(SCNotification* pNotify) {
	::SendMessage((HWND)pNotify->nmhdr.hwndFrom, SCI_CALLTIPCANCEL, 0, 0);
}

void onCharacterAdded(SCNotification* pNotify) {
	switch ((char)pNotify->ch)
	{
	case ('('):
	{
		showLabelCallTip();
		break;
	}
	case (','):
	{
		incrementArgumentLineNumber();
		break;
	}
	case (')'):
	{
		cancelLabelCallTip();
		break;
	}
	default:
		return;
	}
}

void incrementArgumentLineNumber() {

}

void cancelLabelCallTip() {

}

HWND currentScintilla() {
	int which = -1;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);


	if (which == -1)
		return NULL;
	return (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

int currentPosition() 
{
	const auto curScintilla = currentScintilla();
	return ::SendMessage(curScintilla, SCI_GETCURRENTPOS, 0, 0);
}

std::string wordAt(int position) {
	const auto curScintilla = currentScintilla();

	int wordStart = ::SendMessage(curScintilla, SCI_WORDSTARTPOSITION, position, true);
	int wordEnd = ::SendMessage(curScintilla, SCI_WORDENDPOSITION, position, true);


	char word[256];
	Sci_TextRange tr = {
		{ wordStart, wordEnd },
		word
	};

	const auto result = ::SendMessage(curScintilla, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

	if (!result || result > 256) {
		return "";
	}

	return word;
}

char charAt(int position) {
	const auto curScintilla = currentScintilla();
	return (char)::SendMessage(curScintilla, SCI_GETCHARAT, position, 0);

}

void showLabelCallTip() {
	
	const auto currentPos = currentPosition();
	const auto prevWord = wordAt(currentPos - 1);
	const auto calltip = buildCallTip(prevWord);
	displayCallTip(calltip, 3);

}

size_t find_nth(const std::string& haystack, size_t pos, const std::string& needle, size_t nth)
{
	size_t found_pos = haystack.find(needle, pos);
	if (0 == nth || std::string::npos == found_pos)  return found_pos;
	return find_nth(haystack, found_pos + 1, needle, nth - 1);
}

void displayCallTip(std::string calltip, int highlightLine) {
	(void)highlightLine;
	const auto currentPos = currentPosition();
	const auto curScintilla = currentScintilla();
	::SendMessage(curScintilla, SCI_CALLTIPSETBACK, RGB(68, 70, 84), 0);
	::SendMessage(curScintilla, SCI_CALLTIPSETFORE, RGB(209, 213, 219), 0);
	::SendMessage(curScintilla, SCI_CALLTIPUSESTYLE, 0, 0);
	::SendMessage(curScintilla, SCI_CALLTIPSHOW, currentPos, (LPARAM)calltip.c_str());
	::SendMessage(curScintilla, SCI_CALLTIPSETFOREHLT, RGB(3, 128, 226), 0);
	const auto startOfHighlight = find_nth(calltip, 0, "\n", highlightLine - 1);
	const auto endOfHighlight = find_nth(calltip, 0, "\n", highlightLine);
	::SendMessage(curScintilla, SCI_CALLTIPSETHLT, startOfHighlight, endOfHighlight);
}

std::map<std::string, std::string> parseLabels() {
	std::map<std::string, std::string> result;
	const auto curScintilla = currentScintilla();

	// Get the length of the current document.
	const unsigned length = ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);

	// Store text of document in dmcCode
	std::string dmcCode;
	dmcCode.resize(length + 1);
	::SendMessage(curScintilla, SCI_GETTEXT, length, (LPARAM)dmcCode.data());

	const auto filteredLines = toLines(std::istringstream(dmcCode));
	const auto labelDetails = extractLabelDetails(filteredLines);
	return labelDetails;


}

std::vector<std::string> toLines(std::istringstream rawCode) {
	std::vector<std::string> result(1024 * 20);
	std::string line;
	while (std::getline(rawCode, line, '\n')) {
		// Remove any trailing \r if it exists
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		result.push_back(line);
	}
	return result;
}

std::map<std::string, std::string> extractLabelDetails(const std::vector<std::string>& lines) {
	std::map<std::string, std::string> result;
	for (size_t i = 0; i < lines.size(); ++i) {
		const auto& currentLine = lines[i];
		if (startsWith(currentLine, "#")) {
			const std::regex labelPattern("^#(\\w+)$");
			std::smatch match;
			std::regex_search(currentLine, match, labelPattern);
			const auto labelDescription = cleanLabelDescription(extractLabelDescription(lines, i));
			result.emplace(std::pair<std::string, std::string>({ match[1].str(), labelDescription }));
		}
	}
	return result;
}

std::string cleanLabelDescription(const std::string& rawDescription) {
	return std::regex_replace(rawDescription, std::regex("(^REM\\s*)"), "");
}

std::string extractLabelDescription(const std::vector<std::string>& lines, const size_t labelIndex) {
	std::vector<std::string> descriptionLines;
	auto start = labelIndex - 1;
	while (startsWith(lines[start], "REM")) {
		descriptionLines.push_back(lines[start]);
		--start;
	}
	std::string result;
	std::for_each(descriptionLines.rbegin(), descriptionLines.rend(), [&result](const std::string& line) { result += line + "\r\n"; });
	return result;
}

bool startsWith(const std::string& bigString, const std::string& smallString) {
	return bigString.compare(0, smallString.length(), smallString) == 0;
}

void handleError()
{
	const auto errorCode = GetLastError();
	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	::MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK);
}

