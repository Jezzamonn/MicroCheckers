#include <Arduboy.h>
#include "bitmaps.h"

#define BOARD_SIZE 8
#define PIECE_SIZE 8
#define X_OFFSET    ((WIDTH - BOARD_SIZE * PIECE_SIZE) / 2)

#define PLAYER_WHITE (false)
#define PLAYER_BLACK (true)

#define WHITE_PIECE 0b001
#define BLACK_PIECE 0b011
#define WHITE_KING 0b101
#define BLACK_KING 0b111

#define PRINT_DEBUG

Arduboy arduboy;

// key, last three bits:
// 00x -> 0 = no piece there, 1 = piece there
// 0x0 -> 0 = white piece, 1 = black piece
// x00 -> 0 = regular piece, 1 = king piece
uint8_t board[BOARD_SIZE * BOARD_SIZE];

#define STATE_TITLE 0
#define STATE_GAME 1
#define STATE_INSTR 2
uint8_t gameState = STATE_TITLE;

// 0 -> white
// 1 -> black (goes first)
uint8_t currentPlayer = PLAYER_BLACK;
// 0 -> selecting a piece
// 1 -> selecting a move
uint8_t currentMoveState = 0;
bool canCancel = true;
bool mustJump = false;

uint8_t curCursorX = 0;
uint8_t curCursorY = 0;
// This cursor stays on the selected piece when the position for that piece is being chosen.
uint8_t prevCursorX = 0;
uint8_t prevCursorY = 0;

uint8_t frameCount = 0;

uint8_t char_start = 0;

bool leftPressed = false;
bool rightPressed = false;
bool upPressed = false;
bool downPressed = false;
bool aPressed = false;
bool bPressed = false;

#define NAME_LENGTH 5
char blackPlayerName[] = "Black";
char whitePlayerName[] = "White";

/* ------------- HELPERS ------------- */
//{
void delayForFrames(uint8_t n) {
    while (n > 0) {
        while(!(arduboy.nextFrame()));
        n --;
    }
}

void writeText(uint8_t x, uint8_t y, char* message) {
    arduboy.setCursor(x, y);
    uint16_t i;
    for (i = 0; message[i] != 0; i ++) {
        arduboy.write(message[i]);
    }
}

void writeOption(uint8_t x, uint8_t y, char* message, bool selected) {
    if (selected) {
        message[0] = (frameCount & 8) ? 0x07 : 0x09;
    }
    else {
        message[0] = ' ';
    }
    writeText(x, y, message);
}

#ifdef PRINT_DEBUG
void printPoint(int16_t x, int16_t y) {
    Serial.print("(");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y);
    Serial.print(")");
}
#endif // PRINT_DEBUG
//}


/* ------------- BOARD FUNCTIONS ------------- */
//{
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
                setPieceAt(x, y, BLACK_PIECE);
            }
        }
    }
}

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

