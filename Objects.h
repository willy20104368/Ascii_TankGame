#include <vector>
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <random>
#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <functional>
#include <atomic>

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace(task);
        }
        condition.notify_one();
    }
};

class Map {
private:
    int width, height;
    std::vector<std::vector<std::pair<char, int>>> grid; // map
    std::mutex mapMtx; // map mutex
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> distX, distY, distObstacles;
    int max_Obstacle_nums;
    int min_Obstacle_nums;
    

public:
    const char wall = '#'; // 1
    const char path = ' '; // 0
    
    /* 
    id settings: 0 = path
                    1 = wall
                    2 = player tank
                    3~5 = ai tank
                    6 = bullet
    */
    
    Map(int w, int h) : width(w), height(h){
        max_Obstacle_nums = (width - 1) * (height - 1) / 15;
        min_Obstacle_nums = (width - 1) * (height - 1) / 30;
        gen.seed(rd());
        distObstacles = std::uniform_int_distribution<>(min_Obstacle_nums, max_Obstacle_nums);
        distX = std::uniform_int_distribution<>(1, width - 2);
        distY = std::uniform_int_distribution<>(1, height - 2);

        // initialize map
        grid = std::vector<std::vector<std::pair<char, int>>>(height, std::vector<std::pair<char, int>>(width, {path, 0}));

        // setting boundary
        for (int i = 0; i < height; ++i) {
            grid[i][0] = {wall, 1};          // left
            grid[i][width - 1] = {wall, 1};  // right
        }
        for (int j = 0; j < width; ++j) {
            grid[0][j] = {wall, 0};          // up
            grid[height - 1][j] = {wall, 0}; // down
        }

        
    }


    // randomly add obstacle to map
    void addObstacle() {
        int Obstacle_nums = distObstacles(gen);
        std::lock_guard<std::mutex> lock(mapMtx); 
        for(int i = 0; i < Obstacle_nums; ++i){
            int x = distX(gen);
            int y = distY(gen);
            if (grid[y][x].first == path)  {
                grid[y][x] = {wall,1};
            }
        }
        
    }

    void display(SDL_Renderer* renderer) {
        
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                if (grid[i][j].first == wall) {
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // gray wall
                    SDL_Rect wallRect = {j * 20, i * 20, 20, 20}; // 20x20 pixel
                    SDL_RenderFillRect(renderer, &wallRect);
                }
                else if(grid[i][j].first == '*'){
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red bullet
                    SDL_Rect bulletRect = {j * 20 + 5, i * 20 + 5, 5, 5}; // 10x10 pixel
                    SDL_RenderFillRect(renderer, &bulletRect);
                }
                else if(grid[i][j].first == path){
                    continue;
                }
                else if (grid[i][j].first == '^' || grid[i][j].first == 'v' || grid[i][j].first == '<' || grid[i][j].first == '>') {
                    // Tank body
                    if(grid[i][j].second == 2)
                        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green for player
                    else
                        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // blue for ai

                    SDL_Rect body = {j * 20 + 4, i * 20 + 4, 12, 12}; // Tank body (center rectangle)
                    SDL_RenderFillRect(renderer, &body);

                    // Tank turret
                    SDL_Rect turret;
                    if (grid[i][j].first == '^') {
                        turret = {j * 20 + 8, i * 20, 4, 4}; // Up
                    } else if (grid[i][j].first == 'v') {
                        turret = {j * 20 + 8, i * 20 + 16, 4, 4}; // Down
                    } else if (grid[i][j].first == '<') {
                        turret = {j * 20, i * 20 + 8, 4, 4}; // Left
                    } else if (grid[i][j].first == '>') {
                        turret = {j * 20 + 16, i * 20 + 8, 4, 4}; // Right
                    }
                    SDL_RenderFillRect(renderer, &turret);
                }
            }
        }
    }

    // check boundaries
    bool isWithinBounds(int x, int y) const {
        return x >= 1 && x < width - 1 && y >= 1 && y < height - 1;
    }
    
    // set objects
    void setCell(int x, int y, char value, int id) {
        if (isWithinBounds(x, y)) {
            std::lock_guard<std::mutex> lock(mapMtx);
            grid[y][x] = {value, id};
        }
    }
    
    // get cell
    char getCell(int x, int y){
    	std::lock_guard<std::mutex> lock(mapMtx);
    	return grid[y][x].first;
    }

    int getwidth() const{ 
        return width; 
    }
    int getheight() const{
        return height;
    }
};

