#include <iostream>
#include <GL/glew.h>
#include <3dgl/3dgl.h>
#include <GL/glut.h>
#include <GL/freeglut_ext.h>

// Include GLM core features
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/random.hpp"

#pragma comment (lib, "glew32.lib")

using namespace std;
using namespace _3dgl;
using namespace glm;

// Wolf Position and Velocity
vec3 wolfPos = vec3(0, 0, 0);	// note: Y coordinate will be amended in run time
vec3 wolfVel = vec3(0, 0, 0);

// 3D Models
C3dglSkyBox skybox;
C3dglTerrain terrain;
C3dglModel wolf, tree, stone;
vec4 trees[500];

// Texture Ids
GLuint idTexTerrain;
GLuint idTexWolf;
GLuint idTexStone, idTexStoneNormal;
GLuint idTexNone;

// GLSL Objects (Shader Program)
C3dglProgram Program;

// The View Matrix
mat4 matrixView;

// Camera & navigatiobn
float maxspeed = 4.f;	// camera max speed
float accel = 4.f;		// camera acceleration
vec3 _acc(0), _vel(0);	// camera acceleration and velocity vectors
float _fov = 60.f;		// field of view (zoom)

int level = 10;

bool init()
{
	// rendering states
	glEnable(GL_DEPTH_TEST);	// depth test is necessary for most 3D scenes
	glEnable(GL_NORMALIZE);		// normalization is needed by AssImp library models
	glShadeModel(GL_SMOOTH);	// smooth shading mode is the default one; try GL_FLAT here!
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);	// this is the default one; try GL_LINE!

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5);

	// Initialise Shaders
	C3dglShader VertexShader;
	C3dglShader FragmentShader;

	if (!VertexShader.Create(GL_VERTEX_SHADER)) return false;
	if (!VertexShader.LoadFromFile("shaders/basic.vert")) return false;
	if (!VertexShader.Compile()) return false;

	if (!FragmentShader.Create(GL_FRAGMENT_SHADER)) return false;
	if (!FragmentShader.LoadFromFile("shaders/basic.frag")) return false;
	if (!FragmentShader.Compile()) return false;

	if (!Program.Create()) return false;
	if (!Program.Attach(VertexShader)) return false;
	if (!Program.Attach(FragmentShader)) return false;
	if (!Program.Link()) return false;
	if (!Program.Use(true)) return false;

	Program.SendUniform("level", level);

	// glut additional setup
	glutSetVertexAttribCoord3(Program.GetAttribLocation("aVertex"));
	glutSetVertexAttribNormal(Program.GetAttribLocation("aNormal"));

	// load your 3D models here!
	if (!terrain.loadHeightmap("models\\heightmap.png", 50)) return false;
	if (!wolf.load("models\\wolf.dae")) return false;
	wolf.loadAnimations();
	if (!tree.load("models\\tree\\tree.3ds")) return false;
	tree.loadMaterials("models\\tree");
	tree.getMaterial(0)->loadTexture(GL_TEXTURE1, "models\\tree", "pine-trunk-norm.dds");
	tree.getMaterial(1)->loadTexture(GL_TEXTURE1, "models\\tree", "pine-leaf-norm.dds");
	tree.getMaterial(2)->loadTexture(GL_TEXTURE1, "models\\tree", "pine-branch-norm.dds");
	if (!stone.load("models\\stone.obj")) return false;
	
	if (!skybox.load(
		"models\\mountain\\mft.tga",
		"models\\mountain\\mlf.tga",
		"models\\mountain\\mbk.tga",
		"models\\mountain\\mrt.tga",
		"models\\mountain\\mup.tga",
		"models\\mountain\\mdn.tga")) return false;

	for (vec4 & pos : trees)
	{
		float x = linearRand(-128.f, 128.f);
		float z = linearRand(-128.f, 128.f);
		pos = vec4(x, terrain.getInterpolatedHeight(x, z), z, linearRand(0.f, 6.28f));
	}

	// setup lights
	Program.SendUniform("lightDir.direction", -1.0f, 1.0f, 1.0f);
	Program.SendUniform("lightDir.diffuse", 1.0f, 1.0f, 1.0f);

	// load additional textures
	C3dglBitmap bm;
	glActiveTexture(GL_TEXTURE0);

	// Terrain texture
	bm.Load("models/grass.jpg", GL_RGBA);
	if (!bm.GetBits()) return false;
	glGenTextures(1, &idTexTerrain);
	glBindTexture(GL_TEXTURE_2D, idTexTerrain);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	// Wolf texture
	bm.Load("models/wolf.jpg", GL_RGBA);
	if (!bm.GetBits()) return false;
	glGenTextures(1, &idTexWolf);
	glBindTexture(GL_TEXTURE_2D, idTexWolf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	// Stone texture
	bm.Load("models/stone.png", GL_RGBA);
	if (!bm.GetBits()) return false;
	glGenTextures(1, &idTexStone);
	glBindTexture(GL_TEXTURE_2D, idTexStone);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	// Stone normal map
	bm.Load("models/stoneNormal.png", GL_RGBA);
	if (!bm.GetBits()) return false;
	glGenTextures(1, &idTexStoneNormal);
	glBindTexture(GL_TEXTURE_2D, idTexStoneNormal);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.GetWidth(), bm.GetHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.GetBits());

	// none (simple-white) texture
	glGenTextures(1, &idTexNone);
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	BYTE bytes[] = { 255, 255, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &bytes);

	Program.SendUniform("texture0", 0);
	Program.SendUniform("textureNormal", 1);

	// Initialise the View Matrix (initial position of the camera)
	matrixView = lookAt(
		vec3(-2.0, 1.0, 3.0),
		vec3(-2.0, 0.5, 0.0),
		vec3(0.0, 1.0, 0.0));

	// setup the screen background colour
	glClearColor(0.2f, 0.6f, 1.f, 1.0f);   // blue sky background

	cout << endl;
	cout << "Use:" << endl;
	cout << "  WASD or arrow key to navigate" << endl;
	cout << "  QE or PgUp/Dn to move the camera up and down" << endl;
	cout << "  Drag the mouse to look around" << endl;
	cout << endl;

	return true;
}

