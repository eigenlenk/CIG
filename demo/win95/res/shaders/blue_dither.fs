#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

out vec4 finalColor;

void main() {
    vec4 texColor = texture(texture0, fragTexCoord);

    if (texColor.a > 0.0) {
        ivec2 ipos = ivec2(gl_FragCoord.xy);

        if ((ipos.x + ipos.y) % 2 == 0) {
            texColor.rgb = vec3(0, 0, 0.5);
        }
    }

    finalColor = texColor;
}