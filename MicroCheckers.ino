#include <Arduboy.h>
#include "bitmaps.h"

Arduboy arduboy;

#define BOARD_SIZE 8
#define PIECE_SIZE 8
#define X_OFFSET    ((WIDTH - BOARD_SIZE * PIECE_SIZE) / 2)

#define PLAYER_WHITE 0
#define PLAYER_BLACK 1

#define WHITE_PIECE 0b001
#define BLACK_PIECE 0b011
#define WHITE_KING 0b101
#define BLACK_KING 0b111

// key, last three bits:
// 00x -> 0 = no piece there, 1 = piece there
// 0x0 -> 0 = white piece, 1 = black piece
// x00 -> 0 = regular piece, 1 = king piece
uint8_t board[BOARD_SIZE * BOARD_SIZE];

// 0 -> white
// 1 -> black (goes first)
uint8_t currentPlayer = PLAYER_BLACK;
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

int8_t diagonalDistance(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
    int8_t xDist = x1 - x2;
    if (xDist < 0) {
        xDist = -xDist;
    }
    int8_t yDist = y1 - y2;
    if (yDist < 0) {
        yDist = -yDist;
    }
    
    if (xDist == yDist) {
        return xDist;
    }
    return -1;
}

uint8_t getPieceAt(uint8_t x, uint8_t y) {
    return board[y * BOARD_SIZE + x];
}

uint8_t setPieceAt(uint8_t x, uint8_t y, uint8_t value) {
    board[y * BOARD_SIZE + x] = value;
}

bool pieceIsEmpty(uint8_t piece) {
    return !(piece & 1);
}

bool getPiecePlayer(uint8_t piece) {
    return (piece >> 1) & 1;
}

bool pieceIsKing(uint8_t piece) {
    return (piece >> 2) & 1;
}

void initBoard() {
    uint8_t x;
    uint8_t y;
    for (x = 0; x < BOARD_SIZE; x ++) {
        // First, initialise everything to zero
        for (y = 0; y < BOARD_SIZE; y ++) {
            setPieceAt(x, y, 0);
        }
        
        // Add white pieces to every second square at the top
        for (y = 0; y < 3; y ++) {
            if ((x ^ y) & 1) {
                setPieceAt(x, y, WHITE_PIECE);
            }
        }
        // Add black pieces to every second square at the bottom
        for (y = BOARD_SIZE - 3; y < BOARD_SIZE; y ++) {
            if ((x ^ y) & 1) {
                setPieceAt(x, y, );
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
    
    // Move cursor
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
    // Make sure the cursor stays on the board (i.e. with the range 0-7)
    curCursorX &= 0x7;
    curCursorY &= 0x7;
    
    if (aEdge) { // Back
        currentState = 0;
    }
    if (bEdge) {
        
        if (currentState == 0) {
            trySelectPiece();
        }
        else if (currentState == 1) {
            tryMovePiece();
        }
    }
}

void trySelectPiece() {
    uint8_t piece = getPieceAt(curCursorX, curCursorY);
    // TODO: check if there are any pieces that have to jump
    if (pieceIsEmpty(piece) || getPiecePlayer(piece) != currentPlayer) {
        return;
    }
    currentState = 1;
    prevCursorX = curCursorX;
    prevCursorY = curCursorY;
}

void tryMovePiece() {
    // TODO: check if another jump can be made
    // TODO: prevent the player from changing which piece they're moving if they've made a jump and can make another
    uint8_t piece = getPieceAt(prevCursorX, prevCursorY);
    
    // -- Check if the move is valid --
    // In any situation, you can only move onto an empty square
    if (!pieceIsEmpty(getPieceAt(curCursorX, curCursorY))) {
        Serial.println("Can't move to taken square!");
        return;
    }
    
    // Non king pieces can't move backwards
    bool movingUp = curCursorY < prevCursorY;
    if (!pieceIsKing(piece)) {
        // If it's a black piece trying to move down, or a white piece trying to move up, stop it!
        if ((getPiecePlayer(piece) == PLAYER_BLACK && !movingUp) ||
            (getPiecePlayer(piece) == PLAYER_WHITE && movingUp)) {
            
            Serial.println("Non king can't move backways!");
            return;
        }
    }
    
    // Check how far the piece is moving
    uint8_t dist = diagonalDistance(prevCursorX, prevCursorY, curCursorX, curCursorY);
    uint8_t middleX = (prevCursorX + curCursorX) / 2;
    uint8_t middleY = (prevCursorY + curCursorY) / 2;
    uint8_t jumpedPiece = getPieceAt(middleX, middleY);
    // Moving one unit diagonally is ok
    if (dist == 1) {
        // Dont need any extra checks
    }
    // jumping over a piece requires the piece being jumped over to be the opposite color.
    else if (dist == 2) {
        if (pieceIsEmpty(jumpedPiece) || getPiecePlayer(jumpedPiece) == currentPlayer) {
            Serial.println("Need a piece to jump over!");
            return;
        }
    }
    else {
        Serial.println("Moving way too far!");
        return;
    }
    
    // -- Make the move --
    currentState = 0;
    setPieceAt(curCursorX, curCursorY, piece);
    // Clear the previous position of the piece
    setPieceAt(prevCursorX, prevCursorY, 0);
    if (dist == 2) {
        // Clear the piece that was jumped over.
        setPieceAt(middleX, middleY, 0);
    }
    
    currentPlayer = !currentPlayer;
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
            switch (piece & 0b111) {
                case WHITE_PIECE:
                    bitmap = whitePiece;
                    break;
                case BLACK_PIECE:
                    bitmap = blackPiece;
                    break;
                case WHITE_KING:
                    bitmap = whiteKing;
                    break;
                case BLACK_KING:
                    bitmap = blackPiece;
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
    
    Serial.begin(9600);
    
    initBoard();
}

void loop() {
    if (!(arduboy.nextFrame()))
        return;

    frameCount ++;
    
    checkInput();
    draw();
}
