#pragma once
#include <string>
#include <iostream>
#include <FFGLSDK.h>
#include <fstream>
#include "FFGLScopedFBOBinding.h"

class ASlider : public ffglqs::Plugin
{
public:
    ASlider();
    ~ASlider();

    //CFFGLPlugin
    FFResult InitGL( const FFGLViewportStruct* vp ) override;
    FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
    unsigned int Resize( const FFGLViewportStruct* vp )override;
    FFResult DeInitGL() override;

    
    
private:
    
    ffglex::FFGLShader shader;   //!< Utility to help us compile and link some shaders into a program.
    ffglex::FFGLScreenQuad quad; //!< Utility to help us render a full screen quad.
    
    ffglex::FFGLShader dummy_shader;   //!< Utility to help us compile and link some shaders into a program.

    ffglex::FFGLFBO fbos[2];
    
    int index = 0;
    
    bool resolutionChanged(int width, int height);
    uint last_width, last_height;
    
    
    
};
