 #include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "display.h"
#include <SDL2/SDL.h>
#include <math.h>

#define BOARD_SIZE 30
#define ROW_SIZE 25
#define COLUMN_SIZE 25
#define WIDTH 600
#define HEIGHT 600
#define CDIM 20 //cell dimension is CDIM * CDIM
#define FILENAME "main.c"

#define FOOD_ENERGY 10
#define TURN_ENERGY 1
#define MOVE_ENERGY 1
#define START_ENERGY 200
#define START_STRENGTH 10

int dx[] = {-1, 0, 1, 0};
int dy[] = {0, 1, 0, -1};
int g_ant_number = 8;

struct display {
    bool testing;
    int width, height;
    SDL_Renderer *renderer;
    char *file;
    FILE *input;
    char *actual;
};

typedef struct Ant {
  bool alive;
  int strength;
  int total_energy_intake;
  int energy;
  int number;
  int fov;//field of view
  int age;
}Ant;

struct cell {
  bool visited;
  Ant ant;
  bool moved;
  bool terrain;//water = 0, land = 1
  bool food;
};

int min(int A, int B) {
    if (A < B)
        return A;
    return B;
}

int max(int A, int B) {
    if (A > B)
        return A;
    return B;
}

static void fail(char *s1, char *s2) {
    fprintf(stderr, "%s %s %s\n", s1, s2, SDL_GetError());
    SDL_Quit();
    exit(1);
}

void line(display *d, int x0, int y0, int x1, int y1) {
    SDL_SetRenderDrawColor(d->renderer, 0, 0, 0, 255);
    int rc = SDL_RenderDrawLine(d->renderer, x0, y0, x1, y1);
    rc++;//rubbish to avoid warning
    SDL_RenderPresent(d->renderer);
}

void initializeMaze(struct cell maze[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 1; i <= ROW_SIZE; ++i) {
        for (int j = 1; j <= COLUMN_SIZE; ++j) {
            maze[i][j].visited = false;
            maze[i][j].moved = false;
            maze[i][j].ant.alive = false;
            maze[i][j].ant.energy = -1;
            maze[i][j].ant.number = -1;
            maze[i][j].ant.fov = -1;
            maze[i][j].ant.age = -1;
            maze[i][j].terrain = true;//everything's land
            maze[i][j].food = false;
        }
    }
}

void createBorder(struct cell maze[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i <= ROW_SIZE + 1; ++i) {
        maze[i][0].visited = maze[i][COLUMN_SIZE + 1].visited = true;
    }
    for (int j = 0; j <= COLUMN_SIZE + 1; ++j) {
        maze[0][j].visited = maze[ROW_SIZE + 1][j].visited = true;
    }
}

void drawRectangle(display *d, int x, int y, int R, int G, int B, int A) {

    SDL_SetRenderDrawColor(d->renderer, R, G, B, A);
    //struct SDL_Rect rekt = {.x = 0, .y = 0, .w = 15, .h = 15};
    struct SDL_Rect rekt = {.x = y * CDIM + CDIM / 4,
                            .y = x * CDIM + CDIM / 4,
                            .w = CDIM,
                            .h = CDIM};
    SDL_RenderDrawRect(d->renderer, &rekt);
    SDL_RenderPresent(d->renderer);
}

display *createWindow(char *file, int width, int height, bool testing) {
    display *d = malloc(sizeof(display));
    d->testing = false;
    d->file = file;
    d->width = width;
    d->height = height;
    int result = SDL_Init(SDL_INIT_EVERYTHING);
    if (result < 0) fail("Bad SDL", "");
    SDL_Window *window;
    int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
    window = SDL_CreateWindow(file, x, y, width, height, 0);
    if (window == NULL) fail("Bad window", "");
    d->renderer = SDL_CreateRenderer(window, -1, 0);
    if (d->renderer == NULL) fail("Bad renderer", "");
    SDL_SetRenderDrawColor(d->renderer, 255, 255, 255, 255);
    SDL_RenderClear(d->renderer);
    SDL_RenderPresent(d->renderer);
    return d;
}

