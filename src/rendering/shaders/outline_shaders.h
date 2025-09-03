#pragma once

namespace OutlineShaders {

// ==================== Normal Extrusion Method ====================
// 法向量扩展法：通过沿法向量方向扩展顶点来创建轮廓

const char* normalExtrusionVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uOutlineWidth;
uniform bool uIsOutlinePass;

void main() {
    vec3 position = aPosition;
    
    // 在轮廓渲染阶段，沿法向量扩展顶点
    if (uIsOutlinePass) {
        // 将法向量转换到视图空间
        vec3 normalView = normalize(mat3(uView * uModel) * aNormal);
        
        // 在视图空间中扩展顶点
        vec4 posView = uView * uModel * vec4(aPosition, 1.0);
        posView.xyz += normalView * uOutlineWidth;
        
        gl_Position = uProjection * posView;
    } else {
        // 正常渲染
        gl_Position = uProjection * uView * uModel * vec4(position, 1.0);
    }
}
)GLSL";

const char* normalExtrusionFragmentShader = R"GLSL(
#version 330 core

out vec4 FragColor;

uniform vec3 uOutlineColor;
uniform vec3 uObjectColor;
uniform bool uIsOutlinePass;

void main() {
    if (uIsOutlinePass) {
        FragColor = vec4(uOutlineColor, 1.0);
    } else {
        FragColor = vec4(uObjectColor, 1.0);
    }
}
)GLSL";

// ==================== Screen Space Outline ====================
// 屏幕空间轮廓：使用深度和法向量信息在后处理中生成轮廓

const char* screenSpaceOutlineVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    TexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)GLSL";

const char* screenSpaceOutlineFragmentShader = R"GLSL(
#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

// 输入纹理
uniform sampler2D uColorTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uNormalTexture;

// 轮廓参数
uniform vec2 uScreenSize;
uniform float uDepthThreshold;
uniform float uNormalThreshold;
uniform float uOutlineThickness;
uniform vec3 uOutlineColor;
uniform float uOutlineIntensity;

// 线性化深度值
float linearizeDepth(float depth) {
    float near = 0.1;
    float far = 100.0;
    return (2.0 * near) / (far + near - depth * (far - near));
}

// Roberts Cross边缘检测
float robertsCross(sampler2D tex, vec2 uv, vec2 texelSize) {
    float c = texture(tex, uv).r;
    float br = texture(tex, uv + texelSize).r;
    float tr = texture(tex, uv + vec2(texelSize.x, -texelSize.y)).r;
    float bl = texture(tex, uv + vec2(-texelSize.x, texelSize.y)).r;
    
    float gx = abs(c - br) + abs(tr - bl);
    float gy = abs(c - tr) + abs(br - bl);
    
    return sqrt(gx * gx + gy * gy);
}

// Sobel边缘检测（用于法向量）
float sobelNormal(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec3 tl = texture(tex, uv + vec2(-1, -1) * texelSize).rgb;
    vec3 tm = texture(tex, uv + vec2( 0, -1) * texelSize).rgb;
    vec3 tr = texture(tex, uv + vec2( 1, -1) * texelSize).rgb;
    vec3 ml = texture(tex, uv + vec2(-1,  0) * texelSize).rgb;
    vec3 mm = texture(tex, uv).rgb;
    vec3 mr = texture(tex, uv + vec2( 1,  0) * texelSize).rgb;
    vec3 bl = texture(tex, uv + vec2(-1,  1) * texelSize).rgb;
    vec3 bm = texture(tex, uv + vec2( 0,  1) * texelSize).rgb;
    vec3 br = texture(tex, uv + vec2( 1,  1) * texelSize).rgb;
    
    // Sobel X
    vec3 gx = -tl - 2.0*ml - bl + tr + 2.0*mr + br;
    // Sobel Y
    vec3 gy = -tl - 2.0*tm - tr + bl + 2.0*bm + br;
    
    return length(gx) + length(gy);
}

void main() {
    vec2 texelSize = 1.0 / uScreenSize;
    vec3 color = texture(uColorTexture, TexCoord).rgb;
    
    // 深度边缘检测
    float depthEdge = 0.0;
    float centerDepth = linearizeDepth(texture(uDepthTexture, TexCoord).r);
    
    // 使用多个采样点提高质量
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            vec2 offset = vec2(float(i), float(j)) * texelSize * uOutlineThickness;
            float sampleDepth = linearizeDepth(texture(uDepthTexture, TexCoord + offset).r);
            float diff = abs(centerDepth - sampleDepth);
            depthEdge = max(depthEdge, smoothstep(0.0, uDepthThreshold, diff));
        }
    }
    
    // 法向量边缘检测
    float normalEdge = sobelNormal(uNormalTexture, TexCoord, texelSize * uOutlineThickness);
    normalEdge = smoothstep(uNormalThreshold * 0.5, uNormalThreshold, normalEdge);
    
    // 合并边缘
    float edge = clamp(depthEdge + normalEdge, 0.0, 1.0) * uOutlineIntensity;
    
    // 应用轮廓
    vec3 finalColor = mix(color, uOutlineColor, edge);
    FragColor = vec4(finalColor, 1.0);
}
)GLSL";

// ==================== Inverted Hull Method ====================
// 反向外壳法：通过反向渲染扩展的几何体来创建轮廓

