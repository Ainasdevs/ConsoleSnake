#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <deque>
#include <cstdio>

bool GameSetup();
void GameUpdate(float);
void GamePlayspaceClear();
inline int TranslateCoord(SHORT, SHORT);
void InputPolling();

struct SnakeTail {
	int x, y;
	bool bSideways = false;
};

enum SnakeDir {
	FORWARD = 0,
	LEFT = 1,
	BACKWARD = 2,
	RIGHT = 3
};

struct SnakeHead {
	bool alive = true;
	int x = 0, y = 0;
	int px = 0, py = 0;
	std::deque<SnakeTail> tail;
	int dir = SnakeDir::FORWARD;
	int pdir = SnakeDir::FORWARD;
	void display();
	void move();
	void input();
	void eat();
	void die();
	bool pixelIsSnakeHead(int x, int y);
	bool pixelIsSnakeBody(int x, int y);
};

struct Apple {
	int x, y;
	void display();
	void put();
};

bool bInput[5] = { 0, 0, 0, 0, 0 };

HANDLE hConsole = INVALID_HANDLE_VALUE;
HANDLE hScreenBuffer = INVALID_HANDLE_VALUE;
COORD cCon;
COORD cPlayarea;

char* cPixelBuffer;

SnakeHead snake;
Apple apple; // Maybe make instances in a place that makes sense and not global vars ?
bool debug = true;

constexpr unsigned short def_tickrate = 20;
unsigned short tickrate = def_tickrate;

int main(int argc, char* argv[]) {
    puts("Set up CONSOLE and press ENTER to begin.");
    getchar();
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	hScreenBuffer = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hConsole == INVALID_HANDLE_VALUE || hScreenBuffer == INVALID_HANDLE_VALUE) { return EXIT_FAILURE; } // WE DONT HAVE A CONSOLE ?

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(hScreenBuffer, &csbi)) {
		printf("Could not get console screen buffer info; Error: 0x%X\n", (unsigned int)GetLastError());
		return EXIT_FAILURE;
	}
	cCon.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	cCon.Y = csbi.srWindow.Bottom - csbi.srWindow.Top;
	cPixelBuffer = new char[cCon.X * cCon.Y];

	for (int i = 0; i < cCon.X * cCon.Y; ++i) {
		cPixelBuffer[i] = 0;
	}

	GameSetup();
	auto clock = std::chrono::high_resolution_clock::now();
	while (!bInput[4] && snake.alive) {
		InputPolling();
		auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - clock);
		clock = std::chrono::high_resolution_clock::now();
		GameUpdate(delta.count() / 1000.0f);
	}

	delete[] cPixelBuffer;
	return EXIT_SUCCESS;
}

bool GameSetup() {
	int iOffY = debug ? 1 : 0;
	cPlayarea.X = cCon.X - 2;
	cPlayarea.Y = cCon.Y - 2 - iOffY;
	cPixelBuffer[TranslateCoord(0, iOffY)] = char(218);
	cPixelBuffer[TranslateCoord(cCon.X - 1, iOffY)] = char(191);
	cPixelBuffer[TranslateCoord(0, cCon.Y - 1)] = char(192);
	cPixelBuffer[TranslateCoord(cCon.X - 1, cCon.Y - 1)] = char(217);
	for (int i = 1; i < cCon.X - 1; ++i) { cPixelBuffer[TranslateCoord(i, iOffY)] = char(196); }
	for (int i = 1; i < cCon.X - 1; ++i) { cPixelBuffer[TranslateCoord(i, cCon.Y - 1)] = char(196); }
	for (int i = 1 + iOffY; i < cCon.Y - 1; ++i) {
		cPixelBuffer[TranslateCoord(0, i)] = char(179);
		for (int j = 1; j < cCon.X - 1; ++j) {
			cPixelBuffer[TranslateCoord(j, i)] = ' ';
		}
		cPixelBuffer[TranslateCoord(cCon.X - 1, i)] = char(179);
	}
	snake.x = cCon.X / 2;
	snake.y = cCon.Y / 2;
	apple.put();

	return true;
}

void GameUpdate(float fDelta) {
	static float fSecondCounter = 0;
	static float fTickIncrementer = 0;
	fSecondCounter += fDelta;
	fTickIncrementer += fDelta;
	std::string sTitle = "Snake";
	SetConsoleTitleA(sTitle.c_str());

	GamePlayspaceClear();

	snake.input();
	if ((fTickIncrementer * 1000) > 1000/tickrate) {
		fTickIncrementer = 0;
		snake.move();
	}

	apple.display();
	snake.display();

	COORD cBeginning;
	cBeginning.X = 0;
	cBeginning.Y = 0;
	SetConsoleCursorPosition(hScreenBuffer, cBeginning);
	WriteConsoleA(hScreenBuffer, cPixelBuffer, cCon.X * cCon.Y, NULL, NULL);
}


