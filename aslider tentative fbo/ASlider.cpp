#include "ASlider.h"


using namespace ffglex;


#include "FFGLLib.h"


using namespace ffglex;

static CFFGLPluginInfo PluginInfo(
    PluginFactory< ASlider >,// Create method
    "RE01",                      // Plugin unique ID of maximum length 4.
    "ASlider",            // Plugin name
    2,                           // API major version number
    1,                           // API minor version number
    1,                           // Plugin major version number
    0,                           // Plugin minor version number
    FF_EFFECT,                   // Plugin type
    "Slider",  // Plugin description
    "Alex"      // About
);


static const char _vertexShaderCode[] = R"(#version 410 core
    uniform vec2 MaxUV;

    layout( location = 0 ) in vec4 vPosition;
    layout( location = 1 ) in vec2 vUV;

    out vec2 uv;
    out vec2 norm_uv;

    void main()
    {
        gl_Position = vPosition;
        uv = vUV * MaxUV;
        norm_uv = vUV;
    }
)";

static const char _fragmentShaderCode[] = R"(#version 410 core

uniform sampler2D inputTexture;
uniform sampler2D inputTexture2;

uniform float Up;
uniform float Down;

in vec2 uv;
in vec2 norm_uv;

out vec4 fragColor;

void main()
{
    vec4 su;
    vec4 sd;
    vec4 up;
    vec4 down;
    vec4 amount;

    vec4 input0 = vec4(texture(inputTexture, uv));

//    vec2 uv2new = norm_uv - vec2(1./488., 1./488.);

    vec4 input1 = vec4(texture(inputTexture2, norm_uv));


    // get contribution
    amount.x = (input0.x > input1.x) ? 1. : 0.0;
    amount.y = (input0.y > input1.y) ? 1. : 0.0;
    amount.z = (input0.z > input1.z) ? 1. : 0.0;
    amount.w = (input0.w > input1.w) ? 1. : 0.0;


    // calculate slide down
    float d = max(1.0, abs(Down));
    sd = vec4(1.0 / d);
    down = input1 + ((input0 - input1) * sd);

    // calculate slide up
    float u = max(1.0, abs(Up));
    su = vec4(1.0 / u);
    up = input1 + ((input0 - input1) * su);


    // mix between down and up
    vec4 out_color = mix(down, up, amount);

//    if(out_color.r < 0.0080) out_color.r = 0; //003921568627
//    if(out_color.g < 0.0080) out_color.g = 0; //003921568627
//    if(out_color.b < 0.0080) out_color.b = 0; //003921568627

    out_color -= vec4(0.004, 0.004, 0.004, 0.004);
    fragColor  = out_color;
}
)";




using namespace std;

ASlider::ASlider()
{
    // Input properties
    SetMinInputs( 1 );
    SetMaxInputs( 1 );

    pUp = ffglqs::ParamRange::Create( "Up",  1., {1.0, 40.0} );
    AddParam(pUp);
    
    pDown = ffglqs::ParamRange::Create( "Down",  1., {1.0, 40.0} );
    AddParam(pDown);

}
ASlider::~ASlider()
{
}

FFResult ASlider::InitGL( const FFGLViewportStruct* vp )
{
    this->noiseWidth = vp->width;
    this->noiseHeight = vp->height;
    
    if( !shader.Compile( _vertexShaderCode, _fragmentShaderCode ) )
    {
        DeInitGL();
        return FF_FAIL;
    }
    
    if( !quad.Initialise() )
    {
        DeInitGL();
        return FF_FAIL;
    }

    //Use base-class init as success result so that it retains the viewport.
    return CFFGLPlugin::InitGL( vp );
}

