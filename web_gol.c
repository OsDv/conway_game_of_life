#include <raylib.h>
#include <math.h>
#include <pthread.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


// Multi-threading things
volatile int calculation_in_progress = 0; // flag for next generation calculation thread
pthread_t generation_thread; // used to calculate next generation in parallel
int reset_request = 0; // boolean to clear grid
int randomize_request = 0 ;// boolean to reset the grid with new random distribution
int liveCounterNew = 0; // population in the next generation calculated by the thread
int current_generation = 0; // current generation number, used for debugging
float gridLifeTime = 0.0f; // 
// Window dimentions
#define HEIGHT 900
#define WIDTH 1600
// Game of life grid dimentions
#define GRID_HEIGHT 500
#define GRID_WIDTH 500
const float MAX_ZOOM = WIDTH / 20;
// World borders color rgbw
#define BORDER_COLOR 1.f, 0.0f, 0.0f, 1.0f
// two grids to contains cells stats in the current and next generation
int grid[GRID_HEIGHT][GRID_WIDTH] ={0};
int grid2[GRID_HEIGHT][GRID_WIDTH]={0};
// live cells locations, buffer's data for redering
float live[GRID_HEIGHT*GRID_WIDTH][2];
/*
 * @activeGrid : actual generation grid
 * @swapGrid : the next generation grid
 */
int (*activeGrid)[GRID_HEIGHT][GRID_WIDTH] = &grid;
int (*swapGrid)[GRID_HEIGHT][GRID_WIDTH]= &grid2;
unsigned int liveCounter =0; // actual population
// initialze the grid with random stat
void generateRandomGrid(int grid[GRID_HEIGHT][GRID_WIDTH]);
// calculate the next generation and put the value in @swapGrid and new population in @liveCountNew (old without multi-thread)
void nextGeneration(int grid[GRID_HEIGHT][GRID_WIDTH]);
// Reset the @activeGrid to all dead cells
void resetGrid(int grid[GRID_HEIGHT][GRID_WIDTH]);
//
int emod(int a, int b);
//
void RenderGrid(int grid[GRID_HEIGHT][GRID_WIDTH]);
//
void * work_generate_next_generation(void* args);
//
void ProcessInputs();
//
void DrawGridLInes();
//
const Color CELL_COLOR = RAYWHITE;

// Global variables
float updateTimer = 0.0f;
Camera2D camera={0} ;
float cameraVelocity = GRID_WIDTH / 100;
float cameraZoomRelatedVelocity ;
int targetGenerationsPerSecond = 10; // target generations per second
bool Paused = false; // pause the game
// Old values for mouse
int oldCellX=-1;
int oldCellY=-1;
// Draw Grid Properties
const float GridLinesThickness = 0.05f;
const float GridLinesOffset = GridLinesThickness / 2;
const Color GridLinesColor = {211, 211, 211,100};
bool isDrawGrid = false;
bool isDrawStats = true;


void main_loo();
//
int main(){
    InitWindow(WIDTH,HEIGHT,"Game Of Life");
    SetRandomSeed(GetTime()*__INT_FAST32_MAX__);
    SetRandomSeed(GetRandomValue(0,__INT_FAST32_MAX__));
    SetTargetFPS(60);
    generateRandomGrid(*activeGrid);
    gridLifeTime =0;
    // multi-threading things
    calculation_in_progress =1;
    int err = pthread_create(&generation_thread, NULL, work_generate_next_generation, NULL);
    if ( err != 0) {
        // printf("Error creating thread: %s\n", strerror(err)); // used for debugging
        calculation_in_progress = 0;
    } else {
        // Detach thread so its resources are automatically released when it terminates
        pthread_detach(generation_thread);
        // printf("Thread created successfully\n"); // used for debugging
    }
    //

    camera.zoom = 10;
    cameraZoomRelatedVelocity /= camera.zoom;
    camera.offset =(Vector2) {(float)WIDTH/2,(float)HEIGHT/2};
    camera.target =(Vector2) {(float)(GRID_WIDTH/2),(float)(GRID_HEIGHT/2)};
    
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loo, 0, 1);
#else
    while(!WindowShouldClose()){
        main_loo();
    }