void drawMaze(display *d, struct cell maze[BOARD_SIZE][BOARD_SIZE]) {
    SDL_SetRenderDrawColor(d->renderer, 0, 255, 0, 255);
    SDL_RenderPresent(d->renderer);
    for (int i = 0; i <= ROW_SIZE + 1; ++i) {
        for (int j = 0; j <= COLUMN_SIZE + 1; ++j) {
            if (maze[i][j].visited == true)
                drawRectangle(d, j, i, 0, 0, 0, 255);//black
        }
    }
}

void clear(display *d) {
    SDL_SetRenderDrawColor(d->renderer, 255, 255, 255, 255);
    SDL_RenderClear(d->renderer);
    SDL_RenderPresent(d->renderer);
}

void pause(display *d, int ms) {
    if (ms > 0) SDL_Delay(ms);
}

void end(display *d) {
    SDL_Delay(1000);
    SDL_Quit();
    exit(0);
}

char key(display *d) {
    SDL_Event event_structure;
    SDL_Event *event = &event_structure;
    while (true) {
        //printf("Waiting for a key to be pressed\n");
        int r = SDL_WaitEvent(event);
        if (r == 0) fail("Bad event", "");
        if ( event->key.keysym.sym == SDL_QUIT) {
            SDL_Quit();
            exit(0);
        }else if (event->type == SDL_KEYUP) {
            int sym = event->key.keysym.sym;
            return (char)sym;
        }
    }
}


void drawAnt(display *d, int x, int y, int energy) {
    int G;
    if (energy < 50)
        G = 50;
    else
    if (energy < 100)
        G = 100;
    else
    if (energy < 150)
        G = 150;
    else
    if (energy < 200)
        G = 200;
    else G = 250;
    SDL_SetRenderDrawColor(d->renderer, 255 - G, G, 0, 255);
    //struct SDL_Rect rekt = {.x = 0, .y = 0, .w = 15, .h = 15};
    struct SDL_Rect rekt = {.x = y * CDIM + CDIM / 4 + 3,
                            .y = x * CDIM + CDIM / 4 + 3,
                            .w = CDIM - 5,
                            .h = CDIM - 5};
    SDL_RenderFillRect(d->renderer, &rekt);
    // SDL_RenderDrawRect(d->renderer, &rekt);
    SDL_RenderPresent(d->renderer);
}

void clearAnt(display *d, int x, int y) {

    SDL_SetRenderDrawColor(d->renderer, 255, 255, 255, 255);//white
    //struct SDL_Rect rekt = {.x = 0, .y = 0, .w = 15, .h = 15};

    struct SDL_Rect rekt = {.x = y * CDIM + CDIM / 4 + 3,
                            .y = x * CDIM + CDIM / 4 + 3,
                            .w = CDIM - 5,
                            .h = CDIM - 5};
    SDL_RenderFillRect(d->renderer, &rekt);
    // SDL_RenderDrawRect(d->renderer, &rekt);
    SDL_RenderPresent(d->renderer);
}

