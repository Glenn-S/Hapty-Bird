//==============================================================================
/*
    \author    Brandon Sieu, Glenn Skelton

	Sources:
	Background texture: https://www.pexels.com/search/sky%20background/
	Bird model: https://static.free3d.com/models/2/5c434cad26be8b2d438b4567/60-kruk.zip
*/
//==============================================================================

//------------------------------------------------------------------------------
#include "chai3d.h"
#include "Wing.h"
#include "Body.h"
//------------------------------------------------------------------------------
#include <GLFW/glfw3.h>
#include<iostream>
#include<stdlib.h>
#include<time.h>
//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled 
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;

//number of devices
const int MAX_DEVICES = 2;


// structure for storing turublence parameters
typedef struct turbulence {
	double begin;
	double end;
	unsigned int periodRange;
	unsigned int amplitudeRange;
} turbulence;

// game state enumerators
enum GAME_STATE { MENU, LEVEL_SELECT, PLAY, PAUSE, GAME_OVER };
enum LEVEL { LEVEL_1 = 1, LEVEL_2 = 2, LEVEL_3 = 3 };
enum MENU_STATE { M_START, M_QUIT };
enum LEVEL_SELECT_STATE { L_EASY, L_MEDIUM, L_HARD, L_BACK };
enum PAUSE_STATE { P_RESUME, P_RESTART, P_QUIT };
enum CAMERA_ANGLE { CAMERA_1, CAMERA_2, CAMERA_3 };

//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera to render the world in the window display
cCamera* camera;

// a light source to illuminate the objects in the world
cDirectionalLight *light;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice[MAX_DEVICES];

// array to haptic device positions
cVector3d hapticDevicePosition[MAX_DEVICES];

// a small sphere (cursor) representing the haptic device 
cShapeSphere* cursor[MAX_DEVICES];

// flag to indicate if the haptic simulation currently running
bool simulationRunning = false;

// flag to indicate if the haptic simulation has terminated
bool simulationFinished = false;

// a frequency counter to measure the simulation graphic rate
cFrequencyCounter freqCounterGraphics;

// a frequency counter to measure the simulation haptic rate
cFrequencyCounter freqCounterHaptics;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = NULL;

// current width of window
int width  = 0;

// current height of window
int height = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;

//number of haptic devices
int numHapticDevices = 0;

//position of mouse
double mouseX, mouseY;

//max velocity
double maxV = 0.0;




/////////////////////////////////// GLOBAL VARIABLES WE ADDED ///////////////////////////////////////////

//the two wings & body
Body* birdBody = new Body(cVector3d(-0.5, 0.0, 0.0), // initial velocity
						  cVector3d(0, 1, 0), // natural resting wing length
						  5.0, // mass of bird
						  1500.0, // area of wing
						  1.0, // drag of wings
						  0.025, // device max bound in meters
						  -0.025); // device min bound meters



// Gravity
cVector3d GRAVITY(0, 0, -9.81);
cVector3d UPDRAFT(0, 0, 9.79); // counteract force of gravity to adjust playability


// level arrays
vector<cShapeCylinder*> *currentLevel;
vector<cShapeCylinder*> *lvl1 = new vector<cShapeCylinder*>();
vector<cShapeCylinder*> *lvl2 = new vector<cShapeCylinder*>();
vector<cShapeCylinder*> *lvl3 = new vector<cShapeCylinder*>();

// level turbulence
vector<turbulence*> *currentTurbulence;
vector<turbulence*> *lvl1_turbulence = new vector<turbulence*>();
vector<turbulence*> *lvl2_turbulence = new vector<turbulence*>();
vector<turbulence*> *lvl3_turbulence = new vector<turbulence*>();

// top and bottom of levels
constexpr double CEILING = 0.15; // 0.15
constexpr double FLOOR = -0.15; // -0.15



//////////////////////////////////////// GLOBAL STATE VARIABLES /////////////////////////////////////

bool TURBULENCE = false; // to know if turbulence is on or not
bool COLLISION = false; // to know if a collision has happened yet

LEVEL levelFlag = LEVEL_1; // to know which level we are currently in
GAME_STATE gameState = MENU; // state variable for knowing which portion of the game loop we are in (ie. menu or play)
MENU_STATE menuState = M_START;
LEVEL_SELECT_STATE levelSelectState = L_EASY;
PAUSE_STATE pauseState = P_RESUME;
CAMERA_ANGLE cameraAngle = CAMERA_1; // default set to camera 1


bool winState = false; // to know if the user has won the game
bool loseState = false; // to know if the user has lost the game

unsigned int SCORE = 0; // users score

bool isInitialized = false; // used to know if the menu elements have been initialized when first called

bool GAME_STARTED = false; // to know if the initialization period is done
bool INIT_PROCESS = false; // to know if currently in init process


//labels & widgets

// main menu
cPanel* titleMenu;
cLabel* titleMenu_start;
cLabel* titleMenu_quit;

// pause menu
cPanel* pauseMenu;
cLabel* pauseMenu_resume;
cLabel* pauseMenu_restart;
cLabel* pauseMenu_quit;

// level select menu
cPanel* levelMenu;
cLabel* levelMenu_pick;
cLabel* levelMenu_1;
cLabel* levelMenu_2;
cLabel* levelMenu_3;
cLabel* levelMenu_back;

// end of game
cPanel* gameoverMenu;
cLabel* loseMessage;
cLabel* keyagainLose;

cPanel* winMenu;
cLabel* winMessage;
cLabel* keyagainWin;

// score panel
cPanel* scorePanel;
cLabel* scoreLabel;


// GLOBAL CONSTANTS
constexpr double p = 0.038; // air density
constexpr double angleMultiplier = 20.0; // angle multiplier for wings rotation
constexpr double pipeRadius = 0.03; // 0.015

// game clock
cPrecisionClock gameClock; // cClock timer for level 


cBackground *background; // background image variable

//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window display is resized
void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when an error GLFW occurs
void errorCallback(int error, const char* a_description);

// callback when a key is pressed
void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);

// this function renders the scene
void updateGraphics(void);

// this function contains the main haptics simulation loop
void updateHaptics(void);

// this function closes the application
void close(void);






////////////////////////////// OUR ADDED FUNCTIONS ///////////////////////////////////

void pipeGenerator(vector<cShapeCylinder*> *a_pipes, unsigned int a_difficulty);
void turbulenceGenerator(vector<turbulence*> *a_turbulence, double a_lastX, unsigned int a_difficulty);

//scene changers
void showtitleMenu();
void showlevelMenu();
void showpauseMenu();
void showgameoverMenu();
void showwinMenu();

//scene cleaners
void cleanTitle();
void cleanlevelMenu();
void cleanPause();
void cleanLevel();
void cleangameoverMenu();
void cleanwinMenu();

// level changer
void setLevel(int level);

// button detection
bool isPressed(bool btn, bool &firstContact);

// change camera perrspective
void updateCamera(CAMERA_ANGLE a_perspective, cVector3d a_pos);


// haptic loop helpers
void mainMenuOptions(bool r_val[], bool l_val[]);
void levelSelectOptions(bool r_val[], bool l_val[]);
void pauseMenuOptions(bool r_val[], bool l_val[], double &a_prev, double &a_current, double &a_delta_t);
void playOptions(bool r_val[], bool l_val[], double &a_prev, double &a_current, double &a_delta_t);
void endGameOptions(bool r_val[], bool l_val[], double &a_prev, double &a_current, double &a_delta_t);
void resetClockVars(double &prev, double &current, double &delta_t);





//==============================================================================
/*
    Flappy Bird 3D
	AUTHORS: Brandon Sieu, Glenn Skelton
*/
//==============================================================================

int main(int argc, char* argv[])
{
	srand(time(0)); // initialize the random number generator

    //--------------------------------------------------------------------------
    // INITIALIZATION
    //--------------------------------------------------------------------------

    cout << endl;
    cout << "-----------------------------------" << endl;
    cout << "CHAI3D" << endl;
    cout << "-----------------------------------" << endl << endl << endl;
    cout << "Keyboard Options:" << endl << endl;
    cout << "[f] - Enable/Disable full screen mode" << endl;
    cout << "[m] - Enable/Disable vertical mirroring" << endl;
    cout << "[q] - Exit application" << endl;
    cout << endl << endl;


    //--------------------------------------------------------------------------
    // OPENGL - WINDOW DISPLAY
    //--------------------------------------------------------------------------

    // initialize GLFW library
    if (!glfwInit())
    {
        cout << "failed initialization" << endl;
        cSleepMs(1000);
        return 1;
    }

    // set error callback
    glfwSetErrorCallback(errorCallback);

    // compute desired size of window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int w = 0.8 * mode->height;
    int h = 0.5 * mode->height;
    int x = 0.5 * (mode->width - w);
    int y = 0.5 * (mode->height - h);

    // set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // set active stereo mode
    if (stereoMode == C_STEREO_ACTIVE)
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_STEREO, GL_FALSE);
    }

    // create display context
    window = glfwCreateWindow(w, h, "CHAI3D", NULL, NULL);
    if (!window)
    {
        cout << "failed to create window" << endl;
        cSleepMs(1000);
        glfwTerminate();
        return 1;
    }

    // get width and height of window
    glfwGetWindowSize(window, &width, &height);

    // set position of window
    glfwSetWindowPos(window, x, y);

    // set key callback
    glfwSetKeyCallback(window, keyCallback);

	// set mouse button callback
	//glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // set resize callback
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    // set current display context
    glfwMakeContextCurrent(window);

    // sets the swap interval for the current display context
    glfwSwapInterval(swapInterval);

