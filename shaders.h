<heightmap>

<vertex>

#version 440 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 offset;
layout(location = 2) in float height;
layout(location = 3) in float height_clr;
layout(location = 4) in float noise;


uniform vec3 scale;
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

const vec3 xyoffset = vec3(0, 0, 0);

<xyzcoords>

vec3 xyzcoords(int i, float h) {
    i = i % (grid_sz * grid_sz);
    int x_i = (i % grid_sz) ;
    int y_i = (i / grid_sz) ;
    
    float x = x_i;
    float y = y_i;

    int is_even = int((x_i + y_i) % 2 != 0);
    return vec3(x/grid_sz - 0.5, y/grid_sz - 0.5, h);
}
< / xyzcoords>



void main()
{
    vec3 mappoint = xyzcoords(gl_InstanceID, 0);

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

layout(location = 0) in vec3 offset;
layout(location = 1) in float height;
layout(location = 2) in float heightclr;
layout(location = 3) in float noise;
layout(location = 4) in float noiseclr;

uniform int grid_sz;
uniform vec2 mapscale;
uniform mat4 MVP;

out vec3 clr;

<$viridis>
<$xyzcoords>


void main() {
    
    int ins_id = gl_InstanceID;
    
    float h = height;
    float color_h = (ins_id == 0) ? noise : height;
    
    vec3 coords = xyzcoords(gl_VertexID, h);

    clr = color(color_h);

    
    
    if (ins_id == 2)
    {
        coords.yz = coords.zy;
        gl_Position = MVP * vec4(coords, 1);//+ vec4(offset,1);
    }
    else {

        coords.xy *= mapscale;
        coords.xy += offset.xy;
        gl_Position = vec4(coords,1);

    }
    
}

< / vertex>

<fragment>
<$heightmap:fragment>


< / fragment>
< / surface>


<line>

<vertex>
#version 440 core

uniform int grid_sz;
uniform mat4 MVP;


/*layout(binding = 6, std430) readonly buffer line_buffer {
    float height[];
};
*/

layout(location = 2) in float height;


//layout(location = 5) in vec3 height;
//uniform vec3 eye;
//unifrom float thickness;

<$xyzcoords>

void main () {

    //int v_id = gl_VertexID;
    //float h = height[v_id];
    
    int v = gl_VertexID;
    int x_i = v;
    int y_i = v / 2;

    float x = float(x_i) * 0.5 - 0.5;
    float y = float(y_i) * 0.5 - 0.5;

    gl_Position = MVP*vec4(x, y, 0.0, 1.0);
  
    //vec3 coords = xyzcoords(gl_VertexID, h2);

    //vec3 p1 = coords - dot(eye - coords, point);
    //vec3 p2 = coords - dot(eye - coords, point);

    //vec3 p1 = coords ;
    //vec3 p2 = coords ;
    
    //vec3 p12 = p2 - p1;
    //p12.x = -p12.y;
    //p12.y = p12.;
    

}
</vertex>

<fragment>
#version 440 core

out vec4 FragColor;


void main() {

    FragColor = vec4(1, 1, 1, 1);
}

</fragment>

<line>