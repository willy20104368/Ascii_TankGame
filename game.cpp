#include <iostream>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include <condition_variable>
#include "Objects.h"
#include "logger.h"


#define DEBUG

#ifdef DEBUG
    #define LOG(message) Logger::getInstance().writeLog(message)
#else
    #define LOG(message) // do nothing
#endif


std::mutex bulletMutex;                   
std::condition_variable bulletCondition;
std::mutex tankMtx;
std::mutex bulletMtx;

bool gameRunning = true;
bool collision = false;  // collision
bool bulletActive = false; // constrain that only one bullet can exist

std::ofstream logFile("log.txt");

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

void collisionDetector(ObjectsPool* objpool, Map& gameMap){
    while(gameRunning){
        // std::lock_guard<std::mutex> lock(bulletMutex);
        // wait for bullet fired
        // bulletCondition.wait(lock, [] { return !gameRunning || collision; });

        //detect the collision between bullet and tank
        std::vector<Tank*> tankPool = objpool->getTankPool();
        // std::queue<Bullet*> bulletPool = objpool->getBulletPool();
        Bullet* b = objpool->getBullet();
        if(b){
            for(auto const& t:tankPool){
                if(t->getId() != b->getId() && t->is_alive() && t->getX() == b->getX() && t->getY() == b->getY()){
                    t->attacked(gameMap);
                }
                
            }
        }
        // std::this_thread::sleep_for(std::chrono::milliseconds(2));
        
    }
    LOG("end collisionDetector.\n");
}

void bulletController(ObjectsPool* objpool, Map& gameMap){
    std::unique_lock<std::mutex> lock(bulletMutex);
    bulletActive = true;
    lock.unlock();

    Bullet* b = objpool->getBullet();
    b->shoot(gameMap);

    lock.lock();
    bulletActive = false;
    lock.unlock();
    // bullet_pool.pop();
    objpool->delBullet();

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
            bullet_active = bulletActive;
            lock.unlock();
            if(!bulletActive){
                objpool->addBullet(t->getX(), t->getY(), t->getId(), t->getDirection());
                LOG("create bulletController.\n" );
                std::thread bulletThread(bulletController, objpool, std::ref(gameMap));
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
void aiTankController(Tank *aiTank, Tank *playerTank, Map& gameMap){
    
    while(aiTank->is_alive() && gameRunning){
        
        if (!gameRunning) break;

        int dx = playerTank->getX() - aiTank->getX();
        int dy = playerTank->getY() - aiTank->getY();
        
        if(dx < 0) aiTank->move(-1, 0, gameMap); // left
        else if (dx > 0) aiTank->move(1, 0, gameMap); // right

        if(dy < 0) aiTank->move(0, -1, gameMap); // up
        else if (dy > 0) aiTank->move(0, 1, gameMap); // down
        
        // attack
        // if(abs(dx) <= 10 && abs(dy) <= 10){
        //     shoot
        // }
        std::this_thread::sleep_for(std::chrono::milliseconds(700)); // speed of ai tank
    }
    aiTank->killed(gameMap);
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
    LOG(tmp1);
    LOG(tmp2);
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
    objpool->createTank(gameMap, aiTank_n, health);
    std::vector<std::thread> ThreadPool;
    std::vector<Tank*> tankPool = objpool->getTankPool();
    for(int i = 1; i <= aiTank_n; ++i){
        ThreadPool.emplace_back(std::thread(aiTankController, tankPool[i], tankPool[0], std::ref(gameMap)));
    }
    
    gameMap.addObstacle();

    std::thread logicThread(updateGameLogic, objpool, std::ref(gameMap));
    // std::thread bulletThread(bulletController, std::ref(playerTank), std::ref(gameMap));
    std::thread renderThread(renderGame, std::ref(gameMap));
    // std::thread aiTankThread(aiTankController, aiTank, playerTank, std::ref(gameMap));
    std::thread collisionThread(collisionDetector, objpool, std::ref(gameMap));

    logicThread.join();
    // bulletThread.join();
    renderThread.join();
    collisionThread.join();
    // aiTankThread.join();
    for (auto& t : ThreadPool) {
        if (t.joinable()) { 
            t.join();
        }
    }
    objpool->~ObjectsPool();
    
    endwin();
    LOG("end main.\n");
    
    return 0;
}

