// Implements 
#include "anim_state.h"

// Std Headers
#include <random>
#include <iomanip>
#include <fstream>

// Project Headers
#include "mesh.h"

// Ext Headers
// GLM
#include "ext/glm/gtc/matrix_transform.hpp"
#include "ext/glm/gtc/type_ptr.hpp"

#define DRAW_END_EFFECTOR 0 

// =================== Anim_State Implementation ===================

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

	// Reset Anim state
	anim_frame = 0;
	anim_loop = true; 
}

void Anim_State::build_skel()
{
	// Reset Primtive Arrays
	p_bones.clear();
	p_joints.clear();
	p_joints_spheres.clear();

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
	glm::vec4 v0 = trs * glm::vec4(0.f, 0.f, 0.f, 1.f);

	// Joint (point)
//	create_joint_prim(joint, v0, glm::vec3(0.f, 0.f, 0.f));

	// Joint (Sphere)
	create_joint_sphere_prim(joint, v0, glm::vec3(0.f));

	#if DRAW_END_EFFECTOR != 0 
	// ==================== Joint End Site ====================
	if (joint->is_end)
	{
		glm::vec4 v1 = trs * glm::vec4(joint->end, 1.f);
		// End Site Joint (point)
		create_joint_prim(joint, v1, glm::vec3(1.f, 0.f, 0.f));
		// End Site Bone (line)
		create_bone_prim(joint, v0, v1, glm::vec3(1.f, 0.f, 0.f));
	}
	#endif
	// ==================== Children ====================
	// Pass each recurrsive call its own copy of the current (parent) transformations to then apply to children.
	for (std::size_t c = 0; c < joint->children.size(); ++c) 
	{
		// =========== Add Bone for each child offset, rel to parent transform ===========
		Joint *child = joint->children[c];
		glm::vec4 v2 = trs * glm::vec4(child->offset, 1.f);

		// Bone (line)
		create_bone_prim(joint, v0, v2, glm::vec3(0.f, 0.f, 1.f));
		
		// =========== Recurse for all joint children ===========
		build_bones(joint->children[c], trs);
	}
}

void Anim_State::tick()
{
	if (anim_loop) inc_frame();

	build_skel();
}

void Anim_State::render(const glm::mat4x4 &view, const glm::mat4x4 &persp)
{
	// ============== Render Joints (as points) ==============
	for (Primitive *prim : p_joints)
	{
		// Scale Model Matrix (Post BVH transform) 
		prim->scale(glm::vec3(0.05f));

		// Set Camera Transform
		prim->set_cameraTransform(view, persp);

		// Render
		glPointSize(10.f);
		prim->render();
	}
	// ============== Render Joints (as Mesh Sphere) ==============
	for (Primitive *prim : p_joints_spheres)
	{
		// Scale Model Matrix (Post BVH transform) 
		prim->scale(glm::vec3(0.05f));
		prim->model[3] = prim->model[3] * glm::vec4(0.05f, 0.05f, 0.05f, 1.f); // Also Scale Translation (Post Scale)

		// Set Camera Transform
		prim->set_cameraTransform(view, persp);

		// Render
		prim->render();
	}
	// ============== Render Bones (as lines) ==============
	for (Primitive *prim : p_bones)
	{
		// Scale Model Matrix (Post BVH transform) 
		prim->scale(glm::vec3(0.05f));

		// Colour per bone ID
		std::mt19937_64 rng;
		std::uniform_real_distribution<float> dist(0.0, 1.0);
		int32_t seed = prim->name[(prim->name.size() - 1)] + prim->name[(prim->name.size() - 2)];
		rng.seed(seed);        float r = dist(rng);
		rng.seed(seed + 124);  float g = dist(rng);
		rng.seed(seed + 321);  float b = dist(rng);
		prim->set_colour(glm::vec3(r, g, b));

		// Set Camera Transform
		prim->set_cameraTransform(view, persp);

		// Render
		glLineWidth(5.f);
		prim->render();
	}
}

// =============== Set Animation Frame Member Functions ===============

// Increment and Decrement Animation Frame
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


// =============== Create Render Prims Member Functions ===============

// Joint as single point
void Anim_State::create_joint_prim(Joint *joint, const glm::vec4 &pos, const glm::vec3 &colour)
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
	j_prim->set_colour(colour);
	j_prim->mode = Render_Mode::RENDER_POINTS;

	// Append to Joint Prims
	p_joints.push_back(j_prim);
}

