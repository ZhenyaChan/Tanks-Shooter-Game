using namespace std;

#include "vgl.h"
#include "LoadShaders.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtx\rotate_vector.hpp"
#include "..\SOIL\src\SOIL.h"
#include <iostream>
#include <vector>


// SceneNode class and derived classes Tank, Wheel to create tank
class SceneNode {
	glm::vec3 m_location;
public:
	// wheels
	std::vector<SceneNode> m_children;
	// one-argument constructor
	SceneNode(glm::vec3 location) : m_location(location){}

	// function to get the location
	glm::vec3 getLocation() { 
		return m_location; 
	}
	
	// function to set the location
	void setLocation(glm::vec3 location) {
		m_location = location;
	}

	// function to draw the object
	void draw(float scale, GLuint texture, GLuint rowStart, GLuint rowEnd, bool isEnemy);
	void render(bool isEnemy);
};

// wheel class derived from scenenode
class Wheel : public SceneNode {
public:
	Wheel(glm::vec3 location) : SceneNode(location) {}
	~Wheel() {}
};

// tank class derived from scenenode
class Tank : public SceneNode {
	bool m_isEnemy;
public:
	Tank(glm::vec3 location, bool isEnemy) : SceneNode(location), m_isEnemy(isEnemy) {
		// positioning 4 wheels relatively to tank body
		Wheel w1(glm::vec3(0.5, 0.5, 0.1));
		Wheel w2(glm::vec3(-0.5, 0.5, 0.1));
		Wheel w3(glm::vec3(0.5, -0.5, 0.1));
		Wheel w4(glm::vec3(-0.5, -0.5, 0.1));
		// push wheels to scenenode
		m_children.push_back(w1);
		m_children.push_back(w2);
		m_children.push_back(w3);
		m_children.push_back(w4);
	}
	// check if the tank is enemy
	bool isEnemy() { return this->m_isEnemy; }
};


// bullet class
class Bullet {
	glm::vec3 m_location;
	glm::vec3 m_enemyLocation;
	int m_bulletTime;
	bool m_isEnemyGun;
public:
	// 3-argument constructor
	Bullet(glm::vec3 location, glm::vec3 enemyLocation, bool isEnemyGun) : m_location(location), m_enemyLocation(enemyLocation), m_bulletTime(0), m_isEnemyGun(isEnemyGun) {}

	// get the location of the bullet
	glm::vec3 getLocation() { 
		return this->m_location; 
	}

	// set the location
	void setLocation(glm::vec3 location) { 
		this->m_location = location; 
	}

	// set the enemy bullet location
	void setEnemyLocation(glm::vec3 eLocation) { 
		this->m_enemyLocation = eLocation; 
	}

	// get the enemy bullet location
	glm::vec3 getEnemyLocation() {
		return this->m_enemyLocation;
	}

	// set bullet time
	void setBulletLifetime(int dTime) {
		this->m_bulletTime += dTime;
	}

	// get bullet time since spawn (lifespan)
	int getBulletTime() { 
		return this->m_bulletTime; 
	}

	// check if its enemy gun or not
	bool isEnemyGun() { 
		return this->m_isEnemyGun; 
	}
};


// Global variables added
std::vector<Bullet> bulletList;		// player's bullets
std::vector<Bullet> enemyBulletList;// enemies bullets
std::vector<Tank> sceneGraph;

GLuint spawnTime = 0;				// time count to spawn new tanks
GLuint enemyFireBulletTimer = 0;	// time count for enemy bullet
GLuint clearBulletsTimer = 0;		
GLuint playerScore = 0;				// player score
bool alive = true;					// player status
bool playerWins = false;			// win or lose condition
int timer = 0; // timer to count the beginning of the game


GLfloat local = 0.0;		// wheels rotation speed
glm::vec3 randomLocations[10];// random locations to for directions of new spawn tanks

enum VAO_IDs { Triangles, NumVAOs };
enum Buffer_IDs { ArrayBuffer};
enum Attrib_IDs { vPosition = 0 };

const GLint NumBuffers = 2;
GLuint VAOs[NumVAOs];
GLuint Buffers[NumBuffers];
GLuint location;
GLuint cam_mat_location;
GLuint proj_mat_location;
GLuint texture[4];	//Array of pointers to textrure data in VRAM. We use two textures in this example.


