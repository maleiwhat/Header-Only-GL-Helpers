// https://github.com/Flix01/Header-Only-GL-Helpers
//
/** MIT License
 *
 * Copyright (c) 2017 Flix (https://github.com/Flix01/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

/* WHAT'S THIS?
 * A plain C (--std=gnu89) header-only file that can send your rendering to a FBO (frame buffer object) texture,
 * and in a second pass can send that texture to screen. The major benefit of doing so is that when the frame rate is low,
 * this file can reduce the FBO texture area and then stretch it in the second pass, improving the frame rate.
 * This effect can also be switched off completely, so that there's no FBO overhead (but this switch is not automatic).
 *
 * As an extra feature, this file can be optionally used to draw geometry to a shadow map depth texture (owned by this file),
 * so that dynamic resolution works for the shadow map too (basically only a portion of the depth texture can be
 * used as shadow map at runtime). This is the first pass of the shadow mapping technique. In the second pass,
 * user must bind to this shadow texture, and add an additional uniform to the "second shadow map pass shader" with the
 * "dynamic resolution factor" (the change should be rather easy).
*/

/* USAGE:
 * Please see the comments of the functions below and try to follow their order when you call them.
 * Define DYNAMIC_RESOLUTION_IMPLEMENTATION in one of your .c (or .cpp) files before the inclusion of this file.
*/

// WARNING FOR HI-RES SCREEN SIZE:
// The implementation assumes that the screen (or window) width and height are supported for texture creation (otherwise the FBO texture cannot be created).
// That's because if we want to keep screen quality when "dynamic resolution" is 1.0f, we need a full size texture.

// OPTIONAL DEFINITIONS:
//
//#define DYNAMIC_RESOLUTION_USE_GLSL_VERSION_330                   // (Optional) Not sure it's faster...
//
//#define DYNAMIC_RESOLUTION_USE_NEAREST_TEXTURE_FILTER             // (undefined is GL_LINEAR) Used only when dynamic resolution kicks in
//
//#define DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_MULTIPLIER             // needs a value (default is 1.5). When screen is resized, it multiplies its longest dimension to get the shadow texture size.
//#define DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_FORCE_POT              // when defined the shadow texture size is approximated by its nearest power of two
//#define DYNAMIC_RESOLUTION_SHADOW_MAP_MAX_SIZE                    // needs a value (default 2048). It clamps: shadow_texture.size = DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_MULTIPLIER * max(screen.width,screen.height); // (or its nearest POT)
//#define DYNAMIC_RESOLUTION_SHADOW_USE_NEAREST_TEXTURE_FILTER      // (undefined is GL_LINEAR)
//
//
//#define DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS                     // (default is 1) Basically it pingpongs render targets, greatly increasing memory consumption. Not sure if it's faster or not and if it's artifact-free.


#ifndef DYNAMIC_RESOLUTION_H
#define DYNAMIC_RESOLUTION_H

