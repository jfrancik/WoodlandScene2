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
const size_t TREES = 2000;

// Texture Ids
GLuint idTexTerrain;
GLuint idTexWolf;
GLuint idTexStone, idTexStoneNormal;
GLuint idTexNone;

// GLSL Objects (Shader Program)
C3dglProgram program;

// The View Matrix
mat4 matrixView;

// Camera & navigation
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

	if (!VertexShader.create(GL_VERTEX_SHADER)) return false;
	if (!VertexShader.loadFromFile("shaders/basic.vert")) return false;
	if (!VertexShader.compile()) return false;

	if (!FragmentShader.create(GL_FRAGMENT_SHADER)) return false;
	if (!FragmentShader.loadFromFile("shaders/basic.frag")) return false;
	if (!FragmentShader.compile()) return false;

	if (!program.create()) return false;
	if (!program.attach(VertexShader)) return false;
	if (!program.attach(FragmentShader)) return false;
	if (!program.link()) return false;
	if (!program.use(true)) return false;

	program.sendUniform("level", level);

	// glut additional setup
	glutSetVertexAttribCoord3(program.getAttribLocation("aVertex"));
	glutSetVertexAttribNormal(program.getAttribLocation("aNormal"));

	// load your 3D models here!
	if (!terrain.load("models\\heightmap.png", 50)) return false;
	if (!wolf.load("models\\wolf.dae")) return false;
	wolf.loadAnimations();

	if (!stone.load("models\\stone.obj")) return false;

	if (!tree.load("models\\tree\\tree.3ds")) return false;
	tree.loadMaterials("models\\tree");
	tree.getMaterial(0)->loadTexture(GL_TEXTURE1, "models\\tree", "pine-trunk-norm.dds");
	tree.getMaterial(1)->loadTexture(GL_TEXTURE1, "models\\tree", "pine-leaf-norm.dds");
	tree.getMaterial(2)->loadTexture(GL_TEXTURE1, "models\\tree", "pine-branch-norm.dds");
	
	vec3 trees[TREES];
	for (vec3& v : trees)
	{
		float x = linearRand(-128.f, 128.f);
		float z = linearRand(-128.f, 128.f);
		v = vec3(x, terrain.getInterpolatedHeight(x, z), z);
	}

	// three special trees in predefined positions
	trees[0] = vec3(0, terrain.getInterpolatedHeight(0, -2), -2);
	trees[1] = vec3(-5, terrain.getInterpolatedHeight(-5, -1), -1);
	trees[2] = vec3(-4, terrain.getInterpolatedHeight(-4, -4), -4);

	tree.setupInstancingData(program.getAttribLocation("aOffset"), TREES, 3, (float*)&trees[0]);

	if (!skybox.load(
		"models\\mountain\\mft.tga",
		"models\\mountain\\mlf.tga",
		"models\\mountain\\mbk.tga",
		"models\\mountain\\mrt.tga",
		"models\\mountain\\mup.tga",
		"models\\mountain\\mdn.tga")) return false;

	// setup lights
	program.sendUniform("lightDir.direction", vec3(- 1.0f, 1.0f, 1.0f));
	program.sendUniform("lightDir.diffuse", vec3(1.0f, 1.0f, 1.0f));

	// load additional textures
	C3dglBitmap bm;
	glActiveTexture(GL_TEXTURE0);

	// Terrain texture
	bm.load("models/grass.jpg", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexTerrain);
	glBindTexture(GL_TEXTURE_2D, idTexTerrain);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// Wolf texture
	bm.load("models/wolf.jpg", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexWolf);
	glBindTexture(GL_TEXTURE_2D, idTexWolf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// Stone texture
	bm.load("models/stone.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexStone);
	glBindTexture(GL_TEXTURE_2D, idTexStone);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// Stone normal map
	bm.load("models/stoneNormal.png", GL_RGBA);
	if (!bm.getBits()) return false;
	glGenTextures(1, &idTexStoneNormal);
	glBindTexture(GL_TEXTURE_2D, idTexStoneNormal);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bm.getWidth(), bm.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bm.getBits());

	// none (simple-white) texture
	glGenTextures(1, &idTexNone);
	glBindTexture(GL_TEXTURE_2D, idTexNone);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	BYTE bytes[] = { 255, 255, 255 };
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &bytes);

	program.sendUniform("texture0", 0);
	program.sendUniform("textureNormal", 1);

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
	program.sendUniform("materialDiffuse", vec3(0.0f, 0.0f, 0.0f));	// white
	program.sendUniform("materialAmbient", vec3(1.0f, 1.0f, 1.0f));
	glActiveTexture(GL_TEXTURE0);

	m = matrixView;
	if (level >= 4) skybox.render(m);

	// setup materials for the terrain
	program.sendUniform("materialDiffuse", vec3(1.0f, 1.0f, 1.0f));	// white
	program.sendUniform("materialAmbient", vec3(0.1f, 0.1f, 0.1f));
	glBindTexture(GL_TEXTURE_2D, idTexTerrain);

	// render the terrain
	m = matrixView;
	terrain.render(m);

	// setup materials for the wolf
	program.sendUniform("materialDiffuse", vec3(1.0f, 1.0f, 1.0f));	// white background for textures
	program.sendUniform("materialAmbient", vec3(0.1f, 0.1f, 0.1f));
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
		program.sendUniform("bones", &transforms[0], transforms.size());// amount of vertexes 
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
	program.sendUniform("bNormalMap", level >= 8);
	m = matrixView;
	m = translate(m, vec3(-3, terrain.getInterpolatedHeight(-3, -1), -1));
	m = scale(m, vec3(0.01f, 0.01f, 0.01f));
	if (level >= 1) stone.render(m);
	program.sendUniform("bNormalMap", false);

	// render the trees
	program.sendUniform("bNormalMap", level >= 8);
	program.sendUniform("instancing", true);
	m = matrixView;
	m = scale(m, vec3(0.01f, 0.01f, 0.01f));
	tree.render(m);
	program.sendUniform("bNormalMap", false);
	program.sendUniform("instancing", false);
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
		pitch, vec3(1, 0, 0)),	// switch the pitch off
		_vel * deltaTime),		// animate camera motion (controlled by WASD keys)
		-pitch, vec3(1, 0, 0))	// switch the pitch on
		* matrixView;

	cout << degrees(getRoll(matrixView)) << endl;

	// clear screen and buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// move the camera up following the profile of terrain (Y coordinate of the terrain)
	float terrainY = -terrain.getInterpolatedHeight(inverse(matrixView)[3][0], inverse(matrixView)[3][2]);
	matrixView = translate(matrixView, vec3(0, terrainY, 0));

	// setup View Matrix
	program.sendUniform("matrixView", matrixView);
	program.sendUniform("matrixInvView", glm::inverse(matrixView));

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
	program.sendUniform("matrixProjection", perspective(radians(_fov), ratio, 0.02f, 1000.f));
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
	
	case '`': level = 0; program.sendUniform("level", level); break;
	case '1': level = 1; program.sendUniform("level", level); break;
	case '2': level = 2; program.sendUniform("level", level); break;
	case '3': level = 3; program.sendUniform("level", level); break;
	case '4': level = 4; program.sendUniform("level", level); break;
	case '5': level = 5; program.sendUniform("level", level); break;
	case '6': level = 6; program.sendUniform("level", level); break;
	case '7': level = 7; program.sendUniform("level", level); break;
	case '8': level = 8; program.sendUniform("level", level); break;
	case '9': level = 9; program.sendUniform("level", level); break;
	case '0': level = 10; program.sendUniform("level", level); break;
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

	// find delta (change to) pan & pitch
	float deltaYaw = 0.005f * (x - glutGet(GLUT_WINDOW_WIDTH) / 2);
	float deltaPitch = 0.005f * (y - glutGet(GLUT_WINDOW_HEIGHT) / 2);

	if (abs(deltaYaw) > 0.3f || abs(deltaPitch) > 0.3f)
		return;	// avoid warping side-effects

	// View = Pitch * DeltaPitch * DeltaYaw * Pitch^-1 * View;
	constexpr float maxPitch = radians(80.f);
	float pitch = getPitch(matrixView);
	float newPitch = glm::clamp(pitch + deltaPitch, -maxPitch, maxPitch);
	matrixView = rotate(rotate(rotate(mat4(1.f),
		newPitch, vec3(1.f, 0.f, 0.f)),
		deltaYaw, vec3(0.f, 1.f, 0.f)), 
		-pitch, vec3(1.f, 0.f, 0.f)) 
		* matrixView;
}

void onMouseWheel(int button, int dir, int x, int y)
{
	_fov = glm::clamp(_fov - dir * 5.f, 5.0f, 175.f);
	onReshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
}

int main(int argc, char **argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1280, 720);
	glutCreateWindow("CI5520 3D Graphics Programming");

	// default logging options
	C3dglLogger::setOptions(C3dglLogger::LOGGER_COLLAPSE_MESSAGES);

	// init glew
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		C3dglLogger::log("GLEW Error {}", (const char*)glewGetErrorString(err));
		return 0;
	}
	C3dglLogger::log("Using GLEW {}", (const char*)glewGetString(GLEW_VERSION));

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

	C3dglLogger::log("Vendor: {}", (const char *)glGetString(GL_VENDOR));
	C3dglLogger::log("Renderer: {}", (const char *)glGetString(GL_RENDERER));
	C3dglLogger::log("Version: {}", (const char*)glGetString(GL_VERSION));
	C3dglLogger::log("");

	// init light and everything – not a GLUT or callback function!
	if (!init())
	{
		C3dglLogger::log("Application failed to initialise\r\n");
		return 0;
	}

	// enter GLUT event processing cycle
	glutMainLoop();

	done();

	return 1;
}

