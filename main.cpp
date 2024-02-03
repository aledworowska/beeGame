// -------------------------------------------------
// Programowanie grafiki 3D w OpenGL / UG
// -------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <string.h>
#include <ctime>
#include <vector>
#include <array>
// Biblioteki GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ground.hpp"
#include "text_freetype/text-ft.hpp"
CGround myGround;

// Lokalne pliki naglowkowe
#include "utilities.hpp"
#include "obj_loader.hpp"
#include "shadow_dir.hpp"
#include "collider.hpp"

# define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using glm::vec3;
using namespace std;

#define PI	3.1415926535

float cameraDistanceY,cameraDistanceX=-7;
bool KeyboardTable[128] = { false };
// Macierze transformacji i rzutowania
glm::mat4 matProj = glm::mat4(1.0);
glm::mat4 matView = glm::mat4(1.0);
glm::mat4 matModelRocks, matModelGrass,matModelPalms,matModelFlowers;

GLuint idVAO;
GLuint idVBO_coord;
GLuint idVBO_color;
GLuint idVBO_uv;
GLuint idVBO_normal;

glm::mat4x4 matModel = glm::mat4(1.0);

int WindowWidth = 900;
int WindowHeight = 600;


float Time, lightTime, BeeTime;
int DirectionTime = 0;
int BeeDirection = 1;
int frameCount  = 0;
int fps,points = 0;

unsigned int lightModel = 1;
unsigned int lightSwitch = 1;
unsigned int lightAnimation = 0;
unsigned int colorSphere = 2;
unsigned int lightType = 1;
unsigned int currentSkyBox = 1;
unsigned int withReflection = 0;
unsigned int withRefraction = 0;
unsigned int beesFlag = 0;
unsigned int RockReflection = 0;
unsigned int RockRefraction = 0;
unsigned int showMiniMap = 1;
unsigned int postprocessing = 0;

const int InstanceNumber = 150;
const int rocksNumber = 30;
const int palmsNumber = 10;
const int grassNumber = 500;
const int flowersNumber = 42;

float random(float min,float max)
{
    return (min+1) + (((float) rand()) / (float) RAND_MAX)* (max-(min+1));
}

class CProgram
{
public:
    GLuint idProgram;

    void CreateProgram()
    {
        idProgram = glCreateProgram();
    }

    void Use()
    {
        glUseProgram( idProgram );
    }

    void UnUse()
    {
        glUseProgram( 0 );
    }

    void CreateFromShaders(char* vertex_filename, char* fragment_filename)
    {
        idProgram = glCreateProgram();
        glAttachShader( idProgram, LoadShader(GL_VERTEX_SHADER, vertex_filename));
        glAttachShader( idProgram, LoadShader(GL_FRAGMENT_SHADER, fragment_filename));
        LinkAndValidateProgram( idProgram );
    }

    void SetVP_cP(glm::mat4 matView,glm::mat4 matProj,glm::vec3 cameraPos)
    {
        glUniformMatrix4fv( glGetUniformLocation(idProgram, "matView"), 1, GL_FALSE, glm::value_ptr(matView) );
        glUniformMatrix4fv( glGetUniformLocation(idProgram, "matProj"), 1, GL_FALSE, glm::value_ptr(matProj) );
        glUniform3fv( glGetUniformLocation( idProgram, "cameraPos" ), 1, &cameraPos[0] );

    }

    void Clean()
    {
        glDeleteProgram( idProgram );
    }

    void SendInt(char* name, int value)
    {
        glUniform1i(glGetUniformLocation(idProgram, name), value);
    }

    void SendFloat(char* name, float value)
    {
        glUniform1f(glGetUniformLocation(idProgram, name), value);
    }

};
CProgram glownyProgram,SkyBox_Program,DepthMap_idProgram;

class CTexture
{
public:
    GLuint idTexture;
    char* textureName;
    int tex_width, tex_height, tex_n;
    unsigned char *tex_data;

    void Create(const char* textureName)
    {
        printf("Create: %s ",textureName);
        tex_data = stbi_load (textureName, &tex_width, &tex_height, &tex_n, 0);
        if ( tex_data == NULL)
        {
            printf ("Image can’t be loaded!\n");
        }

        glGenTextures(1, &idTexture);
        glBindTexture(GL_TEXTURE_2D, idTexture);
        printf("id: %d ",idTexture);

        if(tex_n == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, tex_data);
        else if(tex_n == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
        else printf("Problem with texture\n");

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    }
};

class CMaterial
{
public:

    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
    float Shininess;

    void Create(vec3 ambient, vec3 diffuse, vec3 specular, float shininess)
    {
        Ambient = ambient;
        Diffuse = diffuse;
        Specular = specular;
        Shininess = shininess;
    }

};
GLfloat positionsSkyBox[8*3] =
{
    1.0f, 1.0f+0.95f, 1.0f,   // 0
    -1.0f, 1.0f+0.95f, 1.0f,  // 1
    -1.0f, -1.0f+0.95f, 1.0f, // 2
    1.0f, -1.0f+0.95f, 1.0f,  // 3
    1.0f, 1.0f+0.95f, -1.0f,  // 4
    -1.0f, 1.0f+0.95f, -1.0f, // 5
    -1.0f, -1.0f+0.95f, -1.0f,// 6
    1.0f, -1.0f+0.95f, -1.0f  // 7
};

GLuint indicesSkyBox[12*3] =
{
    5, 0, 1,
    5, 4, 0,
    2, 0, 3,
    2, 1, 0,
    7, 0, 4,
    7, 3, 0,
    3, 6, 2,
    3, 7, 6,
    1, 2, 6,
    1, 6, 5,
    4, 5, 6,
    4, 6, 7
};
const char  filesSkyBox1[6][30] =
{
    "skybox1/posx.jpg",
    "skybox1/negx.jpg",
    "skybox1/posy.jpg",
    "skybox1/negy.jpg",
    "skybox1/posz.jpg",
    "skybox1/negz.jpg",

};
char filesSkyBox2[6][30] =
{
    "skybox2/posx.jpg",
    "skybox2/negx.jpg",
    "skybox2/posy.jpg",
    "skybox2/negy.jpg",
    "skybox2/posz.jpg",
    "skybox2/negz.jpg",

};

const GLenum targetsSkyBox[6] =
{
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

class CSkyBox
{
public:

    GLuint SkyBox_VAO;
    GLuint SkyBox_Texture;

    void CreateSkyBox(const char (*filesSkyBox)[30])
    {
        // Vertex arrays
        glGenVertexArrays( 1, &SkyBox_VAO );
        glBindVertexArray( SkyBox_VAO );
        // Wspolrzedne wierzchokow
        GLuint vBuffer_pos;
        glGenBuffers( 1, &vBuffer_pos );
        glBindBuffer( GL_ARRAY_BUFFER, vBuffer_pos );
        glBufferData( GL_ARRAY_BUFFER, 8*3*sizeof(GLfloat), positionsSkyBox, GL_STATIC_DRAW );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
        glEnableVertexAttribArray( 0 );
        // Tablica indeksow
        GLuint vBuffer_idx;
        glGenBuffers( 1, &vBuffer_idx );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vBuffer_idx );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, 12*3*sizeof( GLuint ), indicesSkyBox, GL_STATIC_DRAW );
        glBindVertexArray( 0 );


        // Tekstura CUBE_MAP
        glGenTextures( 1, &SkyBox_Texture);
        glBindTexture( GL_TEXTURE_CUBE_MAP, SkyBox_Texture );

        // Wylaczanie flipowania tekstury
        stbi_set_flip_vertically_on_load(false);

        // Utworzenie 6 tekstur dla kazdej sciany
        for (int i=0; i < 6; ++i)
        {
            int tex_width, tex_height, n;
            unsigned char *tex_data;

            tex_data = stbi_load(filesSkyBox[i], &tex_width, &tex_height, &n, 0);

            if (tex_data == NULL)
            {
                printf("Image %s can't be loaded!\n", filesSkyBox[i]);
                exit(1);
            }
            // Zaladowanie danych do tekstury OpenGL
            glTexImage2D( targetsSkyBox[i], 0, GL_RGB, tex_width, tex_height, 0, GL_RGB, GL_UNSIGNED_BYTE, tex_data);


            // Zwolnienie pamieci pliku graficznego
            stbi_image_free(tex_data);
        }

        // Przykladowe opcje tekstury
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT );
        glGenerateMipmap( GL_TEXTURE_CUBE_MAP );

