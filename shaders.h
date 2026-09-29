<heightmap>

<vertex>

#version 440 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 offset;
uniform vec3 scale;
uniform mat4 MVP;
uniform int grid_sz;


out vec3 clr;

void main()
{
    int i = gl_InstanceID;

    float x = i %(grid_sz);
    float y = i /(grid_sz);
    vec3 mappoint = vec3(x,y,0);
    
    clr =   offset+vec3(0.5,1,1);
    gl_Position = MVP*vec4(scale*(pos+mappoint) + offset, 1.0);

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