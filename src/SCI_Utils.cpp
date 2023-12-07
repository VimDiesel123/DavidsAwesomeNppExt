#include "SCI_Utils.h"

extern NppData nppData;

HWND currentScintilla() {
  int which = -1;
  ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0,
                (LPARAM)&which);

  if (which == -1) return NULL;
  return (which == 0) ? nppData._scintillaMainHandle
                      : nppData._scintillaSecondHandle;
}

std::string getDocumentText() {
  const auto curScintilla = currentScintilla();
  // Get the length of the current document.
  const unsigned length = ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);
  // Store text of document in dmcCode
  std::string dmcCode;
  dmcCode.resize(length + 1);
  ::SendMessage(curScintilla, SCI_GETTEXT, length, (LPARAM)dmcCode.data());
  return dmcCode;
}

int currentPosition() {
  const auto curScintilla = currentScintilla();
  return ::SendMessage(curScintilla, SCI_GETCURRENTPOS, 0, 0);
}

void displayCallTip(const std::string& text, const int position,
                    const std::pair<size_t, size_t> highlightRange) {
  const auto curScintilla = currentScintilla();
  ::SendMessage(curScintilla, SCI_CALLTIPSETBACK, RGB(68, 70, 84), 0);
  ::SendMessage(curScintilla, SCI_CALLTIPSETFORE, RGB(209, 213, 219), 0);
  ::SendMessage(curScintilla, SCI_STYLESETSIZE, STYLE_CALLTIP, 9);
  ::SendMessage(curScintilla, SCI_CALLTIPUSESTYLE, 0, 0);
  ::SendMessage(curScintilla, SCI_CALLTIPSHOW, position, (LPARAM)text.c_str());
  setCallTipHighlightRange(highlightRange);
}

void setCallTipHighlightRange(const std::pair<size_t, size_t>& highlightRange) {
  const auto curScintilla = currentScintilla();
  ::SendMessage(curScintilla, SCI_CALLTIPSETFOREHLT, RGB(255, 235, 0), 0);
  ::SendMessage(curScintilla, SCI_CALLTIPSETHLT, highlightRange.first,
                highlightRange.second);
}

std::string wordAt(int position) {
  const auto curScintilla = currentScintilla();
  int wordStart =
      ::SendMessage(curScintilla, SCI_WORDSTARTPOSITION, position, true);
  int wordEnd =
      ::SendMessage(curScintilla, SCI_WORDENDPOSITION, position, true);

  char word[256];
  Sci_TextRange tr = {{wordStart, wordEnd}, word};
  const auto result =
      ::SendMessage(curScintilla, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);
  if (!result || result > 256) {
    return "";
  }
  return word;
}

char charAt(int position) {
  const auto curScintilla = currentScintilla();
  return (char)::SendMessage(curScintilla, SCI_GETCHARAT, position, 0);
}
