#ifndef MULTI_TEXTURE_RECT_H
#define MULTI_TEXTURE_RECT_H

#include "scene/gui/control.h"
#include "core/io/dir_access.h"

class MultiTextureRect : public Control {
	GDCLASS(MultiTextureRect, Control);

public:
	enum ExpandMode {
		EXPAND_KEEP_SIZE,
		EXPAND_IGNORE_SIZE,
		EXPAND_FIT_WIDTH,
		EXPAND_FIT_WIDTH_PROPORTIONAL,
		EXPAND_FIT_HEIGHT,
		EXPAND_FIT_HEIGHT_PROPORTIONAL,
	};

	enum StretchMode {
		STRETCH_SCALE,
		STRETCH_TILE,
		STRETCH_KEEP,
		STRETCH_KEEP_CENTERED,
		STRETCH_KEEP_ASPECT,
		STRETCH_KEEP_ASPECT_CENTERED,
		STRETCH_KEEP_ASPECT_COVERED,
	};

private:
	bool hflip = false;
	bool vflip = false;
	Ref<Texture2D> texture;
	List<Ref<Texture2D>> textures;
	ExpandMode expand_mode = EXPAND_KEEP_SIZE;
	StretchMode stretch_mode = STRETCH_SCALE;
	/* Added for showing images { */
	bool draw_bg = false;
	/* } */
	bool draw_multi_texture = false;
	String texture_path;

	void _texture_changed();

protected:
	void _notification(int p_what);
	virtual Size2 get_minimum_size() const override;
	static void _bind_methods();
#ifndef DISABLE_DEPRECATED
	bool _set(const StringName &p_name, const Variant &p_value);
#endif

public:
	void set_texture_path(const String &path);

	void set_texture(const Ref<Texture2D> &p_tex);
	Ref<Texture2D> get_texture() const;

	void set_expand_mode(ExpandMode p_mode);
	ExpandMode get_expand_mode() const;

	void set_stretch_mode(StretchMode p_mode);
	StretchMode get_stretch_mode() const;

	void set_flip_h(bool p_flip);
	bool is_flipped_h() const;

	void set_flip_v(bool p_flip);
	bool is_flipped_v() const;

/* Added for showing images { */
	void set_draw_bg(bool p_bg);
	bool is_draw_bg() const;
/* } */

	MultiTextureRect();
	~MultiTextureRect();
};

VARIANT_ENUM_CAST(MultiTextureRect::ExpandMode);
VARIANT_ENUM_CAST(MultiTextureRect::StretchMode);

#endif // MULTI_TEXTURE_RECT_H
