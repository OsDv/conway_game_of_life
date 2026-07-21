#version 300 es
precision mediump float;

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec2 gridSize;

out vec4 FragColor;

float transition(float state, float neighbors) {
    // Dead: 3 neighbors -> 1
    // Alive: 2 or 3 neighbors -> 1
    if (state < 0.5) {
        if (neighbors == 3.0) return 1.0;
        else return 0.0;
    } else {
        if (neighbors == 2.0 || neighbors == 3.0) return 1.0;
        else return 0.0;
    }
}

float grid(float x, float y) {
    float tx = x / gridSize.x;
    float ty = 1.0 - y / gridSize.y;
    vec4 t = texture(texture0, vec2(tx, ty));
    return t.x > 0.5 ? 1.0 : 0.0;
}

void main() {
    float cx = fragTexCoord.x * gridSize.x;
    float cy = fragTexCoord.y * gridSize.y;
    float liveNeighbours = 0.0;
    for (float i = -1.0; i <= 1.0; i += 1.0) {
        for (float j = -1.0; j <= 1.0; j += 1.0) {
            if (i == 0.0 && j == 0.0) continue;
            liveNeighbours += grid(cx + i, cy + j);
        }
    }
    vec4 t = texture(texture0, vec2(fragTexCoord.x, 1.0 - fragTexCoord.y));
    float state = t.x > 0.5 ? 1.0 : 0.0;
    float next = transition(state, liveNeighbours);
    FragColor = vec4(next, next, next, 1.0);
}