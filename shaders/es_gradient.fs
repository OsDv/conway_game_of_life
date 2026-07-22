#version 300 es
precision mediump float;
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
// Output fragment color
out vec4 finalColor;
uniform vec4 color1; // dead color
uniform vec4 color2; // live color
void main()
{
vec4 t = texture(texture0, vec2(fragTexCoord.x,1.0-fragTexCoord.y));
finalColor=mix(color1, color2, t.x);
}