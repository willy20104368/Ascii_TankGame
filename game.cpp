#include <iostream>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
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

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

std::mutex bulletMutex;                   
std::vector<bool> bulletActive(4, false);
std::atomic<bool> gameRunning(true);
std::atomic<int> aiTank_n(1);
std::ofstream logFile("log.txt");
ThreadPool threadPool(12);

std::condition_variable gameCondition;
std::mutex gameMutex;


bool initSDL(int screenWidth, int screenHeight) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    if (TTF_Init() < 0) {
        std::cerr << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
        return false;
    }
    window = SDL_CreateWindow("Tank Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}

void closeSDL() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void renderText(const std::string& message, int x, int y, SDL_Color color, TTF_Font* font) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, message.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = { x, y, surface->w, surface->h };
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_DestroyTexture(texture);
}

char meunInput(bool &menuRunning){
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            gameRunning = false;  // exit
            menuRunning = false;
        } 
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_UP:
                case SDLK_w:
                    return 'w'; // up
                case SDLK_DOWN:
                case SDLK_s:
                    return 's'; // down
                case SDLK_RETURN:
                    return 'x';
                default:
                    return 0; // do nothing
            }
        }
    }
    return 0;
}

void displayMenu(int& health, int& aiTankCount) {
    TTF_Font* font = TTF_OpenFont("arial.ttf", 18);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Color white = { 255, 255, 255, 255 };
    bool menuRunning = true;
    char key = 0;
    char text1[100];
    char text2[100];

    while (menuRunning) {

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        while((key = meunInput(menuRunning))!='x'){
            if(key == 's') aiTankCount = std::max(aiTankCount-1, 1);
            else if(key == 'w') aiTankCount = std::min(aiTankCount+1, 3);
            
            SDL_RenderClear(renderer);
            renderText("Welcome to Tank Game!", 100, 50, white, font);
            sprintf(text1, "Set AI Tank number: %d", aiTankCount);
            renderText(text1, 100, 100, white, font);
            renderText("(use 'w' or '^' to increase number, 's' or 'v' to decrease number)", 100, 150, white, font);
            SDL_RenderPresent(renderer);
        }

        renderText("Set Tank Health Point: 1(use 'w' or '^' to increase number, 's' or 'v' to decrease number)", 100, 150, white, font);
        SDL_RenderPresent(renderer);

        while((key = meunInput(menuRunning))!='x'){
            if(key == 's') health = std::max(health-1, 1);
            else if(key == 'w') health = std::min(health+1, 9);

            SDL_RenderClear(renderer);
            renderText("Welcome to Tank Game!", 100, 50, white, font);
            renderText(text1, 100, 100, white, font);
            sprintf(text2, "Set Tank Health Point: %d", health);
            renderText(text2, 100, 150, white, font);
            renderText("(use 'w' or '^' to increase number, 's' or 'v' to decrease number)", 100, 200, white, font);
            SDL_RenderPresent(renderer);
        }
        SDL_RenderClear(renderer);
        renderText("Press Enter to Start the Game", 100, 100, white, font);
        SDL_RenderPresent(renderer);
        while((key = meunInput(menuRunning))!='x'){}
        menuRunning = false;


    }

    TTF_CloseFont(font);
}

void displayEnd(const char* message) {
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Color white = { 255, 255, 255, 255 };

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderText(message, 100, 100, white, font);
    renderText("Press ESC to Quit", 100, 150, white, font);

    SDL_RenderPresent(renderer);

    bool endScreen = true;
    while (endScreen) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                endScreen = false;
                gameRunning = false;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                endScreen = false;
                gameRunning = false;
            }
        }
    }

    TTF_CloseFont(font);
}


void displayHealth(const std::vector<Tank*>& tankPool, SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color white = {255, 255, 255, 255};
    int x = 1210;  // x-axis
    int y = 10;  // y-axis
    for (const auto& tank : tankPool) {
        if (tank->is_alive()) {
            std::string text = "Tank " + std::to_string(tank->getId()) + "(" + tank->getSymbol()+ ")" + " HP: " + std::to_string(tank->getHealth());
            
            SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), white);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

            SDL_Rect rect = {x, y, surface->w, surface->h}; 
            SDL_RenderCopy(renderer, texture, nullptr, &rect);

            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);

            y += 30; 
        }
    }
}