bool coordOnBoard(uint8_t x, uint8_t y) {
    return x >= 0 && y >= 0 && x < BOARD_SIZE && y < BOARD_SIZE;
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

void makePieceKing(uint8_t x, uint8_t y) {
    uint8_t piece = getPieceAt(x, y);
    piece |= 0b100;
    setPieceAt(x, y, piece);
}

bool somePieceCanJump(bool player) {
    uint8_t x;
    uint8_t y;
    for (x = 0; x < BOARD_SIZE; x ++) {
        for (y = 0; y < BOARD_SIZE; y ++) {
            uint8_t piece = getPieceAt(x, y);
            if (pieceIsEmpty(piece) || getPiecePlayer(piece) != player) {
                continue;
            }
            if (pieceAtCoordCanJump(x, y)) {
                #ifdef PRINT_DEBUG
                Serial.print("A piece can jump! At ");
                printPoint(x, y);
                Serial.println();
                #endif // PRINT_DEBUG
                return true;
            }
        }
    }
    return false;
}

bool pieceAtCoordCanJump(uint8_t x, uint8_t y) {
    // assumes there is a piece at this position
    uint8_t piece = getPieceAt(x, y);
    
    // check moving up
    if (pieceIsKing(piece)) {
        // jumping up or down
        return pieceAtCoordCanJumpInDir(x, y, -1, -1) ||
               pieceAtCoordCanJumpInDir(x, y,  1, -1) ||
               pieceAtCoordCanJumpInDir(x, y, -1, 1) ||
               pieceAtCoordCanJumpInDir(x, y,  1, 1);
    }
    else if (getPiecePlayer(piece) == PLAYER_BLACK) {
        // jumping up only
        return pieceAtCoordCanJumpInDir(x, y, -1, -1) ||
               pieceAtCoordCanJumpInDir(x, y,  1, -1);
    }
    else { // PLAYER_WHITE
        // jumping down only
        return pieceAtCoordCanJumpInDir(x, y, -1, 1) ||
               pieceAtCoordCanJumpInDir(x, y,  1, 1);
    }
}

bool pieceAtCoordCanJumpInDir(uint8_t x, uint8_t y, int8_t xDir, int8_t yDir) {
    // assumes there is a piece at this position
    uint8_t piece = getPieceAt(x, y);
    uint8_t jumpedX = x + xDir;
    uint8_t jumpedY = y + yDir;
    uint8_t landX = jumpedX + xDir;
    uint8_t landY = jumpedY + yDir;
    
    // can't jump off the board!
    if (!coordOnBoard(jumpedX, jumpedY) || !coordOnBoard(landX, landY)) {
        return false;
    }
    
    uint8_t jumpedPiece = getPieceAt(jumpedX, jumpedY);
    uint8_t landPiece = getPieceAt(landX, landY);
    
    return !pieceIsEmpty(jumpedPiece) &&
           pieceIsEmpty(landPiece) &&
           getPiecePlayer(piece) != getPiecePlayer(jumpedPiece);
}

bool pieceAtCoordCanMoveInDir(uint8_t x, uint8_t y, int8_t xDir, int8_t yDir) {
    uint8_t moveX = x + xDir;
    uint8_t moveY = y + yDir;
    if (!coordOnBoard(moveX, moveY)) {
        return false;
    }
    uint8_t movePosition = getPieceAt(moveX, moveY);
    
    return pieceIsEmpty(movePosition);
}

bool hasPieceOnBoard(bool player) {
    uint8_t x;
    uint8_t y;
    for (x = 0; x < BOARD_SIZE; x ++) {
        for (y = 0; y < BOARD_SIZE; y ++) {
            uint8_t piece = getPieceAt(x, y);
            if (!pieceIsEmpty(piece) && getPiecePlayer(piece) == player) {
                return true;
            }
        }
    }
    return false;
}
//}

/* ------------- LOGIC ------------- */
//{
void update() {
    checkInput();
}

void switchState(uint8_t state) {
    switch (state) {
        case STATE_GAME:
            initBoard();
            curCursorX = BOARD_SIZE / 2;
            curCursorY = BOARD_SIZE / 2;
            startTurn(PLAYER_BLACK);
            break;
        case STATE_INSTR:
            break;
        case STATE_TITLE:
            currentMoveState = 0;
            break;
    }
    gameState = state;
}

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
    
    switch (gameState) {
        case STATE_TITLE:
            updateMenuInput(leftEdge, rightEdge, upEdge, downEdge, aEdge, bEdge);
            break;
        case STATE_GAME:
            updateGameInput(leftEdge, rightEdge, upEdge, downEdge, aEdge, bEdge);
            break;
        case STATE_INSTR:
            updateInstrInput(leftEdge, rightEdge, upEdge, downEdge, aEdge, bEdge);
            break;
    }
}

void updateMenuInput(bool leftEdge, bool rightEdge, bool upEdge, bool downEdge, bool aEdge, bool bEdge) {
    if (upEdge && currentMoveState > 0) {
        currentMoveState --;
    }
    if (downEdge && currentMoveState < 1) {
        currentMoveState ++;
    }
    
    if (bEdge) {
        if (currentMoveState == 0) {
            switchState(STATE_GAME);
        }
        else {
            switchState(STATE_INSTR);
        }
    }
}

