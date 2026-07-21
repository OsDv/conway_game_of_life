#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <time.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


#include "./shaders/es_draw.fs.h"
#include "./shaders/es_gol.fs.h"
#include "./shaders/es_web.vs.h"
#include "./shaders/gol.fs.h"
#include "./shaders/manualDraw.fs.h"

int current_generation = 0; // current generation number, used for debugging
float gridLifeTime = 0.0f;  //
float updateTimer = 0;
// Window dimentions
#define HEIGHT 900
#define WIDTH 1600
// Game of life grid dimentions
#define GRID_HEIGHT 1000
#define GRID_WIDTH 1000
const float MAX_ZOOM = WIDTH / 20;
#define CELL_COLOR WHITE
// Global variables
Camera2D camera = {0};
float cameraVelocity = GRID_WIDTH / 100;
float cameraZoomRelatedVelocity;
int targetGenerationsPerSecond = 10; // target generations per second
// int liveCounter=0;
unsigned int liveCounterLocation = 0;
// Manualy Draw buffer
#define MAX_DRAW_COMMANDS 10
int drawLocation[2] = {-1, -1};
bool isDrawCommand = false;
// FPS Setting
int FPS_VALUES[] = {40, 60, 90, 120, 144};
int FPS_VALUES_COUNT = 5;
int current_fps = 1;

// Functions
//
void ProcessInputs();
// main loop
void main_loop();
//
void DrawGridLInes();
//
void resetGrid(RenderTexture2D tex);
//
void generateRandomGrid(RenderTexture2D tex);
// Global Variables
bool Paused = false;
bool isDrawGrid = false;
bool isDrawStats = true;
// Draw Grid Properties
const float GridLinesThickness = 0.05f;
const float GridLinesOffset = GridLinesThickness / 2;
const Color GridLinesColor = {211, 211, 211, 100};
//
// Shader shader;

RenderTexture2D state[2];
int selectedTexture = 0;
//
Shader ManualDrawShader;
Shader shader;
unsigned int invertionTargetLoc;

int main() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
  InitWindow(WIDTH, HEIGHT, "GPU Game Of Life");
  SetTargetFPS(60);
  SetRandomSeed(time(NULL));
  SetWindowSize(GetScreenWidth(), GetScreenHeight());

  state[0] = LoadRenderTexture(GRID_WIDTH, GRID_HEIGHT);
  SetTextureWrap(state[0].texture, TEXTURE_WRAP_REPEAT);
  SetTextureFilter(state[0].texture, TEXTURE_FILTER_POINT);

  state[1] = LoadRenderTexture(GRID_WIDTH, GRID_HEIGHT);
  SetTextureWrap(state[1].texture, TEXTURE_WRAP_REPEAT);
  SetTextureFilter(state[1].texture, TEXTURE_FILTER_POINT);

  // Set Camera
  camera.zoom = 10;
  cameraZoomRelatedVelocity /= camera.zoom;
  camera.offset = (Vector2){(float)WIDTH / 2, (float)HEIGHT / 2};
  camera.target = (Vector2){(float)(GRID_WIDTH / 2), (float)(GRID_HEIGHT / 2)};
  // Set first generation Random
  generateRandomGrid(state[selectedTexture]);

#ifdef __EMSCRIPTEN__
  // ManualDrawShader = LoadShader(__es_web_vs, __es_draw_fs);
  ManualDrawShader = LoadShaderFromMemory(es_web_vs, __es_draw_fs);
  shader = LoadShaderFromMemory(__es_web_vs, __es_gol_fs);
  if (!IsShaderValid(ManualDrawShader))
    return 0x10;
  Vector2 gridSize = {GRID_WIDTH, GRID_HEIGHT};
  int size_loc = GetShaderLocation(shader, "gridSize");
  SetShaderValue(shader, size_loc, &gridSize, SHADER_UNIFORM_VEC2);
  size_loc = GetShaderLocation(ManualDrawShader, "gridSize");
  SetShaderValue(ManualDrawShader, size_loc, &gridSize, SHADER_UNIFORM_VEC2);
#else
  // ManualDrawShader = LoadShader(NULL, "manualDraw.fs");
  // shader header now generated with "xdd -i -t shaderfile > header"
  ManualDrawShader = LoadShaderFromMemory(NULL, __manualDraw_fs);
  shader = LoadShaderFromMemory(NULL, __gol_fs);
  if (!IsShaderValid(ManualDrawShader))
    return 0x10;
  int gridSize[2] = {GRID_WIDTH, GRID_HEIGHT};
  int size_loc = GetShaderLocation(shader, "gridSize");
  SetShaderValue(shader, size_loc, &gridSize, SHADER_UNIFORM_IVEC2);
  size_loc = GetShaderLocation(ManualDrawShader, "gridSize");
  SetShaderValue(ManualDrawShader, size_loc, &gridSize, SHADER_UNIFORM_IVEC2);
