#X11
.extern XOpenDisplay

# glX
.extern glXQueryVersion glXChooseFBConfig glXGetVisualFromFBConfig glXGetFBConfigAttrib glXGetVisualFromFBConfig glXQueryExtensionsString glXGetProcAddressARB glXCreateContextAttribsARB glXMakeCurrent glXMakeCurrent glXDestroyContext glXSwapBuffers

# gl
.extern glCreateShader glShaderSource glCompileShader glCreateProgram glAttachShader glLinkProgram glUseProgram glDeleteShader glGenVertexArrays glGenBuffers glBindVertexArray glBindBuffer glBufferData glVertexAttribPointer glEnableVertexAttribArray glGetUniformLocation glClearColor glClear glUniform1f glUniform2f glDrawElements

.global _start

.text
_start:
# write(1, message, 13)
mov     $1, %rax                # system call 1 is write
mov     $1, %rdi                # file handle 1 is stdout
mov     $message, %rsi          # address of string to output
mov     $13, %rdx               # number of bytes
syscall                         # invoke operating system to do the write

# exit(0)
mov     $60, %rax               # system call 60 is exit
xor     %rdi, %rdi              # we want return code 0
syscall                         # invoke operating system to exit
message:
.ascii  "Hello, world\n"
