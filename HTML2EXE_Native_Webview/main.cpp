/**
 * HTML2EXE Native WebView Host
 * Zero .NET dependency. Pure Win32 + WebView2 COM API.
 *
 * Features replicated from C# WebviewForm.cs:
 *   - config.json: title, width, height, url, maximized, resizable,
 *                  fullscreen, always_on_top, context_menu, dev_tools,
 *                  zoom_control, show_in_taskbar, control_box,
 *                  minimizable, maximizable, block_close, icon,
 *                  additional_cmd
 *   - Auto-finds index.html / any single HTML in webfiles/
 *   - F11 / Escape fullscreen toggle
 *   - Tracks page fullscreen events (ContainsFullScreenElementChanged)
 *   - Custom app icon from config
 *   - User data folder scoped to app title  (%APPDATA%\HTML2EXE\<title>)
 *   - Runs additional_cmd in webfiles/ directory
 *   - Extracts webfiles.zip on first run
 */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <wrl.h>
#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iterator>

// ── Tiny JSON reader ──────────────────────────────────────────────────────────

static std::wstring JsonGetString(const std::string& json, const std::string& key)
{
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return L"";
    pos = json.find(':', pos + searchKey.size());
    if (pos == std::string::npos) return L"";
    pos++;
    while (pos < json.size() && (json[pos]==' '||json[pos]=='\t'||json[pos]=='\r'||json[pos]=='\n')) pos++;
    if (pos >= json.size()) return L"";

    std::string raw;
    if (json[pos] == '"') {
        pos++;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos+1 < json.size()) { pos++; raw += json[pos]; }
            else raw += json[pos];
            pos++;
        }
    } else {
        while (pos < json.size() && json[pos]!=',' && json[pos]!='}' && json[pos]!='\r' && json[pos]!='\n')
            raw += json[pos++];
        while (!raw.empty() && (raw.back()==' '||raw.back()=='\t')) raw.pop_back();
    }
    if (raw.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, raw.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring ws(len-1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, raw.c_str(), -1, ws.data(), len);
    return ws;
}

static bool JsonGetBool(const std::string& json, const std::string& key, bool def=false)
{
    auto v = JsonGetString(json, key);
    if (v == L"true")  return true;
    if (v == L"false") return false;
    return def;
}

static int JsonGetInt(const std::string& json, const std::string& key, int def=0)
{
    auto v = JsonGetString(json, key);
    if (v.empty()) return def;
    try { return std::stoi(v); } catch (...) { return def; }
}

// ── Utilities ─────────────────────────────────────────────────────────────────

static bool FileExists_(const std::wstring& p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static std::wstring GetExeDir_()
{
    wchar_t buf[MAX_PATH]; GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring s(buf); return s.substr(0, s.rfind(L'\\'));
}

static std::wstring AppDataDir_(const std::wstring& name)
{
    wchar_t buf[MAX_PATH]; SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buf);
    return std::wstring(buf) + L"\\HTML2EXE\\" + name;
}

static std::string ReadFile_(const std::wstring& p)
{
    std::ifstream f(p); if (!f) return "";
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

static std::wstring ToLower_(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::towlower); return s;
}
static bool HasExt_(const std::wstring& p, const std::wstring& ext)
{
    auto l = ToLower_(p); return l.size()>=ext.size() && l.substr(l.size()-ext.size())==ext;
}

static std::vector<std::wstring> ListFiles_(const std::wstring& dir)
{
    std::vector<std::wstring> r; WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir+L"\\*").c_str(), &fd);
    if (h==INVALID_HANDLE_VALUE) return r;
    do { if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) r.push_back(dir+L"\\"+fd.cFileName); }
    while (FindNextFileW(h, &fd));
    FindClose(h); return r;
}

static std::wstring ExpandEnv_(const std::wstring& s)
{
    wchar_t buf[4096]; ExpandEnvironmentStringsW(s.c_str(), buf, 4096); return buf;
}