#ifdef __cplusplus
extern "C"	{
#endif

#ifndef DYNAMIC_RESOLUTION_VERSION
#   define DYNAMIC_RESOLUTION_VERSION 1.0
#endif //DYNAMIC_RESOLUTION_VERSION

/* The __restrict and __restrict__ keywords are recognized in both C, at all language levels, and C++, at LANGLVL(EXTENDED).*/
#ifdef DYNAMIC_RESOLUTION_NO_RESTRICT  // please define it globally if the keyword __restrict is not present
#   ifndef __restrict
#       define __restrict /*no-op*/
#   endif
#endif //DYNAMIC_RESOLUTION_NO_RESTRICT

#ifndef DYNAMIC_RESOLUTION_USE_DOUBLE_PRECISION
typedef float  droat;       // short form of dynamic_resolution_float
#else
typedef double droat;
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_HALF_PI
#define M_HALF_PI (M_PI/2.0)
#endif
#ifndef M_PIOVER180
#define M_PIOVER180 (3.14159265358979323846/180.0)
#endif
#ifndef M_180OVERPI
#define M_180OVERPI (180.0/3.14159265358979323846)
#endif

#ifdef TEAPOT_H_
#   error Please include dynamic_resolution_h BEFORE teapot_h so you can use additional functions inside teapot_h
#endif

#ifndef DYNAMIC_RESOLUTION_SHADOW_USE_PCF
#   define DYNAMIC_RESOLUTION_SHADOW_USE_PCF 0
#endif //DYNAMIC_RESOLUTION_SHADOW_USE_PCF

// In InitGL() or similar
void Dynamic_Resolution_Init(float desiredFPS, int enabled);
// In InitGL() and/or ResizeGL() or similar
void Dynamic_Resolution_Resize(int width,int height);

// In DrawGL() or similar:--------------------------------------

// Optional: draw shadow map
void Dynamic_Resolution_Bind_Shadow();
// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Here (not above)
void Dynamic_Resolution_Shadow_Set_VpMatrix(const droat vpMatrix[16]);
void Dynamic_Resolution_Shadow_Set_MMatrix(const droat mMatrix[16]);
void Dynamic_Resolution_Shadow_Set_Scaling(float scalingX,float scalingY,float scalingZ);
// draw objects here (disable all glDisableVertexAttrib(...) except zero)
void Dynamic_Resolution_Unbind_Shadow();
GLint Dynamic_Resolution_Get_Shadow_Texture_ID();
// End Optional Draw ShadowMap

void Dynamic_Resolution_Bind();
// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Here (not above)
// ... Draw here as usual...
void Dynamic_Resolution_Unbind();

// Finally Draw the Bound Frame Buffer To Screen
void Dynamic_Resolution_Render(float elapsed_seconds_from_last_frame);
//---------------------------------------------------------------

// In DestroyGL() or similar
void Dynamic_Resolution_Destroy(void);					// In your DestroyGL() or similar

// Setters/Getter
int Dynamic_Resolution_GetEnabled(void);
void Dynamic_Resolution_SetEnabled(int flag);
void Dynamic_Resolution_ToggleEnabled(void);

float Dynamic_Resolution_GetMinimumFPS(void);
void Dynamic_Resolution_SetMinimumFPS(float minimumFPS);

float Dynamic_Resolution_GetFPS(void);
float Dynamic_Resolution_GetDynResFactor(void);
float Dynamic_Resolution_GetShadowMapDynResFactor(void);
float Dynamic_Resolution_GetShadowMapTextureSize(void); // width == height
float Dynamic_Resolution_GetShadowMapTexelIncrement(void);
const char* Dynamic_Resolution_GetInfoString(void);

static __inline void Dynamic_Resolution_Helper_IdentityMatrix(droat* __restrict result16) {
    droat* m = result16;
    m[0]=m[5]=m[10]=m[15]=1;
    m[1]=m[2]=m[3]=m[4]=m[6]=m[7]=m[8]=m[9]=m[11]=m[12]=m[13]=m[14]=0;
}
static __inline void Dynamic_Resolution_Helper_CopyMatrix(droat* __restrict dst16,const droat* __restrict src16) {
    int i;for (i=0;i<16;i++) dst16[i]=src16[i];
}
static __inline void Dynamic_Resolution_Helper_MultMatrix(droat* __restrict result16,const droat* __restrict ml16,const droat* __restrict mr16) {
    int i,j,j4;
    if (result16==ml16) {
        droat ML16[16];Dynamic_Resolution_Helper_CopyMatrix(ML16,ml16);
        Dynamic_Resolution_Helper_MultMatrix(result16,ML16,mr16);
        return;
    }
    else if (result16==mr16) {
        droat MR16[16];Dynamic_Resolution_Helper_CopyMatrix(MR16,mr16);
        Dynamic_Resolution_Helper_MultMatrix(result16,ml16,MR16);
        return;
    }
    for(i = 0; i < 4; i++) {
        for(j = 0; j < 4; j++) {
            j4 = 4*j;
            result16[i+j4] =
                ml16[i]    * mr16[0+j4] +
                ml16[i+4]  * mr16[1+j4] +
                ml16[i+8]  * mr16[2+j4] +
                ml16[i+12] * mr16[3+j4];
        }
    }
}
void Dynamic_Resolution_Helper_InvertFast(droat* __restrict mOut16,const droat* __restrict m16);

static __inline void Dynamic_Resolution_Helper_ConvertMatrixd2f16(float* __restrict result16,const double* __restrict m16) {int i;for(i = 0; i < 16; i++) result16[i]=(float)m16[i];}
static __inline void Dynamic_Resolution_Helper_ConvertMatrixf2d16(double* __restrict result16,const float* __restrict m16) {int i;for(i = 0; i < 16; i++) result16[i]=(double)m16[i];}


#ifdef __cplusplus
}
#endif

#endif //DYNAMIC_RESOLUTION_H


#ifdef DYNAMIC_RESOLUTION_IMPLEMENTATION

#ifndef DYNAMIC_RESOLUTION_IMPLEMENTATION_H
#define DYNAMIC_RESOLUTION_IMPLEMENTATION_H // Additional guard

#include <stdio.h> // sprintf

#ifndef DYNAMIC_RESOLUTION_SHADOW_USE_PCF
#   define DYNAMIC_RESOLUTION_SHADOW_USE_PCF 0
#endif //DYNAMIC_RESOLUTION_SHADOW_USE_PCF

