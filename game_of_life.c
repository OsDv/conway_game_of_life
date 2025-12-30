#include "externals/cglm/cglm.h"
#include "externals/glad.h"
#include "externals/glfw3.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> // to seed random

#define BUFFER_SIZE 1024

volatile int calculation_in_progress =
    0;                       // flag for next generation calculation thread
pthread_t generation_thread; // used to calculate next generation in parallel
int reset_request = 0; // boolean to reset the grid with new random distribution

// Window dimentions
#define HEIGHT 900
#define WIDTH 1600
// input functions
void processInput(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
//
void myInit();
// Colors
#define CELLS_COLOR 1.0f, 1.0f, 1.0f, 1.0f
#define BACKGROUND_COLOR 0.0f, 0.0f, 0.0f, 0.0f

float deltaTime = 0.0f;   // Delta time ;)
float updateRation = 0.1; // seconds for one generation
int generationRatio = 10;
int generationRatioUpdate = 0;
// Game of life grid dimentions
#define GRID_HEIGHT 1000
#define GRID_WIDTH 1000
// World borders color rgbw
#define BORDER_COLOR 1.f, 0.0f, 0.0f, 1.0f
// two grids to contains cells stats in the current and next generation
int grid[GRID_HEIGHT][GRID_WIDTH] = {0};
int grid2[GRID_HEIGHT][GRID_WIDTH] = {0};
// live cells locations, buffer's data for redering
float live[GRID_HEIGHT * GRID_WIDTH][2];

unsigned int liveCounter = 0; // actual population
unsigned int liveCounterNew =
    0; // population in the next generation calculated by the thread
unsigned int offsetsVBO, VAO; // buffers id put here for easy access

// Camera Set-up
vec3 up = {0.0f, 1.0f, 0.0f};
vec3 right = {1.0f, 0.0f, 0.0f};
vec3 front = {0.0f, 0.0f, -1.0f};
const float cameraVelocity = 0.001 * GRID_WIDTH;
float orthogonalZoom = 0.3f;
vec3 cameraPosition = {(float)GRID_WIDTH / 2, (float)GRID_HEIGHT / 2, 1.0f};
vec3 cameraCenter = {0.0f, 0.0f, 0.0f};
vec3 cameraUpSpeed;
vec3 cameraRightSpeed;
/*
 * @activeGrid : actual generation grid
 * @swapGrid : the next generation grid
 */
int (*activeGrid)[GRID_HEIGHT][GRID_WIDTH] = &grid;
int (*swapGrid)[GRID_HEIGHT][GRID_WIDTH] = &grid2;

// initialze the grid with random stat
void generateRandomGrid(int grid[GRID_HEIGHT][GRID_WIDTH]);
// diplay the grid in terminal
void displayGrid(int grid[GRID_HEIGHT][GRID_WIDTH]);
// calculate the next generation and put the value in @swapGrid and new
// population in @liveCountNew (old without multi-thread)
void nextGeneration(int grid[GRID_HEIGHT][GRID_WIDTH]);
// Reset the @activeGrid to all dead cells
void resetGrid(int grid[GRID_HEIGHT][GRID_WIDTH]);
// ender the grid, uses render command for each cell (old)
void rederGrid(int grid[GRID_HEIGHT][GRID_WIDTH], unsigned int modelLoc);
// render the live cells with single draw command using instanced attribute
void optimizedGridRendrer(int grid[GRID_HEIGHT][GRID_WIDTH],
                          unsigned int modelLoc);
// Thread function to calculate the new generation
void *work_generate_next_generation(void *args);
// load Shader program
unsigned int loadShader(const char *vertexPath, const char *fragmentPath);
// calcualte the euclidien mod
int emod(int a, int b);

int main(void) {
  myInit();
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(WIDTH, HEIGHT, "Game Of Life", NULL, NULL);
  if (window == NULL) {
    printf("Failed to create GLFW window\n");
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("Failed to initialize GLAD\n");
    return -1;
  }

  glViewport(0, 0, WIDTH, HEIGHT);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetScrollCallback(window, scroll_callback);

  // load Shader
  unsigned int shader =
      loadShader("../game_of_life.vs\0", "../game_of_life.fs\0");
  glUseProgram(shader);
  // set shader matrices
  unsigned int projectionLoc, viewLoc, modelLoc;
  projectionLoc = glGetUniformLocation(shader, "projection");
  viewLoc = glGetUniformLocation(shader, "view");
  modelLoc = glGetUniformLocation(shader, "model");
  mat4 projection;

  // properties for orthogonal projection matrix
  float orthogonalX = WIDTH * orthogonalZoom;
  float orthogonalY = HEIGHT * orthogonalZoom;
  glm_ortho(-orthogonalX, orthogonalX, -orthogonalY, orthogonalY, 0.1f, 1000.0f,
            projection);
  glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, *projection);
  // Set the view matrix
  mat4 view;
  glm_vec3_add(cameraPosition, front, cameraCenter);
  glm_lookat(cameraPosition, cameraCenter, up, view);
  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, *view);

  // World borders vertices and Indexes
  float borderVertices[] = {
      0.0f,
      0.0f,
      0.0f,
      1.f,
      0.0f,
      0.0f,
      1.0f,
      -1.0f,
      -1.0f,
      0.0f,
      1.f,
      0.0f,
      0.0f,
      1.0f,
      0.0f,
      GRID_HEIGHT,
      0.0f,
      1.f,
      0.0f,
      0.0f,
      1.0f,
      -1.0f,
      GRID_HEIGHT + 1,
      0.0f,
      BORDER_COLOR,
      GRID_WIDTH,
      0.0f,
      0.0f,
      BORDER_COLOR,
      GRID_WIDTH + 1,
      -1.0f,
      0.0f,
      BORDER_COLOR,
      GRID_WIDTH,
      GRID_HEIGHT,
      0.0f,
      BORDER_COLOR,
      GRID_WIDTH + 1,
      GRID_HEIGHT + 1,
      0.0f,
      BORDER_COLOR,
  };
  unsigned int borderIndices[] = {
      0, 1, 2, 1, 2, 3, 2, 3, 7, 2, 6, 7, 0, 1, 5, 0, 4, 5, 4, 5, 6, 5, 6, 7,
  };
  // Set the Vertex Array Object to draw the border
  unsigned int borderVAO, borderVBO, borderEBO;
  glGenVertexArrays(1, &borderVAO);
  glGenBuffers(1, &borderVBO);
  glGenBuffers(1, &borderEBO);
  glBindVertexArray(borderVAO);
  glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(borderVertices), borderVertices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, borderEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(borderIndices), borderIndices,
               GL_STATIC_DRAW);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Cell vertices and Indexes
  float vertices[] = {
      0.0f, 0.0f, 0.0f, CELLS_COLOR, 0.0f, 1.0f, 0.0f, CELLS_COLOR,
      1.0f, 0.0f, 0.0f, CELLS_COLOR, 1.0f, 1.0f, 0.0f, CELLS_COLOR,
  };
  unsigned int indices[] = {0, 1, 2, 1, 2, 3};

  // Set vertex array to draw live cells
  unsigned int VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);
  glGenBuffers(1, &offsetsVBO);
  glBindBuffer(GL_ARRAY_BUFFER, offsetsVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GL_FLOAT) * 2 * GRID_HEIGHT * GRID_WIDTH,
               NULL, GL_DYNAMIC_DRAW);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GL_FLOAT),
                        (void *)0);
  glVertexAttribDivisor(2, 1);
  glEnableVertexAttribArray(2);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // to count delta time
  float lastFrame = 0.0f;

  // timers to print in console and update the grid
  float consolePrintTimer = 0.0f;
  float updateTimer = 0.0f;
  float inputTimer = 0.0f;

  // start with random grid
  generateRandomGrid(*activeGrid);
  calculation_in_progress = 1;
  pthread_create(&generation_thread, NULL, work_generate_next_generation, NULL);
  int fps = 0;
  while (!glfwWindowShouldClose(window)) {
    // Input processing
    processInput(window);
    glClearColor(BACKGROUND_COLOR);
    glm_vec3_add(cameraPosition, front, cameraCenter);
    glm_lookat(cameraPosition, cameraCenter, up, view);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, *view);

    orthogonalX = orthogonalZoom * WIDTH;
    orthogonalY = orthogonalZoom * HEIGHT;
    glm_ortho(-orthogonalX, orthogonalX, -orthogonalY, orthogonalY, 0.1f,
              1000.0f, projection);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, *projection);

    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    consolePrintTimer += deltaTime;
    updateTimer += deltaTime;
    inputTimer += deltaTime;
    fps++;

    if (inputTimer >= 0.4f) {
      if (generationRatioUpdate) {
        generationRatio += generationRatioUpdate;
        if (generationRatio == 0)
          generationRatio = 1;
        updateRation = 1 / (float)generationRatio;
        generationRatioUpdate = 0;
      }

      inputTimer = 0.0f;
    }
    if (consolePrintTimer >= 1.0f) {

      consolePrintTimer = 0.0f;
      printf("fps: %d, Generation: %d generation/second\n", fps,
             generationRatio);
      fps = 0;
    }
    /*if (updateTimer >= 0.1f) // old single thread
    {
        updateTimer = 0.0f;
        float startTime = glfwGetTime();
        nextGeneration((*activeGrid));
        float endTime = glfwGetTime();
        std::cout << "Render time: " << endTime - startTime << "s life: "<<
    liveCounter << std::endl;
    }*/
    if (updateTimer >= updateRation && calculation_in_progress == 0) {
      updateTimer = 0.0f;
      int (*tem)[GRID_HEIGHT][GRID_WIDTH] = swapGrid;
      swapGrid = activeGrid;
      activeGrid = tem;
      liveCounter = liveCounterNew;
      if (reset_request) {
        resetGrid(*activeGrid);
        generateRandomGrid(*activeGrid);
        reset_request = 0;
      }
      // Start a new calculation
      calculation_in_progress = 1;
      if (pthread_create(&generation_thread, NULL,
                         work_generate_next_generation, NULL) != 0) {
        printf("Error creating thread\n");
        calculation_in_progress = 0;
      } else {
        // Detach thread so its resources are automatically released when it
        // terminates
        pthread_detach(generation_thread);
      }
    }

    mat4 borderModel;
    glm_mat4_identity(borderModel);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, *borderModel);
    glBindVertexArray(borderVAO);
    glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);

    // rederGrid((*activeGrid),modelLoc); // old render function (slower)
    optimizedGridRendrer((*activeGrid), modelLoc);

    // Swapping buffers and polling events
    glfwSwapBuffers(window);
    glfwPollEvents();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  /*grid[50][50]=1;
  grid[50][51]=1;
  grid[50][49]=1;
  grid[49][49]=1;
  grid[48][49]=1;
  grid[49][51]=1;
  grid[48][51]=1;*/

  return 0x10;
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
  if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    reset_request = 1;

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    // cameraPosition += up*cameraVelocity;
    glm_vec3_add(cameraPosition, cameraUpSpeed, cameraPosition);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    // cameraPosition -= up*cameraVelocity;
    glm_vec3_sub(cameraPosition, cameraUpSpeed, cameraPosition);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    // cameraPosition -= right*cameraVelocity;
    glm_vec3_sub(cameraPosition, cameraRightSpeed, cameraPosition);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    // cameraPosition += right*cameraVelocity;
    glm_vec3_add(cameraPosition, cameraRightSpeed, cameraPosition);

  // update generation Ratio
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    generationRatioUpdate = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    generationRatioUpdate = -1;
  }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {

  const float minZoom = 0.0001f;
  const float zoomSpeed = 0.01;
  // camera.ProcessMouseScroll(yoffset);
  orthogonalZoom -= (float)yoffset * zoomSpeed;
  if (orthogonalZoom > 1.0f)
    orthogonalZoom = 1.0f;
  if (orthogonalZoom < minZoom)
    orthogonalZoom = minZoom;
}

