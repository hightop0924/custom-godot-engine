

// get inworld position example
/*
Camera3D *const camera =  get_viewport()->get_camera_3d();
auto from = camera->project_ray_origin(get_viewport()->get_mouse_position());
auto to = from + camera->project_ray_normal(get_viewport()->get_mouse_position()) * 1000;
PhysicsDirectSpaceState3D::RayParameters ray_params;
ray_params.from = from;
ray_params.to = to;
PhysicsDirectSpaceState3D::RayResult r;
bool intersected = get_tree()->get_root()->get_world_3d()->get_direct_space_state()->intersect_ray(ray_params, r);
*/

#include "rigging_dock.h"
#include <signal.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

RiggingSystemDock* RiggingSystemDock::singleton = nullptr;

#include "g_skeleton.h"
#include "rigging_dock_mech.ply.h"

#include "editor/scene_tree_dock.h"
#include "scene/main/node.h"
#include "core/io/dir_access.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_player.h"
#include "editor/editor_node.h"
#include "scene/resources/surface_tool.h"
#include "scene/main/viewport.h"

#include "obj_parser.h"


struct surface_t
{
	PackedVector3Array vertices = PackedVector3Array();
	PackedVector3Array normals = PackedVector3Array();
	PackedVector3Array texcoord = PackedVector3Array();
	PackedInt32Array indices = PackedInt32Array();
	PackedFloat32Array weights = PackedFloat32Array();
	PackedInt32Array bones = PackedInt32Array();
	Ref<StandardMaterial3D> material = nullptr;
};

struct bone_t
{
	float r;
	int id;
};
// You can find examples of working with animation in "tests/scene/test_animation.h"

#if !defined(TEST_JSON)
const char TEST_JSON[] = R"JSON(
[
	{
		"name": "root",
		"parent": -1,
		"transform_mat": [
			[1.0, 0.0, 0.0, 0.0],
			[0.0, 1.0, 0.0, 0.0],
			[0.0, 0.0, 1.0, 0.0],
			[0.0, 0.0, 0.0, 1.0]
		]
	},
	{
		"name": "bone0",
		"parent": 0,
		"transform_mat": [
			[1.0, 0.0, 0.0, 0.0],
			[0.0, 1.0, 0.0, 0.0],
			[0.0, 0.0, 1.0, 0.0],
			[0.0, 0.0, 0.0, 1.0]
		]
	},
	{
		"name": "bone01",
		"parent": 1,
		"transform_mat": [
			[1.0, 0.0, 0.0, 0.0],
			[0.0, 1.0, 0.0, 0.0],
			[0.0, 0.0, 1.0, 0.0],
			[0.0, 0.0, 0.0, 1.0]
		]
	},
	{
		"name": "bone1",
		"parent": 0,
		"transform_mat": [
			[1.0, 0.0, 0.0, 0.0],
			[0.0, 1.0, 0.0, 0.0],
			[0.0, 0.0, 1.0, 0.0],
			[0.0, 0.0, 0.0, 1.0]
		]
	},
	{
		"name": "bone11",
		"parent": 3,
		"transform_mat": [
			[1.0, 0.0, 0.0, 0.0],
			[0.0, 1.0, 0.0, 0.0],
			[0.0, 0.0, 1.0, 0.0],
			[0.0, 0.0, 0.0, 1.0]
		]
	}
])JSON";
#endif


void print_trans(const Transform3D& pr)
{
	printf(
		"        {\n"
		"            {%-6f, %-6f, %-6f, %-6f},\n"
		"            {%-6f, %-6f, %-6f, %-6f},\n"
		"            {%-6f, %-6f, %-6f, %-6f},\n"
		"            {%-6f, %-6f, %-6f, %-6f}\n"
		"        },\n",
		pr.basis[0][0], pr.basis[0][1], pr.basis[0][2], 0.0,
		pr.basis[1][0], pr.basis[1][1], pr.basis[1][2], 0.0,
		pr.basis[2][0], pr.basis[2][1], pr.basis[2][2], 0.0,
		pr.origin[0], pr.origin[1], pr.origin[2], 1.0
	);
}

bool parse_json_skeleton(RiggingSystemDock::skvector_t* skvector, const JSON& json)
{
	print_line(__FUNCTION__);
	ERR_FAIL_COND_V(!skvector, false);
	ERR_FAIL_COND_V(!json.get_data().is_array(), false);


	skvector->clear();


	const Array array = json.get_data();

	for (int i = 0; i < array.size(); i++)
	{
		ERR_FAIL_COND_V(array[i].get_type() != Variant::Type::DICTIONARY, false);
		Dictionary dict = array[i];
		ERR_FAIL_COND_V(!dict.has("name"), false);
		ERR_FAIL_COND_V(!dict.has("parent"), false);
		ERR_FAIL_COND_V(!dict.has("transform_mat"), false);

		ERR_FAIL_COND_V(dict["name"].get_type() != Variant::Type::STRING, false);
		ERR_FAIL_COND_V(
			dict["parent"].get_type() != Variant::Type::FLOAT &&
			dict["parent"].get_type() != Variant::Type::INT,
			false);
		ERR_FAIL_COND_V(!dict["transform_mat"].is_array(), false);



		RiggingSystemDock::skeleton_bone_t bone;

		bone.name = (const char*)(String(dict["name"]).ascii());
		if (dict["parent"].get_type() == Variant::Type::FLOAT)
			bone.parent = (float)dict["parent"];
		else
			if (dict["parent"].get_type() == Variant::Type::FLOAT)
				bone.parent = dict["parent"];
		Array transform_mat = dict["transform_mat"];
		for (int a = 0; a < 4; a++)
		{
			ERR_FAIL_COND_V(!transform_mat[a].is_array(), false);
			Array transform_mat_line = transform_mat[a];

			for (int b = 0; b < 4; b++)
				bone.mat[a][b] = transform_mat_line[b];
		}

		skvector->push_back(bone);
	}

	return true;
}

