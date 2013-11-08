/* Input */
varying vec4 v_Color;
varying vec2 v_Texcoord0;
varying vec2 v_Texcoord1;
uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;

void main()
{
    /* Sample the textures */
    vec4 texal0 = texture2D( u_Texture0, v_Texcoord0.st );
    vec4 texal1 = texture2D( u_Texture1, v_Texcoord1.st );
    
    gl_FragColor = mix( texal0, texal1, texal1.a  ) * v_Color;
}