#ifdef GLEW_VERSION
    // initialize GLEW library
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW library" << endl;
        glfwTerminate();
        return 1;
    }
#endif


    //--------------------------------------------------------------------------
    // WORLD - CAMERA - LIGHTING
    //--------------------------------------------------------------------------

    // create a new world.
    world = new cWorld();

    // set the background color of the environment
    world->m_backgroundColor.setBlack();


    // create a camera and insert it into the virtual world
    camera = new cCamera(world);
    world->addChild(camera);

	// set background image
	background = new cBackground;
	background->loadFromFile("background.jpg");
	camera->m_backLayer->addChild(background);
	
    // position and orient the camera
    camera->set( cVector3d (0.5, 0.0, 0.0),    // camera position (eye)
                 cVector3d (0.0, 0.0, 0.0),    // look at position (target)
                 cVector3d (0.0, 0.0, 1.0));   // direction of the (up) vector

    // set the near and far clipping planes of the camera
    camera->setClippingPlanes(0.01, 10.0);

    // set stereo mode
    camera->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    camera->setStereoEyeSeparation(0.01);
    camera->setStereoFocalLength(0.5);

    // set vertical mirrored display mode
    camera->setMirrorVertical(mirroredDisplay);

    // create a directional light source
    light = new cDirectionalLight(world);

    // insert light source inside world
    world->addChild(light);

    // enable light source
    light->setEnabled(true);

    // define direction of light beam
    light->setDir(-1.0, 0.0, 0.0); 

    //--------------------------------------------------------------------------
    // HAPTIC DEVICE
    //--------------------------------------------------------------------------

    // create a haptic device handler
    handler = new cHapticDeviceHandler();

	// get number of haptic devices
	numHapticDevices = handler->getNumDevices();

	// setup each haptic device
	for (int i = 0; i < numHapticDevices; i++)
	{
		// get a handle to the first haptic device
		handler->getDevice(hapticDevice[i], i);

		// open a connection to haptic device
		hapticDevice[i]->open();

		// calibrate device (if necessary)
		hapticDevice[i]->calibrate();

		// retrieve information about the current haptic device
		cHapticDeviceInfo info = hapticDevice[i]->getSpecifications();

		// create a sphere (cursor) to represent the haptic device
		cursor[i] = new cShapeSphere(0.01);

		// insert cursor inside world
		world->addChild(cursor[i]);
		cursor[i]->setShowEnabled(false); // disable seeing the cursors

		// display a reference frame if haptic device supports orientations
		if (info.m_sensedRotation == true)
		{
			// display reference frame
			cursor[i]->setShowFrame(true);

			// set the size of the reference frame
			cursor[i]->setFrameSize(0.05);
		}

		// if the device has a gripper, enable the gripper to simulate a user switch
		hapticDevice[i]->setEnableGripperUserSwitch(true);
	}


	//--------------------------------------------------------------------------
	// WORDL OBJECTS
	//--------------------------------------------------------------------------
	// create bird object
	birdBody->body = new cMultiMesh(); 
	birdBody->body->loadFromFile("birdBody.obj");
	birdBody->body->setUseTransparency(false);
	birdBody->body->setLocalPos(0.0, 0.0, 0.0);

	// create wings
	birdBody->leftWing = new cMultiMesh();
	birdBody->leftWing->loadFromFile("leftWing.obj");
	birdBody->leftWing->setUseTransparency(false);
	birdBody->leftWing->setLocalPos(0, -0.002, 0.002); // relative to the bird body
	birdBody->rightWing = new cMultiMesh();
	birdBody->rightWing->loadFromFile("rightWing.obj");
	birdBody->rightWing->setUseTransparency(false);
	birdBody->rightWing->setLocalPos(0, 0.002, 0.002); // relative to the bird body

	// parent wings and body
	birdBody->body->addChild(birdBody->rightWing);
	birdBody->body->addChild(birdBody->leftWing);
	birdBody->body->setEnabled(false, true);
	birdBody->body->setShowEnabled(false, true);
	world->addChild(birdBody->body);

	// set the birds colour
	cMaterialPtr birdColour = birdBody->body->m_material;
	birdColour->setBlue();
	birdBody->body->setMaterial(birdColour, true);

	// level generation
	pipeGenerator(lvl1, 1);
	pipeGenerator(lvl2, 2);
	pipeGenerator(lvl3, 3);
	
	// generate turbulence for each level
	turbulenceGenerator(lvl1_turbulence, lvl1->back()->getLocalPos().x(), 1);
	turbulenceGenerator(lvl2_turbulence, lvl2->back()->getLocalPos().x(), 2);
	turbulenceGenerator(lvl3_turbulence, lvl3->back()->getLocalPos().x(), 3);


    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    // create a font
    cFontPtr defaultFont = NEW_CFONTCALIBRI20();

	//titleMenu
	cFontPtr titleFont = NEW_CFONTCALIBRI40();
	titleMenu = new cPanel();
	camera->m_frontLayer->addChild(titleMenu);
	titleMenu->setSize(400, 200);
	titleMenu->m_material->setGrayDim();
	titleMenu->setTransparencyLevel(1.0);
	titleMenu->setEnabled(false);

	titleMenu_start = new cLabel(titleFont);
	titleMenu->addChild(titleMenu_start);
	titleMenu_start->setText("Start");
	titleMenu_start->m_fontColor.setRed(); // initial state
	titleMenu_start->setEnabled(false);

	titleMenu_quit = new cLabel(titleFont);
	titleMenu->addChild(titleMenu_quit);
	titleMenu_quit->setText("Quit");
	titleMenu_quit->m_fontColor.setWhite();
	titleMenu_quit->setEnabled(false);

	//pauseMenu
	pauseMenu = new cPanel();
	camera->m_frontLayer->addChild(pauseMenu);
	pauseMenu->setSize(400, 200);
	pauseMenu->m_material->setGrayDim();
	pauseMenu->setTransparencyLevel(0.8);
	pauseMenu->setEnabled(false);

	pauseMenu_resume = new cLabel(defaultFont);
	pauseMenu->addChild(pauseMenu_resume);
	pauseMenu_resume->setText("Resume");
	pauseMenu_resume->m_fontColor.setRed();
	pauseMenu_resume->setEnabled(false);

	pauseMenu_restart = new cLabel(defaultFont);
	pauseMenu->addChild(pauseMenu_restart);
	pauseMenu_restart->setText("Restart");
	pauseMenu_restart->m_fontColor.setWhite();
	pauseMenu_restart->setEnabled(false);

	pauseMenu_quit = new cLabel(defaultFont);
	pauseMenu->addChild(pauseMenu_quit);
	pauseMenu_quit->setText("Quit");
	pauseMenu_quit->m_fontColor.setWhite();
	pauseMenu_quit->setEnabled(false);

	// levelMenu
	levelMenu = new cPanel();
	camera->m_frontLayer->addChild(levelMenu);
	levelMenu->setSize(400, 200);
	levelMenu->m_material->setGrayDim();
	levelMenu->setTransparencyLevel(1.0);
	levelMenu->setEnabled(false);

	levelMenu_pick = new cLabel(defaultFont);
	levelMenu->addChild(levelMenu_pick);
	levelMenu_pick->setText("Choose a Difficulty:");
	levelMenu_pick->m_fontColor.setWhite();
	levelMenu_pick->setEnabled(false);

	levelMenu_1 = new cLabel(defaultFont);
	levelMenu->addChild(levelMenu_1);
	levelMenu_1->setText("Easy");
	levelMenu_1->m_fontColor.setRed(); // initial state
	levelMenu_1->setEnabled(false);

	levelMenu_2 = new cLabel(defaultFont);
	levelMenu->addChild(levelMenu_2);
	levelMenu_2->setText("Medium");
	levelMenu_2->m_fontColor.setWhite();
	levelMenu_2->setEnabled(false);

	levelMenu_3 = new cLabel(defaultFont);
	levelMenu->addChild(levelMenu_3);
	levelMenu_3->setText("Hard");
	levelMenu_3->m_fontColor.setWhite();
	levelMenu_3->setEnabled(false);

	levelMenu_back = new cLabel(defaultFont);
	levelMenu->addChild(levelMenu_back);
	levelMenu_back->setText("Back");
	levelMenu_back->m_fontColor.setWhite();
	levelMenu_back->setEnabled(false);

	// Lose Menu
	cFontPtr endGameFont = NEW_CFONTCALIBRI40();
	gameoverMenu = new cPanel();
	camera->m_frontLayer->addChild(gameoverMenu);
	gameoverMenu->setSize(400, 200);
	gameoverMenu->m_material->setGrayDim();
	gameoverMenu->setTransparencyLevel(1.0);
	gameoverMenu->setEnabled(false);

	loseMessage = new cLabel(endGameFont);
	gameoverMenu->addChild(loseMessage);
	loseMessage->setText("Ouch! You Lost!");
	loseMessage->m_fontColor.setWhite();
	loseMessage->setEnabled(false);

	keyagainLose = new cLabel(defaultFont);
	gameoverMenu->addChild(keyagainLose);
	keyagainLose->setText("Press Any Button to Continue");
	keyagainLose->m_fontColor.setWhite();
	keyagainLose->setEnabled(false);

	// Win Menu
	winMenu = new cPanel();
	camera->m_frontLayer->addChild(winMenu);
	winMenu->setSize(400, 200);
	winMenu->m_material->setGrayDim();
	winMenu->setTransparencyLevel(1.0);
	winMenu->setEnabled(false);

	winMessage = new cLabel(endGameFont);
	winMenu->addChild(winMessage);
	winMessage->setText("Congratulations! You won!");
	winMessage->m_fontColor.setWhite();
	winMessage->setEnabled(false);

	keyagainWin = new cLabel(defaultFont);
	winMenu->addChild(keyagainWin);
	keyagainWin->setText("Press Any Button to Continue");
	keyagainWin->m_fontColor.setWhite();
	keyagainWin->setEnabled(false);


	// score panel
	cFontPtr scoreFont = NEW_CFONTCALIBRI28();
	scorePanel = new cPanel();
	camera->m_frontLayer->addChild(scorePanel);
	scorePanel->setSize(160, 33);
	scorePanel->m_material->setGrayDim();
	scorePanel->setTransparencyLevel(0.8);
	scorePanel->setEnabled(false);

	scoreLabel = new cLabel(scoreFont);
	scorePanel->addChild(scoreLabel);
	scoreLabel->m_fontColor.setWhite();
	scoreLabel->setEnabled(false);
	




    //--------------------------------------------------------------------------
    // START SIMULATION
    //--------------------------------------------------------------------------

    // create a thread which starts the main haptics rendering loop
    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    // setup callback when application exits
    atexit(close);


    //--------------------------------------------------------------------------
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------

    // call window size callback at initialization
    windowSizeCallback(window, width, height);

    // main graphic loop
    while (!glfwWindowShouldClose(window))
    {
        // get width and height of window
        glfwGetWindowSize(window, &width, &height);

        // render graphics
        updateGraphics();

        // swap buffers
        glfwSwapBuffers(window);

        // process events
        glfwPollEvents();

        // signal frequency counter
        freqCounterGraphics.signal(1);
    }

    // close window
    glfwDestroyWindow(window);

    // terminate GLFW library
    glfwTerminate();

    // exit
    return 0;
}

