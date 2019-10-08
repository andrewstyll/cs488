#version 330

in vec3 vertToFragColor;

out vec4 fragColor;

void main() {
	fragColor = vec4( vertToFragColor, 1 );
}
