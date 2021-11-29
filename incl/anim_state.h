#ifndef ANIM_STATE_H
#define ANIM_STATE_H

// Project Headers
#include "bvhdata.h"
#include "primitive.h"

// Std Headers
#include <vector>

// Info :  Class where animation processing is based (BVH, FK, IK). Exists within app as Tick Stage. 

class Anim_State
{
public:
	Anim_State();

	~Anim_State() = default;

	void set_bvhFile(const char *BVHPath);

	// Main Tick
	void tick();

	// Tick Opertaions
	void build_bones(Joint *joint, glm::mat4 trs);
	void build_skel(); 
	void render(const glm::mat4x4 &view, const glm::mat4x4 &persp);

	// Controls
	void set_frame(std::size_t Frame);
	void inc_frame();
	void dec_frame();

	// Create Render Prims
	void create_joint_prim(Joint *joint, const glm::vec4 &pos, const glm::vec3 &colour);
	void create_joint_sphere_prim(Joint *joint, const glm::vec4 &pos, const glm::vec3 &colour);
	void create_bone_prim(Joint *joint, const glm::vec4 &start, const glm::vec4 &end, const glm::vec3 &colour);

	// Debug
	void debug() const;

public:

	// BVH Data / Loader
	BVH_Data *bvh; 

	std::vector<Primitive*>  p_bones;   // Bone Line
	std::vector<Primitive*>  p_joints;  // Joint Points
	std::vector<Primitive*>  p_joints_spheres;  

	// Intrisnic Anim Data 
	std::size_t anim_frame, max_frame;
	float interval;
	bool anim_loop;
};

#endif