//------------------------------------------------------------------------------

void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update window size
    width  = a_width;
    height = a_height;
}

//------------------------------------------------------------------------------

void errorCallback(int a_error, const char* a_description)
{
    cout << "Error: " << a_description << endl;
}

//------------------------------------------------------------------------------

void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
    // filter calls that only include a key press
    if (a_action != GLFW_PRESS)
    {
        return;
    }

    // option - exit
    else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
    {
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);
    }

    // option - toggle fullscreen
    else if (a_key == GLFW_KEY_F)
    {
        // toggle state variable
        fullscreen = !fullscreen;

        // get handle to monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        // get information about monitor
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // set fullscreen or window mode
        if (fullscreen)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
        else
        {
            int w = 0.8 * mode->height;
            int h = 0.5 * mode->height;
            int x = 0.5 * (mode->width - w);
            int y = 0.5 * (mode->height - h);
            glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
    }

    // option - toggle vertical mirroring
    else if (a_key == GLFW_KEY_M)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
    }
	else if (a_key == GLFW_KEY_T) {
		TURBULENCE = !TURBULENCE;
	}
	else if (a_key == GLFW_KEY_C) {
		COLLISION = !COLLISION;
	}
	else if (a_key == GLFW_KEY_P) {
		isInitialized = false;
		if (gameState == PLAY) gameState = PAUSE;
		else if (gameState == PAUSE) {
			cleanPause();
			gameState = PLAY;
		}
	}
}

//------------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

	// close haptic device
	for (int i = 0; i < numHapticDevices; i++)
	{
		hapticDevice[i]->close();
	}

    // delete resources
    delete hapticsThread;
    delete world;
    delete handler;
}

//------------------------------------------------------------------------------

void updateGraphics(void)
{
    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////

    // update shadow maps (if any)
    world->updateShadowMaps(false, mirroredDisplay);

    // render world
    camera->renderView(width, height);

    // wait until all GL commands are completed
    glFinish();

    // check for any OpenGL errors
    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(err);
}

//------------------------------------------------------------------------------

