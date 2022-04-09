#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h> //#include <GLES3/gl3.h>
//#include <GLES2/gl2ext.h>
#include "GL/glew.h" //#include "GL/gl3w.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>


// Unsigned Integers
typedef     uint8_t             u8;
typedef     uint16_t            u16;
typedef     uint32_t            u32;
typedef     uint64_t            u64;

// Signed Integer
typedef     int8_t              s8;
typedef     int16_t             s16;
typedef     int32_t             s32;
typedef     int64_t             s64;

// Floating Point 
typedef     float               f32;
typedef     double              f64;

struct Vertex
{
    f32 x;
    f32 y;
};

pthread_t streamingThreadHandle;
pthread_mutex_t streaming_sync;

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

bool running = true;


u32 dwNumQuads = 0;

GLuint shaderProgram;
GLint posAttrib;
GLuint vertexBuffers[16];

const GLchar* vertexSource =    
    "attribute vec2 position;                 \n"                                 
    "void main()                              \n"
    "{                                        \n"
    "  gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";
const GLchar* fragmentSource =
    "precision mediump float;													 \n"
    "void main()                                  								 \n"
    "{                                            								 \n"
    "  gl_FragColor = vec4(gl_FragCoord.x/640.0, gl_FragCoord.y/480.0, 0.5, 1.0);\n"
    "}";


const char *GetStringErrorCode(EMSCRIPTEN_RESULT res)
{
    if(res == EMSCRIPTEN_RESULT_SUCCESS)
    {
        return "EMSCRIPTEN_RESULT_SUCCESS";
    }
    if(res == EMSCRIPTEN_RESULT_DEFERRED)
    {
        return "EMSCRIPTEN_RESULT_DEFERRED";
    }
    if(res == EMSCRIPTEN_RESULT_NOT_SUPPORTED)
    {
        return "EMSCRIPTEN_RESULT_NOT_SUPPORTED";
    }
    if(res == EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED)
    {
        return "EMSCRIPTEN_RESULT_FAILED_NOT_DEFERRED";
    }
    if(res == EMSCRIPTEN_RESULT_INVALID_TARGET)
    {
        return "EMSCRIPTEN_RESULT_INVALID_TARGET";
    }
    if(res == EMSCRIPTEN_RESULT_UNKNOWN_TARGET)
    {
        return "EMSCRIPTEN_RESULT_UNKNOWN_TARGET";
    }
    if(res == EMSCRIPTEN_RESULT_INVALID_PARAM)
    {
        return "EMSCRIPTEN_RESULT_INVALID_PARAM";
    }
    if(res == EMSCRIPTEN_RESULT_FAILED)
    {
        return "EMSCRIPTEN_RESULT_FAILED";
    }
    if(res == EMSCRIPTEN_RESULT_NO_DATA)
    {
        return "EMSCRIPTEN_RESULT_NO_DATA";
    }
    return "UNKNOWN EMPSCRIPTEN TYPE";
}

