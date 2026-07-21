#version 300 es
precision mediump float;

in vec2 fragTexCoord;

uniform sampler2D texture0;
uniform vec2 invertionTarget; // Pass as float (x, y)
uniform vec2 gridSize;

out vec4 FragColor;

void main() {
    vec2 fragPixel = vec2(floor(fragTexCoord.x * gridSize.x), floor(fragTexCoord.y * gridSize.y));
    fragPixel.y = gridSize.y - fragPixel.y - 1.0;

    vec4 t = texture(texture0, vec2(fragTexCoord.x, 1.0 - fragTexCoord.y));

    // Compare with invertionTarget (use a small epsilon for float comparison)
    float isTarget = step(0.0, 0.5 - abs(fragPixel.x - invertionTarget.x)) * step(0.0, 0.5 - abs(fragPixel.y - invertionTarget.y));

    vec3 inverted = vec3(1.0) - t.xyz;
    vec3 color = mix(t.xyz, inverted, isTarget);

    FragColor = vec4(color, 1.0);
}