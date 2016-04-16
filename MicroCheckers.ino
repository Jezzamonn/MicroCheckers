

#define BOARD_SIZE 8
uint8_t board[BOARD_SIZE * BOARD_SIZE];

uint8_t getPieceAt(uint8_t x, uint8_t y) {
    return board[y * BOARD_SIZE + x];
}

uint8_t setPieceAt(uint8_t x, uin8_t y, uint8_t value) {
    board[y * BOARD_SIZE + x] = value;
}

uint8_t current_player = 0;

void setup() {
}

void loop() {
}