#endif
  invertionTargetLoc = GetShaderLocation(ManualDrawShader, "invertionTarget");
// liveCounterLocation = GetShaderLocation(shader, "liveCounter");
// SetShaderValue(shader,liveCounterLocation,&liveCounter,SHADER_UNIFORM_INT);
#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(main_loop, 0, 1);
#else
  while (!WindowShouldClose()) {
    main_loop();
  }
#endif
}
void ProcessInputs() {
  if (IsKeyPressed(KEY_R)) {
    resetGrid(state[selectedTexture]);
    generateRandomGrid(state[selectedTexture]);
  }
  if (IsKeyPressed(KEY_C)) { // Clear grid
    resetGrid(state[selectedTexture]);
  }
  if (IsKeyPressed(KEY_SPACE)) {
    Paused = !Paused;   // toggle pause state
    updateTimer = 0.0f; // reset the timer when paused
  }
  // Toggle Stats draw
  if (IsKeyPressed(KEY_TAB))
    isDrawStats = !isDrawStats;
  if (IsKeyPressed(KEY_X)) {
    isDrawGrid = !isDrawGrid;
  }
  // Increase and decrease target generation per second
  if (IsKeyPressed(KEY_UP))
    targetGenerationsPerSecond++;
  else if (IsKeyPressed(KEY_DOWN))
    targetGenerationsPerSecond--;

  // camera Movement controls
  // Horizontal movement
  cameraZoomRelatedVelocity = cameraVelocity / camera.zoom;
  if (IsKeyDown(KEY_A))
    camera.target.x -= cameraZoomRelatedVelocity;
  if (IsKeyDown(KEY_D))
    camera.target.x += cameraZoomRelatedVelocity;
  if (camera.target.x > GRID_WIDTH)
    camera.target.x = GRID_WIDTH;
  if (camera.target.x < 0)
    camera.target.x = 0;
  // Vertical movement
  if (IsKeyDown(KEY_W))
    camera.target.y -= cameraZoomRelatedVelocity;
  if (IsKeyDown(KEY_S))
    camera.target.y += cameraZoomRelatedVelocity;
  // Check camera bounds
  if (camera.target.y > GRID_HEIGHT)
    camera.target.y = GRID_HEIGHT;
  if (camera.target.y < 0)
    camera.target.y = 0;
  // Camera zoom controls
  // Uses log scaling to provide consistent zoom speed
  camera.zoom = expf(logf(camera.zoom) + ((float)GetMouseWheelMove() * 0.1f));
  if (camera.zoom > MAX_ZOOM)
    camera.zoom = MAX_ZOOM;
  else if (camera.zoom < 0.5f)
    camera.zoom = 0.5f;
  // Manually draw
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    Vector2 mousePos = GetMousePosition();
    Vector2 worldPos = GetScreenToWorld2D(mousePos, camera);
    int cellX = (int)(worldPos.x);
    int cellY = (int)(worldPos.y);
    if (cellX >= 0 && cellX < GRID_WIDTH && cellY >= 0 && cellY < GRID_HEIGHT) {
      if ((cellX != drawLocation[0] || cellY != drawLocation[1])) {
        drawLocation[0] = cellX;
        drawLocation[1] = cellY;
        isDrawCommand = true;
        // printf("Drawing! on: %d, %d\n",cellX,cellY);
        gridLifeTime = 0;
        current_generation = 0;
      }
      Paused = true;
    }
  }
  // Fps settings
  if (IsKeyPressed(KEY_F)) {
    current_fps = (current_fps + 1) % FPS_VALUES_COUNT;
    SetTargetFPS(FPS_VALUES[current_fps]);
  }
}

