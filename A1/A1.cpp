// Spring 2019

#include "A1.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>

#include <sys/types.h>
#include <unistd.h>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
using namespace std;

static const size_t DIM = 16;

//----------------------------------------------------------------------------------------
// Constructor
A1::A1()
	: current_col( 0 ),
    m_cube_height(1.0f),
    m_cube_color(glm::vec3(1.0f, 1.0f, 1.0f)),
    m_tile_color(glm::vec3(0.0f, 0.0f, 0.0f))
{
	colour[0] = 0.0f;
	colour[1] = 0.0f;
	colour[2] = 0.0f;
    m = new Maze(DIM);
}

//----------------------------------------------------------------------------------------
// Destructor
A1::~A1()
{}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A1::init()
{
	// Initialize random number generator
	int rseed=getpid();
	srandom(rseed);
	// Print random number seed in case we want to rerun with
	// same random numbers
	cout << "Random number seed = " << rseed << endl;
	
	// Set the background colour.
	glClearColor( 0.3, 0.5, 0.7, 1.0 );

	// Build the shaders
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(
		getAssetFilePath( "VertexShader.vs" ).c_str() );
	m_shader.attachFragmentShader(
		getAssetFilePath( "FragmentShader.fs" ).c_str() );
	m_shader.link();

	// Set up the uniforms
	P_uni = m_shader.getUniformLocation( "P" );
	V_uni = m_shader.getUniformLocation( "V" );
	M_uni = m_shader.getUniformLocation( "M" );
	//col_uni = m_shader.getUniformLocation( "colour" );

    initMaze();

	//initGrid();

	// Set up initial view and projection matrices (need to do this here,
	// since it depends on the GLFW window being set up correctly).
	view = glm::lookAt( 
		glm::vec3( 0.0f, 2.*float(DIM)*2.0*M_SQRT1_2, float(DIM)*2.0*M_SQRT1_2 ),
		glm::vec3( 0.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 1.0f, 0.0f ) );

	proj = glm::perspective( 
		glm::radians( 30.0f ),
		float( m_framebufferWidth ) / float( m_framebufferHeight ),
		1.0f, 1000.0f );
}

void A1::initMaze() {
	
	m->digMaze();
	m->printMaze();
    
	// allocate space for VAO
    glGenVertexArrays( 1, &m_maze_vao );
	
    glBindVertexArray( m_maze_vao );
	
    GLint positionAttribLocation = m_shader.getAttribLocation("position");
    glEnableVertexAttribArray(positionAttribLocation);
    
    GLint colorAttribLocation = m_shader.getAttribLocation("color");
    glEnableVertexAttribArray(colorAttribLocation);

    glBindVertexArray( 0 );
     
    initTiles();
}

bool A1::isOutOfRange(int i, int j) {
    return (i < 0 || i >= DIM || j < 0 || j >= DIM);
}

void A1::buildCubeIndices(int i, int j, int index, vec3 *array) {
    
    float height = m_cube_height;
    if(!isOutOfRange(i, j) && m->getValue(i, j) == 0) {
        height = 0.0f;
    }
    
    vec3 zero = vec3(j, 0, i);
    vec3 one = vec3(j, 0, i+1);
    vec3 two = vec3(j+1, 0, i);
    vec3 three = vec3(j+1, 0, i+1);
    
    vec3 four = vec3(j, height, i);
    vec3 five = vec3(j, height, i+1);
    vec3 six = vec3(j+1, height, i);
    vec3 seven = vec3(j+1, height, i+1);
    /*

    vec3 zero = vec3(i, 0, j);
    vec3 one = vec3(i, 0, j+1);
    vec3 two = vec3(i+1, 0, j);
    vec3 three = vec3(i+1, 0, j+1);
    
    vec3 four = vec3(i, height, j);
    vec3 five = vec3(i, height, j+1);
    vec3 six = vec3(i+1, height, j);
    vec3 seven = vec3(i+1, height, j+1);
    */

    vec3 cubeMapping[] = {
        one,    two,    zero,      // Triangle 0
        one,    two,    three,      // Triangle 1
        five,   six,    four,      // Triangle 2
        five,   six,    seven,      // Triangle 3
        six,    zero,   four,      // Triangle 4
        six,    zero,   two,      // Triangle 5
        seven,  two,    six,      // Triangle 6
        seven,  two,    three,      // Triangle 7
        five,   three,  seven,      // Triangle 8
        five,   three,  one,      // Triangle 9
        four,   one,    five,      // Triangle 10
        four,   one,    zero,      // Triangle 11
    };
    
    for(int it = 0; it < 36; it++) {
        array[index+it] = cubeMapping[it];
    }
}

void A1::buildCubeColorIndicies(int i, int j, int index, vec3* array) {
    vec3 color;
    if(isOutOfRange(i, j) || m->getValue(i, j) == 1) {
        color = vec3(m_cube_color.r, m_cube_color.g, m_cube_color.b);
    } else {
        color = vec3(m_tile_color.r, m_tile_color.g, m_tile_color.b);
    }
    for(int it = 0; it < 36; it++) {
        array[index+it] = color;
    }
}

