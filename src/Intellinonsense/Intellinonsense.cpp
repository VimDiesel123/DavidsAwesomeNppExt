#include "Intellinonsense.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <regex>
#include <string>

#include "../../lib/json.hpp"
#include "../../lib/tabulate.hpp"
#include "../SCI_Utils.h"
#include "../StringUtils.h"

Calltip currentCalltip;
size_t currentArgumentNumber;

// JSON data with commands
nlohmann::json commandData;

std::map<std::string, Calltip> labelCalltips;

void loadManualData() {
  std::ifstream f(PATH_TO_MANUAL_DATA);
  f >> commandData;
}

std::string extractCommand(const std::string& word) {
  auto commandRegex =
      std::regex("(?:^|[;=])_?(@?[A-Z]{2})(?:[A-HW-Z0-9]+=?|#|$)");
  std::smatch match;
  const auto matched = std::regex_search(word, match, commandRegex);
  return matched ? match[1].str() : "";
}

std::string stringFromCommandEntry(const nlohmann::json& entry) {
  std::string result;
  // Extract the "Command" and "Description" fields
  result += entry["Command"].get<std::string>() + "\n";
  result += "Description:\n" + entry["Description"].get<std::string>() + "\n";

  tabulate::Table table;

  // Extract the "Usage" array
  if (!entry["Usage"].empty()) {
    for (const auto& usageEntry : entry["Usage"]) {
      table.add_row({"Usage", usageEntry["Example"].get<std::string>(),
                     usageEntry["Explanation"].get<std::string>()});
    }
  }

  // Extract the "Operands" object
  if (!entry["Operands"].empty()) {
    std::string operands;
    for (const std::string operand : entry["Operands"]["Operands"]) {
      operands += operand;
    }
    table.add_row({"Operands", operands,
                   entry["Operands"]["Explanation"].get<std::string>()});
  }

  result += table.str() + "\n";

  // Extract the "Arguments" array
  if (!entry["Arguments"].empty()) {
    result += "Arguments:\n";
    tabulate::Table argumentsTable;
    argumentsTable.add_row({"Argument", "Description"});
    argumentsTable[0].format().font_style({tabulate::FontStyle::bold});
    for (const auto& argEntry : entry["Arguments"]) {
      argumentsTable.add_row({argEntry["Argument"].get<std::string>(),
                              argEntry["Description"].get<std::string>()});
    }
    result += argumentsTable.str();
  }

  return result;
}

std::string cleanLabelDescription(const std::string& rawDescription) {
  return std::regex_replace(rawDescription, std::regex("(^REM)"), "");
}

std::string buildLabelCalltip(const std::string& label) {
  // Try to look the calltip up in the labelCalltips map, if you find it, return
  // the description.
  const auto lines = labelCalltips.at(label).description;
  return label + ":\n" + cleanLabelDescription(linesToString(lines));
}

std::string buildCommandCalltip(const std::string& command) {
  for (const auto& entry : commandData) {
    if (entry["Command"] == command) {
      return stringFromCommandEntry(entry);
    }
  }
  return "";
}

bool isArgumentLine(std::string remark) {
  std::regex argumentPattern(".*\\^[a-h].*");
  std::smatch m;
  return std::regex_match(remark, m, argumentPattern);
}

std::vector<size_t> argumentLinePositions(std::vector<std::string> lines) {
  std::vector<size_t> argumentLines;
  for (size_t i = 0; i < lines.size(); ++i)
    if (isArgumentLine(lines[i])) argumentLines.push_back(i);
  return argumentLines;
}

std::vector<std::string> extractLabelDescription(
    const std::vector<std::string>& lines, const size_t labelIndex) {
  // NOTE: (David) this is a little confusing, but the line at labelIndex is the
  // END of our doc comment and the first line we find looking backwards that
  // starts with "REM", is the START of our doc comment,
  const auto end = std::reverse_iterator(lines.begin() + labelIndex);
  const auto start = std::find_if_not(end, lines.rend(), [](const auto& line) {
    return startsWith(line, "REM");
  });
  return std::vector<std::string>(start.base(), end.base());
}

bool isLabelDecleration(std::string line) { return startsWith(line, "#"); }