void generateRandomGrid(int grid[GRID_HEIGHT][GRID_WIDTH]) {
  int factor = 2;
  int startX = (GRID_WIDTH / factor) - (GRID_WIDTH / (factor * 2));
  int starty = (GRID_HEIGHT / factor) - (GRID_HEIGHT / (factor * 2));

  for (size_t i = starty; i < starty + GRID_HEIGHT / factor; i++) {
    for (size_t j = startX; j < startX + GRID_WIDTH / factor; j++) {
      int k = rand() % 2;
      grid[i][j] = k;
      if (k)
        liveCounter++;
    }
  }
}
void displayGrid(int grid[GRID_HEIGHT][GRID_WIDTH]) {
  for (size_t i = 0; i < GRID_HEIGHT; i++) {
    for (size_t j = 0; j < GRID_WIDTH; j++) {
      // std::cout << grid[i][j] << " ";
      if (grid[i][j])
        fputc('#', stdout);
      else
        fputc(' ', stdout);
    }
    fputc('\n', stdout);
  }
}

void nextGeneration(int grid[GRID_HEIGHT][GRID_WIDTH]) {
  for (int i = 0; i < GRID_HEIGHT; i++) {
    for (int j = 0; j < GRID_WIDTH; j++) {
      int liveNeighbours = 0;
      for (int k = i - 1; k <= i + 1; k++) {
        for (int l = j - 1; l <= j + 1; l++) {
          int ck = emod(k, GRID_HEIGHT);
          int cl = emod(l, GRID_WIDTH);
          if (k == i && l == j)
            continue;
          if (grid[ck][cl] == 1)
            liveNeighbours++;
        }
      }
      if (grid[i][j] && (liveNeighbours < 2 || liveNeighbours > 3)) {
        (*swapGrid)[i][j] = 0;
        liveCounter--;
      } else if (!grid[i][j] && liveNeighbours == 3) {
        (*swapGrid)[i][j] = 1;
        liveCounter++;
      } else
        (*swapGrid)[i][j] = grid[i][j];
    }
  }
  int (*tem)[GRID_HEIGHT][GRID_WIDTH] = swapGrid;
  swapGrid = activeGrid;
  activeGrid = tem;
}
void resetGrid(int grid[GRID_HEIGHT][GRID_WIDTH]) {
  liveCounter = 0;
  for (size_t i = 0; i < GRID_HEIGHT; i++) {
    for (size_t j = 0; j < GRID_WIDTH; j++) {
      grid[i][j] = 0;
    }
  }
}
int emod(int a, int b) { return ((a % b) + b) % b; }
void rederGrid(int grid[GRID_HEIGHT][GRID_WIDTH], unsigned int modelLoc) {
  for (int i = 0; i < GRID_HEIGHT; i++) {
    for (int j = 0; j < GRID_WIDTH; j++) {
      if (grid[i][j]) {
        mat4 model;
        glm_mat4_identity(model);
        vec3 position = {(float)j, (float)GRID_HEIGHT - 1 - i, 0.0f};
        glm_translate(model, position);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, *model);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
      }
    }
  }
}
void optimizedGridRendrer(int grid[GRID_HEIGHT][GRID_WIDTH],
                          unsigned int modelLoc) {
  int stop = 0;
  int counter = 0;
  for (int i = 0; i < GRID_HEIGHT && !stop; i++) {
    for (int j = 0; j < GRID_WIDTH && !stop; j++) {
      if (grid[i][j]) {
        live[counter][0] = j;
        live[counter][1] = GRID_HEIGHT - 1 - i;
        counter++;
        if (counter == liveCounter)
          stop = 1;
      }
    }
  }
  if (counter != 0) {
    glBindBuffer(GL_ARRAY_BUFFER, offsetsVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, counter * 2 * sizeof(float), live);
    mat4 model;
    glm_mat4_identity(model);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, *model);
    glBindVertexArray(VAO);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, counter);
  }
}

