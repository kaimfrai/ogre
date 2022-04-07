/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_PRECOMPILEDHEADERS_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_PRECOMPILEDHEADERS_H

// Fixed Function Library: Common functions
char const constexpr inline FFP_FUNC_LERP[] =
    "FFP_Lerp";
char const constexpr inline FFP_FUNC_DOTPRODUCT[] =
    "FFP_DotProduct";
char const constexpr inline FFP_FUNC_NORMALIZE[] =
    "FFP_Normalize";

// Fixed Function Library: Transform functions
char const constexpr inline FFP_LIB_TRANSFORM[] =
    "FFPLib_Transform";
char const constexpr inline FFP_FUNC_TRANSFORM[] =
    "FFP_Transform";

// Fixed Function Library: Texturing functions
char const constexpr inline FFP_FUNC_TRANSFORM_TEXCOORD[] =
    "FFP_TransformTexCoord";
char const constexpr inline FFP_FUNC_GENERATE_TEXCOORD_ENV_NORMAL[] =
    "FFP_GenerateTexCoord_EnvMap_Normal";
char const constexpr inline FFP_FUNC_GENERATE_TEXCOORD_ENV_SPHERE[] =
    "FFP_GenerateTexCoord_EnvMap_Sphere";
char const constexpr inline FFP_FUNC_GENERATE_TEXCOORD_ENV_REFLECT[] =
    "FFP_GenerateTexCoord_EnvMap_Reflect";
char const constexpr inline FFP_FUNC_GENERATE_TEXCOORD_PROJECTION[] =
    "FFP_GenerateTexCoord_Projection";

char const constexpr inline FFP_FUNC_SAMPLE_TEXTURE_PROJ[] =
    "FFP_SampleTextureProj";
char const constexpr inline FFP_FUNC_MODULATEX2[] =
    "FFP_ModulateX2";
char const constexpr inline FFP_FUNC_MODULATEX4[] =
    "FFP_ModulateX4";
char const constexpr inline FFP_FUNC_ADDSIGNED[] =
    "FFP_AddSigned";
char const constexpr inline FFP_FUNC_ADDSMOOTH[] =
    "FFP_AddSmooth";

// Fixed Function Library: Fog functions
char const constexpr inline FFP_LIB_FOG[] =
    "FFPLib_Fog";
char const constexpr inline FFP_FUNC_VERTEXFOG_LINEAR[] =
    "FFP_VertexFog_Linear";
char const constexpr inline FFP_FUNC_VERTEXFOG_EXP[] =
    "FFP_VertexFog_Exp";
char const constexpr inline FFP_FUNC_VERTEXFOG_EXP2[] =
    "FFP_VertexFog_Exp2";
char const constexpr inline FFP_FUNC_PIXELFOG_DEPTH[] =
    "FFP_PixelFog_Depth";
char const constexpr inline FFP_FUNC_PIXELFOG_LINEAR[] =
    "FFP_PixelFog_Linear";
char const constexpr inline FFP_FUNC_PIXELFOG_EXP[] =
    "FFP_PixelFog_Exp";
char const constexpr inline FFP_FUNC_PIXELFOG_EXP2[] =
    "FFP_PixelFog_Exp2";

// Fixed Function Library: Alpha Test
char const constexpr inline FFP_LIB_ALPHA_TEST[] =
    "FFPLib_AlphaTest";
char const constexpr inline FFP_FUNC_ALPHA_TEST[] =
    "FFP_Alpha_Test";

char const constexpr inline SGX_LIB_PERPIXELLIGHTING[] =
    "SGXLib_PerPixelLighting";
char const constexpr inline SGX_FUNC_LIGHT_DIRECTIONAL_DIFFUSE[] =
    "SGX_Light_Directional_Diffuse";
char const constexpr inline SGX_FUNC_LIGHT_DIRECTIONAL_DIFFUSESPECULAR[] = "SGX_Light_Directional_DiffuseSpecular";
char const constexpr inline SGX_FUNC_LIGHT_POINT_DIFFUSE[] =
    "SGX_Light_Point_Diffuse";
char const constexpr inline SGX_FUNC_LIGHT_POINT_DIFFUSESPECULAR[] =
    "SGX_Light_Point_DiffuseSpecular";
char const constexpr inline SGX_FUNC_LIGHT_SPOT_DIFFUSE[] =
    "SGX_Light_Spot_Diffuse";
char const constexpr inline SGX_FUNC_LIGHT_SPOT_DIFFUSESPECULAR[] =
    "SGX_Light_Spot_DiffuseSpecular";

#endif 
