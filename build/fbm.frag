#version 330 core
in vec2 f_uv;
out vec4 color;
uniform float uTime;
uniform vec2 uResolution;

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(121.9, 305.2))) * 43758.5453);
}

float noise(vec2 p) {
	vec2 i = floor(p);
	vec2 fc = fract(p);
	vec2 u = fc * fc * (3.0 - 2.0 * fc);
	
	//four corners
	float a = hash(i + vec2(0.0, 0.0));
	float b = hash(i + vec2(1.0, 0.0));
	float c = hash(i + vec2(0.0, 1.0));
	float d = hash(i + vec2(1.0, 1.0));
	
	return mix(mix(a, b, u.x), mix(c,d, u.y), u.y);
}

float fbm(vec2 p){
    float v = 0.0;
    float a = 0.5;
    for(int i=0;i<5;i++){
        v += a * noise(p);
        p *= 2.0;
        a *= 0.5;
        p += vec2(0.5, 0.2);
    }
    return clamp(v, 0.0, 1.0);
}

void main() {
	vec2 uv = f_uv;
	vec2 p = (uv - 0.5) * vec2(uResolution.x/uResolution.y, 1.0) * 1.6;
	p += vec2(uTime * 0.02, uTime * 0.01);
	
	vec2 warp = vec2(fbm(p * 0.7 + vec2(5.2, 1.3)), fbm(p * 0.7 + vec2(2.1, 7.4))) - 0.5;
	p += warp * 0.9;
	
	float nebula = fbm(p * 0.8);
	float detail = fbm(p * 3.0 + vec2(12.3, 4.1)) * 0.6;
	float clouds = clamp(nebula * 0.7 + detail * 0.4, 0.0, 1.0);
	
	clouds = pow(clouds, 1.4);
	
	vec3 deep = vec3(0.04, 0.01, 0.02);
	vec3 mid = vec3(0.25, 0.06, 0.10);
	vec3 glow = vec3(0.85, 0.45, 0.60);
	
	vec3 col = mix(deep, mid, clouds);
	
	float highlight = smoothstep(0.62, 0.88, clouds);
	float mask = pow(highlight, 6.0);
	col += glow * mask * 1.6;
	col += glow * pow(clouds, 3.0) * 0.25;
	
	float coverLo = 0.05;
    float coverHi = 0.55;
    float cover = smoothstep(coverLo, coverHi, clouds);
    
    vec3 finalCol = mix(vec3(0.0), col, cover);
	
	float dist = length(uv - 0.5);
	float vignette = smoothstep(1.0, 0.25, dist);
	finalCol *= vignette;
	
	float exposure = 1.30;
	finalCol = 1.0 - exp(-finalCol * exposure);
	finalCol = pow(finalCol, vec3(0.95));
	
	color = vec4(finalCol, 1.0);
	
}