#ifdef __cplusplus
extern "C"	{
#endif

//----------------------------------------------------------------------------------
//
// IMPLEMENTATION START
//
//----------------------------------------------------------------------------------

__inline static void Dynamic_Resolution_Helper_GlUniformMatrix4v(GLint location,GLsizei count,GLboolean transpose,const droat* value) {
    const float* fvalue = NULL;
#   ifndef TEAPOT_MATRIX_USE_DOUBLE_PRECISION
    fvalue = value;
#   else
    float val[16];Dynamic_Resolution_Helper_ConvertMatrixd2f16(val,value);fvalue=val;
#   endif
    glUniformMatrix4fv(location,count,transpose,fvalue);
}

static const char* ScreenQuadVS[] = {
    "#ifdef GL_ES\n"
    "precision highp float;\n"
    "#endif\n"
    "attribute vec3 a_position;\n"
    "\n"
    "void main()	{\n"
    "    gl_Position = vec4( a_position, 1 );\n"
    "}\n"
};
static const char* ScreenQuadFS[] = {
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "uniform lowp sampler2D s_diffuse;\n"
    "#else\n"
    "uniform sampler2D s_diffuse;\n"
    "#endif\n"
    "uniform vec3 screenResAndFactor; // .x and .y in pixels; .z in [0,1]: default: 1\n"
    "\n"
    "void main() {\n"
    "	 vec2 texCoords = (gl_FragCoord.xy/screenResAndFactor.xy)*screenResAndFactor.z;\n"
    "    gl_FragColor = texture2D( s_diffuse, texCoords);\n"
    "}\n"
};


#ifndef DYNAMIC_RESOLUTION_USE_GLSL_VERSION_330
static const char* ShadowVS[] = {
    "#ifdef GL_ES\n"
    "precision highp float;\n"
    "#endif\n"
    "attribute vec3 a_position;\n"
    "uniform vec3 u_scaling;\n"
    "uniform mat4 u_shadowMvpMatrix;\n" // no bias is required here
    "\n"
    "void main()	{\n"
    "    gl_Position = u_shadowMvpMatrix*vec4(a_position*u_scaling,1);\n"
    "}\n"
};
static const char* ShadowFS[] = {"void main() {gl_FragColor = vec4(0,0,0,1);}\n"};
#else //DYNAMIC_RESOLUTION_USE_GLSL_VERSION_330
static const char* ShadowVS[] = {
    "#version 330\n"
    "#ifdef GL_ES\n"
    "precision highp float;\n"
    "#endif\n"
    "layout(location = 0) in vec3 a_position;\n"
    "uniform vec3 u_scaling;\n"
    "uniform mat4 u_shadowMvpMatrix;\n" // no bias is required here
    "\n"
    "void main()	{\n"
    "    gl_Position = u_shadowMvpMatrix*vec4(a_position*u_scaling,1);\n"
    "}\n"
};
static const char* ShadowFS[] = {
    "#version 330\n"
    "void main() {}\n"
};
#endif //DYNAMIC_RESOLUTION_USE_GLSL_VERSION_330


static __inline GLuint DRR_LoadShaderProgramFromSource(const char* vs,const char* fs);

#ifndef DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS
#	define DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS (1)    // (1): LOW MEMORY; (2) OR (3): ONE FRAME BIND FRAME BUFFER, NEXT USE BIND ITS TEXTURE (PROBABLY FASTER BUT LAGS 2 OR 3 FRAMES AND MEMORY USAGE IS 2 OR 3 TIMES BIGGER)
#endif //DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS

typedef struct {
    GLuint frame_buffer[DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS];
    GLuint depth_buffer[DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS];
    GLuint texture[DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS];
    float resolution_factor[DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS];

    GLuint screenQuadProgramId;
    GLint aLoc_APosition;
    GLint uLoc_screenResAndFactor;
    GLint uLoc_SDiffuse;
    int width,height;
    GLint default_frame_buffer;

    float FPS,desiredFPS,factor;
	int enabled;
	int render_target_index;
    char tmp[84];

    // shadow map FBO
    GLuint shadow_frame_buffer[DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS];
    GLuint shadow_texture[DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS];
    float shadow_resolution_factor[DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS];

    GLuint shadowProgramId;
    GLint shadow_aLoc_APosition;
    GLint shadow_uLoc_Scaling;
    GLint shadow_uLoc_shadowMvpMatrix;

    droat shadowVpMatrix[16];

    int shadow_texture_size;
} RenderTarget;

#ifdef DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_FORCE_POT
static __inline int Dynamic_Resolution_Helper_IsPowerOfTwo(unsigned int n) {return ((n & (n - 1)) == 0) ? 1 : 0;}
static __inline unsigned Dynamic_Resolution_Helper_PreviousPowerOfTwo( unsigned x ) {
    // http://stackoverflow.com/questions/2679815/previous-power-of-2
    if (x == 0) return 0;
    else {
        // x--; Uncomment this, if you want a strictly less than 'x' result.
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return x - (x >> 1);
    }
}
static __inline unsigned Dynamic_Resolution_Helper_NextPowerOfTwo( unsigned x ) {
    // http://stackoverflow.com/questions/1322510/given-an-integer-how-do-i-find-the-next-largest-power-of-two-using-bit-twiddlin
    if (x == 0) return 0;
    else {
        x--;
        x |= x >> 1;   // Divide by 2^k for consecutive doublings of k up to 32,
        x |= x >> 2;   // and then or the results.
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x++;           // The result is a number of 1 bits equal to the number
        // of bits in the original number, plus 1. That's the
        // next highest power of 2.
        return x;
    }
}
static __inline unsigned Dynamic_Resolution_Helper_NearestPowerOfTwo( unsigned x ,unsigned max_value_allowed,unsigned min_value_allowed) {
    if (Dynamic_Resolution_Helper_IsPowerOfTwo(x)) return x;
    else {
        const unsigned prev = Dynamic_Resolution_Helper_PreviousPowerOfTwo(x);
        const unsigned next = Dynamic_Resolution_Helper_NextPowerOfTwo(x);
        unsigned best = (x-prev<next-x) ? prev : next;
        if (best>max_value_allowed) best = max_value_allowed;
        else if (best<=min_value_allowed) best = min_value_allowed;
        return best;
    }
}
#endif //DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_FORCE_POT

static void RenderTarget_Create(RenderTarget* rt,float desiredFPS,int enabled) {
    rt->FPS = rt->desiredFPS = desiredFPS;
	rt->enabled = enabled;
	rt->render_target_index = 0;
    rt->factor = 1.f;
    rt->tmp[0]='\0';

    rt->screenQuadProgramId = DRR_LoadShaderProgramFromSource(*ScreenQuadVS,*ScreenQuadFS);
    if (!rt->screenQuadProgramId) {
        fprintf(stderr,"Error: screenQuadProgramId==0\n");
        exit(0);
    }
    else {
        rt->aLoc_APosition = glGetAttribLocation(rt->screenQuadProgramId, "a_position");
        rt->uLoc_SDiffuse = glGetUniformLocation(rt->screenQuadProgramId,"s_diffuse");
        rt->uLoc_screenResAndFactor = glGetUniformLocation(rt->screenQuadProgramId,"screenResAndFactor");
        if (rt->aLoc_APosition<0) fprintf(stderr,"Error: rt->aLoc_APosition<0\n");
        if (rt->uLoc_SDiffuse<0) fprintf(stderr,"Error: uLoc_SDiffuse<0\n");
        if (rt->uLoc_screenResAndFactor<0) fprintf(stderr,"Error: uLoc_screenResAndFactor<0\n");
    }

    rt->shadowProgramId = DRR_LoadShaderProgramFromSource(*ShadowVS,*ShadowFS);
    if (!rt->shadowProgramId) {
        fprintf(stderr,"Error: shadowProgramId==0\n");
        exit(0);
    }
    else {
        rt->shadow_aLoc_APosition = glGetAttribLocation(rt->shadowProgramId, "a_position");
        rt->shadow_uLoc_Scaling = glGetUniformLocation(rt->shadowProgramId,"u_scaling");
        rt->shadow_uLoc_shadowMvpMatrix = glGetUniformLocation(rt->shadowProgramId,"u_shadowMvpMatrix");
        if (rt->shadow_aLoc_APosition<0) fprintf(stderr,"Error: shadow_aLoc_APosition<0\n");
        if (rt->shadow_uLoc_Scaling<0) fprintf(stderr,"Error: shadow_uLoc_Scaling<0\n");
        if (rt->shadow_uLoc_shadowMvpMatrix<0) fprintf(stderr,"Error: shadow_uLoc_shadowMvpMatrix<0\n");

        glUseProgram(rt->shadowProgramId);
        glUniform3f(rt->shadow_uLoc_Scaling,1,1,1);
        glUseProgram(0);
    }

    glGenFramebuffers(DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS, rt->frame_buffer);
    glGenTextures(DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS, rt->texture);
    glGenRenderbuffers(DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS, rt->depth_buffer);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &(rt->default_frame_buffer));

    glGenFramebuffers(DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS, rt->shadow_frame_buffer);
    glGenTextures(DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS, rt->shadow_texture);
}
static void RenderTarget_Destroy(RenderTarget* rt) {
    if (rt->frame_buffer[0]) 	glDeleteBuffers(DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS, rt->frame_buffer);
    if (rt->texture[0]) 		glDeleteTextures(DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS, rt->texture);
    if (rt->depth_buffer[0])	glDeleteBuffers(DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS, rt->depth_buffer);
    if (rt->screenQuadProgramId) {glDeleteProgram(rt->screenQuadProgramId);rt->screenQuadProgramId=0;}

    if (rt->shadow_frame_buffer[0])    glDeleteBuffers(DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS, rt->shadow_frame_buffer);
    if (rt->shadow_texture[0])         glDeleteTextures(DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS,rt->shadow_texture);
    if (rt->shadowProgramId) {glDeleteProgram(rt->shadowProgramId);rt->shadowProgramId=0;}
}
static void RenderTarget_Init(RenderTarget* rt,int width, int height) {
    int i;
    //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (rt->screenQuadProgramId) {
        rt->width = width;
        rt->height = height;

        GLenum filter = GL_LINEAR;
#       ifdef  DYNAMIC_RESOLUTION_USE_NEAREST_TEXTURE_FILTER
        filter = GL_NEAREST;
#       endif //DYNAMIC_RESOLUTION_USE_NEAREST_TEXTURE_FILTER

        for (i=0;i<DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS;i++)	{
            rt->resolution_factor[i] = 1;

            glBindTexture(GL_TEXTURE_2D, rt->texture[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#           ifdef __EMSCRIPTEN__	// WebGL 1.0 (in Firefox) seems to accept only this setting (we could use it for non-emscripten builds too)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rt->width, rt->height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
#           else
            // I've read that 4 channels (e.g. GL_RGBA) are faster then 3 (e.g. GL_RGB)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rt->width, rt->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);	// GL_BGRA, GL_UNSIGNED_BYTE
            //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rt->width, rt->height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);	// This seems what my NVIDIA card seems to support natively
#           endif

#           ifndef DYNAMIC_RESOLUTION_NO_DEPTH_ATTACHMENT
            glBindRenderbuffer(GL_RENDERBUFFER, rt->depth_buffer[i]);
#           ifdef __EMSCRIPTEN__	// WebGL 1.0 (in Firefox) seems to accept only this setting (we could use it for non-emscripten builds too)
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, rt->width, rt->height);	// GL_DEPTH_COMPONENT16 work
#           else //__EMSCRIPTEN__
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, rt->width, rt->height);
#           endif //__EMSCRIPTEN__
#           endif

            glBindFramebuffer(GL_FRAMEBUFFER, rt->frame_buffer[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->texture[i], 0);
#           ifndef DYNAMIC_RESOLUTION_NO_DEPTH_ATTACHMENT
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depth_buffer[i]);
#           endif
            {
                //Does the GPU support current FBO configuration?
                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (status!=GL_FRAMEBUFFER_COMPLETE) printf("glCheckFramebufferStatus(...) FAILED.\n");
            }
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER,rt->default_frame_buffer);
    }

    // Shadow stuff here
    if (rt->shadowProgramId) {
        // These could be multiplied by some kind of constants (and it could make sense to make width=height)
#       ifndef DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_MULTIPLIER
#           define DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_MULTIPLIER (1.5)
#       endif //DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_MULTIPLIER
        rt->shadow_texture_size = (width>height ? width : height)*DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_MULTIPLIER;
#       ifndef DYNAMIC_RESOLUTION_SHADOW_MAP_MAX_SIZE
#           define DYNAMIC_RESOLUTION_SHADOW_MAP_MAX_SIZE (2048)
#       endif //DYNAMIC_RESOLUTION_SHADOW_MAP_MAX_SIZE
#       ifdef DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_FORCE_POT
        rt->shadow_texture_size = (int) Dynamic_Resolution_Helper_NearestPowerOfTwo((unsigned) rt->shadow_texture_size,(DYNAMIC_RESOLUTION_SHADOW_MAP_MAX_SIZE),4);
#       endif //DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_FORCE_POT
        if (rt->shadow_texture_size>DYNAMIC_RESOLUTION_SHADOW_MAP_MAX_SIZE) rt->shadow_texture_size = DYNAMIC_RESOLUTION_SHADOW_MAP_MAX_SIZE;

        // Debug:
        //fprintf(stderr,"screen={%d,%d}\tscreenMaxDimMultiplied=%d\trt->shadow_texture_size=%d\n",width,height,(int)((width>height ? width : height)*DYNAMIC_RESOLUTION_SHADOW_MAP_SIZE_MULTIPLIER),rt->shadow_texture_size);

        GLenum shadow_filter = GL_LINEAR;
#       ifdef  DYNAMIC_RESOLUTION_SHADOW_USE_NEAREST_TEXTURE_FILTER
        shadow_filter = GL_NEAREST;
#       endif //DYNAMIC_RESOLUTION_SHADOW_USE_NEAREST_TEXTURE_FILTER

        for (i=0;i<DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS;i++)	{
            rt->shadow_resolution_factor[i] = 1;

            glBindTexture(GL_TEXTURE_2D, rt->shadow_texture[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, shadow_filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, shadow_filter);

#           if DYNAMIC_RESOLUTION_SHADOW_USE_PCF>0
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            //glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
#           endif //DYNAMIC_RESOLUTION_SHADOW_USE_PCF

#           ifdef __EMSCRIPTEN__
            glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, rt->shadow_texture_size, rt->shadow_texture_size, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, 0);
            const GLenum clampMode = GL_CLAMP_TO_EDGE;  // Unluckily WebGL does not support GL_CLAMP or GL_CLAMP_TO_BORDER
#           else //__EMSCRIPTEN__*/
            glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, rt->shadow_texture_size, rt->shadow_texture_size, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
            const GLenum clampMode = //GL_CLAMP;    // sampling outside of the shadow map gives always shadowed pixels
                   // GL_CLAMP_TO_EDGE;             // sampling outside of the shadow map can give shadowed or unshadowed pixels (it depends on the edge of the shadow map)
                    GL_CLAMP_TO_BORDER;             // sampling outside of the shadow map gives always non-shadowed pixels (if we set the border color correctly)
            if (clampMode==GL_CLAMP_TO_BORDER)  {
               const GLfloat border[] = {1.0f,1.0f,1.0f,0.0f };
               glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
            }
#           endif // //__EMSCRIPTEN__*/
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampMode );
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampMode );

            glBindFramebuffer(GL_FRAMEBUFFER, rt->shadow_frame_buffer[i]);