void rig(ArrayMesh*& mesh, Skeleton3D*& skelet)
{
	int ss_s = mesh->get_surface_count();
	std::vector<surface_t> ss;
	for (int __s = 0; __s < ss_s; __s++)
	{
		surface_t s;
		auto arrs = mesh->surface_get_arrays(__s);
		s.vertices = arrs[Mesh::ARRAY_VERTEX];
		s.weights = arrs[Mesh::ARRAY_WEIGHTS];
		s.bones = arrs[Mesh::ARRAY_BONES];

		s.normals = arrs[Mesh::ARRAY_NORMAL];
		s.texcoord = arrs[Mesh::ARRAY_TEX_UV];
		s.indices = arrs[Mesh::ARRAY_INDEX];

		printf("%i %i %i %i %i %i \n",
			s.vertices.size(),
			s.weights.size(),
			s.bones.size(),
			s.normals.size(),
			s.texcoord.size(),
			s.indices.size()
		);

		s.material = mesh->surface_get_material(__s);

		ss.push_back(s);
	}

	mesh->clear_surfaces();

	mesh = memnew(ArrayMesh);

	int s_s = skelet->get_bone_count();

	std::vector<Transform3D> rests;
	std::vector<int> parents;
	std::vector<String> names;

	for (int j = 0; j < s_s; j++)
	{
		rests.push_back(skelet->get_bone_pose(j));
		parents.push_back(skelet->get_bone_parent(j));
		names.push_back(skelet->get_bone_name(j));
	}

	skelet->clear_bones();

	for (int j = 0; j < s_s; j++)
	{
		skelet->add_bone(names[j]);
		skelet->set_bone_rest(j, rests[j]);
		if (parents[j] >= 0)
			skelet->set_bone_parent(j, parents[j]);
	}

	skelet->reset_bone_poses();
	skelet->register_skin(skelet->create_skin_from_rest_transforms());


	for (int __s = 0; __s < ss_s; __s++)
	{
		auto& vertices = ss[__s].vertices;
		auto& normals = ss[__s].normals;
		auto& texcoord = ss[__s].texcoord;
		auto& indices = ss[__s].indices;
		auto& weights = ss[__s].weights;
		auto& bones = ss[__s].bones;

		bones.clear();
		weights.clear();

		for (int i = 0; i < vertices.size(); i++)
		{
			std::vector<bone_t> bs;
			const auto& v = vertices[i];
			for (int j = 0; j < s_s; j++)
			{
				const auto& b = skelet->get_bone_global_rest(j).origin;
				float r = (b - v).length_squared();

				bs.push_back({ r, j });
			}

			bs[0].r = 1e20f;

			for (int j = 0; j < s_s; j++)
			{
				for (int k = j + 1; k < s_s; k++)
				{
					auto a = bs[j];
					auto b = bs[k];
					if (a.r > b.r)
					{
						bs[j] = b;
						bs[k] = a;
					}
				}
			}


			bs[0].r = 100;
			bs[1].r = 5;
			bs[2].r = 5;
			bs[3].r = 1;

			float s = bs[0].r + bs[1].r + bs[2].r + bs[3].r;

			for (int k = 0; k < 4; k++)
			{
				bones.push_back(bs[k].id);
				weights.push_back(bs[k].r / s);
			}
		}

		Array arrays;
		arrays.resize(Mesh::ARRAY_MAX);


		arrays[Mesh::ARRAY_VERTEX] = vertices;
		if (normals.size() > 0)
		{
			arrays[Mesh::ARRAY_NORMAL] = normals;
		}
		if (texcoord.size() > 0)
		{
			arrays[Mesh::ARRAY_TEX_UV] = texcoord;
		}
		arrays[Mesh::ARRAY_INDEX] = indices;
		arrays[Mesh::ARRAY_WEIGHTS] = weights;
		arrays[Mesh::ARRAY_BONES] = bones;
		int s_id = mesh->get_surface_count();
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);


		mesh->surface_set_material(s_id, ss[__s].material);
	}

}

