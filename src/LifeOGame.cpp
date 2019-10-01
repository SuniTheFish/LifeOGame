// LifeOGame.cpp : This file contains the 'main' function. Program execution begins and ends there.

// std
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <thread>
#include <chrono>

// curses
// this must be before the local includes 
// for definitions to have an effect
#ifdef _WIN32
    #define PDC_NCMOUSE
    #include <curses.h>
#else
    #include <ncurses.h>
#endif // _WIN32


// local
#include "Life.h"
#include "StatusMessage.h"

struct ProgramStatusData
{
    Life& board;
    StatusMessage& mouseLoc;
    StatusMessage& pauseStatus;
    bool shouldExit{ false };
    bool paused{ true };
    bool step{ false };
    bool mouseDown{ false };
};

void processInput(ProgramStatusData& data);
void processMouseState(ProgramStatusData& data, MEVENT& lastEve, MEVENT& eve);

int main(int argc, char* argv[])
{
    initscr();
    curs_set(0);
    cbreak();
    noecho();
    nodelay(stdscr, true);
    keypad(stdscr, true);

    mousemask(REPORT_MOUSE_POSITION | BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_CLICKED, nullptr);

    int rows;
    int cols;
    getmaxyx(stdscr, rows, cols);

    Life board;
    
    try
    {
        board = Life{ rows, cols / 2, (argc > 1) ? argv[1] : "" };
    }
    catch (const std::length_error& e)
    {
        std::cerr << e.what() << '\n';
    }

    StatusMessage pauseStatus{ rows - 1, Align::CENTER, "PAUSED" };

    StatusMessage mouseLoc{ rows - 1, Align::RIGHT, "" };

    ProgramStatusData statusData{ board, mouseLoc, pauseStatus };

    MEVENT eve;
    MEVENT lastEve{};
    while (!statusData.shouldExit)
    {
        auto start = std::chrono::high_resolution_clock::now();

        processInput(statusData);

        processMouseState(statusData, lastEve, eve);

        using namespace std::string_literals;
        mouseLoc.setMessage("X: "s + std::to_string(eve.x) + " Y: "s + std::to_string(eve.y));

        // drawing routines
        erase();

        // draw the paused message
        if (statusData.paused)
            pauseStatus.draw(stdscr);

        // draw the mouse x & y
        mouseLoc.draw(stdscr);

        // draw the actual game
        board.draw(stdscr);

        // draw the cursor
        for (int i{ 0 }; i < 2; ++i)
        {
            move(eve.y, eve.x * 2 + i);
            addch(inch() | A_REVERSE);
        }

        // refresh the screen
        refresh();

        // wait until next frame
        std::this_thread::sleep_until(start + std::chrono::duration<double>{ 1.0 / 15 });
    }

    endwin();
    return 0;
}

void processInput(ProgramStatusData& data)
{
    int ch;
    while ((ch = getch()) != ERR)
    {
        switch (ch)
        {
        case ' ':
            data.paused ^= true;
            break;
        case 'q':
            data.shouldExit = true;
            break;
        case 'r':
            data.board.clear();
            break;
        case 's':
            data.step = true;
            break;
        case KEY_MOUSE:
            MEVENT eve;
            if (getmouse(&eve) == OK)
            {
                if (eve.bstate & BUTTON1_PRESSED)
                    data.mouseDown = true;
                else if (eve.bstate & BUTTON1_RELEASED)
                    data.mouseDown = false;

                if (eve.bstate & BUTTON1_CLICKED)
                    data.board.togglePixel(eve.y, eve.x / 2);
            }
            break;
        case KEY_RESIZE:
            resize_term(0, 0);
            curs_set(0);
            int rows;
            int cols;
            getmaxyx(stdscr, rows, cols);
            data.board.resize(rows, cols / 2);
            data.pauseStatus.moveToRow(rows - 1);
            data.mouseLoc.moveToRow(rows - 1);
            break;
        }
    }

    if (!data.paused)
        data.board.update();
    else if (data.step)
    {
        data.board.update();
        data.step = false;
    }
}

void processMouseState(ProgramStatusData& data, MEVENT& lastEve, MEVENT& eve)
{
    if (getmouse(&eve) == OK)
    {
        eve.x = eve.x / 2;

        if (data.mouseDown)
        {
            if (lastEve.x != eve.x || lastEve.y != eve.y)
            {
                data.board.togglePixel(eve.y, eve.x);
            }
            lastEve = eve;
        }
    }
}