void createAnts(struct cell maze[BOARD_SIZE][BOARD_SIZE]) {
    maze[1][1].ant.alive = true;
    maze[1][1].ant.energy = START_ENERGY;
    maze[1][1].ant.number = 1;
    maze[1][1].ant.fov = 1;
    maze[1][1].ant.age = 0;
    maze[1][1].ant.strength = START_STRENGTH;
    maze[1][1].ant.total_energy_intake = 0;


    maze[2][3].ant.alive = true;
    maze[2][3].ant.energy = START_ENERGY;
    maze[2][3].ant.number = 2;
    maze[2][3].ant.fov = 1;
    maze[2][3].ant.age = 0;
    maze[2][3].ant.strength = START_STRENGTH;
    maze[2][3].ant.total_energy_intake = 0;

    maze[10][10].ant.alive = true;
    maze[10][10].ant.energy = START_ENERGY;
    maze[10][10].ant.number = 3;
    maze[10][10].ant.fov = 1;
    maze[10][10].ant.age = 0;
    maze[10][10].ant.strength = START_STRENGTH;
    maze[10][10].ant.total_energy_intake = 0;


    maze[20][20].ant.alive = true;
    maze[20][20].ant.energy = START_ENERGY;
    maze[20][20].ant.number = 4;
    maze[20][20].ant.fov = 1;
    maze[20][20].ant.age = 0;
    maze[20][20].ant.strength = START_STRENGTH;
    maze[20][20].ant.total_energy_intake = 0;

    maze[20][10].ant.alive = true;
    maze[20][10].ant.energy = START_ENERGY;
    maze[20][10].ant.number = 5;
    maze[20][10].ant.fov = 1;
    maze[20][10].ant.age = 0;
    maze[20][10].ant.strength = START_STRENGTH;
    maze[20][10].ant.total_energy_intake = 0;

    maze[10][20].ant.alive = true;
    maze[10][20].ant.energy = START_ENERGY;
    maze[10][20].ant.number = 6;
    maze[10][20].ant.fov = 1;
    maze[10][20].ant.age = 0;
    maze[10][20].ant.strength = START_STRENGTH;
    maze[10][20].ant.total_energy_intake = 0;

    maze[23][23].ant.alive = true;
    maze[23][23].ant.energy = START_ENERGY;
    maze[23][23].ant.number = 7;
    maze[23][23].ant.fov = 1;
    maze[23][23].ant.age = 0;
    maze[23][23].ant.strength = START_STRENGTH;
    maze[23][23].ant.total_energy_intake = 0;
}

int computeBestDirection(struct cell maze[BOARD_SIZE][BOARD_SIZE], int x, int y) {
    int score[4];
    for (int i = 0; i < 4; ++i)
        score[i] = 0;
    int istart = max(0, x - maze[x][y].ant.fov);
    int ifinish = min(COLUMN_SIZE, x + maze[x][y].ant.fov);
    int jstart = max(0, y - maze[x][y].ant.fov);
    int jfinish = min(ROW_SIZE, y + maze[x][y].ant.fov);
    for (int i = istart; i <= ifinish; ++i) {
        for (int j = jstart; j <= jfinish; ++j) {
            if (i < x /*&& !maze[x - 1][y].ant.alive*/) {//north
                if (maze[i][j].visited) score[0] -= 1;
                if (maze[i][j].food) ++score[0];
                if (maze[i][j].ant.alive && maze[i][j].ant.strength <= maze[x][y].ant.strength)
                    score[0] += 2;
                if (maze[i][j].ant.alive && maze[i][j].ant.strength > maze[x][y].ant.strength)
                    score[0] -= 3;
            }
            if (j > y /*&& !maze[x][y + 1].ant.alive*/) {//east
                if (maze[x][y + 1].ant.alive || maze[i][j].visited) score[1] -= 1;
                if (maze[i][j].food) ++score[1];
                if (maze[i][j].ant.alive && maze[i][j].ant.strength <= maze[x][y].ant.strength)
                    score[1] += 2;
                if (maze[i][j].ant.alive && maze[i][j].ant.strength > maze[x][y].ant.strength)
                    score[1] -= 3;
            }
            if (i > x /*&& !maze[x + 1][y].ant.alive*/) {//south
                if (maze[x + 1][y].ant.alive || maze[i][j].visited) score[2] -= 1;
                if (maze[i][j].food) ++score[2];
                if (maze[i][j].ant.alive && maze[i][j].ant.strength <= maze[x][y].ant.strength)
                    score[2] += 2;
                if (maze[i][j].ant.alive && maze[i][j].ant.strength > maze[x][y].ant.strength)
                    score[2] -= 3;
            }
            if (j < y /*&& !maze[x][y - 1].ant.alive*/) {//west
                if (maze[x][y - 1].ant.alive || maze[i][j].visited) score[3] -= 1;
                if (maze[i][j].food) ++score[3];
                if (maze[i][j].ant.alive && maze[i][j].ant.strength <= maze[x][y].ant.strength)
                    score[3] += 2;
                if (maze[i][j].ant.alive && maze[i][j].ant.strength > maze[x][y].ant.strength)
                    score[3] -= 3;
            }
        }
    }
    int best_score_index = rand() % 4;
    for (int i = 0; i < 4; ++i) {
        if ( score[best_score_index] < score[i] ) {
            best_score_index = i;
        }
    }
    return best_score_index;
}

