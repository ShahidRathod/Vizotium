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
<viridis>
vec3 color(float h)
{
    h = clamp(h, 0.0, 1.0);
    const vec3 c0 = vec3(0.2446, 0.0284, 0.3552);
    const vec3 c1 = vec3(1.3097, 1.2498, 1.0515);
    const vec3 c2 = vec3(-8.5613, -0.5851, -1.0321);
    const vec3 c3 = vec3(13.5837, 0.7695, -0.8296);
    const vec3 c4 = vec3(-5.5793, -0.5587, 0.5341);

    return clamp(c0 + h * (c1 + h * (c2 + h * (c3 + h * c4))), 0.0, 1.0);
}
< / viridis>

const vec3 xyoffset = vec3(0,0,0);

<xyzcoords>
vec3 xyzcoords(int i,float h) {
    i = i % (grid_sz * grid_sz);
    float x = i % grid_sz;
    float y = i / (grid_sz);
    return vec3(x, y, h) + xyoffset;
}
</xyzcoords>

void main()
{
    vec3 mappoint = xyzcoords(gl_InstanceID,0);

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
layout(location = 2) in float height;
uniform int grid_sz;
uniform mat4 MVP;
out vec3 clr;

<$viridis>
const vec3 xyoffset = vec3(-grid_sz/2, -grid_sz/2, 0);
<$xyzcoords>
void main () {

    float h = 8*height;
    clr = color(height);
    vec3 coords = xyzcoords(gl_VertexID,h);
    coords.yz = coords.zy;
    gl_Position = MVP*vec4(coords, grid_sz) / (grid_sz);

}

</vertex>

<fragment>

<$heightmap:fragment>

</fragment>

< / surface>