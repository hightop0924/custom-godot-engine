#include "multi_texture_rect.h"

#include "core/core_string_names.h"
#include "servers/rendering_server.h"
#include "editor/plugins/node_3d_editor_plugin.h"

void MultiTextureRect::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			/* Added for showing images { */
			if(draw_bg)
			{
				Size2 size = get_size();
				draw_rect(Rect2(Point2(0, 0), size), Color(1,1,1));
			}
			/* } */

			if(draw_multi_texture)
			{
				Size2 size = get_size();
				Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
				PackedStringArray file_list = dir->get_files_at(texture_path);
				if(file_list.size() > 0)
					textures.clear();
				for (const String &file_name : file_list) {
					if(file_name.ends_with(".png"))
					{
						String img_path = texture_path + "/" + file_name;
						Ref<Image> img = Image::load_from_file(img_path);
						if(!img->is_empty())
						{
							Size2 img_size = img->get_size();
							Point2 offset;
							if(size.width == 0)
								size.width = Node3DEditor::get_singleton()->get_size().width;
							if(size.height == 0)
								size.height = Node3DEditor::get_singleton()->get_size().height - 30;
							offset.x = abs(rand() % (int)(size.width));
							offset.y = abs(rand() % (int)(size.height));
							if(offset.x + img_size.width > size.width)
								offset.x = size.width - img_size.width;
							if(offset.y + img_size.height > size.height)
								offset.y = size.height - img_size.height;
							Ref<ImageTexture> t = memnew(ImageTexture);
							t->set_image(img);
							draw_texture(t, offset);
							textures.push_back(t);
						}
					}
				}
				break;
			}

			if (texture.is_null()) {
				return;
			}

			Size2 size;
			Point2 offset;
			Rect2 region;
			bool tile = false;

			switch (stretch_mode) {
				case STRETCH_SCALE: {
					size = get_size();
				} break;
				case STRETCH_TILE: {
					size = get_size();
					tile = true;
				} break;
				case STRETCH_KEEP: {
					size = texture->get_size();
				} break;
				case STRETCH_KEEP_CENTERED: {
					offset = (get_size() - texture->get_size()) / 2;
					size = texture->get_size();
				} break;
				case STRETCH_KEEP_ASPECT_CENTERED:
				case STRETCH_KEEP_ASPECT: {
					size = get_size();
					int tex_width = texture->get_width() * size.height / texture->get_height();
					int tex_height = size.height;

					if (tex_width > size.width) {
						tex_width = size.width;
						tex_height = texture->get_height() * tex_width / texture->get_width();
					}

					if (stretch_mode == STRETCH_KEEP_ASPECT_CENTERED) {
						offset.x += (size.width - tex_width) / 2;
						offset.y += (size.height - tex_height) / 2;
					}

					size.width = tex_width;
					size.height = tex_height;
				} break;
				case STRETCH_KEEP_ASPECT_COVERED: {
					size = get_size();

					Size2 tex_size = texture->get_size();
					Size2 scale_size(size.width / tex_size.width, size.height / tex_size.height);
					float scale = scale_size.width > scale_size.height ? scale_size.width : scale_size.height;
					Size2 scaled_tex_size = tex_size * scale;

					region.position = ((scaled_tex_size - size) / scale).abs() / 2.0f;
					region.size = size / scale;
				} break;
			}

			Ref<AtlasTexture> p_atlas = texture;

			if (p_atlas.is_valid() && !region.has_area()) {
				Size2 scale_size(size.width / texture->get_width(), size.height / texture->get_height());

				offset.width += hflip ? p_atlas->get_margin().get_position().width * scale_size.width * 2 : 0;
				offset.height += vflip ? p_atlas->get_margin().get_position().height * scale_size.height * 2 : 0;
			}

			size.width *= hflip ? -1.0f : 1.0f;
			size.height *= vflip ? -1.0f : 1.0f;

			if (region.has_area()) {
				draw_texture_rect_region(texture, Rect2(offset, size), region);
			} else {
				draw_texture_rect(texture, Rect2(offset, size), tile);
			}

			/* Added for showing images { */
			if(draw_bg)
			{
				Size2 size = get_size();
				draw_circle(Point2(size.width-8, 8), 5, Color(0.183,0.824,0.386));
				draw_line(Point2(size.width-11, 8), Point2(size.width-8, 11), Color(1,1,1));
				draw_line(Point2(size.width-8, 11), Point2(size.width-5, 5), Color(1,1,1));
			}
			/* } */
		} break;
		case NOTIFICATION_RESIZED: {
			update_minimum_size();
		} break;
	}
}

Size2 MultiTextureRect::get_minimum_size() const {
	if (!texture.is_null()) {
		switch (expand_mode) {
			case EXPAND_KEEP_SIZE: {
				return texture->get_size();
			} break;
			case EXPAND_IGNORE_SIZE: {
				return Size2();
			} break;
			case EXPAND_FIT_WIDTH: {
				return Size2(get_size().y, 0);
			} break;
			case EXPAND_FIT_WIDTH_PROPORTIONAL: {
				real_t ratio = real_t(texture->get_width()) / texture->get_height();
				return Size2(get_size().y * ratio, 0);
			} break;
			case EXPAND_FIT_HEIGHT: {
				return Size2(0, get_size().x);
			} break;
			case EXPAND_FIT_HEIGHT_PROPORTIONAL: {
				real_t ratio = real_t(texture->get_height()) / texture->get_width();
				return Size2(0, get_size().x * ratio);
			} break;
		}
	}
	return Size2();
}

