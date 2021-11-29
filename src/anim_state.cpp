// Implements 
#include "anim_state.h"

// Ext Headers
// GLM
#include "ext/glm/gtc/matrix_transform.hpp"
#include "ext/glm/gtc/type_ptr.hpp"


// Basic Ctor, state is set on demmand as needed. 
Anim_State::Anim_State()
{
	anim_loop = true;
	anim_frame = 0;
	max_frame = 0;

	bvh = nullptr;
}

void Anim_State::set_bvhFile(const char *BVHPath)
{
	// Clear prev state (could just bvh::clear())
	if (bvh) delete bvh;

	bvh = new BVH_Data(BVHPath);
	bvh->Load();

	max_frame = bvh->num_frame;
	interval  = bvh->interval;
}

void Anim_State::build_skel()
{
	// Reset Primtive Arrays
	p_bones.clear();
	p_joints.clear();

	// Build
	Joint *root = bvh->joints[0];
	build_bones(root, glm::mat4(1.f));
}

void Anim_State::build_bones(Joint *joint, glm::mat4 trs)
{
	//  =========== Get Translation  ===========
	if (!joint->parent) // Root joint, translation from channels. 
	{
		glm::vec4 root_offs(0., 0., 0., 1.);

		for (const Channel *c : joint->channels)
		{
			switch (c->type)
			{
				// Translation
			case ChannelEnum::X_POSITION:
			{
				float x_p = bvh->motion[anim_frame * bvh->num_channel + c->index];
				root_offs.x = x_p;
				break;
			}
			case ChannelEnum::Y_POSITION:
			{
				float y_p = bvh->motion[anim_frame * bvh->num_channel + c->index];
				root_offs.y = y_p;
				break;
			}
			case ChannelEnum::Z_POSITION:
			{
				float z_p = bvh->motion[anim_frame * bvh->num_channel + c->index];
				root_offs.z = z_p;
				break;
			}
			}
		}

		trs = glm::translate(trs, glm::vec3(root_offs));

	}
	else if (joint->parent) // Non root joints, Translation is offset. 
	{
		trs = glm::translate(trs, joint->offset);
	}

	// =========== Get Rotation ===========
	glm::mat4 xx(1.), yy(1.), zz(1.);
	for (const Channel *c : joint->channels)
	{
		switch (c->type)
		{
		case ChannelEnum::Z_ROTATION:
		{
			float z_r = bvh->motion[anim_frame * bvh->num_channel + c->index];
			trs = glm::rotate(trs, glm::radians(z_r), glm::vec3(0., 0., 1.));
			break;
		}
		case ChannelEnum::Y_ROTATION:
		{
			float y_r = bvh->motion[anim_frame * bvh->num_channel + c->index];
			trs = glm::rotate(trs, glm::radians(y_r), glm::vec3(0., 1., 0.));
			break;
		}
		case ChannelEnum::X_ROTATION:
		{
			float x_r = bvh->motion[anim_frame * bvh->num_channel + c->index];
			trs = glm::rotate(trs, glm::radians(x_r), glm::vec3(1., 0., 0.));
			break;
		}
		}
	}

	// Transformations Applied to vertices using current concatenated matrix.

	// ==================== Viz Joints as Points ====================
	// Viz Joints as Points 
	glm::vec4 v0 = trs * glm::vec4(0.f, 0.f, 0.f, 1.f);
	create_joint_prim(joint, v0);
	//skel.add_bone(glm::vec3(v0), glm::vec3(v0), glm::mat4(1.f));

	/*
	// Joint point (as line)
	// ==================== End Point ====================
	if (joint->is_end)
	{
		glm::vec4 v1 = trs * glm::vec4(joint->end, 1.f);
		// Joint point (as line)
		//skel.add_bone(glm::vec3(v1), glm::vec3(v1), glm::mat4(1.f));
		// Bone Line 
		skel.add_bone(glm::vec3(v0), glm::vec3(v1), glm::mat4(1.f));
	}
	*/

	// ==================== Children ====================
	// Pass each recurrsive call its own copy of the current (parent) transformations to then apply to children.
	for (std::size_t c = 0; c < joint->children.size(); ++c) 
	{
		// =========== Add Bone for each child offset, rel to parent transform ===========
		//Joint *child = joint->children[c];
		//glm::vec4 v2 = trs * glm::vec4(child->offset, 1.f);
		// Bone Line
		//skel.add_bone(glm::vec3(v0), glm::vec3(v2), glm::mat4(1.f));
		
		// =========== Recurse for all joint children ===========
		build_bones(joint->children[c], trs);
	}

}

void Anim_State::tick()
{
	build_skel();
}

void Anim_State::render(const glm::mat4x4 &view, const glm::mat4x4 &persp)
{
	// ============== Render Joints ==============
	for (Primitive *prim : p_joints)
	{
		// Set Camera Transform
		prim->set_cameraTransform(view, persp);
		glPointSize(10.f);
		prim->render();
	}
}

// Set Animation Frame Member Functions
// Inc and Dec loop. 

// Need to make sure inc/dec is only done for interval of current glfw dt. 

void Anim_State::inc_frame()
{
	anim_frame = ++anim_frame > max_frame ? 0 : anim_frame;
}

void Anim_State::dec_frame()
{
	anim_frame = --anim_frame < 0 ? 0 : anim_frame;
}

void Anim_State::set_frame(std::size_t Frame)
{
	anim_frame = Frame > max_frame ? max_frame : Frame;
}



void Anim_State::create_joint_prim(Joint *joint, const glm::vec3 &pos)
{
	std::string name = "joint_" + joint->name;
	Primitive *j_prim = new Primitive(name.c_str());

	// Mesh Data
	float data[11] = { 0.f };
	data[0] = pos.x, data[1] = pos.y, data[2] = pos.z,
	j_prim->set_data_mesh(data, 1);

	// Prim Shader
	j_prim->set_shader("../../shaders/basic.vert", "../../shaders/colour.frag");

	// Init Prim Attribs
	j_prim->scale(glm::vec3(0.05f));
	j_prim->mode = Render_Mode::RENDER_POINTS;

	// Append to Joint Prims
	p_joints.push_back(j_prim);
}




void Anim_State::debug() const
{
	std::cout << "Anim::" << bvh->filename << " ::Frame = " << anim_frame << "\n";
}