const GLuint NumVertices = 56;

//Height of camera (player) from the level
float height = 0.8f;

//Player motion speed for movement and pitch/yaw
float travel_speed = 300.0f;		//Motion speed
float mouse_sensitivity = 0.01f;	//Pitch/Yaw speed

//Used for tracking mouse cursor position on screen
int x0 = 0;	
int y_0 = 0;
 
//Transformation matrices and camera vectors
glm::mat4 model_view;
glm::vec3 unit_z_vector = glm::vec3(0, 0, 1);
glm::vec3 cam_pos = glm::vec3(0.0f, 0.0f, height);
glm::vec3 forward_vector = glm::vec3(1, 1, 0);	// Forward vector is parallel to the level at all times (No pitch)

//The direction which the camera is looking, at any instance
glm::vec3 looking_dir_vector = glm::vec3(1, 1, 0);
glm::vec3 up_vector = unit_z_vector;
glm::vec3 side_vector = glm::cross(up_vector, forward_vector);


//Used to measure time between two frames
int oldTimeSinceStart = 0;
int deltaTime;


//Helper function to generate a random float number within a range
float randomFloat(float a, float b) {
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

// inititializing buffers, coordinates, setting up pipeline, etc.
void init(void) {
	glEnable(GL_DEPTH_TEST);

	//Normalizing all vectors
	up_vector = glm::normalize(up_vector);
	forward_vector = glm::normalize(forward_vector);
	looking_dir_vector = glm::normalize(looking_dir_vector);
	side_vector = glm::normalize(side_vector);

	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};

	// Random location array with random locations to use for tanks direction
	for (int i = 0; i < 10; i++) {
		randomLocations[i] = glm::vec3(randomFloat(-40, 40), randomFloat(-40, 40), randomFloat(-40, 40));
	}

	GLuint program = LoadShaders(shaders);
	glUseProgram(program);	//My Pipeline is set up

	//Since we use texture mapping, to simplify the task of texture mapping, 
	//and to clarify the demonstration of texture mapping, we consider 4 vertices per face.
	//Overall, we will have 24 vertices and we have 4 vertices to render the sky (a large square).
	//Therefore, we'll have 28 vertices in total.
	GLfloat vertices[NumVertices][3] = {
		{ -100.0, -100.0, 0.0 }, //Plane to walk on and a sky
		{ 100.0, -100.0, 0.0 },
		{ 100.0, 100.0, 0.0 },
		{ -100.0, 100.0, 0.0 },

		{ -0.45, -0.70, 0.01 }, // bottom face
		{ 0.45, -0.70, 0.01 },
		{ 0.45, 0.70, 0.01 },
		{ -0.45, 0.70, 0.01 },

		{ -0.45, -0.45, 0.7 }, //top face 
		{ 0.45, -0.45, 0.7 },
		{ 0.45, 0.45, 0.7 },
		{ -0.45, 0.45, 0.7 },

		{ 0.45, -0.70, 0.01 }, //left face
		{ 0.45, 0.70, 0.01 },
		{ 0.45, 0.45, 0.7 },
		{ 0.45, -0.45, 0.7 },

		{ -0.45, -0.70, 0.01 }, //right face
		{ -0.45, 0.70, 0.01 },
		{ -0.45, 0.45, 0.7 },
		{ -0.45, -0.45,0.7 },

		{ -0.45, 0.70, 0.01 }, //front face 
		{ 0.45, 0.70, 0.01 },
		{ 0.45, 0.45, 0.7 },
		{ -0.45, 0.45, 0.7 },

		{ -0.45, -0.70, 0.01 }, //back face
		{ 0.45, -0.70, 0.01 },
		{ 0.45, -0.45, 0.7 },
		{ -0.45, -0.45, 0.7 },

		// Hexagon
		{ 0.45, 0.70 ,-0.01 }, // bottom face
		{ -0.45, 0.70 ,-0.01 },
		{ -0.45, -0.70 ,-0.01 },
		{ 0.45, -0.70 ,-0.01 },

		{ -0.45, -0.45 ,-0.7 }, //top face 
		{ 0.45, -0.45 ,-0.7 },
		{ 0.45, 0.45 ,-0.7 },
		{ -0.45, 0.45 ,-0.7 },

		{ 0.45, -0.70 , -0.01 }, //left face
		{ 0.45, 0.70 , -0.01 },
		{ 0.45, 0.45 ,-0.7 },
		{ 0.45, -0.45 ,-0.7 },

		{ -0.45, -0.70, -0.01 }, //right face
		{ -0.45, 0.70 , -0.01 },
		{ -0.45, 0.45 ,-0.7 },
		{ -0.45, -0.45 ,-0.7 },

		{ -0.45, 0.70 , -0.01 }, //front face 
		{ 0.45, 0.70 , -0.01 },
		{ 0.45, 0.45 ,-0.7 },
		{ -0.45, 0.45 ,-0.7 },

		{ -0.45, -0.70 , -0.01 }, //back face
		{ 0.45, -0.70, -0.01 },
		{ 0.45, -0.45 ,-0.7 },
		{ -0.45, -0.45 ,-0.7 },
	};

	//These are the texture coordinates for the second texture
	GLfloat textureCoordinates[56][2] = {
		0.0f, 0.0f,
		200.0f, 0.0f,
		200.0f, 200.0f,
		0.0f, 200.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		// hexagon
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
	};

	//Creating our texture:
	//This texture is loaded from file. To do this, we use the SOIL (Simple OpenGL Imaging Library) library.
	//When using the SOIL_load_image() function, make sure the you are using correct patrameters, or else, your image will NOT be loaded properly, or will not be loaded at all.

	// Added additional textures for firing bullets, wheels, tank body.

	GLint width1, height1;
	GLint width2, height2;
	GLint width3, height3;
	GLint width4, height4;

	unsigned char* textureData1 = SOIL_load_image("grass.png", &width1, &height1, 0, SOIL_LOAD_RGB);
	unsigned char* textureData2 = SOIL_load_image("fire.png", &width2, &height2, 0, SOIL_LOAD_RGB);
	unsigned char* textureData3 = SOIL_load_image("ammo.png", &width3, &height3, 0, SOIL_LOAD_RGB);	
	unsigned char* textureData4 = SOIL_load_image("tankwheel.png", &width4, &height4, 0, SOIL_LOAD_RGB);

	glGenBuffers(2, Buffers);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindAttribLocation(program, 0, "vPosition");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, Buffers[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordinates), textureCoordinates, GL_STATIC_DRAW);
	glBindAttribLocation(program, 1, "vTexCoord");
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(1);

	location = glGetUniformLocation(program, "model_matrix");
	cam_mat_location = glGetUniformLocation(program, "camera_matrix");
	proj_mat_location = glGetUniformLocation(program, "projection_matrix");
	///////////////////////TEXTURE SET UP////////////////////////
	//Allocating two buffers in VRAM
	glGenTextures(4, texture);

	//First Texture: 

	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData1);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	//And now, second texture: 
	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[1]);

	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData2);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// 3rd texture:
	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width3, height3, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData3);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// 4th texture:
	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width4, height4, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData4);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}


