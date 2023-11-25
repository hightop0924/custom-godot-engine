#include "editor_version_warning.h"

#include "core/authors.gen.h"
#include "core/donors.gen.h"
#include "core/license.gen.h"
#include "core/version.h"

// The metadata key used to store and retrieve the version text to copy to the clipboard.
static const String META_TEXT_TO_COPY = "text_to_copy";

void EditorVersionWarning::_theme_changed() {
	_logo->set_texture(get_theme_icon(SNAME("Logo"), SNAME("EditorIcons")));
}

TextureRect *EditorVersionWarning::get_logo() const {
	return _logo;
}

EditorVersionWarning::EditorVersionWarning() {
	set_title(TTR("New Godot version is detected!"));
	set_hide_on_ok(true);
	set_min_size(Size2(360, 180) * EDSCALE);
	set_max_size(Size2(360, 180) * EDSCALE);

	VBoxContainer *vbc = memnew(VBoxContainer);
	add_child(vbc);

	_logo = memnew(TextureRect);
	_logo->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	vbc->add_child(_logo);

	VBoxContainer *version_info_vbc = memnew(VBoxContainer);

	// Add a dummy control node for spacing.
	Control *v_spacer = memnew(Control);
	version_info_vbc->add_child(v_spacer);

	String hash = String(VERSION_HASH);
	if (hash.length() != 0) {
		hash = " " + vformat("[%s]", hash.left(9));
	}

	version_text = memnew(Label);
	version_text->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	version_text->set_text(TTR("         Current: ") + VERSION_FULL_BUILD + hash  + "\n" + TTR("New Version: "));
	version_info_vbc->add_child(version_text);

	vbc->add_child(version_info_vbc);
	
	vbc->connect("theme_changed", callable_mp(this, &EditorVersionWarning::_theme_changed));
}

void EditorVersionWarning::set_new_version(String v){
	String hash = String(VERSION_HASH);
	if (hash.length() != 0) {
		hash = " " + vformat("[%s]", hash.left(9));
	}
	version_text->set_text(TTR("         Current: ") + VERSION_FULL_BUILD + hash  + "\n" + TTR("New Version: ") + v);
}

EditorVersionWarning::~EditorVersionWarning() {}