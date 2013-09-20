#include "stdafx.h"

#include "RenderManager.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "Cube.h"
#include "Globals.h"
#include "RenderUtility.h"
#include "Slider.h"

RenderManager::RenderManager() :
	windowTitle( "Real-timeish Cloud Renderer" ),
	nearPlane( 0.1f ), 
	farPlane( 25.0f ),
	fieldOfView( 75.0f ),
	tanFOV( tan( fieldOfView / 2.0f / 360.0f * 2.0f * 3.14f )),
	showVRC( true ) {};

bool RenderManager::initialize( const int gridX, const int gridY, 
								 const int gridZ ) {

	// Initialize GLFW and opens a window
	if( glfwInit() != GL_TRUE ) {
		return false;
	}

	/*glfwOpenWindowHint( GLFW_OPENGL_VERSION_MAJOR, 3 );
	glfwOpenWindowHint( GLFW_OPENGL_VERSION_MINOR, 2 );
	glfwOpenWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );*/
	glfwOpenWindowHint( GLFW_WINDOW_NO_RESIZE, GL_TRUE );

	if (glfwOpenWindow( global_consts::windowWidth, global_consts::windowHeight,
		0, 0, 0, 0, 24, 8, 
		GLFW_WINDOW ) != GL_TRUE) {
			glfwTerminate();
			return false;
	}
	
	glfwSetWindowTitle( windowTitle );

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if(err != GLEW_OK) {
		std::cerr << "Failed to initialize GLEW: " << 
			glewGetErrorString(err) << "\n";
		return false;
	}
	// Ignore the OpenGL error (1280) that glewInit() causes
	glGetError();

	std::cout << "Running OpenGL version " << glGetString(GL_VERSION) << "\n";

	
	// Load and compile shaders
	raycasterShaderProgram = shaderManager.createFromFile( 
		"RaycasterShader.vert", "RaycasterShader.frag" );
	guiShaderProgram = shaderManager.createFromFile( 
		"GUIshader.vert", "GUIShader.frag" );
	
	initializeTextures( volumeTexture, planarTextures );
	
	glGenVertexArrays( 2, VAOs );

	// Define the raycasting VAO
	glBindVertexArray( VAOs[0] );

	// Create cube that encapsulates the grid for ray casting
	float cubeVertices[24];
	getCubeVertices( 0, 1, 0, 1, 0, 1, cubeVertices );

	cubeVBO = createVBO( cubeVertices, sizeof( cubeVertices )) ;
	defineRaycasterLayout( raycasterShaderProgram );

	int cubeElements[36];
	getCubeElements( cubeElements );
	createEBO( cubeElements, sizeof( cubeElements ));
	
	//Define the GUI VAO
	glBindVertexArray( VAOs[1] );
	glBindTexture( GL_TEXTURE_2D, planarTextures[0] );

	float vertices[] = {
    //   Pos, Texcoords
	 	 0.0f, 0.0f, // Top-left
		 1.0f, 0.0f, // Top-right
		 1.0f, 1.0f, // Bottom-right
		 0.0f, 1.0f  // Bottom-left
	};

	createVBO( vertices, sizeof( vertices ) );

	// Create an element array
	GLuint ebo;
	glGenBuffers( 1, &ebo );

	GLuint elements[] = {
		0, 1, 2,
		2, 3, 0
	};

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( elements ), elements, GL_STATIC_DRAW );

	defineGUILayout( guiShaderProgram );

	// Initialize the camera and the projetion matrices
	camera.initialize( gridX, gridY, gridZ );
	perspectiveProjection = glm::perspective( fieldOfView, 
		(float)global_consts::windowWidth / (float)global_consts::windowHeight, 
		nearPlane, farPlane );
		
	interpolatedData = new float ** [gridX];
	for( int i = 0; i != gridX; ++i ) {
		interpolatedData[i] = new float*[gridY];
		for (int j = 0; j != gridY; ++j ) 
			interpolatedData[i][j] = new float[gridZ];
	}

	// Initialize the sliders
	controls.addSlider( "Density Cutoff", "densityCutoff", 0.0f, 0.2f, 0.06f );
	controls.addSlider( "Density Factor", "densityFactor", 0.0f, 1.0f, 0.35f );
	controls.addSlider( "Color Multiplier", "colorMultiplier", 1.0f, 10.0f, 5.0f );
	controls.addSlider( "Attenuation Factor", "attenuationFactor", 0.0f, 0.5f, 0.05f );
	controls.addSlider( "Light ColorRed", "lightColorRed", 0.0f, 1.0f, 1.0f );
	controls.addSlider( "Light ColorGreen", "lightColorGreen", 0.0f, 1.0f, 1.0f );
	controls.addSlider( "Light ColorBlue", "lightColorBlue", 0.0f, 1.0f, 1.0f );
	controls.addSlider( "Shade ColorRed", "shadeColorRed", 0.0f, 1.0f, 0.0f );
	controls.addSlider( "Shade ColorGreen", "shadeColorGreen", 0.0f, 1.0f, 0.0f );
	controls.addSlider( "Shade ColorBlue", "shadeColorBlue", 0.0f, 1.0f, 0.2f );
	controls.addSlider( "Sun PositionX", "sunPositionX", -1.0f, 1.0f, 0.5f );
	controls.addSlider( "Sun PositionY", "sunPositionY", -1.0f, 1.0f, 0.5f );
	controls.addSlider( "Sun PositionZ", "sunPositionZ", -1.0f, 1.0f, 0.5f );
	controls.addSlider( "View Samples", "viewSamplesF", 1.0f, 1024.0f, 128.0f );
	controls.addSlider( "Light Samples", "lightSamplesF", 1.0f, 64.0f, 64.0f );

	return true;

}

