#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <curses.h>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace fs = std::filesystem;

class Sudoku {
private:
    static constexpr int N = 9;
    std::array<std::array<int, N>, N> grid{};
    bool timed_out = false;
    std::chrono::steady_clock::time_point start_time;
    static constexpr auto TIME_LIMIT = std::chrono::seconds(5);

    bool isSafe(int row, int col, int num) const {
        for (int j = 0; j < N; ++j)
            if (grid[row][j] == num) return false;
        for (int i = 0; i < N; ++i)
            if (grid[i][col] == num) return false;
        int startRow = row - row % 3;
        int startCol = col - col % 3;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                if (grid[startRow + i][startCol + j] == num) return false;
        return true;
    }

    bool findEmptyCell(int& row, int& col) const {
        for (row = 0; row < N; ++row)
            for (col = 0; col < N; ++col)
                if (grid[row][col] == 0) return true;
        return false;
    }

public:
    Sudoku() = default;
    explicit Sudoku(const std::string& filename) : Sudoku() {
        std::ifstream file(filename);
        std::string line;
        int i = 0;
        while (i < N && std::getline(file, line)) {
            if (line.find("---") != std::string::npos) continue;
            int current_col = 0;
            for (char c : line) {
                if (current_col >= N) break;
                if (c == '.') grid[i][current_col++] = 0;
                else if (isdigit(c)) grid[i][current_col++] = c - '0';
            }
            i++;
        }
    }

    bool solve() {
        start_time = std::chrono::steady_clock::now();
        timed_out = false;
        return backtrack();
    }

    bool backtrack() {
        if (std::chrono::steady_clock::now() - start_time > TIME_LIMIT) {
            timed_out = true;
            return false;
        }
        int row, col;
        if (!findEmptyCell(row, col)) return true;
        for (int num = 1; num <= 9; ++num) {
            if (isSafe(row, col, num)) {
                grid[row][col] = num;
                if (backtrack()) return true;
                if (timed_out) return false;
                grid[row][col] = 0;
            }
        }
        return false;
    }

    bool hasTimedOut() const { return timed_out; }

    void saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        file << toString();
    }

    std::string toString() const {
        std::ostringstream oss;
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = 0; j < N; ++j) {
                oss << (grid[i][j] ? static_cast<char>(grid[i][j] + '0') : '.');
                if ((j + 1) % 3 == 0 && j + 1 != N) oss << "|";
            }
            oss << "\n";
            if ((i + 1) % 3 == 0 && i + 1 != N) oss << "-----------\n";
        }
        return oss.str();
    }
};

class TUIClass {
private:
    WINDOW* main_win;
    int color_pair;

    void
        init_colors(void)
    {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLUE);
        init_pair(2, COLOR_BLACK, COLOR_WHITE);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        color_pair = 1;
        bkgd(COLOR_PAIR(color_pair));
    }

    void
        draw_borders(void)
    {
        wattron(main_win, COLOR_PAIR(2));
        box(main_win, ACS_VLINE, ACS_HLINE);
        wattroff(main_win, COLOR_PAIR(2));
        wrefresh(main_win);
    }

    void
        center_window(WINDOW* win, int height, int width)
    {
        int x = (COLS - width) / 2;
        int y = (LINES - height) / 2;
        wresize(win, height, width);
        mvwin(win, y, x);
    }