#endif
    CloseWindow();
    return 0x10;
}


void generateRandomGrid(int grid[GRID_HEIGHT][GRID_WIDTH])
{

    int factor=2;
    int startX = (GRID_WIDTH / factor) - (GRID_WIDTH /(factor*2));
    int starty = (GRID_HEIGHT / factor) - (GRID_HEIGHT /(factor*2));

    for (int i=starty; i <starty+ GRID_HEIGHT/factor; i++)
    {
        for (int j=startX; j <startX + GRID_WIDTH/factor; j++)
        {
            int k = GetRandomValue(0,1);
            grid[i][j] = k;
            if (k) liveCounter++;
        }
    }
}
void nextGeneration(int grid[GRID_HEIGHT][GRID_WIDTH])
{
    for (int i = 0; i < GRID_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_WIDTH; j++)
        {
            int liveNeighbours = 0;
            for (int k = i-1; k <= i+1; k++)
            {
                for (int l = j-1; l <= j+1; l++)
                {
                    int ck = emod(k,GRID_HEIGHT);
                    int cl = emod(l,GRID_WIDTH);
                    if (k==i && l==j) continue;
                    if (grid[ck][cl]==1) liveNeighbours++;
                }
            }
            if (grid[i][j] && (liveNeighbours<2 || liveNeighbours>3)){
                (*swapGrid)[i][j] = 0;
                liveCounter--;
            }
            else if (!grid[i][j] && liveNeighbours==3){
                (*swapGrid)[i][j] = 1;
                liveCounter++;
            }
            else (*swapGrid)[i][j] = grid[i][j];
        }

    }
    int (*tem)[GRID_HEIGHT][GRID_WIDTH] = swapGrid;
    swapGrid = activeGrid;
    activeGrid = tem;

}
void resetGrid(int grid[GRID_HEIGHT][GRID_WIDTH])
{
    liveCounter =0;
    for (int i=0; i < GRID_HEIGHT; i++)
    {
        for (int j=0; j < GRID_WIDTH; j++)
        {
            grid[i][j] = 0;
        }
    }
}
int emod(int a, int b)
{
    return ((a%b)+b)%b;
}

void RenderGrid(int grid[GRID_HEIGHT][GRID_WIDTH])
{
    int count =0;
    int stop=0;
    for(int i=0;i<GRID_HEIGHT && !stop;i++){
        for (int j=0;j<GRID_WIDTH && !stop;j++){
            if (grid[i][j]){
                DrawRectangle(j,i,1,1,CELL_COLOR);
            }
            if (count==liveCounter) stop=1;
        }
    }
}

void * work_generate_next_generation(void* args)
{
    liveCounterNew = liveCounter;
    for (int i = 0; i < GRID_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_WIDTH; j++)
        {
            int liveNeighbours = 0;
            for (int k = i-1; k <= i+1; k++)
            {
                for (int l = j-1; l <= j+1; l++)
                {
                    int ck = emod(k,GRID_HEIGHT);
                    int cl = emod(l,GRID_WIDTH);
                    if (k==i && l==j) continue;
                    if ((*activeGrid)[ck][cl]==1) liveNeighbours++;
                }
            }
            if ((*activeGrid)[i][j] && (liveNeighbours<2 || liveNeighbours>3)){
                (*swapGrid)[i][j] = 0;
                liveCounterNew--;
            }
            else if (!(*activeGrid)[i][j] && liveNeighbours==3){
                (*swapGrid)[i][j] = 1;
                liveCounterNew++;
            }
            else (*swapGrid)[i][j] = (*activeGrid)[i][j];
        }

    }

    calculation_in_progress =0;
    return NULL;
}