void *StreamingThread( void *a_pParameter )
{
    EMSCRIPTEN_RESULT res;
    if( EMSCRIPTEN_RESULT_SUCCESS != (res = emscripten_webgl_make_context_current(ctx) ) )
    {

        printf("Could not make webgl context current in streaming thread %s\n",GetStringErrorCode(res));
        running = false;
        return 0;
    }
    Vertex vertexScratchBuffer[6];
    u32 dwRow = 0;
    u32 dwCol = 0;
    while( running )
    {
        emscripten_sleep(50);
        vertexScratchBuffer[0].x =(dwRow *(2.0f/4.0f)) -1.0f;
        vertexScratchBuffer[0].y =(dwCol *(2.0f/4.0f)) -1.0f;
        vertexScratchBuffer[1].x =((dwRow) *(2.0f/4.0f)) -1.0f;
        vertexScratchBuffer[1].y =((dwCol+1) *(2.0f/4.0f)) -1.0f;
        vertexScratchBuffer[2].x =((dwRow+1) *(2.0f/4.0f)) -1.0f;
        vertexScratchBuffer[2].y =((dwCol) *(2.0f/4.0f)) -1.0f;

        vertexScratchBuffer[3].x =((dwRow) *(2.0f/4.0f)) -1.0f;
        vertexScratchBuffer[3].y =((dwCol+1) *(2.0f/4.0f)) -1.0f;
        vertexScratchBuffer[4].x =((dwRow+1) *(2.0f/4.0f)) -1.0f;
        vertexScratchBuffer[4].y =((dwCol+1) *(2.0f/4.0f)) -1.0f;
        vertexScratchBuffer[5].x =((dwRow+1) *(2.0f/4.0f)) -1.0f;
        vertexScratchBuffer[5].y =((dwCol) *(2.0f/4.0f)) -1.0f;

        if( dwNumQuads < 16 )
        {
			printf( "[%u] Tri 1: <%f, %f> <%f, %f> <%f, %f>\n", dwNumQuads, vertexScratchBuffer[0].x, vertexScratchBuffer[0].y, vertexScratchBuffer[1].x, vertexScratchBuffer[1].y, vertexScratchBuffer[2].x, vertexScratchBuffer[2].y);
        	printf( "[%u] Tri 2: <%f, %f> <%f, %f> <%f, %f>\n", dwNumQuads, vertexScratchBuffer[3].x, vertexScratchBuffer[3].y, vertexScratchBuffer[4].x, vertexScratchBuffer[4].y, vertexScratchBuffer[5].x, vertexScratchBuffer[5].y);
            ++dwCol;
            if( dwCol == 4)
            {
                dwCol = 0;
                ++dwRow;
            }
            pthread_mutex_lock(&streaming_sync);
            glGenBuffers(1,&vertexBuffers[dwNumQuads]);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[dwNumQuads]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertexScratchBuffer), &vertexScratchBuffer[0], GL_STATIC_DRAW);
            //glBufferSubData(GL_ARRAY_BUFFER, 0,sizeof(vertexScratchBuffer), &vertexScratchBuffer[0]);
            glBindBuffer(GL_ARRAY_BUFFER,0);
            ++dwNumQuads;
        	pthread_mutex_unlock(&streaming_sync);
        }
    }
    return 0;
}

bool initWindow()
{
    emscripten_set_canvas_element_size("#canvas", 1200, 960 );
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = 1;
    attr.depth = 1;
    attr.stencil = 1;
    attr.antialias = attr.preserveDrawingBuffer = attr.failIfMajorPerformanceCaveat = 0;
    attr.enableExtensionsByDefault = 1;
    attr.premultipliedAlpha = 0;
    attr.majorVersion = 2;
    attr.minorVersion = 0;
    //attr.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_ALWAYS;
    ctx = emscripten_webgl_create_context("#canvas", &attr);
    EMSCRIPTEN_RESULT res;
    if( EMSCRIPTEN_RESULT_SUCCESS != (res = emscripten_webgl_make_context_current(ctx) ) )
    {
        printf("Could not make webgl context current in main thread %s\n",GetStringErrorCode(res));
        return false;
    }
    
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {

        printf("GLEW init failed: %s!\n", glewGetErrorString(err));
        return false;
    }
    glViewport( 0, 0, 1200, 960 );
    return true;
}


void MainLoop()
{
    glClearColor ( 0.2f, 0.2f, 0.2980392157f, 1.f );
    glClear ( GL_COLOR_BUFFER_BIT );
    pthread_mutex_lock(&streaming_sync);
    for( u32 dwQuad = 0; dwQuad <dwNumQuads; ++dwQuad)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[dwQuad]);
        glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glEnableVertexAttribArray(posAttrib);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    pthread_mutex_unlock(&streaming_sync);
}


int main()
{
    if( !initWindow() )
    {
        return EXIT_FAILURE;
    }

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    /*
    for( u32 dwQuadBuf =0; dwQuadBuf < 16; ++dwQuadBuf)
    {
    	glGenBuffers(1,&vertexBuffers[dwQuadBuf]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[dwQuadBuf]);
        glBufferData(GL_ARRAY_BUFFER, 6*sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);
    }
    glBindBuffer(GL_ARRAY_BUFFER,0);
	*/

    // Specify the layout of the vertex data
    posAttrib = glGetAttribLocation(shaderProgram, "position");

    pthread_mutexattr_t attr;
    pthread_mutexattr_init ( &attr );
    pthread_mutexattr_settype ( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init ( &streaming_sync, &attr );


    pthread_attr_t atAttributes;
    pthread_attr_init ( &atAttributes );
    s32 dwResult = pthread_create ( &streamingThreadHandle, &atAttributes, StreamingThread, 0 );
    if( dwResult != 0 )
    {
        printf("Error creating Streaming Thread!\n");
        return EXIT_FAILURE;
    }
    pthread_attr_destroy( &atAttributes );


    emscripten_set_main_loop( MainLoop, 0, true );
    return EXIT_SUCCESS;
}