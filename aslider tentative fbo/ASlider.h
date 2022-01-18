#pragma once

#include <string>
#include <iostream>
#include <FFGLSDK.h>
#include "FFGL.h"
#include "GLResources.h"
#include <fstream>
#include "FFGLScopedFBOBinding.h"
#include "GLUT/glut.h"




static std::vector<GLuint> texIds;


class ASlider : public ffglqs::Plugin
{
public:
    ASlider();
    ~ASlider();

    //CFFGLPlugin
    FFResult InitGL( const FFGLViewportStruct* vp ) override;
    FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
//    unsigned int Resize( const FFGLViewportStruct* vp )override;
    FFResult DeInitGL() override;

private:
    ffglex::FFGLShader shader;   //!< Utility to help us compile and link some shaders into a program.
    ffglex::FFGLScreenQuad quad; //!< Utility to help us render a full screen quad.

    class FBOPair
    {
    public:
        GLuint firstBufferId;
        GLuint firstTextureId;

        GLuint secondBufferId;
        GLuint secondTextureId;

        bool reversedDirection;

    } *fbos{ nullptr };

    int currentTexId{ 0 };
    bool IsFirstFrame{ true };
    GLuint frameBufferId;
    GLuint renderedTexture;
    void EnsureTexturesCreated(ProcessOpenGLStruct* pGL, int width, int height);
    FBOPair* CreateFBO(const GLuint width, const GLuint height);
    
    std::shared_ptr< ffglqs::ParamRange > pUp;
    std::shared_ptr< ffglqs::ParamRange > pDown;
    
    GLuint noiseWidth;
    GLuint noiseHeight;

};