void main_loo()
{
        ProcessInputs();
        if (!Paused){
            float deltaTIme = GetFrameTime();
            updateTimer+=deltaTIme*targetGenerationsPerSecond;
            gridLifeTime += GetFrameTime();
        }
        // single thread version --(for Github Pages)
        /*if (updateTimer>=1.0f){
            updateTimer=0;
            nextGeneration(*activeGrid);
            if (reset_request)  
            {
                resetGrid(*activeGrid);
                reset_request = 0;
                current_generation = 0; // reset generation count
                gridLifeTime =0; // reset the grid life time
                if (randomize_request) {
                    generateRandomGrid(*activeGrid);
                    randomize_request =  0;
                }
            }
        }*/
        
        // multi-threading version
        if (updateTimer >= 1.0 && calculation_in_progress==0)
        {
            updateTimer = 0.0f;
            // Swap the grids
            int (*tem)[GRID_HEIGHT][GRID_WIDTH] = swapGrid;
            swapGrid = activeGrid;
            activeGrid = tem;
            liveCounter = liveCounterNew;
            current_generation++;
            // If reset is requested, reset the grid and generate a new random distribution
            if (reset_request)
            {
                resetGrid(*activeGrid);
                reset_request = 0;
                current_generation = 0; // reset generation count
                gridLifeTime =0; // reset the grid life time
                if (randomize_request) {
                    generateRandomGrid(*activeGrid);
                    randomize_request =  0;
                }
            }
            // Start a new calculation
            calculation_in_progress = 1;
            int err = pthread_create(&generation_thread, NULL, work_generate_next_generation, NULL);
            if ( err != 0) {
                //printf("Error creating thread: %s\n", strerror(err)); // used for debugging
                calculation_in_progress = 0;
            } else {
                // Detach thread so its resources are automatically released when it terminates
                pthread_detach(generation_thread);
                //printf("Thread created successfully\n"); //used for debugging
            }
        }





    

        BeginDrawing();
        ClearBackground(BLACK);
            BeginMode2D(camera);
            if (isDrawGrid) DrawGridLInes();
            RenderGrid(*activeGrid);
            DrawRectangleLines( 0, 0, GRID_WIDTH, GRID_HEIGHT, BLUE);
            EndMode2D();

            if(isDrawStats){
                DrawRectangle( 10, 10, 500, 155, Fade(SKYBLUE, 0.5f));
                DrawRectangleLines( 10, 10, 500, 155, BLUE);
                float fontSize=20;
                DrawText("Game controls:", 20, 20, 15, BLACK);
                DrawText("- WASD to move Camera", 40, 40, fontSize, CELL_COLOR);
                DrawText("- Mouse Wheel to Zoom in-out", 40, 60, fontSize, CELL_COLOR);
                DrawText("- R to Randomize the grid", 40, 80, fontSize, CELL_COLOR);
                DrawText("- C to Clear the grid", 40, 100, fontSize, CELL_COLOR);
                DrawText("- [SPACE] to PAUSE/RESUME", 40, 120, fontSize, CELL_COLOR);
                DrawText(TextFormat("- DOWN/UP Adjust update speed: %d gen/s",targetGenerationsPerSecond), 40, 140, fontSize, CELL_COLOR);
                // Stats
                DrawRectangle( 10, 200, 350, 100, Fade(LIME, 0.5f));
                DrawRectangleLines( 10, 200, 350, 100, GREEN);
                DrawText(TextFormat("Generation: %d", current_generation), 40, 230, 20, CELL_COLOR);
                DrawText(TextFormat("Population: %d", liveCounter), 40, 250, 20, CELL_COLOR);
                DrawText(TextFormat("Generations per second: %.2f ",current_generation /gridLifeTime), 40, 270, 20, CELL_COLOR);
                DrawText("[TAB] TO HIDE", 50, 310, 25, Fade(YELLOW,0.7f));
            }

        EndDrawing();
}

