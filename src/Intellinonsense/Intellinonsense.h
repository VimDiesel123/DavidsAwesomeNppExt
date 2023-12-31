#pragma once
#include <vector>
#include <string>

typedef struct SCNotification SCNotification;

struct Calltip {
  std::vector<std::string> description;
  std::vector<size_t> argumentLineNums;
};

typedef std::pair<std::string, size_t> LinePosition;

const std::string PATH_TO_MANUAL_DATA =
    "plugins\\DavidsAwesomeTools\\manual.json";

void loadManualData();
void onDwellStart(SCNotification* pNotify);
void onDwellEnd(SCNotification* pNotify);
void onCharacterAdded(SCNotification* pNotify);
void onTextDeleted(SCNotification* pNotify);