FFResult ASlider::ProcessOpenGL( ProcessOpenGLStruct* pGL )
{
    if( pGL->numInputTextures < 1 )
        return FF_FAIL;

    if( pGL->inputTextures[ 0 ] == NULL )
        return FF_FAIL;

    FFGLTextureStruct Texture{ *(pGL->inputTextures[0]) };

    GLint systemCurrentFbo = pGL->HostFBO;
    
   // glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &systemCurrentFbo);
    
    if (texIds.empty())
        EnsureTexturesCreated(pGL, Texture.Width, Texture.Height);

    if (this->fbos == nullptr)
        this->fbos = CreateFBO(Texture.Width, Texture.Height);
        
    //get the max s,t that correspond to the
    //width,height of the used portion of the allocated texture space
    FFGLTexCoords maxCoords{ GetMaxGLTexCoords(Texture) };

    GLuint srcTexture;
    GLuint dstRenderBuffer;
    GLuint dstTexture;

    if (fbos->reversedDirection)
    {
        srcTexture = fbos->secondTextureId;
        dstRenderBuffer = fbos->firstBufferId;
        dstTexture = fbos->firstTextureId;
        fbos->reversedDirection = false;
    }
    else
    {
        srcTexture = IsFirstFrame ? texIds[currentTexId] : fbos->firstTextureId;
        dstRenderBuffer = fbos->secondBufferId;
        dstTexture = fbos->secondTextureId;

        fbos->reversedDirection = true;
        IsFirstFrame = false;
    }

    
//    glEnable(GL_TEXTURE_2D);
//
//    /*
//    glBindFramebuffer(GL_FRAMEBUFFER, dstRenderBuffer);
//    glClear(GL_COLOR_BUFFER_BIT);
//    defaultShader.BindShader();
//    glBindTexture(GL_TEXTURE_2D, texIds[currentTexId]);
//    glBegin(GL_QUADS);
//    //lower left
//    glTexCoord2d(0, 0);
//    glVertex2f(-1, -1);
//    //upper left
//    glTexCoord2d(0, maxCoords.t);
//    glVertex2f(-1, 1);
//    //upper right
//    glTexCoord2d(maxCoords.s, maxCoords.t);
//    glVertex2f(1, 1);
//    //lower right
//    glTexCoord2d(maxCoords.s, 0);
//    glVertex2f(1, -1);
//    glEnd();
//    defaultShader.UnbindShader();
//    */
//
//
//    // set rendering destination to FBO
//    glBindFramebuffer(GL_FRAMEBUFFER, dstRenderBuffer);
//    glClear(GL_COLOR_BUFFER_BIT);
//
//    shader.Set("inputTexture", 0);
//    shader.Set("inputTexture2", 1);
//
//    SendParams(shader);
//
//    //  glUniform1f(usedShader->FindUniform("Up"), Up->GetValue());
//    //  glUniform1f(usedShader->FindUniform("Down"), Down->GetValue());
//
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, srcTexture);
//
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, texIds[currentTexId]);
//
//    glActiveTexture(GL_TEXTURE2);
//    glBindTexture(GL_TEXTURE_2D, Texture.Handle);
//
//    glBegin(GL_QUADS);
//
//    //lower left
//    glMultiTexCoord2d(GL_TEXTURE0, 0, 0);
//    glMultiTexCoord2d(GL_TEXTURE1, 0, 0);
//    glMultiTexCoord2d(GL_TEXTURE2, 0, 0);
//    glVertex2f(-1, -1);
//
//    //upper left
//    glMultiTexCoord2d(GL_TEXTURE0, 0, 1);//maxCoords.t);
//    glMultiTexCoord2d(GL_TEXTURE1, 0, 1);//maxCoords.t);
//    glMultiTexCoord2d(GL_TEXTURE2, 0, maxCoords.t);
//    glVertex2f(-1, 1);
//
//    //upper right
//    glMultiTexCoord2d(GL_TEXTURE0, 1, 1);// maxCoords.s, maxCoords.t);
//    glMultiTexCoord2d(GL_TEXTURE1, 1, 1);//maxCoords.s, maxCoords.t);
//    glMultiTexCoord2d(GL_TEXTURE2, maxCoords.s, maxCoords.t);
//    glVertex2f(1, 1);
//
//    //lower right
//    glMultiTexCoord2d(GL_TEXTURE0, 1, 0);// maxCoords.s, 0);
//    glMultiTexCoord2d(GL_TEXTURE1, 1, 0);// maxCoords.s, 0);
//    glMultiTexCoord2d(GL_TEXTURE2, maxCoords.s, 0);
//    glVertex2f(1, -1);
//    glEnd();
//
//    //unbind the input texture
//    glBindTexture(GL_TEXTURE_2D, 0);
//    //usedShader->UnbindShader();
//
//
//    ///
//    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, systemCurrentFbo);
//
//    //defaultShader.BindShader();
//
//    glBindTexture(GL_TEXTURE_2D, dstTexture);
//    //glBindTexture(GL_TEXTURE_2D, texIds[currentTexId]);
//
//    glBegin(GL_QUADS);
//
//    //lower left
//    glTexCoord2d(0, 0);
//    glVertex2f(-1, -1);
//
//    //upper left
//    glTexCoord2d(0, 1); //maxCoords.t);
//    glVertex2f(-1, 1);
//
//    //upper right
//    glTexCoord2d(1, 1);// (maxCoords.s, maxCoords.t);
//    glVertex2f(1, 1);
//
//    //lower right
//    glTexCoord2d(1, 0); // (maxCoords.s, 0);
//    glVertex2f(1, -1);
//    glEnd();
//
//   // defaultShader.UnbindShader();
//
//    glMatrixMode(GL_PROJECTION);
//
//    //Restoring projection matrix
//    glPopMatrix();
//
//    //unbind the texture
//    glBindTexture(GL_TEXTURE_2D, 0);
//
//    //disable texturemapping
//    glDisable(GL_TEXTURE_2D);
//
//    //restore default color
//    glColor4f(1.f, 1.f, 1.f, 1.f);
//
//
//    this->currentTexId += 1;
//    this->currentTexId %= 2;

    return FF_SUCCESS;
    
    
    
    
    
    
    
    
    
    
    
    
    
    