// renders level
void draw_level() {
	//Select the first texture (grass.png) when drawing the first geometry (floor)
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glDrawArrays(GL_QUADS, 0, 4);
}

// helper function to draw a cube
void drawCube(float scale) {
	model_view = glm::scale(model_view, glm::vec3(scale, scale, scale));
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
	//Select the second texture (fire.png) when drawing the second geometry (cube)
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glDrawArrays(GL_QUADS, 4, 24);
}


// Tank Functions for drawing the Tank body with wheels
// create tank object and push to sceneGraph
void spawnTank(bool isEnemy) {
	if (isEnemy) {
		glm::vec3 tankSpawnLocation(randomFloat(-50, 50), randomFloat(-50, 50), 0.0);
		Tank tank(tankSpawnLocation, true); // create tank at random location and set it as enemy
		sceneGraph.push_back(tank);
	}
}

// draw tank body called by draw() function
void drawTank(float scale, GLuint texture, GLuint rowStart, GLuint rowEnd) {
	model_view = glm::scale(model_view, glm::vec3(scale, scale, scale));
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
	glBindTexture(GL_TEXTURE_2D, texture);
	glDrawArrays(GL_QUADS, rowStart, rowEnd);
}

// render/dispatch tanks stored in sceneGraph
void drawTanks() {
	for (int i = 0; i < sceneGraph.size(); i++) {
		sceneGraph.at(i).render(sceneGraph.at(i).isEnemy());
	}
}