void ProcessInputs()
{
    if (IsKeyPressed(KEY_R)) {
        //resetGrid(*activeGrid);
        //generateRandomGrid(*activeGrid);
        reset_request = 1;
        randomize_request = 1;
    }
    if (IsKeyPressed(KEY_C)){ // Clear grid
        reset_request = 1; // request a reset, multi-threading version
    }
    if (IsKeyPressed(KEY_SPACE)) {
        Paused = !Paused; // toggle pause state
        updateTimer = 0.0f; // reset the timer when paused
        if (!Paused){
            calculation_in_progress =1;
            pthread_create(&generation_thread, NULL, work_generate_next_generation, NULL);
            gridLifeTime = 0;
            current_generation = 0;
        }
    }
    // Toggle Stats draw
    if (IsKeyPressed(KEY_TAB)) isDrawStats=!isDrawStats;
    if (IsKeyPressed(KEY_X)){
        isDrawGrid = !isDrawGrid;
    }
    // Increase and decrease target generation per second
    if (IsKeyPressed(KEY_UP)) targetGenerationsPerSecond++;
    else if (IsKeyPressed(KEY_DOWN)) targetGenerationsPerSecond--;

        // camera Movement controls
        // Horizontal movement
    cameraZoomRelatedVelocity = cameraVelocity / camera.zoom;
    if (IsKeyDown(KEY_A)) camera.target.x -=cameraZoomRelatedVelocity;
    if (IsKeyDown(KEY_D)) camera.target.x+=cameraZoomRelatedVelocity;
    if (camera.target.x > GRID_WIDTH) camera.target.x = GRID_WIDTH;
    if (camera.target.x < 0) camera.target.x =0;
    // Vertical movement
    if (IsKeyDown(KEY_W)) camera.target.y -=cameraZoomRelatedVelocity;
    if (IsKeyDown(KEY_S)) camera.target.y+=cameraZoomRelatedVelocity;
    // Check camera bounds
    if (camera.target.y > GRID_HEIGHT) camera.target.y = GRID_HEIGHT;
    if (camera.target.y < 0) camera.target.y =0;
        // Camera zoom controls
    // Uses log scaling to provide consistent zoom speed
    camera.zoom = expf(logf(camera.zoom) + ((float)GetMouseWheelMove()*0.1f));
    if (camera.zoom > MAX_ZOOM) camera.zoom = MAX_ZOOM;
    else if (camera.zoom < 0.5f) camera.zoom = 0.5f;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
        Vector2 mousePos = GetMousePosition();
        Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
        int cellX = (int)(worldPos.x);
        int cellY = (int)(worldPos.y);
        if (oldCellX!=cellX || oldCellY!=cellY){
            if (cellX >= 0 && cellX < GRID_WIDTH && cellY >= 0 && cellY < GRID_HEIGHT) {
                (*activeGrid)[cellY][cellX] = !(*activeGrid)[cellY][cellX]; // toggle cell state
                if ((*activeGrid)[cellY][cellX]) liveCounter++; // increment live counter if cell is alive
                else liveCounter--; // decrement live counter if cell is dead
            }
            oldCellX = cellX;
            oldCellY = cellY;
        }
        // pause and treat it like new game if a cell manually changed
        Paused = true; 

    }
    //Vector2 GetScreenToWorld2D(Vector2 position, Camera2D camera);    // Get the world space position for a 2d camera screen space position
}

void DrawGridLInes()
{
    Vector2 start={0,0};
    Vector2 end={0,GRID_HEIGHT};
    for (int j=1;j<GRID_WIDTH;j++)
    {
        start.x = j - GridLinesOffset;
        end.x = j - GridLinesOffset;
        DrawLineEx(start,end,GridLinesThickness,GridLinesColor);
        // DrawLineV(start,end,GridLinesColor);
    }
    start.x =0;
    end.x = GRID_WIDTH;
    for (int j=1;j<GRID_HEIGHT;j++)
    {
        start.y = j - GridLinesOffset;
        end.y = j - GridLinesOffset;
        DrawLineEx(start,end,GridLinesThickness,GridLinesColor);
        // DrawLineV(start,end,GridLinesColor);
    }
}
