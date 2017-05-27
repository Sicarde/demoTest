#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <chrono>
#include "shaders.hh"

#ifdef GLFW_MODE
#include <GLFW/glfw3.h>

GLFWwindow* window = nullptr;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	glfwSetWindowShouldClose(window, GL_TRUE);
}

void initWin(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window = glfwCreateWindow(1920, 1080, "demo", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glewExperimental = GL_TRUE;
    glewInit();
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glfwSetKeyCallback(window, key_callback);
}

bool shouldContinue() {
	glfwPollEvents();
	return !glfwWindowShouldClose(window);
}

void swapBuffers() {
    glfwSwapBuffers(window);
}

#elif SDL12_MODE
#include <SDL/SDL.h>

void initWin() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(1920,1080,0,SDL_OPENGL);
    SDL_ShowCursor(SDL_DISABLE);

    glewExperimental = GL_TRUE;
    glewInit();
}

SDL_Event event;
bool shouldContinue() {
    SDL_PollEvent(&event);
    return (event.key.keysym.sym != SDLK_ESCAPE) || (event.type == SDL_QUIT);
}
void swapBuffers() {
    SDL_GL_SwapBuffers();
}
#elif SDL2_MODE
#include <SDL2/SDL.h>
SDL_Window* window;
void initWin() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

     window = SDL_CreateWindow("demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

     SDL_GLContext context = SDL_GL_CreateContext(window);
     glewExperimental = GL_TRUE;
     glewInit();
}
SDL_Event event;
bool shouldContinue() {
    SDL_PollEvent(&event);
    return (event.key.keysym.sym != SDLK_ESCAPE) || (event.type == SDL_QUIT);
}
void swapBuffers() {
    SDL_GL_SwapWindow(window);
}
#else
#error "TODO: IMPLEMENT XCB"
//xcb.freedesktop.org/opengl/
#endif

#define LOGSIZE 1024
GLuint initGL() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* sptr = vert_glsl;
    glShaderSource(vertexShader, 1, &sptr, &vert_glsl_len);
    glCompileShader(vertexShader);
    // Check for compile time errors
    GLint success;
    GLchar infoLog[LOGSIZE];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
	glGetShaderInfoLog(vertexShader, LOGSIZE, NULL, infoLog);
	std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    sptr = frag_glsl;
    glShaderSource(fragmentShader, 1, &sptr, &frag_glsl_len);
    glCompileShader(fragmentShader);
    // Check for compile time errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
	glGetShaderInfoLog(fragmentShader, LOGSIZE, NULL, infoLog);
	std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    //// Geometry shader
    //GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    //sptr = geometry_glsl;
    //glShaderSource(geometryShader, 1, &sptr, &geometry_glsl_len);
    //glCompileShader(geometryShader);
    //// Check for compile time errors
    //glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &success);
    //if (!success)
    //{
    //    glGetShaderInfoLog(geometryShader, LOGSIZE, NULL, infoLog);
    //    std::cout << "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n" << infoLog << std::endl;
    //}

