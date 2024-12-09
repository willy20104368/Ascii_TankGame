#include <iostream>
#include <thread>
#include <chrono>
#include "Objects.h"
#include <ncurses.h>
#include <condition_variable>

std::mutex bulletMutex;                   
std::condition_variable bulletCondition;

bool gameRunning = true;
bool bulletFired = false;

void bulletController(Tank& playerTank, Map& gameMap){
    while (true) {
        std::unique_lock<std::mutex> lock(bulletMutex);
        bulletCondition.wait(lock, [] { return !gameRunning || bulletFired; });

        if (!gameRunning) break;

        playerTank.shoot(gameMap);
        bulletFired = false;

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
}

void updateGameLogic(Tank& playerTank, Map& gameMap) {
    char command;

    gameMap.display();

    while ((command = getch()) != 'q'){
        // std::cin >> command;
        if(command == KEY_UP ||command == 'W'|| command == 'w')
            playerTank.move(0, -1, gameMap); // up
        else if(command == KEY_LEFT || command == 'A' || command == 'a')
            playerTank.move(-1, 0, gameMap); // left
        else if(command == KEY_DOWN || command == 'S' || command == 's')
            playerTank.move(0, 1, gameMap); // down
        else if(command == KEY_RIGHT || command == 'D' || command == 'd')
            playerTank.move(1, 0, gameMap);  // right
        else if(command == ' '){
            std::lock_guard<std::mutex> lock(bulletMutex);
            bulletFired = true;
            bulletCondition.notify_one();
        }

        gameMap.display();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

    }

    std::lock_guard<std::mutex> lock(bulletMutex);
    gameRunning = false;
    bulletCondition.notify_all();
}

int main() {
    // initialize tank & map
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    Map gameMap(50, 30);
    Tank playerTank(1, 1, 'T', 3);
    playerTank.render(gameMap);


    gameMap.addObstacle();


    std::thread logicThread(updateGameLogic, std::ref(playerTank), std::ref(gameMap));
    std::thread bulletThread(bulletController, std::ref(playerTank), std::ref(gameMap));
    // std::thread renderThread(renderGame, std::ref(gameMap));

    
    logicThread.join();
    bulletThread.join();
    endwin();
    return 0;
}

