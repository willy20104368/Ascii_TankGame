#include <vector>
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <ncurses.h>
#include <random>
#include <stdlib.h>
#include <stdio.h>

class Map {
private:
    int width;
    int height;
    std::vector<std::vector<char>> grid; // map
    std::mutex mtx; // map mutex
    int max_Obstacle_nums;
    int min_Obstacle_nums;
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> dist_nums;
    std::uniform_int_distribution<> dist_obstacle_x;
    std::uniform_int_distribution<> dist_obstacle_y;
    

public:
    char wall = '#';
    char path = ' ';
    

    Map(int w, int h) : width(w), height(h){
        max_Obstacle_nums = (width - 1) * (height - 1) / 15;
        min_Obstacle_nums = (width - 1) * (height - 1) / 30;
        gen.seed(rd());
        dist_nums = std::uniform_int_distribution<>(min_Obstacle_nums, max_Obstacle_nums);
        dist_obstacle_x = std::uniform_int_distribution<>(1, width - 2);
        dist_obstacle_y = std::uniform_int_distribution<>(1, height - 2);

        // initialize map
        grid = std::vector<std::vector<char>>(height, std::vector<char>(width, path));

        // setting boundary
        for (int i = 0; i < height; ++i) {
            grid[i][0] = wall;          // left
            grid[i][width - 1] = wall;  // right
        }
        for (int j = 0; j < width; ++j) {
            grid[0][j] = wall;          // up
            grid[height - 1][j] = wall; // down
        }

        
    }

    // randomly add obstacle to map
    void addObstacle() {
        int Obstacle_nums = dist_nums(gen);
        std::lock_guard<std::mutex> lock(mtx); 
        for(int i = 0; i < Obstacle_nums; ++i){
            int x = dist_obstacle_x(gen);
            int y = dist_obstacle_y(gen);
            if (x > 0 && x < width - 1 && y > 0 && y < height - 1 && grid[y][x] == path)  {
                grid[y][x] = wall;
            }
        }
        
    }

    
    // refresh & display whole map
    void display() {
        // std::lock_guard<std::mutex> lock(mtx);
        mtx.lock();
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                mvaddch(i, j, grid[i][j]);  
            }
        }
        refresh(); 
        mtx.unlock();
    }

    // refresh two input cells
    void display_changes(int x1, int y1, int x2, int y2){
        std::lock_guard<std::mutex> lock(mtx);
        mvaddch(y1, x1, grid[y1][x1]);
        mvaddch(y2, x2, grid[y2][x2]);    
        refresh(); 
    }

    // check boundaries
    bool isWithinBounds(int x, int y) const {
        return x >= 1 && x < width - 1 && y >= 1 && y < height - 1;
    }
    
    // set objects
    void setCell(int x, int y, char value) {
        if (isWithinBounds(x, y)) {
            std::lock_guard<std::mutex> lock(mtx);
            grid[y][x] = value;
        }
    }
    
    // get cell
    char getCell(int x, int y){
    	std::lock_guard<std::mutex> lock(mtx);
    	return grid[y][x];
    }
};

class Tank {
private:
    int x, y; // tank location
    char symbol; // symbol of tank
    char direction; // head direction of tank
    int health;
    std::mutex mtx; // tank mutex
    int tank_id;

public:
    Tank(int startX, int startY, char sym, int hp, int id) : x(startX), y(startY), symbol(sym), health(hp), tank_id(id){
        direction = 's';
    }
     
    void move(int dx, int dy, Map& map) {
        std::unique_lock<std::mutex> lock(mtx); 
        int newX = x + dx;
        int newY = y + dy;
        if(dx == -1) direction = 'a';
        else if(dx == 1) direction = 'd';
        else if(dy == -1) direction = 'w';
        else if(dy == 1) direction = 's';
        lock.unlock();
        
        if (map.isWithinBounds(newX, newY) && map.getCell(newX, newY) == map.path) {
            map.setCell(x, y, map.path);
            x = newX;
            y = newY;
            map.setCell(x, y, symbol);
        }
    }


    void render(Map& map) {
        std::lock_guard<std::mutex> lock(mtx); 
        map.setCell(x, y, symbol);
    }

    bool is_alive() {
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
};

class Bullet{
    
private:
    int x, y;
    char symbol;
    char direction;
    int owner_id;
public:
    Bullet(int x, int y, int owner_id, int direction){
        this->owner_id = owner_id;
        this->x = x;
        this->y = y;
        this->direction = direction;
        symbol = '*';
    }

    void shoot(Map& map){
        switch(direction){
            case 'a':
                x--;
                while(map.isWithinBounds(x, y) && map.getCell(x, y) == map.path){
                    map.setCell(x, y, symbol);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    map.setCell(x, y, map.path);
                    x--;
                }
                break;
            case 'd':
                x++;
                while(map.isWithinBounds(x, y) && map.getCell(x, y) == map.path){
                    map.setCell(x, y, symbol);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    map.setCell(x, y, map.path);
                    x++;
                }
                break;
            case 'w':
                y--;
                while(map.isWithinBounds(x, y) && map.getCell(x, y) == map.path){
                    map.setCell(x, y, symbol);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    map.setCell(x, y, map.path);
                    y--;
                }
                break;
            case 's':
                y++;
                while(map.isWithinBounds(x, y) && map.getCell(x, y) == map.path){
                    map.setCell(x, y, symbol);
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    map.setCell(x, y, map.path);
                    y++;
                }
                break;
        }


    }

};

class ObjectsPool{
private:
    std::vector<Tank*> Tankpool;
    std::vector<char> tank_symbol = {'O', 'A', 'T', 'X'};
    std::mutex t_mtx;
    int tank_nums = 0;

public:
    ObjectsPool(){

    }
    // automatically create player tank & at least one ai tank
    void createTank(Map& map, int tank_n=1, int health=1){
        if(tank_n > 3) tank_n = 3;
        tank_nums = tank_n + 1;
        // create player tank
        {
            std::lock_guard<std::mutex> lock(t_mtx);
            Tankpool.emplace_back(new Tank(1, 1, tank_symbol[0], health, 0));
        }
        Tankpool[0]->render(map);
        // create ai tank
        for(int i = 1; i <= 3 && i <= tank_n; ++i){
            {
                std::lock_guard<std::mutex> lock(t_mtx);
                Tankpool.emplace_back(new Tank(1+5*i,1+5*i, tank_symbol[i], health, i));
            }
            Tankpool[i]->render(map);
        }
    }
    
    ~ObjectsPool() {
        for (Tank* tank : Tankpool) {
            delete tank;
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
    
};