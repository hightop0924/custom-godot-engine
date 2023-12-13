#include "image_composer_dock.h"

#include "core/config/project_settings.h"
#include "editor/editor_node.h"
#include "editor/editor_resource_preview.h"
#include "editor/editor_scale.h"
//#include "scene/gui/tree.h"
#include "editor/editor_settings.h"

ImageComposerDock *ImageComposerDock::singleton = nullptr;

#include <iostream>

ImageComposerDock::ImageComposerDock() {
	singleton = this;
	set_name("ImageComposer");

	set_size({500,500});
	
	scroll_container = memnew(ScrollContainer);
	vb = memnew(VBoxContainer);
	
//================== this ==================
	this->set_anchors_preset(Control::PRESET_FULL_RECT);
	this->set_anchor(SIDE_RIGHT, 1.0);
	this->set_anchor(SIDE_BOTTOM, 1.0);
	this->set_offset(SIDE_RIGHT, -787.0);
	this->set_offset(SIDE_BOTTOM, -2.0);
	this->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	this->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
//================== scroll_container ==================
	scroll_container->set_name("ScrollContainer");
	this->add_child(scroll_container);
	scroll_container->set_layout_direction(Control::LAYOUT_DIRECTION_LOCALE);
	scroll_container->set_anchors_preset(Control::PRESET_FULL_RECT);
	scroll_container->set_anchor(SIDE_RIGHT, 1.0);
	scroll_container->set_anchor(SIDE_BOTTOM, 1.0);
	scroll_container->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	scroll_container->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
	scroll_container->set_scale(Vector2(1, 0.983573));

//================== split ==================
	vb->set_name("Split");
	scroll_container->add_child(vb);
	vb->set_h_size_flags(3);
	vb->set_v_size_flags(3);

	HBoxContainer *hb = memnew(HBoxContainer);

	image_path = memnew(LineEdit);
	image_path->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	image_path->set_editable(false);
	hb->add_child(image_path);
	path_browse = memnew(Button);
	hb->add_child(path_browse);
	path_browse->set_text(TTR("Browse"));
	path_browse->connect("pressed", callable_mp(this, &ImageComposerDock::_browse_install));
	hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hb->set_custom_minimum_size(Size2(200 * EDSCALE, 0));

	vb->add_child(hb);

	add_child(vb);

	browse_dialog = memnew(EditorFileDialog);
	browse_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_DIR);
	browse_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);

	browse_dialog->connect("dir_selected", callable_mp(this, &ImageComposerDock::_select_dir));

	add_child(browse_dialog);
}

ImageComposerDock::~ImageComposerDock() {
	singleton = nullptr;
}

void ImageComposerDock::_browse_install() {
	if (image_path->get_text() != String()) {
		browse_dialog->set_current_dir(image_path->get_text());
	}
	browse_dialog->popup_file_dialog();
}

void ImageComposerDock::_select_dir(const String &p_path) {
	if(p_path.length() > 0)
	{
		image_path->set_text(p_path);
		EditorNode::get_singleton()->show_image_composer(p_path);
	}
}