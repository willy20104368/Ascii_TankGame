#include <iostream>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include <condition_variable>
#include "Objects.h"
#include "logger.h"
#include <atomic>

#define DEBUG

#ifdef DEBUG
    #define LOG(message) Logger::getInstance().writeLog(message)
#else
    #define LOG(message) // do nothing
#endif


std::mutex bulletMutex;                   
std::vector<bool> bulletActive(4, false);
std::atomic<bool> gameRunning(true);
std::atomic<int> aiTank_n(1);
std::ofstream logFile("log.txt");
ThreadPool threadPool(12);

std::condition_variable gameCondition;
std::mutex gameMutex;


void displayMenu(int &health) {
    clear();
    mvprintw(5, 10, "Welcome to Tank Game!");

    // Validate AI Tank Count input
    int aiTankCount = 0;
    do {
        mvprintw(7, 10, "Enter AI Tank Count (1~3): ");
        clrtoeol(); // Clear line to handle overwrites
        echo(); // Enable echoing of user input
        char input[10];
        getstr(input); // Get user input for AI Tank Count
        try {
            aiTankCount = std::stoi(input);
        } catch (std::invalid_argument &) {
            aiTankCount = 0; // Invalid input
        }
        if (aiTankCount < 1 || aiTankCount > 3) {
            mvprintw(8, 10, "Invalid input. Please enter a number between 1 and 3.");
            refresh();
        }
    } while (aiTankCount < 1 || aiTankCount > 3);

    aiTank_n = aiTankCount;

    // Validate Tank Health input
    int tankHealth = 0;
    do {
        clear();
        mvprintw(5, 10, "Welcome to Tank Game!");
        mvprintw(7, 10, "Enter Tank Health (1~9): ");
        clrtoeol(); // Clear line to handle overwrites
        char input[10];
        getstr(input); // Get user input for Tank Health
        try {
            tankHealth = std::stoi(input);
        } catch (std::invalid_argument &) {
            tankHealth = 0; // Invalid input
        }
        if (tankHealth < 1 || tankHealth > 9) {
            mvprintw(8, 10, "Invalid input. Please enter a number between 1 and 9.");
            refresh();
        }
    } while (tankHealth < 1 || tankHealth > 9);

    health = tankHealth;
    noecho(); // Disable echoing again

    clear();
    mvprintw(5, 10, "Settings Updated!");
    mvprintw(7, 10, "AI Tank Count: %d", aiTankCount);
    mvprintw(8, 10, "Tank Health: %d", health);
    mvprintw(10, 10, "Press any key to start the game...");
    refresh();
    getch(); // Wait for user to press a key
}

void displayEnd(const char* s){
    clear();
    mvprintw(7, 10, s);
    mvprintw(10, 10, "Press any key to end the game...");
    refresh();
    getch(); // Wait for user to press a key
}

// void collisionDetector(ObjectsPool* objpool, Map& gameMap){
//     while(gameRunning){
//         // std::lock_guard<std::mutex> lock(bulletMutex);
//         // wait for bullet fired
//         // bulletCondition.wait(lock, [] { return !gameRunning || collision; });

//         //detect the collision between bullet and tank
//         std::vector<Tank*> tankPool = objpool->getTankPool();
//         // std::queue<Bullet*> bulletPool = objpool->getBulletPool();
//         Bullet* b = objpool->getBullet();
//         if(b){
//             for(auto const& t:tankPool){
//                 if(t->getId() != b->getId() && t->is_alive() && t->getX() == b->getX() && t->getY() == b->getY()){
//                     if(t->getId() != 0)
//                         t->takeDamage(gameMap);
//                     std::string tmp = std::to_string(t->getId()) + "take damage.";
//                     LOG(tmp);
//                 }
                
//             }
//         }
//         // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
//     }
//     LOG("end collisionDetector.\n");
// }