void unrig(ArrayMesh*& mesh)
{
	int ss_s = mesh->get_surface_count();
	std::vector<surface_t> ss;
	for (int __s = 0; __s < ss_s; __s++)
	{
		surface_t s;
		auto arrs = mesh->surface_get_arrays(__s);
		s.vertices = arrs[Mesh::ARRAY_VERTEX];
		s.weights = arrs[Mesh::ARRAY_WEIGHTS];
		s.bones = arrs[Mesh::ARRAY_BONES];

		s.normals = arrs[Mesh::ARRAY_NORMAL];
		s.texcoord = arrs[Mesh::ARRAY_TEX_UV];
		s.indices = arrs[Mesh::ARRAY_INDEX];

		printf("%i %i %i %i %i %i \n",
			s.vertices.size(),
			s.weights.size(),
			s.bones.size(),
			s.normals.size(),
			s.texcoord.size(),
			s.indices.size()
		);

		s.material = mesh->surface_get_material(__s);

		ss.push_back(s);
	}

	mesh->clear_surfaces();

	mesh = memnew(ArrayMesh);

	for (int __s = 0; __s < ss_s; __s++)
	{
		surface_t& s = ss[__s];

		auto& vertices = s.vertices;
		auto& normals = s.normals;
		auto& texcoord = s.texcoord;
		auto& indices = s.indices;
		auto& weights = s.weights;
		auto& bones = s.bones;

		bones.clear();
		weights.clear();

		printf("%i %i %i %i %i %i \n",
			vertices.size(),
			weights.size(),
			bones.size(),
			normals.size(),
			texcoord.size(),
			indices.size()
		);



		Array arrays;
		arrays.resize(Mesh::ARRAY_MAX);


		arrays[Mesh::ARRAY_VERTEX] = vertices;
		if (normals.size() > 0)
		{
			arrays[Mesh::ARRAY_NORMAL] = normals;
		}
		if (texcoord.size() > 0)
		{
			arrays[Mesh::ARRAY_TEX_UV] = texcoord;
		}
		arrays[Mesh::ARRAY_INDEX] = indices;

		int s_id = mesh->get_surface_count();
		printf("%i\n", s_id);
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);


		mesh->surface_set_material(s_id, ss[__s].material);
	}
}

RiggingSystemDock::RiggingSystemRet RiggingSystemDock::Gen(std::string obj, skvector_t* skvector)
{
	print_line("step 1");
	ArrayMesh* mesh = memnew(ArrayMesh);
	MeshInstance3D* meshinst = memnew(MeshInstance3D);

	std::vector<surface_t> surfacees;
	if (obj.size() == 0)
	{
		surfacees.push_back(surface_t());
		for (int i = 0; i < rigging_dock_vertex_s; i++)
			surfacees[0].vertices.append(Vector3(rigging_dock_vertex[i * 3], rigging_dock_vertex[i * 3 + 1], rigging_dock_vertex[i * 3 + 2]));

		for (int i = 0; i < rigging_dock_face_s; i += 3)
		{
			surfacees[0].indices.append(rigging_dock_face[i + 2]);
			surfacees[0].indices.append(rigging_dock_face[i + 1]);
			surfacees[0].indices.append(rigging_dock_face[i + 0]);
		}
	}
	else
	{
		obj_parser parser;
		print_line("step 2");
		parser.Load(obj, obj_path);
		print_line("step 3");
		int id = 0;

		for (auto& e : parser.LoadedMeshes)
		{
			surface_t surface;
			auto& v = e.v;
			auto& f = e.f;
			int s = v.size();

			for (int i = 0; i < s; ++i)
			{
				surface.vertices.push_back(Vector3
				(
					v[i].v[0],
					v[i].v[1],
					v[i].v[2]
				));
				if (e.mode & obj_parser::EMode_VT)
					surface.texcoord.push_back(Vector3
					(
						v[i].vt[0],
						1 - v[i].vt[1],
						0
					));

				if (e.mode & obj_parser::EMode_VN)
					surface.normals.push_back(Vector3
					(
						v[i].vn[0],
						v[i].vn[1],
						v[i].vn[2]
					));
			}

			s = f.size();

			print_line("step 4");

			for (int i = 0; i < s; i += 3)
			{
				surface.indices.push_back(f[i + 2]);
				surface.indices.push_back(f[i + 1]);
				surface.indices.push_back(f[i + 0]);
			}

			if (!e.MeshMaterialName.empty())
			{
				Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);

				material->set_name(e.MeshMaterialName.c_str());

				auto& Kd = e.MeshMaterial.Kd;
				auto& Ks = e.MeshMaterial.Ks;
				auto& Ns = e.MeshMaterial.Ns;
				auto& d = e.MeshMaterial.d;
				auto& Tr = e.MeshMaterial.Tr;

				auto a = d * 0.5 + (1 - Tr) * 0.5;




				std::string pathtomat = obj_path.substr(0, obj_path.find_last_of("/\\") + 1);

				print_line((pathtomat + e.MeshMaterial.map_Kd).c_str());
				print_line((pathtomat + e.MeshMaterial.map_Ns).c_str());
				if (!e.MeshMaterial.map_Kd.empty())
				{
					material->set_texture(BaseMaterial3D::TextureParam::TEXTURE_ALBEDO, ImageTexture::create_from_image(Image::load_from_file((pathtomat + e.MeshMaterial.map_Kd).c_str())));
				}
				else
					material->set_albedo(Color(Kd[0], Kd[1], Kd[2], a));

				if (!e.MeshMaterial.map_Ns.empty())
					material->set_texture(BaseMaterial3D::TextureParam::TEXTURE_METALLIC, ImageTexture::create_from_image(Image::load_from_file((pathtomat + e.MeshMaterial.map_Ns).c_str())));
				else
					material->set_metallic((1000.0 - Ns) / 1000.0);



				surface.material = material;
				print_line(e.name.c_str());
				print_line(e.MeshMaterial.to_string().c_str());
			}
			print_line("step 5");
			surfacees.push_back(surface);

		}
	}

	std::vector<Vector3> origins;

	Skeleton3D* skelet = memnew(Skeleton3D);

	const skvector_t* skvector_l_p = &g_skeleton;

	if (skvector != nullptr)
		skvector_l_p = skvector;

	const skvector_t& skvector_l = *skvector_l_p;

	for (int i = 0; i < skvector_l.size(); i++)
	{
		int id = skelet->get_bone_count();
		skelet->add_bone(skvector_l[i].name.c_str());
		if (skvector_l[i].parent >= 0)
			skelet->set_bone_parent(id, skvector_l[i].parent);
		auto& mat = skvector_l[i].mat;
		Transform3D trans = Transform3D(
			mat[0][0], mat[0][1], mat[0][2],
			mat[1][0], mat[1][1], mat[1][2],
			mat[2][0], mat[2][1], mat[2][2],
			mat[3][0], mat[3][1], mat[3][2]
		);

		skelet->set_bone_rest(id, trans);
		skelet->reset_bone_pose(id);


	}

	origins.resize(skelet->get_bone_count());

	auto trans = Transform3D(
		{ 0.00,  0.00,-1.00 },
		{ -1.00,  0.00, 0.00 },
		{ 0.00,  1.00, 0.00 },
		{ 0.00,  0.00, 0.00 }
	);

	skelet->set_bone_rest(0, trans);

	int s_s = skelet->get_bone_count();


	for (int j = 0; j < s_s; j++)
	{
		skelet->reset_bone_pose(j);
	}

	for (int __s = 0; __s < surfacees.size(); __s++)
	{
		auto& vertices = surfacees[__s].vertices;
		auto& normals = surfacees[__s].normals;
		auto& texcoord = surfacees[__s].texcoord;
		auto& indices = surfacees[__s].indices;

		Array arrays;
		arrays.resize(Mesh::ARRAY_MAX);


		arrays[Mesh::ARRAY_VERTEX] = vertices;
		if (normals.size() > 0)
		{
			arrays[Mesh::ARRAY_NORMAL] = normals;
		}
		if (texcoord.size() > 0)
		{
			arrays[Mesh::ARRAY_TEX_UV] = texcoord;
		}
		arrays[Mesh::ARRAY_INDEX] = indices;
		int s_id = mesh->get_surface_count();
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);


		mesh->surface_set_material(s_id, surfacees[__s].material);
	}

	rig(mesh, skelet);

	meshinst->set_mesh(mesh);
	skelet->set_name("Generated_Skeleton3D");
	meshinst->set_name("Generated_MeshInstance3D");

	return {mesh, meshinst, skelet};
}