void bulletController(ObjectsPool* objpool, Map& gameMap){

    Bullet* b = objpool->getBullet();

    std::unique_lock<std::mutex> lock(bulletMutex);
    bulletActive[b->getId()] = true;
    lock.unlock();

    
    b->shoot(objpool->getTankPool(), gameMap, renderer);

    lock.lock();
    bulletActive[b->getId()] = false;
    lock.unlock();
  
    LOG("end bulletController.\n" );
}


char processInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            gameRunning = false;  // exit
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_UP:
                case SDLK_w:
                    return 'w'; // up
                case SDLK_DOWN:
                case SDLK_s:
                    return 's'; // down
                case SDLK_LEFT:
                case SDLK_a:
                    return 'a'; // left
                case SDLK_RIGHT:
                case SDLK_d:
                    return 'd'; // right
                case SDLK_SPACE:
                    return ' '; // shoot
                case SDLK_q:
                    return 'q'; // quit game
                default:
                    return 0; // do nothing
            }
        }
    }
    return 0; // do nothing
}


void updateGameLogic(ObjectsPool *objpool, Map& gameMap) {

    char command;
    bool bullet_active = false;
    Tank *t = objpool->getplayer();

    while (t->is_alive() && gameRunning){
        command = processInput();
        if(command == 'w')
            t->move(0, -1, gameMap); // up
        else if(command == 'a')
            t->move(-1, 0, gameMap); // left
        else if(command == 's')
            t->move(0, 1, gameMap); // down
        else if(command == 'd')
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



void renderGame(Map& gameMap, ObjectsPool* objpool) {

    TTF_Font* font = TTF_OpenFont("arial.ttf", 18);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        return;
    }

    while (gameRunning) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        gameMap.display(renderer);

        std::vector<Tank*> tankPool = objpool->getTankPool();
        displayHealth(tankPool, renderer, font);

        SDL_RenderPresent(renderer);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    if (aiTank_n > 0) {
        displayEnd("You lose...");
    } else {
        displayEnd("You Win!!!");
    }
    gameCondition.notify_one();
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

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        if(dy < 0) aiTank->move(0, -1, gameMap); // up
        else if (dy > 0) aiTank->move(0, 1, gameMap); // down
        

        // attack
        if (abs(dx) <= 15 && abs(dy) <= 15) {
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
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // speed of ai tank
    }

    aiTank_n--;
    if(aiTank_n <= 0)
        gameRunning = false;
    LOG("end aiTankController.\n");
}


int main(int argc, char** argv) {

    if (!initSDL(1400, 800)) {
        return -1;
    }

    int health = 1;
    int aiTankCount = 1;

    displayMenu(health, aiTankCount);

    aiTank_n = aiTankCount;
    Map gameMap(60, 40);
    gameMap.addObstacle();

    ObjectsPool* objpool = new ObjectsPool();
    objpool->createTank(gameMap, aiTankCount, health);

    std::vector<Tank*> tankPool = objpool->getTankPool();

    threadPool.enqueue([&]() { updateGameLogic(objpool, std::ref(gameMap)); });
    threadPool.enqueue([&]() { renderGame(std::ref(gameMap), objpool); });


    // if not ref i, it will cause Segmentation fault (core dumped)
    for(int i = 1; i <= aiTank_n; ++i){
        threadPool.enqueue([&, i]() { aiTankController(objpool, tankPool[i], tankPool[0], std::ref(gameMap)); });
    }
    
    
    {
        std::unique_lock<std::mutex> lock(gameMutex);
        gameCondition.wait(lock, [] { return !gameRunning; });
    }

    delete objpool;

    closeSDL();
    LOG("end main.\n");
    
    return 0;
}

