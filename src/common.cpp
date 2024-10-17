// header
#include "common.h"

// std
#include <iostream>

#ifdef _WIN32
#include <conio.h>
#include <windows.h> // For system("cls")
#else
#include <cstdlib> // For system("clear")
#include <termios.h>
#include <unistd.h>
#endif

char getch() {
#ifdef _WIN32
  return _getch();
#else
  struct termios oldt, newt;
  char ch;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

  return ch;
#endif
}

void clear_screen() {
#ifdef _WIN32
  system("cls"); // Clear screen for Windows
#else
  system("clear"); // Clear screen for Unix-like systems
#endif
}

void hideCursor() {
#ifdef _WIN32
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_CURSOR_INFO cursorInfo;
  GetConsoleCursorInfo(hConsole, &cursorInfo);
  cursorInfo.bVisible = FALSE; // Set the cursor visibility to false
  SetConsoleCursorInfo(hConsole, &cursorInfo);
#else
  std::cout << "\033[?25l"; // ANSI escape code to hide the cursor
  std::cout.flush();        // Ensure the command is sent to the terminal
#endif
}