void MultiTextureRect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_texture", "texture"), &MultiTextureRect::set_texture);
	ClassDB::bind_method(D_METHOD("get_texture"), &MultiTextureRect::get_texture);
	ClassDB::bind_method(D_METHOD("set_expand_mode", "expand_mode"), &MultiTextureRect::set_expand_mode);
	ClassDB::bind_method(D_METHOD("get_expand_mode"), &MultiTextureRect::get_expand_mode);
	ClassDB::bind_method(D_METHOD("set_flip_h", "enable"), &MultiTextureRect::set_flip_h);
	ClassDB::bind_method(D_METHOD("is_flipped_h"), &MultiTextureRect::is_flipped_h);
	ClassDB::bind_method(D_METHOD("set_flip_v", "enable"), &MultiTextureRect::set_flip_v);
	ClassDB::bind_method(D_METHOD("is_flipped_v"), &MultiTextureRect::is_flipped_v);
	ClassDB::bind_method(D_METHOD("set_stretch_mode", "stretch_mode"), &MultiTextureRect::set_stretch_mode);
	ClassDB::bind_method(D_METHOD("get_stretch_mode"), &MultiTextureRect::get_stretch_mode);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture", "get_texture");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "expand_mode", PROPERTY_HINT_ENUM, "Keep Size,Ignore Size,Fit Width,Fit Width Proportional,Fit Height,Fit Height Proportional"), "set_expand_mode", "get_expand_mode");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "stretch_mode", PROPERTY_HINT_ENUM, "Scale,Tile,Keep,Keep Centered,Keep Aspect,Keep Aspect Centered,Keep Aspect Covered"), "set_stretch_mode", "get_stretch_mode");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_h"), "set_flip_h", "is_flipped_h");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_v"), "set_flip_v", "is_flipped_v");

	BIND_ENUM_CONSTANT(EXPAND_KEEP_SIZE);
	BIND_ENUM_CONSTANT(EXPAND_IGNORE_SIZE);
	BIND_ENUM_CONSTANT(EXPAND_FIT_WIDTH);
	BIND_ENUM_CONSTANT(EXPAND_FIT_WIDTH_PROPORTIONAL);
	BIND_ENUM_CONSTANT(EXPAND_FIT_HEIGHT);
	BIND_ENUM_CONSTANT(EXPAND_FIT_HEIGHT_PROPORTIONAL);

	BIND_ENUM_CONSTANT(STRETCH_SCALE);
	BIND_ENUM_CONSTANT(STRETCH_TILE);
	BIND_ENUM_CONSTANT(STRETCH_KEEP);
	BIND_ENUM_CONSTANT(STRETCH_KEEP_CENTERED);
	BIND_ENUM_CONSTANT(STRETCH_KEEP_ASPECT);
	BIND_ENUM_CONSTANT(STRETCH_KEEP_ASPECT_CENTERED);
	BIND_ENUM_CONSTANT(STRETCH_KEEP_ASPECT_COVERED);
}

#ifndef DISABLE_DEPRECATED
bool MultiTextureRect::_set(const StringName &p_name, const Variant &p_value) {
	if ((p_name == SNAME("expand") || p_name == SNAME("ignore_texture_size")) && p_value.operator bool()) {
		expand_mode = EXPAND_IGNORE_SIZE;
		return true;
	}
	return false;
}
#endif

void MultiTextureRect::_texture_changed() {
	if (texture.is_valid()) {
		queue_redraw();
		update_minimum_size();
	}
}

void MultiTextureRect::set_texture_path(const String &path)
{
	draw_multi_texture = true;
	texture_path = path;
	queue_redraw();
	update_minimum_size();
}

void MultiTextureRect::set_texture(const Ref<Texture2D> &p_tex) {
	if (p_tex == texture) {
		return;
	}

	if (texture.is_valid()) {
		texture->disconnect(CoreStringNames::get_singleton()->changed, callable_mp(this, &MultiTextureRect::_texture_changed));
	}

	texture = p_tex;

	if (texture.is_valid()) {
		texture->connect(CoreStringNames::get_singleton()->changed, callable_mp(this, &MultiTextureRect::_texture_changed));
	}

	queue_redraw();
	update_minimum_size();
}

Ref<Texture2D> MultiTextureRect::get_texture() const {
	return texture;
}

void MultiTextureRect::set_expand_mode(ExpandMode p_mode) {
	if (expand_mode == p_mode) {
		return;
	}

	expand_mode = p_mode;
	queue_redraw();
	update_minimum_size();
}

MultiTextureRect::ExpandMode MultiTextureRect::get_expand_mode() const {
	return expand_mode;
}

void MultiTextureRect::set_stretch_mode(StretchMode p_mode) {
	if (stretch_mode == p_mode) {
		return;
	}

	stretch_mode = p_mode;
	queue_redraw();
}

MultiTextureRect::StretchMode MultiTextureRect::get_stretch_mode() const {
	return stretch_mode;
}

void MultiTextureRect::set_flip_h(bool p_flip) {
	if (hflip == p_flip) {
		return;
	}

	hflip = p_flip;
	queue_redraw();
}

bool MultiTextureRect::is_flipped_h() const {
	return hflip;
}

void MultiTextureRect::set_flip_v(bool p_flip) {
	if (vflip == p_flip) {
		return;
	}

	vflip = p_flip;
	queue_redraw();
}

bool MultiTextureRect::is_flipped_v() const {
	return vflip;
}

/* Added for showing images { */
void MultiTextureRect::set_draw_bg(bool p_bg){
	if (draw_bg == p_bg) {
		return;
	}
	draw_bg = p_bg;
}

bool MultiTextureRect::is_draw_bg() const{
	return draw_bg;
}
/* } */

MultiTextureRect::MultiTextureRect() {
	set_mouse_filter(MOUSE_FILTER_PASS);
}

MultiTextureRect::~MultiTextureRect() {
}
