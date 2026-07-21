#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform ivec2 invertionTarget;
uniform ivec2 gridSize;

void main()
{
    // Compute the current pixel coordinates
    ivec2 fragPixel = ivec2(fragTexCoord.x*gridSize.x,(fragTexCoord.y*gridSize.y));
	fragPixel.y = gridSize.y - fragPixel.y -1;

    // Sample the texture without flipping Y
    vec4 t = texture(texture0, vec2(fragTexCoord.x,1-fragTexCoord.y));

    // Invert color only at the target pixel
    if (fragPixel.x==invertionTarget.x && fragPixel.y==invertionTarget.y) {
        finalColor = vec4(vec3(1.0)-t.xyz,1.0);
    } else {
        finalColor = t;
    }
}