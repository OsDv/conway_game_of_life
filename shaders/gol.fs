#version 420

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
// Constant transition matrix for game of life
int transition[18] = {0,0,0,1,0,0,0,0,0,
						 0,0,1,1,0,0,0,0,0};
// Input uniform values
uniform sampler2D texture0;
uniform int isGradient = 0;
// Output fragment color
out vec4 finalColor;
//uniform int liveCounter;
uniform ivec2 gridSize;
// uniform for cells color
uniform vec4 cell_color;
bool grid(float x, float y)
{
	float tx = x/gridSize.x;
    float ty =1.0 - y/gridSize.y;
    vec4 t = texture(texture0, vec2(tx, ty));
	return (t.x == 1.0);
}
void main()
{
	float cx = fragTexCoord.x*gridSize.x;
	float cy = fragTexCoord.y*gridSize.y;
	int liveNeighbours = 0;
	for (float i = cx-1.0; i <= cx+1.0; i+=1.0)
	{
		for (float j = cy-1.0; j <= cy+1.0; j+=1.0)
		{
			if (i==cx && j==cy) continue;
			if (grid(i,j)) liveNeighbours++;
		}
	}
	vec4 t = texture(texture0, vec2(fragTexCoord.x,1-fragTexCoord.y));
	int r = 0;
	if (t.x==1.0) {
		r=1;
	//	liveCounter--;
	}
  if (transition[r*9+liveNeighbours]==1)
  {
  	finalColor = vec4(1.0);
  } else {
    vec4 t = texture(texture0, vec2(fragTexCoord.x,1-fragTexCoord.y));
    if(t.x>0)t -= vec4(0.1);
    if (t.x<0) t=vec4(0.0);
    finalColor = t*isGradient;
  }
	//liveCounter += transition[r*9+liveNeighbours];
}
