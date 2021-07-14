#include <ncurses.h>

void PrintHello()
{
    border('|', '|', '-', '-', '+', '+', '+', '+');
    mvaddstr(1, 1, "Hello World");
    mvaddstr(2, 1, "This is the beggining of the proposed dbus-top tool.");
    mvaddstr(3, 1, "Thank you.");
}
int main()
{

    initscr(); /* Start curses mode 		  */
    PrintHello();
    refresh(); /* Print it on to the real screen */
    getch();   /* Wait for user input */
    endwin();  /* End curses mode		  */

    return 0;
}
