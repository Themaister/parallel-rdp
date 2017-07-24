/* Copyright (c) 2017 Hans-Kristian Arntzen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once
#include "intrusive.hpp"
#include "math.hpp"
#include <vector>

namespace Granite
{
class FlatRenderer;

namespace UI
{
enum class Alignment
{
	TopLeft,
	TopCenter,
	TopRight,
	CenterLeft,
	Center,
	CenterRight,
	BottomLeft,
	BottomCenter,
	BottomCenterRight,
};

class Widget : public Util::IntrusivePtrEnabled<Widget>
{
public:
	virtual ~Widget() = default;

	virtual void add_child(Util::IntrusivePtr<Widget> widget);
	virtual Util::IntrusivePtr<Widget> remove_child(const Widget &widget);

	template <typename T, typename... P>
	inline T *add_child(P&&... p)
	{
		auto handle = Util::make_abstract_handle<Widget, T>(std::forward<P>(p)...);
		add_child(handle);
		return static_cast<T *>(handle.get());
	}

	void set_minimum_geometry(vec2 size)
	{
		geometry.minimum = size;
		geometry_changed();
	}

	void set_target_geometry(vec2 size)
	{
		geometry.target = size;
		geometry_changed();
	}

	vec2 get_target_geometry() const
	{
		return geometry.target;
	}

	vec2 get_minimum_geometry() const
	{
		return geometry.minimum;
	}

	void set_margin(float pixels)
	{
		geometry.margin = pixels;
		geometry_changed();
	}

	float get_margin() const
	{
		return geometry.margin;
	}

	void set_size_is_flexible(bool enable)
	{
		geometry.flexible_size = enable;
		geometry_changed();
	}

	bool get_size_is_flexible() const
	{
		return geometry.flexible_size;
	}

	void set_visible(bool visible)
	{
		geometry.visible = visible;
		geometry_changed();
	}

	bool get_visible() const
	{
		return geometry.visible;
	}

	void set_background_color(vec4 color)
	{
		bg_color = color;
		needs_redraw = true;
	}

	bool get_needs_redraw() const;
	void reconfigure_geometry();

	virtual float render(FlatRenderer & /* renderer */, float layer, vec2 /* offset */, vec2 /* size */)
	{
		return layer;
	}

protected:
	void geometry_changed();

	vec4 bg_color = vec4(1.0f, 1.0f, 1.0f, 0.0f);
	bool needs_redraw = true;

	struct
	{
		vec2 minimum = vec2(1);
		vec2 target = vec2(1);
		float margin = 0.0f;
		bool flexible_size = false;
		bool visible = true;
	} geometry;

	float render_children(FlatRenderer &renderer, float layer, vec2 offset);

	Widget *parent = nullptr;

	struct Child
	{
		vec2 offset;
		vec2 size;
		Util::IntrusivePtr<Widget> widget;
	};
	std::vector<Child> children;
	bool needs_reconfigure = false;

	virtual void reconfigure()
	{
	}
};

using WidgetHandle = Util::IntrusivePtr<Widget>;
}
}