void RiggingSystemDock::find_skeleton_and_print_path(Node* start_node, String state) {

    if (!start_node) {
        return;
    }

    node_path = start_node->get_path();
	//print_line(node_path);

    if (node_path.find("Generated_Skeleton3D") != -1) {
		path_node = start_node;
        set_target_skeleton(start_node, state);
    }

	for (int i = 0; i < start_node->get_child_count(); i++) {
        find_skeleton_and_print_path(start_node->get_child(i), state);
    }
}

void RiggingSystemDock::set_target_skeleton(Node* node, String state) {
    Skeleton3D* skeleton = Object::cast_to<Skeleton3D>(node);
    if (skeleton) {
		print_line("Skeleton pose was found!");
        target_skeleton = skeleton;
    } else {
		print_line("Skeleton pose was not found!");

        ERR_PRINT("Invalid path or not a Skeleton3D node");
    }
	save_current_pose(state);
}

void RiggingSystemDock::save_current_pose(String state) {
    if (!target_skeleton) {
        ERR_PRINT("Target skeleton not set or not found");
        return;
    }

	Vector<Vector3>* position;
	Vector<Quaternion>* rotation;
	if(state == "start"){
		position = &s_position;
		rotation = &s_rotation;
	} else {
		position = &e_position;
		rotation = &e_rotation;
	}

	position->resize(target_skeleton->get_bone_count());
	rotation->resize(target_skeleton->get_bone_count());

	for (int i = 0; i < target_skeleton->get_bone_count(); i++) {
		position->write[i] = target_skeleton->get_bone_pose_position(i) / target_skeleton->get_motion_scale();
		rotation->write[i] = target_skeleton->get_bone_pose_rotation(i);
	}


    print_line("Skeleton pose saved!");
}

