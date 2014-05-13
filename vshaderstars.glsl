#version 120
attribute vec4 vPosition;
attribute vec4 vColor;
attribute float size;
uniform mat4 camera_view;
uniform mat4 projection_view;
varying vec4 fColor;

void
main()
{
    gl_Position = projection_view * camera_view * vPosition;
    gl_PointSize = size;
    fColor = vColor;
}
