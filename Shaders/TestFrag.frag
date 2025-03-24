#version 450

struct GlobalData
{
	float m_Time;
	float m_DeltaTime;
	float m_Random1;
	float m_Random2;
};

layout(std140, set = 0, binding = 0) readonly buffer GlobalDataBufferType
{
    GlobalData elems[];
} GlobalDataBuffer;

layout(location = 0) in vec3 v_color;

layout(location = 0) out vec4 o_color;

void main()
{
    GlobalData global_data = GlobalDataBuffer.elems[0];

    float s = abs(sin(global_data.m_Time));

    o_color = vec4(v_color.x, v_color.y, s, 1.0);
}


