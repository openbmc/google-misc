// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <ncurses.h>

#include <ctime>
#include <sstream>
#include <string>
#include <vector>

constexpr int MARGIN_BOTTOM = 1;

std::vector<std::string> g_sensor_list;

int maxx, maxy, halfx, halfy;

constexpr int NUM_WINDOWS = 4;
// [0]: summary and history
// [1]: detail info for 1 sensor
// [2]: list of traffic
// [3]: status bar

WINDOW* g_windows[NUM_WINDOWS];

struct Rect
{
    int x, y, w, h; // X, Y, Width, Height
    Rect(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h)
    {}
    Rect() : x(0), y(0), w(1), h(1)
    {}
};

Rect g_window_rects[NUM_WINDOWS];

void CreateWindows()
{
    for (int i = 0; i < NUM_WINDOWS; i++)
    {
        g_windows[i] = newwin(maxy, maxx, 0, 0);
    }
}

void InitColorPairs()
{
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
}

// A warpper of the ncurses WINDOW with the following added:
// - A method to render its contents
// - Function to accept keystrokes   :todo
// - Code path to deal with the window's specific data  :todo
class DBusTopWindow
{
  public:
    DBusTopWindow()
    {
        win = newwin(25, 80, 0, 0); // Default to 80x25, will be updated
        has_border_ = true;
    }
    virtual ~DBusTopWindow()
    {}
    virtual void Render() = 0;
    virtual void OnResize(int win_w, int win_h) = 0;
    void UpdateWindowSizeAndPosition()
    {
        mvwin(win, rect.y, rect.x);
        wresize(win, rect.h, rect.w);
    }
    void DrawBorderIfNeeded()
    {
        wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
        wattrset(win, 0);
        wrefresh(win);
    }
    WINDOW* win;
    Rect rect;
    bool has_border_;
};

class SummaryView : public DBusTopWindow
{
  public:
    SummaryView() : DBusTopWindow()
    {}
    void Render() override
    {
        werase(win);
        mvwaddstr(win, 1, 1, "This is window A");
        DrawBorderIfNeeded();
        wrefresh(win);
    }
    void OnResize(int win_w, int win_h) override
    {
        rect.h = 8;
        rect.w = win_w;
        rect.x = 0;
        rect.y = 0;
        UpdateWindowSizeAndPosition();
    }
};

class SensorDetailView : public DBusTopWindow
{
  public:
    SensorDetailView() : DBusTopWindow()
    {}
    void Render() override
    {
        werase(win);
        mvwaddstr(win, 1, 1, "This is window B");
        DrawBorderIfNeeded();
        wrefresh(win);
    }

    void OnResize(int win_w, int win_h) override
    {
        rect.x = 0;
        rect.y = 8 - MARGIN_BOTTOM;
        rect.w = win_w / 2;
        rect.h = win_h - rect.y - MARGIN_BOTTOM;
        UpdateWindowSizeAndPosition();
    }
};

class DBusStatListView : public DBusTopWindow
{
  public:
    DBusStatListView() : DBusTopWindow()
    {}
    void Render() override
    {
        werase(win);
        // Write print statement here
        mvwaddstr(win, 1, 1, "This is window C");
        DrawBorderIfNeeded();
        wrefresh(win);
    }
    void OnResize(int win_w, int win_h) override
    {
        rect.y = 8 - MARGIN_BOTTOM;
        rect.w =
            win_w - (win_w / 2) + 1; // Perfectly overlap on the vertical edge
        rect.x = win_w - rect.w;
        rect.h = win_h - rect.y - MARGIN_BOTTOM;
        UpdateWindowSizeAndPosition();
    }
};

class FooterView : public DBusTopWindow
{
  public:
    FooterView() : DBusTopWindow()
    {}
    void OnResize(int win_w, int win_h) override
    {
        rect.h = 1;
        rect.w = win_w;
        rect.x = 0;
        rect.y = win_h - 1;
        UpdateWindowSizeAndPosition();
    }
    void Render() override
    {
        werase(win);
        const time_t now = time(nullptr);
        const char* date_time = ctime(&now);
        const std::string help_info = "PRESS ? FOR HELP";
        wbkgd(win, COLOR_PAIR(1));
        wattrset(win, COLOR_PAIR(1));
        mvwaddstr(win, 0, 1, date_time);
        mvwaddstr(win, 0, rect.w - int(help_info.size()) - 1,
                  help_info.c_str());
        wrefresh(win);
    }
};

SummaryView* g_summary_window;
SensorDetailView* g_sensor_detail_window;
DBusStatListView* g_dbus_stat_list_view;
FooterView* g_footer_view;
std::vector<DBusTopWindow*> g_views;

void UpdateWindowSizes()
{
    /* calculate window sizes and locations */
    getmaxyx(stdscr, maxy, maxx);
    halfx = maxx >> 1;
    halfy = maxy >> 1;
    for (DBusTopWindow* v : g_views)
    {
        v->OnResize(maxx, maxy);
    }
}

// Refresh all views, but do not touch data
void DBusTopRefresh()
{
    UpdateWindowSizes();
    for (DBusTopWindow* v : g_views)
    {
        v->Render();
    }
}

int main()
{
    // ncurses initialization
    initscr();  
    use_default_colors();
    start_color();
    noecho();

    // dbus-top initialization
    InitColorPairs();
    CreateWindows();
    // Initialize views
    g_summary_window = new SummaryView();
    g_sensor_detail_window = new SensorDetailView();
    g_dbus_stat_list_view = new DBusStatListView();
    g_footer_view = new FooterView();
    g_views.push_back(g_summary_window);
    g_views.push_back(g_sensor_detail_window);
    g_views.push_back(g_dbus_stat_list_view);
    g_views.push_back(g_footer_view);
    UpdateWindowSizes();
    DBusTopRefresh();

    return 0;
}
__attribute__((destructor)) void ResetWindowMode() {
  endwin();
  echo();
}