void done()
{
}

void renderScene(mat4& matrixView, float time, float deltaTime)
{
	mat4 m;

	// render skybox
	Program.SendUniform("materialDiffuse", 0.0f, 0.0f, 0.0f);	// white
	Program.SendUniform("materialAmbient", 1.0f, 1.0f, 1.0f);
	glActiveTexture(GL_TEXTURE0);

	m = matrixView;
	if (level >= 4) skybox.render(m);

	// setup materials for the terrain
	Program.SendUniform("materialDiffuse", 1.0f, 1.0f, 1.0f);	// white
	Program.SendUniform("materialAmbient", 0.1f, 0.1f, 0.1f);
	glBindTexture(GL_TEXTURE_2D, idTexTerrain);

	// render the terrain
	m = matrixView;
	terrain.render(m);

	// setup materials for the wolf
	Program.SendUniform("materialDiffuse", 1.0f, 1.0f, 1.0f);	// white background for textures
	Program.SendUniform("materialAmbient", 0.1f, 0.1f, 0.1f);
	glBindTexture(GL_TEXTURE_2D, idTexWolf);

	// Position of the moving wolf
	mat4 inv = inverse(translate(matrixView, vec3(-1, 0, 1)));
	vec3 myPos = vec3(inv[3].x, 0, inv[3].z);

	if (length(myPos - wolfPos) > 0.01f || level < 10)
	{
		if (level >= 6) wolfVel = normalize(myPos - wolfPos) * 0.01f;
		wolfPos = wolfPos + wolfVel;

		// calculate the animation time
		if (level < 9) time = 0;
		// calculate and send bone transforms
		std::vector<mat4> transforms;
		wolf.getAnimData(0, time * 1.45f, transforms);// choose animation cycle & speed of animation
		Program.SendUniformMatrixv("bones", (float*)&transforms[0], (GLuint)(transforms.size()));// amount of vertexes 
	}

	// render the wolf
	m = matrixView;
	vec3 amendY = vec3(vec3(0, terrain.getInterpolatedHeight(wolfPos.x, wolfPos.z), 0));
	m = translate(m, wolfPos + amendY);
	if (level >= 7) m = rotate(m, atan2(wolfVel.z, -wolfVel.x) - half_pi<float>(), vec3(0, 1, 0));
	wolf.render(m);

	// render the stone
	glBindTexture(GL_TEXTURE_2D, idTexStone);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, idTexStoneNormal);
	Program.SendUniform("bNormalMap", level >= 8);
	m = matrixView;
	m = translate(m, vec3(-3, terrain.getInterpolatedHeight(-3, -1), -1));
	m = scale(m, vec3(0.01f, 0.01f, 0.01f));
	if (level >= 1) stone.render(m);
	Program.SendUniform("bNormalMap", false);

	// render the trees
	Program.SendUniform("bNormalMap", level >= 8);
	for (vec4 pos : trees)
	{
		m = translate(matrixView, vec3(pos));
		m = scale(m, vec3(0.01f, 0.01f, 0.01f));
		m = rotate(m, pos.w, vec3(0.f, 1.f, 0.f));
		tree.render(m);
	}

	m = translate(matrixView, vec3(0, terrain.getInterpolatedHeight(0, -2), -2));
	m = scale(m, vec3(0.01f, 0.01f, 0.01f));
	tree.render(m);
	m = translate(matrixView, vec3(-5, terrain.getInterpolatedHeight(-5, -1), -1));
	m = scale(m, vec3(0.01f, 0.01f, 0.01f));
	m = rotate(m, radians(150.f), vec3(0.f, 1.f, 0.f));
	tree.render(m);
	m = translate(matrixView, vec3(-4, terrain.getInterpolatedHeight(-4, -4), -4));
	m = scale(m, vec3(0.01f, 0.01f, 0.01f));
	m = rotate(m, radians(70.f), vec3(0.f, 1.f, 0.f));
	tree.render(m);
	Program.SendUniform("bNormalMap", false);
}

