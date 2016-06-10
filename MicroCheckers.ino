#include <Arduboy.h>
#include "bitmaps.h"

Arduboy arduboy;

#define BOARD_SIZE 8
#define PIECE_SIZE 8
#define X_OFFSET    ((WIDTH - BOARD_SIZE * PIECE_SIZE) / 2)



// key, last three bits:
// 00x -> 0 = no piece there, 1 = piece there
// 0x0 -> 0 = white piece, 1 = black piece
// x00 -> 0 = regular piece, 1 = king piece
uint8_t board[BOARD_SIZE * BOARD_SIZE];

// 0 -> white
// 1 -> black (goes first)
uint8_t currentPlayer = 1;
// 0 -> selecting a piece
// 1 -> selecting a move
uint8_t currentState = 0;

uint8_t curCursorX = 0;
uint8_t curCursorY = 0;
// This cursor stays on the selected piece when the position for that piece is being chosen.
uint8_t prevCursorX = 0;
uint8_t prevCursorY = 0;

uint8_t frameCount = 0;

bool leftPressed = false;
bool rightPressed = false;
bool upPressed = false;
bool downPressed = false;
bool aPressed = false;
bool bPressed = false;

uint8_t getPieceAt(uint8_t x, uint8_t y) {
    return board[y * BOARD_SIZE + x];
}

uint8_t setPieceAt(uint8_t x, uint8_t y, uint8_t value) {
    board[y * BOARD_SIZE + x] = value;
}

void initBoard() {
    uint8_t x;
    uint8_t y;
    for (x = 0; x < BOARD_SIZE; x ++) {
        for (y = 0; y < BOARD_SIZE; y ++) {
            setPieceAt(x, y, 0);
        }
        
        for (y = 0; y < 3; y ++) {
            if ((x ^ y) & 1) {
                setPieceAt(x, y, 1);
            }
        }
        for (y = BOARD_SIZE - 3; y < BOARD_SIZE; y ++) {
            if ((x ^ y) & 1) {
                setPieceAt(x, y, 3);
            }
        }
    }
}

/* ------------- LOGIC ------------- */
void checkInput() {
    bool leftEdge = false;
    bool rightEdge = false;
    bool upEdge = false;
    bool downEdge = false;
    bool aEdge = false;
    bool bEdge = false;
    
    uint8_t piece;

    if (arduboy.pressed(LEFT_BUTTON)) {
        if (!leftPressed) {
            leftEdge = true;
        }
        leftPressed = true;
    }
    else {
        leftPressed = false;
    }
    if (arduboy.pressed(RIGHT_BUTTON)) {
        if (!rightPressed) {
            rightEdge = true;
        }
        rightPressed = true;
    }
    else {
        rightPressed = false;
    }
    if (arduboy.pressed(UP_BUTTON)) {
        if (!upPressed) {
            upEdge = true;
        }
        upPressed = true;
    }
    else {
        upPressed = false;
    }
    if (arduboy.pressed(DOWN_BUTTON)) {
        if (!downPressed) {
            downEdge = true;
        }
        downPressed = true;
    }
    else {
        downPressed = false;
    }
    if (arduboy.pressed(A_BUTTON)) {
        if (!aPressed) {
            aEdge = true;
        }
        aPressed = true;
    }
    else {
        aPressed = false;
    }
    if (arduboy.pressed(B_BUTTON)) {
        if (!bPressed) {
            bEdge = true;
        }
        bPressed = true;
    }
    else {
        bPressed = false;
    }
    
    if (leftEdge) {
        curCursorX --;
    }
    if (rightEdge) {
        curCursorX ++;
    }
    if (upEdge) {
        curCursorY --;
    }
    if (downEdge) {
        curCursorY ++;
    }
    curCursorX &= 0x7;
    curCursorY &= 0x7;
    
    if (aEdge) { // Back
        currentState = 0;
    }
    if (bEdge) {
        if (currentState == 0) {
            // TODO: check if there are any pieces that have to jump
            piece = getPieceAt(curCursorX, curCursorY);
            // piece & 1 -> there is a piece on this square
            // !(piece & 2) == !currentPlayer -> the piece is the right color
            if ((piece & 1) && !(piece & 2) == !currentPlayer) {
                currentState = 1;
                prevCursorX = curCursorX;
                prevCursorY = curCursorY;
            }
        }
        else if (currentState == 1) {
            // TODO: check if the moved postion is valid
            // TODO: check if another jump can be made
            // TODO: prevent the player from changing which piece they're moving if they've made a jump and can make another
            currentState = 0;
            piece = getPieceAt(prevCursorX, prevCursorY);
            setPieceAt(curCursorX, curCursorY, piece);
            // This will first clear the moved piece, and then clear any pieces that have been jumped over
            while (prevCursorX != curCursorX || prevCursorY != curCursorY) {
                setPieceAt(prevCursorX, prevCursorY, 0);
                if (prevCursorX < curCursorX) {
                    prevCursorX ++;
                }
                else if (prevCursorX > curCursorX) {
                    prevCursorX --;
                }
                if (prevCursorY < curCursorY) {
                    prevCursorY ++;
                }
                else if (prevCursorY > curCursorY) {
                    prevCursorY --;
                }
            }
            currentPlayer = !currentPlayer;
        }
    }
}

/* ------------- DRAWING ------------- */   
void drawBoard() {
    uint8_t x;
    uint8_t y;
    for (y = 0; y < BOARD_SIZE; y ++) {
        for (x = 0; x < BOARD_SIZE; x ++) {
            if ((x ^ y) & 1) {
                arduboy.fillRect(X_OFFSET + x * PIECE_SIZE, y * PIECE_SIZE,
                    PIECE_SIZE, PIECE_SIZE, WHITE);
            }
            uint8_t piece = getPieceAt(x, y);
            const unsigned char *bitmap = NULL;
            switch (piece) {
                case 1:
                    bitmap = piece1;
                    break;
                case 3:
                    bitmap = piece2;
                    break;
                case 5:
                    bitmap = king1;
                    break;
                case 7:
                    bitmap = king2;
                    break;
            }
            if (bitmap == NULL) {
                continue;
            }
            arduboy.drawBitmap(X_OFFSET + x * PIECE_SIZE, y * PIECE_SIZE,
                bitmap, PIECE_SIZE, PIECE_SIZE, INVERT);
        }
    }
    arduboy.fillRect(X_OFFSET - 1, 0, 1, HEIGHT, WHITE);
    arduboy.fillRect(X_OFFSET + BOARD_SIZE * PIECE_SIZE, 0, 1, HEIGHT, WHITE);
}

void drawCursor() {
    drawCursorAt(curCursorX, curCursorY, true);
    if (currentState == 1) {
        drawCursorAt(prevCursorX, prevCursorY, false);
    }
}

void drawCursorAt(uint8_t x, uint8_t y, bool flashOnTrue) {
    if (!(frameCount & 0x8) == flashOnTrue) {
        return;
    }
    uint8_t color = WHITE;
    if ((x ^ y) & 1 && (getPieceAt(x, y) & 1) == 0) {
        color = BLACK;
    }
    arduboy.fillRect(X_OFFSET + x * PIECE_SIZE, y * PIECE_SIZE,
        PIECE_SIZE, PIECE_SIZE, color);
}

void draw() {
    arduboy.clear();
    drawBoard();
    drawCursor();
    arduboy.display();
}

void setup() {
    arduboy.begin();
    arduboy.setFrameRate(60);
    
    initBoard();
}

void loop() {
    if (!(arduboy.nextFrame()))
        return;

    frameCount ++;
    
    checkInput();
    draw();
}