void updateHaptics(void)
{
    simulationRunning  = true; // to know if the haptic loop is still going
    simulationFinished = false; // to know if we need to terminate

	cVector3d velocity[2] = { cVector3d(0, 0, 0), cVector3d(0, 0, 0) }; // array for veocity for each device
	cVector3d position[2] = { cVector3d(0, 0, 0), cVector3d(0, 0, 0) }; // array for position for each device
	cMatrix3d rotation[2];

	cVector3d netForce; // force accumulator
	cVector3d acceleration; // acceleration variable
	cVector3d newVelocity; // storage for new velocity calculation

	// haptic device accumulators
	cVector3d force(0, 0, 0);
	cVector3d torque(0, 0, 0);
	double gripperForce = 0.0;

	double prev = 0.0; // stores prev time value
	double current = 0.0; // stores current time value
	double delta_t = 0.0; // stores difference in prev and curr

	bool collisionDetected = false; // to tell if a collision has occured
	bool beginCycle = false; // bool for knowing when to go into collision feedback cycle
	double startTime = 0.0; // start time for working with the collision cycle

	// clock 
	double startUpTime = 2.0; // amount of time needed to setup device constraints smoothly
	double currTime = 0.0; // variable for storing the current time for the init stage
	bool startSetupClock = false; // bool for knowing if the init clock has been started or not

	bool scoreUpdate = false; // to know when to update the score after passing a pipe


	// button readers
	bool button0, button1, button2, button3; // bool for reading in button states from Falcon

	// arrays for keeping track of buttons and storing button state to determine if pressed or not
	bool rightFalconButtons[4] = { false, false, false, false };
	bool rightIsPressed[4] = { false, false, false, false };
	bool rightButtonValues[4] = { false, false, false, false };
	bool leftFalconButtons[4] = { false, false, false, false };
	bool leftIsPressed[4] = { false, false, false, false };
	bool leftButtonValues[4] = { false, false, false, false };



    // main haptic simulation loop
    while(simulationRunning)
    {
		for (int i = 0; i < numHapticDevices; i++) {
			// read position 
			hapticDevice[i]->getPosition(position[i]);

			// read orientation 
			hapticDevice[i]->getRotation(rotation[i]);

			hapticDevice[i]->getLinearVelocity(velocity[i]);

			// button calls
			// read user-switch status (button 0)
			button0 = false, button1 = false, button2 = false, button3 = false;
			hapticDevice[i]->getUserSwitch(0, button0);
			hapticDevice[i]->getUserSwitch(1, button1);
			hapticDevice[i]->getUserSwitch(2, button2);
			hapticDevice[i]->getUserSwitch(3, button3);

			if (i == 0) {
				rightFalconButtons[0] = button0;
				rightFalconButtons[1] = button1;
				rightFalconButtons[2] = button2; 
				rightFalconButtons[3] = button3;
			} else {
				leftFalconButtons[0] = button0;
				leftFalconButtons[1] = button1;
				leftFalconButtons[2] = button2;
				leftFalconButtons[3] = button3;
			}


			/////////////////////////////////////////////////////////////////////
			// UPDATE 3D CURSOR MODEL
			/////////////////////////////////////////////////////////////////////

			// update position and orienation of cursor
			cursor[i]->setLocalPos(position[i]);
			cursor[i]->setLocalRot(rotation[i]);

		}
		// read and set button values
		for (unsigned int i = 0; i < (sizeof(rightFalconButtons) / sizeof(rightFalconButtons[0])); i++) {
			bool pressed = isPressed(rightFalconButtons[i], rightIsPressed[i]);
			rightButtonValues[i] = pressed;
		}
		for (unsigned int i = 0; i < (sizeof(leftFalconButtons) / sizeof(leftFalconButtons[0])); i++) {
			bool pressed = isPressed(leftFalconButtons[i], leftIsPressed[i]);
			leftButtonValues[i] = pressed;
		}




		switch (gameState) {
		case MENU: {
			showtitleMenu();
			mainMenuOptions(rightButtonValues, leftButtonValues);
			break;
		}
		case LEVEL_SELECT: {			
			showlevelMenu();
			levelSelectOptions(rightButtonValues, leftButtonValues);
			break;
		}
		case PLAY: {
			playOptions(rightButtonValues, leftButtonValues, prev, current, delta_t);


			/////////////////////////////////////////////////////////////////////
			// READ HAPTIC DEVICE
			/////////////////////////////////////////////////////////////////////

			netForce = cVector3d(0, 0, 0); // reset net force

			if (!GAME_STARTED) {
				updateCamera(cameraAngle, birdBody->body->getLocalPos()); // allow camera anlge readjust during setup
				
				if (!startSetupClock) {
					gameClock.reset();
					gameClock.start(); // turn clock on
					startSetupClock = !startSetupClock;
				}
				double currTime = gameClock.getCurrentTimeSeconds();

				if (!INIT_PROCESS) {
					switch (numHapticDevices) {
					case 1:
						// apply easing force to controller to guide user into correct position
						birdBody->controllerStartup(position[0], position[0], currTime, startUpTime);
						force = birdBody->m_rightWing->m_F_net + birdBody->m_leftWing->m_F_net / 2.0;
						hapticDevice[0]->setForceAndTorqueAndGripperForce(force, torque, gripperForce);
						break;
					case 2:
						// apply easing force to controller to guide user into correct position
						birdBody->controllerStartup(position[0], position[1], currTime, startUpTime);
						hapticDevice[0]->setForceAndTorqueAndGripperForce(birdBody->m_rightWing->m_F_net, torque, gripperForce);
						hapticDevice[1]->setForceAndTorqueAndGripperForce(birdBody->m_leftWing->m_F_net, torque, gripperForce);
						break;
					}
					if (currTime > startUpTime) {
						INIT_PROCESS = !INIT_PROCESS;
					}
				} else {
					gameClock.reset();
					gameClock.start(); // turn clock on
					startSetupClock = !startSetupClock; // reset for if init gets called again from reseting the level
					GAME_STARTED = !GAME_STARTED; // set to true to make sure init doesn't get called until the game is reset
					scoreUpdate = false; 
				}
				break;
			}
			
			
			/////////////////////////////////////////////////////////////////////
			// COMPUTE FORCES
			/////////////////////////////////////////////////////////////////////

			force = cVector3d(0, 0, 0);
			torque = cVector3d(0, 0, 0);
			gripperForce = 0.0;

			cVector3d Flift_right;
			cVector3d Flift_left;


			// Get the cumulative forces for each wing
			switch (numHapticDevices) {
			case 1:
				birdBody->updateForces(position[0], position[0], velocity[0], velocity[0]);
				break;
			case 2:
				birdBody->updateForces(position[0], position[1], velocity[0], velocity[1]);
				break;
			}

			// define game turbulence regions
			if (currentTurbulence->size() > 0) {
				if (birdBody->body->getLocalPos().x() > currentTurbulence->at(0)->begin) {
					// do nothin, don't pop
				}
				else if ((birdBody->body->getLocalPos().x() <= currentTurbulence->at(0)->begin) &&
					(birdBody->body->getLocalPos().x() >= currentTurbulence->at(0)->end)) {
					// turbulence applies
					birdBody->applyTurbulence(gameClock.getCurrentTimeSeconds(), // was timer
						currentTurbulence->at(0)->periodRange,
						currentTurbulence->at(0)->amplitudeRange);
				}
				else if (birdBody->body->getLocalPos().x() < currentTurbulence->at(0)->end) {
					// turbulence is done for this element, pop since done for a little bit
					currentTurbulence->erase(currentTurbulence->begin());
				}
			}

			// get final forces
			Flift_right = birdBody->m_rightWing->m_F_net;
			Flift_left = birdBody->m_leftWing->m_F_net;
			// get force of both wings combined
			netForce += birdBody->m_rightWing->m_F_up;
			netForce += birdBody->m_leftWing->m_F_up;



			//////////////////////////////////////////////////////////////////////
			// UPDATE GRAPHICS
			//////////////////////////////////////////////////////////////////////
			// update position of the bird, bird graphic, camera
			current = gameClock.getCurrentTimeSeconds(); // was timer
			delta_t = current - prev;

			cVector3d Flift;

			// calculate force difference for graphical portion
			double wingRatio = (birdBody->m_rightWing->m_currentAreaRatio + birdBody->m_leftWing->m_currentAreaRatio) / 2.0;
			acceleration = ((netForce) / birdBody->m_mass) + GRAVITY + (UPDRAFT * wingRatio);
			newVelocity = birdBody->m_velocity + acceleration * delta_t;

			//cVector3d newPos = birdBody->m_localPos + newVelocity * delta_t;
			cVector3d newPos = birdBody->body->getLocalPos() + newVelocity * delta_t;

			// update bird haptic object
			if (!collisionDetected) { // check to make sure we haven't hit anything before updating
				if ((newPos.z() < CEILING) && (newPos.z() > FLOOR)) {
					birdBody->m_velocity = newVelocity;
					birdBody->body->setLocalPos(newPos); //birdBody->m_localPos = newPos;
				}
				else {
					birdBody->m_velocity = cVector3d(newVelocity.x(), 0, 0);
					if ((newPos.z() >= CEILING))
						birdBody->body->setLocalPos(cVector3d(newPos.x(), 0, CEILING));
					else if ((newPos.z() <= FLOOR))
						birdBody->body->setLocalPos(cVector3d(newPos.x(), 0, FLOOR));
				}
			}

			
			
			//////////////////////////////////////////////////////////////////
			// APPLY ROTATION ON WINGS
			//////////////////////////////////////////////////////////////////
			double wingAngle;
			cVector3d resultant;
		
			switch (numHapticDevices) {
			case 1:
				resultant = birdBody->m_rightWing->m_initialPos + position[0]; // only need to get one since one controller
				wingAngle = cAngle(resultant, birdBody->m_rightWing->m_initialPos);
				if (position[0].z() < 0.0) wingAngle = -wingAngle;

				birdBody->rightWing->setLocalRot(cMatrix3d(1.0, 0.0, 0.0, wingAngle * angleMultiplier));
				birdBody->leftWing->setLocalRot(cMatrix3d(1.0, 0.0, 0.0, -wingAngle * angleMultiplier));
				break;
			case 2:
				// right wing
				resultant = birdBody->m_rightWing->m_initialPos + position[0]; // only need to get one
				wingAngle = cAngle(resultant, birdBody->m_rightWing->m_initialPos);
				if (position[0].z() < 0.0) wingAngle = -wingAngle;
				birdBody->rightWing->setLocalRot(cMatrix3d(1.0, 0.0, 0.0, wingAngle * angleMultiplier)); // apply transformation

																										 // left wing
				resultant = birdBody->m_leftWing->m_initialPos + position[1]; // only need to get one
				wingAngle = cAngle(resultant, birdBody->m_leftWing->m_initialPos);
				if (position[1].z() < 0.0) wingAngle = -wingAngle;
				birdBody->leftWing->setLocalRot(cMatrix3d(1.0, 0.0, 0.0, -wingAngle * angleMultiplier)); // apply transformation
				break;
			}
			
			
			
			///////////////////////////////////////////////////////////////
			// collision detection
			///////////////////////////////////////////////////////////////
			if (currentLevel->size() >= 2) {
				
				if (birdBody->collisionDetector(currentLevel->at(0))) {
					collisionDetected = true;
				} else if (birdBody->collisionDetector(currentLevel->at(1))) {
					collisionDetected = true;
				} else {
					if ((birdBody->body->getLocalPos().x() - currentLevel->at(0)->getLocalPos().x() - currentLevel->at(0)->getBaseRadius() <= 0.0) &&
						(birdBody->body->getLocalPos().x() - currentLevel->at(1)->getLocalPos().x() - currentLevel->at(1)->getBaseRadius() <= 0.0)) {
						if (!scoreUpdate) {
							// update score
							SCORE = SCORE + 20; // increment by 20 points
							scoreLabel->setText("SCORE: " + cStr(SCORE)); // update score label
							scoreUpdate = !scoreUpdate;
						}
						
					}

					// check if we moved beyond the pipe
					if ((birdBody->body->getLocalPos().x() - currentLevel->at(0)->getLocalPos().x() - currentLevel->at(0)->getBaseRadius() <= -1.0) &&
						(birdBody->body->getLocalPos().x() - currentLevel->at(1)->getLocalPos().x() - currentLevel->at(1)->getBaseRadius() <= -1.0)) {
						
						// remove pipes that we moved past
						// remove children from the world and remove from array
						world->removeChild(currentLevel->at(0));
						world->removeChild(currentLevel->at(1));
						currentLevel->erase(currentLevel->begin(), currentLevel->begin() + 2);
						scoreUpdate = !scoreUpdate;
					}
				}
			} else {
				// YOU'VE WON!!!!
				gameState = GAME_OVER;
				winState = true;
				break;
			}


			
			/////////////////////////////////////////////////////////////////
			// UPDATE CAMERA
			/////////////////////////////////////////////////////////////////
			updateCamera(cameraAngle, newPos);
			

			// update prev
			prev = current; // move time index forward

		
			/////////////////////////////////////////////////////////////////////
			// APPLY FORCES
			/////////////////////////////////////////////////////////////////////
			//cVector3d netVelocity;
			cVector3d outputForce(0, 0, 0);
			cVector3d outputForceL(0, 0, 0);
			cVector3d outputForceR(0, 0, 0);

			constexpr double frequency = 1.0;
			constexpr double amplitude = 0.015;
			constexpr double k = 2000.0;

			//////////////////////////////////////////////////////////////////////
			// SEQUENCE FOR IF COLLISION DETECTED FROM BEFORE
			//////////////////////////////////////////////////////////////////////
			switch (numHapticDevices) {
			case 1:
				if (collisionDetected) {
					if (!beginCycle) startTime = gameClock.getCurrentTimeSeconds();

					double time = gameClock.getCurrentTimeSeconds() - startTime;
					double targetX = amplitude * sin(2 * M_PI * frequency * time);

					if (!beginCycle) {
						beginCycle = !beginCycle;
					} else {
						if (targetX <= 0.0) {
							collisionDetected = !collisionDetected;
							beginCycle = !beginCycle;
							// pause game (game over)
							gameState = GAME_OVER;
							loseState = true;
							break;
						}
					}
					outputForce = -k * cVector3d(-targetX, 0.0, 0.0);
				} else {
					outputForce = (Flift_right + Flift_left) / 2.0;
				}
				hapticDevice[0]->setForceAndTorqueAndGripperForce(outputForce, torque, gripperForce);
				break;
			case 2:
				if (collisionDetected) {
					if (!beginCycle) startTime = gameClock.getCurrentTimeSeconds();

					double time = gameClock.getCurrentTimeSeconds() - startTime;
					double targetX = amplitude * sin(2 * M_PI * frequency * time);

					if (!beginCycle) {
						beginCycle = !beginCycle;
					} else {
						if (targetX <= 0.0) {
							collisionDetected = !collisionDetected;
							beginCycle = !beginCycle;
							// pause game (game over)
							gameState = GAME_OVER;
							loseState = true;
							break;
						}
					}
					outputForceR = -k * cVector3d(-targetX, 0.0, 0.0);
					outputForceL = -k * cVector3d(-targetX, 0.0, 0.0);
				}
				else {
					outputForceR = Flift_right;
					outputForceL = Flift_left;
				}
				hapticDevice[0]->setForceAndTorqueAndGripperForce(outputForceR, torque, gripperForce);
				hapticDevice[1]->setForceAndTorqueAndGripperForce(outputForceL, torque, gripperForce);
				break;
			}
			break;
		}
		case PAUSE: {
			// apply boundary force but nothing more
			birdBody->controllerStartup(position[0], position[1], 1.0, 1.0);
			hapticDevice[0]->setForceAndTorqueAndGripperForce(birdBody->m_rightWing->m_F_net, torque, gripperForce);
			hapticDevice[1]->setForceAndTorqueAndGripperForce(birdBody->m_leftWing->m_F_net, torque, gripperForce);

			showpauseMenu();
			pauseMenuOptions(rightButtonValues, leftButtonValues, prev, current, delta_t);
			break;
		}
		case GAME_OVER:{
			// apply boundary force but nothing more
			birdBody->controllerStartup(position[0], position[1], 1.0, 1.0);
			hapticDevice[0]->setForceAndTorqueAndGripperForce(birdBody->m_rightWing->m_F_net, torque, gripperForce);
			hapticDevice[1]->setForceAndTorqueAndGripperForce(birdBody->m_leftWing->m_F_net, torque, gripperForce);

			endGameOptions(rightButtonValues, leftButtonValues, prev, current, delta_t);
			break;
		}
		}
		
	
        // signal frequency counter
        freqCounterHaptics.signal(1);
    }
    
    // exit haptics thread
    simulationFinished = true;
}

