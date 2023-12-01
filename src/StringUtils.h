#pragma once
#include <string>
#include <vector>

std::string linesToString(std::vector<std::string> lines);

bool startsWith(const std::string& bigString, const std::string& smallString);

std::vector<std::string> split(const std::string& string, char delim);

size_t find_nth(const std::string& haystack, size_t pos,
                const std::string& needle, size_t nth);

size_t startOfNthLine(const std::string& haystack, const size_t nth);
