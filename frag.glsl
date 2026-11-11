
#version 330 core

out vec4 FragColor;
uniform sampler2D u_state;
uniform ivec2 u_size;
uniform ivec2 u_mouse;
uniform int u_painting;
uniform int u_frame;

bool inside(ivec2 p) {
    return p.x >= 0 && p.y >= 0 && p.x < u_size.x && p.y < u_size.y;
}

bool occupied(ivec2 p) {
    return inside(p) && texelFetch(u_state, p, 0).r > 0.5;
}

ivec2 target(ivec2 p) {
    ivec2 down = p + ivec2(0, -1);

    if (inside(down) && !occupied(down))
        return down;

    ivec2 left = p + ivec2(-1, -1);
    ivec2 right = p + ivec2(1, -1);

    bool canLeft = inside(left) && !occupied(left);
    bool canRight = inside(right) && !occupied(right);

    if (canLeft && canRight) {
        int dir = ((p.x + p.y) & 1) == 0 ? -1 : 1;
        return p + ivec2(dir, -1);
    }

    if (canLeft) return left;
    if (canRight) return right;

    return ivec2(-1);
}

bool same(ivec2 a, ivec2 b) {
    return all(equal(a, b));
}

void main() {
    ivec2 p = ivec2(gl_FragCoord.xy);
    bool sand = occupied(p);

    if (u_painting == 1 && distance(vec2(p), vec2(u_mouse)) < 5.0) {
        FragColor = vec4(1.0);
        return;
    }

    if (sand) {
        ivec2 t = target(p);
        FragColor = vec4(t.x >= 0 ? 0.0 : 1.0);
        return;
    }

    ivec2 sources[3] = ivec2[3](
        p + ivec2(0, 1),
        p + ivec2(-1, 1),
        p + ivec2(1, 1)
    );

    bool incoming = false;

    for (int i = 0; i < 3; i++) {
        ivec2 s = sources[i];

        if (occupied(s) && same(target(s), p))
            incoming = true;
    }
    vec4 color= vec4(incoming ? 1.0 : 0.0);
    if (u_painting == 1 && distance(vec2(p),vec2(90,90))<50){
        color = vec4(1.0,0.0,0.0,1.0);
    }
    FragColor = color;
}
