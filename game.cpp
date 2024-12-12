#include <iostream>
#include <thread>
#include <chrono>
#include "Objects.h"
#include <ncurses.h>
#include <condition_variable>

std::mutex bulletMutex;                   
std::condition_variable bulletCondition;
std::mutex tankMtx;
std::mutex bulletMtx;

bool gameRunning = true;
// bool bulletFired = false;  // fired
bool bulletActive = false; // constrain that only one bullet can exist

// void bulletController(Tank& playerTank, Map& gameMap){
//     while (true) {
//         std::unique_lock<std::mutex> lock(bulletMutex);
//         bulletCondition.wait(lock, [] { return !gameRunning || bulletFired; });

//         if (!gameRunning) break;

//         bulletActive = true;
//         lock.unlock();
//         playerTank.shoot(gameMap);

//         lock.lock();
//         bulletActive = false;
//         bulletFired = false;
//         std::this_thread::sleep_for(std::chrono::milliseconds(16));
//     }
    
// }

// void collisionDetector(){
//     while(true){
//         std::lock_guard<std::mutex> lock(tankMtx);
//         for(auto const& t:Tankpool){
//             std::lock_guard<std::mutex> lock(bulletMtx);
//             for()
//         }
//     }
// }

void bulletController(Bullet *b, Map& gameMap){
    std::unique_lock<std::mutex> lock(bulletMutex);
    bulletActive = true;
    lock.unlock();

    b->shoot(gameMap);

    lock.lock();
    bulletActive = false;
    lock.unlock();
    // bullet_pool.pop();
    delete b;
}


void updateGameLogic(Tank *t, Map& gameMap) {

    char command;
    bool bullet_active = false;

    while ((command = getch()) != 'q'){
        
        if(command == KEY_UP ||command == 'W'|| command == 'w')
            t->move(0, -1, gameMap); // up
        else if(command == KEY_LEFT || command == 'A' || command == 'a')
            t->move(-1, 0, gameMap); // left
        else if(command == KEY_DOWN || command == 'S' || command == 's')
            t->move(0, 1, gameMap); // down
        else if(command == KEY_RIGHT || command == 'D' || command == 'd')
            t->move(1, 0, gameMap);  // right
        else if(command == ' ' ){
            std::unique_lock<std::mutex> lock(bulletMutex);
            bullet_active = bulletActive;
            lock.unlock();
            if(!bulletActive){
                Bullet *b = new Bullet(t->getX(), t->getY(), t->getId(), t->getDirection());
                std::thread bulletThread(bulletController, b, std::ref(gameMap));
                std::unique_lock<std::mutex> lock(bulletMtx);
                bulletThread.detach();

            }
            
            // std::lock_guard<std::mutex> lock(bulletMutex);
            // if(!bulletActive){
            //     bulletFired = true;
            //     bulletCondition.notify_one();
            // }
            
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));


    }

    std::lock_guard<std::mutex> lock(bulletMutex);
    gameRunning = false;
    bulletCondition.notify_all();
}



void renderGame(Map& gameMap){
    while(true){
        if (!gameRunning) break;

        gameMap.display();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

// controll ai tank moving
void aiTankController(Tank *aiTank, Tank *playerTank, Map& gameMap){
    
    while(aiTank->is_alive()){
        
        if (!gameRunning) break;

        int dx = playerTank->getX() - aiTank->getX();
        int dy = playerTank->getY() - aiTank->getY();
        
        if(dx < 0) aiTank->move(-1, 0, gameMap); // left
        else if (dx > 0) aiTank->move(1, 0, gameMap); // right

        if(dy < 0) aiTank->move(0, -1, gameMap); // up
        else if (dy > 0) aiTank->move(0, 1, gameMap); // down
        
        // attact
        // if(abs(dx) <= 10 && abs(dy) <= 10){
        //     shoot
        // }
        std::this_thread::sleep_for(std::chrono::milliseconds(700)); // speed of ai tank
    }
    
}

int main(int argc, char** argv) {

    int aiTank_n;
    if(argc == 2){
        int n = atoi(argv[1]);
        if(n >= 0 && n <= 3)
            aiTank_n = n;
        else
            aiTank_n = 0;
    }
    else{
        aiTank_n = 2;
    }
    // initialize tank & map
    initscr();
    cbreak();
    noecho();
    // start_color();
    // use_default_colors();
    keypad(stdscr, TRUE);


    // map setting
    Map gameMap(80, 30);
    

    // tank settings
    // Tank *playerTank = new Tank(1, 1, 'O', 3, 0);
    // Tank *aiTank = new Tank(75, 25, 'A', 1, 1);
    // std::unique_lock<std::mutex> lock(tankMtx);
    // Tankpool.emplace_back(playerTank);
    // Tankpool.emplace_back(aiTank);
    // lock.unlock();
    // playerTank->render(gameMap);
    // aiTank->render(gameMap);
    ObjectsPool* objpool = new ObjectsPool();
    objpool->createTank(gameMap, aiTank_n);
    std::vector<std::thread> ThreadPool;
    std::vector<Tank*> tankPool = objpool->getTankPool();
    for(int i = 1; i <= aiTank_n; ++i){
        ThreadPool.emplace_back(std::thread(aiTankController, tankPool[i], tankPool[0], std::ref(gameMap)));
    }
    
    gameMap.addObstacle();

    std::thread logicThread(updateGameLogic, objpool->getplayer(), std::ref(gameMap));
    // std::thread bulletThread(bulletController, std::ref(playerTank), std::ref(gameMap));
    std::thread renderThread(renderGame, std::ref(gameMap));
    // std::thread aiTankThread(aiTankController, aiTank, playerTank, std::ref(gameMap));
    
    logicThread.join();
    // bulletThread.join();
    renderThread.join();
    // aiTankThread.join();
    for (auto& t : ThreadPool) {
        if (t.joinable()) { 
            t.join();
        }
    }
    endwin();
    return 0;
}