#           ifndef __EMSCRIPTEN__
            glDrawBuffer(GL_NONE); // Instruct openGL that we won't bind a color texture with the currently bound FBO
            glReadBuffer(GL_NONE);
#           endif //__EMSCRIPTEN__
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,rt->shadow_texture[i], 0);
            {
                //Does the GPU support current FBO configuration?
                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (status!=GL_FRAMEBUFFER_COMPLETE) printf("glCheckFramebufferStatus(...) FAILED for shadow_frame_buffer.\n");
            }
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER,rt->default_frame_buffer);
    }
}
static void RenderTarget_Resize(RenderTarget* rt,int width, int height)    {
   RenderTarget_Init(rt,width,height);
}
static RenderTarget render_target;

int Dynamic_Resolution_GetEnabled(void) {return render_target.enabled;}
void Dynamic_Resolution_SetEnabled(int flag) {
    int i;
    render_target.enabled=flag;render_target.factor=1.f;
    for (i=0;i<DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS;i++) {render_target.resolution_factor[i]=render_target.shadow_resolution_factor[i]=1.f;}
}
void Dynamic_Resolution_ToggleEnabled(void) {Dynamic_Resolution_SetEnabled(!Dynamic_Resolution_GetEnabled());}

float Dynamic_Resolution_GetMinimumFPS(void)  {return render_target.desiredFPS;}
void Dynamic_Resolution_SetMinimumFPS(float minimumFPS) {render_target.desiredFPS=minimumFPS;}