public:
    TUIClass(void)
    {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        init_colors();
        main_win = newwin(LINES - 4, COLS - 8, 2, 4);
        refresh();
    }

    ~TUIClass(void)
    {
        delwin(main_win);
        endwin();
    }

    void
        show_header(const std::string& title)
    {
        wattron(main_win, COLOR_PAIR(2));
        mvwprintw(main_win, 0, 2, " %s ", title.c_str());
        wattroff(main_win, COLOR_PAIR(2));
        draw_borders();
    }

    int
        show_menu(void)
    {
        const char* items[]{ "Export Sudoku", "Import & Solve", "Exit" };
        int choice = 0;
        int start_y = (LINES - 10) / 2;

        WINDOW* menu_win = newwin(8, 40, start_y, (COLS - 40) / 2);
        keypad(menu_win, TRUE);

        while (true) {
            wbkgd(menu_win, COLOR_PAIR(1));
            box(menu_win, 0, 0);
            mvwprintw(menu_win, 1, 2, "Sudoku Solver - Main Menu");

            for (int i = 0; i < 3; i++) {
                mvwprintw(menu_win, 3 + i, 5, "%c %s",
                    (i == choice) ? '>' : ' ', items[i]);
            }

            wrefresh(menu_win);
            int ch = wgetch(menu_win);

            switch (ch)
            {
            case KEY_UP: if (choice > 0) choice--; break;
            case KEY_DOWN: if (choice < 2) choice++; break;
            case 10:
                delwin(menu_win);
                return choice + 1;
            case 27:
                delwin(menu_win);
                return 0;
            }
        }
    }

    std::string
        input_box(const std::string& prompt)
    {
        echo();
        curs_set(1);
        WINDOW* input_win = newwin(5, 50, (LINES - 5) / 2, (COLS - 50) / 2);
        box(input_win, 0, 0);
        mvwprintw(input_win, 1, 2, "%s:", prompt.c_str());
        wrefresh(input_win);

        char filename[256];
        mvwgetnstr(input_win, 2, 2, filename, sizeof(filename) - 1);

        noecho();
        curs_set(0);
        delwin(input_win);
        return filename;
    }

    void
        message_box(const std::string& msg, bool error = false)
    {
        WINDOW* msg_win = newwin(5, static_cast<int>(msg.length()) + 4, (LINES - 5) / 2, (COLS - static_cast<int>(msg.length()) - 4) / 2);
        wbkgd(msg_win, COLOR_PAIR(error ? 3 : 1));
        box(msg_win, 0, 0);
        mvwprintw(msg_win, 2, 2, "%s", msg.c_str());
        wrefresh(msg_win);
        wgetch(msg_win);
        delwin(msg_win);
    }

    void
        show_grid(const std::string& grid_str)
    {
        std::istringstream iss(grid_str);
        std::vector<std::string> lines;
        std::string line;

        while (std::getline(iss, line))
            lines.push_back(line);

        size_t win_height = lines.size() + 4;
        size_t win_width = lines[0].length() + 4;
        WINDOW* grid_win = newwin(static_cast<int>(win_height), static_cast<int>(win_width),
            (LINES - static_cast<int>(win_height)) / 2,
            (COLS - static_cast<int>(win_width)) / 2);
        box(grid_win, 0, 0);

        for (size_t i = 0; i < lines.size(); i++)
            mvwprintw(grid_win, static_cast<int>(i) + 2, 2, "%s", lines[i].c_str());

        mvwprintw(grid_win, static_cast<int>(win_height) - 2, 2, "Press any key to continue...");
        wrefresh(grid_win);
        wgetch(grid_win);
        delwin(grid_win);
    }
};

void reinit_ui(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    bkgd(COLOR_PAIR(1));
    refresh();
}


void
openEditor(const std::string& filename)
{
    endwin();
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", "notepad.exe", filename.c_str(), nullptr, SW_SHOW);
#else
    system(("nano " + filename).c_str());
#endif
    reinit_ui();
}

int 
main() {
    TUIClass tui;
    Sudoku sudoku;

    while (true) {
        tui.show_header(" Sudoku Solver ");
        int choice = tui.show_menu();
        if (choice == 1) {
            std::string filename = tui.input_box("Enter filename (.txt)");
            sudoku.saveToFile(filename);
            tui.message_box("File saved. Opening editor...");
            openEditor(filename);
        }
        else if (choice == 2) {
            std::string filename = tui.input_box("Enter filename to import");
            if (!fs::exists(filename)) {
                tui.message_box("File not found!", true);
                continue;
            }
            sudoku = Sudoku(filename);
            bool solved = sudoku.solve();
            if (solved) {
                tui.show_grid(sudoku.toString());
            }
            else if (sudoku.hasTimedOut()) {
                tui.message_box("Solver timed out! Puzzle may not be solvable.", true);
            }
            else {
                tui.message_box("No solution found!", true);
            }
        }
        else if (choice == 3 || choice == 0) {
            break;
        }
    }
    return 0;
}