// render tank body called by drawTanks()
void SceneNode::render(bool isEnemy) {
	glm::mat4 temp = model_view;
	model_view = glm::translate(model_view, this->getLocation());
	draw(3, texture[2], 4, 24, isEnemy);
	model_view = temp;
}

// draw tank and wheels called by render() function
void SceneNode::draw(float scale, GLuint tex, GLuint rowStart, GLuint rowEnd, bool isEnemy) {
	drawTank(scale, tex, rowStart, rowEnd);

	// draw wheels of the tank
	if (isEnemy) {
		for (int i = 0; i < m_children.size(); i++) {
			glm::mat4 temp = model_view;
			glm::vec3 vec = m_children.at(i).getLocation();
			model_view = glm::translate(model_view, vec);
			model_view = glm::rotate(model_view, local, glm::vec3(-0.5, 0, 0));
			m_children.at(i).draw(0.3, texture[3], 4, 59, isEnemy);	// vertices for hexagon texture wheels
			model_view = temp;
		}
	}
}

//Final Exam code Added 
//Rationale: Collision functions and Bullet functions for drawing the bullets and updating them
// check if there is collision between objects
void checkCollision(int index, bool enemyFired) {
	if (enemyFired) {
		// current player's position range
		GLfloat highX = cam_pos.x + 1.7;
		GLfloat lowX = cam_pos.x - 1.7;
		GLfloat highY = cam_pos.y + 1.7;
		GLfloat lowY = cam_pos.y - 1.7;

		// Check if enemy bullet hit the current player position range
		if (enemyBulletList.size() > 0) {
			// if the enemy bullet hit players position, set status to dead and print game over
			if (enemyBulletList.at(index).getLocation().x <= highX && enemyBulletList.at(index).getLocation().x >= lowX && enemyBulletList.at(index).getLocation().y <= highY && enemyBulletList.at(index).getLocation().y >= lowY) {
				alive = false;
				enemyBulletList.erase(enemyBulletList.begin() + index);
				cout << "You were shot by enemy Tank.\n Game Over!" << endl;
			}
			else {
				// enemy bullet erased after 6 seconds
				if (enemyBulletList.at(index).getBulletTime() >= 6000) {
					enemyBulletList.erase(enemyBulletList.begin() + index);
				}
			}
		}
	}
	else if (!enemyFired) { // Check if player bullet hit any enemies
		for (int i = 0; i < sceneGraph.size(); i++) {
			if (sceneGraph.at(i).isEnemy()) {
				// Enemy current location
				GLfloat highX = sceneGraph.at(i).getLocation().x + 2.0;
				GLfloat lowX = sceneGraph.at(i).getLocation().x - 2.0;
				GLfloat highY = sceneGraph.at(i).getLocation().y + 2.0;
				GLfloat lowY = sceneGraph.at(i).getLocation().y - 2.0;

				// check each player's bullet if it hit an enemy
				if (bulletList.size() > 0) {
					if (bulletList.at(index).getLocation().x <= highX && bulletList.at(index).getLocation().x >= lowX && bulletList.at(index).getLocation().y <= highY && bulletList.at(index).getLocation().y >= lowY) {
						playerScore++;
						cout << "You destroyed 1 tank.\n Score: " << playerScore << endl;
						sceneGraph.erase(sceneGraph.begin() + i);
					}
					else {
						// erase bullet after 2 seconds
						if (bulletList.at(index).getBulletTime() >= 2000) {
							bulletList.erase(bulletList.begin() + index);
						}
					}

				}
			}
		}
	}
}

// draw bullets at current position
void drawBullets(bool isEnemy) {
	if (isEnemy) {
		for (size_t i = 0; i < enemyBulletList.size(); i++) {
			model_view = glm::translate(model_view, enemyBulletList.at(i).getLocation());
			glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
			drawCube(0.60); // set scale of cube
			model_view = glm::mat4(1.0);
		}
	}
	else if (!isEnemy) {
		for (size_t i = 0; i < bulletList.size(); i++) {
			model_view = glm::translate(model_view, bulletList.at(i).getLocation());
			glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
			drawCube(0.60); // set scale of cube
			model_view = glm::mat4(1.0);
		}
	}
}

