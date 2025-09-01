#include "rb_to_lemon.hpp"
#include <windows.h>
#include <commdlg.h>
#include <string>

static std::string NarrowUTF8(const std::wstring& ws) {
    if (ws.empty()) return {};

    // כמה בתים נצטרך
    int sz = WideCharToMultiByte(CP_UTF8, 0,
        ws.c_str(), static_cast<int>(ws.size()),
        nullptr, 0, nullptr, nullptr);
    if (sz <= 0) return {};

    // נריץ פעם שנייה לכתיבה
    std::string out(sz, '\0');

    // חלק מהכלים רואים data() כ-const; נשתמש ב-&out[0] שהוא char* תקני
    char* dst = out.empty() ? nullptr : &out[0];

    int written = WideCharToMultiByte(CP_UTF8, 0,
        ws.c_str(), static_cast<int>(ws.size()),
        dst, sz, nullptr, nullptr);
    if (written <= 0) return {};

    // אם נכתב פחות (לא אמור לקרות), נתקן אורך
    if (written != sz) out.resize(written);

    return out;
}


static bool PickRBFile(std::wstring& outPath) {
    wchar_t buf[MAX_PATH] = L"";                 // <<< בופר רחב
    OPENFILENAMEW ofn{};                         // <<< OPENFILENAMEW
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = L"RB files (*.rb)\0*.rb\0All files (*.*)\0*.*\0"; // <<< רחב
    ofn.lpstrFile = buf;                       // <<< LPWSTR
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"בחרי קובץ RB להמרה";     // <<< רחב
    if (GetOpenFileNameW(&ofn)) {                // <<< גרסת W
        outPath = buf;
        return true;
    }
    return false;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    std::wstring rbPathW;
    if (!PickRBFile(rbPathW)) {
        MessageBoxW(nullptr, L"ביטלת בחירת קובץ.", L"RB→LEMON", MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    std::string rbPath = NarrowUTF8(rbPathW);

    rbconv::RBToLemonConverter converter;
    if (!converter.readRutherfordBoeing(rbPath)) {
        MessageBoxW(nullptr, L"שגיאה בקריאת קובץ RB.", L"RB→LEMON", MB_OK | MB_ICONERROR);
        return 1;
    }

    std::string base = rbPath;
    size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);

    converter.saveToLemonFormat(base + "_graph.lgf");
    converter.saveToGraphML(base + "_graph.graphml");

    MessageBoxW(nullptr, L"ההמרה הושלמה!\nנוצרו *_graph.lgf ו-*_graph.graphml ליד הקובץ.",
        L"RB→LEMON", MB_OK | MB_ICONINFORMATION);
    return 0;
}