        // Powrot Flipowanie tekstury
        stbi_set_flip_vertically_on_load(true);
    }
    void DrawSkyBox(glm::mat4 matProj, glm::mat4 matView)
    {
        GLint programId;
        glGetIntegerv(GL_CURRENT_PROGRAM, &programId);
        // Specjalny potok dla SkyBoxa (uproszczony)
        // Ten program nie ma oswietlenia/cieni itp.
        glUseProgram(programId);

        // Przeskalowanie boxa i przeslanie macierzy rzutowania
        // korzystajac z macierzy Proj i View naszej sceny
        glm::mat4 matPVM = matProj * matView * glm::scale(glm::mat4(1), glm::vec3(40.0, 40.0, 40.0));
        glUniformMatrix4fv( glGetUniformLocation( programId, "matPVM" ), 1, GL_FALSE, glm::value_ptr(matPVM) );

        // Aktywacja tekstury CUBE_MAP
        glActiveTexture(GL_TEXTURE1);
        glBindTexture( GL_TEXTURE_CUBE_MAP, SkyBox_Texture );
        //SkyBox_Program.SendInt("tex_skybox",1);
        glUniform1i(glGetUniformLocation(SkyBox_Program.idProgram, "tex_skybox"), 1);

        // Rendering boxa
        glBindVertexArray( SkyBox_VAO );
        glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL );
        glBindVertexArray( SkyBox_VAO );

        glUseProgram(0);
    }
};
CSkyBox SkyBox1,SkyBox2;

// Kwadrat wspolrzedne
GLfloat vertices_pos[] =
{
    1.0f, 1.0f, 0.0f,
    2.0f, 1.0f, 0.0f,
    2.0f, 2.0f, 0.0f,

    2.0f, 2.0f, 0.0f,
    1.0f, 2.0f, 0.0f,
    1.0f, 1.0f, 0.0f,

};
GLfloat vertices_pos2[] =
{
    -2.0f, -2.0f, 0.0f,//1
        2.0f, -2.0f, 0.0f,//2
        2.0f, 2.0f, 0.0f,//3

        2.0f, 2.0f, 0.0f,//4
        -2.0f, 2.0f, 0.0f,//5
        -2.0f, -2.0f, 0.0f,//6

    };
GLfloat vertices_tex[] =
{
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,

    1.0f, 1.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,
};

class CMiniMap
{
public:

    GLuint idFrameBuffer;   // FBO
    GLuint idDepthBuffer;	// Bufor na skladowa glebokosci
    GLuint idTextureBuffer; // Tekstura na skladowa koloru
    GLuint idVAO_Board;
    GLuint vao_pos;
    GLuint vao_uv;
    GLuint Flag;


    //GLuint idProgram;
    CProgram MiniMap_Program;
    int Buffer_Width;
    int Buffer_Height;

    void Initialize(char* vertex_filename, char* fragment_filename,int buffer_Width,int buffer_Height, int flag )
    {
        this->Buffer_Width = buffer_Width;
        this->Buffer_Height = buffer_Height;
        this->Flag = flag;

        MiniMap_Program.CreateFromShaders(vertex_filename,fragment_filename);

        CreateBoard();

        // 1. Stworzenie obiektu FBO
        glGenFramebuffers(1, &idFrameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, idFrameBuffer);

        // 2. Stworzenie obiektu tekstury na skladowa koloru
        glGenTextures(1, &idTextureBuffer);
        glBindTexture(GL_TEXTURE_2D, idTextureBuffer);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Buffer_Width, Buffer_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 3. Polaczenie tekstury ze skladowa koloru FBO
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               idTextureBuffer, 0);

        // 4. Stworzenie obiektu render buffer dla skladowej glebokosci
        glGenRenderbuffers(1, &idDepthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, idDepthBuffer);

        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, Buffer_Width, Buffer_Height);

        // 5. Polaczenie bufora glebokosci z aktualnym obiektem FBO
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, idDepthBuffer);

        // 6. Sprawdzenie czy pomyslnie zostal utworzony obiekt bufora ramki
        //    a nastepnie powrot do domyslnego bufora ramki

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            printf("Error: Framebuffer is not complete!\n");
            exit(1);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    }

    void CreateBoard()
    {
        glGenVertexArrays( 1, &idVAO_Board );
        glBindVertexArray( idVAO_Board );


        glGenBuffers( 1, &vao_pos );
        glBindBuffer( GL_ARRAY_BUFFER, vao_pos );

        if(Flag == 0)
            glBufferData( GL_ARRAY_BUFFER, sizeof(vertices_pos), vertices_pos, GL_STATIC_DRAW );
        else
            glBufferData( GL_ARRAY_BUFFER, sizeof(vertices_pos2), vertices_pos2, GL_STATIC_DRAW );

        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
        glEnableVertexAttribArray( 0 );

        glGenBuffers( 1, &vao_uv );
        glBindBuffer( GL_ARRAY_BUFFER, vao_uv );

        glBufferData( GL_ARRAY_BUFFER, sizeof(vertices_tex), vertices_tex, GL_STATIC_DRAW );
        glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, NULL );
        glEnableVertexAttribArray( 1 );

        glBindVertexArray( 0 );
    }
    void Use()
    {
        glViewport(0, 0, Buffer_Width, Buffer_Height);

        glBindFramebuffer(GL_FRAMEBUFFER, idFrameBuffer);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }
    void Create()
    {
        MiniMap_Program.Use();

        glm::mat4 matProjmm = glm::ortho(-2.0,2.0,-2.0,2.0);
        glUniformMatrix4fv(glGetUniformLocation(MiniMap_Program.idProgram, "matPVM"), 1, GL_FALSE, glm::value_ptr(matProjmm));


        MiniMap_Program.SendInt("miniMap",showMiniMap);
        MiniMap_Program.SendInt("postprocessing",postprocessing);


        glBindVertexArray(idVAO_Board);

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, idTextureBuffer);
        __CHECK_FOR_ERRORS
        glDrawArrays(GL_TRIANGLES, 0, 6);
        __CHECK_FOR_ERRORS

        MiniMap_Program.UnUse();

    }
};
CMiniMap MiniMap, Postprocessing1,Postprocessing2;

class CMesh
{
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    GLuint idVAO;		// tablic wierzcholkow
    GLuint idVBO_coord;	// bufor wspolrzednych
    GLuint idVBO_color; // bufor na kolory

    glm::mat4 matModel;
    CTexture texture;
    bool isTexture;
    CMaterial material;

    // Metody tworzaca VAO i VBO z pliku OBJ
    void CreateFromOBJ(const char* fileName, vec3 ambient, vec3 diffuse, vec3 specular, float shininess, const char* texName = NULL)
    {
        // Wczytanie pliku OBJ
        if (!loadOBJ(fileName, vertices, uvs, normals))
        {
            printf("File not loaded!\n");
        }
        printf("Loaded %d vertices\n", vertices.size());

        // Wykorzystamy dane wczytane z pliku OBJ
        // do stworzenia buforow w VAO
        glGenVertexArrays( 1, &idVAO );
        glBindVertexArray( idVAO );
        // Bufor na wspolrzedne wierzcholkow
        glGenBuffers( 1, &idVBO_coord );
        glBindBuffer( GL_ARRAY_BUFFER, idVBO_coord );
        glBufferData( GL_ARRAY_BUFFER, sizeof( glm::vec3 ) * vertices.size(), &vertices[0], GL_STATIC_DRAW );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
        glEnableVertexAttribArray( 0 );

        // Bufor na wspolrzedne UV
        glGenBuffers( 1, &idVBO_uv );
        glBindBuffer( GL_ARRAY_BUFFER, idVBO_uv );
        glBufferData( GL_ARRAY_BUFFER, sizeof(glm::vec2) * uvs.size(), &uvs[0], GL_STATIC_DRAW );
        glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, NULL );
        glEnableVertexAttribArray( 1 );