#if TESSELATION
    // Tesselation Control shader
    GLuint tesselationControlShader = glCreateShader(GL_TESS_CONTROL_SHADER);
    sptr = tesselationCtrl_glsl;
    glShaderSource(tesselationControlShader, 1, &sptr, &tesselationCtrl_glsl_len);
    glCompileShader(tesselationControlShader);
    // Check for compile time errors
    glGetShaderiv(tesselationControlShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(tesselationControlShader, LOGSIZE, NULL, infoLog);
        std::cout << "ERROR::SHADER::TESSELATIONCONTROL::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // Tesselation Evaluation shader
    GLuint tesselationEvaluationShader = glCreateShader(GL_TESS_EVALUATION_SHADER);
    sptr = tesselationEval_glsl;
    glShaderSource(tesselationEvaluationShader, 1, &sptr, &tesselationEval_glsl_len);
    glCompileShader(tesselationEvaluationShader);
    // Check for compile time errors
    glGetShaderiv(tesselationEvaluationShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(tesselationEvaluationShader, LOGSIZE, NULL, infoLog);
        std::cout << "ERROR::SHADER::TESSELATIONEVALUATION::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
#endif

    // Link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    //glAttachShader(shaderProgram, geometryShader);
#if TESSELATIONEVALUATION
    glAttachShader(shaderProgram, tesselationControlShader);
    glAttachShader(shaderProgram, tesselationEvaluationShader);
#endif
    //glProgramParameteriEXT(shaderProgram, GL_GEOMETRY_INPUT_TYPE_EXT, GL_LINES);
    //glProgramParameteriEXT(shaderProgram, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_LINE_STRIP);
    //glProgramParameteriEXT(shaderProgram, GL_GEOMETRY_VERTICES_OUT_EXT, 2);
    glLinkProgram(shaderProgram);
    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
	glGetProgramInfoLog(shaderProgram, LOGSIZE, NULL, infoLog);
	std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    //glDeleteShader(geometryShader);

    GLfloat vertices[] = {
	-1.0f,  1.0f, 0.0f,   // Top Left
	1.0f,  1.0f, 0.0f,  // Top Right
	1.0f, -1.0f, 0.0f,  // Bottom Right
	-1.0f, -1.0f, 0.0f  // Bottom Left
    };
    GLuint indices[] = {  // Note that we start from 0!
	0, 1, 2,
	0, 2, 3
    };

    //GLuint indices[] = {  // Note that we start from 0!
    //    //0, 1, 2,  // First Triangle
    //    //2, 3, 0   // Second Triangle
    //    0, 1, 1, 2//, 2, 0, //isoline first triangle
    //    //2, 3, 3, 0, 0, 2 //isoline second triangle
    //};
    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    //glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind
    return shaderProgram;
}

int main() {
    initWin();
    GLuint shaderProgram = initGL();
    glm::mat4 projectionMat = glm::perspective(glm::radians(90.0f), 1920.0f / 1080.0f, 1.0f, 100.0f);
    glm::mat4 viewProjMat = projectionMat * glm::lookAt(glm::vec3(0.0f, 6.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    GLint timeLoc = glGetUniformLocation(shaderProgram, "u_time");
    GLint resLoc = glGetUniformLocation(shaderProgram, "u_resolution");
    //GLint mouseLoc = glGetUniformLocation(shaderProgram, "u_mouse");
    GLint mvpLoc = glGetUniformLocation(shaderProgram, "mvp");
    GLint rotationLoc = glGetUniformLocation(shaderProgram, "rotation");
    auto originTime = std::chrono::high_resolution_clock::now();

    while(shouldContinue()) {
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	float uTime = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - originTime).count();
	//glm::mat4 mvp = projectionMat * glm::lookAt(glm::vec3(20.0f, 20.0f, 20.0), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), uTime / 500.0f, glm::vec3(1.0f, 0.0f, 1.0f));
	//glm::mat4 mvp = projectionMat * glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));// * glm::rotate(glm::mat4(1.0f), uTime / 500.0f, glm::vec3(1.0f, 0.0f, 1.0f));
	//glm::mat4 rotationGeometry = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 1.0f, 1.0f));
	//glm::mat4 rotationGeometry = glm::rotate(glm::mat4(1.0f), uTime / 500.0f, glm::vec3(0.0f, 1.0f, 1.0f));
	//glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, glm::value_ptr(rotationGeometry));
	//glm::mat4 mvp = projectionMat * glm::lookAt(glm::vec3(0.0f, 0.0f, -3.0), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)); //from top
	//glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
	glUniform1f(timeLoc, uTime / 1000.0f);
	glUniform2f(resLoc, 1920, 1080);

	// Draw our first triangle
	//glDrawArrays(GL_POINTS, 0, 4);
#if TESSELATION
	glPatchParameteri(GL_PATCH_VERTICES, 2);
	glDrawElements(GL_PATCHES, 4, GL_UNSIGNED_INT, 0);
#else
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
#endif

	swapBuffers();
    }
    return 0;
}
