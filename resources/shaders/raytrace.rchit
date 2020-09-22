#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "raycommon.glsl"
#include "lights.glsl"

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec3 color;
  vec2 texCoord;
};

struct SceneDescription
{
  int modelIndex;
  int txtOffset;
  mat4 transform;
  mat4 transformIT;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;
hitAttributeEXT vec3 attribs;

//layout(binding = 0, set = 1) uniform sampler2D texSampler;

layout(binding = 0, set = 1, scalar) buffer Vertices
{
  Vertex v[];
} vertices[];

layout(binding = 1, set = 1) buffer Indices
{
  uint i[];
} indices[];

layout(binding = 0, set = 2) uniform LightSources
{
  DirectionalLight directionalLights[10];
  PointLight pointLights[10];
} lightSources;

layout(binding = 1, set = 2, scalar) buffer SceneDescriptions
{
  SceneDescription i[];
} sceneDescription;

layout(push_constant) uniform Constants
{
  vec4 clearColor;
};

void main()
{
  uint modelIndex = sceneDescription.i[gl_InstanceID].modelIndex;
  
  ivec3 ind = ivec3(indices[nonuniformEXT(modelIndex)].i[3 * gl_PrimitiveID + 0],   //
                    indices[nonuniformEXT(modelIndex)].i[3 * gl_PrimitiveID + 1],   //
                    indices[nonuniformEXT(modelIndex)].i[3 * gl_PrimitiveID + 2]);  //

  Vertex v0 = vertices[nonuniformEXT(modelIndex)].v[ind.x];
  Vertex v1 = vertices[nonuniformEXT(modelIndex)].v[ind.y];
  Vertex v2 = vertices[nonuniformEXT(modelIndex)].v[ind.z];

  const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
  vec3 normal = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
  normal = normalize(vec3(sceneDescription.i[gl_InstanceID].transformIT * vec4(normal, 0.0)));

  // Calculate world space position (the unprecise way)
  vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

  // or better like this:
  // Computing the coordinates of the hit position
  //vec3 worldPos = v0.pos * barycentrics.x + v1.pos * barycentrics.y + v2.pos * barycentrics.z;
  // Transforming the position to world space
  //worldPos = vec3(scnDesc.i[gl_InstanceID].transfo * vec4(worldPos, 1.0));

  vec3 L = normalize( lightSources.directionalLights[0].direction.xyz - vec3( 0 ) );
  float dotNL = max(dot(normal, L), 0.2 );

  prd.hitValue = vec3( dotNL );
}