void RiggingSystemDock::lerp_pose_with_animation_player() {
    ERR_FAIL_COND(!target_skeleton);  // Ensure target_skeleton is valid.

    // Ensure the poses match the number of bones.
    ERR_FAIL_COND(s_position.size() != target_skeleton->get_bone_count());
    ERR_FAIL_COND(e_position.size() != target_skeleton->get_bone_count());

	ERR_FAIL_COND(s_rotation.size() != target_skeleton->get_bone_count());
    ERR_FAIL_COND(e_rotation.size() != target_skeleton->get_bone_count());

    // Assuming anim_player is already created and added to the scene tree.
    ERR_FAIL_COND(!anim_player);  // Ensure anim_player is valid.
	ERR_FAIL_COND(!anim_lib);  // Ensure anim_lib is valid.

    // Create a new animation named "LerpPose".
    Ref<Animation> animation = memnew(Animation);
    animation->set_name("LerpPose");
    animation->set_length(5.0);  // Set the animation length to 5 seconds.


    // Loop through each bone and create tracks for them.
	String path = last_genned.meshinst->get_path_to(last_genned.skelet);

    for (int i = 0; i < target_skeleton->get_bone_count(); i++) {

		String bone_path = path + ":" + target_skeleton->get_bone_name(i);

        // Add a track for the bone's transform.
		int track_idx_r = animation->add_track(Animation::TYPE_ROTATION_3D);
		int track_idx_p = animation->add_track(Animation::TYPE_POSITION_3D);
        animation->track_set_path(track_idx_r, bone_path); 
		animation->track_set_path(track_idx_p, bone_path); 

        // Insert keyframes.
        animation->track_insert_key(track_idx_r, 0.0, s_rotation[i]);  // Start at t=0 seconds.
        animation->track_insert_key(track_idx_r, 5.0, e_rotation[i]);  // End at t=5 seconds.

		animation->track_insert_key(track_idx_p, 0.0, s_position[i]);   // Start at t=0 seconds.
        animation->track_insert_key(track_idx_p, 5.0, e_position[i]);   // End at t=5 seconds.
    }


    // Add the animation to the AnimationPlayer and play it.
    anim_lib->add_animation("LerpPose", animation);
	print_line(1);
	anim_player->add_animation_library("LerpLib", anim_lib);
	List<StringName> p_animations;
	anim_player->get_animation_list(&p_animations);
	for(int i = 0; i < p_animations.size(); i++)
	{
		print_line(p_animations[i]);
	}
	print_line(1);
	anim_lib->get_animation_list(&p_animations);
	for(int i = 0; i < p_animations.size(); i++)
	{
		print_line(p_animations[i]);
	}
	print_line(2);
	auto proces_func = [this]()
	{
		anim_player->play("LerpLib/LerpPose");
		anim_player->pause();
		camera->set_current(true);
		vwp->set_update_mode(SubViewport::UPDATE_ALWAYS);
		//RS::get_singleton()->viewport_set_active(vwp->get_viewport_rid(), true);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		for(int i = 0; i < 50; i++)
		{
			anim_player->seek(i * 0.1, true);
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			vwp->update_canvas_items();
			vwp->set_size(vwp->get_size());
			Ref<Image> img = vwp->get_texture()->get_image()->duplicate();
			img->flip_y();
			std::string pth = "";
			if(path_to_export.size() != 0)
				pth = path_to_export +  "/";
			img->save_png((std::string(pth) + "test_" + std::to_string(i) + ".png").c_str());

		}
		
	};
	std::thread ths(proces_func);
	ths.detach();
	print_line(3);
}


// Function to ensure AnimationPlayer exists and is set up correctly
void RiggingSystemDock::ensure_animation_player() {
	print_line("Failure #1!");
	int c_c = last_genned.meshinst->get_child_count();
	for(int i = 0; i < c_c; i++)
	{
		print_line(last_genned.meshinst->get_child(i));
	}

    anim_player = Object::cast_to<AnimationPlayer>(last_genned.meshinst->get_node(NodePath("AnimPlayer")));
    if (!anim_player) {
        // If AnimationPlayer doesn't exist, create it
        anim_player = memnew(AnimationPlayer);
        last_genned.meshinst->add_child(anim_player);
        anim_player->set_name("AnimPlayer");
    }
	print_line(anim_player);
	c_c = last_genned.meshinst->get_child_count();
	for(int i = 0; i < c_c; i++)
	{
		print_line(last_genned.meshinst->get_child(i));
	}
	SceneTreeDock::get_singleton()->get_tree_editor()->update_tree();
    if (!anim_lib) {
        anim_lib = memnew(AnimationLibrary);
    }

	lerp_pose_with_animation_player();
	// anim_player->set_owner(get_node(this.set_owner()));
}

void RiggingSystemDock::save_start_position() {
	print_line("print_all_node_paths!");

    Node* root = get_tree()->get_root();
    find_skeleton_and_print_path(root, "start");
}

void RiggingSystemDock::save_final_position() {
	print_line("print_all_node_paths!");

    Node* root = get_tree()->get_root();
    find_skeleton_and_print_path(root, "final");
}

void RiggingSystemDock::on_button_pressed()
{
	if(vwp == nullptr)
		vwp = memnew(SubViewport);


	if(last_genned.meshinst)
	{
		vwp->remove_child(last_genned.meshinst);
		last_genned.meshinst->remove_child(anim_player);
	}

	if(skeleton_json_str.size() != 0)
	{
		skvector_t skelet;

		JSON json;
		json.parse(skeleton_json_str.c_str());
		skeleton_json_str.clear();

		if(parse_json_skeleton(&skelet, json))
			last_genned = Gen(obj, &skelet);
		else
			last_genned = Gen(obj);
		obj.resize(0);
	}
	else
	{
		last_genned = Gen(obj);
		obj.resize(0);
	}
	
	if(camera == nullptr)
		camera = memnew(Camera3D);
		

	SceneTreeDock::get_singleton()->add_root_node(vwp);

	vwp->add_child(last_genned.meshinst);
	vwp->add_child(camera);
	camera->set_owner(vwp);
		
	last_genned.meshinst->add_child(last_genned.skelet);
	last_genned.meshinst->set_owner(vwp);
	last_genned.skelet->set_owner(vwp);

	if (!anim_player) {
        anim_player = memnew(AnimationPlayer);
    }
	
	last_genned.meshinst->add_child(anim_player);
	anim_player->set_owner(vwp);
	anim_player->set_name("Generated_AnimPlayer");

	if (!anim_lib) {
        anim_lib = memnew(AnimationLibrary);
    }

	anim_player->add_animation_library("LerpLib", anim_lib);
	vwp->set_name("SubViewport");
	camera->set_name("Camera");
	camera->set_position({ 0, 0, 3 });

	auto path = last_genned.skelet->get_path();
	print_line("skelet->get_path: " + path);
	path = last_genned.meshinst->get_path_to(last_genned.skelet);
	print_line("meshinst->get_path_to(skelet): " + path);

	last_genned.meshinst->set_skeleton_path(path);
	target_skeleton = last_genned.skelet;
}