        // Bufor na wektory normalne
        glGenBuffers( 1, &idVBO_normal );
        glBindBuffer( GL_ARRAY_BUFFER, idVBO_normal );
        glBufferData( GL_ARRAY_BUFFER, sizeof(glm::vec3) * normals.size(), &normals[0], GL_STATIC_DRAW );
        glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, 0, NULL );
        glEnableVertexAttribArray( 2 );
        glBindVertexArray( 0 );

        if (texName != NULL)
        {
            this->texture.Create(texName);
            isTexture = true;
        }
        else isTexture = false;

        this->material.Create(ambient, diffuse, specular, shininess);

    }
    // Metody tworzaca VAO i VBO z pliku OBJ
    void Create(GLfloat *vertices, GLfloat *uvs, vec3 ambient, vec3 diffuse, vec3 specular, float shininess, const char* texName = NULL)
    {

        glGenVertexArrays( 1, &idVAO );
        glBindVertexArray( idVAO );
        // Bufor na wspolrzedne wierzcholkow
        glGenBuffers( 1, &idVBO_coord );
        glBindBuffer( GL_ARRAY_BUFFER, idVBO_coord );
        glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW );
        glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
        glEnableVertexAttribArray( 0 );

        // Bufor na wspolrzedne UV
        glGenBuffers( 1, &idVBO_uv );
        glBindBuffer( GL_ARRAY_BUFFER, idVBO_uv );
        glBufferData( GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW );
        glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, NULL );
        glEnableVertexAttribArray( 1 );

        if (texName != NULL)
        {
            this->texture.Create(texName);
            isTexture = true;
        }
        else isTexture = false;

        this->material.Create(ambient, diffuse, specular, shininess);

    }
    void Draw(CProgram currentProgram)
    {
        __CHECK_FOR_ERRORS
        GLint idProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &idProgram);

        sendMaterialParameters(material);
        __CHECK_FOR_ERRORS
        glUniformMatrix4fv( glGetUniformLocation(idProgram, "matModel"), 1, GL_FALSE, glm::value_ptr(this->matModel) );
        glm::mat3 matNormal = glm::mat3(transpose(inverse(matModel)));
        glUniformMatrix3fv(glGetUniformLocation(idProgram, "matNormal"), 1, GL_FALSE, glm::value_ptr(matNormal) );
        __CHECK_FOR_ERRORS
        if(isTexture)
        {
            __CHECK_FOR_ERRORS
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, texture.idTexture);
            currentProgram.SendInt("isTexture",1);
            currentProgram.SendInt("tex",2);

            glActiveTexture(GL_TEXTURE1);
            if(currentSkyBox == 1)
                glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBox1.SkyBox_Texture);
            else
                glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBox2.SkyBox_Texture);
            currentProgram.SendInt("tex_skybox",1);
            __CHECK_FOR_ERRORS
        }
        else
        {
            __CHECK_FOR_ERRORS
            glBindTexture(GL_TEXTURE_2D, 0);
            __CHECK_FOR_ERRORS
            currentProgram.SendInt("isTexture",0);
            __CHECK_FOR_ERRORS
            glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBox2.SkyBox_Texture);
            currentProgram.SendInt("tex_skybox",1);
            __CHECK_FOR_ERRORS
        }
        __CHECK_FOR_ERRORS
        glBindVertexArray( idVAO );
        glDrawArrays( GL_TRIANGLES, 0, vertices.size() );
        glBindVertexArray( 0 );
        __CHECK_FOR_ERRORS
    }
    void Draw(CProgram currentProgram,glm::mat4 matModelForDraw)
    {
        __CHECK_FOR_ERRORS
        GLint idProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &idProgram);

        sendMaterialParameters(material);
        __CHECK_FOR_ERRORS
        glUniformMatrix4fv( glGetUniformLocation(idProgram, "matModel"), 1, GL_FALSE, glm::value_ptr(matModelForDraw) );
        glm::mat3 matNormal = glm::mat3(transpose(inverse(matModelForDraw)));
        glUniformMatrix3fv(glGetUniformLocation(idProgram, "matNormal"), 1, GL_FALSE, glm::value_ptr(matNormal) );
        __CHECK_FOR_ERRORS
        if(isTexture)
        {
            __CHECK_FOR_ERRORS
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, texture.idTexture);
            currentProgram.SendInt("isTexture",1);
            currentProgram.SendInt("tex",2);

            glActiveTexture(GL_TEXTURE1);
            if(currentSkyBox == 1)
                glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBox1.SkyBox_Texture);
            else
                glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBox2.SkyBox_Texture);
            currentProgram.SendInt("tex_skybox",1);
            __CHECK_FOR_ERRORS
        }
        else
        {
            __CHECK_FOR_ERRORS
            glBindTexture(GL_TEXTURE_2D, 0);
            __CHECK_FOR_ERRORS
            currentProgram.SendInt("isTexture",0);
            __CHECK_FOR_ERRORS
            glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBox2.SkyBox_Texture);
            currentProgram.SendInt("tex_skybox",1);
            __CHECK_FOR_ERRORS
        }
        __CHECK_FOR_ERRORS
        glBindVertexArray( idVAO );
        glDrawArrays( GL_TRIANGLES, 0, vertices.size() );
        glBindVertexArray( 0 );
        __CHECK_FOR_ERRORS
    }
    void DrawShadow()
    {
        GLint idProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &idProgram);
        glUniformMatrix4fv( glGetUniformLocation(idProgram, "matModel"), 1, GL_FALSE, glm::value_ptr(this->matModel) );
        glBindVertexArray( idVAO );
        glDrawArrays( GL_TRIANGLES, 0, vertices.size() );
        glBindVertexArray( 0 );
    }
    void DrawShadow(glm::mat4 matModels)
    {
        GLint idProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &idProgram);
        glUniformMatrix4fv( glGetUniformLocation(idProgram, "matModel"), 1, GL_FALSE, glm::value_ptr(matModels) );
        glBindVertexArray( idVAO );
        glDrawArrays( GL_TRIANGLES, 0, vertices.size() );
        glBindVertexArray( 0 );
    }
    // Przeslanie parametrow materialow do shadera
    void sendMaterialParameters(CMaterial material)
    {
        GLint programId;
        glGetIntegerv(GL_CURRENT_PROGRAM, &programId);

        glUniform3fv(glGetUniformLocation(programId, "myMaterial.Ambient"), 1, glm::value_ptr(material.Ambient));
        glUniform3fv(glGetUniformLocation(programId, "myMaterial.Diffuse"), 1, glm::value_ptr(material.Diffuse));
        glUniform3fv(glGetUniformLocation(programId, "myMaterial.Specular"), 1, glm::value_ptr(material.Specular));
        glUniform1f(glGetUniformLocation(programId, "myMaterial.Shininess"), material.Shininess);
    }
    void Rotate(float value, float x, float y, float z)
    {
        matModel = glm::rotate(matModel, value, glm::vec3(x, y, z));
    }

    void Move(float x, float y_shift, float z)
    {
        float y = ((myGround.getAltitute(glm::vec2(x,z)))+y_shift);
        matModel = glm::translate(matModel, glm::vec3(x, y, z));
    }
    void Scale(float scale)
    {
        matModel = glm::scale(matModel, glm::vec3(scale, scale, scale));
    }
    void setPosition(float x = 0.0, float y_shift = 0.0, float z = 0.0)
    {
        resetPosition();
        float y = ((myGround.getAltitute(glm::vec2(x,z)))+y_shift);
        matModel = glm::translate(matModel, glm::vec3(x, y, z));
    }

    void resetPosition()
    {
        matModel = glm::mat4(1.0);
    }
    void setRotation(float value = 0.0, float x = 0.0, float y = 0.0, float z = 0.0)
    {
        matModel = glm::rotate(matModel, value, glm::vec3(x, y, z));
    }
    void Clean()
    {
        glDeleteVertexArrays( 1, &idVBO_coord );
        glDeleteVertexArrays( 1, &idVBO_color );
        glDeleteVertexArrays( 1, &idVBO_uv );
        glDeleteVertexArrays( 1, &idVAO );
    }

};
CMesh ground, palm, rock2, sphere, sphere2,sphere3,grass,flower1,flower2,flower3,flower4,flower5,flower6,flower7;
std::array<glm::vec3, rocksNumber> rocksPosition;
std::array<glm::vec3, grassNumber> grassPosition;
std::array<glm::vec3, palmsNumber> palmsPosition;
std::array<glm::vec3, flowersNumber> flowersPosition;
std::array<float, rocksNumber> rocksScale;
std::array<float, palmsNumber> palmsScale;
std::array<bool, flowersNumber> isFlowerVisible;

class CMultipleMesh : public CMesh
{
public:
    GLuint vInstances;
    int Number;
    glm::mat4x4* Table_of_matModel;
    float angle;


