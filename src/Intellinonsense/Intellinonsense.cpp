#include <string>
#include <regex>
#include <map>
#include <fstream>
#include "../../lib/json.hpp"
#include "../../lib/tabulate.hpp"
#include "../PluginDefinition.h"
#include "Intellinonsense.h"

Calltip currentCalltip;
size_t currentArgumentNumber;

// JSON data with commands
nlohmann::json commandData;

std::map<std::string, Calltip> labelCalltips;

extern NppData nppData;

void loadManualData() {
	std::ifstream f(PATH_TO_MANUAL_DATA);
	f >> commandData;
}
std::string extractCommand(const std::string& word) {
	auto commandRegex = std::regex("(?:^|[;=])_?(@?[A-Z]{2})(?:[A-HW-Z0-9]+=?|#|$)");
	std::smatch match;
	const auto matched = std::regex_search(word, match, commandRegex);
	return matched ? match[1].str() : "";
	}
	return match[1];
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


std::string buildCallTipString(const std::string& word) {
	try {
		return labelCalltips.at(word).description == "" ? "" : word + "\n" + labelCalltips.at(word).description;
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

std::vector<Argument> extractArguments(std::string rawDescription) {
	std::vector<Argument> result;
	std::regex argumentPattern(".*\\^[a-h].*");
	std::smatch match;
	const auto lines = toLines(std::istringstream(rawDescription));
	for (size_t i = 0; i < lines.size(); ++i) {
		const auto line = lines[i];
		if (std::regex_match(lines[i], match, argumentPattern)) {
			Argument argument;
			argument.startLine = i;
			if (!result.empty()) {
				result[result.size() - 1].endLine = i - 1;
			}
			result.push_back(argument);
		}
	}
	if (!result.empty()) {
		result[result.size() - 1].endLine = lines.size() - 1;
	}
	return result;
	void loadManualData();

}


std::map<std::string, Calltip> extractLabelDetails(const std::vector<std::string>& lines) {
	std::map<std::string, Calltip> result;
	for (size_t i = 0; i < lines.size(); ++i) {
		const auto& currentLine = lines[i];
		if (startsWith(currentLine, "#")) {
			const std::regex labelPattern("^#(\\w+)($|;)");
			std::smatch match;
			std::regex_search(currentLine, match, labelPattern);
			const auto labelDescription = cleanLabelDescription(extractLabelDescription(lines, i));
			const auto arguments = extractArguments(labelDescription);
			Calltip calltip = { labelDescription, arguments };
			result.emplace(std::pair<std::string, Calltip>({ match[1].str(), calltip }));
		}
	}
	return result;
}


std::map<std::string, Calltip> parseLabels() {
	std::map<std::string, Calltip> result;
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


void onDwellStart(SCNotification* pNotify) {
	labelCalltips = parseLabels();
	const HWND handle = (HWND)pNotify->nmhdr.hwndFrom;
	int wordStart = ::SendMessage(handle, SCI_WORDSTARTPOSITION, pNotify->position, true);

	const auto word = wordAt(pNotify->position);

	const auto prevChar = charAt(wordStart - 1);
	const auto label = prevChar == '#';

	const auto calltip = label ? buildCallTipString(word) : buildCallTipString(extractCommand(word));

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
	if (currentArgumentNumber == currentCalltip.arguments.size() - 1) return;
	currentArgumentNumber++;
	displayCallTip();
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
	currentArgumentNumber = 0;
	currentCalltip = labelCalltips[prevWord];
	displayCallTip();

}

size_t find_nth(const std::string& haystack, size_t pos, const std::string& needle, size_t nth)
{
	size_t found_pos = haystack.find(needle, pos);
	if (0 == nth || std::string::npos == found_pos)  return found_pos;
	return find_nth(haystack, found_pos + 1, needle, nth - 1);
}

void displayCallTip() {
	const auto currentPos = currentPosition();
	const auto curScintilla = currentScintilla();
	::SendMessage(curScintilla, SCI_CALLTIPSETBACK, RGB(68, 70, 84), 0);
	::SendMessage(curScintilla, SCI_CALLTIPSETFORE, RGB(209, 213, 219), 0);
	::SendMessage(curScintilla, SCI_CALLTIPUSESTYLE, 0, 0);
	::SendMessage(curScintilla, SCI_CALLTIPSHOW, currentPos, (LPARAM)currentCalltip.description.c_str());
	::SendMessage(curScintilla, SCI_CALLTIPSETFOREHLT, RGB(3, 128, 226), 0);
	std::size_t startOfFirstLine, endOfLastLine;
	if (currentCalltip.arguments.empty()) {
		startOfFirstLine = 0;
		endOfLastLine = find_nth(currentCalltip.description, 0, "\n", 1);
	}
	else {
		startOfFirstLine = find_nth(currentCalltip.description, 0, "\n", currentCalltip.arguments[currentArgumentNumber].startLine - 1);
		endOfLastLine = find_nth(currentCalltip.description, 0, "\n", currentCalltip.arguments[currentArgumentNumber].endLine);
	}
	::SendMessage(curScintilla, SCI_CALLTIPSETHLT, startOfFirstLine, endOfLastLine);
}

std::vector<std::string> toLines(std::istringstream rawCode) {
	std::vector<std::string> result;
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

std::vector<std::string> split(const std::string& string, char delim) {
	std::vector<std::string> result;
	std::stringstream ss(string);
	std::string line;
	while (std::getline(ss, line, delim)) result.push_back(line);
	return result;
}

std::string cleanLabelDescription(const std::string& rawDescription) {
	return std::regex_replace(rawDescription, std::regex("(^REM)"), "");
}

std::string extractLabelDescription(const std::vector<std::string>& lines, const size_t labelIndex) {
	std::vector<std::string> descriptionLines;
	auto start = labelIndex - 1;
	if (labelIndex == 0) return "";
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

