// Hand Example
// Author: Yi Kangrui <yikangrui@pku.edu.cn>
// Modified by Percy Joonwoo Jang, 1800094804
//#define DIFFUSE_TEXTURE_MAPPING

#include "gl_env.h"
#define _CRT_SECURE_NO_WARNINGS
#include <cstdlib>
#include <cstdio>


#ifndef M_PI
#define M_PI (3.1415926535897932)
#endif


#include <iostream>

#include "skeletal_mesh.h"
#include <gl\glut.h>
#include <gl\glew.h>

#include <glm/gtc/matrix_transform.hpp>

GLFWwindow* window;

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix() {
    return ViewMatrix;
}
glm::mat4 getProjectionMatrix() {
    return ProjectionMatrix;
}

// Initial position : on +Z
glm::vec3 position = glm::vec3(0, 0, 30);
// Initial horizontal angle : toward -Z
float horizontalAngle = 3.14f;
// Initial vertical angle : none
float verticalAngle = 45.0f;
// Initial Field of View
float initialFoV = 45.0f;

float speed = 3.0f; // 3 units / second
float mouseSpeed = 0.005f;

void computeMatricesFromInputs() {

    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);

    // Get mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Reset mouse position for next frame
    glfwSetCursorPos(window, 1024 / 2, 768 / 2);

    // Compute new orientation
    horizontalAngle += mouseSpeed * float(1024 / 2 - xpos);
    verticalAngle += mouseSpeed * float(768 / 2 - ypos);

    // Direction : Spherical coordinates to Cartesian coordinates conversion
    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );

    // Right vector
    glm::vec3 right = glm::vec3(
        sin(horizontalAngle - 3.14f / 2.0f),
        0,
        cos(horizontalAngle - 3.14f / 2.0f)
    );

    // Up vector
    glm::vec3 up = glm::cross(right, direction);

    // Move forward
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        position += direction * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        position -= direction * deltaTime * speed;
    }
    // Strafe right
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        position += right * deltaTime * speed;
    }
    // Strafe left
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        position -= right * deltaTime * speed;
    }

    float FoV = initialFoV;// - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires setting up a callback for this. It's a bit too complicated for this beginner's tutorial, so it's disabled instead.

    // Projection matrix : 45?Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    ProjectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 100.0f);
    // Camera matrix
    ViewMatrix = glm::lookAt(
        position,           // Camera is here
        position + direction, // and looks here : at the same position, plus "direction"
        up                  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // For the next frame, the "last time" will be "now"
    lastTime = currentTime;
}

namespace SkeletalAnimation {
    const char *vertex_shader_330 =
            "#version 330 core\n"
            "const int MAX_BONES = 100;\n"
            "uniform mat4 u_bone_transf[MAX_BONES];\n"
            "uniform mat4 u_mvp;\n"
            "layout(location = 0) in vec3 in_position;\n"
            "layout(location = 1) in vec2 in_texcoord;\n"
            "layout(location = 2) in vec3 in_normal;\n"
            "layout(location = 3) in ivec4 in_bone_index;\n"
            "layout(location = 4) in vec4 in_bone_weight;\n"
            "out vec2 pass_texcoord;\n"
            "void main() {\n"
            "    float adjust_factor = 0.0;\n"
            "    for (int i = 0; i < 4; i++) adjust_factor += in_bone_weight[i] * 0.25;\n"
            "    mat4 bone_transform = mat4(1.0);\n"
            "    if (adjust_factor > 1e-3) {\n"
            "        bone_transform -= bone_transform;\n"
            "        for (int i = 0; i < 4; i++)\n"
            "            bone_transform += u_bone_transf[in_bone_index[i]] * in_bone_weight[i] / adjust_factor;\n"
            "	 }\n"
            "    gl_Position = u_mvp * bone_transform * vec4(in_position, 1.0);\n"
            "    pass_texcoord = in_texcoord;\n"
            "}\n";