    void CreateInstances(int flag, int number, float minX, float maxX, float minY, float maxY, float minZ, float maxZ, float minScale, float maxScale, float minAngle, float maxAngle)
    {
        Number = number;
        Table_of_matModel = new  glm::mat4x4[Number];

        glBindVertexArray( idVAO );
        for (int i = 0; i < Number; ++i)
        {
            // Wylosuj dane dla kazdej instancji
            float x = random(minX,maxX);
            float z = random(minZ,maxZ);
            float y;
            if(flag == 1)
                y = random(minY,maxY);
            else
                y = myGround.getAltitute(glm::vec2(x,z));

            float scale = random(minScale,maxScale);
            if(flag == 1)
                angle = random(minAngle,maxAngle);

            // Tworzenie macierzy modelu dla kazdej instancji
            Table_of_matModel[i] = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
            if(flag == 1)
                Table_of_matModel[i] = glm::rotate(Table_of_matModel[i], angle, glm::vec3(0.0f, 1.0f, 0.0f));
            Table_of_matModel[i] = glm::scale(Table_of_matModel[i], glm::vec3(scale, scale, scale));
        }

        // Tworzenie bufora VBO dla macierzy modelu instancji
        GLuint vInstances;
        glGenBuffers(1, &vInstances);
        glBindBuffer(GL_ARRAY_BUFFER, vInstances);
        glBufferData(GL_ARRAY_BUFFER, Number * sizeof(glm::mat4), Table_of_matModel, GL_STATIC_DRAW);

        // Ustawianie atrybutów dla macierzy modelu instancji
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)0);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(3 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(6);

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }
    void resetPosition()
    {
        for (int i = 0; i < Number; ++i)
        {
            Table_of_matModel[i] = glm::mat4(1.0);
        }
    }
    void Rotate(float value)
    {
        for (int i = 0; i < Number; ++i)
        {
            Table_of_matModel[i] = glm::rotate(Table_of_matModel[i], value, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
    //metoda generujaca obraz na ekranie
    void MultiDraw(CProgram currentProgram)
    {
        GLint idProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &idProgram);

        sendMaterialParameters(material);

        if(isTexture)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, texture.idTexture);
            currentProgram.SendInt("isTexture",1);
            currentProgram.SendInt("tex",2);

            glActiveTexture(GL_TEXTURE1);
            if(currentSkyBox == 1)
                glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBox1.SkyBox_Texture);
            else
                glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBox2.SkyBox_Texture);
            currentProgram.SendInt("tex_skybox",1);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
            currentProgram.SendInt("isTexture",0);

            glBindTexture(GL_TEXTURE_CUBE_MAP, SkyBox2.SkyBox_Texture);
            currentProgram.SendInt("tex_skybox",1);
        }

        glBindVertexArray( idVAO );

        currentProgram.SendInt("instanced",1);
        glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size(), Number);
        currentProgram.SendInt("instanced",0);

        glBindVertexArray(0);
    }

};
CMultipleMesh bees;

class CPlayer
{

public:

    // pozycja obiektu
    glm::vec3 Position = glm::vec3(0.0, 0.0, 0.0);

    // kat orientacji obiektu
    float Angle = 0.0;

    // wektor orientacji (UWAGA! wyliczany z Angle)
    glm::vec3 Direction = glm::vec3(1.0, 0.0, 0.0);

    // wskaznik do obiektu podloza
    CGround *myGround = NULL;

    // macierz modelu
    glm::mat4 matModel = glm::mat4(1.0);

    CMesh player;

    CCollider *Collider = NULL;

    CPlayer() { }

    // Inicjalizacja obiektu
    void Init(CGround *ground)
    {
        myGround = ground;

        glm::vec2 pos = glm::vec2(Position.x, Position.z);
        Position.y = myGround->getAltitute(pos);

        this->player.CreateFromOBJ("models/bee.obj",  vec3 (0.25f, 0.20725f, 0.20725f),vec3 (10.0f, 10.0f, 10.0f),vec3 (0.0f, 0.0f, 0.0f),11.264f,"textures/bee.jpg");

        // Aktualizacja polozenia/macierzy itp.
        Update();

    }
    // renderowanie obiektu
    void Draw(CProgram currentProgram)
    {
        // pobieranie aktualnego potoku
        GLint idProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &idProgram);

        this->player.Draw(currentProgram);

    }
    void DrawShadow()
    {
        this->player.DrawShadow();
    }

    // Obliczenie wysokosci nad ziemia
    void Update()
    {
        glm::vec2 pos = glm::vec2(Position.x, Position.z);
        Position.y = myGround->getAltitute(pos);

        matModel = glm::translate(glm::mat4(1.0), Position);
        matModel = glm::rotate(matModel, Angle, glm::vec3(0.0, 1.0, 0.0));

        player.matModel = matModel;
        player.Rotate(1.5,0.0,1.0,0.0);
        player.Move(0.0,0.5,0.0);
        player.Scale(5);
    }

    bool isCollision(glm::vec3 colliderPosition,float collisionDistance)
    {

        float distance = glm::distance(Position, colliderPosition);

        if (distance < collisionDistance)
            return true;
        return false;
    }
    bool isSceneCollision()
    {
        float distance = glm::distance(Position, glm::vec3(0.0, 1.0, 0.0));

        if (distance > 39.0f)
            return true;
        return false;
    }

    // ustawienie pozycji na scenie
    void SetPosition(glm::vec3 pos)
    {
        Position = pos;
        Update();
    }

    void Move(float val)
    {
        float value;
        // kopia polozenia
        glm::vec3 oldPosition = Position;

        // aktualizujemy polozenie
        Position += Direction * val;

        // sprawdzamy kolizje
        for(int i=0; i<rocksNumber; i++)
        {
            if (rocksScale[i]>0.5)
                value = 2.5f;
            else
                value = 1.5f;
            if (isCollision(rocksPosition[i],value))
            {
                Position = oldPosition;
                return;
            }
        }
        for(int i=0; i<palmsNumber; i++)
        {
            if (isCollision(palmsPosition[i],1.5f))
            {
                Position = oldPosition;
                return;
            }
        }
        for(int i=0; i<flowersNumber; i++)
        {
            if (isCollision(flowersPosition[i],1.5f))
            {
                if(isFlowerVisible[i] && i<=flowersNumber/7*4)
                {
                    isFlowerVisible[i]=false;
                    points++;
                }
                else if(isFlowerVisible[i] && i>flowersNumber/7*4) {
                    isFlowerVisible[i]=false;
                    points--;
                }
            }
        }
        if (isSceneCollision())
        {
            Position = oldPosition;
            return;
        }

        // aktualizacja
        Update();
    }
// zmiana orientacji obiektu
    void Rotate(float angle)
    {
        Angle += angle;
        Direction.x = cos(Angle);
        Direction.z = -sin(Angle);

        // aktualizacja
        Update();
    }

};
CPlayer myPlayer;
class CCamera
{

public:

    // Macierze rzutowania i widoku
    glm::mat4 matProj;
    glm::mat4 matView;

    // Skladowe kontrolujace matView
    glm::vec3 Position;        // polozenie kamery
    glm::vec3 Angles;          // pitch, yaw, roll
    glm::vec3 Up;              // domyslnie wektor (0,1,0)
    glm::vec3 Direction;       // wektor kierunku obliczany z Angles

    // Skladowe kontrolujace matProj
    float Width, Height;       // proporcje bryly obcinania
    float NearPlane, FarPlane; // plaszczyzny tnace
    float Fov;	               // kat widzenia kamery


    // ---------------------------------------------
    // Domyslny konstruktor
    CCamera()
    {



        Up = glm::vec3(0.0f, 0.5f, 0.0f);
        Position = glm::vec3(0.0f, 0.0f, 0.0f);
        Angles = glm::vec3(0.0f, 0.0f, 0.0f);
        Fov = 80.0f;

        NearPlane = 0.1f;
        FarPlane = 150.0f;

        // Wywolanie metody aktualizujacej
        // m.in. Direction i matView
        this->Update();
    }

    // ---------------------------------------------
    // Wirtualna metoda aktualizujaca dane kamery
    // przydatna w klasach pochodnych
    virtual void Update()
    {
        // wektor Direction
        Direction.x = cos(Angles.y) * cos(Angles.x);
        Direction.y = sin(Angles.x);
        Direction.z = -sin(Angles.y) * cos(Angles.x);

        // macierz widoku
        matView = glm::lookAt(Position, Position+Direction, Up);
    }
    void UpdateMatView()
    {
        matView = glm::lookAt(Position, Position+Direction, Up);
    }

    // ---------------------------------------------
    // Metoda aktualizujaca macierz projection
    // wywolywana np. w Reshape()
    void UpdatePerspective(float width, float height)
    {
        Width = width;
        Height = height;
        matProj = glm::perspectiveFov(glm::radians(Fov), Width, Height, NearPlane, FarPlane);
    }

    // ---------------------------------------------
    // Metoda aktualizujaca macierz projection
    // wywolywana np. w Reshape()
    void UpdateOrtho(float width, float height)
    {}


    // ---------------------------------------------
    // przesylanie obu macierzy do programu pod
    // wskazane nazwy zmiennych uniform
    void SendPV(const char *proj = "matProj", const char *view = "matView")
    {

        // pobranie id aktualnego programu
        GLint programId;
        glGetIntegerv(GL_CURRENT_PROGRAM, &programId);

        glUniformMatrix4fv( glGetUniformLocation( programId, proj ), 1, GL_FALSE, glm::value_ptr(matProj) );
        glUniformMatrix4fv( glGetUniformLocation( programId, view ), 1, GL_FALSE, glm::value_ptr(matView) );
    }

    // ---------------------------------------------
    // przesylanie iloczynu macierzy matProj*matView
    // do programu pod wskazana nazwe
    void SendPV(const char *projview)
    {
        // pobranie id aktualnego programu
        GLint programId;
        glGetIntegerv(GL_CURRENT_PROGRAM, &programId);

        glm::mat4 matProjView = matProj * matView;
        glUniformMatrix4fv( glGetUniformLocation( programId, projview ), 1, GL_FALSE, glm::value_ptr(matProjView) );
    }

