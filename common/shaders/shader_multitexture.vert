/* Input */
uniform mat4 u_MVPMatrix;   /* A constant representing the combined model/view/projection matrix. */
attribute vec3 a_Position;  /* Per-vertex position information */
attribute vec4 a_Color;     /* Per-vertex color information */
attribute vec2 a_Texcoord0; /* Per-vertex texcoord information */
attribute vec2 a_Texcoord1; /* Per-vertex texcoord information */
/* Output */
varying vec4 v_Color;
varying vec2 v_Texcoord0;
varying vec2 v_Texcoord1;

void main()
{
    /* Send the color to the fragment shader */
    v_Color = a_Color;
    /* Send the texcoords to the fragment shader */
    v_Texcoord0 = a_Texcoord0;
    v_Texcoord1 = a_Texcoord1;    
    /* gl_Position is a special variable used to store the final position. */
    /* Multiply the vertex by the matrix to get the final point in normalized screen coordinates. */
    gl_Position = u_MVPMatrix * vec4(a_Position, 1.0);
}