void moveAllAnts(display *d, struct cell maze[BOARD_SIZE][BOARD_SIZE]) {
    int dir, aux_x, aux_y;
    for (int i = 1; i <= ROW_SIZE; ++i) {
        for (int j = 1; j <= COLUMN_SIZE; ++j) {
            if (maze[i][j].ant.alive && !maze[i][j].moved) {
                ++maze[i][j].ant.age;
                maze[i][j].ant.energy -= TURN_ENERGY;//each turn consumes 1 energy point, whether the ant manages to move or not
                if ( maze[i][j].ant.energy <= 0) {
                    printf("Ant #%d died of low energy\n", maze[i][j].ant.number);
                    maze[i][j].ant.alive = false;
                    clearAnt(d, i, j);
                    continue;
                }
                //int stop_after = 0;
                dir = computeBestDirection(maze, i, j);
                aux_x = i + dx[dir];
                aux_y = j + dy[dir];
                //wall, so can't go
                if (maze[aux_x][aux_y].visited)
                    continue;
                //can't eat the ant
                if (maze[aux_x][aux_y].ant.alive && maze[aux_x][aux_y].ant.strength >= maze[i][j].ant.strength)
                    continue;
                //We are ABSOLUTELY sure we want to move
                maze[i][j].ant.energy -= MOVE_ENERGY;//each move consumes 2 energy points
                maze[aux_x][aux_y].moved = true;
                int temp_energy = maze[aux_x][aux_y].ant.energy;
                bool was_alive = maze[aux_x][aux_y].ant.alive;
                maze[aux_x][aux_y].ant = maze[i][j].ant;

                maze[i][j].ant.alive = false;
                clearAnt(d, i, j);

                if (maze[aux_x][aux_y].food) {
                    maze[aux_x][aux_y].ant.energy += FOOD_ENERGY;
                    maze[aux_x][aux_y].ant.total_energy_intake += FOOD_ENERGY;
                    if (maze[aux_x][aux_y].ant.total_energy_intake % 50 == 0) {
                         maze[aux_x][aux_y].ant.strength++;
                         printf("Ant %d strength increased to %d\n", maze[aux_x][aux_y].ant.number, maze[aux_x][aux_y].ant.strength);
                     }
                     if (maze[aux_x][aux_y].ant.total_energy_intake % 2000 == 0) {
                          maze[aux_x][aux_y].ant.fov++;
                          printf("Ant %d FOV increased to %d\n", maze[aux_x][aux_y].ant.number, maze[aux_x][aux_y].ant.strength);
                      }
                    //printf("Ant #%d energy increased to %d\n", maze[aux_x][aux_y].ant.number, maze[aux_x][aux_y].ant.energy);
                    if (maze[aux_x][aux_y].ant.energy > 250)//limit energy to 1000
                        maze[aux_x][aux_y].ant.energy = 250;
                }
                if (was_alive) {
                    printf("Ant %d ate another ant\n", maze[aux_x][aux_y].ant.number);
                    maze[aux_x][aux_y].ant.energy += 0.2*temp_energy - ((int)0.2*temp_energy) % 10;
                    maze[aux_x][aux_y].ant.total_energy_intake += 0.2*temp_energy - ((int)(0.2*temp_energy)) % 10;
                    if (maze[aux_x][aux_y].ant.total_energy_intake % 50 == 0) {
                         maze[aux_x][aux_y].ant.strength++;
                         printf("Ant %d strength increased to %d\n", maze[aux_x][aux_y].ant.number, maze[aux_x][aux_y].ant.strength);
                    }
                    if (maze[aux_x][aux_y].ant.total_energy_intake % 2000 == 0) {
                         maze[aux_x][aux_y].ant.fov++;
                         printf("Ant %d strength increased to %d\n", maze[aux_x][aux_y].ant.number, maze[aux_x][aux_y].ant.strength);
                     }
                }

                if ( maze[aux_x][aux_y].ant.energy <= 0 ) {
                    printf("Ant #%d died\n", maze[aux_x][aux_y].ant.number);
                    maze[aux_x][aux_y].ant.alive = false;
                }else {
                    drawAnt(d, aux_x, aux_y, maze[aux_x][aux_y].ant.energy);//new position
                    int birth_probability = rand() % 100;
                    //for every 50 energy points, +2% chance of baby
                    if (birth_probability < 2 * (maze[aux_x][aux_y].ant.energy / 50)) {
                        maze[i][j].ant.alive = true;
                        maze[i][j].ant.energy = 10;
                        maze[i][j].ant.fov = 1;
                        maze[i][j].ant.age = 0;
                        maze[i][j].ant.total_energy_intake = 0;
                        maze[i][j].ant.strength = 10;
                        printf("Ant #%d gave birth to ant...%d\n", maze[aux_x][aux_y].ant.number, g_ant_number);
                        maze[i][j].ant.number = g_ant_number++;
                        maze[aux_x][aux_y].ant.energy = (double)50/100 * maze[aux_x][aux_y].ant.energy;//90% energy left after giving birth
                        //with the current rule, ants do not die when giving birth
                        if ( maze[aux_x][aux_y].ant.energy <= 0 ) {
                            printf("Ant #%d died after giving birth...\n", maze[aux_x][aux_y].ant.number);
                            maze[aux_x][aux_y].ant.alive = false;
                            clearAnt(d, aux_x, aux_y);
                        }
                        //child born on old position, we we don't care about overlapping
                        drawAnt(d, i, j, maze[aux_x][aux_y].ant.energy);
                    }
                }
                //pause(d, 1000);
            }
        }
    }
  //reset move boolean values
    for (int i = 1; i <= ROW_SIZE; ++i) {
        for (int j = 1; j <= COLUMN_SIZE; ++j) {
            maze[i][j].moved = false;
        }
    }
}