    const char *fragment_shader_330 =
            "#version 330 core\n"
            "uniform sampler2D u_diffuse;\n"
            "in vec2 pass_texcoord;\n"
            "out vec4 out_color;\n"
            "void main() {\n"
            #ifdef DIFFUSE_TEXTURE_MAPPING
            "    out_color = vec4(texture(u_diffuse, pass_texcoord).xyz, 1.0);\n"
            #else
            "    out_color = vec4(pass_texcoord, 0.0, 1.0);\n"
            #endif
            "}\n";
}

static void error_callback(int error, const char *description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main(int argc, char *argv[]) {
    //GLFWwindow *window;
    GLuint vertex_shader, fragment_shader, program;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__ // for macos
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(800, 800, "OpenGL output", NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    if (glewInit() != GLEW_OK)
        exit(EXIT_FAILURE);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &SkeletalAnimation::vertex_shader_330, NULL);
    glCompileShader(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &SkeletalAnimation::fragment_shader_330, NULL);
    glCompileShader(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    int linkStatus;
    if (glGetProgramiv(program, GL_LINK_STATUS, &linkStatus), linkStatus == GL_FALSE)
        std::cout << "Error occured in glLinkProgram()" << std::endl;

    SkeletalMesh::Scene &sr = SkeletalMesh::Scene::loadScene("Hand", "Hand.fbx");
    if (&sr == &SkeletalMesh::Scene::error)
        std::cout << "Error occured in loadMesh()" << std::endl;

    sr.setShaderInput(program, "in_position", "in_texcoord", "in_normal", "in_bone_index", "in_bone_weight");

    float passed_time;
    SkeletalMesh::SkeletonModifier modifier;

    glEnable(GL_DEPTH_TEST);
    while (!glfwWindowShouldClose(window)) {
        passed_time = (float) glfwGetTime();

        // --- You may edit below ---

        // Compute the MVP matrix from keyboard and mouse input
        computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();
        glm::mat4 ModelMatrix = glm::mat4(1.0);
        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

    
        // Example: Rotate the hand
        // * turn around every 4 seconds
        float metacarpals_angle = passed_time * (M_PI / 4.0f);
        // * target = metacarpals
        // * rotation axis = (1, 0, 0)
        modifier["metacarpals"] = glm::rotate(glm::mat4(), metacarpals_angle, glm::fvec3(1.0, 0.0, 0.0));

        /**********************************************************************************\
        *
        * To animate fingers, modify modifier["HAND_SECTION"] each frame,
        * where HAND_SECTION can only be one of the bone names in the Hand's Hierarchy.
        *
        * A virtual hand's structure is like this: (slightly DIFFERENT from the real world)
        *    5432 1
        *    ....        1 = thumb           . = fingertip
        *    |||| .      2 = index finger    | = distal phalange
        *    $$$$ |      3 = middle finger   $ = intermediate phalange
        *    #### $      4 = ring finger     # = proximal phalange
        *    OOOO#       5 = pinky           O = metacarpals
        *     OOO
        * (Hand in the real world -> https://en.wikipedia.org/wiki/Hand)
        *
        * From the structure we can infer the Hand's Hierarchy:
        *	- metacarpals
        *		- thumb_proximal_phalange
        *			- thumb_intermediate_phalange
        *				- thumb_distal_phalange
        *					- thumb_fingertip
        *		- index_proximal_phalange
        *			- index_intermediate_phalange
        *				- index_distal_phalange
        *					- index_fingertip
        *		- middle_proximal_phalange
        *			- middle_intermediate_phalange
        *				- middle_distal_phalange
        *					- middle_fingertip
        *		- ring_proximal_phalange
        *			- ring_intermediate_phalange
        *				- ring_distal_phalange
        *					- ring_fingertip
        *		- pinky_proximal_phalange
        *			- pinky_intermediate_phalange
        *				- pinky_distal_phalange
        *					- pinky_fingertip
        *
        * Notice that modifier["HAND_SECTION"] is a local transformation matrix,
        * where (1, 0, 0) is the bone's direction, and apparently (0, 1, 0) / (0, 0, 1)
        * is perpendicular to the bone.
        * Particularly, (0, 0, 1) is the rotation axis of the nearer joint.
        *
        \**********************************************************************************/

        // Example: Animate the index finger
        // * period = 2.4 seconds
        float period = 2.4f;
        float time_in_period = fmod(passed_time, period);
        // * angle: 0 -> PI/3 -> 0
        float thumb_angle = abs(time_in_period / (period * 0.5f) - 1.0f) * (M_PI / 3.0);
		float one = 1;
		float ninety = (M_PI/2.0);
        // * target = proximal phalange of the index
        // * rotation axis = (0, 0, 1)

        //Gesture 1. Hello World! Waving hand
        //modifier["metacarpals"] = glm::rotate(glm::mat4(), thumb_angle - 0.5f, glm::fvec3(0.0, 1.0, 0.0));
        
        //Gesture 2. "Rabbit"
		/*modifier["index_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["index_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["thumb_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));

		modifier["ring_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));

		modifier["pinky_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));*/

		//Gesture 3. Fist and back
		/*modifier["thumb_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["index_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["index_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["index_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["index_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["middle_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["ring_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["pinky_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));*/

		//Gesture 4. 666
		/*modifier["metacarpals"] = glm::rotate(glm::mat4(), thumb_angle - 0.5f, glm::fvec3(0.0, 1.0, 0.0));
	
		modifier["index_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["index_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["index_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["index_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));

		modifier["middle_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));

		modifier["ring_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));*/

		//Gesture 5. OK
		/*modifier["index_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["index_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["index_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["thumb_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["thumb_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));*/

		//Gesture 6. Handgun
		/*modifier["metacarpals"] = glm::rotate(glm::mat4(), ninety, glm::fvec3(0.0, 1.0, 0.0));
		modifier["index_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["index_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["index_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["index_distal_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

        modifier["thumb_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["thumb_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["middle_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));

		modifier["ring_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));

		modifier["pinky_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));*/

		//Gesture 7. Shooting Railgun (Misaka Mikoto)
		/*modifier["metacarpals"] = glm::rotate(glm::mat4(), ninety, glm::fvec3(0.0, 1.0, 0.0));
		
        modifier["thumb_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 1.0, 0.0));
        modifier["thumb_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		//modifier["thumb_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		//modifier["thumb_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["middle_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));

		modifier["ring_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));

		modifier["pinky_proximal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_distal_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_fingertip"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 0.0, 1.0));*/
		

		

        // --- You may edit above ---

        float ratio;
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glClearColor(0.5, 0.5, 0.5, 1.0);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glm::fmat4 mvp = glm::ortho(-12.5f * ratio, 12.5f * ratio, -5.f, 20.f, -20.f, 20.f)
                         *
                         glm::lookAt(glm::fvec3(.0f, .0f, -1.f), glm::fvec3(.0f, .0f, .0f), glm::fvec3(.0f, 1.f, .0f));
        glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, &MVP[0][0]);
        //glUniformMatrix4fv(glGetUniformLocation(program, "u_mvp"), 1, GL_FALSE, (const GLfloat *) &mvp);
        glUniform1i(glGetUniformLocation(program, "u_diffuse"), SCENE_RESOURCE_SHADER_DIFFUSE_CHANNEL);
        SkeletalMesh::Scene::SkeletonTransf bonesTransf;
        sr.getSkeletonTransform(bonesTransf, modifier);
        if (!bonesTransf.empty())
            glUniformMatrix4fv(glGetUniformLocation(program, "u_bone_transf"), bonesTransf.size(), GL_FALSE,
                               (float *) bonesTransf.data());
        sr.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    SkeletalMesh::Scene::unloadScene("Hand");

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}