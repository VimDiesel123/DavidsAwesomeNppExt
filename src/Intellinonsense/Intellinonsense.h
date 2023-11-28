#pragma once

struct CurrentCalltipInfo {
	int argumentNumber;
};

struct Argument {
	size_t startLine, endLine;
};

struct Calltip {
	std::string description;
	std::vector<Argument> arguments;
};

typedef std::pair<std::string, size_t> LinePosition;

const std::string PATH_TO_MANUAL_DATA = "plugins\\DavidsAwesomeTools\\manual.json";

void loadManualData();