//------------------------------------------------------------------------------


////////////////////////////////////////////////////// MAIN GAME LOOP HELPER FUNCTIONS //////////////////////////////////////////////////
void mainMenuOptions(bool r_val[], bool l_val[]) {
	if (l_val[0] || r_val[0]) { // center button
		switch (menuState) {
		case (M_START):
			cleanTitle();
			gameState = LEVEL_SELECT;
			break;
		case (M_QUIT):
			simulationRunning = false; // end the haptic loop
			glfwSetWindowShouldClose(window, GLFW_TRUE);
			break;
		}
	}
	else if (l_val[1] || r_val[1]) { // left button
	 // iterate downwards in menu
		switch (menuState) {
		case M_START:
			titleMenu_start->m_fontColor.setWhite();
			titleMenu_quit->m_fontColor.setRed();
			menuState = M_QUIT;
			break;
		case M_QUIT:
			titleMenu_start->m_fontColor.setRed();
			titleMenu_quit->m_fontColor.setWhite();
			menuState = M_START;
			break;
		}
	}
	else if (l_val[3] || r_val[3]) { // right button
	 // iterate upwards in menu
		switch (menuState) { // change based on where we just were
		case M_START:
			titleMenu_start->m_fontColor.setWhite();
			titleMenu_quit->m_fontColor.setRed();
			menuState = M_QUIT;
			break;
		case M_QUIT:
			titleMenu_start->m_fontColor.setRed();
			titleMenu_quit->m_fontColor.setWhite();
			menuState = M_START;
			break;
		}
	}

	return;
}
void levelSelectOptions(bool r_val[], bool l_val[]) {
	if (l_val[0] || r_val[0]) { // center button
		switch (levelSelectState) {
		case (L_EASY):
			cleanlevelMenu();
			setLevel(LEVEL_1);
			levelFlag = LEVEL_1;
			gameState = PLAY;
			break;
		case (L_MEDIUM):
			cleanlevelMenu();
			setLevel(LEVEL_2);
			levelFlag = LEVEL_2;
			gameState = PLAY;
			break;
		case (L_HARD):
			cleanlevelMenu();
			setLevel(LEVEL_3);
			levelFlag = LEVEL_3;
			gameState = PLAY;
			break;
		case (L_BACK):
			cleanlevelMenu();
			gameState = MENU;
			break;
		}
	}
	else if (l_val[1] || r_val[1]) { // left button
	 // iterate downwards in menu

		switch (levelSelectState) {
		case L_EASY:
			levelMenu_1->m_fontColor.setWhite();
			levelMenu_2->m_fontColor.setWhite();
			levelMenu_3->m_fontColor.setWhite();
			levelMenu_back->m_fontColor.setRed();
			levelSelectState = L_BACK;
			break;
		case L_MEDIUM:
			levelMenu_1->m_fontColor.setRed();
			levelMenu_2->m_fontColor.setWhite();
			levelMenu_3->m_fontColor.setWhite();
			levelMenu_back->m_fontColor.setWhite();
			levelSelectState = L_EASY;
			break;
		case L_HARD:
			levelMenu_1->m_fontColor.setWhite();
			levelMenu_2->m_fontColor.setRed();
			levelMenu_3->m_fontColor.setWhite();
			levelMenu_back->m_fontColor.setWhite();
			levelSelectState = L_MEDIUM;
			break;
		case L_BACK:
			levelMenu_1->m_fontColor.setWhite();
			levelMenu_2->m_fontColor.setWhite();
			levelMenu_3->m_fontColor.setRed();
			levelMenu_back->m_fontColor.setWhite();
			levelSelectState = L_HARD;
			break;
		}
	}
	else if (l_val[3] || r_val[3]) { // right button
	 // iterate upwards in menu
		switch (levelSelectState) {
		case L_EASY:
			levelMenu_1->m_fontColor.setWhite();
			levelMenu_2->m_fontColor.setRed();
			levelMenu_3->m_fontColor.setWhite();
			levelMenu_back->m_fontColor.setWhite();
			levelSelectState = L_MEDIUM;
			break;
		case L_MEDIUM:
			levelMenu_1->m_fontColor.setWhite();
			levelMenu_2->m_fontColor.setWhite();
			levelMenu_3->m_fontColor.setRed();
			levelMenu_back->m_fontColor.setWhite();
			levelSelectState = L_HARD;
			break;
		case L_HARD:
			levelMenu_1->m_fontColor.setWhite();
			levelMenu_2->m_fontColor.setWhite();
			levelMenu_3->m_fontColor.setWhite();
			levelMenu_back->m_fontColor.setRed();
			levelSelectState = L_BACK;
			break;
		case L_BACK:
			levelMenu_1->m_fontColor.setRed();
			levelMenu_2->m_fontColor.setWhite();
			levelMenu_3->m_fontColor.setWhite();
			levelMenu_back->m_fontColor.setWhite();
			levelSelectState = L_EASY;
			break;
		}
	}
}
void pauseMenuOptions(bool r_val[], bool l_val[], double &a_prev, double &a_current, double &a_delta_t) {
	if (l_val[0] || r_val[0]) { // center button										// refactor
		switch (pauseState) {
		case (P_RESUME):
			cleanPause();
			gameClock.start(); // start clock back up 
			gameState = PLAY; // continue playing
			break;
		case (P_RESTART):
			//put restart code
			cleanPause();
			cleanLevel();
			setLevel(levelFlag);
			gameClock.reset(); // reste the clock for good measure
			resetClockVars(a_prev, a_current, a_delta_t);
			gameState = PLAY; // continue playing
			break;
		case (P_QUIT):
			cleanLevel(); // remove all level elements, clean up memory
			cleanPause();
			gameClock.reset(); // reste the clock for good measure
			resetClockVars(a_prev, a_current, a_delta_t);
			gameState = MENU;

			// reset device forces to 0
			hapticDevice[0]->setForceAndTorqueAndGripperForce(cVector3d(0, 0, 0), cVector3d(0, 0, 0), 0.0);
			hapticDevice[1]->setForceAndTorqueAndGripperForce(cVector3d(0, 0, 0), cVector3d(0, 0, 0), 0.0);
			break;
		}
	}
	else if (l_val[1] || r_val[1]) { // left button
	   // iterate downwards in menu
		switch (pauseState) {
		case P_RESUME:
			pauseMenu_resume->m_fontColor.setWhite();
			pauseMenu_restart->m_fontColor.setWhite();
			pauseMenu_quit->m_fontColor.setRed();
			pauseState = P_QUIT;
			break;
		case P_RESTART:
			pauseMenu_resume->m_fontColor.setRed();
			pauseMenu_restart->m_fontColor.setWhite();
			pauseMenu_quit->m_fontColor.setWhite();
			pauseState = P_RESUME;
			break;
		case P_QUIT:
			pauseMenu_resume->m_fontColor.setWhite();
			pauseMenu_restart->m_fontColor.setRed();
			pauseMenu_quit->m_fontColor.setWhite();
			pauseState = P_RESTART;
			break;
		}
	}
	else if (l_val[3] || r_val[3]) { // right button
	   // iterate upwards in menu
		switch (pauseState) {
		case P_RESUME:
			pauseMenu_resume->m_fontColor.setWhite();
			pauseMenu_restart->m_fontColor.setRed();
			pauseMenu_quit->m_fontColor.setWhite();
			pauseState = P_RESTART;
			break;
		case P_RESTART:
			pauseMenu_resume->m_fontColor.setWhite();
			pauseMenu_restart->m_fontColor.setWhite();
			pauseMenu_quit->m_fontColor.setRed();
			pauseState = P_QUIT;
			break;
		case P_QUIT:
			pauseMenu_resume->m_fontColor.setRed();
			pauseMenu_restart->m_fontColor.setWhite();
			pauseMenu_quit->m_fontColor.setWhite();
			pauseState = P_RESUME;
			break;
		}
	}
}
void playOptions(bool r_val[], bool l_val[], double &a_prev, double &a_current, double &a_delta_t) {
	// check for mouse button clicks
	if (l_val[0] || r_val[0]) { // center button
		// pause the game
		gameClock.stop();
		gameState = PAUSE;
		//break;
	}
	else if (l_val[2] || r_val[2]) { // front button
		// restart the level
		cleanPause();
		cleanLevel();
		setLevel(levelFlag);																
		gameClock.reset(); // reste the clock for good measure
		resetClockVars(a_prev, a_current, a_delta_t);
		gameState = PLAY; // continue playing
		//break;
	}
	else if (l_val[1] || r_val[1]) { // left button
		switch (cameraAngle) {
		case (CAMERA_1):
			cameraAngle = CAMERA_3;
			break;
		case (CAMERA_2):
			cameraAngle = CAMERA_1;
			break;
		case (CAMERA_3):
			cameraAngle = CAMERA_2;
			break;
		}
	}
	else if (l_val[3] || r_val[3]) { // right button
		switch (cameraAngle) {
		case (CAMERA_1):
			cameraAngle = CAMERA_2;
			break;
		case (CAMERA_2):
			cameraAngle = CAMERA_3;
			break;
		case (CAMERA_3):
			cameraAngle = CAMERA_1;
			break;
		}
	}
}
void endGameOptions(bool r_val[], bool l_val[], double &a_prev, double &a_current, double &a_delta_t) {
	if (winState) {
		showwinMenu();
	}
	else if (loseState) {
		showgameoverMenu();
	}
	// check to see if any button is pressed
	if ((l_val[0] || r_val[0]) ||
		(l_val[1] || r_val[1]) ||
		(l_val[2] || r_val[2]) ||
		(l_val[3] || r_val[3])) { // center button

		if (winState) cleanwinMenu();
		else if (loseState) cleangameoverMenu();
		cleanLevel(); // remove all level elements, clean up memory

		gameClock.reset(); // reste the clock for good measure
		resetClockVars(a_prev, a_current, a_delta_t);
		winState = false;
		loseState = false;
		gameState = MENU;

		// reset device forces to 0
		hapticDevice[0]->setForceAndTorqueAndGripperForce(cVector3d(0, 0, 0), cVector3d(0, 0, 0), 0.0);
		hapticDevice[1]->setForceAndTorqueAndGripperForce(cVector3d(0, 0, 0), cVector3d(0, 0, 0), 0.0);
	}
}


