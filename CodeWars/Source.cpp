#pragma once
#include <iostream>
#include <ctime>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <SDL.h>
#include <windows.h>
#undef min
#undef max

// color definition
#define OFF_COLOUR 0x00
#define ON_COLOUR 0xEE
#define ALIVE_COLOUR 0x00FF00

// Limit loop rate for visibility
#define LIMIT_RATE 0
// Tick-rate in milliseconds (if LIMIT_RATE == 1)
#define TICK_RATE 25

SDL_Window* window = NULL;
SDL_Surface* surface = NULL;

struct Cell_State {
	signed int neihbours_counter ;
};

// Structure for dividing the screen into two parts
struct DrawData {
	DrawData(unsigned int height, unsigned int width, unsigned int start_x, unsigned int start_y, unsigned int del);
	 unsigned int height;
	 unsigned int width;
	 unsigned int start_x;
	 unsigned int start_y;
	 unsigned int del;
};


struct Matrix {
	Matrix(unsigned int height, unsigned int width);
	~Matrix();
	bool** matrix;
	bool** temp_matrix;
	unsigned int height;
	unsigned int width;
};

void Print_Matrix(Matrix& Matrix, DrawData& DrawMap);
void Next_Generation(Matrix& Matrix, DrawData& DrawMap, Cell_State& info);
Cell_State& Neighbours_Count(Matrix& Matrix, Cell_State& info, DrawData& DrawMap, const int x, const int y);



int main(int argc, char* argv[]) {

	setlocale(LC_ALL, "ru");
	srand(time(NULL));
	unsigned int height = 20, width = 30;

	//divide the screen into two parts
	DrawData DrawMap_1(height, width/2, 0, 0, 2);
	DrawData DrawMap_2(height, width, 0, width / 2, 2);
	Matrix Matrix(height, width);
	Cell_State info_1;
	Cell_State info_2;

	//surface creation for SDL
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("GAME ", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width*width, height*height, SDL_WINDOW_SHOWN);
	surface = SDL_GetWindowSurface(window);
	

	bool running1 = true;
	bool running2 = true;
	std::mutex mutex1;
	std::mutex mutex2;
	std::condition_variable cv1;
	std::condition_variable cv2;

	//distribution of functions by streams for both halves of the screen
	std::thread left_thread([&Matrix, &DrawMap_1, &info_1, &running1, &mutex1, &cv1]() {
		while (running1) {
			{
				std::unique_lock<std::mutex> lock(mutex1);
				Print_Matrix(Matrix, DrawMap_1);
				SDL_Delay(TICK_RATE);
				Next_Generation(Matrix, DrawMap_1, info_1);
			}
			cv1.notify_one(); 
		}
		});

	std::thread right_thread([&Matrix, &DrawMap_2, &info_2, &running2, &mutex2, &cv1, &cv2]() {
		while (running2) {
			{
				std::unique_lock<std::mutex> lock(mutex2);
				cv1.wait(lock); 
				Print_Matrix(Matrix, DrawMap_2);
				SDL_Delay(TICK_RATE);
				Next_Generation(Matrix, DrawMap_2, info_2);
			}
			cv2.notify_one(); 
		}
		});

	SDL_Event e;
	bool quit = false;
	while (!quit)
	{
		while (SDL_PollEvent(&e) != 0)
			if (e.type == SDL_QUIT) quit = true;
		SDL_UpdateWindowSurface(window);

#if LIMIT_RATE
		SDL_Delay(TICK_RATE);
#endif
	}
	SDL_DestroyWindow(window);
	SDL_Quit();
	system("pause");
	return 0;
}

DrawData::DrawData(unsigned int height, unsigned int width, unsigned int start_x, unsigned int start_y, unsigned int del) {
	this->start_x = start_x;
	this->start_y = start_y;
	this->height = height;
	if (start_y != 0)
		this->del = 1;
	else
		this->del = del;
	this->width = width;
}

Matrix::Matrix(unsigned int height, unsigned int width) {
	this->height = height;
	this->width = width;
	matrix = new bool* [height];
	for (unsigned int i = 0; i < height ; i++)
		matrix[i] = new bool[width];
	for (unsigned int i = 0; i < height ; i++) {
		for (unsigned int j =0; j < width ; j++)
			matrix[i][j] = rand() % 2;
	}
	temp_matrix = new bool* [height];
	for (unsigned int i = 0; i < height ; i++)
		temp_matrix[i] = new bool[width];
}

Matrix:: ~Matrix() {
	for (unsigned int i = 0; i < height; i++)
		delete[] matrix[i];
	for (unsigned int i = 0; i < height; i++)
		delete[] temp_matrix[i];
	delete[] matrix;
	delete[] temp_matrix;
}

//display field on the screen
void Print_Matrix(Matrix& Matrix, DrawData& DrawMap) {
	SDL_FillRect(surface, NULL, OFF_COLOUR);
	int count = 0;
	for (unsigned int i = DrawMap.start_x; i < DrawMap.height ; i++) {
		for (unsigned int j = DrawMap.start_y; j < DrawMap.width ; j++) {
			if (Matrix.matrix[i][j] == 1) {
				SDL_Rect pixel;
				pixel.x = j * DrawMap.width*DrawMap.del;
				pixel.y = i * DrawMap.height;
				pixel.w = 1 * DrawMap.width*DrawMap.del;
				pixel.h = 1 * DrawMap.height;
				if(DrawMap.start_y!=0)
				SDL_FillRect(surface, &pixel, ALIVE_COLOUR);
				else
					SDL_FillRect(surface, &pixel, ON_COLOUR);
				count++;
			}
		}
	}
	
}

//next generation computing
void Next_Generation(Matrix& Matrix, DrawData& DrawMap, Cell_State& info) {

	memcpy(Matrix.temp_matrix, Matrix.matrix, sizeof(bool) * Matrix.height * Matrix.width);

	for (unsigned int i = DrawMap.start_x; i < DrawMap.height ; i++) {
		for (unsigned int j = DrawMap.start_y; j < DrawMap.width ; j++) {
			Cell_State neighbour = Neighbours_Count(Matrix, info, DrawMap, i, j);
			if (Matrix.matrix[i][j] == 1) {
				if (neighbour.neihbours_counter < 2 || neighbour.neihbours_counter > 3) {
					Matrix.temp_matrix[i][j] = 0;
				}
			}
			else {
				if (neighbour.neihbours_counter == 3)
					Matrix.temp_matrix[i][j] = 1;
			}
		}
	}
	memcpy(Matrix.matrix, Matrix.temp_matrix, sizeof(bool) * Matrix.height * Matrix.width);

}

//calculation of the neighbors of each cell
Cell_State& Neighbours_Count(Matrix& Matrix, Cell_State& info, DrawData& DrawMap, const int x, const int y) {
	info.neihbours_counter = 0;
	const int height = DrawMap.height;
	const int width = DrawMap.width;
	const int start_x = DrawMap.start_x;
	const int start_y = DrawMap.start_y;
	for (unsigned int i = std::max(start_x, x - 1); i <= std::min(height - 1, x + 1); i++) {
		for (unsigned int j = std::max(start_y, y - 1); j <= std::min(width - 1, y + 1); j++) {
			if (i == x && j == y)
				continue;
			else if (Matrix.matrix[i][j] == 1 && i >= 0 && i < height && j >= 0 && j < width)
				info.neihbours_counter++;
		}
	}
	return info;
}