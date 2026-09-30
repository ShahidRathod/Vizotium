<heightmap>

<vertex>

#version 440 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 offset;
layout(location = 2) in float height;

uniform vec3 scale;
uniform mat4 MVP;
uniform int grid_sz;

out vec3 clr;

vec3 color(float h)
{
    h = clamp(h, 0.0, 1.0);

    if (h < 0.25)
        return mix(vec3(0.267, 0.005, 0.329),
            vec3(0.230, 0.322, 0.546), h / 0.25);

    if (h < 0.50)
        return mix(vec3(0.230, 0.322, 0.546),
            vec3(0.128, 0.567, 0.551), (h - 0.25) / 0.25);

    if (h < 0.75)
        return mix(vec3(0.128, 0.567, 0.551),
            vec3(0.369, 0.789, 0.383), (h - 0.50) / 0.25);

    return mix(vec3(0.369, 0.789, 0.383),
        vec3(0.993, 0.906, 0.144), (h - 0.75) / 0.25);
}

void main()
{
    int i = gl_InstanceID;
    i = i % (grid_sz * grid_sz);
    float x = i % grid_sz;
    float y = i / (grid_sz);

    vec3 mappoint = vec3(x, y, 0);

    clr = color(height);
    gl_Position = vec4(scale * (pos + mappoint) + offset, 1.0);

}

< / vertex>

<fragment>

#version 440 core
in vec3 clr;
out vec4 FragColor;

void main()
{

    FragColor = vec4(clr, 1.0);
}
< / fragment>

< / heightmap>


<surface>

<vertex>

#version 440 core
uniform int grid_sz;
out vec3 clr;

void main () {

    clr = vec3(1,1,1);
    int i = gl_VertexID;
    i = i % (grid_sz * grid_sz);
    float x = i % grid_sz;
    float y = i / (grid_sz);

    gl_Position = vec4(x/2, y/2, 0,grid_sz)/(grid_sz);

}

</vertex>

<fragment>

<$heightmap:fragment>

</fragment>

< / surface>