float Dynamic_Resolution_GetFPS(void)   {return render_target.FPS;}
float Dynamic_Resolution_GetDynResFactor(void)  {return render_target.resolution_factor[render_target.render_target_index];}
float Dynamic_Resolution_GetShadowMapDynResFactor(void)  {
    //return render_target.shadow_resolution_factor[render_target.render_target_index];
    int render_target_index2 = render_target.render_target_index + 1;
    if (render_target_index2>=DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS) render_target_index2-=DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS;
    return render_target.shadow_resolution_factor[render_target_index2];
}
float Dynamic_Resolution_GetShadowMapTextureSize(void)  {return (float) render_target.shadow_texture_size;}
float Dynamic_Resolution_GetShadowMapTexelIncrement(void)   {return Dynamic_Resolution_GetShadowMapDynResFactor()/(float) render_target.shadow_texture_size;}
GLint Dynamic_Resolution_Get_Shadow_Texture_ID() {
    //return render_target.shadow_texture[render_target.render_target_index];
    int render_target_index2 = render_target.render_target_index + 1;
    if (render_target_index2>=DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS) render_target_index2-=DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS;
    return render_target.shadow_texture[render_target_index2];
}


static GLuint screenQuadVbo = 0;
static void ScreenQuadVBO_Init() {	
    const float verts[6] = {-1,-1,3,-1,-1,3};
    glGenBuffers(1,&screenQuadVbo);
    glBindBuffer(GL_ARRAY_BUFFER, screenQuadVbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);
    glBufferData(GL_ARRAY_BUFFER, 6*sizeof(float), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
static void ScreenQuadVBO_Destroy() {if (screenQuadVbo) {glDeleteBuffers(1,&screenQuadVbo);screenQuadVbo=0;}}
static void ScreenQuadVBO_Bind() {
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, screenQuadVbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);
}
void ScreenQuadVBO_Draw() {glDrawArrays(GL_TRIANGLES,0,3);}
static void ScreenQuadVBO_Unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);
}



