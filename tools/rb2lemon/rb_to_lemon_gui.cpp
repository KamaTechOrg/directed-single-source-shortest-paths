#include "rb_to_lemon.hpp"
#include <windows.h>
#include <commdlg.h>
#include <string>

static std::string NarrowUTF8(const std::wstring& ws) {
    if (ws.empty()) return {};

    int sz = WideCharToMultiByte(CP_UTF8, 0,
        ws.c_str(), static_cast<int>(ws.size()),
        nullptr, 0, nullptr, nullptr);
    if (sz <= 0) return {};

    std::string out(sz, '\0');

    char* dst = out.empty() ? nullptr : &out[0];

    int written = WideCharToMultiByte(CP_UTF8, 0,
        ws.c_str(), static_cast<int>(ws.size()),
        dst, sz, nullptr, nullptr);
    if (written <= 0) return {};

    if (written != sz) out.resize(written);

    return out;
}


static bool PickRBFile(std::wstring& outPath) {
    wchar_t buf[MAX_PATH] = L"";                 
    OPENFILENAMEW ofn{};                       
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = L"RB files (*.rb)\0*.rb\0All files (*.*)\0*.*\0"; 
    ofn.lpstrFile = buf;                      
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"בחרי קובץ RB להמרה";     
    if (GetOpenFileNameW(&ofn)) {               
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