void *work_generate_next_generation(void *args) {
  liveCounterNew = liveCounter;
  for (int i = 0; i < GRID_HEIGHT; i++) {
    for (int j = 0; j < GRID_WIDTH; j++) {
      int liveNeighbours = 0;
      for (int k = i - 1; k <= i + 1; k++) {
        for (int l = j - 1; l <= j + 1; l++) {
          int ck = emod(k, GRID_HEIGHT);
          int cl = emod(l, GRID_WIDTH);
          if (k == i && l == j)
            continue;
          if ((*activeGrid)[ck][cl] == 1)
            liveNeighbours++;
        }
      }
      if ((*activeGrid)[i][j] && (liveNeighbours < 2 || liveNeighbours > 3)) {
        (*swapGrid)[i][j] = 0;
        liveCounterNew--;
      } else if (!(*activeGrid)[i][j] && liveNeighbours == 3) {
        (*swapGrid)[i][j] = 1;
        liveCounterNew++;
      } else
        (*swapGrid)[i][j] = (*activeGrid)[i][j];
    }
  }
  calculation_in_progress = 0;
  return NULL;
}

unsigned int loadShader(const char *vertexPath, const char *fragmentPath) {
  // Read shaders codes from files
  FILE *vertexFile = fopen(vertexPath, "r");
  FILE *fragmentFile = fopen(fragmentPath, "r");
  if (!vertexFile || !fragmentFile) {
    fprintf(stderr, "Error opening vertex file or fragment\n");
    exit(EXIT_FAILURE);
  }
  char *vShaderCode = (char *)malloc(BUFFER_SIZE * sizeof(char));
  const GLchar *vShaderCodePtr = vShaderCode;
  int count = fread(vShaderCode, 1, BUFFER_SIZE, vertexFile);
  vShaderCode[count] = '\0';
  char *fShaderCode = (char *)malloc(sizeof(char) * BUFFER_SIZE * sizeof(char));
  const GLchar *fShaderCodePtr = fShaderCode;
  count = fread(fShaderCode, 1, BUFFER_SIZE, fragmentFile);
  fShaderCode[count] = '\0';
  // compile shaders
  unsigned int vertex, fragment;
  // vertex shader
  vertex = glCreateShader(GL_VERTEX_SHADER);

  glShaderSource(vertex, 1, &vShaderCodePtr, NULL);
  glCompileShader(vertex);
  // fragment Shader
  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fShaderCodePtr, NULL);
  glCompileShader(fragment);

  // shader Program
  unsigned int ID = glCreateProgram();
  glAttachShader(ID, vertex);
  glAttachShader(ID, fragment);
  glLinkProgram(ID);
  // delete the shaders as they're linked into our program now and no longer
  // necessary
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  free(vShaderCode);
  free(fShaderCode);
  return ID;
}

void myInit() {
  srand(time(NULL));
  glm_vec3_scale(up, cameraVelocity, cameraUpSpeed);
  glm_vec3_scale(right, cameraVelocity, cameraRightSpeed);
}