void RiggingSystemDock::on_button_open_obj_pressed()
{
	Point2 pos = button_select_obj->get_position();
	Point2 wpos = get_position() + get_window()->get_position() + Point2(20, 20);
	Rect2i rect = Rect2i(pos + wpos, {800, 800});
	file_dialog_open_obj->popup(rect);
}

void RiggingSystemDock::on_button_open_skeleton_pressed()
{
	Point2 pos = button_select_skeleton->get_position();
	Point2 wpos = get_position() + get_window()->get_position() + Point2(20, 20);
	Rect2i rect = Rect2i(pos + wpos, {800, 800});
	file_dialog_open_skeleton->popup(rect);
}

void RiggingSystemDock::on_button_export_animation_pressed()
{
	Point2 pos = button_export_animation->get_position();
	Point2 wpos = get_position() + get_window()->get_position() + Point2(20, 20);
	Rect2i rect = Rect2i(pos + wpos, {800, 800});
	file_dialog_export_animation->popup(rect);
}

void RiggingSystemDock::on_button_export_skeleton_pressed()
{
	ERR_FAIL_COND(!(last_genned.skelet));
	int s_s = last_genned.skelet->get_bone_count();
	char buff[40960];

	std::string path = "__out.bone.json";
	auto l_path = line_edit_export_skeleton->get_text();
	if (!l_path.is_empty())
		path = l_path.ascii();

	std::ofstream fout(path);
	fout << "[\n";

	for (int j = 0; j < s_s; j++)
	{
		const auto& or = last_genned.skelet->get_bone_pose(j).origin;
		const auto& b = last_genned.skelet->get_bone_pose(j).basis;

		for (int i = 0; i < 40960; i++)
			buff[i] = '\0';

		snprintf(buff, 40960, R"(  {
    "name": "%s",
    "parent": %i,
    "transform_mat": 
    [
      [
        %f,
        %f,
        %f,
        %f
      ],
      [
        %f,
        %f,
        %f,
        %f
      ],
      [
        %f,
        %f,
        %f,
        %f
      ],
      [
        %f,
        %f,
        %f,
        %f
      ]
    ]

  })",
			last_genned.skelet->get_bone_name(j).ascii().operator const char* (),
			last_genned.skelet->get_bone_parent(j),
			b[0][0], b[0][1], b[0][2], 0.0,
			b[1][0], b[1][1], b[1][2], 0.0,
			b[2][0], b[2][1], b[2][2], 0.0,
			or [0], or [1], or [2], 1.0
		);
		fout << buff;
		if (j < s_s - 1)
			fout << ",\n";
		else
			fout << "\n";
	}

	fout << "]\n";

	fout.close();

	fout = std::ofstream("g_skeleton.h");
	fout <<
		R"(#ifndef __G_SKELETON_H__
#define __G_SKELETON_H__

#include "rigging_dock.h"

const RiggingSystemDock::skvector_t g_skeleton = {
)";

	for (int j = 0; j < s_s; j++)
	{
		const auto& or = last_genned.skelet->get_bone_pose(j).origin;
		const auto& b = last_genned.skelet->get_bone_pose(j).basis;

		for (int i = 0; i < 40960; i++)
			buff[i] = '\0';

		snprintf(buff, 40960, R"(  {
    {
      {%f, %f, %f %f},
      {%f, %f, %f %f},
      {%f, %f, %f %f},
      {%f, %f, %f %f}
    },
	"%s",
    %i
  })",
			b[0][0], b[0][1], b[0][2], 0.0,
			b[1][0], b[1][1], b[1][2], 0.0,
			b[2][0], b[2][1], b[2][2], 0.0,
			or [0], or [1], or [2], 1.0,
			last_genned.skelet->get_bone_name(j).ascii().operator const char* (),
			last_genned.skelet->get_bone_parent(j)
		);
		fout << buff;
		if (j < s_s - 1)
			fout << ",\n";
		else
			fout << "\n";
	}

	fout <<
		R"(};

#endif // __G_SKELETON_H__)";
}

void RiggingSystemDock::file_dialog_open_obj_file_selected(String path)
{
	line_edit_select_obj->set_text(path);
	obj_path = path.ascii().get_data();
	std::ifstream fin(obj_path);
	fin.seekg(0, std::ios::end);
	size_t size = fin.tellg();
	obj.resize(size, ' ');
	fin.seekg(0);
	fin.read(obj.data(), size);
}

void RiggingSystemDock::file_dialog_open_skeleton_file_selected(String path)
{
	line_edit_select_skeleton->set_text(path);
	std::ifstream fin(path.ascii().get_data());
	fin.seekg(0, std::ios::end);
	size_t size = fin.tellg();
	skeleton_json_str.resize(size, ' ');
	fin.seekg(0);
	fin.read(skeleton_json_str.data(), size);
}

void RiggingSystemDock::file_dialog_export_animation_file_selected(String path)
{
	line_edit_export_animation->set_text(path);
	path_to_export = path.ascii().get_data();
}