// enemy tank fires bullet at player position
void fireBullet(int index) {
	bool isEnemyGun = true;
	if (enemyFireBulletTimer >= 1000) { // Enemy fires every 1 seconds
		Bullet bullet(sceneGraph.at(index).getLocation(), looking_dir_vector, isEnemyGun);
		enemyBulletList.push_back(bullet);
		drawBullets(true);
		enemyFireBulletTimer = 0;
	}
}

// The enemy range to target the player
bool enemyInRange(int enemyIdx) {
	bool inRange = false;
	GLfloat highX = cam_pos.x + 100.0;
	GLfloat lowX = cam_pos.x - 100.0;
	GLfloat highY = cam_pos.y + 100.0;
	GLfloat lowY = cam_pos.y - 100.0;

	// if enemy is within range then return true else return false
	if (sceneGraph.at(enemyIdx).getLocation().x <= highX && sceneGraph.at(enemyIdx).getLocation().x >= lowX
		&& sceneGraph.at(enemyIdx).getLocation().y <= highY && sceneGraph.at(enemyIdx).getLocation().y >= lowY) {
		inRange = true;
	}
	return inRange;
}

// update tank location and direction in the scene
void updateTank() {
	for (int i = 0; i < sceneGraph.size(); i++) {
		if (sceneGraph.at(i).isEnemy()) {

			// if enemy is near cam_pos follow the cam_pos else move in random directions
			if (enemyInRange(i)) {
				// move toward cam_pos
				glm::vec3 tank_dir = sceneGraph.at(i).getLocation() - glm::vec3(0.0005, 0.0005, 0.0005) * (looking_dir_vector + glm::vec3(1, 1, 1));

				tank_dir.z = 0.2;
				sceneGraph.at(i).setLocation(tank_dir);

				// Enemy will fire at player
				fireBullet(i);
			}
			else if (!enemyInRange(i)) {
				// move in random directions with random speeds
				glm::vec3 tank_dir = sceneGraph.at(i).getLocation() +
					glm::vec3(randomFloat(0.00010, 0.00005), randomFloat(0.00010, 0.00005), randomFloat(0.00010, 0.00005)) * randomLocations[i];

				tank_dir.z = 0.2;
				sceneGraph.at(i).setLocation(tank_dir);
			}
		}
	}
}


// update the location of the bullets in the scene
void updateBullet() {
	for (int i = 0; i < enemyBulletList.size(); i++) {
		glm::vec3 bullet_dir = enemyBulletList.at(i).getLocation() - glm::vec3(0.01, 0.01, 0.01) * enemyBulletList.at(i).getEnemyLocation(); // current player position
		bullet_dir.z = 0.5;
		enemyBulletList.at(i).setLocation(bullet_dir);
		drawBullets(true);

		enemyBulletList.at(i).setBulletLifetime(deltaTime);
		checkCollision(i, true);
	}

	for (int i = 0; i < bulletList.size(); i++) {
		glm::vec3 bullet_dir = bulletList.at(i).getLocation() + glm::vec3(0.05, 0.05, 0.05) * bulletList.at(i).getEnemyLocation();
		bulletList.at(i).setLocation(bullet_dir);
		drawBullets(false);

		bulletList.at(i).setBulletLifetime(deltaTime);
		checkCollision(i, false);
	}
}


//---------------------------------------------------------------------
//
// display
//
//---------------------------------------------------------------------
void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	model_view = glm::mat4(1.0);
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

	//The 3D point in space that the camera is looking
	glm::vec3 look_at = cam_pos + looking_dir_vector;

	glm::mat4 camera_matrix = glm::lookAt(cam_pos, look_at, up_vector);
	glUniformMatrix4fv(cam_mat_location, 1, GL_FALSE, &camera_matrix[0][0]);

	glm::mat4 proj_matrix = glm::frustum(-0.01f, +0.01f, -0.01f, +0.01f, 0.01f, 100.0f);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, &proj_matrix[0][0]);

	// draw objects, tanks, keep drawing bullets
	draw_level();
	drawTanks();
	updateBullet();

	glFlush();
}


