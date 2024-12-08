#include <iostream>
#include <thread>
#include <chrono>
#include "Objects.h"
#include <ncurses.h>


void updateGameLogic(Tank& playerTank, Map& gameMap) {
    char command;

    gameMap.display();

    while ((command = getch()) != 'q'){
        // std::cin >> command;
        if(command == KEY_UP ||command == 'W'|| command == 'w')
            playerTank.move(0, -1, gameMap); // up
        else if(command == KEY_DOWN || command == 'A' || command == 'a')
            playerTank.move(-1, 0, gameMap); // down
        else if(command == KEY_LEFT || command == 'S' || command == 's')
            playerTank.move(0, 1, gameMap); // left
        else if(command == KEY_RIGHT || command == 'D' || command == 'd')
            playerTank.move(1, 0, gameMap);  // right

        gameMap.display();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 60FPS，每幀間隔16ms

    }

    // endwin();
}

int main() {
    // initialize tank & map
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    Map gameMap(50, 20);
    Tank playerTank(1, 1, 'T');
    playerTank.render(gameMap);

    // 初始化障礙物
    gameMap.addObstacle();


    
    // 創建兩個執行緒
    std::thread logicThread(updateGameLogic, std::ref(playerTank), std::ref(gameMap));
    // std::thread renderThread(renderGame, std::ref(gameMap));

    // 等待執行緒結束（實際情況中，你可以用其他條件控制退出）
    logicThread.join();
    // renderThread.join();
    endwin();
    return 0;
}

