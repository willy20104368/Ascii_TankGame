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
std::mutex collisionMutex;
std::ofstream logFile("log.txt");
// ThreadPool threadPool(std::thread::hardware_concurrency());
ThreadPool threadPool(12);

// }

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
            bullet_active = bulletActive[0];
            lock.unlock();
            if(!bullet_active){
                objpool->addBullet(t->getX(), t->getY(), t->getId(), t->getDirection());
                LOG("create bulletController.\n" );
                threadPool.enqueue([&]() { bulletController(objpool, gameMap); });
                // std::thread bulletThread(bulletController, objpool, std::ref(gameMap));
                // bulletThread.detach();

            }
            
            
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));


    }

    gameRunning = false;
    LOG("end updateGameLogic.\n" );

    // std::cout << "end !!!" << std::endl;
    // bulletCondition.notify_all();
}



void renderGame(Map& gameMap){
    while(gameRunning){
        gameMap.display();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
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
    LOG("end aiTankController.\n");
}

int main(int argc, char** argv) {

    int aiTank_n = 2; // -n
    int health = 1; // -h
    if(argc > 1){
        for(int i = 1; i < argc; i+=2){
            // LOG(argv[i]);
            if (argv[i][0] == '-') {
                for (int j = 1; argv[i][j] != '\0'; j++){
                    switch (argv[i][j]) {
                        case 'n':
                            aiTank_n = atoi(argv[i+1]);
                            break;
                        case 'h':
                            health = atoi(argv[i+1]);
                            break;
                        default:
                            aiTank_n = 2;
                            health = 1;
                    }
                }
            }
        }
    }
    std::string tmp1 = "health" + std::to_string(health);
    std::string tmp2 = "aiTank_n:" + std::to_string(aiTank_n);
    LOG(std::to_string(std::thread::hardware_concurrency()));
    LOG(tmp1);
    LOG(tmp2);
    // initialize tank & map
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);


    // map setting
    Map gameMap(80, 30);
    
    ObjectsPool* objpool = new ObjectsPool();
    objpool->createTank(gameMap, aiTank_n, health);
    std::vector<Tank*> tankPool = objpool->getTankPool();

    
    gameMap.addObstacle();

    threadPool.enqueue([&]() { updateGameLogic(objpool, std::ref(gameMap)); });
    threadPool.enqueue([&]() { renderGame(std::ref(gameMap)); });

    // threadPool.enqueue([&]() { aiTankController(objpool, tankPool[1], tankPool[0], std::ref(gameMap)); });
    // threadPool.enqueue([&]() { aiTankController(objpool, tankPool[2], tankPool[0], std::ref(gameMap)); });

    // if not ref i, it will cause Segmentation fault (core dumped)
    for(int i = 1; i <= aiTank_n; ++i){
        threadPool.enqueue([&, i]() { aiTankController(objpool, tankPool[i], tankPool[0], std::ref(gameMap)); });
    }
    
    
    while (gameRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    objpool->~ObjectsPool();
    
    endwin();
    LOG("end main.\n");
    
    return 0;
}

