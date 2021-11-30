#### BVH Animation Data Loader (with Rendering and Export)

##### Info

This is a fork of a University Assignment, that required loading `.bvh` motion capture / animation files, and rendering them (using a forward kinematics like approach). The rendering is implemented in Modern OpenGL (4.3) in a compact viewer application that allows for animation playback, scrubbing, stepping and free camera movement. This can be used to view BVH animation files. 

The parser assumes the rotation angles are in Euler (Z,Y,X) order and the only the root joint has translation channels (6 DOF including rotation) where each child joint has 3DOF (rotation only).

The `.bhv` hierarchy is re-built per frame / tick, this is not optimal as it involves depth first traversal using recursion and re-drawing the bone and joint primitives. Could be improved in future to only update transforms of pre-existing bone / skeleton data per tick, based on current set animation frame. 

##### Usage

As there is not GUI present at the moment, use the following keyboard inputs to control the animation : 

[..]

##### Building

This project was developed on Windows using MSVC C++, however it should build on Linux fine (when I get round to writing the make file) however the BVH parsing may produce issues with the carriage return `\r` char within the bvh files. 

##### Dependencies

* **GLM** - For Linear Algebra / transformation operations
* **GLEW** - OpenGL Function loading
* **GLFW** - OpenGL Context, Window and Input handling. 
* **Stb Image** - Texture loading. 

You will of course need to link with OpenGL `opengl32.lib` on Windows or `-lopengl32` on Linux. 

##### References 

* **BVH Format Guide** : *https://research.cs.wisc.edu/graphics/Courses/cs-838-1999/Jeff/BVH.html*

* **BVH reference code** :  *Masaki OSHITA (www.oshita-lab.org)*