/**
 * To reset the clock variables for keeping track of position for integration.
 */
void resetClockVars(double &a_prev, double &a_current, double &a_delta_t) {
	a_prev = 0.0;
	a_current = 0.0;
	a_delta_t = 0.0;
}

//////////////////////////////////////////////////////// FALCON BUTTON HELPER FUNCTIONS //////////////////////////////////////////////////

/**
 * To detect when a button has been pressed and then released
 *
 */
bool isPressed(bool btn, bool &firstContact) {

	if ((btn == false) && (firstContact == false)) {
		firstContact = false; // nothing happened, keep false
		return false;
	}
	else if ((btn == true) && (firstContact == false)) {
		// rising edge, set first contact
		firstContact = true;
		return false;
	}
	else if ((btn == true) && (firstContact == true)) {
		firstContact = true; // still on contact
		return false;
	}
	else if ((btn == false) && (firstContact == true)) {
		// falling edge detected
		firstContact = false;
		return true;
	}
}



///////////////////////////////////////////////////////// LEVEL GENERATION FUNCTIONS //////////////////////////////////////////////////////

/**
 * Randomly generate the locations and sizes of the pipes and store them
 */
void pipeGenerator(vector<cShapeCylinder*> *a_pipes, unsigned int a_difficulty) {
	a_pipes->clear(); // make sure pipes array is empty

	

	double pipeExtension = 1.0;

	double minGap;
	double maxGap;
	double minDistance;
	double maxDistance;
	double numberOfPipes;

	switch (a_difficulty) {
	case 1:
		minGap = 0.14;
		maxGap = 0.18;
		minDistance = 1.2;
		maxDistance = 2.2;
		numberOfPipes = 20;
		break;
	case 2:
		minGap = 0.10;
		maxGap = 0.12;
		minDistance = 1.2;
		maxDistance = 2.0;
		numberOfPipes = 30;
		break;
	case 3:
		minGap = 0.09;
		maxGap = 0.11;
		minDistance = 1.2;
		maxDistance = 1.8;
		numberOfPipes = 40;
		break;
	}

	cVector3d position(-1.0, 0, 0); // give meter offset

	for (unsigned int i = 0; i < numberOfPipes; i++) {
		double distance = ( (rand() % 101 / 100.0) * (maxDistance - minDistance) ) + minDistance;
		position = cVector3d(position.x() - distance, 0, 0); // update current pipe position placement

		// calculate gap size to be used to determine placement
		double gapSize = ((rand() % 101 / 100.0) * (maxGap - minGap)) + minGap; //

		// get gap position and move into relation of FLOOR and CEILING
		double gapPosition = ( (((rand() % 100 + 1) / 100.0) * (CEILING - FLOOR - gapSize) ) + FLOOR + gapSize / 2.0); // ensure no pipe is of height 0
		
		// create pipes
		cShapeCylinder *topPipe = new cShapeCylinder(pipeRadius, pipeRadius, (CEILING - gapPosition) - (gapSize / 2.0) + pipeExtension);
		topPipe->setLocalPos(cVector3d(position.x(), 0, CEILING - topPipe->getHeight() + pipeExtension)); // set the top pipes position 
		
		cShapeCylinder *bottomPipe = new cShapeCylinder(pipeRadius, pipeRadius, (gapPosition - FLOOR) - (gapSize / 2.0) + pipeExtension);
		bottomPipe->setLocalPos(cVector3d(position.x(), 0, FLOOR - pipeExtension)); // set the bottom pipe position  

		topPipe->m_material->setGreenLight();
		bottomPipe->m_material->setGreenLight();
		
		a_pipes->push_back(topPipe);
		a_pipes->push_back(bottomPipe);
	}

}