    // ---------------------------------------------
    // zmiana kata widzenia kamery (np. przy zoomowaniu)
    void AddFov(GLfloat _fov)
    {
        this->Fov += _fov;
        this->UpdatePerspective(this->Width, this->Height);
    }

};

// ----------------------------------------------------------
class CFPSCamera : public CCamera
{

public:

    CPlayer *Player = NULL;
    glm::vec3 ShiftUp;

    // ---------------------------------------------
    void Init()
    {
    }
    virtual void Update(CPlayer player)
    {
        // Ustawienie katow orientacji kamery
        // zgodnie z katami postaci
        Angles = glm::vec3(0.0, Player->Angle, 0.0);

        // Ustawienie polozenia kamery zgodnie
        // z polozeniem postaci
        Position = Player->Position + ShiftUp;

        CCamera::Update();
    }

};

class CTPSCamera : public CFPSCamera
{

public:

    // dodatkowe przesuniecie kamery trzecioosobowej
    // w przypadku kamery TPS bedzie ono wyliczane
    // z wektora Direction
    glm::vec3 ShiftBack;


    // ---------------------------------------------
    void Init(CPlayer *player, glm::vec3 shiftBack)
    {
        Player = player;
        ShiftBack = shiftBack;
    }

    // ---------------------------------------------
    void Update()
    {
        // Ustawienie katow orientacji kamery
        // zgodnie z katami postaci
        Angles = glm::vec3(0.0, Player->Angle, 0.0);

        // Ustawienie polozenia kamery zgodnie
        // z polozeniem postaci
        Position = Player->Position - ShiftBack;

        CCamera::Update();
    }

};
CTPSCamera myCamera;
// Struktura parametrow swiatla
struct LightParam
{
    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
    vec3 Attenuation;
    vec3 Position;
};

struct DirectionalLightParam
{
    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
    vec3 Direction;
};

DirectionalLightParam myDirectionalLight =
{
    vec3(0.1, 0.1, 0.1),   // ambient
    vec3(1.0, 1.0, 1.0),   // diffuse
    vec3(1.0, 1.0, 1.0),   // specular
    vec3(1.0, -1.0, -1.0) // direction
};

// Przykladowe swiatlo punktowe
struct LightParam lights[] =
{
    {
        vec3 (0.1, 0.1, 0.1),   // ambient
        vec3 (1.0, 1.0, 1.0),   // diffuse
        vec3 (1.0, 1.0, 1.0),   // specular
        vec3 (1.0, 0.0, 0.01),   // attenuation
        vec3 (2.0, 1.0, 1.0)
    },    // position

    {
        vec3(0.2, 0.2, 0.2),
        vec3(1.0, 0.0, 0.0),
        vec3(0.5, 0.5, 0.5),
        vec3(1.0, 0.0, 0.02),
        vec3(-6.0, 3.5, 0.5)
    }
};


