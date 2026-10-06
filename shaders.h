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
uniform int nolines;


layout(binding = 6, std430) readonly buffer line_buffer {
    float height[];
};


layout(location = 2) in float h;


//layout(location = 5) in vec3 height;
//uniform vec3 eye;
//unifrom float thickness;

<$xyzcoords>

vec3 line_point(int v_id, int ins_id) {

    int line_space = grid_sz / nolines;

    int t = ins_id / (nolines);

    int i = v_id / 2;
    int j = ins_id ;

    int index =
        i * (1 - t) + i * t * line_space +
        j * t + j * (1 - t) * line_space;
        

    int index = i + j * grid_sz;
    return xyzcoords(v_id, height[v_is]);
}

void main() {

    int v_id = gl_VertexID;
    int ins_id = gl_InstanceID;

    vec3 p1 = line_point(v_id, ins_id);

    bool is_even = v_id % 2 == 0;

    vec4 mvp_p1 = MVP * vec4(p1, 1);
    mvp_p1.yz = mvp_p1.zy;
    gl_PointSize = 20.0;

    if (is_even) {
        gl_Position = mvp_p1;
    }

    else {


        vec4 mvp_p2 = vec4(line_point(v_id + 2, ins_id), 1);
        mvp_p2.yz = mvp_p2.zy;

        float w1 = mvp_p1[3];
        float w2 = mvp_p2[3];

        mvp_p1 /= w1;
        mvp_p2 /= w2;


        vec2 line = mvp_p2.xy - mvp_p1.xy;

        vec2 line_up;

        line_up.x = -line_up.y;
        line_up.y = line_up.x;

        mvp_p1.xy += line_up * 0.001;
        vec4 coords = mvp_p1 * w1;

        gl_Position = coords;
    }
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