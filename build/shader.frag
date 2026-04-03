#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main()
{
    vec2 c = gl_PointCoord - vec2(0.5);
    float d = length(c) / 0.5; // normalized 0..~1 across the point square

    // core parameters
    float coreR = 0.25;
    float coreEdge = 0.10;
    float core = 1.0 - smoothstep(coreR - coreEdge, coreR + coreEdge, d);

    // halo parameters (separate radius and edge)
    float haloR = 0.75;        // center of halo falloff (0..1)
    float haloEdge = 0.20;     // softness/thickness of halo edge
    float haloIntensity = 0.30; // overall halo strength
    float halo = (1.0 - smoothstep(haloR - haloEdge, haloR + haloEdge, d)) * haloIntensity;

    // combine core + halo using the star color (no separate haloColor)
    float alpha = clamp(core + halo, 0.0, 1.0);
    if (alpha < 0.01) discard; // make round sprite

    FragColor = vec4(vColor * (core + halo), alpha);
}