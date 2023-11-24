#ifndef ADD_RIGGING_H
#define ADD_RIGGING_H


#include "editor/editor_file_system.h"
#include "editor/editor_inspector.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/gui/button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/file_dialog.h"
#include "scene/animation/animation_player.h"
#include "core/io/json.h"
#include <string>
#include <vector>

#include "player_handler.h"


class TreeItem;


class RiggingSystemDock : public Control {
	GDCLASS(RiggingSystemDock, Control);
public:

	struct RiggingSystemRet
	{
		ArrayMesh* mesh = nullptr;
		MeshInstance3D* meshinst = nullptr;
		Skeleton3D* skelet = nullptr;
	};
	
	struct skeleton_bone_t
	{
		float mat[4][4];
		std::string name;
		int parent;
	};

	typedef std::vector<RiggingSystemDock::skeleton_bone_t> skvector_t;

private:

	static RiggingSystemDock *singleton;
	
	VBoxContainer* v_box_container = nullptr;
	HBoxContainer* h_box_container1 = nullptr;
	Button* button_select_obj = nullptr;
	LineEdit* line_edit_select_obj = nullptr;
	
	HBoxContainer* h_box_container2 = nullptr;
	Button* button_select_skeleton = nullptr;
	LineEdit* line_edit_select_skeleton = nullptr;

	HBoxContainer* h_box_container3 = nullptr;
	Button* button_select_start_state = nullptr;
	LineEdit* line_edit_select_start_state = nullptr;

	HBoxContainer* h_box_container4 = nullptr;
	Button* button_select_final_state = nullptr;
	LineEdit* line_edit_select_final_state = nullptr;

	HBoxContainer* h_box_container5 = nullptr;
	Button* button_export_skeleton = nullptr;
	LineEdit* line_edit_export_skeleton = nullptr;

	HBoxContainer* h_box_container6 = nullptr;
	Button* button_export_animation = nullptr;
	LineEdit* line_edit_export_animation = nullptr;

	HBoxContainer* h_box_container7 = nullptr;
	Button* button_generate = nullptr;
	Button* button_generate_animation = nullptr;


	String node_path = "";
	Node* path_node = nullptr;

	FileDialog *file_dialog_open_obj = nullptr;
	FileDialog *file_dialog_open_skeleton = nullptr;
	FileDialog *file_dialog_export_animation = nullptr;
	SubViewport* vwp = nullptr;
	AnimationPlayer* anim_player = nullptr;
	AnimationLibrary* anim_lib = nullptr;
	Camera3D* camera = nullptr;

	std::string obj = "";
	std::string skeleton_json_str = "";
	std::string obj_path = "";
	std::string path_to_export = "";

	RiggingSystemRet last_genned;

	void on_button_pressed();
	void on_button_open_obj_pressed();
	void on_button_open_skeleton_pressed();
	void file_dialog_open_obj_file_selected(String path);
	void file_dialog_open_skeleton_file_selected(String path);
	void file_dialog_export_animation_file_selected(String path);
	void find_skeleton_and_print_path(Node* start_node, String state);
	void save_start_position();
	void save_final_position();
	void interpolate_animation();
	void lerp_pose_with_animation_player();
	void on_button_export_skeleton_pressed();
	void on_button_export_animation_pressed();

protected:
    Vector<Vector3> s_position;
	Vector<Vector3> e_position;
	Vector<Quaternion> s_rotation;
	Vector<Quaternion> e_rotation;
	Skeleton3D *target_skeleton;

public:
    void capture_start_pose();
	void capture_final_pose();
	void ensure_animation_player();
	void set_target_skeleton(Node* node, String state);
	void save_current_pose(String state);

	static RiggingSystemDock *get_singleton() { return singleton; }

	RiggingSystemRet Gen(std::string obj = "", skvector_t* = nullptr);

	RiggingSystemDock();
	~RiggingSystemDock();
};

#endif // ADD_CUBEMAP_DOCK_H