// ---------------------------------------------------
// Przeslanie parametrow oswietlenia do shadera
void sendLightParameters(const LightParam lights[])
{
    // pobranie id aktualnego programu
    GLint programId;
    glGetIntegerv(GL_CURRENT_PROGRAM, &programId);

    glownyProgram.SendInt("lightModel",lightModel);
    glownyProgram.SendInt("lightTime",lightTime);
    glownyProgram.SendInt("lightAnimation",lightAnimation);
    glownyProgram.SendInt("lightType",lightType);
    glownyProgram.SendInt("lightSwitch",lightSwitch);

    for (int i = 0; i < 2; ++i)
    {
        glUniform3fv(glGetUniformLocation(programId, ("lights[" + to_string(i) + "]." + "Ambient").c_str()), 1, glm::value_ptr(lights[i].Ambient));
        glUniform3fv(glGetUniformLocation(programId, ("lights[" + to_string(i) + "]." + "Diffuse").c_str()), 1, glm::value_ptr(lights[i].Diffuse));
        glUniform3fv(glGetUniformLocation(programId, ("lights[" + to_string(i) + "]." + "Specular").c_str()), 1, glm::value_ptr(lights[i].Specular));
        glUniform3fv(glGetUniformLocation(programId, ("lights[" + to_string(i) + "]." + "Attenuation").c_str()), 1, glm::value_ptr(lights[i].Attenuation));
        glUniform3fv(glGetUniformLocation(programId, ("lights[" + to_string(i) + "]." + "Position").c_str()), 1, glm::value_ptr(lights[i].Position));
    }

}
void sendDirectionalLightParameters(DirectionalLightParam light)
{
    // pobranie id aktualnego programu
    GLint programId;
    glGetIntegerv(GL_CURRENT_PROGRAM, &programId);

    glownyProgram.SendInt("lightModel",lightModel);
    glownyProgram.SendInt("lightTime",lightTime);
    glownyProgram.SendInt("lightAnimation",lightAnimation);
    glownyProgram.SendInt("lightType",lightType);
    glownyProgram.SendInt("lightSwitch",lightSwitch);

    glUniform3fv(glGetUniformLocation(programId, "myDirectionalLight.Ambient"), 1, glm::value_ptr(light.Ambient));
    glUniform3fv(glGetUniformLocation(programId, "myDirectionalLight.Diffuse"), 1, glm::value_ptr(light.Diffuse));
    glUniform3fv(glGetUniformLocation(programId, "myDirectionalLight.Specular"), 1, glm::value_ptr(light.Specular));
    glUniform3fv(glGetUniformLocation(programId, "myDirectionalLight.Direction"), 1, glm::value_ptr(light.Direction));
}
void GenerateXPosition()
{

    float x=myPlayer.Position.x,y=myPlayer.Position.y,z=myPlayer.Position.z,s=0;
    for(int i=0; i<rocksNumber; i++)
    {
        x = random(-30.0,30.0);
        z = random(-30.0,30.0);
        while( (glm::distance(glm::vec3(x, (myGround.getAltitute(glm::vec2(x, z))), z),myPlayer.Position))<2.5f  )
        {
            x = random(-30.0,30.0);
            z = random(-30.0,30.0);
        }
        y = myGround.getAltitute(glm::vec2(x, z));

        if(i<rocksNumber/3)
            s = random(0.05,0.3);
        else if(i<rocksNumber*(2/3))
            s = random(0.4,0.6);
        else
            s = random(0.7,1.0);
        rocksPosition[i] = glm::vec3(x, y, z);
        rocksScale[i] = s;
    }
    for(int i=0; i<grassNumber; i++)
    {
        x = random(-30.0,30.0);
        z = random(-30.0,30.0);
        y = myGround.getAltitute(glm::vec2(x, z));
        grassPosition[i] = glm::vec3(x, y, z);
    }
    for(int i=0; i<palmsNumber; i++)
    {
        x = random(-30.0,30.0);
        z = random(-30.0,30.0);
        while( (glm::distance(glm::vec3(x, (myGround.getAltitute(glm::vec2(x, z))), z),myPlayer.Position))<2.5f  )
        {
            x = random(-30.0,30.0);
            z = random(-30.0,30.0);
        }
        y = myGround.getAltitute(glm::vec2(x, z));
        s = random(0.7,1.0);
        palmsPosition[i] = glm::vec3(x, y, z);
        palmsScale[i] = s;
    }
    for(int i=0; i<flowersNumber; i++)
    {
        x = random(-30.0,30.0);
        z = random(-30.0,30.0);
        while( (glm::distance(glm::vec3(x, (myGround.getAltitute(glm::vec2(x, z))), z),myPlayer.Position))<2.5f  )
        {
            x = random(-30.0,30.0);
            z = random(-30.0,30.0);
        }
        y = myGround.getAltitute(glm::vec2(x, z));
        flowersPosition[i] = glm::vec3(x, y, z);
    }
}
void DrawX(CProgram currentProgram,float Time)
{
    matModelRocks = glm::translate(matModel, glm::vec3(0.0, 0.0, 0.0));
    for(int i=0; i<rocksNumber; i++)
    {
        matModelRocks = glm::translate(glm::mat4(1.0), rocksPosition[i]);
        float value = Time*1.2;
        matModelRocks = glm::rotate(matModelRocks, value, glm::vec3(0.0, 1.0, 0.0));
        matModelRocks = glm::scale(matModelRocks, glm::vec3(rocksScale[i], rocksScale[i], rocksScale[i]));
        rock2.Draw(currentProgram,matModelRocks);
    }

    if(RockReflection==1)
    {
        glownyProgram.SendInt("withReflection",0);
    }
    if(RockRefraction==1)
    {
        glownyProgram.SendInt("withRefraction",0);
    }

    matModelGrass = glm::translate(matModel, glm::vec3(0.0, 0.0, 0.0));
    for(int i=0; i<grassNumber; i++)
    {
        matModelGrass = glm::translate(glm::mat4(1.0), grassPosition[i]);
        matModelGrass = glm::scale(matModelGrass, glm::vec3(0.5, 0.5, 0.5));
        grass.Draw(currentProgram,matModelGrass);
    }

    matModelPalms = glm::translate(matModel, glm::vec3(0.0, 0.0, 0.0));
    for(int i=0; i<palmsNumber; i++)
    {
        matModelPalms = glm::translate(glm::mat4(1.0), palmsPosition[i]);
        matModelPalms = glm::scale(matModelPalms, glm::vec3(palmsScale[i], palmsScale[i], palmsScale[i]));
        palm.Draw(currentProgram,matModelPalms);
    }
    matModelFlowers = glm::translate(matModel, glm::vec3(0.0, 0.0, 0.0));
    for(int i=0; i<flowersNumber; i++)
    {
        if(isFlowerVisible[i])
        {
            matModelFlowers = glm::translate(glm::mat4(1.0), glm::vec3(flowersPosition[i].x, flowersPosition[i].y+2, flowersPosition[i].z));
            matModelFlowers = glm::rotate(matModelFlowers, -Time, glm::vec3(0.0, 1.0, 0.0));
            matModelFlowers = glm::scale(matModelFlowers, glm::vec3(0.9, 0.9, 0.9));
            glownyProgram.SendInt("flower",1);
            if(i<=flowersNumber/6)
                flower1.Draw(currentProgram,matModelFlowers);
            else if(i<=(flowersNumber/7*2))
                flower2.Draw(currentProgram,matModelFlowers);
            else if(i<=(flowersNumber/7*3))
                flower3.Draw(currentProgram,matModelFlowers);
            else if(i<=(flowersNumber/7*4))
                flower4.Draw(currentProgram,matModelFlowers);
            else if(i<=(flowersNumber/7*5))
                flower5.Draw(currentProgram,matModelFlowers);
            else if(i<=(flowersNumber/7*6))
                flower6.Draw(currentProgram,matModelFlowers);
            else
                flower7.Draw(currentProgram,matModelFlowers);
            glownyProgram.SendInt("flower",0);
        }
    }

}
void DrawShadowX()
{
    matModelRocks = glm::translate(matModel, glm::vec3(0.0, 0.0, 0.0));
    for(int i=0; i<rocksNumber; i++)
    {
        matModelRocks = glm::translate(glm::mat4(1.0), rocksPosition[i]);
        float value = Time*1.2;
        matModelRocks = glm::rotate(matModelRocks, value, glm::vec3(0.0, 1.0, 0.0));
        matModelRocks = glm::scale(matModelRocks, glm::vec3(rocksScale[i], rocksScale[i], rocksScale[i]));
        rock2.DrawShadow(matModelRocks);
    }
    matModelPalms = glm::translate(matModel, glm::vec3(0.0, 0.0, 0.0));
    for(int i=0; i<palmsNumber; i++)
    {
        matModelPalms = glm::translate(glm::mat4(1.0), palmsPosition[i]);
        matModelPalms = glm::scale(matModelPalms, glm::vec3(palmsScale[i], palmsScale[i], palmsScale[i]));
        palm.DrawShadow(matModelPalms);
    }
}
void RenderScene_to_Texture()
{
    matView=myCamera.matView;


    matView = glm::translate(matView, glm::vec3(cameraDistanceY, 0.0, cameraDistanceX));

    glm::vec3 cameraPos = ExtractCameraPos(matView);

    if(showMiniMap == 1)
    {
        vec3 pozycja = myPlayer.Position;
        pozycja.y = 15;
        matView = glm::lookAt(pozycja,myPlayer.Position,glm::vec3(1.0,0.0,0.0));
    }

    // Wlaczenie programu
    glownyProgram.Use();

    glownyProgram.SetVP_cP(matView,matProj,cameraPos);

    sendLightParameters(lights);
    sendDirectionalLightParameters(myDirectionalLight);

    ground.resetPosition();
    ground.Draw(glownyProgram);

    glownyProgram.SendInt("withRefraction",0);
    glownyProgram.SendInt("withReflection",0);

    if(RockReflection == 1)
        glownyProgram.SendInt("withReflection",1);
    else
        glownyProgram.SendInt("withReflection",0);
    if(RockRefraction == 1)
        glownyProgram.SendInt("withRefraction",1);
    else
        glownyProgram.SendInt("withRefraction",0);

    DrawX(glownyProgram,Time);

    if(RockReflection==1)
    {
        glownyProgram.SendInt("withReflection",0);
    }
    if(RockRefraction==1)
    {
        glownyProgram.SendInt("withRefraction",0);
    }

    if(lightType == 0 )
    {
        glownyProgram.SendInt("colorSphere",0);

        sphere.resetPosition();
        sphere.Scale(0.5);
        sphere.Move(lights[1].Position.x,lights[1].Position.y, lights[1].Position.z);
        sphere.Draw(glownyProgram);

        glownyProgram.SendInt("colorSphere",1);
        sphere2.resetPosition();
        sphere2.Scale(0.5);
        sphere2.Move(lights[0].Position.x,lights[0].Position.y, lights[0].Position.z);
        sphere2.Draw(glownyProgram);
    }
    glownyProgram.SendInt("colorSphere",2);

    myPlayer.Draw(glownyProgram);

    glownyProgram.UnUse();

    SkyBox_Program.Use();
    switch ( currentSkyBox )
    {
    case 1:
        SkyBox1.DrawSkyBox(matProj,matView);
        break;
    case 2:
        SkyBox2.DrawSkyBox(matProj,matView);
        break;
    }

    SkyBox_Program.UnUse();

}
void RenderScene_on_Screen()
{

    glViewport(0, 0, WindowWidth, WindowHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



    matView=myCamera.matView;

    glm::vec3 cameraPos = ExtractCameraPos(matView);

    // Wlaczenie programu
    glownyProgram.Use();
    myCamera.Update();
    myCamera.SendPV("matProj","matView");

    // ----------------------------------------
    // Przeslanie macierzy oswietlenia
    // do wyliczenia wspolczynnika cienia
    // ----------------------------------------
    glUniformMatrix4fv( glGetUniformLocation( glownyProgram.idProgram, "lightProj" ), 1, GL_FALSE, glm::value_ptr(lightProj) );
    glUniformMatrix4fv( glGetUniformLocation( glownyProgram.idProgram, "lightView" ), 1, GL_FALSE, glm::value_ptr(lightView) );


    // Przeslanie danych do swiatla kierunkowego
    glUniform3fv( glGetUniformLocation( glownyProgram.idProgram, "lightDirection" ), 1, glm::value_ptr(Light_Direction) );
    glUniform3fv( glGetUniformLocation( glownyProgram.idProgram, "cameraPos" ), 1, glm::value_ptr(cameraPos) );

    // -----------------------------------------------
    // NOWE: ustawiamy teksture shadow mapy na slot 2
    // -----------------------------------------------
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, DepthMap_idTexture);
    glUniform1i(glGetUniformLocation( glownyProgram.idProgram, "tex_shadowMap" ), 4);


    sendLightParameters(lights);
    sendDirectionalLightParameters(myDirectionalLight);


    ground.Draw(glownyProgram);

    glownyProgram.SendInt("withRefraction",0);
    glownyProgram.SendInt("withReflection",0);

    glownyProgram.SendFloat("Time",BeeTime*2);
    glownyProgram.SendInt("Direction",BeeDirection);

    glownyProgram.SendInt("beesFlag",1);
    bees.resetPosition();

    glownyProgram.SendInt("flower",1);
    bees.MultiDraw(glownyProgram);
    glownyProgram.SendInt("flower",0);
    glownyProgram.SendInt("beesFlag",0);

    if(RockReflection == 1)
        glownyProgram.SendInt("withReflection",1);
    else
        glownyProgram.SendInt("withReflection",0);
    if(RockRefraction == 1)
        glownyProgram.SendInt("withRefraction",1);
    else
        glownyProgram.SendInt("withRefraction",0);

    DrawX(glownyProgram,Time);

    if(RockReflection==1)
    {
        glownyProgram.SendInt("withReflection",0);
    }
    if(RockRefraction==1)
    {
        glownyProgram.SendInt("withRefraction",0);
    }

    if(lightType == 0 )
    {
        glownyProgram.SendInt("colorSphere",0);

        sphere.resetPosition();
        sphere.Scale(0.5);
        sphere.Move(lights[1].Position.x,lights[1].Position.y, lights[1].Position.z);
        sphere.Draw(glownyProgram);

        glownyProgram.SendInt("colorSphere",1);
        sphere2.resetPosition();
        sphere2.Scale(0.5);
        sphere2.Move(lights[0].Position.x,lights[0].Position.y, lights[0].Position.z);
        sphere2.Draw(glownyProgram);
    }

    glownyProgram.SendFloat("Time2",Time*2);

    myPlayer.Draw(glownyProgram);

    glownyProgram.SendInt("colorSphere",2);

    glownyProgram.UnUse();

    __CHECK_FOR_ERRORS

    if(showMiniMap==1)
    {
        MiniMap.Create();
    }
    else if(postprocessing == 1)
    {
        Postprocessing1.Create();
    }
    else if(postprocessing == 2)
    {
        Postprocessing2.Create();
    }
    __CHECK_FOR_ERRORS

    SkyBox_Program.Use();
    switch ( currentSkyBox )
    {
    case 1:
        SkyBox1.DrawSkyBox(matProj,matView);
        break;
    case 2:
        SkyBox2.DrawSkyBox(matProj,matView);
        break;
    }

    SkyBox_Program.UnUse();

    __CHECK_FOR_ERRORS
}

