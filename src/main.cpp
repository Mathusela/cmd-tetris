#include <iostream>
#include <bitset>
#include <array>
#include <vector>
#include <string>
#include <conio.h>

#include <thread>
#include <chrono>

using t_size = long long;

const long long WIDTH= 10;
const long long HEIGHT = 22;
const int BLOCK_MAP_SCALE = 2;

const int TPS = 30;
const int DELTA_TIME = 1000/TPS;
const int DROP_DELAY = 500;
const int DROP_COUNT = DROP_DELAY/DELTA_TIME;

bool g_updated = true;

using t_board = std::bitset<WIDTH * HEIGHT>;
using t_blockMap = std::bitset<WIDTH * HEIGHT * BLOCK_MAP_SCALE>;

enum t_blockId { SQUARE, LINE, L, J, T, S, Z };

struct Block {
	std::array<t_size, 4> lines;
	t_blockId m_id;

	Block(t_size l1, t_size l2, t_size l3, t_size l4, t_blockId id) {
		lines[0] = l1;
		lines[1] = l2;
		lines[2] = l3;
		lines[3] = l4;
		m_id = id;
	}
};

void gen_block_map(t_blockMap& blockMap, std::vector<Block> blocks) {
	t_blockMap tempMap;
	for (auto it = blocks.begin(); it != blocks.end(); it++) {
		tempMap = 0;
		for (auto line : it->lines) {
			tempMap |= line;
			tempMap <<= WIDTH;
		}
		tempMap <<= WIDTH * it->m_id * 4;
		blockMap |= tempMap;
	}
	blockMap >>= WIDTH;
}

t_size constexpr power(t_size b, t_size i) {
	return i == 0 ? 1 : b * power(b, i - 1);
}

void spawn_block(t_board& board, t_blockMap blockMap, Block block) {
	blockMap >>= WIDTH * block.m_id * 4;
	blockMap &= power(2, WIDTH * 4) - 1;
	board = t_board { blockMap.to_string().substr((WIDTH * HEIGHT), (WIDTH * HEIGHT)*2) };
	board <<= (int)(WIDTH/2) - 2;
}

void format_screen() {
	std::cout << "\033[46;37;3m";
}

void print_board(const t_board board) {
	for (int i = 0; i < HEIGHT; i++) {
		std::cout << i << ":\t";
		for (int j = 0; j < WIDTH; j++)	{
			if (board[i * WIDTH + j]) std::cout << "\033[101m";
			std::cout << board[i * WIDTH + j] << " ";
			format_screen();
		}
		std::cout << std::endl;
	}
}

void clear_screen_refresh() {
	std::cout << "\x1B[2J\x1B[H";
}
void clear_screen() {
	std::cout << "\x1B[3J\x1B[H";
}

bool collision_check(t_board dynamicBoard, t_board staticBoard) {
	return (dynamicBoard & staticBoard) != 0;
}

t_size most_significant_bit(t_size n) {
	t_size count = 1;
	while (n >>= 1 != 0) count++;
	return count;
}

t_board fall_board(t_board board) {
	return board << WIDTH;
}
t_board right_board(t_board board) {
	return board << 1;
}
t_board left_board(t_board board) {
	return board >> 1;
}

void move_left(t_board& dynamicBoard, t_board& staticBoard) {
	bool move = true;
	for (int i = 0; i < HEIGHT; i++) {
		auto line = std::stoi(dynamicBoard.to_string().substr(i * WIDTH, WIDTH), 0, 2);
		if ( line&1 == 1 || (left_board(dynamicBoard)&staticBoard) != 0 ) {move = false; break;}
	}
	if (move) dynamicBoard = left_board(dynamicBoard);
	g_updated = true;
}
void move_right(t_board& dynamicBoard, t_board& staticBoard) {
	bool move = true;
	for (int i = 0; i < HEIGHT; i++) {
		auto line = std::stoi(dynamicBoard.to_string().substr(i * WIDTH, WIDTH), 0, 2);
		if ( most_significant_bit(line) == WIDTH || (right_board(dynamicBoard)&staticBoard) != 0 ) {move = false; break;}
	}
	if (move) dynamicBoard = right_board(dynamicBoard);
	g_updated = true;
}
void soft_drop(t_board& dynamicBoard, t_board& staticBoard) {
	t_board maskBoard { power(2, WIDTH)-1 }; maskBoard <<= (HEIGHT-1) * WIDTH;
	if (collision_check(fall_board(dynamicBoard), staticBoard) || (maskBoard & dynamicBoard) != 0) {
		staticBoard |= dynamicBoard;
		dynamicBoard ^= dynamicBoard;
	}
	else dynamicBoard = fall_board(dynamicBoard);
	g_updated = true;
}

void handle_input(t_board& dynamicBoard, t_board& staticBoard) {
	if (kbhit() != 0) {
		switch (getch()) {
		case 97: // A
			move_left(dynamicBoard, staticBoard);
			break;
		case 100: // D
			move_right(dynamicBoard, staticBoard);
			break;
		case 115: // S
			soft_drop(dynamicBoard, staticBoard);
			break;
		default:
			break;
		}

		while (kbhit()) getch();
	};
}

void step_game(t_board& dynamicBoard, t_board& staticBoard) {
	soft_drop(dynamicBoard, staticBoard);
}

int main() {

	t_board dynamicBoard;
	t_board staticBoard;

	t_blockMap blockMap;

	std::vector<Block> blocks {
		Block(0, 0, 3, 3, SQUARE),
		Block(0, 0, 0, 15, LINE),
		Block(0, 0, 1, 7, L),
		Block(0, 0, 4, 7, J),
		Block(0, 0, 2, 7, T),
		Block(0, 0, 3, 6, S),
		Block(0, 0, 6, 3, Z)
	};
	gen_block_map(blockMap, blocks);

	auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	srand(time);

	format_screen();
	clear_screen_refresh();

	int count = 0;
	while (true) {
		if (dynamicBoard == 0) spawn_block(dynamicBoard, blockMap, blocks[rand() % blocks.size()]);
		
		std::this_thread::sleep_for(std::chrono::milliseconds(DELTA_TIME));

		handle_input(dynamicBoard, staticBoard);
		
		if (g_updated) {
			print_board(dynamicBoard | staticBoard);
			clear_screen();
			g_updated = false;
		}
		
		if (count == DROP_COUNT) { step_game(dynamicBoard, staticBoard); count = 0; }
		count++;
	}
	std::cout << "\033[0m";

	// TODO: Block rotation
	// TODO: Loose condition
	// TODO: Line clearing

	return 0;
}