// Loading shader function
static __inline GLhandleARB DRR_LoadShader(const char* buffer, const unsigned int type)
{
    GLhandleARB handle;
    const GLcharARB* files[1];

    // shader Compilation variable
    GLint result;				// Compilation code result
    GLint errorLoglength ;
    char* errorLogText;
    GLsizei actualErrorLogLength;

    handle = glCreateShader(type);
    if (!handle)
    {
        //We have failed creating the vertex shader object.
        printf("Failed creating vertex shader object.\n");
        exit(0);
    }

    files[0] = (const GLcharARB*)buffer;
    glShaderSource(
                handle, //The handle to our shader
                1, //The number of files.
                files, //An array of const char * data, which represents the source code of theshaders
                NULL);

    glCompileShader(handle);

    //Compilation checking.
    glGetShaderiv(handle, GL_COMPILE_STATUS, &result);

    // If an error was detected.
    if (!result)
    {
        //We failed to compile.
        printf("Shader failed compilation.\n");

        //Attempt to get the length of our error log.
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &errorLoglength);

        //Create a buffer to read compilation error message
        errorLogText =(char*) malloc(sizeof(char) * errorLoglength);

        //Used to get the final length of the log.
        glGetShaderInfoLog(handle, errorLoglength, &actualErrorLogLength, errorLogText);

        // Display errors.
        printf("%s\n",errorLogText);

        // Free the buffer malloced earlier
        free(errorLogText);
    }

    return handle;
}

