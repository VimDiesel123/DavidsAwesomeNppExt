#include "FixSetupTreeItems.h"
#include <Windows.h>
#include <regex>
#include "../PluginDefinition.h"

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