void main_loop() {
  ProcessInputs();
  if (!Paused) {
    float deltaTime = GetFrameTime();
    updateTimer += deltaTime * targetGenerationsPerSecond;
    gridLifeTime += deltaTime;
  } else {
    updateTimer = 0;
    if (isDrawCommand) {
#ifdef __EMSCRIPTEN__
      Vector2 drawLoc = {drawLocation[0], drawLocation[1]};
      SetShaderValue(ManualDrawShader, invertionTargetLoc, &drawLoc,
                     SHADER_UNIFORM_VEC2);
#else
      SetShaderValue(ManualDrawShader, invertionTargetLoc, drawLocation,
                     SHADER_UNIFORM_IVEC2);
#endif
      BeginTextureMode(state[1 - selectedTexture]);
      ClearBackground(BLACK);
      BeginShaderMode(ManualDrawShader);
      DrawTexture(state[selectedTexture].texture, 0, 0, WHITE);
      EndShaderMode();
      EndTextureMode();
      selectedTexture = 1 - selectedTexture;
      isDrawCommand = false;
    }
  }
  // Next Generation
  if (updateTimer >= 1.0f) {
    updateTimer -= 1.0;
    BeginTextureMode(state[1 - selectedTexture]);
    ClearBackground(BLACK);
    BeginShaderMode(shader);
    DrawTexture(state[selectedTexture].texture, 0, 0, WHITE);
    EndShaderMode();
    EndTextureMode();
    selectedTexture = 1 - selectedTexture;
    current_generation++;
  }

  BeginDrawing();
  ClearBackground(BLACK);
  BeginMode2D(camera);
  DrawTextureEx(state[selectedTexture].texture, (Vector2){0, 0}, 0, 1, WHITE);
  DrawRectangleLines(0, 0, GRID_WIDTH, GRID_HEIGHT, RED);
  if (isDrawGrid)
    DrawGridLInes();
  EndMode2D();
  if (isDrawStats) {
    DrawRectangle(10, 10, 500, 155, Fade(SKYBLUE, 0.5f));
    DrawRectangleLines(10, 10, 500, 155, BLUE);
    float fontSize = 20;
    DrawText("Game controls:", 20, 20, 15, BLACK);
    DrawText("- WASD to move Camera", 40, 40, fontSize, CELL_COLOR);
    DrawText("- Mouse Wheel to Zoom in-out", 40, 60, fontSize, CELL_COLOR);
    DrawText("- R to Randomize the grid", 40, 80, fontSize, CELL_COLOR);
    DrawText("- C to Clear the grid", 40, 100, fontSize, CELL_COLOR);
    DrawText("- [SPACE] to PAUSE/RESUME", 40, 120, fontSize, CELL_COLOR);
    DrawText(TextFormat("- DOWN/UP Adjust update speed: %d gen/s",
                        targetGenerationsPerSecond),
             40, 140, fontSize, CELL_COLOR);
    // Stats
    DrawRectangle(10, 200, 350, 100, Fade(LIME, 0.5f));
    DrawRectangleLines(10, 200, 350, 130, GREEN);
    DrawText(TextFormat("Generation: %d", current_generation), 40, 230, 20,
             CELL_COLOR);
    // DrawText(TextFormat("Population: %d", liveCounter), 40, 250, 20,
    // CELL_COLOR);
    DrawText(TextFormat("Generations per second: %.2f ",
                        current_generation / (gridLifeTime + 1)),
             40, 270, 20, CELL_COLOR);
    DrawText(TextFormat("Target FPS: %d ", FPS_VALUES[current_fps]), 40, 300,
             20, CELL_COLOR);
    DrawFPS(WIDTH - 300, 10);

    DrawText("[TAB] TO HIDE", 50, 330, 25, Fade(YELLOW, 0.7f));
    if (Paused) {

      DrawText("PAUSED", (WIDTH - MeasureText("PAUSED", 20)) / 2, HEIGHT - 23,
               20, Fade(RED, 0.7));
    }
  }
  EndDrawing();
}

void DrawGridLInes() {
  Vector2 start = {0, 0};
  Vector2 end = {0, GRID_HEIGHT};
  for (int j = 1; j < GRID_WIDTH; j++) {
    start.x = j - GridLinesOffset;
    end.x = j - GridLinesOffset;
    DrawLineEx(start, end, GridLinesThickness, GridLinesColor);
    // DrawLineV(start,end,GridLinesColor);
  }
  start.x = 0;
  end.x = GRID_WIDTH;
  for (int j = 1; j < GRID_HEIGHT; j++) {
    start.y = j - GridLinesOffset;
    end.y = j - GridLinesOffset;
    DrawLineEx(start, end, GridLinesThickness, GridLinesColor);
    // DrawLineV(start,end,GridLinesColor);
  }
}

void resetGrid(RenderTexture2D tex) {
  // liveCounter=0;
  gridLifeTime = 0;
  current_generation = 0;
  BeginTextureMode(tex);
  ClearBackground(BLACK);
  EndTextureMode();
}
void generateRandomGrid(RenderTexture2D tex) {
  // liveCounter=0;
  int factor = 2;
  int startX = (GRID_WIDTH / factor) - (GRID_WIDTH / (factor * 2));
  int starty = (GRID_HEIGHT / factor) - (GRID_HEIGHT / (factor * 2));
  Image image = GenImageColor(GRID_WIDTH, GRID_HEIGHT, BLACK);
  for (int y = starty; y < starty + GRID_HEIGHT / factor; ++y) {
    for (int x = startX; x < startX + GRID_HEIGHT / factor; ++x) {
      int v = GetRandomValue(0, 1);
      if (v) {
        v = 255;
        // liveCounter++;
      }
      Color color = {v, v, v, 255};
      ImageDrawPixel(&image, x, y, color);
    }
  }
  UpdateTexture(tex.texture, image.data);
}
