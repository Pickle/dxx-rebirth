#ifndef _OGL_H
#define _OGL_H

#include "cml/cml.h"
#include "cshader.h"
//#include "cbuffer.h"

#define SIZEOF_FLOAT                sizeof(GLfloat)
#define SIZEOF_POSITION             (SIZEOF_FLOAT*3)
#define SIZEOF_TEXCOORD             (SIZEOF_FLOAT*2)
#define SIZEOF_COLOR                (SIZEOF_FLOAT*4)
#define SIZEOF_VERTEX               (SIZEOF_POSITION+SIZEOF_TEXCOORD+SIZEOF_COLOR)

#define OFFSET_VERTEX_POSITION      0                                                        // 3
#define OFFSET_VERTEX_TEXCOORD      BUFFER_OFFSET(OFFSET_VERTEX_POSITION+SIZEOF_POSITION)    // 2
#define OFFSET_VERTEX_COLOR         BUFFER_OFFSET(OFFSET_VERTEX_TEXCOORD+SIZEOF_TEXCOORD)    // 4

enum {
    POSITION_XY = 0,
    POSITION_XYZ,
    POSITION_TOTAL
};

enum {
    PROGRAM_COLOR=0,
    PROGRAM_COLOR_UNIFORM,
    PROGRAM_TEXTURE,
    PROGRAM_TEXTURE_UNIFORM,
    PROGRAM_MULTITEXTURE,
    PROGRAM_TOTAL,
    PROGRAM_NONE
};

enum {
    BUFFER_COLOR=0,
    BUFFER_COLOR_UNIFORM,
    BUFFER_TEXTURE,
    BUFFER_MULTITEXTURE,
    BUFFER_TOTAL,
    BUFFER_NONE
};

typedef cml::vector2i       vec2i_t;
typedef cml::vector3i       vec3i_t;
typedef cml::vector4i       vec4i_t;
typedef cml::vector2f       vec2f_t;
typedef cml::vector3f       vec3f_t;
typedef cml::vector4f       vec4f_t;
typedef cml::matrix44f_c    mat44f_t;
typedef cml::matrix33f_c    mat33f_t;

typedef struct VERTEX_CLR_T {
    vec3f_t position;
    vec4f_t color;
} vertex_clr_t;

typedef struct VERTEX_TEX_T {
    vec3f_t position;
    vec2f_t texcoord;
    GLubyte color[4];
} vertex_tex_t;

typedef struct VERTEX_TEX_UNI_T {
    vec3f_t position;
    vec2f_t texcoord;
} vertex_tex_uni_t;

typedef struct VERTEX_MTEX_T {
    vec3f_t position;
    vec2f_t texcoord[2];
    GLubyte color[4];
} vertex_mtex_t;

typedef struct GLSTATE_T {
    GLenum   mode;
    uint8_t  view;
    int32_t  count;
    int32_t  stride;
    GLfloat  pointsize;
    vec4f_t  color4f;
    GLint    texwrap[2];
    GLuint   tex[2];    
    GLfloat* v_pos;
    GLfloat* v_tex[2];
    GLubyte* v_clr;
} glstate_t;

extern GLuint ProgramCurrent;
extern glstate_t GLState;
extern CShader Shaders[PROGRAM_TOTAL];
//extern CBuffer Buffer[BUFFER_TOTAL];
extern vector<mat44f_t>    ModelView;
extern mat44f_t            Projection;
extern mat44f_t            ProjPersp;
extern mat44f_t            ProjOrtho;

#if defined(OGL1)
#define ogl_draw_arrays ogl1_draw_arrays
#elif defined(OGL2)
#define ogl_draw_arrays ogl2_draw_arrays
#endif

#if defined(OGL1)
void ogl1_draw_arrays( const glstate_t& state );
#endif
#if defined(OGL2)
void ogl2_draw_arrays( const glstate_t& state );
bool ogl2_shader_compile( void );
void og12_use_shader( uint8_t program );
#endif

void ogl_scale( GLfloat x, GLfloat y, GLfloat z );
void ogl_translate( GLfloat x, GLfloat y, GLfloat z );

#endif /* _OGL_H */
