#version 400            
uniform mat4 PVM;
uniform int pass;

in vec3 pos_attrib;
in vec2 tex_coord_attrib;
in vec3 normal_attrib;

out vec2 tex_coord;  

void main(void)
{
	if(pass == 1)
	{
		gl_Position = PVM*vec4(pos_attrib, 1.0);
		tex_coord = tex_coord_attrib;
	}
	else
	{
		gl_Position = vec4(pos_attrib, 1.0);
      tex_coord = 0.5*pos_attrib.xy + vec2(0.5);
	}
}