static __inline GLuint DRR_LoadShaderProgramFromSource(const char* vs,const char* fs)	{
    // shader Compilation variable
    GLint result;				// Compilation code result
    GLint errorLoglength ;
    char* errorLogText;
    GLsizei actualErrorLogLength;

    GLhandleARB vertexShaderHandle;
    GLhandleARB fragmentShaderHandle;
    GLuint programId = 0;

    vertexShaderHandle   = DRR_LoadShader(vs,GL_VERTEX_SHADER);
    fragmentShaderHandle = DRR_LoadShader(fs,GL_FRAGMENT_SHADER);
    if (!vertexShaderHandle || !fragmentShaderHandle) return 0;

    programId = glCreateProgram();

    glAttachShader(programId,vertexShaderHandle);
    glAttachShader(programId,fragmentShaderHandle);
    glLinkProgram(programId);

    //Link checking.
    glGetProgramiv(programId, GL_LINK_STATUS, &result);

    // If an error was detected.
    if (!result)
    {
        //We failed to compile.
        printf("Program failed to link.\n");

        //Attempt to get the length of our error log.
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &errorLoglength);

        //Create a buffer to read compilation error message
        errorLogText =(char*) malloc(sizeof(char) * errorLoglength);

        //Used to get the final length of the log.
        glGetProgramInfoLog(programId, errorLoglength, &actualErrorLogLength, errorLogText);

        // Display errors.
        printf("%s\n",errorLogText);

        // Free the buffer malloced earlier
        free(errorLogText);
    }

    glDeleteShader(vertexShaderHandle);
    glDeleteShader(fragmentShaderHandle);

    return programId;
}

void Dynamic_Resolution_Init(float desiredFPS, int enabled) {
    RenderTarget_Create(&render_target,desiredFPS,enabled);
	ScreenQuadVBO_Init();
}

void Dynamic_Resolution_Destroy(void) {
	ScreenQuadVBO_Destroy();
	RenderTarget_Destroy(&render_target);
}

void Dynamic_Resolution_Resize(int width,int height) {
    RenderTarget_Resize(&render_target,width,height);
}

void Dynamic_Resolution_Bind() {
    if (render_target.enabled)	{
        render_target.resolution_factor[render_target.render_target_index] = render_target.factor;
        glBindFramebuffer(GL_FRAMEBUFFER, render_target.frame_buffer[render_target.render_target_index]); //NUM_RENDER_TARGETS
        glViewport(0, 0, (int)(render_target.width * render_target.factor),(int) (render_target.height * render_target.factor));
    }
    else glViewport(0, 0, render_target.width, render_target.height);
}

void Dynamic_Resolution_Unbind() {
    if (render_target.enabled)	{
        glBindFramebuffer(GL_FRAMEBUFFER,render_target.default_frame_buffer);
        glViewport(0, 0, render_target.width, render_target.height);
    }
}

void Dynamic_Resolution_Bind_Shadow() {
    glBindFramebuffer(GL_FRAMEBUFFER, render_target.shadow_frame_buffer[render_target.render_target_index]);
    if (render_target.enabled)  {
        render_target.shadow_resolution_factor[render_target.render_target_index] = render_target.factor;
        glViewport(0, 0, (int)(render_target.shadow_texture_size * render_target.factor),(int) (render_target.shadow_texture_size * render_target.factor));
    }
    else
        glViewport(0, 0, (int)(render_target.shadow_texture_size),(int) (render_target.shadow_texture_size));
    glEnableVertexAttribArray(render_target.shadow_aLoc_APosition);
    glUseProgram(render_target.shadowProgramId);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glCullFace(GL_FRONT);
    //glFrontFace(GL_CW);   // Same as above
    //glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-2.0f, -2.0f);
    glEnable(GL_DEPTH_CLAMP);
    //printf("%1.0f,%1.0f\n",render_target.shadow_texture_size * render_target.shadow_resolution_factor,render_target.shadow_texture_size * render_target.shadow_resolution_factor);
    //printf("render_target.shadow_aLoc_APosition: %d\n",render_target.shadow_aLoc_APosition);
}

void Dynamic_Resolution_Shadow_Set_Scaling(float scalingX,float scalingY,float scalingZ) {
    glUniform3f(render_target.shadow_uLoc_Scaling,scalingX,scalingY,scalingZ);
}

void Dynamic_Resolution_Shadow_Set_VpMatrix(const droat vpMatrix[16]) {
    Dynamic_Resolution_Helper_CopyMatrix(render_target.shadowVpMatrix,vpMatrix);
}

void Dynamic_Resolution_Shadow_Set_MMatrix(const droat mMatrix[16]) {
    droat tmp[16];
    Dynamic_Resolution_Helper_MultMatrix(tmp,render_target.shadowVpMatrix,mMatrix);
    Dynamic_Resolution_Helper_GlUniformMatrix4v(render_target.shadow_uLoc_shadowMvpMatrix,1,GL_FALSE,tmp);
}