void A1::initTiles() {
	
    size_t numVerts = (DIM+2) * (DIM+2) * 6 * 2 * 3; // (DIM+2)^2 "cubes", 6 faces per square, 2 triangles per face, 3 verts per triangle

	vec3 *verts = new vec3[ numVerts ];
    vec3 *vertColors = new vec3[ numVerts ];
   
    size_t vertCount = 0;
    for(int i = 0; i < DIM+2; i++) {
        for(int j = 0; j < DIM+2; j++) {
            buildCubeIndices(i-1, j-1, vertCount, verts); 
            buildCubeColorIndicies(i-1, j-1, vertCount, vertColors);
            vertCount += 36; 
        }
    }

	glBindVertexArray( m_maze_vao );
    
    glGenBuffers( 1, &m_tile_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_tile_vbo );
	glBufferData( GL_ARRAY_BUFFER, numVerts*sizeof(vec3),
		verts, GL_DYNAMIC_DRAW );  

    GLint positionAttribLocation = m_shader.getAttribLocation("position");
	glVertexAttribPointer(positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    
    
    glGenBuffers( 1, &m_colors_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_colors_vbo );
	glBufferData( GL_ARRAY_BUFFER, numVerts*sizeof(vec3),
		vertColors, GL_DYNAMIC_DRAW ); 

    GLint colorAttribLocation = m_shader.getAttribLocation("color");
	glVertexAttribPointer(colorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindVertexArray(0);
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	
    delete[] verts;
    delete[] vertColors;
    
    CHECK_GL_ERRORS;
}

/*
void A1::initGrid()
{
	size_t sz = 3 * 2 * 2 * (DIM+3);

	float *verts = new float[ sz ];
	size_t ct = 0;
	for( int idx = 0; idx < DIM+3; ++idx ) {
        verts[ ct ] = -1;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = idx-1;
		verts[ ct+3 ] = DIM+1;
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = idx-1;
		ct += 6;

		verts[ ct ] = idx-1;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = -1;
		verts[ ct+3 ] = idx-1;
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = DIM+1;
        ct += 6;
  
    }

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays( 1, &m_maze_vao );
	glBindVertexArray( m_maze_vao );

	// Create the cube vertex buffer
	glGenBuffers( 1, &m_grid_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_grid_vbo );
	glBufferData( GL_ARRAY_BUFFER, sz*sizeof(float),
		verts, GL_STATIC_DRAW );

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation( "position" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

	// Reset state to prevent rogue code from messing with *my* 
	// stuff!
	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	// OpenGL has the buffer now, there's no need for us to keep a copy.
	delete [] verts;

	CHECK_GL_ERRORS;
}
*/

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A1::appLogic()
{
	// Place per frame, application logic here ...
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A1::guiLogic()
{
	// We already know there's only going to be one window, so for 
	// simplicity we'll store button states in static local variables.
	// If there was ever a possibility of having multiple instances of
	// A1 running simultaneously, this would break; you'd want to make
	// this into instance fields of A1.
	static bool showTestWindow(false);
	static bool showDebugWindow(true);

	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Debug Window", &showDebugWindow, ImVec2(100,100), opacity, windowFlags);
		if( ImGui::Button( "Quit Application" ) ) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}

		// Eventually you'll create multiple colour widgets with
		// radio buttons.  If you use PushID/PopID to give them all
		// unique IDs, then ImGui will be able to keep them separate.
		// This is unnecessary with a single colour selector and
		// radio button, but I'm leaving it in as an example.

		// Prefixing a widget name with "##" keeps it from being
		// displayed.


		ImGui::PushID( 0 );

		ImGui::ColorEdit3( "Colour", colour );
		ImGui::SameLine();
		if( ImGui::RadioButton( "Col", &current_col, 0 ) ) {
			// Select this colour.
		}
		ImGui::PopID();

/*
		// For convenience, you can uncomment this to show ImGui's massive
		// demonstration window right in your application.  Very handy for
		// browsing around to get the widget you want.  Then look in 
		// shared/imgui/imgui_demo.cpp to see how it's done.
		if( ImGui::Button( "Test Window" ) ) {
			showTestWindow = !showTestWindow;
		}
*/

		ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );

	ImGui::End();

	if( showTestWindow ) {
		ImGui::ShowTestWindow( &showTestWindow );
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A1::draw()
{
	// Create a global transformation for the model (centre it).
	mat4 W;
	W = glm::translate( W, vec3( -float(DIM)/2.0f, 0, -float(DIM)/2.0f ) );

	m_shader.enable();
		glEnable( GL_DEPTH_TEST );

		glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( proj ) );
		glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( view ) );
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );

		// Just draw the grid for now.
		glBindVertexArray( m_maze_vao );
		//glUniform3f( col_uni, 1, 1, 1 );
       
        const GLsizei numIndices = (DIM+2) * (DIM+2) * 6 * 2 * 3;
		glDrawArrays(GL_TRIANGLES, 0, numIndices);
		// Draw the cubes
		// Highlight the active square.
	m_shader.disable();

	// Restore defaults
	glBindVertexArray( 0 );

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A1::cleanup()
{
    delete m;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A1::cursorEnterWindowEvent (
		int entered
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool A1::mouseMoveEvent(double xPos, double yPos) 
{
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// Put some code here to handle rotations.  Probably need to
		// check whether we're *dragging*, not just moving the mouse.
		// Probably need some instance variables to track the current
		// rotation amount, and maybe the previous X position (so 
		// that you can rotate relative to the *change* in X.
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A1::mouseButtonInputEvent(int button, int actions, int mods) {
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// The user clicked in the window.  If it's the left
		// mouse button, initiate a rotation.
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A1::mouseScrollEvent(double xOffSet, double yOffSet) {
	bool eventHandled(false);

	// Zoom in or out.

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A1::windowResizeEvent(int width, int height) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A1::keyInputEvent(int key, int action, int mods) {
	bool eventHandled(false);

	// Fill in with event handling code...
	if( action == GLFW_PRESS ) {
		// Respond to some key events.
	}

	return eventHandled;
}
