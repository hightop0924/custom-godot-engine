#ifndef IMAGE_COMPOSER_H
#define IMAGE_COMPOSER_H

#include "editor/editor_file_dialog.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/button.h"
#include "scene/gui/multi_texture_rect.h"

class ImageComposer : public ConfirmationDialog {
	GDCLASS(ImageComposer, ConfirmationDialog)

	MultiTextureRect *image_panel;

	static ImageComposer *singleton;

protected:
	void _notification(int p_what);

public:
	static ImageComposer *get_singleton() { return singleton; }

	void show_dialog(const String &path, bool p_exclusive = false);

	ImageComposer();
};

#endif // IMAGE_COMPOSER_H
