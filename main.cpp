#define GLEW_STATIC
#include <GL/glew.h>
#include <stdio.h>

#include "shaders.hh"

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define SUPERSAMPLE 1

//retina
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
#define RESOLUTIONX 1280
#define RESOLUTIONY 720

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
unsigned int gBuffer;
GLuint sceneShader;
GLuint postProcessShader;
unsigned int gPositionDepth;
unsigned int gNormal;
unsigned int gColour;
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

void initGL() {
    glewExperimental = GL_TRUE;
    glewInit();



    // -- MRT
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // - position color buffer
    glGenTextures(1, &gPositionDepth);
    glBindTexture(GL_TEXTURE_2D, gPositionDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, RESOLUTIONX * SUPERSAMPLE, RESOLUTIONY * SUPERSAMPLE, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPositionDepth, 0);

    // - normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, RESOLUTIONX * SUPERSAMPLE, RESOLUTIONY * SUPERSAMPLE, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // - color + specular color buffer
    glGenTextures(1, &gColour);
    glBindTexture(GL_TEXTURE_2D, gColour);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, RESOLUTIONX * SUPERSAMPLE, RESOLUTIONY * SUPERSAMPLE, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColour, 0);

    // - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);


    // ??
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, RESOLUTIONX * SUPERSAMPLE, RESOLUTIONY * SUPERSAMPLE);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	    puts("Framebuffer not complete!");
    // ----------------




    // -- scene Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* sptr = vert_vert;
    glShaderSource(vertexShader, 1, &sptr, &vert_vert_len);
    glCompileShader(vertexShader);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    sptr = scene_frag;
    glShaderSource(fragmentShader, 1, &sptr, &scene_frag_len);
    glCompileShader(fragmentShader);

    // Link shaders
    sceneShader = glCreateProgram();
    glAttachShader(sceneShader, vertexShader);
    glAttachShader(sceneShader, fragmentShader);
    glLinkProgram(sceneShader);
    glUseProgram(sceneShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    // ----------------



    // -- postprocess Shader
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    sptr = vert_vert;
    glShaderSource(vertexShader, 1, &sptr, &vert_vert_len);
    glCompileShader(vertexShader);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    sptr = post_frag;
    glShaderSource(fragmentShader, 1, &sptr, &post_frag_len);
    glCompileShader(fragmentShader);

    // Link shaders
    postProcessShader = glCreateProgram();
    glAttachShader(postProcessShader, vertexShader);
    glAttachShader(postProcessShader, fragmentShader);
    glLinkProgram(postProcessShader);
    glUseProgram(postProcessShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    // ----------------



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
}


//#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//void _start() {
#include <sys/time.h>
#include <iostream>
int main() {
    initWin();
    initGL();
    glUseProgram(sceneShader);
    GLint sceneTimeLoc = glGetUniformLocation(sceneShader, "u_time");
    GLint sceneResLoc = glGetUniformLocation(sceneShader, "u_resolution");

    GLint postTimeLoc = glGetUniformLocation(sceneShader, "u_time");
    GLint postResLoc = glGetUniformLocation(sceneShader, "u_resolution");
    //GLint mouseLoc = glGetUniformLocation(shaderProgram, "u_mouse");
    GLint gposLoc = glGetUniformLocation(postProcessShader, "gPositionDepth");
    GLint gNormLoc = glGetUniformLocation(postProcessShader, "gNormal");
    GLint gColLoc = glGetUniformLocation(postProcessShader, "gColour");

    float uTime = 0.0f;
    timeval base;
    timeval stamp;
    gettimeofday(&base, 0);
    //float uTime = 0.0f;
    while(shouldContinue(uTime)) {
	//asm ("movl $96,%eax\n" "mov %0,%rdi\n" "mov $0,%rsi\n" : "=r" (&stamp)); // gettimeofday syscall
	gettimeofday(&stamp, 0); // 96
	uTime = (float((stamp.tv_usec - base.tv_usec) / 1000000.0f)) + float(stamp.tv_sec - base.tv_sec);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(sceneShader);
	glUniform1f(sceneTimeLoc, uTime);
	glUniform2f(sceneResLoc, RESOLUTIONX * SUPERSAMPLE, RESOLUTIONY * SUPERSAMPLE);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(postProcessShader);
	glUniform1f(postTimeLoc, uTime);
	glUniform2f(postResLoc, RESOLUTIONX, RESOLUTIONY);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPositionDepth);
	glUniform1i(gposLoc, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glUniform1i(gNormLoc, 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gColour);
	glUniform1i(gColLoc, 2);

	//glClear(GL_COLOR_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glUseProgram(0);
	swapBuffers();
    }
    //asm ("movl $1,%eax\n" "xor %ebx,%ebx\n" "int $128\n");
}