static bool ExtractZip_(const std::wstring& zip, const std::wstring& dest)
{
    std::wstring cmd = L"powershell.exe -NoProfile -Command \"Expand-Archive -Force -LiteralPath '"
                     + zip + L"' -DestinationPath '" + dest + L"'\"";
    STARTUPINFOW si{}; si.cb=sizeof(si); si.dwFlags=STARTF_USESHOWWINDOW; si.wShowWindow=SW_HIDE;
    PROCESS_INFORMATION pi{}; DWORD code=1;
    if (CreateProcessW(nullptr,(LPWSTR)cmd.c_str(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&si,&pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    }
    return code==0;
}

// ── App State ─────────────────────────────────────────────────────────────────

struct App {
    HWND hwnd{};
    Microsoft::WRL::ComPtr<ICoreWebView2Environment> env;
    Microsoft::WRL::ComPtr<ICoreWebView2Controller>  ctrl;
    Microsoft::WRL::ComPtr<ICoreWebView2>            wv;

    std::string  cfg;
    std::wstring exeDir;
    std::wstring webfilesPath;

    bool blockClose{false}, resizable{true}, cfgFullscreen{false};
    bool isFullscreen{false};
    LONG savedStyle{}, savedExStyle{};
    WINDOWPLACEMENT savedWP{};
} G;

// ── Fullscreen ────────────────────────────────────────────────────────────────

static void EnterFS()
{
    if (G.isFullscreen) return;
    G.isFullscreen = true;
    G.savedStyle   = GetWindowLongW(G.hwnd, GWL_STYLE);
    G.savedExStyle = GetWindowLongW(G.hwnd, GWL_EXSTYLE);
    G.savedWP.length = sizeof(G.savedWP);
    GetWindowPlacement(G.hwnd, &G.savedWP);
    MONITORINFO mi{sizeof(mi)};
    GetMonitorInfoW(MonitorFromWindow(G.hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
    SetWindowLongW(G.hwnd, GWL_STYLE,   G.savedStyle   & ~(WS_CAPTION|WS_THICKFRAME|WS_MINIMIZE|WS_MAXIMIZE|WS_SYSMENU));
    SetWindowLongW(G.hwnd, GWL_EXSTYLE, G.savedExStyle & ~(WS_EX_DLGMODALFRAME|WS_EX_WINDOWEDGE|WS_EX_CLIENTEDGE|WS_EX_STATICEDGE));
    SetWindowPos(G.hwnd, HWND_TOP,
                 mi.rcMonitor.left, mi.rcMonitor.top,
                 mi.rcMonitor.right-mi.rcMonitor.left,
                 mi.rcMonitor.bottom-mi.rcMonitor.top,
                 SWP_NOOWNERZORDER|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
}
static void ExitFS()
{
    if (!G.isFullscreen) return;
    G.isFullscreen = false;
    SetWindowLongW(G.hwnd, GWL_STYLE,   G.savedStyle);
    SetWindowLongW(G.hwnd, GWL_EXSTYLE, G.savedExStyle);
    SetWindowPlacement(G.hwnd, &G.savedWP);
    SetWindowPos(G.hwnd, nullptr, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
}

// ── WndProc ───────────────────────────────────────────────────────────────────

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_SIZE:
        if (G.ctrl) { RECT rc; GetClientRect(hwnd,&rc); G.ctrl->put_Bounds(rc); }
        break;
    case WM_KEYDOWN:
        if (wp==VK_F11 && G.resizable) { G.isFullscreen ? ExitFS() : EnterFS(); }
        else if (wp==VK_ESCAPE && G.isFullscreen) ExitFS();
        break;
    case WM_CLOSE:
        if (G.blockClose) return 0;
        DestroyWindow(hwnd); break;
    case WM_DESTROY:
        PostQuitMessage(0); break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ── WebView2 init ─────────────────────────────────────────────────────────────

static void InitWebView()
{
    auto& cfg = G.cfg;

    // Determine URL
    std::wstring url;
    std::wstring cfgUrl = JsonGetString(cfg, "url");
    if (!cfgUrl.empty()) {
        url = ExpandEnv_(cfgUrl);
    } else {
        auto& wp = G.webfilesPath;
        auto files = ListFiles_(wp);
        std::sort(files.begin(), files.end(), [](auto&a,auto&b){ return ToLower_(a)<ToLower_(b); });
        std::vector<std::wstring> htmls;
        for (auto& f : files) if (HasExt_(f,L".html")||HasExt_(f,L".htm")) htmls.push_back(f);

        if (files.size()==1)                          url = files[0];
        else if (FileExists_(wp+L"\\index.html"))     url = wp+L"\\index.html";
        else if (FileExists_(wp+L"\\index.htm"))      url = wp+L"\\index.htm";
        else if (htmls.size()==1)                     url = htmls[0];
        else if (files.size()==2) {
            for (auto& f:files) if (!HasExt_(f,L".ico")) { url=f; break; }
        }
        else if (!htmls.empty())                      url = htmls[0];

        if (!url.empty() && url.substr(0,8)!=L"https://" && url.substr(0,7)!=L"http://")
            url = L"file:///" + url;
    }

    std::wstring title = JsonGetString(cfg, "title");
    if (title.empty()) title = L"default";
    std::wstring udf = AppDataDir_(title);
    CreateDirectoryW(udf.c_str(), nullptr);

    auto opts = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    CreateCoreWebView2EnvironmentWithOptions(nullptr, udf.c_str(), opts.Get(),
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [url](HRESULT hr, ICoreWebView2Environment* env) -> HRESULT {
                if (FAILED(hr)) {
                    MessageBoxW(nullptr,
                        L"WebView2 Runtime not found.\nDownload it from https://developer.microsoft.com/en-us/microsoft-edge/webview2/",
                        L"HTML2EXE", MB_ICONERROR);
                    PostQuitMessage(1); return hr;
                }
                G.env = env;
                env->CreateCoreWebView2Controller(G.hwnd,
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [url](HRESULT hr, ICoreWebView2Controller* ctrl) -> HRESULT {
                            if (FAILED(hr)) { PostQuitMessage(1); return hr; }
                            G.ctrl = ctrl;
                            ctrl->get_CoreWebView2(&G.wv);

                            // Settings
                            Microsoft::WRL::ComPtr<ICoreWebView2Settings> s;
                            G.wv->get_Settings(&s);
                            if (s) {
                                s->put_AreDefaultContextMenusEnabled(JsonGetBool(G.cfg,"context_menu",false));
                                s->put_AreDevToolsEnabled(JsonGetBool(G.cfg,"dev_tools",false));
                                s->put_IsZoomControlEnabled(JsonGetBool(G.cfg,"zoom_control",false));
                                s->put_IsStatusBarEnabled(FALSE);
                            }

                            // Title sync
                            if (JsonGetString(G.cfg,"title").empty()) {
                                G.wv->add_DocumentTitleChanged(
                                    Microsoft::WRL::Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
                                        [](ICoreWebView2* wv, IUnknown*) -> HRESULT {
                                            LPWSTR t=nullptr; wv->get_DocumentTitle(&t);
                                            if (t) { SetWindowTextW(G.hwnd,t); CoTaskMemFree(t); }
                                            return S_OK;
                                        }).Get(), nullptr);
                            }

                            // Fullscreen sync
                            G.wv->add_ContainsFullScreenElementChanged(
                                Microsoft::WRL::Callback<ICoreWebView2ContainsFullScreenElementChangedEventHandler>(
                                    [](ICoreWebView2* wv, IUnknown*) -> HRESULT {
                                        if (G.cfgFullscreen) return S_OK;
                                        BOOL full=FALSE; wv->get_ContainsFullScreenElement(&full);
                                        if (full) EnterFS(); else ExitFS();
                                        return S_OK;
                                    }).Get(), nullptr);

                            RECT rc; GetClientRect(G.hwnd,&rc);
                            ctrl->put_Bounds(rc);
                            ctrl->put_IsVisible(TRUE);
                            if (!url.empty()) G.wv->Navigate(url.c_str());
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());
}

// ── Entry Point ───────────────────────────────────────────────────────────────

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
    SetProcessDPIAware();
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    G.exeDir       = GetExeDir_();
    G.webfilesPath = G.exeDir + L"\\webfiles";

    // Extract webfiles.zip if present
    std::wstring zip = G.exeDir + L"\\webfiles.zip";
    if (FileExists_(zip)) { ExtractZip_(zip, G.exeDir); DeleteFileW(zip.c_str()); }

    // Read config
    G.cfg = ReadFile_(G.exeDir + L"\\config.json");
    auto& cfg = G.cfg;

    std::wstring title       = JsonGetString(cfg, "title");
    int          w           = JsonGetInt(cfg, "width",  1024);
    int          h           = JsonGetInt(cfg, "height",  768);
    bool         maximized   = JsonGetBool(cfg, "maximized",  false);
    bool         resizable   = JsonGetBool(cfg, "resizable",  true);
    bool         fullscreen  = JsonGetBool(cfg, "fullscreen",  false);
    bool         alwaysTop   = JsonGetBool(cfg, "always_on_top",  false);
    bool         showInBar   = JsonGetBool(cfg, "show_in_taskbar",  true);
    bool         ctrlBox     = JsonGetBool(cfg, "control_box",  true);
    bool         minimizable = JsonGetBool(cfg, "minimizable",  true);
    bool         maximizable = JsonGetBool(cfg, "maximizable",  true);
    std::wstring iconPath    = JsonGetString(cfg, "icon");
    std::wstring additCmd    = JsonGetString(cfg, "additional_cmd");

    G.blockClose    = JsonGetBool(cfg, "block_close", false);
    G.resizable     = resizable;
    G.cfgFullscreen = fullscreen;

    // Register window class
    WNDCLASSEXW wc{}; wc.cbSize=sizeof(wc); wc.lpfnWndProc=WndProc;
    wc.hInstance=hInst; wc.lpszClassName=L"H2E_NWV";
    wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor=LoadCursor(nullptr, IDC_ARROW);

    // Load icon
    HICON hIcon = nullptr;
    if (!iconPath.empty()) {
        std::wstring icoFull = (iconPath.find(L'\\')==std::wstring::npos && iconPath.find(L'/')==std::wstring::npos)
            ? G.exeDir + L"\\" + iconPath : iconPath;
        hIcon = (HICON)LoadImageW(nullptr, icoFull.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE|LR_DEFAULTSIZE);
    }
    wc.hIcon = hIcon; wc.hIconSm = hIcon;
    RegisterClassExW(&wc);

    // Build styles
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!resizable)   style &= ~WS_THICKFRAME;
    if (!maximizable) style &= ~WS_MAXIMIZEBOX;
    if (!minimizable) style &= ~WS_MINIMIZEBOX;
    if (!ctrlBox)     style &= ~(WS_SYSMENU|WS_CAPTION);

    DWORD exStyle = WS_EX_APPWINDOW;
    if (!showInBar) exStyle &= ~WS_EX_APPWINDOW;
    if (alwaysTop)  exStyle |= WS_EX_TOPMOST;

    int sx=(GetSystemMetrics(SM_CXSCREEN)-w)/2;
    int sy=(GetSystemMetrics(SM_CYSCREEN)-h)/2;

    G.hwnd = CreateWindowExW(exStyle, L"H2E_NWV",
        title.empty() ? L"HTML2EXE App" : title.c_str(),
        style, sx, sy, w, h, nullptr, nullptr, hInst, nullptr);
    if (!G.hwnd) { CoUninitialize(); return 1; }

    if (fullscreen) { ShowWindow(G.hwnd, SW_SHOWNORMAL); EnterFS(); }
    else if (maximized) ShowWindow(G.hwnd, SW_MAXIMIZE);
    else ShowWindow(G.hwnd, SW_SHOWNORMAL);
    UpdateWindow(G.hwnd);

    // additional_cmd
    if (!additCmd.empty()) {
        STARTUPINFOW si{}; si.cb=sizeof(si); si.dwFlags=STARTF_USESHOWWINDOW; si.wShowWindow=SW_HIDE;
        PROCESS_INFORMATION pi{};
        std::wstring cmd = L"cmd.exe /c " + additCmd;
        if (CreateProcessW(nullptr,(LPWSTR)cmd.c_str(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,
                           G.webfilesPath.c_str(),&si,&pi)) {
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
        }
    }

    InitWebView();

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    CoUninitialize();
    return (int)msg.wParam;
}