RiggingSystemDock::RiggingSystemDock()
{
#define TEST
	TEST;
	singleton = this;
	set_name("RiggingSystem");
	set_size({ 500,500 });

	file_dialog_open_obj = memnew(FileDialog);
	file_dialog_open_skeleton = memnew(FileDialog);
	file_dialog_export_animation = memnew(FileDialog);

	TEST;
	v_box_container = memnew(VBoxContainer);
	h_box_container1 = memnew(HBoxContainer);
	button_select_obj = memnew(Button);
	line_edit_select_obj = memnew(LineEdit);
	h_box_container2 = memnew(HBoxContainer);
	button_select_skeleton = memnew(Button);
	line_edit_select_skeleton = memnew(LineEdit);
	h_box_container3 = memnew(HBoxContainer);
	button_select_start_state = memnew(Button);
	line_edit_select_start_state = memnew(LineEdit);
	h_box_container4 = memnew(HBoxContainer);
	button_select_final_state = memnew(Button);
	line_edit_select_final_state = memnew(LineEdit);
	h_box_container5 = memnew(HBoxContainer);
	button_export_skeleton = memnew(Button);
	line_edit_export_skeleton = memnew(LineEdit);
	h_box_container6 = memnew(HBoxContainer);
	button_export_animation = memnew(Button);
	line_edit_export_animation = memnew(LineEdit);
	h_box_container7 = memnew(HBoxContainer);
	button_generate = memnew(Button);
	button_generate_animation = memnew(Button);
	TEST;

	TEST;//================== this ==================
	this->set_name("Control");
	this->set_anchors_preset(Control::PRESET_FULL_RECT);
	this->set_anchor(SIDE_RIGHT, 1.0);
	this->set_anchor(SIDE_BOTTOM, 1.0);
	this->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	this->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
	TEST;//================== v_box_container ==================
	v_box_container->set_name("VBoxContainer");
	this->add_child(v_box_container);
	v_box_container->set_anchors_preset(Control::PRESET_FULL_RECT);
	v_box_container->set_anchor(SIDE_RIGHT, 1.0);
	v_box_container->set_anchor(SIDE_BOTTOM, 1.0);
	v_box_container->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	v_box_container->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
	TEST;//================== h_box_container1 ==================
	h_box_container1->set_name("HBoxContainer1");
	v_box_container->add_child(h_box_container1);
	TEST;//================== button_select_obj ==================
	button_select_obj->set_name("ButtonSelectObj");
	h_box_container1->add_child(button_select_obj);
	button_select_obj->set_custom_minimum_size(Vector2(200, 0));
	button_select_obj->set_text("Select Object file");
	TEST;//================== line_edit_select_obj ==================
	line_edit_select_obj->set_name("LineEditSelectObj");
	h_box_container1->add_child(line_edit_select_obj);
	line_edit_select_obj->set_custom_minimum_size(Vector2(200, 0));
	line_edit_select_obj->set_editable(false);
	TEST;//================== h_box_container2 ==================
	h_box_container2->set_name("HBoxContainer2");
	v_box_container->add_child(h_box_container2);
	TEST;//================== button_select_skeleton ==================
	button_select_skeleton->set_name("ButtonSelecSkeleton");
	h_box_container2->add_child(button_select_skeleton);
	button_select_skeleton->set_custom_minimum_size(Vector2(200, 0));
	button_select_skeleton->set_text("Select Skeleton file");
	TEST;//================== line_edit_select_skeleton ==================
	line_edit_select_skeleton->set_name("LineEditSelectSkeleton");
	h_box_container2->add_child(line_edit_select_skeleton);
	line_edit_select_skeleton->set_custom_minimum_size(Vector2(200, 0));
	line_edit_select_skeleton->set_editable(false);
	TEST;//================== h_box_container3 ==================
	h_box_container3->set_name("HBoxContainer3");
	v_box_container->add_child(h_box_container3);
	TEST;//================== button_select_start_state ==================
	button_select_start_state->set_name("ButtonSelectStartState");
	h_box_container3->add_child(button_select_start_state);
	button_select_start_state->set_custom_minimum_size(Vector2(200, 0));
	button_select_start_state->set_text("Select Statr State");
	TEST;//================== line_edit_select_start_state ==================
	line_edit_select_start_state->set_name("LineEditSelectStartState");
	h_box_container3->add_child(line_edit_select_start_state);
	line_edit_select_start_state->set_custom_minimum_size(Vector2(200, 0));
	line_edit_select_start_state->set_editable(false);
	TEST;//================== h_box_container4 ==================
	h_box_container4->set_name("HBoxContainer4");
	v_box_container->add_child(h_box_container4);
	TEST;//================== button_select_final_state ==================
	button_select_final_state->set_name("ButtonSelectFinalState");
	h_box_container4->add_child(button_select_final_state);
	button_select_final_state->set_custom_minimum_size(Vector2(200, 0));
	button_select_final_state->set_text("Select Final State");
	TEST;//================== line_edit_select_final_state ==================
	line_edit_select_final_state->set_name("LineEditSelectFinalState");
	h_box_container4->add_child(line_edit_select_final_state);
	line_edit_select_final_state->set_custom_minimum_size(Vector2(200, 0));
	line_edit_select_final_state->set_editable(false);
	TEST;//================== h_box_container5 ==================
	h_box_container5->set_name("HBoxContainer5");
	v_box_container->add_child(h_box_container5);
	TEST;//================== button_export_skeleton ==================
	button_export_skeleton->set_name("ButtonExportSkeleton");
	h_box_container5->add_child(button_export_skeleton);
	button_export_skeleton->set_custom_minimum_size(Vector2(200, 0));
	button_export_skeleton->set_text("Export Skeleton");
	TEST;//================== line_edit_export_skeleton ==================
	line_edit_export_skeleton->set_name("LineEditExportSkeleton");
	h_box_container5->add_child(line_edit_export_skeleton);
	line_edit_export_skeleton->set_custom_minimum_size(Vector2(200, 0));
	line_edit_export_skeleton->set_editable(true);
	TEST;//================== h_box_container6 ==================
	h_box_container6->set_name("HBoxContainer6");
	v_box_container->add_child(h_box_container6);
	TEST;//================== button_export_animation ==================
	button_export_animation->set_name("ButtonExportSkeleton");
	h_box_container6->add_child(button_export_animation);
	button_export_animation->set_custom_minimum_size(Vector2(200, 0));
	button_export_animation->set_text("Export Animation");
	TEST;//================== line_edit_export_animation ==================
	line_edit_export_animation->set_name("LineEditExportSkeleton");
	h_box_container6->add_child(line_edit_export_animation);
	line_edit_export_animation->set_custom_minimum_size(Vector2(200, 0));
	line_edit_export_animation->set_editable(true);
	TEST;//================== h_box_container7 ==================
	h_box_container6->set_name("HBoxContainer7");
	v_box_container->add_child(h_box_container7);
	TEST;//================== button_generate ==================
	button_generate->set_name("ButtonGenerate");
	h_box_container7->add_child(button_generate);
	button_generate->set_custom_minimum_size(Vector2(200, 0));
	button_generate->set_text("Generate");
	TEST;//================== button_generate_animation ==================
	button_generate_animation->set_name("ButtonGenerateAnimation");
	h_box_container7->add_child(button_generate_animation);
	button_generate_animation->set_custom_minimum_size(Vector2(200, 0));
	button_generate_animation->set_text("Generate Animation");


	TEST;//================== file_dialog_open_obj ==================
	file_dialog_open_obj->set_name("FileDialogOpenObj");
	this->add_child(file_dialog_open_obj);
	file_dialog_open_obj->set_title("Open a object(*.obj) File");
	file_dialog_open_obj->set_size(Vector2i(327, 300));
	file_dialog_open_obj->set_ok_button_text("Open");
	file_dialog_open_obj->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	file_dialog_open_obj->set_access(FileDialog::Access::ACCESS_FILESYSTEM);
	file_dialog_open_obj->set_filters({ "*.obj" });
	TEST;//================== file_dialog_open_skeleton ==================
	file_dialog_open_skeleton->set_name("FileDialogOpenSkeleton");
	this->add_child(file_dialog_open_skeleton);
	file_dialog_open_skeleton->set_title("Open a skelet(*.json) File");
	file_dialog_open_skeleton->set_size(Vector2i(327, 300));
	file_dialog_open_skeleton->set_ok_button_text("Open");
	file_dialog_open_skeleton->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	file_dialog_open_skeleton->set_access(FileDialog::Access::ACCESS_FILESYSTEM);
	file_dialog_open_skeleton->set_filters({ "*.json" });
	TEST;//================== file_dialog_export_animation ==================
	file_dialog_export_animation->set_name("FileDialogExportAnimation");
	this->add_child(file_dialog_export_animation);
	file_dialog_export_animation->set_title("Select folder for export animation");
	file_dialog_export_animation->set_size(Vector2i(327, 300));
	file_dialog_export_animation->set_ok_button_text("Select");
	file_dialog_export_animation->set_file_mode(FileDialog::FILE_MODE_OPEN_DIR);
	file_dialog_export_animation->set_access(FileDialog::Access::ACCESS_FILESYSTEM);
	TEST;

	button_generate->connect("pressed", callable_mp(this, &RiggingSystemDock::on_button_pressed));
	button_select_obj->connect("pressed", callable_mp(this, &RiggingSystemDock::on_button_open_obj_pressed));
	button_select_skeleton->connect("pressed", callable_mp(this, &RiggingSystemDock::on_button_open_skeleton_pressed));
	file_dialog_open_obj->connect("file_selected", callable_mp(this, &RiggingSystemDock::file_dialog_open_obj_file_selected));
	file_dialog_open_skeleton->connect("file_selected", callable_mp(this, &RiggingSystemDock::file_dialog_open_skeleton_file_selected));
	file_dialog_export_animation->connect("dir_selected", callable_mp(this, &RiggingSystemDock::file_dialog_export_animation_file_selected));
	button_select_start_state->connect("pressed", callable_mp(this, &RiggingSystemDock::save_start_position));
	button_select_final_state->connect("pressed", callable_mp(this, &RiggingSystemDock::save_final_position));
	button_generate_animation->connect("pressed", callable_mp(this, &RiggingSystemDock::ensure_animation_player));
	// button_rig->connect("pressed", callable_mp(this, &RiggingSystemDock::on_button_rig_pressed));
	// button_unrig->connect("pressed", callable_mp(this, &RiggingSystemDock::on_button_unrig_pressed));
	button_export_skeleton->connect("pressed", callable_mp(this, &RiggingSystemDock::on_button_export_skeleton_pressed));
	button_export_animation->connect("pressed", callable_mp(this, &RiggingSystemDock::on_button_export_animation_pressed));
}

RiggingSystemDock::~RiggingSystemDock()
{
	singleton = nullptr;
}
