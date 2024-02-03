#version 330 core

// Atrybuty wierzcholkow z VAO
layout( location = 0 ) in vec4 inPosition;
layout( location = 1 ) in vec2 inUV;


// Macierze rzutowania i transformacji
uniform mat4 matPVM;

// Dane przesylane do kolejnego etapu
out vec4 Position;
out vec2 UV;


// ------------------------------------------------------------
void main()
{

    Position = inPosition;
    UV = inUV;
    gl_Position = matPVM * inPosition;
}