/**
 * Randomly generate the turbulence values and store them in the level arrays for turbulence.
 */
void turbulenceGenerator(vector<turbulence*> *a_turbulence, double a_lastX, unsigned int a_difficulty) {
	a_turbulence->clear(); // make sure pipes array is empty

	// how long turblence runs for
	double minLength; 
	double maxLength;
	// how often turbulence happens
	double minDistance;
	double maxDistance;
	unsigned int numTurbulence;
	unsigned int period;
	unsigned int amplitude;
	
	// tweak values
	switch (a_difficulty) {
	case 1:
		minLength = 1.0;
		maxLength = 3.0;
		numTurbulence = 3.0;
		minDistance = 5.0;
		period = 50;
		amplitude = 5;
		break;
	case 2:
		minLength = 3.0;
		maxLength = 5.0;
		numTurbulence = 6.0;
		minDistance = 2.0;
		period = 50; // TODO re-evaluate values
		amplitude = 7.5; // TODO re-evaluate values
		break;
	case 3:
		minLength = 5.0;
		maxLength = 7.0;
		numTurbulence = 9.0;
		minDistance = 1.0;
		period = 50; // TODO re-evaluate values
		amplitude = 10; // TODO re-evaluate values
		break;
	}
	maxDistance = a_lastX / (double)numTurbulence;
	cVector3d position(0, 0, 0);

	for (unsigned int i = 0; i < numTurbulence; i++) {
		if (position.x() <= a_lastX) {
			break;
		} else {
			
			// generate beginning value
			double startTurbulence = ((rand() % 100 + 1) / 100.0) * (maxDistance - minDistance) + position.x();
			
			// generate ending value
			double endTurbulence = startTurbulence - ((rand() % 100 + 1) / 100.0) * (maxLength - minLength);
			position = cVector3d(endTurbulence, 0, 0);

			turbulence *t = new turbulence{startTurbulence, endTurbulence, period, amplitude};
			a_turbulence->push_back(t);

		}
	}

}




////////////////////////////////////////////////// MENU ELEMENT FUNCTIONS ////////////////////////////////////////////////////////////

/**
 * Show the title menu elements
 */
void showtitleMenu() {
	if (!isInitialized) {
		titleMenu->setEnabled(true);
		titleMenu->setShowEnabled(true);
		titleMenu_start->setEnabled(true);
		titleMenu_start->setShowEnabled(true);
		titleMenu_quit->setEnabled(true);
		titleMenu_quit->setShowEnabled(true);
		isInitialized = true;
	}

	titleMenu->setLocalPos((int)(0.5 * (width - titleMenu->getWidth())), (int)(0.5 * (height - titleMenu->getHeight())));
	titleMenu_start->setLocalPos((int)(0.5 * (titleMenu->getWidth() - titleMenu_start->getWidth())), (int)(0.5 * (titleMenu->getHeight() - titleMenu_start->getHeight())) + (0.5 * titleMenu_start->getHeight()));
	titleMenu_quit->setLocalPos((int)(0.5 * (titleMenu->getWidth() - titleMenu_quit->getWidth())), (int)(0.5 * (titleMenu->getHeight() - titleMenu_quit->getHeight())) - titleMenu_start->getHeight()); // translate down 20 pixels
}


/**
 * hide all title (launch menu) elements
 */
void cleanTitle() {
	titleMenu->setEnabled(false);
	titleMenu->setShowEnabled(false);
	titleMenu_start->setEnabled(false);
	titleMenu_start->setShowEnabled(false);
	titleMenu_quit->setEnabled(false);
	titleMenu_quit->setShowEnabled(false);

	// reset the colours
	titleMenu_start->m_fontColor.setRed();
	titleMenu_quit->m_fontColor.setWhite();

	menuState = M_START;

	isInitialized = false; // reset for game state to repopulate the game
}


/**
 * Show all menu elements
 */
void showlevelMenu() {
	if (!isInitialized) {
		levelMenu->setEnabled(true);
		levelMenu->setShowEnabled(true);
		levelMenu_pick->setEnabled(true);
		levelMenu_pick->setShowEnabled(true);
		levelMenu_1->setEnabled(true);
		levelMenu_1->setShowEnabled(true);
		levelMenu_2->setEnabled(true);
		levelMenu_2->setShowEnabled(true);
		levelMenu_3->setEnabled(true);
		levelMenu_3->setShowEnabled(true);
		levelMenu_back->setEnabled(true);
		levelMenu_back->setShowEnabled(true);
		isInitialized = true;
	}

	levelMenu->setLocalPos((int)(0.5 * (width - levelMenu->getWidth())), (int)(0.5 * (height - levelMenu->getHeight())));
	levelMenu_pick->setLocalPos((int)(0.5 * (levelMenu->getWidth() - levelMenu_pick->getWidth())), (int)(levelMenu->getHeight() - (1.5 * levelMenu_pick->getHeight())));
	levelMenu_1->setLocalPos((int)(0.5 * (levelMenu->getWidth() - levelMenu_1->getWidth())), (int)(levelMenu_pick->getLocalPos().y() - (1.5 * levelMenu_1->getHeight())));
	levelMenu_2->setLocalPos((int)(0.5 * (levelMenu->getWidth() - levelMenu_2->getWidth())), (int)(levelMenu_1->getLocalPos().y() - (1.5 * levelMenu_2->getHeight())));
	levelMenu_3->setLocalPos((int)(0.5 * (levelMenu->getWidth() - levelMenu_3->getWidth())), (int)(levelMenu_2->getLocalPos().y() - (1.5 * levelMenu_3->getHeight())));
	levelMenu_back->setLocalPos((int)(0.5 * (levelMenu->getWidth() - levelMenu_back->getWidth())), (int)(levelMenu_3->getLocalPos().y() - (1.5 * levelMenu_back->getHeight())));
}


/**
 * hide the level menu elements
 */
void cleanlevelMenu() {
	levelMenu->setEnabled(false);
	levelMenu->setShowEnabled(false);
	levelMenu_pick->setEnabled(false);
	levelMenu_pick->setShowEnabled(false);
	levelMenu_1->setEnabled(false);
	levelMenu_1->setShowEnabled(false);
	levelMenu_2->setEnabled(false);
	levelMenu_2->setShowEnabled(false);
	levelMenu_3->setEnabled(false);
	levelMenu_3->setShowEnabled(false);
	levelMenu_back->setEnabled(false);
	levelMenu_back->setShowEnabled(false);

	// reset the colours
	levelMenu_1->m_fontColor.setRed();
	levelMenu_2->m_fontColor.setWhite();
	levelMenu_3->m_fontColor.setWhite();
	levelMenu_back->m_fontColor.setWhite();

	levelSelectState = L_EASY;

	isInitialized = false;
}


/**
 * show the menu elements for pausing 
 */
void showpauseMenu() {
	if (!isInitialized) {
		pauseMenu->setEnabled(true);
		pauseMenu_resume->setEnabled(true);
		pauseMenu_restart->setEnabled(true);
		pauseMenu_quit->setEnabled(true);
		isInitialized = true;
	}

	pauseMenu->setLocalPos((int)(0.5 * (width - pauseMenu->getWidth())), (int)(0.5 * (height - pauseMenu->getHeight())));
	pauseMenu_resume->setLocalPos((int)(0.5 * (pauseMenu->getWidth() - pauseMenu_resume->getWidth())), (int)(pauseMenu->getHeight() - ((1.0/4.0) * pauseMenu->getHeight())));
	pauseMenu_restart->setLocalPos((int)(0.5 * (pauseMenu->getWidth() - pauseMenu_restart->getWidth())), (int)(pauseMenu->getHeight() - ((2.0 / 4.0) * pauseMenu->getHeight())));
	pauseMenu_quit->setLocalPos((int)(0.5 * (pauseMenu->getWidth() - pauseMenu_quit->getWidth())), (int)(pauseMenu->getHeight() - ((3.0 / 4.0) * pauseMenu->getHeight())));
}


/**
 * clean up the pause menu, hide the menu elements
 */