void onRender()
{
	// these variables control time & animation
	static float prev = 0;
	float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;	// time since start in secs
	float deltaTime = time - prev;						// time since last frame
	prev = time;										// framerate is 1/deltaTime

	// setup the View Matrix (camera)
	_vel = clamp(_vel + _acc * deltaTime, -vec3(maxspeed), vec3(maxspeed));
	float pitch = getPitch(matrixView);
	matrixView = rotate(translate(rotate(mat4(1),
		pitch, vec3(1, 0, 0)),	// switch tilt off
		_vel * deltaTime),	// animate camera motion (controlled by WASD keys)
		-pitch, vec3(1, 0, 0))	// switch tilt on
		* matrixView;

	// clear screen and buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// move the camera up following the profile of terrain (Y coordinate of the terrain)
	float terrainY = -terrain.getInterpolatedHeight(inverse(matrixView)[3][0], inverse(matrixView)[3][2]);
	matrixView = translate(matrixView, vec3(0, terrainY, 0));

	// setup View Matrix
	Program.SendUniform("matrixView", matrixView);

	// render the scene objects
	renderScene(matrixView, time, deltaTime);

	// the camera must be moved down by terrainY to avoid unwanted effects
	matrixView = translate(matrixView, vec3(0, -terrainY, 0));

	// essential for double-buffering technique
	glutSwapBuffers();

	// proceed the animation
	glutPostRedisplay();
}

// called before window opened or resized - to setup the Projection Matrix
void onReshape(int w, int h)
{
	float ratio = w * 1.0f / h;      // we hope that h is not zero
	glViewport(0, 0, w, h);
	Program.SendUniform("matrixProjection", perspective(radians(_fov), ratio, 0.02f, 1000.f));
}

// Handle WASDQE keys
void onKeyDown(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w': _acc.z = accel; break;
	case 's': _acc.z = -accel; break;
	case 'a': _acc.x = accel; break;
	case 'd': _acc.x = -accel; break;
	case 'e': _acc.y = accel; break;
	case 'q': _acc.y = -accel; break;
	
	case '`': level = 0; Program.SendUniform("level", level); break;
	case '1': level = 1; Program.SendUniform("level", level); break;
	case '2': level = 2; Program.SendUniform("level", level); break;
	case '3': level = 3; Program.SendUniform("level", level); break;
	case '4': level = 4; Program.SendUniform("level", level); break;
	case '5': level = 5; Program.SendUniform("level", level); break;
	case '6': level = 6; Program.SendUniform("level", level); break;
	case '7': level = 7; Program.SendUniform("level", level); break;
	case '8': level = 8; Program.SendUniform("level", level); break;
	case '9': level = 9; Program.SendUniform("level", level); break;
	case '0': level = 10; Program.SendUniform("level", level); break;
	}
}