void updateInstrInput(bool leftEdge, bool rightEdge, bool upEdge, bool downEdge, bool aEdge, bool bEdge) {
    if (aEdge || bEdge) {
        switchState(STATE_TITLE);
    }
}

void updateGameInput(bool leftEdge, bool rightEdge, bool upEdge, bool downEdge, bool aEdge, bool bEdge) {
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
        if (canCancel) {
            currentMoveState = 0;
        }
        else {
            playInvalidSound();
        }
    }
    if (bEdge) {
        if (!hasPieceOnBoard(currentPlayer)) {
            switchState(STATE_TITLE);
        }
        else if (currentMoveState == 0) {
            trySelectPiece();
        }
        else if (currentMoveState == 1) {
            tryMovePiece();
        }
    }
}

void trySelectPiece() {
    uint8_t piece = getPieceAt(curCursorX, curCursorY);
    // TODO: check if there are any pieces that have to jump
    if (pieceIsEmpty(piece) || getPiecePlayer(piece) != currentPlayer) {
        playInvalidSound();
        return;
    }
    if (mustJump && !pieceAtCoordCanJump(curCursorX, curCursorY)) {
        playInvalidSound();
        return;
    }
    currentMoveState = 1;
    prevCursorX = curCursorX;
    prevCursorY = curCursorY;
    
    playPieceSelect();
}

void tryMovePiece() {
    // TODO: check if another jump can be made
    // TODO: prevent the player from changing which piece they're moving if they've made a jump and can make another
    uint8_t piece = getPieceAt(prevCursorX, prevCursorY);
    
    // -- Check if the move is valid --
    // In any situation, you can only move onto an empty square
    if (!pieceIsEmpty(getPieceAt(curCursorX, curCursorY))) {
        #ifdef PRINT_DEBUG
        Serial.println("Can't move to taken square!");
        #endif // PRINT_DEBUG
        playInvalidSound();
        return;
    }
    
    // Non king pieces can't move backwards
    bool movingUp = curCursorY < prevCursorY;
    if (!pieceIsKing(piece)) {
        // If it's a black piece trying to move down, or a white piece trying to move up, stop it!
        if ((getPiecePlayer(piece) == PLAYER_BLACK && !movingUp) ||
            (getPiecePlayer(piece) == PLAYER_WHITE && movingUp)) {
            
            #ifdef PRINT_DEBUG
            Serial.println("Non king can't move backways!");
            #endif // PRINT_DEBUG
            playInvalidSound();
            return;
        }
    }
    
    // Check how far the piece is moving
    uint8_t dist = diagonalDistance(prevCursorX, prevCursorY, curCursorX, curCursorY);
    uint8_t middleX = (prevCursorX + curCursorX) / 2;
    uint8_t middleY = (prevCursorY + curCursorY) / 2;
    uint8_t jumpedPiece = getPieceAt(middleX, middleY);
    // Moving one unit diagonally
    if (dist == 1) {
        // If there is a jump, then you can't just move one piece.
        if (mustJump) {
            #ifdef PRINT_DEBUG
            Serial.println("You must jump!");
            #endif // PRINT_DEBUG
            playInvalidSound();
            return;
        }
    }
    // jumping over a piece requires the piece being jumped over to be the opposite color.
    else if (dist == 2) {
        if (pieceIsEmpty(jumpedPiece) || getPiecePlayer(jumpedPiece) == currentPlayer) {
            #ifdef PRINT_DEBUG
            Serial.println("Need a piece to jump over!");
            #endif // PRINT_DEBUG
            playInvalidSound();
            return;
        }
    }
    else {
        #ifdef PRINT_DEBUG
        Serial.println("Moving way too far!");
        #endif // PRINT_DEBUG
        playInvalidSound();
        return;
    }
    
    // -- Make the move --
    setPieceAt(curCursorX, curCursorY, piece);
    // Clear the previous position of the piece
    setPieceAt(prevCursorX, prevCursorY, 0);
    if (dist == 2) {
        // Clear the piece that was jumped over.
        setPieceAt(middleX, middleY, 0);
    }
    
    // Check if the piece should become a king
    if (!pieceIsKing(piece) && (curCursorY == 0 || curCursorY == BOARD_SIZE - 1)) {
        // You can't move onto your own edge, so we don't need to worry about what color the piece is.
        makePieceKing(curCursorX, curCursorY);
        playGotKingedUp();
        startTurn(!currentPlayer);
    }
    else if (dist == 1) {
        playPieceMove();
        startTurn(!currentPlayer);
    }
    else if (dist == 2) {
        if (pieceAtCoordCanJump(curCursorX, curCursorY)) {
            playCaptureAndContinue();
            // stay in this state
            canCancel = false;
            prevCursorX = curCursorX;
            prevCursorY = curCursorY;
        }
        else {
            playCaptureDone();
            startTurn(!currentPlayer);
        }
    }
}

