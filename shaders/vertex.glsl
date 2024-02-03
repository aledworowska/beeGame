#version 330 core

// Atrybuty wierzcholkow z VAO
layout( location = 0 ) in vec4 inPosition;
layout( location = 1 ) in vec2 inUV;
layout( location = 2 ) in vec3 inNormal;
layout( location = 3 ) in mat4 matModelInst;

// Macierze rzutowania i transformacji
uniform mat4 matProj;
uniform mat4 matView;
uniform mat4 matModel;
uniform vec3 cameraPos;
uniform mat3 matNormal;
uniform int instanced;
uniform int beesFlag;
uniform float Time;
uniform float Time2;
uniform int Direction;
uniform mat4 lightProj;
uniform mat4 lightView;

// Dane przesylane do kolejnego etapu
out vec4 Position;
out vec2 UV;
out vec3 Normal;
out vec4 fragPosLight;

// ------------------------------------------------------------
void main()
{
    vec4 newPosition = inPosition;
    fragPosLight = lightProj * lightView * matModel * inPosition;

    mat4 MatrixY;
    MatrixY[0] = vec4(cos(3.14), 0 , -sin(3.14), 0);
    MatrixY[1] = vec4(0,1,0,0);
    MatrixY[2] = vec4(sin(3.14), 0,cos(3.14), 0);
    MatrixY[3] = vec4(0, 0, 0, 1);

    if(instanced==0)
        Position = matModel * inPosition;
    else
        Position = matModelInst * inPosition;

    UV = inUV;

    if(instanced==0)
    {
        Normal = matNormal * inNormal;
    }
    else if(beesFlag == 1)
    {
        if(Direction == -1){
            newPosition = MatrixY * newPosition;
        }
        newPosition.x += sin(Time)/8;
        newPosition.y += cos(Time)/16;
        newPosition.z += Time/2;


        mat3 matNormal = mat3(transpose(inverse(matModelInst)));
        Normal = matNormal * inNormal;
    }
    else
    {
        float newY = (inPosition.y + 1.0)/2.0;
        newPosition.x += sin(Time2/50)*newY;
        newPosition.z += cos(Time2/50)*newY;

        mat3 matNormal = mat3(transpose(inverse(matModelInst)));
        Normal = matNormal * inNormal;
    }

    // Ostateczna pozycja wierzcholka
    if(instanced==0)
        gl_Position = matProj * matView * matModel * inPosition;
    else
        gl_Position = matProj * matView * matModelInst * newPosition;
}
