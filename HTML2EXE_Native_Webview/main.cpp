/**
 * HTML2EXE Native WebView Host
 * Zero .NET dependencies. Pure Win32 + WebView2 COM API.
 * Portable across MSVC, Clang, and MinGW-w64.
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
#include <string>
#include <vector>
#include <algorithm>
#include <utility>

// WebView2 header provided by NuGet package in CI or local build
#include <WebView2.h>

// ── Lightweight COM smart pointer (no WRL dependency) ─────────────────────────

template <typename T>
class ComPtr {
    T* m_ptr = nullptr;
public:
    ComPtr() = default;
    ComPtr(T* p) : m_ptr(p) { if (m_ptr) m_ptr->AddRef(); }
    ~ComPtr() { Reset(); }
    ComPtr(const ComPtr& o) : m_ptr(o.m_ptr) { if (m_ptr) m_ptr->AddRef(); }
    ComPtr(ComPtr&& o) noexcept : m_ptr(o.m_ptr) { o.m_ptr = nullptr; }
    ComPtr& operator=(const ComPtr& o) {
        if (this != &o) {
            Reset();
            m_ptr = o.m_ptr;
            if (m_ptr) m_ptr->AddRef();
        }
        return *this;
    }
    ComPtr& operator=(ComPtr&& o) noexcept {
        if (this != &o) {
            Reset();
            m_ptr = o.m_ptr;
            o.m_ptr = nullptr;
        }
        return *this;
    }
    ComPtr& operator=(T* p) {
        if (m_ptr != p) {
            Reset();
            m_ptr = p;
            if (m_ptr) m_ptr->AddRef();
        }
        return *this;
    }
    void Reset() { if (m_ptr) { m_ptr->Release(); m_ptr = nullptr; } }
    T* Get() const { return m_ptr; }
    T** GetAddressOf() { Reset(); return &m_ptr; }
    T* operator->() const { return m_ptr; }
    explicit operator bool() const { return m_ptr != nullptr; }
};

// ── COM Callback Handlers for WebView2 Events ─────────────────────────────────

template <typename F>
class EnvCompletedHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler {
    F m_fn;
    ULONG m_ref = 1;
public:
    EnvCompletedHandler(F&& fn) : m_fn(std::forward<F>(fn)) {}
    virtual ~EnvCompletedHandler() = default;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** ppv) override {
        if (!ppv) return E_POINTER;
        *ppv = static_cast<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = --m_ref;
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT hr, ICoreWebView2Environment* env) override {
        return m_fn(hr, env);
    }
};
template <typename F>
auto MakeEnvHandler(F&& fn) {
    return new EnvCompletedHandler<typename std::decay<F>::type>(std::forward<F>(fn));
}

template <typename F>
class CtrlCompletedHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler {
    F m_fn;
    ULONG m_ref = 1;
public:
    CtrlCompletedHandler(F&& fn) : m_fn(std::forward<F>(fn)) {}
    virtual ~CtrlCompletedHandler() = default;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** ppv) override {
        if (!ppv) return E_POINTER;
        *ppv = static_cast<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = --m_ref;
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(HRESULT hr, ICoreWebView2Controller* ctrl) override {
        return m_fn(hr, ctrl);
    }
};
template <typename F>
auto MakeCtrlHandler(F&& fn) {
    return new CtrlCompletedHandler<typename std::decay<F>::type>(std::forward<F>(fn));
}

template <typename F>
class TitleChangedHandler : public ICoreWebView2DocumentTitleChangedEventHandler {
    F m_fn;
    ULONG m_ref = 1;
public:
    TitleChangedHandler(F&& fn) : m_fn(std::forward<F>(fn)) {}
    virtual ~TitleChangedHandler() = default;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** ppv) override {
        if (!ppv) return E_POINTER;
        *ppv = static_cast<ICoreWebView2DocumentTitleChangedEventHandler*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = --m_ref;
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* wv, IUnknown* args) override {
        return m_fn(wv, args);
    }
};
template <typename F>
auto MakeTitleHandler(F&& fn) {
    return new TitleChangedHandler<typename std::decay<F>::type>(std::forward<F>(fn));
}

template <typename F>
class FullScreenChangedHandler : public ICoreWebView2ContainsFullScreenElementChangedEventHandler {
    F m_fn;
    ULONG m_ref = 1;
public:
    FullScreenChangedHandler(F&& fn) : m_fn(std::forward<F>(fn)) {}
    virtual ~FullScreenChangedHandler() = default;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** ppv) override {
        if (!ppv) return E_POINTER;
        *ppv = static_cast<ICoreWebView2ContainsFullScreenElementChangedEventHandler*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = --m_ref;
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2* wv, IUnknown* args) override {
        return m_fn(wv, args);
    }
};
template <typename F>
auto MakeFullScreenHandler(F&& fn) {
    return new FullScreenChangedHandler<typename std::decay<F>::type>(std::forward<F>(fn));
}

template <typename F>
class AccelKeyPressedHandler : public ICoreWebView2AcceleratorKeyPressedEventHandler {
    F m_fn;
    ULONG m_ref = 1;
public:
    AccelKeyPressedHandler(F&& fn) : m_fn(std::forward<F>(fn)) {}
    virtual ~AccelKeyPressedHandler() = default;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** ppv) override {
        if (!ppv) return E_POINTER;
        *ppv = static_cast<ICoreWebView2AcceleratorKeyPressedEventHandler*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = --m_ref;
        if (r == 0) delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE Invoke(ICoreWebView2Controller* ctrl, ICoreWebView2AcceleratorKeyPressedEventArgs* args) override {
        return m_fn(ctrl, args);
    }
};
template <typename F>
auto MakeAccelHandler(F&& fn) {
    return new AccelKeyPressedHandler<typename std::decay<F>::type>(std::forward<F>(fn));
}

// ── Tiny JSON parser ──────────────────────────────────────────────────────────

static std::wstring JsonGetString(const std::string& json, const std::string& key)
{
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return L"";
    pos = json.find(':', pos + searchKey.size());
    if (pos == std::string::npos) return L"";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) pos++;
    if (pos >= json.size()) return L"";

    std::string raw;
    if (json[pos] == '"') {
        pos++;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) { pos++; raw += json[pos]; }
            else raw += json[pos];
            pos++;
        }
    } else {
        while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != '\r' && json[pos] != '\n')
            raw += json[pos++];
        while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\t')) raw.pop_back();
    }
    if (raw.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, raw.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring ws(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, raw.c_str(), -1, &ws[0], len);
    return ws;
}

static bool JsonGetBool(const std::string& json, const std::string& key, bool defVal = false)
{
    std::wstring v = JsonGetString(json, key);
    if (v == L"true")  return true;
    if (v == L"false") return false;
    return defVal;
}

static int JsonGetInt(const std::string& json, const std::string& key, int defVal = 0)
{
    std::wstring v = JsonGetString(json, key);
    if (v.empty()) return defVal;
    try { return std::stoi(v); } catch (...) { return defVal; }
}

// ── File and Path Helpers ─────────────────────────────────────────────────────

static bool FileExists_(const std::wstring& p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static std::wstring GetExeDir_()
{
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring s(buf);
    return s.substr(0, s.rfind(L'\\'));
}

static std::wstring AppDataDir_(const std::wstring& name)
{
    wchar_t buf[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buf);
    std::wstring base = std::wstring(buf) + L"\\HTML2EXE";
    CreateDirectoryW(base.c_str(), nullptr);
    std::wstring target = base + L"\\" + name;
    CreateDirectoryW(target.c_str(), nullptr);
    return target;
}

static std::string ReadFileUtf8_(const std::wstring& p)
{
    HANDLE hFile = CreateFileW(p.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return "";
    DWORD size = GetFileSize(hFile, nullptr);
    if (size == 0 || size == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return "";
    }
    std::string s(size, '\0');
    DWORD read = 0;
    ReadFile(hFile, &s[0], size, &read, nullptr);
    CloseHandle(hFile);
    s.resize(read);
    return s;
}

static std::wstring ToLower_(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(), ::towlower);
    return s;
}

static bool HasExt_(const std::wstring& p, const std::wstring& ext)
{
    std::wstring l = ToLower_(p);
    return l.size() >= ext.size() && l.substr(l.size() - ext.size()) == ext;
}

static std::vector<std::wstring> ListFiles_(const std::wstring& dir)
{
    std::vector<std::wstring> r;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return r;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            r.push_back(dir + L"\\" + fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return r;
}

static std::wstring ExpandEnv_(const std::wstring& s)
{
    wchar_t buf[4096];
    ExpandEnvironmentStringsW(s.c_str(), buf, 4096);
    return buf;
}

static bool RunHiddenCmd(const std::wstring& cmd, const std::wstring& workDir = L"")
{
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    std::wstring mutableCmd = cmd;
    LPCWSTR cwd = workDir.empty() ? nullptr : workDir.c_str();

    if (CreateProcessW(nullptr, &mutableCmd[0], nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW, nullptr, cwd, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 30000);
        DWORD code = 1;
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return code == 0;
    }
    return false;
}

static void ExtractZip_(const std::wstring& zipPath, const std::wstring& destDir)
{
    std::wstring tarCmd = L"tar.exe -xf \"" + zipPath + L"\" -C \"" + destDir + L"\"";
    if (!RunHiddenCmd(tarCmd, destDir)) {
        std::wstring psCmd = L"powershell.exe -NoProfile -NonInteractive -Command \"Expand-Archive -Force -LiteralPath '"
                           + zipPath + L"' -DestinationPath '" + destDir + L"'\"";
        RunHiddenCmd(psCmd, destDir);
    }
}

// ── Application State ─────────────────────────────────────────────────────────

struct AppState {
    HWND hwnd = nullptr;
    ComPtr<ICoreWebView2Environment> env;
    ComPtr<ICoreWebView2Controller>  ctrl;
    ComPtr<ICoreWebView2>            wv;

    std::string  cfg;
    std::wstring exeDir;
    std::wstring webfilesPath;

    bool blockClose = false;
    bool resizable = true;
    bool cfgFullscreen = false;
    bool isFullscreen = false;

    LONG savedStyle = 0;
    LONG savedExStyle = 0;
    WINDOWPLACEMENT savedWP{};
} G;

// ── Fullscreen Support ────────────────────────────────────────────────────────

static void EnterFS()
{
    if (G.isFullscreen || !G.hwnd) return;
    G.isFullscreen = true;
    G.savedStyle = GetWindowLongW(G.hwnd, GWL_STYLE);
    G.savedExStyle = GetWindowLongW(G.hwnd, GWL_EXSTYLE);
    G.savedWP.length = sizeof(G.savedWP);
    GetWindowPlacement(G.hwnd, &G.savedWP);

    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfoW(MonitorFromWindow(G.hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);

    SetWindowLongW(G.hwnd, GWL_STYLE,
                   G.savedStyle & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU));
    SetWindowLongW(G.hwnd, GWL_EXSTYLE,
                   G.savedExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

    SetWindowPos(G.hwnd, HWND_TOP,
                 mi.rcMonitor.left, mi.rcMonitor.top,
                 mi.rcMonitor.right - mi.rcMonitor.left,
                 mi.rcMonitor.bottom - mi.rcMonitor.top,
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

static void ExitFS()
{
    if (!G.isFullscreen || !G.hwnd) return;
    G.isFullscreen = false;
    SetWindowLongW(G.hwnd, GWL_STYLE, G.savedStyle);
    SetWindowLongW(G.hwnd, GWL_EXSTYLE, G.savedExStyle);
    SetWindowPlacement(G.hwnd, &G.savedWP);
    SetWindowPos(G.hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

// ── WndProc ───────────────────────────────────────────────────────────────────

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_SIZE:
        if (G.ctrl) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            G.ctrl->put_Bounds(rc);
        }
        break;

    case WM_KEYDOWN:
        if (wp == VK_F11 && G.resizable) {
            G.isFullscreen ? ExitFS() : EnterFS();
            return 0;
        } else if (wp == VK_ESCAPE && G.isFullscreen) {
            ExitFS();
            return 0;
        }
        break;

    case WM_CLOSE:
        if (G.blockClose) return 0;
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ── Dynamic WebView2 Loader ───────────────────────────────────────────────────

typedef HRESULT (STDAPICALLTYPE *CreateCoreWebView2EnvironmentWithOptionsFn)(
    PCWSTR browserExecutableFolder,
    PCWSTR userDataFolder,
    ICoreWebView2EnvironmentOptions* environmentOptions,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler);

static void InitWebView()
{
    auto& cfg = G.cfg;

    // 1. Determine target URL
    std::wstring url;
    std::wstring cfgUrl = JsonGetString(cfg, "url");
    if (!cfgUrl.empty()) {
        url = ExpandEnv_(cfgUrl);
    } else {
        auto files = ListFiles_(G.webfilesPath);
        std::sort(files.begin(), files.end(), [](const std::wstring& a, const std::wstring& b) {
            return ToLower_(a) < ToLower_(b);
        });

        std::vector<std::wstring> htmls;
        for (const auto& f : files) {
            if (HasExt_(f, L".html") || HasExt_(f, L".htm"))
                htmls.push_back(f);
        }

        // Priority 1: single file
        if (files.size() == 1) {
            url = files[0];
        }
        // Priority 2: index.html
        else if (FileExists_(G.webfilesPath + L"\\index.html")) {
            url = G.webfilesPath + L"\\index.html";
        }
        // Priority 3: index.htm
        else if (FileExists_(G.webfilesPath + L"\\index.htm")) {
            url = G.webfilesPath + L"\\index.htm";
        }
        // Priority 4: single HTML file
        else if (htmls.size() == 1) {
            url = htmls[0];
        }
        // Priority 5: 2 files where one is .ico -> choose the non-ico file
        else if (files.size() == 2) {
            for (const auto& f : files) {
                if (!HasExt_(f, L".ico")) { url = f; break; }
            }
        }
        // Priority 6: any HTML file
        else if (!htmls.empty()) {
            url = htmls[0];
        }

        if (!url.empty() && url.substr(0, 8) != L"https://" && url.substr(0, 7) != L"http://") {
            url = L"file:///" + url;
        }
    }

    // 2. User Data Folder scoped to AppData\HTML2EXE\<title>
    std::wstring title = JsonGetString(cfg, "title");
    if (title.empty()) title = L"default";
    std::wstring udf = AppDataDir_(title);

    // 3. Load WebView2Loader.dll
    std::wstring loaderPath = G.exeDir + L"\\WebView2Loader.dll";
    HMODULE hLoader = LoadLibraryW(loaderPath.c_str());
    if (!hLoader) hLoader = LoadLibraryW(L"WebView2Loader.dll");

    if (!hLoader) {
        MessageBoxW(G.hwnd,
            L"WebView2Loader.dll could not be loaded.\nEnsure WebView2Loader.dll is present in the application folder.",
            L"HTML2EXE Error", MB_ICONERROR);
        PostQuitMessage(1);
        return;
    }

    auto pCreateEnv = (CreateCoreWebView2EnvironmentWithOptionsFn)GetProcAddress(
        hLoader, "CreateCoreWebView2EnvironmentWithOptions");

    if (!pCreateEnv) {
        MessageBoxW(G.hwnd,
            L"Failed to locate CreateCoreWebView2EnvironmentWithOptions in WebView2Loader.dll.",
            L"HTML2EXE Error", MB_ICONERROR);
        PostQuitMessage(1);
        return;
    }

    auto envHandler = MakeEnvHandler(
        [url](HRESULT hr, ICoreWebView2Environment* env) -> HRESULT {
            if (FAILED(hr) || !env) {
                MessageBoxW(G.hwnd,
                    L"Microsoft Edge WebView2 Runtime is required.\nPlease download and install it from:\nhttps://developer.microsoft.com/en-us/microsoft-edge/webview2/",
                    L"HTML2EXE - Runtime Required", MB_ICONEXCLAMATION);
                PostQuitMessage(1);
                return hr;
            }

            G.env = env;

            auto ctrlHandler = MakeCtrlHandler(
                [url](HRESULT hr, ICoreWebView2Controller* ctrl) -> HRESULT {
                    if (FAILED(hr) || !ctrl) {
                        PostQuitMessage(1);
                        return hr;
                    }

                    G.ctrl = ctrl;
                    ctrl->get_CoreWebView2(G.wv.GetAddressOf());

                    // Configure browser settings
                    ComPtr<ICoreWebView2Settings> settings;
                    G.wv->get_Settings(settings.GetAddressOf());
                    if (settings) {
                        settings->put_AreDefaultContextMenusEnabled(JsonGetBool(G.cfg, "context_menu", false));
                        settings->put_AreDevToolsEnabled(JsonGetBool(G.cfg, "dev_tools", false));
                        settings->put_IsZoomControlEnabled(JsonGetBool(G.cfg, "zoom_control", false));
                        settings->put_IsStatusBarEnabled(FALSE);
                    }

                    // Dynamic title sync if title not hardcoded in config
                    if (JsonGetString(G.cfg, "title").empty()) {
                        auto titleHandler = MakeTitleHandler(
                            [](ICoreWebView2* wv, IUnknown*) -> HRESULT {
                                LPWSTR docTitle = nullptr;
                                wv->get_DocumentTitle(&docTitle);
                                if (docTitle) {
                                    SetWindowTextW(G.hwnd, docTitle);
                                    CoTaskMemFree(docTitle);
                                }
                                return S_OK;
                            });
                        G.wv->add_DocumentTitleChanged(titleHandler, nullptr);
                    }

                    // Track HTML5 video/element fullscreen events
                    auto fsHandler = MakeFullScreenHandler(
                        [](ICoreWebView2* wv, IUnknown*) -> HRESULT {
                            if (G.cfgFullscreen) return S_OK;
                            BOOL full = FALSE;
                            wv->get_ContainsFullScreenElement(&full);
                            if (full) EnterFS(); else ExitFS();
                            return S_OK;
                        });
                    G.wv->add_ContainsFullScreenElementChanged(fsHandler, nullptr);

                    // Keyboard shortcuts while webview has focus (F11 fullscreen, Escape exit)
                    auto accelHandler = MakeAccelHandler(
                        [](ICoreWebView2Controller*, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT {
                            COREWEBVIEW2_KEY_EVENT_KIND kind;
                            args->get_KeyEventKind(&kind);
                            if (kind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN) {
                                UINT key = 0;
                                args->get_VirtualKey(&key);
                                if (key == VK_F11 && G.resizable) {
                                    G.isFullscreen ? ExitFS() : EnterFS();
                                    args->put_Handled(TRUE);
                                } else if (key == VK_ESCAPE && G.isFullscreen) {
                                    ExitFS();
                                    args->put_Handled(TRUE);
                                }
                            }
                            return S_OK;
                        });
                    G.ctrl->add_AcceleratorKeyPressed(accelHandler, nullptr);

                    // Resize and show WebView2
                    RECT rc;
                    GetClientRect(G.hwnd, &rc);
                    ctrl->put_Bounds(rc);
                    ctrl->put_IsVisible(TRUE);

                    // Navigate
                    if (!url.empty()) {
                        G.wv->Navigate(url.c_str());
                    }

                    return S_OK;
                });

            env->CreateCoreWebView2Controller(G.hwnd, ctrlHandler);
            return S_OK;
        });

    pCreateEnv(nullptr, udf.c_str(), nullptr, envHandler);
}

// ── Application Entry Point ───────────────────────────────────────────────────

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int)
{
    SetProcessDPIAware();
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    G.exeDir = GetExeDir_();
    G.webfilesPath = G.exeDir + L"\\webfiles";

    // 1. Extract webfiles.zip if present
    std::wstring zipPath = G.exeDir + L"\\webfiles.zip";
    if (FileExists_(zipPath)) {
        ExtractZip_(zipPath, G.exeDir);
        DeleteFileW(zipPath.c_str());
    }

    // 2. Read config.json
    G.cfg = ReadFileUtf8_(G.exeDir + L"\\config.json");
    auto& cfg = G.cfg;

    std::wstring title       = JsonGetString(cfg, "title");
    int          w           = JsonGetInt(cfg, "width", 1024);
    int          h           = JsonGetInt(cfg, "height", 768);
    bool         maximized   = JsonGetBool(cfg, "maximized", false);
    bool         resizable   = JsonGetBool(cfg, "resizable", true);
    bool         fullscreen  = JsonGetBool(cfg, "fullscreen", false);
    bool         alwaysTop   = JsonGetBool(cfg, "always_on_top", false);
    bool         showInBar   = JsonGetBool(cfg, "show_in_taskbar", true);
    bool         ctrlBox     = JsonGetBool(cfg, "control_box", true);
    bool         minimizable = JsonGetBool(cfg, "minimizable", true);
    bool         maximizable = JsonGetBool(cfg, "maximizable", true);
    std::wstring iconPath    = JsonGetString(cfg, "icon");
    std::wstring additCmd    = JsonGetString(cfg, "additional_cmd");

    G.blockClose    = JsonGetBool(cfg, "block_close", false);
    G.resizable     = resizable;
    G.cfgFullscreen = fullscreen;

    // 3. Register window class
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"HTML2EXE_NativeHost";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);

    HICON hIcon = nullptr;
    if (!iconPath.empty()) {
        std::wstring icoFull = (iconPath.find(L'\\') == std::wstring::npos && iconPath.find(L'/') == std::wstring::npos)
            ? G.exeDir + L"\\" + iconPath : iconPath;
        hIcon = (HICON)LoadImageW(nullptr, icoFull.c_str(), IMAGE_ICON, 0, 0,
                                  LR_LOADFROMFILE | LR_DEFAULTSIZE);
    }
    wc.hIcon   = hIcon;
    wc.hIconSm = hIcon;
    RegisterClassExW(&wc);

    // 4. Calculate styles and window positioning
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!resizable)   style &= ~WS_THICKFRAME;
    if (!maximizable) style &= ~WS_MAXIMIZEBOX;
    if (!minimizable) style &= ~WS_MINIMIZEBOX;
    if (!ctrlBox)     style &= ~(WS_SYSMENU | WS_CAPTION);

    DWORD exStyle = WS_EX_APPWINDOW;
    if (!showInBar) exStyle &= ~WS_EX_APPWINDOW;
    if (alwaysTop)  exStyle |= WS_EX_TOPMOST;

    int sx = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int sy = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

    G.hwnd = CreateWindowExW(
        exStyle, L"HTML2EXE_NativeHost",
        title.empty() ? L"HTML2EXE" : title.c_str(),
        style, sx, sy, w, h, nullptr, nullptr, hInst, nullptr);

    if (!G.hwnd) {
        CoUninitialize();
        return 1;
    }

    if (fullscreen) {
        ShowWindow(G.hwnd, SW_SHOWNORMAL);
        EnterFS();
    } else if (maximized) {
        ShowWindow(G.hwnd, SW_MAXIMIZE);
    } else {
        ShowWindow(G.hwnd, SW_SHOWNORMAL);
    }
    UpdateWindow(G.hwnd);

    // 5. Execute additional_cmd if specified
    if (!additCmd.empty()) {
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        PROCESS_INFORMATION pi{};
        std::wstring cmd = L"cmd.exe /c " + additCmd;
        if (CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE,
                           CREATE_NO_WINDOW, nullptr, G.webfilesPath.c_str(), &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    // 6. Initialize WebView2
    InitWebView();

    // 7. Windows Message Loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return (int)msg.wParam;
}

// WinMain forwarder for toolchains and CRT startups expecting WinMain
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR, int nCmdShow)
{
    return wWinMain(hInst, hPrev, GetCommandLineW(), nCmdShow);
}