void startTurn(bool player) {
    currentPlayer = player;
    currentMoveState = 0;
    canCancel = true;
    mustJump = somePieceCanJump(player);
}
//}

/* ------------- DRAWING ------------- */
//{
void draw() {
    arduboy.clear();
    switch (gameState) {
        case STATE_TITLE:
            drawMenu();
            break;
        case STATE_GAME:
            drawBoard();
            drawCursor();
            drawPlayer();
            break;
        case STATE_INSTR:
            drawInstructions();
            break;
       
    }
    arduboy.display();
}

void drawMenu() {
    writeText(25, 10, "MicroCheckers");
    
    writeOption(30, 30, " Start", currentMoveState == 0);
    writeOption(30, 40, " Instructions", currentMoveState == 1); 
}

void drawInstructions() {
    char* instructions = "Use the arrow keys to\n"
                         "move the cursor.\n"
                         "Press B to select a\n"
                         "piece, and press B on\n"
                         "the square you want\n"
                         "to move it to.\n"
                         "Press A to cancel.\n"
    ;
    writeText(0, 1, instructions);
}

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
                    bitmap = blackKing;
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
    if (currentMoveState == 1) {
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

void drawPlayer() {
    char* playerText = currentPlayer == PLAYER_BLACK ? blackPlayerName : whitePlayerName;
    writeText(0, 1, playerText);
}
//}

/* ------------- SFX ------------- */
//{
#define NOTE_A3  220
#define NOTE_D4  294
#define NOTE_Fs4 369
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_D5  587

void playInvalidSound() {
    arduboy.tunes.tone(NOTE_A3, 20);
}

void playPieceSelect() {
    arduboy.tunes.tone(NOTE_D4, 40);
}

void playPieceMove() {
    arduboy.tunes.tone(NOTE_Fs4, 40);
}

void playCaptureAndContinue() {
    arduboy.tunes.tone(NOTE_A4, 40);
}

void playCaptureDone() {
    arduboy.tunes.tone(NOTE_B4, 40);
    delay(40);
    arduboy.tunes.tone(NOTE_D5, 80);
}

void playGotKingedUp() {
    uint16_t notes[] = {NOTE_D4, NOTE_Fs4, NOTE_A4, NOTE_B4, NOTE_D5};
    uint8_t i;
    for (i = 0; i < 5; i ++) {
        arduboy.tunes.tone(notes[i], 40);
        delay(40);
    }
}
//}

/* ------------- SET UP AND SUCH ------------- */
//{
void setup() {
    arduboy.begin();
    arduboy.setFrameRate(60);
    arduboy.audio.on();
    
    #ifdef PRINT_DEBUG
    Serial.begin(9600);
    #endif // PRINT_DEBUG
    
    gameState = STATE_TITLE;
}

void loop() {
    if (!(arduboy.nextFrame()))
        return;

    frameCount ++;
    
    update();
    draw();
}
//}