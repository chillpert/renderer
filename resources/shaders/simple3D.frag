#version 450
// necessary for vulkan shaders
#extension GL_ARB_separate_shader_objects : enable

// IN
layout(location = 0) in vec3 fragColor;

// OUT
layout(location = 0) out vec4 outColor;

void main()
{
  outColor = vec4(fragColor, 1.0);
}