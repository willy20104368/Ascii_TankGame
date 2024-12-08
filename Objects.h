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
    std::vector<std::vector<char>> grid; // 用來表示地圖，字符可表示不同物體
    std::mutex mtx; // 用來保護地圖資料的互斥鎖
    int max_Obstacle_nums;
    int min_Obstacle_nums;
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> dist_nums;
    std::uniform_int_distribution<> dist_obstacle_x;
    std::uniform_int_distribution<> dist_obstacle_y;
    

public:
    char wall = '#';
    char path = '.';
    

    Map(int w, int h) : width(w), height(h){
        max_Obstacle_nums = (width - 1) * (height - 1) / 15;
        min_Obstacle_nums = (width - 1) * (height - 1) / 30;
        gen.seed(rd());
        dist_nums = std::uniform_int_distribution<>(min_Obstacle_nums, max_Obstacle_nums);
        dist_obstacle_x = std::uniform_int_distribution<>(1, width - 2);
        dist_obstacle_y = std::uniform_int_distribution<>(1, height - 2);

        // 初始化地圖，填充邊界為 '#', 內部為 '*'
        grid = std::vector<std::vector<char>>(height, std::vector<char>(width, path));

        // 設置邊界
        for (int i = 0; i < height; ++i) {
            grid[i][0] = wall;          // 左邊界
            grid[i][width - 1] = wall;  // 右邊界
        }
        for (int j = 0; j < width; ++j) {
            grid[0][j] = wall;          // 上邊界
            grid[height - 1][j] = wall; // 下邊界
        }

        
    }

    void addObstacle() {
        int Obstacle_nums = dist_nums(gen);
        std::lock_guard<std::mutex> lock(mtx); // 鎖定地圖資料
        for(int i = 0; i < Obstacle_nums; ++i){
            int x = dist_obstacle_x(gen);
            int y = dist_obstacle_y(gen);
            if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                grid[y][x] = wall; // 障礙物用 '#' 表示
            }
        }
        
    }

    // void display() {
    //     std::lock_guard<std::mutex> lock(mtx); // 鎖定地圖資料
    //     for (const auto& row : grid) {
    //         for (char cell : row) {
    //             std::cout << cell << ' ';
    //         }
    //         std::cout << std::endl;
    //     }
    // }
    void display() {
        for (int i = 0; i < height; ++i) {
            for (int j = 0; j < width; ++j) {
                mvaddch(i, j, grid[i][j]);  
        }
    }
    refresh(); 
}


    bool isWithinBounds(int x, int y) const {
        return x >= 1 && x < width - 1 && y >= 1 && y < height - 1; // 不包括邊界
    }

    void setCell(int x, int y, char value) {
        std::lock_guard<std::mutex> lock(mtx); // 鎖定地圖資料
        if (isWithinBounds(x, y)) { // 不允許修改邊界
            grid[y][x] = value;
        }
    }
    
    char getCell(int x, int y){
    	std::lock_guard<std::mutex> lock(mtx);
    	return grid[y][x];
    }
};

class Tank {
private:
    int x, y; // 坦克的位置
    char symbol; // 坦克的符號，例如 'T'
    std::mutex mtx; // 用來保護坦克位置的互斥鎖

public:
    Tank(int startX, int startY, char sym) : x(startX), y(startY), symbol(sym) {}

    void move(int dx, int dy, Map& map) {
        std::lock_guard<std::mutex> lock(mtx); // 鎖定坦克資料
        int newX = x + dx;
        int newY = y + dy;

        // 檢查是否在地圖範圍內，且新位置不是障礙物
        if (map.isWithinBounds(newX, newY) && map.getCell(newX, newY) == map.path) {
            map.setCell(x, y, map.path);
            x = newX;
            y = newY;
            map.setCell(x, y, symbol);
            
        }
    }

    void render(Map& map) {
        std::lock_guard<std::mutex> lock(mtx); // 鎖定坦克資料
        map.setCell(x, y, symbol); // 在地圖上顯示坦克
    }

    int getX() const { return x; }
    int getY() const { return y; }
};