// Handle WASDQE keys (key up)
void onKeyUp(unsigned char key, int x, int y)
{
	switch (tolower(key))
	{
	case 'w':
	case 's': _acc.z = _vel.z = 0; break;
	case 'a':
	case 'd': _acc.x = _vel.x = 0; break;
	case 'q':
	case 'e': _acc.y = _vel.y = 0; break;
	}
}

// Handle arrow keys and Alt+F4
void onSpecDown(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_F4:		if ((glutGetModifiers() & GLUT_ACTIVE_ALT) != 0) exit(0); break;
	case GLUT_KEY_UP:		onKeyDown('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyDown('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyDown('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyDown('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyDown('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyDown('e', x, y); break;
	case GLUT_KEY_F11:		glutFullScreenToggle();
	}
}

// Handle arrow keys (key up)
void onSpecUp(int key, int x, int y)
{
	maxspeed = glutGetModifiers() & GLUT_ACTIVE_SHIFT ? 20.f : 4.f;
	switch (key)
	{
	case GLUT_KEY_UP:		onKeyUp('w', x, y); break;
	case GLUT_KEY_DOWN:		onKeyUp('s', x, y); break;
	case GLUT_KEY_LEFT:		onKeyUp('a', x, y); break;
	case GLUT_KEY_RIGHT:	onKeyUp('d', x, y); break;
	case GLUT_KEY_PAGE_UP:	onKeyUp('q', x, y); break;
	case GLUT_KEY_PAGE_DOWN:onKeyUp('e', x, y); break;
	}
}

// Handle mouse click
void onMouse(int button, int state, int x, int y)
{
	glutSetCursor(state == GLUT_DOWN ? GLUT_CURSOR_CROSSHAIR : GLUT_CURSOR_INHERIT);
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);
	if (button == 1)
	{
		_fov = 60.0f;
		onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
	}
}

// handle mouse move
void onMotion(int x, int y)
{
	glutWarpPointer(glutGet(GLUT_WINDOW_WIDTH) / 2, glutGet(GLUT_WINDOW_HEIGHT) / 2);

	// find delta (change to) pan & tilt
	float deltaYaw = 0.005f * (x - glutGet(GLUT_WINDOW_WIDTH) / 2);
	float deltaPitch = 0.005f * (y - glutGet(GLUT_WINDOW_HEIGHT) / 2);

	if (abs(deltaYaw) > 0.3f || abs(deltaPitch) > 0.3f)
		return;	// avoid warping side-effects

	// View = Tilt * DeltaTilt * DeltaPan * Tilt^-1 * View;
	float pitch = getPitch(matrixView);
	matrixView = rotate(rotate(rotate(mat4(1.f),
		pitch + deltaPitch, vec3(1.f, 0.f, 0.f)), 
		deltaYaw, vec3(0.f, 1.f, 0.f)), 
		-pitch, vec3(1.f, 0.f, 0.f)) 
		* matrixView;
}

void onMouseWheel(int button, int dir, int x, int y)
{
	_fov = glm::clamp(_fov - dir * 5.f, 5.0f, 175.f);
	onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

template<typename ... Args> auto to_string(std::string fmt, Args ... args)
{
	return std::vformat(fmt, std::make_format_args(args ...));
}


int main(int argc, char **argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("CI5520 3D Graphics Programming");

	// init glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		cerr << "GLEW Error: " << glewGetErrorString(err) << endl;
		return 0;
	}
	cout << "Using GLEW " << glewGetString(GLEW_VERSION) << endl;

	// register callbacks
	glutDisplayFunc(onRender);
	glutReshapeFunc(onReshape);
	glutKeyboardFunc(onKeyDown);
	glutSpecialFunc(onSpecDown);
	glutKeyboardUpFunc(onKeyUp);
	glutSpecialUpFunc(onSpecUp);
	glutMouseFunc(onMouse);
	glutMotionFunc(onMotion);
	glutMouseWheelFunc(onMouseWheel);

	cout << "Vendor: " << glGetString(GL_VENDOR) << endl;
	cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Version: " << glGetString(GL_VERSION) << endl << endl;

	// init light and everything – not a GLUT or callback function!
	if (!init())
	{
		cerr << "Application failed to initialise" << endl << endl;
		return 0;
	}

	// enter GLUT event processing cycle
	glutMainLoop();

	done();

	return 1;
}

