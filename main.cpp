#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shaders.hh"

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

// retina
//#define RESOLUTIONX 2280
//#define RESOLUTIONY 1800

//1080p
#define RESOLUTIONX 1920
#define RESOLUTIONY 1080
//// half 1080p
//#define RESOLUTIONX 960
//#define RESOLUTIONY 420
//// 900p
//#define RESOLUTIONX 1600
//#define RESOLUTIONY 900
//720p
//#define RESOLUTIONX 1280
//#define RESOLUTIONY 720

#define END 10

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
Display *display;
Window win;
GLXContext ctx;
Colormap cmap;
void initWin() {
    display = XOpenDisplay(NULL);
    static int visual_attribs[] = {
	GLX_X_RENDERABLE    , True,
	GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
	GLX_RENDER_TYPE     , GLX_RGBA_BIT,
	GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
	GLX_RED_SIZE        , 8,
	GLX_GREEN_SIZE      , 8,
	GLX_BLUE_SIZE       , 8,
	GLX_ALPHA_SIZE      , 8,
	GLX_DEPTH_SIZE      , 24,
	GLX_STENCIL_SIZE    , 8,
	GLX_DOUBLEBUFFER    , True,
	//GLX_SAMPLE_BUFFERS  , 1,
	//GLX_SAMPLES         , 4,
	None
    };

    int glx_major, glx_minor;

    // FBConfigs were added in GLX version 1.3.

    int fbcount;
    GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
    int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;
    int i;
    for (i=0; i<fbcount; ++i) {
	XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbc[i] );
	if (vi) {
	    int samp_buf, samples;
	    glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
	    glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLES       , &samples  );
	    if ( best_fbc < 0 || (samp_buf && samples > best_num_samp)) {
		best_fbc = i, best_num_samp = samples;
	    }
	    if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp ) {
		worst_fbc = i, worst_num_samp = samples;
	    }
	}
	XFree( vi );
    }
    GLXFBConfig bestFbc = fbc[best_fbc];
    XFree(fbc);
    // Get a visual
    XVisualInfo *vi = glXGetVisualFromFBConfig(display, bestFbc);
    XSetWindowAttributes swa;
    swa.colormap = cmap = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);
    swa.background_pixmap = None ;
    swa.border_pixel      = 0;
    swa.event_mask        = StructureNotifyMask;
    swa.override_redirect = true;
    //win = XCreateWindow(display, RootWindow(display, vi->screen), 0, 15, RESOLUTIONX, RESOLUTIONY, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel|CWColormap|CWEventMask|CWOverrideRedirect, &swa); // force fixed resolution
    win = XCreateWindow(display, RootWindow(display, vi->screen), 0, 15, RESOLUTIONX, RESOLUTIONY, 0, vi->depth, InputOutput, vi->visual, CWBorderPixel|CWColormap|CWEventMask, &swa);
    XFree(vi);
    XStoreName(display, win, STRINGIZE(DEMONAME));
    XMapWindow(display, win);
    // Get the default screen's GLX extension list
    const char *glxExts = glXQueryExtensionsString(display, DefaultScreen(display));
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddressARB((const GLubyte *) "glXCreateContextAttribsARB");

    ctx = 0;
    static int context_attribs[] = {
	GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
	GLX_CONTEXT_MINOR_VERSION_ARB, 3,
	//GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
	None
    };
    ctx = glXCreateContextAttribsARB( display, bestFbc, 0,
	    True, context_attribs );
    XSync(display, False);
    glXMakeCurrent(display, win, ctx);
}

short shouldContinue(float time) {
    if (time < END) {
	XResizeWindow(display, win, RESOLUTIONX, RESOLUTIONY);
	return 1;
    } else {
	glXMakeCurrent(display, 0, 0 );
	glXDestroyContext(display, ctx);

	XDestroyWindow(display, win );
	XFreeColormap(display, cmap );
	XCloseDisplay(display);
	return 0;
    }
}

void swapBuffers() {
    glXSwapBuffers(display, win);
}

GLuint initGL() {
    glewExperimental = GL_TRUE;
    glewInit();
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* sptr = vert_vert;
    glShaderSource(vertexShader, 1, &sptr, &vert_vert_len);
    glCompileShader(vertexShader);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    sptr = frag_frag;
    glShaderSource(fragmentShader, 1, &sptr, &frag_frag_len);
    glCompileShader(fragmentShader);

    // Link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLfloat vertices[12] = {
	-1.0f,  1.0f, 0.0f,  // Top Left
	1.0f,  1.0f, 0.0f, // Top Right
	1.0f, -1.0f, 0.0f, // Bottom Right
	-1.0f, -1.0f, 0.0f// Bottom Left
    };
    GLuint indices[6] = {  // Note that we start from 0!
	0, 1, 2,
	0, 3, 2
    };
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


//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//void _start() {
#include <sys/time.h>
#include <iostream>
int main() {
    initWin();
    GLuint shaderProgram = initGL();
    GLint timeLoc = glGetUniformLocation(shaderProgram, "u_time");
    GLint resLoc = glGetUniformLocation(shaderProgram, "u_resolution");
    //GLint mouseLoc = glGetUniformLocation(shaderProgram, "u_mouse");
    float uTime = 0.0f;
    timeval base;
    timeval stamp;
    gettimeofday(&base, 0);
    //float uTime = 0.0f;
    while(shouldContinue(uTime)) {
	//asm ("movl $96,%eax\n" "mov %0,%rdi\n" "mov $0,%rsi\n" : "=r" (&stamp)); // gettimeofday syscall
	gettimeofday(&stamp, 0); // 96
	uTime = (float((stamp.tv_usec - base.tv_usec) / 1000000.0f)) + float(stamp.tv_sec - base.tv_sec);
	glClearColor(0.9f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUniform1f(timeLoc, uTime);
	glUniform2f(resLoc, RESOLUTIONX, RESOLUTIONY);

	// Draw our first triangle
	//glDrawArrays(GL_POINTS, 0, 4);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	swapBuffers();
    }
    //asm ("movl $1,%eax\n" "xor %ebx,%ebx\n" "int $128\n");
}