class Tank {
private:
    int x, y; // tank location
    char symbol; // symbol of tank
    char direction; // head direction of tank
    int health;
    std::mutex mtx; // tank mutex
    int tank_id; // 2, 3, 4, 5

public:
    Tank(int startX, int startY, char sym, int hp, int id):
        x(startX), y(startY), symbol(sym), health(hp), tank_id(id), direction('s'){}
        
     
    void move(int dx, int dy, Map& map) {
        std::lock_guard<std::mutex> lock(mtx); 
        int newX = x + dx;
        int newY = y + dy;
        direction = (dx == -1 ? 'a' : dx == 1 ? 'd' : dy == -1 ? 'w' : 's');
        symbol = (dx == -1 ? '<' : dx == 1 ? '>' : dy == -1 ? '^' : 'v');
        if (map.isWithinBounds(newX, newY) && map.getCell(newX, newY) == map.path) {
            map.setCell(x, y, map.path, 0);
            x = newX;
            y = newY;
            map.setCell(x, y, symbol, tank_id+2);
        }
    }

    void setup_tank(Map& map){
        std::lock_guard<std::mutex> lock(mtx); 
        map.setCell(x, y, symbol, tank_id+2);
        // avoid tanks getting trapped
        if(map.isWithinBounds(x-1, y)) map.setCell(x-1, y, map.path, 0);
        if(map.isWithinBounds(x+1, y)) map.setCell(x+1, y, map.path, 0);
        if(map.isWithinBounds(x, y-1)) map.setCell(x, y-1, map.path, 0);
        if(map.isWithinBounds(x, y+1)) map.setCell(x, y+1, map.path, 0);
        
    }

    bool is_alive() const{
        return health > 0;
    }

    char getDirection() const{
        return direction;
    }
    
    int getId() const{
        return tank_id;
    }
    int getX() const{ 
        return x; 
    }
    int getY() const{
        return y;
    }
    
    char getSymbol() const{
        return symbol;
    }

    int getHealth() const{
        return health;
    }
    void takeDamage(Map& map, SDL_Renderer* renderer){
        std::lock_guard<std::mutex> lock(mtx); 
        health--;

        map.setCell(x, y, map.path, 0);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        map.setCell(x, y, symbol, tank_id+2);

        if(!is_alive()){
            map.setCell(x, y, map.path, 0);
            symbol = 'x';
        }
        
    }

};

class Bullet{
    
private:
    int x, y;
    char symbol;
    char direction;
    int owner_id;
    std::mutex mtx;
    
public:
    Bullet(int x, int y, int owner_id, int direction){
        this->owner_id = owner_id;
        this->x = x;
        this->y = y;
        this->direction = direction;
        symbol = '*';
    }

    bool collision(std::vector<Tank*> tanks, Map& map, SDL_Renderer* renderer){
        std::lock_guard<std::mutex> lock(mtx);
        for(auto const& t:tanks){
            if(t->getId() != owner_id && t->is_alive() && t->getX() == x && t->getY() == y){
                t->takeDamage(map, renderer);
                return false;
            }
                
        }
        return true;
    }
    
