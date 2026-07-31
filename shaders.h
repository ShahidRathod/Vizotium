<surface>

	<vertex>

#version 330 core
layout(location = 0) in vec3 coords;
layout(location = 1) in vec3 coords_side;

uniform mat4 MVP;
uniform float t;
uniform float f;
uniform float factr;
uniform bool is_grid;

uniform int XSZ;
uniform int YSZ;

out vec3 fragPos;
out vec3 sidePos;
out vec3 normal;


float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise2(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return 1.2*mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float sum = 0.0;
    float amp = 0.5;
    float freq = 1.0;
    for (int i = 0; i != 4; i++) {
        sum += amp * noise2(p * freq);
        amp *= 0.5;
        freq *= 2.1;
    }
    return 1.2*sum;
}

float spiral_y(vec3 p) {
    vec2 q = p.xz * 0.6;

    vec2 warpA = vec2(
        fbm(q + vec2(0.0, 0.0) + t * 0.15),
        fbm(q + vec2(5.2, 1.3) - t * 0.1)
    );
    vec2 warped = q + 1.4 * warpA;

    vec2 warpB = vec2(
        fbm(warped * 1.5 + vec2(1.7, 9.2) - t * 0.2),
        fbm(warped * 1.5 + vec2(8.3, 2.8) + t * 0.15)
    );
    vec2 finalCoord = warped + 1.0 * warpB;

    float terrain = fbm(finalCoord) - 0.5;   // centered around 0

    float swell1 = sin(1.5 * p.x + 0.8 * p.z - 1.8 * t);
    float swell2 = sin(2.2 * p.x - 1.1 * p.z + 1.3 * t + 1.0);
    float swell = 0.5 * swell1 + 0.5 * swell2;

    float r = length(p.xz);
    float falloff = exp(-0.08 * r * r);

    float h = falloff * (0.55 * terrain + 0.35 * swell);

    // crests that push past ~0.45 blow through your fragment shader's
    // R/B clip points -> reads as a glowing hot peak instead of banding
    float peakBoost = smoothstep(0.35, 0.6, h) * 0.25;

    return h + peakBoost;
}
void main()
{
    int x = gl_VertexID;


    vec3 pos = coords;
    vec3 side_pos = coords_side;

    side_pos.y = spiral_y(side_pos) / 2;

    pos.y = spiral_y(pos) ;

    if (is_grid) {
        pos.y += 0.001;
    }
    
    vec4 Pos = MVP * vec4(pos, 1.0);

    fragPos = pos;
    sidePos = coords_side;




    gl_Position = Pos;
}

</vertex>

<fragment>

#version 330 core

in vec3 fragPos;
in vec3 sidePos;
out vec4 FragColor;

in vec3 normal; 
uniform bool is_grid;
uniform vec4 grid_clr;

void main()
{
    if (is_grid) {
        FragColor   = grid_clr;

    }
    else {
        float h = 2.5 * fragPos.y;
        FragColor = vec4(h + h * h + 3 * h * h * h, 0.8 - h * h, 1 - 0.5 * exp(h), 1);

    }
}

</fragment>

</surface>


<grid> 

<vertex>

</vertex>

</grid>