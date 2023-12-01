#pragma once
#include "PluginDefinition.h"

HWND currentScintilla();

std::string getDocumentText();

int currentPosition();

void displayCallTip(const std::string& text, const int position,
                    const std::pair<size_t, size_t> highlightRange);

void setCallTipHighlightRange(const std::pair<size_t, size_t>& highlightRange); 

std::string wordAt(int position);

char charAt(int position);
