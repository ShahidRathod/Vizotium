<heightmap>

<vertex>

#version 440 core

layout(location = 0) in vec3 pos;
uniform in vec2 scale;
layout(location = 2) in vec3 offset;

out vec3 clr;

void main()
{
    clr = pos;
    gl_Position = vec4(scale*pos, 1.0);

}
</vertex>

<fragment>

#version 440 core
in vec3 clr;
out vec4 FragColor;

void main()
{
    FragColor = vec4(clr, 1.0);
}
</fragment>

</heightmap>


<surface>
</surface>