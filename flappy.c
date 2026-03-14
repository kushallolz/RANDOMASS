#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#endif

/*
 * Simple terminal Flappy Bird style game
 * --------------------------------------
 * Controls:
 *   SPACE = flap up
 *   q     = quit
 */

#define WIDTH 50
#define HEIGHT 20
#define PIPE_WIDTH 3
#define PIPE_GAP 6
#define PIPE_COUNT 4
#define PIPE_SPACING 16
#define GRAVITY 1
#define FLAP_STRENGTH -3
#define FRAME_DELAY_MS 90

typedef struct {
    int x;
    int gapY;
    int passed;
} Pipe;

static int birdY;
static int birdVelocity;
static int score;
static int gameOver;
static Pipe pipes[PIPE_COUNT];

#ifndef _WIN32
/* Store terminal settings so we can restore them on exit */
static struct termios originalTermios;
#endif

/* ----------- Platform specific console helpers ----------- */
void sleepMs(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

void clearScreen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

#ifndef _WIN32
/* Enable non-canonical mode so keypresses can be read immediately */
void setupInput(void) {
    struct termios newTermios;
    tcgetattr(STDIN_FILENO, &originalTermios);
    newTermios = originalTermios;
    newTermios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);

    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

/* Restore terminal settings before exiting */
void restoreInput(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
}

/* Replacement for conio.h style helpers on non-Windows */
int kbhit(void) {
    int ch = getchar();
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

int getch(void) {
    return getchar();
}
#endif

/* ----------- Game logic functions ----------- */

/* Return random gap start row for a pipe */
int randomGapY(void) {
    /* Keep one row border above and below gap area */
    return 2 + rand() % (HEIGHT - PIPE_GAP - 3);
}

/* Initialize bird, score and pipes */
void initGame(void) {
    int i;

    birdY = HEIGHT / 2;
    birdVelocity = 0;
    score = 0;
    gameOver = 0;

    for (i = 0; i < PIPE_COUNT; i++) {
        pipes[i].x = WIDTH + i * PIPE_SPACING;
        pipes[i].gapY = randomGapY();
        pipes[i].passed = 0;
    }
}

/* Handle user input (SPACE flap, q quit) */
void handleInput(void) {
    if (kbhit()) {
        int key = getch();

        if (key == ' ') {
            birdVelocity = FLAP_STRENGTH;
        } else if (key == 'q' || key == 'Q') {
            gameOver = 1;
        }
    }
}

/* Apply gravity and move bird */
void updateBird(void) {
    birdVelocity += GRAVITY;
    birdY += birdVelocity;

    /* Stop at ceiling */
    if (birdY < 1) {
        birdY = 1;
        birdVelocity = 0;
    }
}

/* Move pipes left and recycle off-screen pipes */
void updatePipes(void) {
    int i;
    int farthestX = 0;

    for (i = 0; i < PIPE_COUNT; i++) {
        pipes[i].x--;
        if (pipes[i].x > farthestX) {
            farthestX = pipes[i].x;
        }
    }

    for (i = 0; i < PIPE_COUNT; i++) {
        if (pipes[i].x + PIPE_WIDTH < 0) {
            pipes[i].x = farthestX + PIPE_SPACING;
            pipes[i].gapY = randomGapY();
            pipes[i].passed = 0;
            farthestX = pipes[i].x;
        }
    }
}

/* Increase score when bird passes a pipe */
void updateScore(void) {
    int i;
    const int birdX = 10;

    for (i = 0; i < PIPE_COUNT; i++) {
        if (!pipes[i].passed && pipes[i].x + PIPE_WIDTH < birdX) {
            score++;
            pipes[i].passed = 1;
        }
    }
}

/* Check collision with ground or pipe */
void checkCollision(void) {
    int i;
    const int birdX = 10;

    /* Ground collision */
    if (birdY >= HEIGHT - 1) {
        gameOver = 1;
        return;
    }

    /* Pipe collision */
    for (i = 0; i < PIPE_COUNT; i++) {
        if (birdX >= pipes[i].x && birdX < pipes[i].x + PIPE_WIDTH) {
            if (birdY < pipes[i].gapY || birdY >= pipes[i].gapY + PIPE_GAP) {
                gameOver = 1;
                return;
            }
        }
    }
}

/* Draw current frame as plain text */
void drawScreen(void) {
    int x, y, i;
    const int birdX = 10;

    clearScreen();

    /* Top border */
    for (x = 0; x < WIDTH; x++) {
        putchar('=');
    }
    putchar('\n');

    for (y = 1; y < HEIGHT - 1; y++) {
        for (x = 0; x < WIDTH; x++) {
            char ch = ' ';

            /* Draw bird */
            if (x == birdX && y == birdY) {
                ch = 'O';
            }

            /* Draw pipes */
            for (i = 0; i < PIPE_COUNT; i++) {
                if (x >= pipes[i].x && x < pipes[i].x + PIPE_WIDTH) {
                    if (y < pipes[i].gapY || y >= pipes[i].gapY + PIPE_GAP) {
                        ch = '|';
                    }
                }
            }

            putchar(ch);
        }
        putchar('\n');
    }

    /* Bottom border */
    for (x = 0; x < WIDTH; x++) {
        putchar('=');
    }
    putchar('\n');

    printf("Score: %d\n", score);
    printf("Press SPACE to flap, Q to quit\n");
}

int main(void) {
    srand((unsigned int)time(NULL));

#ifndef _WIN32
    setupInput();
#endif

    initGame();

    while (!gameOver) {
        handleInput();
        updateBird();
        updatePipes();
        updateScore();
        checkCollision();
        drawScreen();
        sleepMs(FRAME_DELAY_MS);
    }

    clearScreen();
    printf("Game Over! Final Score: %d\n", score);

#ifndef _WIN32
    restoreInput();
#endif

    return 0;
}