//    FFGLTexCoords maxCoords = GetMaxGLTexCoords( *pGL->inputTextures[ 0 ] );
//
//    if(resolutionChanged(pGL->inputTextures[ 0 ]->Width, pGL->inputTextures[ 0 ]->Height))
//    {
//        fbos[0].Initialise(pGL->inputTextures[ 0 ]->HardwareWidth, pGL->inputTextures[ 0 ]->HardwareHeight);
//        fbos[1].Initialise(pGL->inputTextures[ 0 ]->HardwareWidth, pGL->inputTextures[ 0 ]->HardwareHeight);
//    }
//
//    {
//        ffglex::ScopedFBOBinding fbobind(fbos[index].GetGLID(), ScopedFBOBinding::RestoreBehaviour::RB_REVERT);
//
//        ScopedShaderBinding shaderBinding( shader.GetGLID() );
//
//        ScopedSamplerActivation activateSampler( 0 );
//        Scoped2DTextureBinding textureBinding( pGL->inputTextures[ 0 ]->Handle );
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
//        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
//
//        ScopedSamplerActivation activateSampler2( 1 );
//        Scoped2DTextureBinding textureBinding2( fbos[1 - index].GetTextureInfo().Handle );
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
//        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
//
//        shader.Set( "inputTexture", 0 );
//        shader.Set( "inputTexture2", 1 );
//        shader.Set( "MaxUV", maxCoords.s, maxCoords.t );
//
//        SendParams( shader );
//
//        quad.Draw();
//    }
//
//    ScopedShaderBinding s_shader_binding( dummy_shader.GetGLID() );
//
//    ScopedSamplerActivation s_activateSampler( 0 );
//    Scoped2DTextureBinding s_textureBinding( fbos[1-index].GetTextureInfo().Handle );
//    dummy_shader.Set( "inputTexture", 0 );
//    dummy_shader.Set( "MaxUV", maxCoords.s, maxCoords.t );
//
//    SendParams( dummy_shader );
//
//    quad.Draw();
//    index = 1 - index;
//
//    return FF_SUCCESS;
}

ASlider::FBOPair* ASlider::CreateFBO(const GLuint width, const GLuint height)
{

    GLenum status{ GL_INVALID_FRAMEBUFFER_OPERATION };
    FBOPair* pair = new FBOPair();

    // Init evenBufferId/evenTextureId
    glGenFramebuffers(1, &pair->firstBufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, pair->firstBufferId);

    glGenTextures(1, &pair->firstTextureId);
    glBindTexture(GL_TEXTURE_2D, pair->firstTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pair->firstTextureId, 0);

    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        return nullptr;

    // Init oddBufferId/oddTextureId
    glGenFramebuffers(1, &pair->secondBufferId);
    glBindFramebuffer(GL_FRAMEBUFFER, pair->secondBufferId);

    glGenTextures(1, &pair->secondTextureId);
    glBindTexture(GL_TEXTURE_2D, pair->secondTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pair->secondTextureId, 0);

    status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        return nullptr;

    pair->reversedDirection = false;

    return pair;

}

void ASlider::EnsureTexturesCreated(ProcessOpenGLStruct* pGL, int width, int height)
{
    const int patternSize{ width * height };

    GLubyte lut[256];
    for (int i = 0; i < 256; i++) lut[i] = i < 127 ? 0 : 255;

    GLubyte *phase = new GLubyte[patternSize];
    GLubyte* pattern = new GLubyte[patternSize];


    for (int i = 0; i < patternSize; i++)
        phase[i] = rand() % 256;

    texIds.resize(1);

    glGenTextures(1, texIds.data());

    for (int i = 0; i < 1; i++)
    {
        int arg{ i * 1 / 1 };

        for (int j = 0; j < patternSize; j++)
            pattern[j] = lut[(arg + phase[j]) % 1];

        glBindTexture(GL_TEXTURE_2D, texIds[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE8,  pGL->inputTextures[0]->Width, pGL->inputTextures[1]->Height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pattern);
    }

    delete phase;
    delete pattern;
}



FFResult ASlider::DeInitGL()
{
    shader.FreeGLResources();
    
    quad.Release();

    return FF_SUCCESS;
}