const char* invertedHullVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uOutlineWidth;

void main() {
    // 在模型空间中沿法向量扩展
    vec3 expandedPosition = aPosition + aNormal * uOutlineWidth;
    gl_Position = uProjection * uView * uModel * vec4(expandedPosition, 1.0);
}
)GLSL";

const char* invertedHullFragmentShader = R"GLSL(
#version 330 core

out vec4 FragColor;
uniform vec3 uOutlineColor;

void main() {
    FragColor = vec4(uOutlineColor, 1.0);
}
)GLSL";

// ==================== Jump Flooding Algorithm (JFA) ====================
// 跳跃泛洪算法：用于生成距离场的高效GPU算法

const char* jfaInitVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    TexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 0.0, 1.0);
}
)GLSL";

const char* jfaInitFragmentShader = R"GLSL(
#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uSilhouetteTexture;

void main() {
    float silhouette = texture(uSilhouetteTexture, TexCoord).r;
    if (silhouette > 0.5) {
        // 存储最近轮廓点的坐标
        FragColor = vec4(TexCoord, 0.0, 1.0);
    } else {
        // 无效值
        FragColor = vec4(-1.0, -1.0, 0.0, 0.0);
    }
}
)GLSL";

const char* jfaStepFragmentShader = R"GLSL(
#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uJFATexture;
uniform float uStepSize;
uniform vec2 uScreenSize;

void main() {
    vec2 texelSize = 1.0 / uScreenSize;
    vec4 currentSeed = texture(uJFATexture, TexCoord);
    float minDist = 9999999.0;
    vec2 closestSeed = currentSeed.xy;
    
    // 检查9个方向
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 sampleCoord = TexCoord + vec2(x, y) * uStepSize * texelSize;
            vec4 sampleSeed = texture(uJFATexture, sampleCoord);
            
            if (sampleSeed.x >= 0.0) {
                float dist = distance(TexCoord, sampleSeed.xy);
                if (dist < minDist) {
                    minDist = dist;
                    closestSeed = sampleSeed.xy;
                }
            }
        }
    }
    
    FragColor = vec4(closestSeed, minDist, 1.0);
}
)GLSL";

const char* jfaOutlineFragmentShader = R"GLSL(
#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uColorTexture;
uniform sampler2D uDistanceField;
uniform float uOutlineWidth;
uniform vec3 uOutlineColor;
uniform vec2 uScreenSize;

void main() {
    vec3 color = texture(uColorTexture, TexCoord).rgb;
    float distance = texture(uDistanceField, TexCoord).z * length(uScreenSize);
    
    // 使用距离场生成轮廓
    float outline = 1.0 - smoothstep(0.0, uOutlineWidth, distance);
    outline *= smoothstep(uOutlineWidth * 2.0, uOutlineWidth, distance);
    
    vec3 finalColor = mix(color, uOutlineColor, outline);
    FragColor = vec4(finalColor, 1.0);
}
)GLSL";

// ==================== Geometry Shader Outline ====================
// 几何着色器轮廓：在几何着色器中生成轮廓线

const char* geometryOutlineVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

out vec3 Normal;
out vec3 Position;

uniform mat4 uModel;
uniform mat4 uView;

void main() {
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    Position = vec3(uModel * vec4(aPosition, 1.0));
    gl_Position = uView * vec4(Position, 1.0);
}
)GLSL";

const char* geometryOutlineGeometryShader = R"GLSL(
#version 330 core

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

in vec3 Normal[];
in vec3 Position[];

uniform mat4 uProjection;
uniform vec3 uViewPosition;
uniform float uCreaseAngle;

bool isSilhouetteEdge(vec3 p0, vec3 p1, vec3 n0, vec3 n1) {
    vec3 viewDir0 = normalize(uViewPosition - p0);
    vec3 viewDir1 = normalize(uViewPosition - p1);
    
    float dot0 = dot(n0, viewDir0);
    float dot1 = dot(n1, viewDir1);
    
    return dot0 * dot1 < 0.0;
}

bool isCreaseEdge(vec3 n0, vec3 n1) {
    float angle = acos(clamp(dot(n0, n1), -1.0, 1.0));
    return angle > uCreaseAngle;
}

void emitEdge(int i0, int i1) {
    gl_Position = uProjection * gl_in[i0].gl_Position;
    EmitVertex();
    gl_Position = uProjection * gl_in[i1].gl_Position;
    EmitVertex();
    EndPrimitive();
}

void main() {
    // 计算三角形法向量
    vec3 faceNormal = normalize(cross(
        Position[1] - Position[0],
        Position[2] - Position[0]
    ));
    
    // 检查每条边
    for (int i = 0; i < 3; i++) {
        int next = (i + 1) % 3;
        
        // 这里应该检查相邻面，简化起见只检查当前面
        if (isSilhouetteEdge(Position[i], Position[next], faceNormal, faceNormal) ||
            isCreaseEdge(Normal[i], Normal[next])) {
            emitEdge(i, next);
        }
    }
}
)GLSL";

const char* geometryOutlineFragmentShader = R"GLSL(
#version 330 core

out vec4 FragColor;
uniform vec3 uOutlineColor;

void main() {
    FragColor = vec4(uOutlineColor, 1.0);
}
)GLSL";

} // namespace OutlineShaders