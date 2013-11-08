/* Input */
uniform mat4 u_MVPMatrix;   /* A constant representing the combined model/view/projection matrix */
uniform float u_PointSize;  /* Point size for GL_POINTS */
uniform vec4 u_Color;       /* Color information */
attribute vec3 a_Position;  /* Per-vertex position information */
/* Output */
varying vec4 v_Color;

void main()
{
    /* Send the color to the fragment shader */
    v_Color = u_Color;
    /* gl_Position is a special variable used to store the final position. */
    /* Multiply the vertex by the matrix to get the final point in normalized screen coordinates. */
    gl_Position = u_MVPMatrix * vec4(a_Position, 1.0);
    gl_PointSize = u_PointSize;    
}