void RenderManager::defineRaycasterLayout( const GLuint raycasterShaderProgram ) {
	
	GLint posAttrib = glGetAttribLocation( raycasterShaderProgram, 
		"cubeVert" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 
		3*sizeof(float), 0 );

}

void RenderManager::defineGUILayout( const GLuint guiShaderProgram ) {

	GLint texAttrib = glGetAttribLocation( guiShaderProgram, "coord" );
	glEnableVertexAttribArray( texAttrib );
	glVertexAttribPointer( texAttrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof( float ), 
		0 );

}

void RenderManager::draw( const SimulationData& data, GLFWmutex simMutex, 
						   const double time ) {
	// Update the camera
	camera.updateCamera();

	// Clear the screen with background (sky) color
	glClearColor( 155/256.0f, 225/256.0f, 251/256.0f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	// Lock mutex because we will use data, which is shared with simulation
	glfwLockMutex( simMutex );

	interpolateCloudData( data, time );
	
	if( showVRC )
		renderRayCastingClouds( data, time );
	
	glfwUnlockMutex( simMutex );

	glUseProgram( raycasterShaderProgram );
	controls.update();
	
	renderGUI();

	// Check for errors
	GLint glErr = glGetError();
	if ( glErr ) std::cout << "OpenGL error " << glErr << "!\n";

	// Swap the buffer
	glfwSwapBuffers();

}

void RenderManager::interpolateCloudData( const SimulationData & data,  
										  const double time ) {
	
	int x = data.getGridLength();
	int y = data.getGridWidth();
	int z = data.getGridHeight();

	// Calculate relative difference for linear interpolation
	float relDiff = (time - data.nextTime)/(data.nextTime - data.prevTime);
	if( relDiff > 1.0f ) relDiff = 1.0f;

	for( int i = 0; i < x; ++i ) 
		for( int j = 0; j < y; ++j ) 
			for( int k = 0; k < z; ++k )
				if( data.prevDen[i][j][k] > 0.0f )
					// Lineary interpolate the density
					interpolatedData[i][j][k] = data.prevDen[i][j][k] + relDiff
						* (data.nextDen[i][j][k] - data.prevDen[i][j][k] );
				else
					interpolatedData[i][j][k] = 0.0f;
}

// Shade clouds by performing volume ray casting
void RenderManager::renderRayCastingClouds( const SimulationData & data, 
											const double time ) {

	glBindVertexArray( VAOs[0] );
	glUseProgram( raycasterShaderProgram );
	setUniform( "view", camera.getLookAtMatrix() );
	setUniform( "viewInverse", glm::inverse(camera.getLookAtMatrix()) );
	setUniform( "proj", perspectiveProjection );
	setUniform( "tanFOV", tanFOV );
	setUniform( "screenSize", glm::vec2( global_consts::windowWidth, 
		global_consts::windowHeight ) );
	setUniform( "eyePosition", camera.getEyeLocation() );
	setUniform( "near", nearPlane );
	setUniform( "far", farPlane );

	glDisable( GL_CULL_FACE );
	//glEnable( GL_DEPTH_TEST );

	int x = data.getGridLength();
	int y = data.getGridWidth();
	int z = data.getGridHeight();

	// Convert float*** to float* (stream of data)
	float* texData = new float[x*y*z];
	int pos = 0;
	for( int i = 0; i != x; ++i )
		for( int j = 0; j != y; ++j ) 
			for( int k = 0; k != z; ++k ) {
				texData[pos] = interpolatedData[i][j][k];
				++pos;
			}

	// Fill the data into 3D texture. A texture cell includes only one
	// component (GL_RED = density, float). 
	glTexImage3D( GL_TEXTURE_3D, 0, GL_R32F, x, y, z, 0, GL_RED, 
		GL_FLOAT, texData );

	delete[] texData;

	glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0 );

} 

void RenderManager::renderGUI() {

	glBindVertexArray( VAOs[1] );
	glUseProgram( guiShaderProgram );
	
	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA );
	
	// Render the controls in orthographic mode
	controls.render( planarTextures );

	glDisable( GL_BLEND );
	glDepthMask( GL_TRUE );

}

void RenderManager::terminate() {

	shaderManager.terminate();
	glDeleteVertexArrays( 2, VAOs );
	deleteTextures( volumeTexture, planarTextures );

	// Terminate GLFW
	glfwTerminate();

}