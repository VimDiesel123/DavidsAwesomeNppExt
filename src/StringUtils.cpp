#include "StringUtils.h"
#include <sstream>

std::string linesToString(std::vector<std::string> lines) {
  std::ostringstream result;
  std::copy(begin(lines), end(lines),
            std::ostream_iterator<std::string>(result, "\n"));
  return result.str();
}

bool startsWith(const std::string& bigString, const std::string& smallString) {
  return bigString.compare(0, smallString.length(), smallString) == 0;
}

std::vector<std::string> split(const std::string& string, char delim) {
  std::vector<std::string> result;
  std::stringstream ss(string);
  std::string line;
  while (std::getline(ss, line, delim)) result.push_back(line);
  return result;
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


