#include "ShowFunny.h"

#include <string>

#include "../../lib/json.hpp"

void handleError() {
  const auto errorCode = GetLastError();
  LPVOID lpMsgBuf;

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf, 0, NULL);

  ::MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK);
}

void showFunny() {
  HINTERNET hInternet = InternetOpen(L"David's Awesome Tools",
                                     INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
  if (hInternet == NULL) {
    OutputDebugStringA("Failed to intialize internet handle.");
    handleError();
    return;
  }

  HINTERNET hRequest = InternetOpenUrl(
      hInternet,
      L"https://v2.jokeapi.dev/joke/"
      L"Any?blacklistFlags=nsfw,religious,political,racist,sexist,explicit",
      NULL, 0, INTERNET_FLAG_RELOAD, 0);
  if (hRequest == NULL) {
    OutputDebugStringA("Failed to create request.");
    handleError();
    return;
  }

  DWORD bytesRead;
  char buffer[1024];
  InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead);
  buffer[bytesRead] = '\0';
  const auto JSONResponse = std::string(buffer);

  // Parse the JSON response using JSON for Modern C++
  try {
    nlohmann::json jsonData = nlohmann::json::parse(JSONResponse);

    std::string category = jsonData["category"];
    std::string type = jsonData["type"];
    std::string joke = type == "twopart"
                           ? std::string(jsonData["setup"]) + "\n\n" +
                                 std::string(jsonData["delivery"])
                           : jsonData["joke"];

    ::MessageBoxA(NULL, joke.c_str(), "FUNNY", MB_OK);

  } catch (const nlohmann::json::parse_error& e) {
    OutputDebugStringA(e.what());
  }

  InternetCloseHandle(hRequest);
  InternetCloseHandle(hInternet);
}