void Dynamic_Resolution_Unbind_Shadow() {
    glDisable(GL_DEPTH_CLAMP);
    //glDisable(GL_POLYGON_OFFSET_FILL);
    //glFrontFace(GL_CCW);   // Same as below
    glCullFace(GL_BACK);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glUseProgram(0);
    glDisableVertexAttribArray(render_target.shadow_aLoc_APosition);
    glViewport(0, 0, render_target.shadow_texture_size, render_target.shadow_texture_size);
    glBindFramebuffer(GL_FRAMEBUFFER,render_target.default_frame_buffer);
}


const char* Dynamic_Resolution_GetInfoString(void) {return render_target.tmp;}

void Dynamic_Resolution_Helper_InvertFast(droat* __restrict mOut16,const droat* __restrict m16)	{
    // It works only for translation + rotation, and only
    // when rotation can be represented by an unit quaternion
    // scaling is discarded
    const droat* m = m16;
    droat* n = mOut16;
    const droat T[3] = {-m[12],-m[13],-m[14]};
    droat w;
    // Step 1. Transpose the 3x3 submatrix
    n[3]=m[3];n[7]=m[7];n[11]=m[11];n[15]=m[15];
    n[0]=m[0];n[1]=m[4];n[2]=m[8];
    n[4]=m[1];n[5]=m[5];n[6]=m[9];
    n[8]=m[2];n[9]=m[6];n[10]=m[10];
    // Step2. Adjust translation
    n[12]=T[0]*n[0] + T[1]*n[4] +T[2]*n[8];
    n[13]=T[0]*n[1] + T[1]*n[5] +T[2]*n[9];
    n[14]=T[0]*n[2] + T[1]*n[6] +T[2]*n[10];
    w    =T[0]*n[3] + T[1]*n[7] +T[2]*n[11];
    if (w!=0 && w!=1) {n[12]/=w;n[13]/=w;n[14]/=w;} // These last 2 lines are not strictly necessary AFAIK
}


static void Dynamic_Resolution_Calculate_Factor(float elapsed_seconds_from_last_frame) {
    // Do FPS count and adjust resolution_factor
    static unsigned frames = 0;
    static float total_seconds = 0;
    ++frames;
    total_seconds+=elapsed_seconds_from_last_frame;;
    if (total_seconds>2.f) {
        const float FPS_TARGET = (float) render_target.desiredFPS;
        render_target.FPS = (float)(frames/total_seconds);
        total_seconds-= 2.f;
        frames = 0;
        if (render_target.enabled)
        {
            if (render_target.FPS<FPS_TARGET) {
                render_target.factor-= render_target.factor*(FPS_TARGET-render_target.FPS)*0.0175f;
                if (render_target.factor<0.15f) render_target.factor=0.15f;
            }
            else {
                render_target.factor+= render_target.factor*(render_target.FPS-FPS_TARGET)*0.0175f;
                if (render_target.factor>1.0f) render_target.factor=1.0f;
            }
        }
        else render_target.factor=1.f;
        sprintf(render_target.tmp,"FPS: %1.0f DYN-RES:%s DRF=%1.3f (%dx%d) (shadow_map (when used):%dx%d)",render_target.FPS,render_target.enabled ? "ON " : "OFF",render_target.factor,render_target.width,render_target.height,render_target.shadow_texture_size,render_target.shadow_texture_size);
    }
}

void Dynamic_Resolution_Render(float elapsed_seconds_from_last_frame)   {
    int render_target_index2 = render_target.render_target_index + 1;
    if (render_target_index2>=DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS) render_target_index2-=DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS;
    //printf("%d - %d\n",render_target_index,render_target_index2);
    if (render_target.enabled)	{
        glViewport(0, 0, render_target.width, render_target.height);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, render_target.texture[render_target_index2]);

        ScreenQuadVBO_Bind();
        glUseProgram(render_target.screenQuadProgramId);
        glUniform1i(render_target.uLoc_SDiffuse,0);
        glUniform3f(render_target.uLoc_screenResAndFactor,render_target.width,render_target.height,render_target.resolution_factor[render_target_index2]);

        //glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);glDepthMask(GL_FALSE);

        ScreenQuadVBO_Draw();

        //glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);glDepthMask(GL_TRUE);

        ScreenQuadVBO_Unbind();
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    if (++render_target.render_target_index>=DYNAMIC_RESOLUTION_NUM_RENDER_TARGETS) render_target.render_target_index=0;
    Dynamic_Resolution_Calculate_Factor(elapsed_seconds_from_last_frame);
}


//----------------------------------------------------------------------------------
//
// IMPLEMENTATION END
//
//----------------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif



#endif //DYNAMIC_RESOLUTION_IMPLEMENTATION_H
#endif //DYNAMIC_RESOLUTION_IMPLEMENTATION


