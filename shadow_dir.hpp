// -------------------------------------------------
// Programowanie grafiki 3D w OpenGL / UG
// -------------------------------------------------
#ifndef __SHADOW_DIR_H
#define __SHADOW_DIR_H

// References:
// - https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
// - Janusz Ganczarski, OpenGL. Podstawy programowania grafiki 3D, Helion.


// Identyfikatory obiektow
//GLuint DepthMap_idProgram;
GLuint DepthMap_idFrameBuffer;
GLuint DepthMap_idTexture;

// Rozdzielczosc depth mapy
const unsigned int DepthMap_Width = 1024;
const unsigned int DepthMap_Height = 1024;


// Parametry swiatla kierunkowego, ale znacznie lepiej ubrac
// to w strukture, ktora uzywalismy podczas zajec z oswietlenia
glm::vec3 Light_Direction;
glm::vec3 Light_Position;

// Macierze rzutowania dla kamery patrzacej z punktu widzenia oswietlenia
glm::mat4 lightProj;
glm::mat4 lightView;

// Pozycja kamery potrzebna do oswietlenia
glm::vec3 cameraPos;



// -------------------------------------------------------
void ShadowMapDir_Init()
{
	// Ustawienie polozenia i kierunku swiatla
	Light_Direction = glm::normalize(glm::vec3(-1.0, -0.8, -0.4));
	Light_Position = glm::vec3(30.0f, 30.0f, 0.0f);

	// Ustawienie macierzy kamery swiatla
	lightProj = glm::ortho(-65.0f, 40.0f, -40.0f, 40.0f, 0.1f, 80.0f);
	lightView = glm::lookAt(Light_Position, Light_Position + Light_Direction, glm::vec3(0.0f, 1.0f, 0.0f));

	// 1. Stworzenie obiektu tekstury
	glGenTextures(1, &DepthMap_idTexture);
	glBindTexture(GL_TEXTURE_2D, DepthMap_idTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, DepthMap_Width, DepthMap_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// 2. Stworzenie obiektu ramki bufora jedynie z buforem glebokosci
	// (skladowa koloru nie jest nam potrzebna)
	glGenFramebuffers(1, &DepthMap_idFrameBuffer);

	// 3. Dolaczanie tekstury do ramki bufora
	glBindFramebuffer(GL_FRAMEBUFFER, DepthMap_idFrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, DepthMap_idTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



#endif // __SHADOW_DIR_H
