#version 330
layout(lines) in;
layout(points, max_vertices = 102) out;

flat in vec3 pre_g[];
flat in vec3 post_g[];

in float tick_f[];

out float tick_f2;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;

		float HermiteInterpolate(
			float y0, float y1,
			float y2, float y3,
			float mu,
			float tension,
			float bias)
		{
			float m0, m1, mu2, mu3;
			float a0, a1, a2, a3;

			mu2 = mu * mu;
			mu3 = mu2 * mu;
			m0 = (y1 - y0)*(1 + bias)*(1 - tension) / 2;
			m0 += (y2 - y1)*(1 - bias)*(1 - tension) / 2;
			m1 = (y2 - y1)*(1 + bias)*(1 - tension) / 2;
			m1 += (y3 - y2)*(1 - bias)*(1 - tension) / 2;
			a0 = 2 * mu3 - 3 * mu2 + 1;
			a1 = mu3 - 2 * mu2 + mu;
			a2 = mu3 - mu2;
			a3 = -2 * mu3 + 3 * mu2;

			return (a0*y1 + a1*m0 + a2*m1 + a3*y2);
		}

void main() {

    
    gl_Position = MVP * gl_in[0].gl_Position;
    gl_PointSize = 10.0f;
    tick_f2 = tick_f[0];
    EmitVertex();
    
    gl_Position = MVP * gl_in[1].gl_Position;
    gl_PointSize = 10.0f;
    tick_f2 = tick_f[1];
    EmitVertex();
    
	const float tension = 0.0f;
	const float bias = 0.0f;

    for (int i = 1; i < 101; ++i) {
        //gl_Position = gl_in[0].gl_Position + (gl_in[1].gl_Position - gl_in[0].gl_Position) * i / 101.0;
        
		float mu = i / 100.0f;

		gl_Position = MVP * vec4(	
							HermiteInterpolate(pre_g[0].x, gl_in[0].gl_Position.x, gl_in[1].gl_Position.x, post_g[1].x, mu, tension, bias),
							HermiteInterpolate(pre_g[0].y, gl_in[0].gl_Position.y, gl_in[1].gl_Position.y, post_g[1].y, mu, tension, bias),
							HermiteInterpolate(pre_g[0].z, gl_in[0].gl_Position.z, gl_in[1].gl_Position.z, post_g[1].z, mu, tension, bias), 1.0f );

        gl_PointSize = 3.0f;
        
        tick_f2 = tick_f[1] - tick_f[0];
        
        EmitVertex();
    }
    
    EndPrimitive();
}

