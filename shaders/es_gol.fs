#version 300 es
precision mediump float;

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec2 gridSize;
uniform int isGradient;
out vec4 FragColor;

int transition(int state, int neighbors) {
    // Dead: 3 neighbors -> 1
    // Alive: 2 or 3 neighbors -> 1
    if (state == 0) {
        if (neighbors == 3) return 1;
        else return 0;
    } else {
        if (neighbors == 2 || neighbors == 3) return 1;
        else return 0;
    }
}

int grid(float x, float y) {
    float tx = x / gridSize.x;
    float ty = 1.0 - y / gridSize.y;
    vec4 t = texture(texture0, vec2(tx, ty));
    return t.x == 1.0 ? 1 : 0;
}

void main() {
    float cx = fragTexCoord.x * gridSize.x;
    float cy = fragTexCoord.y * gridSize.y;
    int liveNeighbours = 0;
    for (int i = -1; i <= 1; i += 1) {
        for (int j = -1; j <= 1; j += 1) {
            if (i == 0 && j == 0) continue;
            liveNeighbours += grid(cx + float(i), cy + float(j));
        }
    }
    vec4 t = texture(texture0, vec2(fragTexCoord.x, 1.0 - fragTexCoord.y));
    int state = t.x == 1.0 ? 1:0;
    int next = transition(state, liveNeighbours);
    if (next ==1)
    {
      FragColor = vec4(1.0);
    } else {
      if (t.x>0.0) t -= vec4(0.1,0.1,0.1,0.0);
      if (t.x<=0.0) t = vec4(0.0);
      if (isGradient==1) FragColor = t;
      else FragColor = vec4(0.0);
    }
}
