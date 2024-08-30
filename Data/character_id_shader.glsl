uniform sampler2D texture;
uniform vec3 characterID;

void main()
{
    vec4 pixel_color = texture2D(texture, gl_TexCoord[0].xy);
    pixel_color.r = characterID.x;
    pixel_color.g = characterID.y;
    pixel_color.b = characterID.z;
    gl_FragColor = pixel_color; 
}