void Anim_State::create_joint_sphere_prim(Joint *joint, const glm::vec4 &pos, const glm::vec3 &colour)
{
	std::string name = "joint_" + joint->name;
	Mesh *j_spher = new Mesh(name.c_str(), "../../assets/mesh/sphere.obj");

	j_spher->load_obj(false);
	j_spher->set_shader("../../shaders/basic.vert", "../../shaders/colour.frag");
	j_spher->set_colour(glm::vec3(1.f, 0.f, 0.f));

	// Transform Operations (mesh model mat)
	// Scale Sphere By Radius (LS)
	//j_spher->scale(glm::vec3(2.f, 2.f, 2.f));

	// Translate to Joint Pos (WS)
	j_spher->translate(pos);

	// Append to Joint Prims
	p_joints_spheres.push_back(j_spher);
}

void Anim_State::create_bone_prim(Joint *joint, const glm::vec4 &start, const glm::vec4 &end, const glm::vec3 &colour)
{
	std::string name = "Bone_" + std::to_string(p_bones.size() + 1);
	Primitive *b_prim = new Primitive(name.c_str());

	// Mesh Data
	std::vector<vert> line_data; line_data.resize(2);
	line_data[0].pos = glm::vec3(start), line_data[1].pos = glm::vec3(end);
	b_prim->set_data_mesh(line_data);

	// Prim Shader
	b_prim->set_shader("../../shaders/basic.vert", "../../shaders/colour.frag");

	// Init Prim Attribs
	b_prim->set_colour(colour);
	b_prim->mode = Render_Mode::RENDER_LINES;

	// Append to Bone Prims
	p_bones.push_back(b_prim);
}

// =============== Debug Member Functions ===============

void Anim_State::debug() const
{
	std::cout << "Anim::" << bvh->filename << " ::Frame = " << anim_frame << "\n";
}


// =============== Write out BVH File Member Functions ===============

void Anim_State::write_bvh(const char *out_path)
{
	// ============== Check output file is good ==============
	std::ofstream out(out_path);
	if (!out.is_open())
	{
		std::cerr << "ERROR::Writing BVH File::" << out_path << "::Path is not valid" << std::endl;
		std::terminate();
	}

	// ============== Init Write Stream ==============
	std::stringstream stream; 
	stream << "HIERARCHY\n";

	// ============== Get Joint Hierachy ==============
	write_bvh_traverse(bvh->joints[0], stream);

	// ============== Get Motion Data ==============
	stream << "MOTION\nFrames: " << bvh->num_frame << "\nFrame Time: " << bvh->interval << "\n";

	for (std::size_t c = 0; c < (bvh->num_channel * bvh->num_frame); ++c)
	{
		stream << std::fixed << std::setprecision(6) << bvh->motion[c]; 
		//if (c != 0 && c % bvh->num_channel == 0) stream << "\n"; else stream << " ";
		if ((c+1) % bvh->num_channel == 0) stream << "\n"; else stream << " ";
	}

	stream << "\n";

	// Debug
	//std::cout << "Stream Output :\n\n" << stream.str() << "\n";
	// ============== Write to file ==============
	out << stream.str();
	out.close(); 
}

void Anim_State::write_bvh_traverse(Joint *joint, std::stringstream &stream)
{
	// Get tab indent for joint based on index in tree. 
	std::string tab; for (std::size_t i = 0; i < joint->idx; ++i) tab += "\t";
	
	if (joint->is_root) // Root joint, translation from channels. 
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

		// We know root has 6 DOFs so we can hardcode these. 
		stream << "ROOT " << joint->name << "\n{\n\t"
			<< "OFFSET " << std::fixed << std::setprecision(6)
			<< root_offs.x << " " << root_offs.y << " " << root_offs.z << "\n\t"
			<< "CHANNELS 6 Xposition Yposition Zposition Zrotation Yrotation Xrotation\n";
	}
	else // Standard Joint
	{
		stream << tab.c_str() << "JOINT " << joint->name << "\n" << tab.c_str() << "{\n" << tab.c_str() << "\t" 
			<< "OFFSET " << std::fixed << std::setprecision(6)
			<< joint->offset.x << " " << joint->offset.y << " " << joint->offset.z << "\n" << tab.c_str() << "\t"
			<< "CHANNELS 3 Zrotation Yrotation Xrotation\n";

		// End Site
		if (joint->is_end)
		{
			stream << tab.c_str() << "\tEnd Site\n" << tab.c_str() << "\t{\n" << tab.c_str() << "\t\t"
				<< "OFFSET " << joint->end.x << " " << joint->end.y << " " << joint->end.z 
				<< "\n" << tab.c_str() << "\t}\n";
		}
	}

	for (Joint *child : joint->children)
	{
		write_bvh_traverse(child, stream);
	}

	stream << tab.c_str() << "}\n";
}