void bulletController(ObjectsPool* objpool, Map& gameMap){

    Bullet* b = objpool->getBullet();

    std::unique_lock<std::mutex> lock(bulletMutex);
    bulletActive[b->getId()] = true;
    lock.unlock();

    
    b->shoot(objpool->getTankPool(), gameMap);

    lock.lock();
    bulletActive[b->getId()] = false;
    lock.unlock();
  
    LOG("end bulletController.\n" );
}


void updateGameLogic(ObjectsPool *objpool, Map& gameMap) {

    char command;
    bool bullet_active = false;
    Tank *t = objpool->getplayer();

    while (t->is_alive() && gameRunning){
        command = getch();
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
            bullet_active = bulletActive[0];
            lock.unlock();
            if(!bullet_active){
                objpool->addBullet(t->getX(), t->getY(), t->getId(), t->getDirection());
                LOG("create bulletController.\n" );
                threadPool.enqueue([&]() { bulletController(objpool, gameMap); });

            }
            
            
        }
        else if(command == 'q'){
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));


    }

    gameRunning = false;
    LOG("end updateGameLogic.\n" );
}



void renderGame(Map& gameMap){
    while(gameRunning){
        gameMap.display();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    if(aiTank_n > 0)
        displayEnd("You lose........");
    else
        displayEnd("You Win !!!");
    gameCondition.notify_one();
    LOG("end renderGame.\n");
}

// controll ai tank moving
void aiTankController(ObjectsPool* objpool, Tank *aiTank, Tank *playerTank, Map& gameMap){
    int id = aiTank->getId();
    bool bullet_active = false;
    while(aiTank->is_alive() && gameRunning){
        
        if (!gameRunning) break;

        int dx = playerTank->getX() - aiTank->getX();
        int dy = playerTank->getY() - aiTank->getY();
        
        if(dx < 0) aiTank->move(-1, 0, gameMap); // left
        else if (dx > 0) aiTank->move(1, 0, gameMap); // right

        if(dy < 0) aiTank->move(0, -1, gameMap); // up
        else if (dy > 0) aiTank->move(0, 1, gameMap); // down
        

        // attack
        if (abs(dx) <= 10 && abs(dy) <= 10) {
            std::unique_lock<std::mutex> lock(bulletMutex);
            bullet_active = bulletActive[id];
            lock.unlock();
            if(!bullet_active){
                objpool->addBullet(aiTank->getX(), aiTank->getY(), id, aiTank->getDirection());
                std::string tmp = std::to_string(id) + "create AI bulletController." ;
                threadPool.enqueue([&]() { bulletController(objpool, gameMap); });
                LOG(tmp);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // speed of ai tank
    }
    // aiTank->killed(gameMap);
    aiTank_n--;
    if(aiTank_n <= 0)
        gameRunning = false;
    LOG("end aiTankController.\n");
}



int main(int argc, char** argv) {

    int health = 1; // -h
    
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    displayMenu(health);
    // map setting
    Map gameMap(80, 30);
    gameMap.addObstacle();

    ObjectsPool* objpool = new ObjectsPool();
    objpool->createTank(gameMap, aiTank_n, health);
    std::vector<Tank*> tankPool = objpool->getTankPool();

    
    

    threadPool.enqueue([&]() { updateGameLogic(objpool, std::ref(gameMap)); });
    threadPool.enqueue([&]() { renderGame(std::ref(gameMap)); });


    // if not ref i, it will cause Segmentation fault (core dumped)
    for(int i = 1; i <= aiTank_n; ++i){
        threadPool.enqueue([&, i]() { aiTankController(objpool, tankPool[i], tankPool[0], std::ref(gameMap)); });
    }
    
    
    {
        std::unique_lock<std::mutex> lock(gameMutex);
        gameCondition.wait(lock, [] { return !gameRunning; });
    }

    delete objpool;
    

    // displayEnd();

    endwin();
    LOG("end main.\n");
    
    return 0;
}

