#include <ncurses.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sqlite.h"

#include <vector>
using namespace std;

#define X COLS
#define Y LINES

#define BORDER_COLOR                1
#define MENU_BAR                    2
#define NORMAL_TEXT_COLOR           3
#define SELECTED_TEXT_COLOR         4
#define LOG_NORMAL_TEXT_COLOR       5
#define LOG_ERROR_TEXT_COLOR        6

int pr_index, pr_cur_line;
WINDOW *win_prlist, *win_scratch;
vector<PrInfo> prlist;

bool format_flag;
const char *format1 = "%-25s| %-25s| %-25s| %2d| %s";
const char *format2 = "%-10s| %-10s| %-10s| %2d| %s";
#define format_pr(pr) pr.pr_number, pr_state_tostring(pr.pr_state), time_since(pr.pr_date), pr.num_desc, pr.pr_header

static void finish(int sig)
{
    if(win_prlist != NULL)
        delwin(win_prlist);
    if(win_scratch!= NULL)
        delwin(win_scratch);   
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

    init_pair(BORDER_COLOR,  COLOR_RED, COLOR_BLACK);
    init_pair(SELECTED_TEXT_COLOR,  COLOR_BLACK, COLOR_WHITE);
    init_pair(NORMAL_TEXT_COLOR,  COLOR_WHITE, COLOR_BLACK);
    init_pair(MENU_BAR,  COLOR_WHITE, COLOR_BLUE);

    init_pair(LOG_NORMAL_TEXT_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(LOG_ERROR_TEXT_COLOR,  COLOR_RED, COLOR_BLACK);
}

void draw_box(int x, int y, int x1, int y1, const char *ptr)
{
    attrset(COLOR_PAIR(BORDER_COLOR));
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

void log(const bool error, const char *format, ...)
{
    const int color = (error) ? LOG_ERROR_TEXT_COLOR : LOG_NORMAL_TEXT_COLOR;
    va_list list;
    va_start(list, format);    
    move(Y-1, 0);
    clrtoeol();
    move(Y-1, 0);
    wchgat(stdscr, X, 0, color, 0);
    attrset(COLOR_PAIR(color));
    vw_printw(stdscr, format, list);
    wrefresh(stdscr);
}
void print_menu_bar(void)
{
    wmove(stdscr, 0, 0);
    wchgat(stdscr, X, 0, MENU_BAR, 0);
    attrset(COLOR_PAIR(MENU_BAR));
    mvwprintw(stdscr, 0, 0, "%s %20s %20s %20s %20s", "F1 NewPR", "F2 UpdatePR", "F3 DeletePR", "F4 Exit", "F5 Search");
}
void draw_ui(void)
{
    if(NULL != win_prlist || NULL != win_scratch)
    {
        werase(win_prlist);
        wrefresh(win_prlist);
        werase(win_scratch);
        wrefresh(win_scratch);
        delwin(win_prlist);
        delwin(win_scratch);
    }
    win_prlist = subwin(stdscr, Y>>2, X-2, 2, 1);
    win_scratch = subwin(stdscr, (3*Y)/4 - 4, X-2, Y/4+3, 1);
    scrollok(win_prlist, true);
    format_flag = false;
    pr_index = pr_cur_line = 0;
    wclear(stdscr);
    werase(stdscr);
//    box(win_prlist, 0, 0);
//    box(win_scratch, 0, 0);
    wrefresh(win_scratch);
    wrefresh(win_prlist);
    print_menu_bar();
    draw_box(0, 1, X-1, Y/4+2, "PR List");

    wrefresh(stdscr);
}

SqlDB OpenDB(void)
{
    SqlDB todo;
    if(todo.connect("1.db") == false)
    {
        log(true, "%s", todo.last_error()); 
    }
    return todo;
}
void updatePrList(void)
{
    SqlDB todo = OpenDB();
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
        if(snprintf(NULL, 0, format1, format_pr(prlist[i])) >= x)
        {
            format_flag = true;
            break;
        }
    }
    wattrset(win_prlist, COLOR_PAIR(NORMAL_TEXT_COLOR));
    for(int i = 0; i < length; i++)
    {
        char str[1024];
        snprintf(str, 1024, format_flag ? format2 : format1, format_pr(prlist[i]));
        str[x-1] = 0;
        mvwprintw(win_prlist, i, 0, "%s", str);
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
        snprintf(str, 1024, format_flag ? format2 : format1, format_pr(prlist[pr_index+pr_cur_line]));
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
        snprintf(str, 1024, format_flag ? format2 : format1, format_pr(prlist[pr_index+pr_cur_line]));
        str[x-1] = 0;
        mvwprintw(win_prlist, pr_cur_line, 0, "%s", str);
    }

    wmove(win_prlist, pr_cur_line, 0);
    wchgat(win_prlist, x, A_DIM, SELECTED_TEXT_COLOR, 0);
    wrefresh(win_prlist);
}

char* read_string(WINDOW *win, const bool number, int min_length )
{
    int i = 0;
    static char temp[128];
    while(1)
    {
        int ch = wgetch(win);
        if(ch == 27)
        {
            return NULL;
        }
        else if(isdigit(ch) || (ch >= 0x20 && ch <= 0x7E) && (number == false) )
        {
            temp[i] = ch;
            wprintw(win, "%c", ch);
            wrefresh(win);
            i++;
        }
        else if(ch == 127 && i > 0)
        {
            wprintw(win, "\b \b");
            wrefresh(win);       
            i--;
        }
        else if(ch == '\n')
        {
            if(min_length == 0 || (i >= min_length))
            {
                temp[i] = 0;
                return (temp); 
            }
            else
            {
                int y, x;
                getyx(win, y, x);
                log(true, "min length of string should be %d", min_length);
                wmove(win, y, x);
            }
        }
    }
}

void ReadNewPr(void)
{
    SqlDB todo = OpenDB();
    char pr_number[16], pr_header[64];
    char *ptr;
    wclear(win_scratch);
    mvwprintw(win_scratch, 1, 1, "Enter the pr number : ");          
    ptr = read_string(win_scratch, true, 6);
    if(ptr == NULL)
    {
        goto EXIT;
    }
    strcpy(pr_number, ptr);
    if(todo.get_pr(pr_number, NULL) == true)
    {
        log(true, "Error: PR %s already exists in the list", pr_number);
    }
    else
    {
        mvwprintw(win_scratch, 2, 1, "Enter the pr header: ");
        ptr = read_string(win_scratch, false, 10);
        if(NULL == ptr)
        {
            goto EXIT;
        }
           
        strcpy(pr_header, ptr);  
        if(todo.add_new_pr(NewPR(pr_number, pr_header)) == false)
        {
            log(true, "Error: %s", todo.last_error());
        }
        updatePrList();
        updatePrWindow();
    }
EXIT:    
    wclear(win_scratch);
    wrefresh(win_scratch);
    return;
}

const char * date_to_str(time_t t)
{
    struct tm *timeinfo;
    static char buffer[1024];
    timeinfo = localtime(&t);
    strftime(buffer, 1024, "%e-%b-%y %I%p", timeinfo);
    return buffer;
}
void show_pr(const char *pr_number)
{
    PrInfo pr;
    SqlDB todo = OpenDB();
    if(todo.get_pr(pr_number, &pr) == false)
    {
        log(true, "%s", todo.last_error());
        return;        
    }

    wclear(win_scratch);
    mvwprintw(win_scratch, 1, 1, "Pr Number: %s", pr.pr_number);
    mvwprintw(win_scratch, 2, 1, "Pr Header: %s", pr.pr_header);
    mvwprintw(win_scratch, 3, 1, "Pr  State: %s", pr_state_tostring(pr.pr_state));
    mvwprintw(win_scratch, 4, 1, "Pr   Date: %s", date_to_str(pr.pr_date));
    for(int i = 0; i < pr.pr_desc.size(); i++)
    {
        mvwprintw(win_scratch, 6+i, 1, "    %s> %s", date_to_str(pr.pr_desc[i].updated_date), pr.pr_desc[i].pr_desc);
    }
    wrefresh(win_scratch);
    return;
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
            case '\n':
                {
#if 0                    
                    PrInfo pr;
                    SqlDB todo = OpenDB();
                    if(todo.get_pr( prlist[pr_index + pr_cur_line].pr_number, &pr))
                        log(false, "selected text : %s %s", pr.pr_header);
                    else
                        log(true, "%s", todo.last_error());
#endif
                    show_pr(prlist[pr_index+pr_cur_line].pr_number);
                    break;
                }
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
            case KEY_F(1):
                {
                    ReadNewPr();
                    break;
                }
            case 27: /*ESC Key*/ 
            case KEY_F(4):
                {
                    finish(0);                    
                }
            default:
                {
                    log(false, "key pressed %d", ch);
                    break;
                }
        }
    }
    finish(0);
}