std::map<std::string, Calltip> buildLabelLookupTable(
    const std::vector<std::string>& lines) {
  std::map<std::string, Calltip> result;

  for (size_t i = 0; i < lines.size(); ++i) {
    const auto& currentLine = lines[i];

    if (!isLabelDecleration(currentLine)) continue;

    const auto labelDescription = extractLabelDescription(lines, i);
    const auto argLines = argumentLinePositions(labelDescription);
    Calltip calltip = {labelDescription, argLines};

    const std::regex labelPattern("^#(\\w+)($|;)");
    std::smatch match;
    std::regex_search(currentLine, match, labelPattern);
    result.emplace(std::pair<std::string, Calltip>({match[1].str(), calltip}));
  }
  return result;
}

std::map<std::string, Calltip> parseDocument() {
  auto rawCode = getDocumentText();
  const auto filteredCode = std::regex_replace(
      rawCode, std::regex("\r"),
      "");  // Get rid of \r because it's needlessly complicated
  const auto lines = split(filteredCode, '\n');
  return buildLabelLookupTable(lines);
}

std::pair<size_t, size_t> argumentLineRange(
    std::string calltip, std::vector<size_t> argumentLineNums,
    const size_t currentArgument) {
  if (argumentLineNums.empty()) return {0, 0};
  const auto current = argumentLineNums.begin() + currentArgument;
  const auto last = argumentLineNums.end();

  // TODO: (David) This is way uglier than it needs to be.
  const auto startOfFirstLine = startOfNthLine(calltip, *current);
  auto endOfLastLine =
      std::next(current) < last
          ? startOfNthLine(calltip, *std::next(current))
          : find_nth(calltip, startOfFirstLine, "\n\n",
                     1);  // We are either going to highlight to the next blank
                          // line OR to the end of the calltip if there isn't
                          // another blank line
  if (endOfLastLine == std::string::npos) endOfLastLine = calltip.length();
  return {startOfFirstLine, endOfLastLine};
}

void updateCurrentCalltip() {
  const auto prevWord = wordAt(currentPosition() - 1);
  currentArgumentNumber = 0;
  currentCalltip = labelCalltips[prevWord];
}

void incrementArgumentLineNumber() {
  if (currentArgumentNumber == currentCalltip.argumentLineNums.size() - 1)
    return;
  currentArgumentNumber++;
}

void decrementArgumentLineNumber() {
  if (currentArgumentNumber == 0) return;
  currentArgumentNumber--;
}

void generateLabelCalltip() {
  const auto callTipString =
      cleanLabelDescription(linesToString(currentCalltip.description));
  displayCallTip(
      callTipString, currentPosition(),
      argumentLineRange(callTipString, currentCalltip.argumentLineNums,
                        currentArgumentNumber));
}

void updateArgumentHighlight() {
  const auto callTipString =
      cleanLabelDescription(linesToString(currentCalltip.description));
  const auto argumentRange = argumentLineRange(
      callTipString, currentCalltip.argumentLineNums, currentArgumentNumber);
  setCallTipHighlightRange(argumentRange);
}

void cancelLabelCallTip() {}

void onCharacterAdded(SCNotification* pNotify) {
  switch ((char)pNotify->ch) {
    case ('('): {
      updateCurrentCalltip();
      generateLabelCalltip();
      break;
    }
    case (','): {
      incrementArgumentLineNumber();
      updateArgumentHighlight();
      break;
    }
    case (')'): {
      cancelLabelCallTip();
      break;
    }
    default:
      return;
  }
}

void onTextDeleted(SCNotification* pNotify) {
  const auto textDeleted = std::string(pNotify->text);
  if (textDeleted.find(",") != std::string::npos) {
    decrementArgumentLineNumber();
    updateArgumentHighlight();
  }
}

void onDwellStart(SCNotification* pNotify) {
  labelCalltips = parseDocument();
  const HWND handle = (HWND)pNotify->nmhdr.hwndFrom;
  int wordStart =
      ::SendMessage(handle, SCI_WORDSTARTPOSITION, pNotify->position, true);
  const auto prevChar = charAt(wordStart - 1);
  const auto label = prevChar == '#';

  const auto word = wordAt(pNotify->position);
  const auto calltip = label ? buildLabelCalltip(word)
                             : buildCommandCalltip(extractCommand(word));

  const auto endOfFirstLine = calltip.find_first_of('\n');
  displayCallTip(calltip, pNotify->position, {0, endOfFirstLine});
}

void onDwellEnd(SCNotification* pNotify) {
  ::SendMessage((HWND)pNotify->nmhdr.hwndFrom, SCI_CALLTIPCANCEL, 0, 0);
}
