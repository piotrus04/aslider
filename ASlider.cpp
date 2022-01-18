#include "ASlider.h"
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


static const char _dummyvertexShaderCode[] = R"(#version 410 core
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


static const char _dummyfragmentShaderCode[] = R"(#version 410 core
uniform sampler2D inputTexture;

in vec2 uv;
in vec2 norm_uv; // si on prend pas le normuv, ça dépeint d'un pixel

out vec4 fragColor;

void main()
{
    vec4 smp0 = texture(inputTexture, norm_uv);
    fragColor = smp0;
}
)";



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

    AddParam( ffglqs::ParamRange::Create( "Up",  1., {1.0, 40.0} ) );
    AddParam( ffglqs::ParamRange::Create( "Down",  1., {1.0, 40.0} ) );
    
    last_width = -1;
    last_height = -1;
}
ASlider::~ASlider()
{
}

FFResult ASlider::InitGL( const FFGLViewportStruct* vp )
{
    if( !shader.Compile( _vertexShaderCode, _fragmentShaderCode ) )
    {
        cout << "!dummy_shader.Compile(_dummyvertexShaderCode, _dummyfragmentShaderCode)"<< "\n";

        DeInitGL();
        return FF_FAIL;
    }
    
    if( !dummy_shader.Compile(_dummyvertexShaderCode, _dummyfragmentShaderCode) )
    {
        DeInitGL();
        cout << "!dummy_shader.Compile(_dummyvertexShaderCode, _dummyfragmentShaderCode)"<< "\n";

        return FF_FAIL;
    }
    
    if( !quad.Initialise() )
    {
        
        cout << "!quad.Initialise()"<< "\n";
        DeInitGL();
        return FF_FAIL;
    }

    cout << "INITALIZED ASLIDER\n";
    
    //Use base-class init as success result so that it retains the viewport.
    return CFFGLPlugin::InitGL( vp );
}

bool ASlider::resolutionChanged(int width, int height)
{
    bool bResolutionChange;

    if((width != last_width) || (height != last_height))
    {
        last_width = width;
        last_height = height;
        bResolutionChange = true;
//        printf("init change size %u %u %u %u\n", width, height, last_width);
    }
    else
        bResolutionChange = false;
    
    return bResolutionChange;
}


FFResult ASlider::ProcessOpenGL( ProcessOpenGLStruct* pGL )
{
    if( pGL->numInputTextures < 1 )
        return FF_FAIL;

    if( pGL->inputTextures[ 0 ] == NULL )
        return FF_FAIL;

    FFGLTexCoords maxCoords = GetMaxGLTexCoords( *pGL->inputTextures[ 0 ] );


    if(resolutionChanged(pGL->inputTextures[ 0 ]->Width, pGL->inputTextures[ 0 ]->Height))
    {
        Resize(&currentViewport);
//        fbos[0].Initialise(pGL->inputTextures[ 0 ]->HardwareWidth, pGL->inputTextures[ 0 ]->HardwareHeight);
//        fbos[1].Initialise(pGL->inputTextures[ 0 ]->HardwareWidth, pGL->inputTextures[ 0 ]->HardwareHeight);
    }

    {
        ffglex::ScopedFBOBinding fbobind(fbos[index].GetGLID(), ScopedFBOBinding::RestoreBehaviour::RB_REVERT);

        ScopedShaderBinding shaderBinding( shader.GetGLID() );

        ScopedSamplerActivation activateSampler( 0 );
        Scoped2DTextureBinding textureBinding( pGL->inputTextures[ 0 ]->Handle );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

        ScopedSamplerActivation activateSampler2( 1 );
        Scoped2DTextureBinding textureBinding2( fbos[1 - index].GetTextureInfo().Handle );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

        shader.Set( "inputTexture", 0 );
        shader.Set( "inputTexture2", 1 );
        shader.Set( "MaxUV", maxCoords.s, maxCoords.t );

        SendParams( shader );

        quad.Draw();
    }

    ScopedShaderBinding s_shader_binding( dummy_shader.GetGLID() );

    ScopedSamplerActivation s_activateSampler( 0 );
    Scoped2DTextureBinding s_textureBinding( fbos[1-index].GetTextureInfo().Handle );
    dummy_shader.Set( "inputTexture", 0 );
    dummy_shader.Set( "MaxUV", maxCoords.s, maxCoords.t );

    SendParams( dummy_shader );

    quad.Draw();
    
    index = 1 - index;
        
    return FF_SUCCESS;
}

unsigned int ASlider::Resize( const FFGLViewportStruct* vp )
{
    
    if(resolutionChanged(vp->width, vp->height))
    {
        fbos[0].Release();
        fbos[1].Release();
        fbos[0].Initialise(vp->width, vp->height);
        fbos[1].Initialise(vp->width, vp->height);
    }

    currentViewport = *vp;

    return FF_SUCCESS;
}

FFResult ASlider::DeInitGL()
{
    shader.FreeGLResources();
    dummy_shader.FreeGLResources();
    fbos[0].Release();
    fbos[1].Release();
    quad.Release();

    return FF_SUCCESS;
}
