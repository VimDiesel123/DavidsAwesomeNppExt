#pragma once

struct CurrentCalltipInfo {
	int argumentNumber;
};

struct Argument {
	int startLine, endLine;
};

struct Calltip {
	std::string description;
	std::vector<Argument> arguments;
};

const std::string PATH_TO_MANUAL_DATA = "plugins\\DavidsAwesomeTools\\manual.json";

void loadManualData();
