#### BVH Animation Viewer Application

##### Info

This is a fork of a University Assignment I wrote in November 2021, that required loading `.bvh` motion capture / animation files, and rendering them (using a forward kinematics like approach). The rendering is implemented in Modern OpenGL (4.3) in a compact viewer application that allows for animation playback, scrubbing, stepping and free camera movement. This can be used to view BVH animation files. 

The parser assumes the rotation angles are in Euler (Z,Y,X) order and the only the root joint has translation channels (6 DOF including rotation) where each child joint has 3DOF (rotation only).

The `.bhv` hierarchy is re-built per frame / tick, this is not optimal as it involves depth first traversal using recursion and re-drawing the bone and joint primitives. Could be improved in future to only update transforms of pre-existing bone / skeleton data per tick, based on current set animation frame. 

Performance is not great currently, and could benefit from many optimization approaches, however as this was written at speed for an assignment within about 6 days, I had little time. For example, the Bone Primitives are re-built per tick, meaning GPU Resources are constantly in flux, ideally we would only update the tree / joint hierarchy per tick. 

___

##### Usage

Launch the application `./bvh_viewer` and input the bvh file string into the GUI text field, press `load_bvh` to load the set file path string. 

____

##### Building

This project was developed on Windows using MSVC C++, however it should build on Linux fine (when I get round to writing the make file) however the BVH parsing may produce issues with the carriage return `\r` char within the bvh files. 

___

##### Dependencies

* **GLM** - For Linear Algebra / transformation operations
* **GLEW** - OpenGL Function loading. *Ideally eliminate this dependency if time*. 
* **GLFW** - OpenGL Context, Window and Input handling. 
* **DearImGui** - GUI (via GLFW and OpenGL Implementation backends).
* **Stb Image** - Texture image file loading (jpg, png, tiff, etc). 

You will of course need to link with OpenGL `opengl32.lib` on Windows or `-lopengl32` on Linux. 

___

##### References 

* **BVH Format Guide** : *https://research.cs.wisc.edu/graphics/Courses/cs-838-1999/Jeff/BVH.html*

* **BVH reference code** :  *Masaki OSHITA (www.oshita-lab.org)*
