﻿/*********************************************************************
Matt Marchant 2015 - 2016
http://trederia.blogspot.com

xygine Sprite Editor - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using Newtonsoft.Json;

namespace SpriteEditor
{
    public partial class MainWindow : Form
    {
        private static string normalVertex =
                "#version 120\n" +
                "#define MAX_POINT_LIGHTS 8\n" +

                "uniform vec3 u_pointLightPositions[MAX_POINT_LIGHTS] = vec3[MAX_POINT_LIGHTS]\n" +
                "(\n" +
                "    vec3(0.0, 0.0, 230.0),\n" +
                "    vec3(0.0),\n" +
                "    vec3(0.0),\n" +
                "    vec3(0.0),\n" +
                "    vec3(0.0),\n" +
                "    vec3(0.0),\n" +
                "    vec3(0.0),\n" +
                "    vec3(0.0)\n" +
                ");\n" +
                "uniform vec3 u_cameraWorldPosition = vec3(960.0, 540.0, 1780.0);\n" +
                "uniform mat4 u_inverseWorldViewMatrix;\n" +

                "varying vec3 v_eyeDirection;\n" +
                "varying vec3 v_pointLightDirections[MAX_POINT_LIGHTS];\n" +

                "const vec3 tangent = vec3(1.0, 0.0, 0.0);\n" +
                "const vec3 normal = vec3(0.0, 0.0, 1.0);\n" +

                "void main()\n" +
                "{\n" +
                "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n" +
                "    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n" +
                "    gl_FrontColor = gl_Color;\n" +

                "    mat3 normalMatrix = transpose(mat3(u_inverseWorldViewMatrix));\n" +
                "    vec3 n = normalize(normalMatrix * normal);\n" +
                "    vec3 t = normalize(normalMatrix * tangent);\n" +
                "    vec3 b = cross(n,t);\n" +
                "    mat3 tangentSpaceTransformMatrix = mat3(t.x, b.x, n.x, t.y, b.y, n.y, t.z, b.z, n.z);\n" +

                "    vec3 viewVertex = vec3(gl_ModelViewMatrix * gl_Vertex);\n" +
                "    for(int i = 0; i < MAX_POINT_LIGHTS; ++i)\n" +
                "    {\n" +
                "        vec3 viewLightDirection = u_pointLightPositions[i] - viewVertex;\n" +
                "        v_pointLightDirections[i] = tangentSpaceTransformMatrix * viewLightDirection;\n" + 
                "    }\n" +

                "    v_eyeDirection = tangentSpaceTransformMatrix * ((gl_ModelViewMatrix * vec4(u_cameraWorldPosition, 1.0)).xyz - viewVertex);\n" +
                "}";

        private static string NORMAL_FRAGMENT_TEXTURED = "#version 120\n#define TEXTURED\n";
        private static string NORMAL_FRAGMENT_TEXTURED_SPECULAR = "#version 120\n#define TEXTURED\n#define SPECULAR\n";

        private static string normalFrag =
                "#define MAX_POINT_LIGHTS 8\n" +

                "struct PointLight\n" +
                "{\n" +
                "    vec4 diffuseColour;\n" +
                "    vec4 specularColour;\n" +
                "    float inverseRange;\n" +
                "    float intensity;\n" +
                "};\n" +

                "#if defined(TEXTURED)\n" +
                "uniform sampler2D u_diffuseMap;\n" +
                "#endif\n" +
                "uniform sampler2D u_normalMap;\n" +
                "uniform sampler2D u_maskMap;\n" +
                "uniform vec3 u_ambientColour = vec3 (0.2, 0.2, 0.2);\n" +

                "uniform PointLight u_pointLights[MAX_POINT_LIGHTS] = PointLight[MAX_POINT_LIGHTS]\n" +
                "(\n" +
                "    PointLight(vec4(1.0), vec4(1.0), 0.001, 1.0),\n" +
                "    PointLight(vec4(1.0), vec4(1.0), 0.5, 0.0),\n" +
                "    PointLight(vec4(1.0), vec4(1.0), 0.5, 0.0),\n" +
                "    PointLight(vec4(1.0), vec4(1.0), 0.5, 0.0),\n" +
                "    PointLight(vec4(1.0), vec4(1.0), 0.5, 0.0),\n" +
                "    PointLight(vec4(1.0), vec4(1.0), 0.5, 0.0),\n" +
                "    PointLight(vec4(1.0), vec4(1.0), 0.5, 0.0),\n" +
                "    PointLight(vec4(1.0), vec4(1.0), 0.5, 0.0)\n" +
                ");\n" +

                "varying vec3 v_eyeDirection;\n" +
                "varying vec3 v_pointLightDirections[MAX_POINT_LIGHTS];\n" +

                "vec4 diffuseColour;\n" +
                "vec4 maskColour;\n" +
                "vec3 calcLighting(vec3 normal, vec3 lightDirection, vec3 lightDiffuse, vec3 lightSpec, float falloff)\n" +
                "{\n" +
                "    float diffuseAmount = max(dot(normal, lightDirection), 0.0);\n" +
                "    diffuseAmount = pow((diffuseAmount * 0.5) + 0.5, 2.0);\n" +
                "    vec3 mixedColour = lightDiffuse * diffuseColour.rgb * diffuseAmount * falloff;\n" +

                /*Blinn-Phong specular calc*/
                "#if defined(SPECULAR)\n" +
                "    vec3 eyeDirection = normalize(v_eyeDirection);\n" +
                "    vec3 halfVec = normalize(lightDirection + eyeDirection);\n" +
                "    float specularAngle = clamp(dot(normal, halfVec), 0.0, 1.0);\n" +

                "    vec3 specularColour = lightSpec * vec3(pow(clamp(specularAngle, 0.0, 1.0), (maskColour.r * 255.0))) * falloff;\n" +
                "    return mixedColour + (specularColour * maskColour.g);\n" +
                "#else\n" +
                "    return mixedColour;\n" +
                "#endif\n" +
                "}\n" +

                "void main()\n" +
                "{\n" +
                "#if defined(TEXTURED)\n" +
                "    diffuseColour = texture2D(u_diffuseMap, gl_TexCoord[0].xy) * gl_Color;\n" +
                "#elif defined(COLOURED)\n" +
                "    diffuseColour = gl_Color;\n" +
                "#endif\n" +
                "    vec3 normalVector = texture2D(u_normalMap, gl_TexCoord[0].xy).rgb * 2.0 - 1.0;\n" +
                "    maskColour = texture2D(u_maskMap, gl_TexCoord[0].xy);\n" +

                "    vec3 blendedColour = diffuseColour.rgb * u_ambientColour;\n" +
                "    for(int i = 0; i < MAX_POINT_LIGHTS; ++i)\n" +
                "    {\n" +
                "        vec3 pointLightDir = v_pointLightDirections[i] * u_pointLights[i].inverseRange;\n" +
                "        float falloff = clamp(1.0 - sqrt(dot(pointLightDir, pointLightDir)), 0.0, 1.0);\n" +
                "        blendedColour += calcLighting(normalVector, normalize(v_pointLightDirections[i]), u_pointLights[i].diffuseColour.rgb, u_pointLights[i].specularColour.rgb, falloff) * u_pointLights[i].intensity;\n" +
                "    }\n" +
                "#if defined(SELF_ILLUM)\n" +
                "    gl_FragColor.rgb = mix(blendedColour, diffuseColour.rgb, maskColour.b);\n" +
                "#else\n" +
                "    gl_FragColor.rgb = blendedColour;\n" +
                "#endif\n" +
                "    gl_FragColor.a = diffuseColour.a;\n" +
                "}";


        private Stream ShaderAsStream(string s)
        {
            MemoryStream stream = new MemoryStream();
            StreamWriter writer = new StreamWriter(stream);
            writer.Write(s);
            writer.Flush();
            stream.Position = 0;
            return stream;
        }
    }
}