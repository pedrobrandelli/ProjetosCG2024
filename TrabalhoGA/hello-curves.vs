#version 430 
layout (location = 0) in vec3 positionC;
 
void main()
{
    gl_Position = vec4(positionC, 1.0f);
    
}
