#include "tasks.hpp"
#include "ui.hpp"
HWND list, entry, priority, notes, clockLabel, filter;
std::vector<Task> tasks;
std::vector<int> visible;
std::filesystem::path dir;
int remaining = 25 * 60;
bool running = false, smoke = false;
void persist() {
    atomicWrite(dir / L"tasks.txt", encode(tasks));
    atomicWrite(dir / L"notes.txt", utf8(text(notes)));
}
void updateClock() {
    SetWindowTextW(clockLabel,
                   (L"Фокус: " + std::to_wstring(remaining / 60) + L":" +
                    (remaining % 60 < 10 ? L"0" : L"") + std::to_wstring(remaining % 60) +
                    (running ? L" • идёт" : L" • пауза"))
                       .c_str());
}
void refresh() {
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    visible.clear();
    int f = (int)SendMessageW(filter, CB_GETCURSEL, 0, 0);
    for (auto &t : tasks) {
        if ((f == 1 && t.done) || (f == 2 && !t.done))
            continue;
        std::wstring line = (t.done ? L"✓ " : L"○ ") + std::wstring(L"[P") +
                            std::to_wstring(t.priority) + L"] " + wide(t.title);
        SendMessageW(list, LB_ADDSTRING, 0, (LPARAM)line.c_str());
        visible.push_back(t.id);
    }
    updateClock();
}
LRESULT CALLBACK proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    try {
        switch (msg) {
        case WM_CREATE: {
            smoke = wcsstr(GetCommandLineW(), L"--smoke") != nullptr;
            wchar_t local[32768];
            if (!GetEnvironmentVariableW(L"LOCALAPPDATA", local, 32768))
                throw std::runtime_error("LOCALAPPDATA missing");
            dir = std::filesystem::path(local) / L"WorkDesk";
            if (smoke)
                dir = std::filesystem::temp_directory_path() /
                      (L"WorkDesk-smoke-" + std::to_wstring(GetCurrentProcessId()));
            std::filesystem::create_directories(dir);
            control(w, L"STATIC", L"WORKDESK  /  Задачи • заметки • фокус", 0, 20, 18, 920, 30);
            entry = control(w, L"EDIT", L"", 10, 20, 58, 600, 30,
                            WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP);
            SendMessageW(entry, EM_SETCUEBANNER, 0, (LPARAM)L"Новая задача");
            priority =
                control(w, L"COMBOBOX", L"", 11, 635, 58, 120, 140, CBS_DROPDOWNLIST | WS_TABSTOP);
            for (auto s : {L"P1 Высокий", L"P2 Средний", L"P3 Низкий"})
                SendMessageW(priority, CB_ADDSTRING, 0, (LPARAM)s);
            SendMessageW(priority, CB_SETCURSEL, 1, 0);
            control(w, L"BUTTON", L"Добавить", 1, 775, 58, 170, 30, WS_TABSTOP);
            filter =
                control(w, L"COMBOBOX", L"", 12, 20, 100, 240, 140, CBS_DROPDOWNLIST | WS_TABSTOP);
            for (auto s : {L"Все задачи", L"Активные", L"Завершённые"})
                SendMessageW(filter, CB_ADDSTRING, 0, (LPARAM)s);
            SendMessageW(filter, CB_SETCURSEL, 0, 0);
            list = control(w, L"LISTBOX", L"", 13, 20, 140, 600, 330,
                           WS_BORDER | WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP);
            control(w, L"BUTTON", L"Готово / вернуть", 2, 20, 485, 190, 32, WS_TABSTOP);
            control(w, L"BUTTON", L"Удалить", 3, 225, 485, 125, 32, WS_TABSTOP);
            control(w, L"BUTTON", L"Экспорт CSV", 4, 365, 485, 170, 32, WS_TABSTOP);
            control(w, L"STATIC", L"Заметки — сохранение кнопкой ниже", 0, 645, 110, 300, 25);
            notes = control(w, L"EDIT", L"", 14, 645, 140, 300, 330,
                            WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN |
                                WS_TABSTOP);
            SendMessageW(notes, EM_SETLIMITTEXT, 1000000, 0);
            control(w, L"BUTTON", L"Сохранить всё", 5, 645, 485, 300, 32, WS_TABSTOP);
            clockLabel = control(w, L"STATIC", L"", 0, 20, 560, 550, 30);
            control(w, L"BUTTON", L"Старт / пауза", 6, 20, 605, 170, 32, WS_TABSTOP);
            control(w, L"BUTTON", L"Сброс 25 минут", 7, 210, 605, 180, 32, WS_TABSTOP);
            std::ifstream tf(dir / L"tasks.txt", std::ios::binary);
            if (tf) {
                std::ostringstream s;
                s << tf.rdbuf();
                tasks = decode(s.str());
            }
            std::ifstream nf(dir / L"notes.txt", std::ios::binary);
            if (nf) {
                std::ostringstream s;
                s << nf.rdbuf();
                SetWindowTextW(notes, wide(s.str()).c_str());
            }
            if (smoke) {
                tasks.push_back({1, 2, false, "Smoke Unicode: задача"});
                persist();
                if (decode(encode(tasks)).size() != 1)
                    throw std::runtime_error("Smoke failed");
            }
            refresh();
            SetTimer(w, 1, 1000, nullptr);
            if (smoke)
                SetTimer(w, 2, 1500, nullptr);
            return 0;
        }
        case WM_TIMER:
            if (wp == 2) {
                SendMessageW(w, WM_CLOSE, 0, 0);
                return 0;
            }
            if (running && remaining > 0) {
                --remaining;
                if (!remaining) {
                    running = false;
                    MessageBeep(MB_ICONINFORMATION);
                }
            }
            updateClock();
            return 0;
        case WM_COMMAND: {
            int id = LOWORD(wp);
            if (id == 12 && HIWORD(wp) == CBN_SELCHANGE) {
                refresh();
                return 0;
            }
            if (HIWORD(wp) != BN_CLICKED)
                return 0;
            if (id == 1) {
                auto title = utf8(text(entry));
                if (title.find_first_not_of(" \t\r\n") == std::string::npos)
                    return 0;
                tasks.push_back({nextId(tasks), (int)SendMessageW(priority, CB_GETCURSEL, 0, 0) + 1,
                                 false, title});
                persist();
                SetWindowTextW(entry, L"");
            }
            if (id == 2 || id == 3) {
                int s = (int)SendMessageW(list, LB_GETCURSEL, 0, 0);
                if (s < 0 || s >= (int)visible.size())
                    return 0;
                auto it = std::find_if(tasks.begin(), tasks.end(),
                                       [&](auto &t) { return t.id == visible[s]; });
                if (id == 2)
                    it->done = !it->done;
                else {
                    if (MessageBoxW(w, L"Удалить выбранную задачу?", L"WorkDesk",
                                    MB_YESNO | MB_ICONQUESTION) != IDYES)
                        return 0;
                    tasks.erase(it);
                }
                persist();
            }
            if (id == 4) {
                auto p = savePath(w);
                if (!p.empty()) {
                    std::string data = "\xef\xbb\xbfID,Priority,Done,Title\r\n";
                    for (auto &t : tasks)
                        data += std::to_string(t.id) + "," + std::to_string(t.priority) + "," +
                                (t.done ? "true" : "false") + "," + csv(t.title) + "\r\n";
                    atomicWrite(p, data);
                }
            }
            if (id == 5)
                persist();
            if (id == 6 && remaining)
                running = !running;
            if (id == 7) {
                running = false;
                remaining = 1500;
            }
            refresh();
            return 0;
        }
        case WM_CLOSE:
            persist();
            DestroyWindow(w);
            return 0;
        case WM_DESTROY:
            if (smoke)
                std::filesystem::remove_all(dir);
            PostQuitMessage(0);
            return 0;
        }
    } catch (const std::exception &e) {
        if (smoke) {
            PostQuitMessage(1);
            return 0;
        }
        MessageBoxW(w, wide(e.what()).c_str(), L"Ошибка — данные не сохранены",
                    MB_OK | MB_ICONERROR);
        if (msg == WM_CREATE)
            return -1;
    }
    return DefWindowProcW(w, msg, wp, lp);
}
int WINAPI wWinMain(HINSTANCE h, HINSTANCE, PWSTR, int) {
    return runApp(h, L"WorkDesk", proc);
}
