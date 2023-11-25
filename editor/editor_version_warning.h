#ifndef EDITOR_VERSION_WARNING_H
#define EDITOR_VERSION_WARNING_H

#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"
#include "scene/gui/link_button.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"

#include "editor/editor_scale.h"

/**
 * NOTE: Do not assume the EditorNode singleton to be available in this class' methods.
 * EditorVersionWarning is also used from the project manager where EditorNode isn't initialized.
 */
class EditorVersionWarning : public AcceptDialog {
	GDCLASS(EditorVersionWarning, AcceptDialog);

private:
	Label *version_text;
	TextureRect *_logo = nullptr;

	void _theme_changed();

public:
	TextureRect *get_logo() const;
	void set_new_version(String v);
	EditorVersionWarning();
	~EditorVersionWarning();
};
#endif // EDITOR_VERSION_WARNING_H