void RenderScene_to_ShadowMap()
{
    matView = myCamera.matView;

    __CHECK_FOR_ERRORS
    // 1. Renderowanie z pozycji swiatla do textury DepthMap
    glViewport(0, 0, DepthMap_Width, DepthMap_Height);
    glBindFramebuffer(GL_FRAMEBUFFER, DepthMap_idFrameBuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
    glm::vec3 cameraPos = ExtractCameraPos(matView);
    DepthMap_idProgram.Use();
    myCamera.SendPV("matProj","matView");


    // Przesylamy macierze kamery z punktu
    // widzenia zrodla swiatla
    glUniformMatrix4fv( glGetUniformLocation( DepthMap_idProgram.idProgram, "lightProj" ), 1, GL_FALSE, glm::value_ptr(lightProj) );
    glUniformMatrix4fv( glGetUniformLocation( DepthMap_idProgram.idProgram, "lightView" ), 1, GL_FALSE, glm::value_ptr(lightView) );
    __CHECK_FOR_ERRORS

    ground.DrawShadow();

    myPlayer.DrawShadow();

    DrawShadowX();

    // WYLACZAMY program
    DepthMap_idProgram.UnUse();
}
// ---------------------------------------------------
// ----------------------------------------------------------------------------------
void InitText(const char *font_filename, int font_size)
{

    // Fonts
    text_program = glCreateProgram();
    glAttachShader( text_program, LoadShader(GL_VERTEX_SHADER, "text_freetype/text-ft-vertex.glsl"));
    glAttachShader( text_program, LoadShader(GL_FRAGMENT_SHADER, "text_freetype/text-ft-fragment.glsl"));
    LinkAndValidateProgram( text_program );

    int Window_Width = glutGet(GLUT_WINDOW_WIDTH);
    int Window_Height = glutGet(GLUT_WINDOW_HEIGHT);
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(Window_Width), 0.0f, static_cast<float>(Window_Height));
    glUseProgram(text_program);

    glUniformMatrix4fv(glGetUniformLocation(text_program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_filename, 0, &face))
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

    // set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, font_size);

    // disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // load first 128 characters of ASCII set
    for (unsigned char c = 0; c < 128; c++)
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character =
        {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        text_Characters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);


    // -----------------------------------
    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &text_vao);
    glBindVertexArray(text_vao);
    glGenBuffers(1, &text_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}
int currentTime = 0;
int previousTime = 0;
void calculateFPS()
{
    // Aktualny czas
    currentTime = glutGet(GLUT_ELAPSED_TIME);

    // Jeżeli minął jeden sekundy
    if (currentTime - previousTime > 1000)
    {
        // Oblicz FPS
        fps = frameCount  * 1000 / (currentTime - previousTime);
        // Zresetuj liczniki
        frameCount  = 0;
        previousTime = currentTime;
    }
}
void RenderText()
{
    char txt[255];
    calculateFPS();
    sprintf(txt, "FPS: %d", fps);
    RenderText(txt, 25, 25, 0.5f, glm::vec3(0.5, 0.8f, 0.2f));

    sprintf(txt, "Points: %d", points);
    RenderText(txt, 25, 70, 1.0f, glm::vec3(0.6, 0.9f, 0.3f));

    RenderText("ESC - Exit", 25, 680, 0.5f, glm::vec3(0.3, 0.7f, 0.9f));

}
void DisplayScene()
{
    __CHECK_FOR_ERRORS

    matView=myCamera.matView;
    if(showMiniMap==1)
    {
        MiniMap.Use();
        RenderScene_to_Texture();
    }
    else if(postprocessing == 1)
    {
        Postprocessing1.Use();
        RenderScene_to_Texture();
    }
    else if(postprocessing == 2)
    {
        Postprocessing2.Use();
        RenderScene_to_Texture();
    }
    RenderScene_to_ShadowMap();

    RenderScene_on_Screen();

    RenderText();

    glutSwapBuffers();
    __CHECK_FOR_ERRORS
}

// ---------------------------------------------------
void Initialize()
{
    stbi_set_flip_vertically_on_load(true);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    InitText("text_freetype/arial.ttf", 36);

    for(int i=0; i<flowersNumber; i++)
    {
        isFlowerVisible[i]=true;
    }

    __CHECK_FOR_ERRORS
    // Ustawienia globalne
    glEnable(GL_DEPTH_TEST);
    glClearColor( 0.3f, 0.3f, 0.3f, 1.0f );

    // Potok
    glownyProgram.CreateFromShaders("shaders/vertex.glsl", "shaders/fragment.glsl");

    ground.CreateFromOBJ("models/myGround.obj",vec3 (0.2, 0.2, 0.2),vec3 (4.0, 4.0, 4.0),vec3 (0.5, 0.5, 0.5),32.0, "textures/grass.jpg");
    myGround.Init(ground.vertices);
    myPlayer.Init(&myGround);

    palm.CreateFromOBJ("models/palm.obj", vec3 (0.19125f, 0.0735f, 0.0225f),vec3 (0.7038f, 0.27048f, 0.0828f),vec3 (0.256777f, 0.137622f, 0.086014f),12.8f);

    rock2.CreateFromOBJ("models/rock.obj",  vec3 (0.25f, 0.20725f, 0.20725f),vec3 (0.8f, 0.8f, 0.8f),vec3 (0.0f, 0.0f, 0.0f),11.264f,"textures/rock.jpg");

    flower2.CreateFromOBJ("models/plane2.obj", vec3 (0.25f, 0.20725f, 0.20725f),vec3 (0.8f, 0.8f, 0.8f),vec3 (0.0f, 0.0f, 0.0f),11.264f, "textures/flower2.png");
    flower1.CreateFromOBJ("models/plane2.obj", vec3 (0.25f, 0.20725f, 0.20725f),vec3 (0.8f, 0.8f, 0.8f),vec3 (0.0f, 0.0f, 0.0f),11.264f, "textures/flower1.png");
    flower4.CreateFromOBJ("models/plane2.obj", vec3 (0.25f, 0.20725f, 0.20725f),vec3 (0.8f, 0.8f, 0.8f),vec3 (0.0f, 0.0f, 0.0f),11.264f, "textures/flower4.png");
    flower3.CreateFromOBJ("models/plane2.obj", vec3 (0.25f, 0.20725f, 0.20725f),vec3 (0.8f, 0.8f, 0.8f),vec3 (0.0f, 0.0f, 0.0f),11.264f, "textures/flower3.png");
    flower5.CreateFromOBJ("models/plane2.obj", vec3 (0.25f, 0.20725f, 0.20725f),vec3 (0.8f, 0.8f, 0.8f),vec3 (0.0f, 0.0f, 0.0f),11.264f, "textures/flower5.png");
    flower6.CreateFromOBJ("models/plane2.obj", vec3 (0.25f, 0.20725f, 0.20725f),vec3 (0.8f, 0.8f, 0.8f),vec3 (0.0f, 0.0f, 0.0f),11.264f, "textures/flower6.png");
    flower7.CreateFromOBJ("models/plane2.obj", vec3 (0.25f, 0.20725f, 0.20725f),vec3 (0.8f, 0.8f, 0.8f),vec3 (0.0f, 0.0f, 0.0f),11.264f, "textures/flower7.png");

    GenerateXPosition();

    bees.CreateFromOBJ("models/bee.obj",  vec3 (0.25f, 0.20725f, 0.20725f),vec3 (10.0f, 10.0f, 10.0f),vec3 (0.0f, 0.0f, 0.0f),11.264f,"textures/bee.jpg");
    bees.CreateInstances(1,InstanceNumber,-80.0,80.0,0.0,15.0,-40.0,40.0,0.0,2.0,-10.0,10.0);

    grass.CreateFromOBJ("models/grass.obj",  vec3 (0.25f, 0.20725f, 0.20725f),vec3 (10.0f, 10.0f, 10.0f),vec3 (0.0f, 0.0f, 0.0f),2.264f, "textures/grass.jpg");



    sphere.CreateFromOBJ("models/sphere.obj", vec3 (0.2, 0.2, 0.2),vec3 (1.0, 1.0, 1.0),vec3 (1.0, 1.0, 1.0),128.0f);

    sphere2.CreateFromOBJ("models/sphere.obj", vec3 (0.2, 0.2, 0.2),vec3 (1.0, 1.0, 1.0),vec3 (1.0, 1.0, 1.0),128.0f);

    SkyBox_Program.CreateFromShaders("shaders/skybox-vertex.glsl", "shaders/skybox-fragment.glsl");

    SkyBox1.CreateSkyBox(filesSkyBox1);
    SkyBox2.CreateSkyBox(filesSkyBox2);

    MiniMap.Initialize("shaders/minimap-vertex.glsl", "shaders/minimap-fragment.glsl",256,256,0);
    Postprocessing1.Initialize("shaders/minimap-vertex.glsl", "shaders/minimap-fragment.glsl",WindowWidth,WindowHeight,1);
    Postprocessing2.Initialize("shaders/minimap-vertex.glsl", "shaders/minimap-fragment.glsl",WindowWidth,WindowHeight,2);

    // 4. Stworzenie oddzielnego/uproszczonego programu,
    // ktory bedzie generowal mape cieni

    DepthMap_idProgram.CreateFromShaders("shaders/depthmap-vertex.glsl", "shaders/depthmap-fragment.glsl");

    ShadowMapDir_Init();

    myCamera.Init(&myPlayer,glm::vec3(0.0, -3.0, 0.0));

    __CHECK_FOR_ERRORS
}