void keyboard(unsigned char key, int x, int y) {
	// if player wins or lose, he cannot move
	if (!playerWins) {
		if (alive) {
			if (key == 'a') {
				//Moving camera along opposit direction of side vector
				cam_pos += side_vector * travel_speed * ((float)deltaTime) / 1000.0f;
			}
			if (key == 'd') {
				//Moving camera along side vector
				cam_pos -= side_vector * travel_speed * ((float)deltaTime) / 1000.0f;
			}
			if (key == 'w') {
				//Moving camera along forward vector. To be more realistic, we use X=V.T equation in physics
				cam_pos += forward_vector * travel_speed * ((float)deltaTime) / 1000.0f;
			}
			if (key == 's') {
				//Moving camera along backward (negative forward) vector. To be more realistic, we use X=V.T equation in physics
				cam_pos -= forward_vector * travel_speed * ((float)deltaTime) / 1000.0f;
			}
			// shooting functionality for player
			// create bullet object and push to bulletlist collection
			if (key == 'f' || key == 'F') {
				bool isPlayer = false;
				Bullet bullet(cam_pos, looking_dir_vector, isPlayer); // player bullet boolean
				bulletList.push_back(bullet);
			}
		}
	}
}

// Controlling Pitch with vertical mouse movement
void mouse(int x, int y) {
	//: if player wins or lose, he cannot move
	if (!playerWins) {
		if (alive) {
			//Controlling Yaw with horizontal mouse movement
			int delta_x = x - x0;

			//The following vectors must get updated during a yaw movement
			forward_vector = glm::rotate(forward_vector, -delta_x * mouse_sensitivity, unit_z_vector);
			looking_dir_vector = glm::rotate(looking_dir_vector, -delta_x * mouse_sensitivity, unit_z_vector);
			side_vector = glm::rotate(side_vector, -delta_x * mouse_sensitivity, unit_z_vector);
			up_vector = glm::rotate(up_vector, -delta_x * mouse_sensitivity, unit_z_vector);
			x0 = x;

			//The following vectors must get updated during a pitch movement
			int delta_y = y - y_0;
			glm::vec3 tmp_up_vec = glm::rotate(up_vector, delta_y * mouse_sensitivity, side_vector);
			glm::vec3 tmp_looking_dir = glm::rotate(looking_dir_vector, delta_y * mouse_sensitivity, side_vector);

			//The dot product is used to prevent the user from over-pitch (pitching 360 degrees)
			//The dot product is equal to cos(theta), where theta is the angle between looking_dir and forward vector
			GLfloat dot_product = glm::dot(tmp_looking_dir, forward_vector);

			//If the angle between looking_dir and forward vector is between (-90 and 90) degress 
			if (dot_product > 0) {
				up_vector = glm::rotate(up_vector, delta_y * mouse_sensitivity, side_vector);
				looking_dir_vector = glm::rotate(looking_dir_vector, delta_y * mouse_sensitivity, side_vector);
			}
			y_0 = y;
		}
	}
}

void idle() {
	// check the player status all the time, update tank and bullet locations and update timers
	
	if (playerScore == 5) {
		playerWins = true;
		cout << "Congratulations! You won!" << endl;
	}

	else if (!alive) {
		cout << "Game over! You were shot!" << endl;
	}

	if (timer >= 10000) {
		alive = false;
		cout << "Game over! Time out!" << endl;
	}
	if (spawnTime >= 1000) { // tank spawn every 1 second
		spawnTank(true);
		spawnTime = 0;
	}

	local += 0.005;
	updateTank();
	updateBullet();

	//Calculating the delta time between two frames
	//We will use this delta time when moving forward (in keyboard function)
	int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	deltaTime = timeSinceStart - oldTimeSinceStart;
	oldTimeSinceStart = timeSinceStart;
	enemyFireBulletTimer += deltaTime; // counting time since the bullet spawn
	spawnTime += deltaTime; // counting time since the spawn, once 1 seconds are passed, spawn a tank
	timer += deltaTime;
	glutPostRedisplay();
}

//---------------------------------------------------------------------
// main
//---------------------------------------------------------------------
int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowSize(1024, 1024);
	glutCreateWindow("Final Project - Tran Quang Dung");
	glewInit();	//Initializes the glew and prepares the drawing pipeline.
	init();
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);
	glutPassiveMotionFunc(mouse);
	glutMainLoop();
}
