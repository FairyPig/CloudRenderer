// Module responsible for drawing stuff

#ifndef RENDERERMODULE_H
#define RENDERERMODULE_H

#include "SimulationData.h"
#include "Camera.h"

class RendererModule {
public:
	RendererModule();
	bool initialize( int, int, int );
	void terminate();
	void draw( SimulationData*, GLFWmutex, double );
	inline void changeShowSplat() { showSplat = !showSplat; };
	inline void changeShowVRC() { showVRC = !showVRC; };
private:
	void renderSplattingClouds( SimulationData*, double );
	void renderRayCastingClouds( SimulationData*, double );
	void defineBillboardLayout( GLuint ); 
	void defineRaycasterLayout( GLuint );

	bool showSplat;
	bool showVRC;

	Camera camera;
	glm::mat4 perspectiveProjection;
	glm::mat4 orthographicProjection;
	glm::vec3 sunPosition;
	glm::mat4 sunTransformation;
};

#endif