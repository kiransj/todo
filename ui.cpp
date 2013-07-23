#include <ncurses.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "sqlite.h"

#include <vector>
using namespace std;

#define X COLS
#define Y LINES

#define MENU_BAR 2
#define NORMAL_TEXT_COLOR   3
#define SELECTED_TEXT_COLOR 4

int pr_index, pr_cur_line;
WINDOW *win_prlist, *win_scratch;
vector<PrInfo> prlist;

bool format_flag;
const char *format1 = "%-20lu %-20s %-20s%20s%s";
const char *format2 = "%lu %-10s %-10s%s%s";

static void finish(int sig)
{
    endwin();
    exit(0);
}

int ncurse_init(void)
{
    initscr();         
    keypad(stdscr, TRUE);
    raw();
    nl();
    cbreak();
    noecho();
    if (has_colors())
    {
        start_color();
    }
    refresh();
    signal(SIGINT, finish);     

    init_pair(SELECTED_TEXT_COLOR,  COLOR_BLACK, COLOR_WHITE);
    init_pair(NORMAL_TEXT_COLOR,  COLOR_WHITE, COLOR_BLACK);
    init_pair(MENU_BAR,  COLOR_WHITE, COLOR_BLUE);
}
 
void draw_box(int x, int y, int x1, int y1, const char *ptr)
{
    mvwhline(stdscr, y, x, 0, (x1 - x));
    mvwhline(stdscr, y1, x, 0, (x1 - x));
    mvwvline(stdscr, y, x, 0, (y1 -y));
    mvwvline(stdscr, y, x1, 0, (y1 -y));
    mvwaddch(stdscr, y, x, ACS_ULCORNER);
    mvwaddch(stdscr, y, x1, ACS_URCORNER);
    mvwaddch(stdscr, y1, x, ACS_LLCORNER);
    mvwaddch(stdscr, y1, x1, ACS_LRCORNER);
    if(NULL != ptr)
    {
        mvwprintw(stdscr, y, (x1+x - strlen(ptr))/2, "%s", ptr);
    }
    wrefresh(stdscr);
}   

void draw_ui(void)
{
    if(NULL == win_prlist || NULL == win_scratch)
    {
        win_prlist = subwin(stdscr, Y/2-2, X-2, 2, 1);
        scrollok(win_prlist, true);
    }
    format_flag = false;
    pr_index = pr_cur_line = 0;
    wclear(stdscr);
    wrefresh(stdscr);    
    draw_box(0, 1, X-1, Y/2, "PR List");

    wmove(stdscr, 0, 0);
    wchgat(stdscr, X, 0, MENU_BAR, 0);

    wrefresh(stdscr);
}

void updatePrList(void)
{
    SqlDB todo;    
    todo.connect("1.db");
    prlist = todo.get_all_pr();
    return;
}

void updatePrWindow(void)
{
    int y, x, length;
    wclear(win_prlist);
    getmaxyx(win_prlist, y, x);

    length = prlist.size();
    if(length > y)
        length = y;
    for(int i = 0; i < prlist.size(); i++)
    {
        char format[1024];                
        if(snprintf(NULL, 0, format1, prlist[i].pr_number, pr_state_tostring(prlist[i].pr_state), time_since(prlist[i].pr_date), " ", prlist[i].pr_desc) >= x)
        {
            format_flag = true;
            break;
        }
    }
    for(int i = 0; i < length; i++)
    {
        char str[1024];                
        snprintf(str, 1024, format_flag ? format2 : format1, prlist[i].pr_number, pr_state_tostring(prlist[i].pr_state), time_since(prlist[i].pr_date), " ", prlist[i].pr_desc);
        str[x-1] = 0;
        mvwprintw(win_prlist, pr_index+i, 0, "%s", str);
    }
    wmove(win_prlist, pr_cur_line, 0);
    wchgat(win_prlist, x, A_DIM, SELECTED_TEXT_COLOR, 0);
    wrefresh(win_prlist);
}

void PrWindowScrollUp(void)
{
    int cur_ln = pr_cur_line;
    int y, x;


    if(pr_cur_line == 0 && pr_index == 0)
    {
        return;
    }

    getmaxyx(win_prlist, y, x);
    wmove(win_prlist, pr_cur_line, 0);
    wchgat(win_prlist, x, 0, NORMAL_TEXT_COLOR, 0);

    if(cur_ln == 0)
    {
        wscrl(win_prlist, -1);
        pr_index--;
        wrefresh(win_prlist);
    }
    else
    {
        pr_cur_line--;
    }

    {
        char str[1024];                
        snprintf(str, 1024, format_flag ? format2 : format1, prlist[pr_index+pr_cur_line].pr_number, pr_state_tostring(prlist[pr_index+pr_cur_line].pr_state), time_since(prlist[pr_index+pr_cur_line].pr_date), " ", prlist[pr_index + pr_cur_line].pr_desc);
        str[x-1] = 0;
        mvwprintw(win_prlist, pr_cur_line , 0, "%s", str);
    }

    wmove(win_prlist, pr_cur_line, 0);
    wchgat(win_prlist, x, A_DIM, SELECTED_TEXT_COLOR, 0);
    wrefresh(win_prlist);
}
void PrWindowScrollDown(void)
{
    int cur_ln = pr_cur_line;
    int y, x;


    if((pr_index + cur_ln) == (prlist.size() - 1))
    {
        return;
    }

    getmaxyx(win_prlist, y, x);
    wmove(win_prlist, pr_cur_line, 0);
    wchgat(win_prlist, x, 0, NORMAL_TEXT_COLOR, 0);

    if(cur_ln == y-1)
    {
        wscrl(win_prlist, +1);
        pr_index++;
        wrefresh(win_prlist);
    }
    else
    {
        pr_cur_line++;
    }

    {
        char str[1024];                
        snprintf(str, 1024, format_flag ? format2 : format1, prlist[pr_index+pr_cur_line].pr_number, pr_state_tostring(prlist[pr_index+pr_cur_line].pr_state), time_since(prlist[pr_index+pr_cur_line].pr_date), " ", prlist[pr_index + pr_cur_line].pr_desc);
        str[x-1] = 0;
        mvwprintw(win_prlist, pr_cur_line , 0, "%s", str);
    }

    wmove(win_prlist, pr_cur_line, 0);
    wchgat(win_prlist, x, A_DIM, SELECTED_TEXT_COLOR, 0);
    wrefresh(win_prlist);
}
int main(int argc, char *argv[])
{
    ncurse_init();
    draw_ui();
    updatePrList();
    updatePrWindow();
    while(1)
    {
        int ch = getch();
        switch(ch)
        {
            case KEY_RESIZE:
                {
                    draw_ui();
                    updatePrWindow();
                }
                break;
            case KEY_DOWN:
                {
                    PrWindowScrollDown();
                    break;
                }
             case KEY_UP:
                {
                    PrWindowScrollUp();
                    break;
                }               
        }
    }
    finish(0);
}