    void shoot(std::vector<Tank*> tanks, Map& map, SDL_Renderer* renderer){
        switch(direction){
            case 'a':
                mtx.lock();
                x--;
                mtx.unlock();
                while(collision(tanks, map, renderer) && map.isWithinBounds(x, y) && map.getCell(x, y) == map.path){
                    map.setCell(x, y, symbol, 6);
                    std::this_thread::sleep_for(std::chrono::milliseconds(60));
                    map.setCell(x, y, map.path, 0);
                    mtx.lock();
                    x--;
                    mtx.unlock();
                }
                break;
            case 'd':
                x++;
                while(collision(tanks, map, renderer) && map.isWithinBounds(x, y) && map.getCell(x, y) == map.path){
                    map.setCell(x, y, symbol, 6);
                    std::this_thread::sleep_for(std::chrono::milliseconds(60));
                    map.setCell(x, y, map.path, 0);
                    mtx.lock();
                    x++;
                    mtx.unlock();
                }
                break;
            case 'w':
                y--;
                while(collision(tanks, map, renderer) && map.isWithinBounds(x, y) && map.getCell(x, y) == map.path){
                    map.setCell(x, y, symbol, 6);
                    std::this_thread::sleep_for(std::chrono::milliseconds(60));
                    map.setCell(x, y, map.path, 0);
                    mtx.lock();
                    y--;
                    mtx.unlock();
                }
                break;
            case 's':
                y++;
                while(collision(tanks, map, renderer) && map.isWithinBounds(x, y) && map.getCell(x, y) == map.path){
                    map.setCell(x, y, symbol, 6);
                    std::this_thread::sleep_for(std::chrono::milliseconds(60));
                    map.setCell(x, y, map.path, 0);
                    mtx.lock();
                    y++;
                    mtx.unlock();
                }
                break;
        }


    }

    int getX() const{ 
        return x; 
    }
    int getY() const{
        return y;
    }
    int getId() const{
        return owner_id;
    }
};

class ObjectsPool{
private:
    std::vector<Tank*> Tankpool;
    std::queue<Bullet*> Bulletpool;
    // std::vector<char> tank_symbol = {'O', 'A', 'T', 'X'};
    std::mutex t_mtx;
    std::mutex b_mtx;

public:
    ObjectsPool(){

    }
    // automatically create player tank & at least one ai tank
    void createTank(Map& map, int tank_n=1, int health=1){

        int width = map.getwidth(), height = map.getheight();
        std::vector<std::pair<int,int>> pos = {{1,1}, {width-1, 1}, {1, height-1}, {width-2, height-1}};

        for(int i = 0; i <= 3 && i <= tank_n; ++i){
            {
                std::lock_guard<std::mutex> lock(t_mtx);
                Tankpool.emplace_back(new Tank(pos[i].first, pos[i].second, 'v', health, i));
            }
            Tankpool[i]->setup_tank(map);
        }
    }
    
    void addBullet(int x, int y, int id, char d){
        std::lock_guard<std::mutex> lock(b_mtx);
        Bulletpool.push(new Bullet(x, y, id, d));
    }

    ~ObjectsPool() {
        for (Tank* tank : Tankpool) {
            delete tank;
        }
        while(!Bulletpool.empty()){
            Bulletpool.pop();
        }
    }

    Tank* getplayer(){
        std::lock_guard<std::mutex> lock(t_mtx); 
        return Tankpool[0];
    }
    std::vector<Tank*> getTankPool(){
        std::lock_guard<std::mutex> lock(t_mtx);
        return Tankpool;
    }
    std::queue<Bullet*> getBulletPool(){
        std::lock_guard<std::mutex> lock(b_mtx);
        return Bulletpool;
    }
    Bullet* getBullet(){
        std::lock_guard<std::mutex> lock(b_mtx);
        if(!Bulletpool.empty()){
            Bullet* bullet = (Bullet*) malloc(sizeof(Bullet));
            bullet = std::move(Bulletpool.front());
            Bulletpool.pop();
            return bullet;  
        }
        else return nullptr;
          
    }
    void delBullet(){
        std::lock_guard<std::mutex> lock(b_mtx);
        Bullet* tmp = Bulletpool.front();
        delete tmp;
        Bulletpool.pop();
    }
    
};