void cleanPause() {
	pauseMenu->setEnabled(false);
	pauseMenu_resume->setEnabled(false);
	pauseMenu_restart->setEnabled(false);
	pauseMenu_quit->setEnabled(false);

	// reset the colours
	pauseMenu_resume->m_fontColor.setRed();
	pauseMenu_restart->m_fontColor.setWhite();
	pauseMenu_quit->m_fontColor.setWhite();

	pauseState = P_RESUME;

	isInitialized = false;
}

/**
 * show the menu elements for losing
 */
void showgameoverMenu() {
	double transparency;
	if (!isInitialized) {
		gameoverMenu->setEnabled(true);
		gameoverMenu->setShowEnabled(true);
		loseMessage->setEnabled(true);
		loseMessage->setShowEnabled(true);
		keyagainLose->setEnabled(true);
		keyagainLose->setEnabled(true);
		transparency = 1.0;
		isInitialized = true;
	}

	gameoverMenu->setLocalPos((int)(0.5 * (width - gameoverMenu->getWidth())), (int)(0.5 * (height - gameoverMenu->getHeight())));
	loseMessage->setLocalPos((int)(0.5 * (gameoverMenu->getWidth() - loseMessage->getWidth())), (int)(gameoverMenu->getHeight() - ((1.0 / 3)*(gameoverMenu->getHeight()))));
	keyagainLose->setLocalPos((int)(0.5 * (gameoverMenu->getWidth() - keyagainLose->getWidth())), (int)(gameoverMenu->getHeight() - ((2.0 / 3)*(gameoverMenu->getHeight()))));

	bool hitMax;
	bool hitMin;
	if (transparency == 1.0) {
		hitMax = true;
		hitMin = false;
	}
	else if (transparency == 0.0) {
		hitMax = false;
		hitMin = true;
	}
	if (hitMax = true) transparency -= 0.01;
	else if (hitMin == true) transparency += 0.01;

	keyagainLose->setTransparencyLevel(transparency);

}

/**
 * clean up the lose menu, hide the menu elements
 */
void cleangameoverMenu() {
	gameoverMenu->setEnabled(false);
	gameoverMenu->setShowEnabled(false);
	loseMessage->setEnabled(false);
	loseMessage->setShowEnabled(false);
	keyagainLose->setEnabled(false);
	keyagainLose->setShowEnabled(false);
	isInitialized = false;
}


/**
 * show the menu elements for winning
 */
void showwinMenu() {
	double transparency;
	if (!isInitialized) {
		winMenu->setEnabled(true);
		winMenu->setShowEnabled(true);
		winMessage->setEnabled(true);
		winMessage->setShowEnabled(true);
		keyagainWin->setEnabled(true);
		keyagainWin->setShowEnabled(true);
		transparency = 1.0;
		isInitialized = true;
	}

	winMenu->setLocalPos((int)(0.5 * (width - winMenu->getWidth())), (int)(0.5 * (height - winMenu->getHeight())));
	winMessage->setLocalPos((int)(0.5 * (winMenu->getWidth() - winMessage->getWidth())), (int)(winMenu->getHeight() - ((1.0 / 3)*(winMenu->getHeight()))));
	keyagainWin->setLocalPos((int)(0.5 * (winMenu->getWidth() - keyagainWin->getWidth())), (int)(winMenu->getHeight() - ((2.0 / 3)*(winMenu->getHeight()))));

	bool hitMax;
	bool hitMin;
	if (transparency == 1.0) {
		hitMax = true;
		hitMin = false;
	}
	else if (transparency == 0.0) {
		hitMax = false;
		hitMin = true;
	}
	if (hitMax = true) transparency -= 0.01;
	else if (hitMin == true) transparency += 0.01;

	keyagainWin->setTransparencyLevel(transparency);
}

/**
 * clean up the win menu, hide the menu elements
 */
void cleanwinMenu() {
	winMenu->setEnabled(false);
	winMenu->setShowEnabled(false);
	winMessage->setEnabled(false);
	winMessage->setShowEnabled(false);
	keyagainWin->setEnabled(false);
	keyagainWin->setShowEnabled(false);
	isInitialized = false;
}


/**
 * assigns the turbulence array and level pipes array to the current arrays for level playing
 */
void setLevel(int LEVEL) {
	currentLevel = new vector<cShapeCylinder*>();

	switch (LEVEL) {
	case LEVEL_1:
		//for (cShapeCylinder *p : *lvl1)
		//	currentLevel->push_back(new cShapeCylinder(*p));
		currentLevel = new vector<cShapeCylinder*>(*lvl1); // call copy constructor
		currentTurbulence = new vector<turbulence*>(*lvl1_turbulence);
		break;
	case LEVEL_2:
		for (cShapeCylinder *p : *lvl2)
			currentLevel->push_back(new cShapeCylinder(*p));
		currentLevel = new vector<cShapeCylinder*>(*lvl2); // call copy constructor
		currentTurbulence = new vector<turbulence*>(*lvl2_turbulence);
		break;
	case LEVEL_3:
		for (cShapeCylinder *p : *lvl3)
			currentLevel->push_back(new cShapeCylinder(*p));
		currentTurbulence = new vector<turbulence*>(*lvl3_turbulence);
		break;
	}
	for (cShapeCylinder *p : *currentLevel) { // make the cylinders visible
		world->addChild(p);
		p->setShowEnabled(true);
		p->setEnabled(true);
	}

	// set the bird up
	birdBody->body->setEnabled(true, true);
	birdBody->body->setShowEnabled(true, true);
	birdBody->body->setLocalPos(0.0, 0.0, 0.0);

	// reset wing rotation
	birdBody->rightWing->setLocalRot(cMatrix3d(1.0, 0.0, 0.0, 0)); // apply transformation
	birdBody->leftWing->setLocalRot(cMatrix3d(1.0, 0.0, 0.0, 0)); // apply transformation

	// set win and lose to false
	winState = false;
	loseState = false;

	// reset the camera
	camera->set(cVector3d(0.5, 0.0, 0.0),    // camera position (eye)
				cVector3d(0.0, 0.0, 0.0),    // look at position (target)
				cVector3d(0.0, 0.0, 1.0));   // direction of the (up) vector

	// update score label
	SCORE = 0;
	scorePanel->setShowEnabled(true, true);
	scorePanel->setEnabled(true, true);

	scorePanel->setLocalPos((int)(0.5 * (width - scorePanel->getWidth())), 5);
	scoreLabel->setText("SCORE: " + cStr(SCORE)); // update score label
	scoreLabel->setLocalPos((int)(0.5 * (scorePanel->getWidth() - scoreLabel->getWidth())), (int)(0.5 * (scorePanel->getHeight() - (scoreLabel->getHeight()))));
	
}



/**
 * turn off level pipes being visible, clear the current turbulence list
 */
void cleanLevel() {
	for (cShapeCylinder *p : *currentLevel) {
		p->setShowEnabled(false);
		p->setEnabled(false);
		world->removeChild(p); // remove from world
	}

	currentTurbulence->clear();
	delete currentTurbulence; // clean up memory
	currentLevel->clear();
	delete currentLevel; // clean up memory

	INIT_PROCESS = false; // reset so that level can go through initialization again
	GAME_STARTED = false;
	
	birdBody->body->setEnabled(false, true);
	birdBody->body->setShowEnabled(false, true);

	// reset camera perspective
	cameraAngle = CAMERA_1;

	scorePanel->setEnabled(false, true);
	scorePanel->setShowEnabled(false, true);
}

/**
 * Update the camera angle depending on what the user selects. Either 3rd person,
 * first person, or side scroll view.
 */
void updateCamera(CAMERA_ANGLE a_perspective, cVector3d a_pos) {
	switch (a_perspective) {
	case CAMERA_1: // 3rd person
		camera->set(cVector3d(a_pos.x(), 0.0, 0.0) + cVector3d(0.5, 0.0, 0.0),				// camera position (eye)
					cVector3d(a_pos.x(), 0.0, 0.0),											// look at position (target)
					cVector3d(0.0, 0.0, 1.0));												// direction of the (up) vector
		break;
	case CAMERA_2: // 1st person
		camera->set(cVector3d(a_pos.x(), 0.0, a_pos.z()) + cVector3d(-0.04, 0.0, 0.0),     // camera position (eye)
					cVector3d(a_pos.x() - 2.0, 0.0, a_pos.z()),									// look at forward
					cVector3d(0.0, 0.0, 1.0));												// direction of the (up) vector
		break;
	case CAMERA_3: // side scroll
		camera->set(cVector3d(a_pos.x(), 0.0, 0.0) + cVector3d(0.0, 1.5, 0.0),				// camera position (eye)
					cVector3d(a_pos.x(), 0.0, 0.0),											// look at position (target)
					cVector3d(0.0, 0.0, 1.0));												// direction of the (up) vector
		break;
	}
}