void generateFood(display *d, struct cell maze[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 1; i <= ROW_SIZE; ++i) {
        for (int j = 1; j <= COLUMN_SIZE; ++j) {
            int probability = rand() % 100;
            if (!maze[i][j].ant.alive && probability < 1) {//2% chance of food
                maze[i][j].food = true;
                SDL_SetRenderDrawColor(d->renderer, 255, 255, 20, 150);
                //struct SDL_Rect rekt = {.x = 0, .y = 0, .w = 15, .h = 15};
                struct SDL_Rect rekt = {.x = j * CDIM + CDIM / 4 + 3,
                                        .y = i * CDIM + CDIM / 4 + 3,
                                        .w = CDIM - 5,
                                        .h = CDIM - 5};
                SDL_RenderFillRect(d->renderer, &rekt);
                SDL_RenderPresent(d->renderer);
            }
        }
    }
}

int main() {
    srand(time(NULL));
    struct cell maze[BOARD_SIZE][BOARD_SIZE];
    printf("Creating window...\n");
    display *d = createWindow(FILENAME, WIDTH, HEIGHT, false);
    printf("Initializing maze...\n");
    initializeMaze(maze);
    printf("Creating borders...\n");
    createBorder(maze);
    printf("Creating ants...\n");
    createAnts(maze);
    printf("Drawing Maze...\n");
    drawMaze(d, maze);
    for (int i = 0; i < 10; ++i) {
        printf("Generating food...\n");
        generateFood(d, maze);//generate food every 16th move
    }
    //main thread
    int flag = 0;
    bool quit = false;
    while ( !quit ) {
        moveAllAnts(d, maze);
        if (!flag) {
            printf("Generating food...\n");
            generateFood(d, maze);//generate food every 16th move
            char c = key(d);
            if (c == 'q') {
                end(d);
            }
        }

        flag++;
        flag %= 16;
        pause(d, 250);
        //printf("\n");
    }
    printf("Congratulations! You've solved the maze...\n");
    printf("Closing window...\n");
    end(d);
    return 0;
}
