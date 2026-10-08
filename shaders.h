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

</viridis>

const vec3 xyoffset = vec3(0, 0, 0);

<xyzcoords>

vec3 xyzcoords(int i, float h) {
    i = i % (grid_sz * grid_sz);
    int x_i = (i % grid_sz);
    int y_i = (i / grid_sz);

    float x = x_i;
    float y = y_i;

    int is_even = int((x_i + y_i) % 2 != 0);
    return vec3(x / grid_sz - 0.5, y / grid_sz - 0.5, h-0.35);
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

vec3 contour(vec3 c)
{
    float fac = 100.0;
    float interval = 10.0;
    float k = fac * c.z;

    for (float i = -fac; i < fac; i += interval)
    {
        if (i-0.5 <= k && k < i + 0.5)
            return vec3(1.0);
    }

    return c;
}


void main() {


    int ins_id = gl_InstanceID;

    float h = height;
    float color_h = (ins_id == 0) ? noise : height;

    vec3 coords = xyzcoords(gl_VertexID, h);

    if (ins_id == 0) color_h = pow(color_h, 0.11f);
    clr = color(color_h);



    if (ins_id == 2)
    {
        coords.yz = coords.zy;
        gl_Position = MVP * vec4(coords, 1);

    }
    else {

        coords.xy *= mapscale;
        coords.xy += offset.xy;
        gl_Position = vec4(coords, 1);
        if (ins_id == 1) clr = contour(clr);
         
    }

}

</   vertex>

<fragment>
<$heightmap:fragment>


</fragment>
</surface>


<line>

<vertex>
#version 440 core

uniform int grid_sz;
uniform mat4 MVP;
uniform int nolines;

layout(location = 3) in float height_clr;

layout(binding = 6, std430) readonly buffer line_buffer {
    float height[];
};

out vec3 clr;


<$xyzcoords>

<$viridis>

int index(int v_id, int ins_id) {
    int line_space = grid_sz / nolines;
    int t = ins_id / (nolines);
    int i = v_id / 2;
    int j = line_space * ins_id;
    int index =
        i * (1 - t) + i * t * grid_sz +
        j * t + j * (1 - t) * grid_sz;

    return index;
}


vec3 line_point(int v_id, int ins_id) {
    int line_space = grid_sz / nolines;
    int t = ins_id / (nolines);
    int i = v_id/2;
    int j = line_space*ins_id;
    int index =
        i * (1 - t) + i * t * grid_sz +
        j * t + j * (1 - t) * grid_sz;
    
    //float(i + j) / (grid_sz * 2))
    return xyzcoords(index, height[index]);

}

void main() {


    int v_id = gl_VertexID;
    int ins_id = gl_InstanceID;

    int p1_index = index(v_id, ins_id);

    vec3 p1 = line_point(v_id, ins_id);
    
    p1.yz = p1.zy;
    vec4 p1_mvp = MVP * vec4(p1, 1.0);
    
    bool is_even = v_id % 2 == 0;

   
    clr = color(height[p1_index]);

    if (is_even) {
        gl_Position = p1_mvp;
        
    }
    else {
        vec3 p2 = line_point(v_id + 2, ins_id);
        p2.yz = p2.zy;

        vec4 p2_mvp = MVP * vec4(p2, 1.0);

        float w1 = p1_mvp[3];
        float w2 = p2_mvp[3];

        p1_mvp /= w1;
        p2_mvp /= w2;

        vec2 line = p2_mvp.xy - p1_mvp.xy;
        vec2 line_p;
        
        line_p.x = -line.y;
        line_p.y = line.x;

        line_p = normalize(line_p) / (2*grid_sz);

        p1_mvp.xy += line_p;
        gl_Position = p1_mvp;
    }

}

</  vertex>

<fragment>
#version 440 core


out vec4 FragColor;

in vec3 clr;

void main() {

    float b = 1;
    FragColor = vec4(clr*0.75, b);
}

< / fragment>

< /line>
