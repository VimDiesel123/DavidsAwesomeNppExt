#include "Intellinonsense.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <regex>
#include <string>

#include "../../lib/json.hpp"
#include "../../lib/tabulate.hpp"
#include "../SCI_Utils.h"

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

std::string buildTooltipStringFromCommandEntry(const nlohmann::json& entry) {
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

std::string linesToString(std::vector<std::string> lines) {
  std::ostringstream result;
  std::copy(begin(lines), end(lines),
            std::ostream_iterator<std::string>(result, "\n"));
  return result.str();
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
      return buildTooltipStringFromCommandEntry(entry);
    }
  }
  return "";
}

bool isFirstLineOfArgumentRemark(std::string remark) {
  std::regex argumentPattern(".*\\^[a-h].*");
  std::smatch m;
  return std::regex_match(remark, m, argumentPattern);
}

std::vector<size_t> argumentLinePositions(std::vector<std::string> lines) {
  std::vector<size_t> argumentLines;
  for (size_t i = 0; i < lines.size(); ++i)
    if (isFirstLineOfArgumentRemark(lines[i])) argumentLines.push_back(i);
  return argumentLines;
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

bool startsWith(const std::string& bigString, const std::string& smallString) {
  return bigString.compare(0, smallString.length(), smallString) == 0;
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

std::vector<std::string> split(const std::string& string, char delim) {
  std::vector<std::string> result;
  std::stringstream ss(string);
  std::string line;
  while (std::getline(ss, line, delim)) result.push_back(line);
  return result;
}

std::map<std::string, Calltip> buildLabelLookupTable(
    const std::vector<std::string>& lines) {
  std::map<std::string, Calltip> result;

  for (size_t i = 0; i < lines.size(); ++i) {
    const auto& currentLine = lines[i];
    if (startsWith(currentLine, "#")) {
      // TODO: (David) this is stupid. extractLabelDescription flattens a vector
      // of strings and then I'm turning it back into a vector on the next line.
      const auto labelDescription = extractLabelDescription(lines, i);
      const auto argLines = argumentLinePositions(labelDescription);
      Calltip calltip = {labelDescription, argLines};

      const std::regex labelPattern("^#(\\w+)($|;)");
      std::smatch match;
      std::regex_search(currentLine, match, labelPattern);
      result.emplace(
          std::pair<std::string, Calltip>({match[1].str(), calltip}));
    }
  }
  return result;
}
std::map<std::string, Calltip> parseDocument() {
  const auto dmcCode = getDocumentText();
  const auto filteredLines = toLines(std::istringstream(dmcCode));
  const auto labelDetails = buildLabelLookupTable(filteredLines);
  return labelDetails;
}

size_t find_nth(const std::string& haystack, size_t pos,
                const std::string& needle, size_t nth) {
  size_t found_pos = haystack.find(needle, pos);
  if (0 == nth || std::string::npos == found_pos) return found_pos;
  return find_nth(haystack, found_pos + 1, needle, nth - 1);
}

size_t startOfNthLine(const std::string& haystack, const size_t nth) {
  return find_nth(haystack, 0, "\n",
                  nth - 1);  // - 1 because the first line doesn't have a '\n'
                             // associated with it.
}

std::pair<size_t, size_t> argumentLineRange(
    std::string calltip, std::vector<size_t> argumentLineNums,
    const size_t currentArgument) {
  const auto current = argumentLineNums.begin() + currentArgument;
  const auto last = argumentLineNums.end();
  if (current == last) return {0, 0};

  const auto startOfFirstLine = startOfNthLine(calltip, *current);
  const auto endOfLastLine = std::next(current) < last
                                 ? startOfNthLine(calltip, *std::next(current))
                                 : calltip.length();
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

void generateLabelCalltip() {
  const auto callTipString =
      cleanLabelDescription(linesToString(currentCalltip.description));
  displayCallTip(
      callTipString, currentPosition(),
      argumentLineRange(callTipString, currentCalltip.argumentLineNums,
                        currentArgumentNumber));
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
      generateLabelCalltip();
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

char charAt(int position) {
  const auto curScintilla = currentScintilla();
  return (char)::SendMessage(curScintilla, SCI_GETCHARAT, position, 0);
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
