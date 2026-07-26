
#include "Surface.h"

template <int coords_n> struct VectorEbo {
	Ebo_tringl cap1[coords_n];
	Ebo_sqre cylinder[coords_n];
	Ebo_tringl cap2[coords_n];
	Ebo_tringl cone[coords_n];
};

template <int coords_n> struct VectorVbo {
	
	Vertex centre1;
	Vertex cap1[coords_n];

	Vertex centre2[coords_n];
	Vertex cap2[coords_n];

	Vertex cone_center;
};

template <int coords_n> struct Vector {

	VectorVbo<coords_n> vbo;
	Vertex* c1 = vbo.circle1;
	Vertex* c2 = vbo.circle2;

	VectorEbo <coords_n> ebo;

	template <Vertex arr>
	inline void assign_circle_coords(int i,float angle,float x_val) {
		
		arr[i].Z = sin(angle);
		arr[i].Y = cos(angle);
		arr[i].X = x_val;
	}

	Vector() {

		vbo.centre1 = { 0,0,0 };
		vbo.centre2 = { 0,0,0 };
		vbo.cone_center = { 0,0,1 };

		// loop starts from i = 1
		// the arr[0] is the circle centre
		// for the n sided poly there are (n-1) sides, our loops also has n-1 iterations

		for (int i = 0; i < coords_n; i++)
		{
			float angle = (2 * i * PI) / coords_n;

			assign_circle_coords<vbo.cap1>(i, angle, 0);
			assign_circle_coords<vbo.cap2>(i, angle, 1);
		}

		for (int i = 0; i <  coords_n - 1; i++)
		{
			// ebo for cylinder 
			ebo.cylinder[i] = { {c1 + i - vbo,c1 + i + 1 - vbo,c2 + i - vbo},
								{c1 + i + 1 - vbo,c2 + i - vbo,c2 + i + 1 - vbo,i} };

			// ebo for cap1,cap2
			ebo.cap1[i] = { c1 + i - vbo,c1 + i + 1 - vbo,vbo.centre1 - vbo };
			ebo.cap2[i] = { c2 + i - vbo,c2 + i + 1 - vbo,vbo.centre2 - vbo };


			// ebo for cone
			ebo.cone2[i] = { c2 + i - vbo,c2 + i + 1 - vbo,&(vbo.cone_centre) - vbo };

		}
	}


};