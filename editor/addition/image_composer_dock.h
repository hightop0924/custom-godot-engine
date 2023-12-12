#ifndef ADD_IMAGE_COMPOSER_DOCK_H
#define ADD_IMAGE_COMPOSER_DOCK_H

#include "core/io/config_file.h"
#include "core/io/resource_importer.h"
#include "editor/editor_file_system.h"
#include "editor/editor_inspector.h"
#include "scene/gui/box_container.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/popup_menu.h"
//#include "scene/gui/tree.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/file_dialog.h"
/* Added for showing images { */
#include "scene/gui/texture_rect.h"
/* } */

#include "editor/editor_file_dialog.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/line_edit.h"

//class TreeItem;

class ImageComposerDock : public Control {
	GDCLASS(ImageComposerDock, Control);

	void _select_dir(const String &p_path);
	void _browse_install();

private:
	static ImageComposerDock *singleton;

public:
	static ImageComposerDock *get_singleton() { return singleton; }

protected:
	ScrollContainer *scroll_container = nullptr;
	VBoxContainer* vb = nullptr;

	LineEdit *image_path = nullptr;
	Button *path_browse = nullptr;

	EditorFileDialog *browse_dialog = nullptr;

public:
	ImageComposerDock();
	~ImageComposerDock();
};

#endif // ADD_IMAGE_COMPOSER_DOCK_H