// ---------------------------------------------------
void Reshape( int width, int height )
{
    WindowWidth = width;
    WindowHeight = height;
    glViewport( 0, 0, WindowWidth, WindowHeight );
    matProj = glm::perspective(glm::radians(60.0f), width/(float)height, 0.1f, 100.0f);
    myCamera.UpdatePerspective(width,height);
}
void HandleKeyPress(unsigned char key, int x, int y)
{
    KeyboardTable[key] = true;
}

void HandleKeyRelease(unsigned char key, int x, int y)
{
    KeyboardTable[key] = false;
}
// ---------------------------------------------------
void Keyboard( )
{
    if (KeyboardTable['w'])
    {
        myPlayer.Move(0.06);
        myCamera.Update();
    }
    if (KeyboardTable['s'])
    {
        myPlayer.Move(-0.06);
        myCamera.Update();
    }
    if (KeyboardTable['d'])
    {
        myPlayer.Rotate(-0.015);
        myCamera.Update();
    }
    if (KeyboardTable['a'])
    {
        myPlayer.Rotate(0.015);
        myCamera.Update();
    }
    if (KeyboardTable[27])
    {
        glutLeaveMainLoop();
    }
    glutPostRedisplay();

}
void Menu( int value )
{
    switch ( value )
    {
    case 1: // Phonga
        lightModel = 1;
        printf("Model Phonga\n");
        break;
    case 2: // Blinna-Phonga
        lightModel = 2;
        printf("Model Blinna-Phonga\n");
        break;
    case 3:
        lightSwitch = 1;
        printf("Swiatlo wlaczone\n");
        break;
    case 4:
        lightSwitch = 0;
        printf("Swiatlo wylaczone\n");
        break;
    case 5:
        if(lightAnimation == 1)
            lightAnimation = 0;
        else
            lightAnimation = 1;
        break;
    case 6:
        lightType = 0;
        printf("Swiatlo punktowe\n");
        break;
    case 7:
        lightType = 1;
        printf("Swiatlo kierunkowe\n");
        break;
    case 8:
        currentSkyBox = 1;
        printf("SkyBox 1\n");
        break;
    case 9:
        currentSkyBox = 2;
        printf("SkyBox 2\n");
        break;
    case 10:
        if(RockReflection == 1)
            RockReflection = 0;
        else
            RockReflection = 1;
        printf("Odbicie na kamieniach\n");
        break;
    case 11:
        if(RockRefraction == 1)
            RockRefraction = 0;
        else
            RockRefraction = 1;
        printf("Refrakcja na kamieniach\n");
        break;
    case 12:
        if(showMiniMap == 1)
        {
            showMiniMap = 0;
            printf("Mini mapa off\n");

        }
        else
        {
            printf("Mini mapa on\n");
            showMiniMap = 1;
            postprocessing = 0;
        }
        break;
    case 13:
        postprocessing = 0;
        printf("postprocessing off\n");
        break;
    case 14:
        postprocessing = 1;
        showMiniMap = 0;
        printf("postprocessing 1\n");
        break;
    case 15:
        postprocessing = 2;
        showMiniMap = 0;
        printf("postprocessing 2\n");
        break;
    }
    glutPostRedisplay();
}
// ---------------------------------------------------

void Animation(int frame)
{
    Time += 0.01;
    frameCount++;

    calculateFPS();
    if(BeeDirection == 1)
    {
        DirectionTime += 1;
        BeeTime += 0.1;
    }
    else
    {
        DirectionTime -= 1;
        BeeTime -= 0.1;
    }
    if(DirectionTime == 400)
        BeeDirection = -1;
    if(DirectionTime == 0)
        BeeDirection = 1;

    if (!lightAnimation)
    {
        lightTime += 0.5;
        lights[0].Position.x = 0.5 * sin(0.05 * lightTime);
        lights[0].Position.y = 3.0 * sin(0.01 * lightTime) + 3.0;
        lights[0].Position.z = 2.0 * sin(0.03 * lightTime);
    }

    glutPostRedisplay();
    glutTimerFunc(1000/60, Animation, 0);
}

// ---------------------------------------------------
int main( int argc, char *argv[] )
{
    // GLUT
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    glutInitContextVersion( 3, 2 );
    glutInitContextProfile( GLUT_CORE_PROFILE );
    glutInitWindowSize( 1280, 720 );
    glutCreateWindow( "Projekt" );

    // Rejestracja funkcji obsługujących klawiaturę
    glutKeyboardFunc(HandleKeyPress);
    glutKeyboardUpFunc(HandleKeyRelease);

    glutDisplayFunc( DisplayScene );
    glutReshapeFunc( Reshape );
    glutMouseFunc( MouseButton );
    glutMotionFunc( MouseMotion );

    glutIdleFunc(Keyboard);

    glutSpecialFunc( SpecialKeys );

    // GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if( GLEW_OK != err )
    {
        printf("GLEW Error\n");
        exit(1);
    }

    // OpenGL
    if( !GLEW_VERSION_3_2 )
    {
        printf("Brak OpenGL 3.2!\n");
        exit(1);
    }

    int podmenuA = glutCreateMenu( Menu );
    glutAddMenuEntry( "Phonga", 1 );
    glutAddMenuEntry( "Blinna-Phonga", 2 );

    int podmenuB = glutCreateMenu( Menu );
    glutAddMenuEntry( "wlaczone", 3 );
    glutAddMenuEntry( "wylaczone", 4 );

    int podmenuC = glutCreateMenu( Menu );
    glutAddMenuEntry( "punktowe", 6 );
    glutAddMenuEntry( "kierunkowe", 7 );

    int podmenuD = glutCreateMenu( Menu );
    glutAddMenuEntry( "1", 8 );
    glutAddMenuEntry( "2", 9 );

    int podmenuE = glutCreateMenu( Menu );
    glutAddMenuEntry( "odbicie swiatla [on/off]", 10 );
    glutAddMenuEntry( "zalamanie swiatla [on/off]", 11 );

    int podmenuF = glutCreateMenu( Menu );
    glutAddMenuEntry( "off", 13 );
    glutAddMenuEntry( "1", 14 );
    glutAddMenuEntry( "2", 15 );


    glutCreateMenu( Menu );
    glutAddSubMenu( "Model swiatla odbiciowego", podmenuA );
    glutAddSubMenu( "Oswietlenie", podmenuB );
    glutAddMenuEntry( "Animacja swiatla [on/off]", 5 );
    glutAddSubMenu( "Oswietlenie", podmenuC );
    glutAddSubMenu( "SkyBox", podmenuD );
    glutAddSubMenu( "Opcje kamieni", podmenuE );
    glutAddMenuEntry( "mini mapa [on/off]", 12 );
    glutAddSubMenu( "Postprocessing", podmenuF );

    glutAttachMenu( GLUT_RIGHT_BUTTON );// GLUT_LEFT_BUTTON, GLUT_MIDDLE_BUTTON, GLUT_RIGHT_BUTTON

    Initialize();

    glutTimerFunc(1000/60, Animation, 0);
    glutMainLoop();


    return 0;
}
