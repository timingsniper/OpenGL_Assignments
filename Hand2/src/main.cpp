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
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion_utils.hpp>

GLFWwindow* window;

glm::vec3 gPosition1(-1.5f, 0.0f, 0.0f);
glm::vec3 gOrientation1;
 
glm::vec3 gPosition2( 1.5f, 0.0f, 0.0f);
glm::quat gOrientation2;

glm::vec3 desiredDir = gPosition1-gPosition2;
glm::vec3 desiredUp = glm::vec3(0.0f, 1.0f, 0.0f); 
//glm::quat targetOrientation = normalize(LookAt(desiredDir, desiredUp));
//gOrientation2 = RotateTowards(gOrientation2, targetOrientation, 1.0f*deltaTime);

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix() {
    return ViewMatrix;
}
glm::mat4 getProjectionMatrix() {
    return ProjectionMatrix;
}
glm::quat LookAt(glm::vec3 direction, glm::vec3 desiredUp) {

    if (length2(direction) < 0.0001f)
        return glm::quat();

    // Recompute desiredUp so that it's perpendicular to the direction
    // You can skip that part if you really want to force desiredUp
    glm::vec3 right = cross(direction, desiredUp);
    desiredUp = cross(right, direction);

    // Find the rotation between the front of the object (that we assume towards +Z,
    // but this depends on your model) and the desired direction
    glm::quat rot1 = RotationBetweenVectors(glm::vec3(0.0f, 0.0f, 1.0f), direction);
    // Because of the 1rst rotation, the up is probably completely screwed up. 
    // Find the rotation between the "up" of the rotated object, and the desired up
    glm::vec3 newUp = rot1 * glm::vec3(0.0f, 1.0f, 0.0f);
    glm::quat rot2 = RotationBetweenVectors(newUp, desiredUp);

    // Apply them
    return rot2 * rot1; // remember, in reverse order.
}

// Like SLERP, but forbids rotation greater than maxAngle (in radians)
// In conjunction to LookAt, can make your characters 
glm::quat RotateTowards(glm::quat q1, glm::quat q2, float maxAngle) {

    if (maxAngle < 0.001f) {
        // No rotation allowed. Prevent dividing by 0 later.
        return q1;
    }

    float cosTheta = dot(q1, q2);

    // q1 and q2 are already equal.
    // Force q2 just to be sure
    if (cosTheta > 0.9999f) {
        return q2;
    }

    // Avoid taking the long path around the sphere
    if (cosTheta < 0) {
        q1 = q1 * -1.0f;
        cosTheta *= -1.0f;
    }

    float angle = acos(cosTheta);

    // If there is only a 2?difference, and we are allowed 5?
    // then we arrived.
    if (angle < maxAngle) {
        return q2;
    }

    // This is just like slerp(), but with a custom t
    float t = maxAngle / angle;
    angle = maxAngle;
    std::cout << t << std::endl;
    glm::quat res = (sin((1.0f - t) * angle) * q1 + sin(t * angle) * q2) / sin(angle);
    res = normalize(res);
    return res;

}

glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest) {
    start = normalize(start);
    dest = normalize(dest);

    float cosTheta = dot(start, dest);
    glm::vec3 rotationAxis;

    if (cosTheta < -1 + 0.001f) {
        // special case when vectors in opposite directions :
        // there is no "ideal" rotation axis
        // So guess one; any will do as long as it's perpendicular to start
        // This implementation favors a rotation around the Up axis,
        // since it's often what you want to do.
        rotationAxis = cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
        if (length2(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
            rotationAxis = cross(glm::vec3(1.0f, 0.0f, 0.0f), start);

        rotationAxis = normalize(rotationAxis);
        return angleAxis(glm::radians(180.0f), rotationAxis);
    }

    // Implementation from Stan Melax's Game Programming Gems 1 article
    rotationAxis = cross(start, dest);

    float s = sqrt((1 + cosTheta) * 2);
    float invs = 1 / s;

    return glm::quat(
        s * 0.5f,
        rotationAxis.x * invs,
        rotationAxis.y * invs,
        rotationAxis.z * invs
    );


}

// Initial position : on +Z
glm::vec3 position = glm::vec3(1, 3, 30);
// Initial horizontal angle : toward -Z
float horizontalAngle = 10.0f;
// Initial vertical angle : none
float verticalAngle = -10.0f;
// Initial Field of View
float initialFoV = 80.0f;

float speed = 3.0f; // 3 units / second
float mouseSpeed = 0.005f;

/*void computeMatricesFromInputs() {

    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    float deltaTime = float(currentTime - lastTime);

    // Get mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Reset mouse position for next frame
    glfwSetCursorPos(window, 800 / 2, 800 / 2);

    // Compute new orientation
    horizontalAngle += mouseSpeed * float(800 / 2 - xpos);
    verticalAngle += mouseSpeed * float(800 / 2 - ypos);

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
    glm::vec3 viewPoint = position+direction;
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

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        viewPoint = glm::vec3(0,0,0);
    }

    
    float FoV = initialFoV;

    // Projection matrix : 45?Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    ProjectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 100.0f);
    // Camera matrix
    ViewMatrix = glm::lookAt(
        position,           
        viewPoint, 
        up                 
    );

  
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
    double lastFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        passed_time = (float) glfwGetTime();

        // --- You may edit below ---
        float passed_time = (float) glfwGetTime();

        // --- You may edit below ---
        
        // Measure speed
        float deltaTime = (float)(passed_time - lastFrameTime);

        float period1 = 2.4f;
        float time_in_period1 = fmod(deltaTime, period1);
        // * angle: 0 -> PI/3 -> 0
        float thumb_angle1 = abs(time_in_period1 / (period1 * 0.5f) - 1.0f);
        //(float(int(deltaTime) % 5) / 4)

        if (int(deltaTime) % 10 > 4) {
            glm::vec3 desiredDir = gPosition1 + (fmod(deltaTime, period1) / period1) * (gPosition2 - gPosition1);
            glm::vec3 desiredUp = glm::vec3(0.0f, 1.0f, 0.0f); // +Y

        // Compute the desired orientation
            glm::quat targetOrientation = normalize(LookAt(desiredDir, desiredUp));
            // And interpolate
            
            gOrientation2 = RotateTowards(gOrientation2, targetOrientation, 1.0f * float(deltaTime));

        }
        else {
            glm::vec3 desiredDir = gPosition2 + (fmod(deltaTime, period1) / period1) * (gPosition1 - gPosition2);
            glm::vec3 desiredUp = glm::vec3(0.0f, 1.0f, 0.0f); // +Y

        // Compute the desired orientation
            glm::quat targetOrientation = normalize(LookAt(desiredDir, desiredUp));

            // And interpolate
            gOrientation2 = RotateTowards(gOrientation2, targetOrientation, 1.0f * float(deltaTime));

        }

        glm::mat4 RotationMatrix = mat4_cast(gOrientation2);
        glm::mat4 ScalingMatrix = glm::mat4(1.0);
        glm::mat4 ModelMatrix = RotationMatrix * ScalingMatrix;
        glm::mat4 ViewMatrix = glm::lookAt(glm::fvec3(.0f, .0f, -1.f), glm::fvec3(.0f, .0f, .0f), glm::fvec3(.0f, 1.f, .0f));
       /* computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();
        glm::mat4 ModelMatrix = glm::mat4(1.0);
        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;*/

    
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
        modifier["metacarpals"] = glm::rotate(glm::mat4(), thumb_angle - 0.5f, glm::fvec3(0.0, 1.0, 0.0));
        
        //Gesture 2. "Rabbit"
		/*modifier["index_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["index_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["thumb_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["thumb_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["ring_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["pinky_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));*/

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
		modifier["ring_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));*/

		//Gesture 5. OK
		/*modifier["index_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["index_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["index_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["thumb_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
        modifier["thumb_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));*/

		//Gesture 6. Handgun
		/*modifier["metacarpals"] = glm::rotate(glm::mat4(), ninety, glm::fvec3(0.0, 1.0, 0.0));
	
		modifier["middle_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["ring_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["pinky_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));*/

		//Gesture 7. Shooting Railgun (Misaka Mikoto)
		/*modifier["metacarpals"] = glm::rotate(glm::mat4(), ninety, glm::fvec3(0.0, 1.0, 0.0));
		
        modifier["thumb_intermediate_phalange"] = glm::rotate(glm::mat4(), one, glm::fvec3(0.0, 1.0, 0.0));
        modifier["thumb_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		//modifier["thumb_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		//modifier["thumb_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["middle_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["middle_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["ring_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["ring_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));

		modifier["pinky_proximal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_intermediate_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_distal_phalange"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));
		modifier["pinky_fingertip"] = glm::rotate(glm::mat4(), thumb_angle, glm::fvec3(0.0, 0.0, 1.0));*/
		

		

        // --- You may edit above ---

        float ratio;
        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glClearColor(0.5, 0.5, 0.5, 1.0);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
         glm::fmat4 mvp = glm::ortho(-12.5f * ratio, 12.5f * ratio, -5.f, 20.f, -20.f, 20.f);
         glm::mat4 MVP = mvp * ViewMatrix * ModelMatrix;
       
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