void GamePlayspaceClear() {
	int iOffY = debug ? 1 : 0;
	for (int i = 1 + iOffY; i < cCon.Y - 1; ++i) {
		for (int j = 1; j < cCon.X - 1; ++j) {
			cPixelBuffer[TranslateCoord(j, i)] = 0;
		}
	}
}

void InputPolling() {
	bInput[0] = (GetAsyncKeyState(0x57) >> 1);
	bInput[1] = (GetAsyncKeyState(0x41) >> 1);
	bInput[2] = (GetAsyncKeyState(0x53) >> 1);
	bInput[3] = (GetAsyncKeyState(0x44) >> 1);
	bInput[4] = (GetAsyncKeyState(0x58) >> 1);
	cPixelBuffer[1] = bInput[0] ? 'W' : 0;
	cPixelBuffer[3] = bInput[1] ? 'A' : 0;
	cPixelBuffer[5] = bInput[2] ? 'S' : 0;
	cPixelBuffer[7] = bInput[3] ? 'D' : 0;
	cPixelBuffer[9] = bInput[4] ? 'X' : 0;
}

inline int TranslateCoord(SHORT X, SHORT Y) {
	return static_cast<int>(cCon.X * Y + X);
}

void SnakeHead::display() {
	cPixelBuffer[TranslateCoord(x, y)] = char(254);
	for (int i = 0; i < (int)tail.size(); ++i) {
		cPixelBuffer[TranslateCoord(tail[i].x, tail[i].y)] = tail[i].bSideways ? char(254) : char(219);
	}
}

void SnakeHead::move() {
	snake.px = x;
	snake.py = y;
	snake.pdir = dir;
	switch (snake.dir) {
	case SnakeDir::FORWARD:
	    tickrate = def_tickrate/2;
		--y;
		break;
	case SnakeDir::BACKWARD:
	    tickrate = def_tickrate/2;
		++y;
		break;
	case SnakeDir::LEFT:
	    tickrate = def_tickrate;
		--x;
		break;
	case SnakeDir::RIGHT:
	    tickrate = def_tickrate;
		++x;
		break;
	}
	if (pixelIsSnakeBody(x, y)) { die(); }
	if (x < 1 || x > cPlayarea.X || y < (debug ? 2 : 1) || y > cPlayarea.Y + (debug ? 1 : 0)) { die(); }
	if (pixelIsSnakeHead(apple.x, apple.y)) {
		eat();
	} else {
		SnakeTail t;
		t.bSideways = (pdir == SnakeDir::LEFT || pdir == SnakeDir::RIGHT);
		t.x = px;
		t.y = py;
		tail.push_front(t);
		tail.pop_back();
	}
}

void SnakeHead::input() {
	if (bInput[0]) { snake.dir = snake.dir != SnakeDir::BACKWARD ? SnakeDir::FORWARD : snake.dir; }
	if (bInput[1]) { snake.dir = snake.dir != SnakeDir::RIGHT ? SnakeDir::LEFT : snake.dir; }
	if (bInput[2]) { snake.dir = snake.dir != SnakeDir::FORWARD ? ::BACKWARD : snake.dir; }
	if (bInput[3]) { snake.dir = snake.dir != SnakeDir::LEFT ? SnakeDir::RIGHT : snake.dir; }
}

void SnakeHead::eat() {
	SnakeTail t;
	t.bSideways = (pdir == SnakeDir::LEFT || pdir == SnakeDir::RIGHT);
	t.x = px;
	t.y = py;
	tail.push_front(t);
	apple.put();
}

void SnakeHead::die() {
	alive = false;
}

bool SnakeHead::pixelIsSnakeHead(int x, int y) {
	return (this->x == x && this->y == y);
}

bool SnakeHead::pixelIsSnakeBody(int x, int y) {
	for (int i = 0; i < (int)this->tail.size(); ++i) {
		if (this->tail[i].x == x && this->tail[i].y == y) { return true; }
	}
	return false;
}

void Apple::put() {
	srand(time(NULL));
	do {
		x = (rand() % cPlayarea.X) + 1;
		y = (rand() % cPlayarea.Y) + (debug ? 2 : 1);
	} while (snake.pixelIsSnakeBody(x, y) || snake.pixelIsSnakeHead(x, y));
}

void Apple::display() {
	cPixelBuffer[TranslateCoord(x, y)] = char(79);
}
