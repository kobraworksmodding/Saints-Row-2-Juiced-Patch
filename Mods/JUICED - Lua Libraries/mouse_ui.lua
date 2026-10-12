-- mouse_ui.lua
-- Mouse support for vint menus, loaded by Juiced (Player/MouseUI.cpp).
-- Juiced appends mouse_ui_attach() to menu_base, main_menu, dialog_box, gmb_*, cellphone and hud.

Mouse_ui = {
	gens = { },			-- per screen thread id, thread exits when it changes
	m = { },				-- mouse state for this frame
	lock_frame = 0,	-- no hover changes until this frame after a click
	drag = nil,			-- number slider drag
	handles = { },
	cy = { }, hy = { }, x0 = { }, x1 = { },
}

Mouse_ui_handlers = { }
Mouse_ui_wrapped = { }

Mouse_ui_G = getfenv(0)

-- helpers

-- script globals live in system_lib's _DynamicGlobals, this doesn't error if missing
function mui_get(name)
	local G = Mouse_ui_G
	if G == nil then
		return nil
	end
	local finder = rawget(G, "_GetAnyGlobalSilent")
	if finder ~= nil then
		return finder(name)
	end
	return rawget(G, name)
end

function mui_abs(v)
	if v < 0 then
		return -v
	end
	return v
end

-- x0, y0, x1, y1 in vint screen space (includes children), nil if invalid
function mouse_ui_bbox(h)
	if h == nil or h == 0 or h == -1 then
		return nil
	end
	local x0, y0 = vint_get_property(h, "screen_nw")
	local x1, y1 = vint_get_property(h, "screen_se")
	if x0 == nil or y0 == nil or x1 == nil or y1 == nil then
		return nil
	end
	if x1 < x0 then
		x0, x1 = x1, x0
	end
	if y1 < y0 then
		y0, y1 = y1, y0
	end
	return x0, y0, x1, y1
end

-- element and its parents visible
function mouse_ui_visible(h)
	if h == nil or h == 0 or h == -1 then
		return false
	end
	local depth = 0
	while h ~= nil and h ~= 0 and depth < 16 do
		local v = vint_get_property(h, "visible")
		if v == false or v == 0 then
			return false
		end
		local a = vint_get_property(h, "alpha")
		if a ~= nil and type(a) == "number" and a < 0.05 then
			return false
		end
		h = vint_object_parent(h)
		depth = depth + 1
	end
	return true
end

function mouse_ui_hit(h, x, y, pad)
	local x0, y0, x1, y1 = mouse_ui_bbox(h)
	if x0 == nil then
		return false
	end
	if pad == nil then
		pad = 0
	end
	return x >= x0 - pad and x <= x1 + pad and y >= y0 - pad and y <= y1 + pad
end

function mouse_ui_hit_visible(h, m, pad)
	if h == nil or h == 0 or h == -1 then
		return false
	end
	if mouse_ui_hit(h, m.x, m.y, pad) == false then
		return false
	end
	return mouse_ui_visible(h)
end

-- mouse state for this frame
function mouse_ui_state()
	local m = Mouse_ui.m
	m.x, m.y, m.moved, m.lpress, m.lrelease, m.ldown, m.rpress, m.wheel, m.frame, m.active, m.mpress = mouse_ui_poll()
	if m.x == nil or m.y == nil then
		m.x = -10000
		m.y = -10000
		m.moved = false
		m.lpress = false
		m.lrelease = false
		m.ldown = false
		m.rpress = false
		m.mpress = false
		m.wheel = 0
		m.frame = 0
		m.active = false
	end
	if m.wheel == nil then
		m.wheel = 0
	end
	return m
end

-- only the first screen per frame gets it (phone + pause menu both run)
function mouse_ui_take(key, m)
	if Mouse_ui[key] == m.frame then
		return false
	end
	Mouse_ui[key] = m.frame
	return true
end

function mouse_ui_lock(m, frames)
	Mouse_ui.lock_frame = m.frame + frames
end

function mouse_ui_locked(m)
	return m.frame < Mouse_ui.lock_frame
end

-- right click = back
function mouse_ui_back(m)
	if m.rpress == true and mouse_ui_take("rmb_frame", m) then
		mouse_ui_inject("back", 0)
	end
end

-- one event per wheel notch
function mouse_ui_wheel(m, up_event, down_event)
	if m.wheel == 0 then
		return
	end
	if mouse_ui_take("wheel_frame", m) == false then
		return
	end
	local n = m.wheel
	local ev = up_event
	if n < 0 then
		n = -n
		ev = down_event
	end
	if n > 4 then
		n = 4
	end
	for i = 1, n do
		mouse_ui_inject(ev, 0)
	end
end

-- a dialog box owns input
function mouse_ui_dialog_open()
	local d = mui_get("Dialog_box")
	return d ~= nil and d.input_subs ~= nil
end

-- index of the vertical list entry under the cursor, -1 if none
function mouse_ui_pick_list(m, n, get_h, xpad)
	local cy = Mouse_ui.cy
	local hy = Mouse_ui.hy
	local lx0 = Mouse_ui.x0
	local lx1 = Mouse_ui.x1
	for i = 0, n - 1 do
		local x0, y0, x1, y1 = mouse_ui_bbox(get_h(i))
		if x0 ~= nil then
			cy[i] = (y0 + y1) * 0.5
			hy[i] = (y1 - y0) * 0.5
			lx0[i] = x0
			lx1[i] = x1
		else
			cy[i] = nil
		end
	end

	local best = -1
	local best_d = 0
	for i = 0, n - 1 do
		if cy[i] ~= nil and m.x >= lx0[i] - xpad and m.x <= lx1[i] + xpad then
			local half = nil
			if i > 0 and cy[i - 1] ~= nil then
				half = mui_abs(cy[i] - cy[i - 1]) * 0.5
			end
			if i < n - 1 and cy[i + 1] ~= nil then
				local h2 = mui_abs(cy[i + 1] - cy[i]) * 0.5
				if half == nil or h2 < half then
					half = h2
				end
			end
			if half == nil or half < 1 then
				half = hy[i] + 2
			end
			local dist = mui_abs(m.y - cy[i])
			if dist <= half and (best < 0 or dist < best_d) then
				best = i
				best_d = dist
			end
		end
	end
	return best
end

-- game callbacks run in their own thread so a yield/error doesn't kill ours
function mouse_ui_call(action, a, b)
	thread_new("mouse_ui_do", action, a, b)
end

function mouse_ui_do(action, a, b)
	if action == "menu_item" then
		mouse_ui_menu_set_item(a)
	elseif action == "menu_horz" then
		mouse_ui_menu_horz_jump(a)
	elseif action == "menu_grid" then
		mouse_ui_menu_grid_set(a)
	elseif action == "menu_scroll" then
		mouse_ui_menu_scroll_to(a)
	elseif action == "plist_item" then
		mouse_ui_plist_set(a, b)
	elseif action == "plist_view" then
		mouse_ui_plist_view(a, b)
	elseif action == "main_menu_hover" then
		if mui_get("Main_menu_can_has_input") == true and mui_get("Menu_active") == 0 and menu_input_is_blocked() == false then
			main_menu_update_nav_bar(a)
			audio_play(Menu_sound_item_nav)
		end
	elseif action == "main_menu_nav" then
		if mui_get("Main_menu_can_has_input") == true and mui_get("Menu_active") == 0 then
			main_menu_nav(0, a)
		end
	elseif action == "cell_main" then
		local c = mui_get("Cellphone")
		if c ~= nil and c.menu_active ~= nil and c.cur_idx ~= nil and c.cur_idx ~= a then
			local old = c.cur_idx
			c.cur_idx = a
			c.menu_active.cur_idx = a
			if c.menu_active.on_nav ~= nil then
				c.menu_active.on_nav(a, old)
			end
		end
	end
end

-- threads

function mouse_ui_report(kind, err)
	local msg = "mouse_ui (" .. tostring(kind) .. "): " .. tostring(err)
	if Mouse_ui.last_error ~= msg then
		Mouse_ui.last_error = msg
		debug_print("vint", msg .. "\n")
	end
end

function mouse_ui_thread(kind, gen)
	while Mouse_ui.gens[kind] == gen do
		local fn = Mouse_ui_handlers[kind]
		if fn ~= nil then
			Mouse_ui.cur_kind = kind
			local ok, err = pcall(fn)
			Mouse_ui.cur_kind = nil
			if ok == false then
				mouse_ui_report(kind, err)
			end
		end
		thread_yield()
	end
end

Mouse_ui_kinds = { "menu", "main_menu", "dialog", "blackjack", "poker", "cellphone", "radial" }
Mouse_ui.req = { }

-- remembers which screen asked this frame
function mui_request(hover)
	local kind = Mouse_ui.cur_kind
	if kind ~= nil then
		Mouse_ui.req[kind] = Mouse_ui.m.frame
	end
	mouse_ui_request(hover)
end

-- give the mouse back now if no other screen still wants it
function mouse_ui_release(kind)
	Mouse_ui.req[kind] = nil
	local f = Mouse_ui.m.frame
	if f == nil then
		f = 0
	end
	for i = 1, 7 do
		local k = Mouse_ui_kinds[i]
		local rf = Mouse_ui.req[k]
		if k ~= kind and rf ~= nil and rf >= f - 2 then
			return
		end
	end
	local fn = mui_get("mouse_ui_end")
	if fn ~= nil then
		fn()
	end
end

function mouse_ui_start(kind)
	if mouse_ui_poll == nil then
		return
	end
	local g = Mouse_ui.gens[kind]
	if g == nil then
		g = 0
	end
	g = g + 1
	Mouse_ui.gens[kind] = g
	thread_new("mouse_ui_thread", kind, g)
end

function mouse_ui_stop(kind)
	local g = Mouse_ui.gens[kind]
	if g == nil then
		g = 0
	end
	Mouse_ui.gens[kind] = g + 1
	mouse_ui_release(kind)
end

-- runs before() / after() around a global function
function mouse_ui_wrap(fname, before, after)
	local G = Mouse_ui_G
	if G == nil then
		return false
	end
	local orig = mui_get(fname)
	if type(orig) ~= "function" then
		return false
	end
	if Mouse_ui_wrapped[fname] == orig then
		return true	-- already wrapped
	end
	local wrapper = function(...)
		if before ~= nil then
			before()
		end
		local r = { orig(unpack(arg)) }
		if after ~= nil then
			after()
		end
		return unpack(r)
	end
	Mouse_ui_wrapped[fname] = wrapper
	-- goes through system_lib's __newindex into the script's table
	G[fname] = wrapper
	return true
end

function mouse_ui_menu_start()
	Mouse_ui.handles = { }
	Mouse_ui.drag = nil
	Mouse_ui.sdrag = nil
	Mouse_ui.menu_closing = false
	mouse_ui_start("menu")
end

function mouse_ui_menu_stop()
	Mouse_ui.drag = nil
	Mouse_ui.sdrag = nil
	mouse_ui_horz_hover(nil)
	mouse_ui_stop("menu")
end

-- menu_close() animates out before releasing input, don't wait for it
function mouse_ui_menu_closing()
	Mouse_ui.drag = nil
	Mouse_ui.sdrag = nil
	Mouse_ui.menu_closing = true
	mouse_ui_horz_hover(nil)
	mouse_ui_release("menu")
end

function mouse_ui_dialog_released()
	mouse_ui_release("dialog")
end

function mouse_ui_main_menu_start()		mouse_ui_start("main_menu")	end
function mouse_ui_main_menu_stop()		mouse_ui_stop("main_menu")		end
function mouse_ui_dialog_start()			mouse_ui_start("dialog")		end
function mouse_ui_dialog_stop()			mouse_ui_stop("dialog")			end
function mouse_ui_blackjack_start()		mouse_ui_start("blackjack")	end
function mouse_ui_blackjack_stop()		mouse_ui_stop("blackjack")		end
function mouse_ui_poker_start()			mouse_ui_start("poker")			end
function mouse_ui_poker_stop()			mouse_ui_stop("poker")			end
function mouse_ui_cellphone_start()		mouse_ui_start("cellphone")	end
function mouse_ui_cellphone_stop()		mouse_ui_stop("cellphone")		end

-- appended to the scripts by Juiced
function mouse_ui_attach(script)
	if mouse_ui_poll == nil then
		return
	end
	if script == "menu_base" then
		mouse_ui_wrap("menu_grab_input", nil, mouse_ui_menu_start)
		mouse_ui_wrap("menu_release_input", nil, mouse_ui_menu_stop)
		mouse_ui_wrap("menu_close", mouse_ui_menu_closing, nil)
	elseif script == "main_menu" then
		mouse_ui_wrap("main_menu_init", nil, mouse_ui_main_menu_start)
		mouse_ui_wrap("main_menu_cleanup", mouse_ui_main_menu_stop, nil)
	elseif script == "dialog_box" then
		mouse_ui_wrap("dialog_box_init", nil, mouse_ui_dialog_start)
		mouse_ui_wrap("dialog_box_cleanup", mouse_ui_dialog_stop, nil)
		mouse_ui_wrap("dialog_box_release_input", nil, mouse_ui_dialog_released)
	elseif script == "gmb_blackjack" then
		mouse_ui_wrap("gmb_blackjack_init", nil, mouse_ui_blackjack_start)
		mouse_ui_wrap("gmb_blackjack_cleanup", mouse_ui_blackjack_stop, nil)
	elseif script == "gmb_poker" then
		mouse_ui_wrap("gmb_poker_init", nil, mouse_ui_poker_start)
		mouse_ui_wrap("gmb_poker_cleanup", mouse_ui_poker_stop, nil)
	elseif script == "cellphone" then
		mouse_ui_wrap("cellphone_init", nil, mouse_ui_cellphone_start)
		mouse_ui_wrap("cellphone_cleanup", mouse_ui_cellphone_stop, nil)
	elseif script == "hud" then
		mouse_ui_wrap("hud_inventory_show", nil, mouse_ui_radial_start)
		mouse_ui_wrap("hud_inventory_hide", mouse_ui_radial_stop, nil)
		mouse_ui_radial_hook_input()
	end
end

-- menu_base (pause menu, sub menus, stores, garage, cribs...)

function mouse_ui_menu_handler()
	local m = mouse_ui_state()
	local active = mui_get("Menu_active")
	if active == nil or active == 0 or Mouse_ui.menu_closing == true then
		Mouse_ui.drag = nil
		Mouse_ui.sdrag = nil
		mouse_ui_horz_hover(nil)
		return
	end
	if mouse_ui_dialog_open() then
		Mouse_ui.drag = nil
		mouse_ui_horz_hover(nil)
		mui_request(false)
		return
	end
	Mouse_ui.horz_want = nil

	local can_act = m.active == true and menu_input_is_blocked() == false and Menu_transition_in_progress ~= true
	local hover = false
	local plab, pdata = mouse_ui_plist_get()

	if mouse_ui_menu_slider_drag(m) or mouse_ui_menu_scroll_drag(m, can_act) then
		hover = true
	else
		if mouse_ui_menu_horz(m, can_act) then
			hover = true
		end
		if mouse_ui_menu_tips(m, can_act) then
			hover = true
		elseif plab ~= nil then
			if mouse_ui_plist(m, can_act, plab, pdata) then
				hover = true
			end
		elseif Menu_active.custom_show ~= true and Menu_option_labels ~= nil and mouse_ui_visible(Menu_option_labels.frame) then
			if mouse_ui_menu_scroll_bar(m, can_act) then
				hover = true
			elseif mouse_ui_menu_rows(m, can_act) then
				hover = true
			end
		end
		if can_act then
			mouse_ui_back(m)
			-- the playlist and the phone handle the wheel themselves
			if plab == nil and mouse_ui_phone_open() == false then
				mouse_ui_wheel(m, "nav_up", "nav_down")
			end
		end
	end

	if m.active == true then
		mouse_ui_horz_hover(Mouse_ui.horz_want)
	else
		mouse_ui_horz_hover(nil)
	end
	mui_request(hover)
end
Mouse_ui_handlers["menu"] = mouse_ui_menu_handler

-- true when the cursor is over a row
function mouse_ui_menu_rows(m, can_act)
	local menu = Menu_active
	local labels = Menu_option_labels
	local num_items = menu.num_items
	if num_items == nil or num_items < 1 or labels.max_rows == nil or labels[0] == nil then
		return false
	end

	-- swatch grids (colours, tattoos, ...) are a single item menu
	if num_items == 1 and menu[0] ~= nil and menu[0].type == MENU_ITEM_TYPE_GRID and menu[0].swatches ~= nil then
		return mouse_ui_menu_grid(m, can_act, menu[0])
	end

	local first = menu.first_vis_item
	if first == nil or first < 0 then
		first = 0
	end
	local num_rows = num_items - first
	if num_rows > labels.max_rows then
		num_rows = labels.max_rows
	end
	if num_rows < 1 then
		return false
	end

	-- row stripes are 28 high and centred on the row, rows are MENU_ITEM_HEIGHT apart
	local sx0, sy0, sx1, sy1
	local ref = -1
	for r = 0, num_rows - 1 do
		if labels[r] ~= nil then
			sx0, sy0, sx1, sy1 = mouse_ui_bbox(labels[r].stripe_h)
			if sx0 ~= nil and sy1 - sy0 > 1 then
				ref = r
				break
			end
		end
	end
	if ref < 0 then
		return false
	end
	local scale = (sy1 - sy0) / 28
	if scale <= 0.01 then
		return false
	end
	local row_h = MENU_ITEM_HEIGHT * scale
	local row_x0 = sx0 - 6 * scale
	local row_x1 = sx1
	if labels.item_width ~= nil then
		row_x1 = sx0 + (labels.item_width + 6) * scale
	end
	if m.x < row_x0 or m.x > row_x1 then
		return false
	end

	local top = (sy0 + sy1) * 0.5 - row_h * 0.5 - ref * row_h
	local rel = (m.y - top) / row_h
	if rel < 0 then
		return false
	end
	local r = 0
	while r + 1 <= rel and r < num_rows do
		r = r + 1
	end
	if r >= num_rows then
		return false
	end

	local item = first + r
	local data = menu[item]
	if data == nil then
		return false
	end
	if data.disabled == true then
		return true
	end

	if can_act then
		if item ~= menu.highlighted_item and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
			mouse_ui_call("menu_item", item)
		end
		if item == menu.highlighted_item and data.type == MENU_ITEM_TYPE_PALETTE and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
			mouse_ui_menu_palette_hover(m, r, data)
		end
		if m.lpress == true and Menu_active == menu and menu.highlighted_item == item then
			mouse_ui_menu_click(m, r, item, data)
		end
	end
	return true
end

-- colour swatch (0 - 2) of a palette row under the cursor, -1 if none
function mouse_ui_menu_palette_swatch(m, r, data)
	local o = Menu_option_labels[r]
	if o == nil or o.control == nil or o.control.grp_h == nil or data.palette_colors == nil then
		return -1
	end
	local n = data.palette_colors.num_colors
	if n == nil then
		return -1
	end
	if n > 3 then
		n = 3
	end
	for i = 0, n - 1 do
		if mouse_ui_hit(vint_object_find("swatch_color_" .. i, o.control.grp_h), m.x, m.y, 2) then
			return i
		end
	end
	return -1
end

-- hovering a colour picks it, like nav_left / nav_right does
function mouse_ui_menu_palette_hover(m, r, data)
	local i = mouse_ui_menu_palette_swatch(m, r, data)
	local o = Menu_option_labels[r]
	if i >= 0 and o.control.highlighted_index ~= i then
		menu_pallette_update_selected(o, data, i)
		audio_play(Menu_sound_item_nav)
	end
end

-- highlight without scrolling, otherwise the list runs away under the cursor
function mouse_ui_menu_set_item(item)
	if Menu_active == 0 or Menu_active == nil or Menu_active[item] == nil then
		return
	end
	if menu_input_is_blocked() or Menu_transition_in_progress == true or item == Menu_active.highlighted_item then
		return
	end
	local first = Menu_active.first_vis_item
	local saved_limit = limit
	if first ~= nil and first >= 0 and saved_limit ~= nil then
		limit = function(value, low, high)
			limit = saved_limit
			if first < low then
				return low
			elseif first > high then
				return high
			end
			return first
		end
	end
	menu_update_nav_bar(item)
	limit = saved_limit

	audio_play(Menu_sound_item_nav)
	if Menu_active.on_nav ~= nil then
		Menu_active.on_nav(Menu_active)
	end
	local tips = mui_get("btn_tips_update")
	if tips ~= nil then
		tips()
	end
end

function mouse_ui_menu_click(m, r, item, data)
	local t = data.type
	local o = Menu_option_labels[r]
	local control = nil
	if o ~= nil then
		control = o.control
	end

	if t == MENU_ITEM_TYPE_PALETTE and control ~= nil and control.grp_h ~= nil then
		-- click a colour: pick it and open the colour picker for it, the arrows still step
		local i = mouse_ui_menu_palette_swatch(m, r, data)
		if i >= 0 then
			if control.highlighted_index ~= i then
				menu_pallette_update_selected(o, data, i)
			end
		elseif mouse_ui_hit_visible(vint_object_find("arrow_w", control.grp_h), m, 4) then
			mouse_ui_inject("nav_left", 2)
			mouse_ui_lock(m, 6)
			return
		elseif mouse_ui_hit_visible(vint_object_find("arrow_e", control.grp_h), m, 4) then
			mouse_ui_inject("nav_right", 2)
			mouse_ui_lock(m, 6)
			return
		end
		mouse_ui_inject("select", 0)
		mouse_ui_lock(m, 8)
		return
	end

	if control ~= nil and control.grp_h ~= nil and (t == MENU_ITEM_TYPE_NUM_SLIDER or t == MENU_ITEM_TYPE_TEXT_SLIDER) then
		local cx0, cy0, cx1, cy1 = mouse_ui_bbox(control.grp_h)
		if cx0 ~= nil and m.x >= cx0 - 8 then
			if t == MENU_ITEM_TYPE_NUM_SLIDER then
				-- grab the thumb
				Mouse_ui.drag = { item = item, sound_frame = 0 }
				mouse_ui_menu_slider_drag(m)
			else
				local ev = "nav_right"
				local wx0, wy0, wx1, wy1 = mouse_ui_bbox(vint_object_find("text_slider_arrow_w", control.grp_h))
				local ex0, ey0, ex1, ey1 = mouse_ui_bbox(vint_object_find("text_slider_arrow_e", control.grp_h))
				if wx0 ~= nil and m.x <= wx1 + 4 then
					ev = "nav_left"
				elseif ex0 ~= nil and m.x >= ex0 - 4 then
					ev = "nav_right"
				elseif m.x < (cx0 + cx1) * 0.5 then
					ev = "nav_left"
				end
				-- delay, nav_left/right subscriptions made this frame aren't ready yet
				mouse_ui_inject(ev, 2)
			end
			mouse_ui_lock(m, 6)
			return
		end
	end

	mouse_ui_inject("select", 0)
	mouse_ui_lock(m, 8)
end

-- number slider drag
function mouse_ui_menu_slider_drag(m)
	local d = Mouse_ui.drag
	if d == nil then
		return false
	end
	if m.ldown ~= true or Menu_active == 0 or Menu_active == nil or Menu_active.highlighted_item ~= d.item then
		Mouse_ui.drag = nil
		return false
	end
	local data = Menu_active[d.item]
	local first = Menu_active.first_vis_item
	if first == nil or first < 0 then
		first = 0
	end
	local o = Menu_option_labels[d.item - first]
	if data == nil or o == nil or o.control == nil or data.type ~= MENU_ITEM_TYPE_NUM_SLIDER then
		Mouse_ui.drag = nil
		return false
	end
	if menu_input_is_blocked() then
		return true
	end

	local g = o.control.grp_h
	local wx0, wy0, wx1, wy1 = mouse_ui_bbox(vint_object_find("num_slider_arrow_w", g))
	local ex0, ey0, ex1, ey1 = mouse_ui_bbox(vint_object_find("num_slider_arrow_e", g))
	if wx0 == nil or ex0 == nil then
		return true
	end
	local left = wx1
	local right = ex0
	if right - left < 4 then
		return true
	end
	local v = (m.x - left) / (right - left)
	if v < 0 then
		v = 0
	elseif v > 1 then
		v = 1
	end
	local old = data.cur_value
	if old == nil or mui_abs(v - old) > 0.002 then
		data.cur_value = v
		menu_num_slider_update_value(o, data)
		if m.frame >= d.sound_frame then
			audio_play(Menu_sound_value_nav)
			d.sound_frame = m.frame + 4
		end
	end
	return true
end

-- scroll bar drag

-- kind, total, visible, first of the list/grid using the scroll bar
-- owner is nil for the menu, "edit" / "playlist" for the playlist editor panes
function mouse_ui_scroll_target(owner)
	if owner == "edit" or owner == "playlist" then
		local lab, data = mouse_ui_plist_get()
		if lab == nil then
			return nil
		end
		local pd = data[owner]
		if pd == nil or pd.has_scroll_bar ~= true or pd.scroll_bar == nil then
			return nil
		end
		return "plist", pd.num_items, mui_get("PLAYLIST_MAX_ITEMS"), pd.first_vis_item
	end
	local menu = Menu_active
	local labels = Menu_option_labels
	if menu == nil or menu == 0 or labels == nil or labels.scroll_bar == nil then
		return nil
	end
	if labels.scroll_bar.visible ~= true then
		return nil
	end
	local grid = menu[0]
	if menu.num_items == 1 and grid ~= nil and grid.type == MENU_ITEM_TYPE_GRID and grid.scroll_bar_visible == true then
		if grid.total_rows == nil or grid.visible_rows == nil or grid.first_vis_row == nil then
			return nil
		end
		return "grid", grid.total_rows, grid.visible_rows, grid.first_vis_row
	end
	if labels.scroll_bar_visible == true and menu.num_items ~= nil and labels.max_rows ~= nil then
		local first = menu.first_vis_item
		if first == nil or first < 0 then
			first = 0
		end
		return "list", menu.num_items, labels.max_rows, first
	end
	return nil
end

-- thumb top / bottom
function mouse_ui_scroll_thumb(sd)
	local nx0, ny0, nx1, ny1 = mouse_ui_bbox(sd.thumb_n_h)
	local sx0, sy0, sx1, sy1 = mouse_ui_bbox(sd.thumb_s_h)
	if nx0 == nil or sx0 == nil then
		return nil
	end
	local y0 = ny0
	local y1 = sy1
	if sy0 < y0 then
		y0 = sy0
	end
	if ny1 > y1 then
		y1 = ny1
	end
	return y0, y1
end

function mouse_ui_menu_scroll_bar(m, can_act)
	local kind, total, vis, first = mouse_ui_scroll_target()
	if kind == nil or total <= vis then
		return false
	end
	return mouse_ui_scroll_bar_click(m, can_act, Menu_option_labels.scroll_bar, nil)
end

-- true when the cursor is on the bar, a click grabs it
function mouse_ui_scroll_bar_click(m, can_act, sd, owner)
	local x0, y0, x1, y1 = mouse_ui_bbox(sd.bar_grp)
	if x0 == nil or m.x < x0 - 6 or m.x > x1 + 6 or m.y < y0 or m.y > y1 then
		return false
	end
	if can_act and m.lpress == true then
		local ty0, ty1 = mouse_ui_scroll_thumb(sd)
		local scale = (y1 - y0) / (sd.bar_height + 54)
		if ty0 ~= nil and scale > 0.01 then
			local travel = (sd.bar_height - sd.thumb_height) * scale
			local grab = (ty1 - ty0) * 0.5	-- track click centres the thumb
			if m.y >= ty0 and m.y <= ty1 then
				grab = m.y - ty0					-- grabbing the thumb keeps it under the cursor
			end
			local pos = sd.thumb_pos
			if pos == nil then
				pos = 0
			end
			Mouse_ui.sdrag = { owner = owner, grab = grab, top = ty0 - pos * travel, travel = travel, sound_frame = 0 }
			mouse_ui_menu_scroll_drag(m, can_act)
		end
	end
	return true
end

function mouse_ui_menu_scroll_drag(m, can_act)
	local d = Mouse_ui.sdrag
	if d == nil then
		return false
	end
	if m.ldown ~= true then
		Mouse_ui.sdrag = nil
		return false
	end
	local kind, total, vis, first = mouse_ui_scroll_target(d.owner)
	if kind == nil or total <= vis then
		Mouse_ui.sdrag = nil
		return false
	end
	if can_act == false or d.travel <= 1 then
		return true
	end

	local pos = (m.y - d.grab - d.top) / d.travel
	if pos < 0 then
		pos = 0
	elseif pos > 1 then
		pos = 1
	end
	local target = floor(pos * (total - vis) + 0.5)
	if target ~= first then
		if kind == "plist" then
			mouse_ui_call("plist_view", d.owner, target)
		else
			mouse_ui_call("menu_scroll", target)
		end
		if m.frame >= d.sound_frame then
			audio_play(Menu_sound_item_nav)
			d.sound_frame = m.frame + 3
		end
	end
	return true
end

-- scroll so "first" is the top row, highlight follows
function mouse_ui_menu_scroll_to(first)
	local kind, total, vis, cur = mouse_ui_scroll_target()
	if kind == nil or first == cur or menu_input_is_blocked() or Menu_transition_in_progress == true then
		return
	end

	if kind == "grid" then
		local grid = Menu_active[0]
		-- update_swatches keeps the focus row one inside the window
		local row
		if first > cur then
			row = first + vis - 2
		else
			row = first + 1
		end
		if row < 0 then
			row = 0
		elseif row > total - 1 then
			row = total - 1
		end
		local col = grid.cur_col
		if col == nil then
			col = 0
		end
		while col > 0 and row * grid.num_cols + col >= grid.swatches.num_swatches do
			col = col - 1
		end
		menu_grid_update_highlighted(grid, col, row)
		if grid.on_nav ~= nil then
			grid.on_nav(Menu_option_labels[0], grid)
		end
		return
	end

	local menu = Menu_active
	local item = menu.highlighted_item
	if item == nil or item < first then
		item = first
	elseif item > first + vis - 1 then
		item = first + vis - 1
	end
	-- skip disabled entries, stay inside the new window
	if menu[item] ~= nil and menu[item].disabled == true then
		local found = -1
		for i = first, first + vis - 1 do
			if menu[i] ~= nil and menu[i].disabled ~= true then
				found = i
				break
			end
		end
		if found < 0 then
			return
		end
		item = found
	end

	-- menu_update_nav_bar normally centres the highlight, force the window instead
	local saved_limit = limit
	limit = function(value, low, high)
		limit = saved_limit
		if first < low then
			return low
		elseif first > high then
			return high
		end
		return first
	end
	menu_update_nav_bar(item)
	limit = saved_limit

	if menu.on_nav ~= nil then
		menu.on_nav(menu)
	end
	local tips = mui_get("btn_tips_update")
	if tips ~= nil then
		tips()
	end
end

-- tabs
function mouse_ui_menu_horz(m, can_act)
	local horz = Menu_horz_active
	if horz == nil or horz == 0 or horz.num_items == nil or horz.num_items < 1 then
		return false
	end

	local doc = MENU_BASE_DOC_HANDLE
	local hd = Mouse_ui.handles
	if hd.doc ~= doc then
		hd.doc = doc
		hd.horz = vint_object_find("menu_horz", nil, doc)
		hd.arrow_w = vint_object_find("horz_arrow_w", nil, doc)
		hd.arrow_e = vint_object_find("horz_arrow_e", nil, doc)
		hd.trig_l = vint_object_find("upper_nav_trigger_left", nil, doc)
		hd.trig_r = vint_object_find("upper_nav_trigger_right", nil, doc)
	end
	if mouse_ui_visible(hd.horz) == false then
		return false
	end

	-- LT / RT icons and the scroll arrows step one tab
	if horz.num_items > 1 then
		if mouse_ui_hit_visible(hd.trig_l, m, 4) or mouse_ui_hit_visible(hd.arrow_w, m, 4) then
			if can_act and m.lpress == true then
				mouse_ui_inject("scroll_left", 0)
				mouse_ui_lock(m, 8)
			end
			return true
		end
		if mouse_ui_hit_visible(hd.trig_r, m, 4) or mouse_ui_hit_visible(hd.arrow_e, m, 4) then
			if can_act and m.lpress == true then
				mouse_ui_inject("scroll_right", 0)
				mouse_ui_lock(m, 8)
			end
			return true
		end
	end

	-- labels scrolled outside of the clip region aren't clickable
	local clip_x0 = nil
	local clip_x1 = nil
	if mouse_ui_visible(hd.arrow_w) and mouse_ui_visible(hd.arrow_e) then
		local a0, b0, a1, b1 = mouse_ui_bbox(hd.arrow_w)
		if a0 ~= nil then
			clip_x0 = a1
		end
		a0, b0, a1, b1 = mouse_ui_bbox(hd.arrow_e)
		if a0 ~= nil then
			clip_x1 = a0
		end
	end

	for i = 0, horz.num_items - 1 do
		local item = horz[i]
		if item ~= nil and item.label_h ~= nil then
			local x0, y0, x1, y1 = mouse_ui_bbox(item.label_h)
			if x0 ~= nil and m.x >= x0 - 6 and m.x <= x1 + 6 and m.y >= y0 - 4 and m.y <= y1 + 4
				and (clip_x0 == nil or m.x >= clip_x0) and (clip_x1 == nil or m.x <= clip_x1) then
				Mouse_ui.horz_want = i
				if can_act and m.lpress == true and horz.num_items > 1 and i ~= horz.current_selection then
					mouse_ui_call("menu_horz", i)
					mouse_ui_lock(m, 8)
				end
				return true
			end
		end
	end
	return false
end

-- tab under the cursor goes light grey, the selected tab keeps its own colour
MOUSE_UI_HORZ_HOVER_TINT = { r = 0.82, g = 0.82, b = 0.82 }

function mouse_ui_horz_hover(index)
	local horz = mui_get("Menu_horz_active")
	if horz == 0 then
		horz = nil
	end
	local new_h = nil
	if index ~= nil and horz ~= nil and horz[index] ~= nil and index ~= horz.current_selection then
		new_h = horz[index].label_h
	end

	local old_h = Mouse_ui.horz_hover
	if old_h ~= nil and old_h ~= new_h then
		-- only if it's still one of the tabs and isn't the selected one (its tween owns the colour)
		if horz ~= nil and horz.num_items ~= nil and horz.norm_item ~= nil and horz.norm_item.tint ~= nil then
			local t = horz.norm_item.tint
			for i = 0, horz.num_items - 1 do
				local item = horz[i]
				if item ~= nil and item.label_h == old_h then
					if i ~= horz.current_selection and t.r ~= nil then
						vint_set_property(old_h, "tint", t.r, t.g, t.b)
					end
					break
				end
			end
		end
	end

	Mouse_ui.horz_hover = new_h
	if new_h ~= nil then
		local c = MOUSE_UI_HORZ_HOVER_TINT
		vint_set_property(new_h, "tint", c.r, c.g, c.b)
	end
end

-- menu_horz_nav but straight to a tab
function mouse_ui_menu_horz_jump(index)
	local horz = Menu_horz_active
	if horz == nil or horz == 0 or Menu_active == nil or Menu_active == 0 then
		return
	end
	if menu_input_is_blocked() or Menu_transition_in_progress == true then
		return
	end
	if horz.num_items == nil or horz.num_items < 2 or horz[index] == nil then
		return
	end
	local previous = horz.current_selection
	if index == previous then
		return
	end

	if index > previous then
		horz.trans_animation = "menu_horz_rt_anim"
	else
		horz.trans_animation = "menu_horz_lt_anim"
	end
	lua_play_anim(vint_object_find(horz.trans_animation, nil, MENU_BASE_DOC_HANDLE), 0, MENU_BASE_DOC_HANDLE)

	if Menu_active.on_horz_show ~= nil then
		Menu_active.on_horz_show(index)
	else
		audio_play(Menu_sound_scroll)
		horz.previous_selection = previous
		horz.current_selection = index

		if horz.on_nav ~= nil then
			horz.on_nav(horz[index].sub_menu)
		end

		menu_horz_animate_label_pos()
		horz[index].sub_menu.highlighted_item = 0
		menu_show(horz[index].sub_menu, MENU_TRANSITION_SWAP)
	end

	local horz_item = horz[horz.current_selection]
	if horz_item ~= nil and mui_get("event_tracking_interface_tab_change_string") ~= nil then
		if horz_item.label_crc ~= nil then
			event_tracking_interface_tab_change_string(horz_item.label_crc)
		elseif horz_item.label ~= nil then
			event_tracking_interface_tab_change_string(horz_item.label)
		end
	end
end

-- same check as menu_grid_update_swatches
function mouse_ui_grid_scrolls(g, focus_row)
	local first = g.first_vis_row
	local vis = g.visible_rows
	local total = g.total_rows
	if first == nil or vis == nil or total == nil then
		return true
	end
	local new_first
	if focus_row < 2 then
		new_first = 0
	elseif focus_row >= total - 1 then
		new_first = total - vis
	elseif focus_row < first + 1 then
		new_first = focus_row - 1
	elseif focus_row >= first + vis - 1 then
		new_first = focus_row - vis + 2
	else
		return false
	end
	return new_first ~= first
end

function mouse_ui_grid_swatch_hit(sw, idx, m)
	local s = sw[idx]
	if s == nil or s.swatch_h == nil then
		return false
	end
	local h = s.swatch_bg_h
	if h == nil or h == 0 then
		h = s.swatch_h
	end
	return mouse_ui_hit(h, m.x, m.y, 0)
end

function mouse_ui_menu_grid(m, can_act, grid)
	local sw = grid.swatches
	if sw == nil or sw.num_swatches == nil or grid.num_cols == nil or grid.num_cols < 1 then
		return false
	end

	local cur = grid.cur_idx
	if cur == nil and grid.cur_row ~= nil and grid.cur_col ~= nil then
		cur = grid.cur_row * grid.num_cols + grid.cur_col
	end

	-- highlighted swatch is scaled up, test it first
	local hit = -1
	if cur ~= nil and mouse_ui_grid_swatch_hit(sw, cur, m) then
		hit = cur
	else
		for idx = 0, sw.num_swatches - 1 do
			if mouse_ui_grid_swatch_hit(sw, idx, m) then
				hit = idx
				break
			end
		end
	end
	if hit < 0 then
		return false
	end
	if can_act == false then
		return true
	end

	if hit ~= cur and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
		local row = 0
		local col = hit
		while col >= grid.num_cols do
			col = col - grid.num_cols
			row = row + 1
		end
		-- hovering never scrolls the grid, clicking does
		if m.lpress == true or mouse_ui_grid_scrolls(grid, row) == false then
			mouse_ui_call("menu_grid", hit)
		end
	end

	if m.lpress == true then
		mouse_ui_inject("select", 0)
		mouse_ui_lock(m, 8)
	end
	return true
end

function mouse_ui_menu_grid_set(idx)
	if Menu_active == nil or Menu_active == 0 or menu_input_is_blocked() then
		return
	end
	local grid = Menu_active[0]
	if grid == nil or grid.type ~= MENU_ITEM_TYPE_GRID or grid.swatches == nil or grid.num_cols == nil then
		return
	end
	local row = 0
	local col = idx
	while col >= grid.num_cols do
		col = col - grid.num_cols
		row = row + 1
	end
	menu_grid_update_highlighted(grid, col, row)
	audio_play(Menu_sound_value_nav)
	if grid.on_nav ~= nil then
		grid.on_nav(Menu_option_labels[0], grid)
	end
end

-- button tips along the bottom (and in menu footers) are clickable

Mouse_ui_tip_events = { [0] = "select", [1] = "alt_select", [2] = "back" }

function mouse_ui_tip_hit(m, bmp_h, label_h)
	return mouse_ui_hit_visible(bmp_h, m, 4) or mouse_ui_hit_visible(label_h, m, 4)
end

function mouse_ui_menu_tips(m, can_act)
	local ev = nil
	local tips = mui_get("Menu_base_btn_tips")
	if tips ~= nil then
		for i = 0, 2 do
			local t = tips[i]
			if t ~= nil and t.visible == true and mouse_ui_tip_hit(m, t.bitmap_h, t.label_h) then
				ev = Mouse_ui_tip_events[i]
				break
			end
		end
	end
	local f = Menu_active.footer
	if ev == nil and f ~= nil and f.footer_grp ~= nil and f.footer_grp ~= 0 then
		local g = f.footer_grp
		if mouse_ui_tip_hit(m, vint_object_find("footer_tip_a_bmp", g), vint_object_find("footer_tip_a_label", g)) then
			ev = "select"
		elseif mouse_ui_tip_hit(m, vint_object_find("footer_tip_b_bmp", g), vint_object_find("footer_tip_b_label", g)) then
			ev = "back"
		end
	end
	if ev == nil then
		return false
	end
	if can_act and m.lpress == true and mouse_ui_locked(m) == false then
		mouse_ui_inject(ev, 0)
		mouse_ui_lock(m, 8)
	end
	return true
end

-- playlist editor (two panes: tracks on the left, playlist on the right)

function mouse_ui_plist_get()
	local t = mui_get("PAUSE_MENU_CONTROL_PLAYLIST_EDITOR")
	if t == nil or Menu_option_labels == nil or Menu_active.num_items ~= 1 or Menu_active[0] == nil or Menu_active[0].type ~= t then
		return nil
	end
	local lab = Menu_option_labels[0]
	local data = Menu_active[0]
	if lab == nil or lab.edit == nil or lab.playlist == nil or lab.active_pane == nil then
		return nil
	end
	if lab.edit.option_labels == nil or lab.playlist.option_labels == nil then
		return nil
	end
	if data.edit == nil or data.playlist == nil or data.active_pane == nil then
		return nil
	end
	return lab, data
end

-- row 0 centre, row step, x range of a pane
function mouse_ui_plist_rows(pl, pd)
	local max = mui_get("PLAYLIST_MAX_ITEMS")
	if max == nil or pl.option_labels == nil then
		return nil
	end
	for i = 0, max - 1 do
		local o = pl.option_labels[i]
		if o ~= nil then
			local x0, y0, x1, y1 = mouse_ui_bbox(o.label_h)
			local w, h = element_get_actual_size(o.label_h)
			if x0 ~= nil and h ~= nil and h > 1 and y1 - y0 > 1 then
				local s = (y1 - y0) / h
				local step = 33 * s
				local width = pl.select_bar_width
				if width == nil then
					width = 330
				end
				if pd.has_scroll_bar == true then
					width = width - 40
				end
				return (y0 + y1) * 0.5 - i * step, step, x0 - 6 * s, x0 + (width - 10) * s, max
			end
		end
	end
	return nil
end

-- pane name and row under the cursor, row is -1 when between rows / past the end
function mouse_ui_plist_pick(m, lab, data)
	for k = 1, 2 do
		local name = "edit"
		if k == 2 then
			name = "playlist"
		end
		local pd = data[name]
		local cy0, step, x0, x1, max = mouse_ui_plist_rows(lab[name], pd)
		if cy0 ~= nil and m.x >= x0 and m.x <= x1 then
			local top = cy0 - step * 0.5
			if m.y >= top and m.y <= top + step * max then
				local r = floor((m.y - top) / step)
				local item = pd.first_vis_item + r
				if r < 0 or r >= max or item >= pd.num_items or pd.no_items == true then
					return name, -1
				end
				return name, item
			end
		end
	end
	return nil, -1
end

function mouse_ui_plist(m, can_act, lab, data)
	-- pane scroll bars
	if data.edit.has_scroll_bar == true and data.edit.scroll_bar ~= nil and mouse_ui_scroll_bar_click(m, can_act, data.edit.scroll_bar, "edit") then
		return true
	end
	if data.playlist.has_scroll_bar == true and data.playlist.scroll_bar ~= nil and mouse_ui_scroll_bar_click(m, can_act, data.playlist.scroll_bar, "playlist") then
		return true
	end

	local name, item = mouse_ui_plist_pick(m, lab, data)
	if can_act and item >= 0 then
		local pd = data[name]
		local current = data.active_pane == pd and pd.highlighted_item == item
		if current == false and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
			mouse_ui_call("plist_item", name, item)
		end
		if m.lpress == true and mouse_ui_locked(m) == false then
			Mouse_ui.plist_click = { pane = name, item = item, expire = m.frame + 30 }
			mouse_ui_lock(m, 8)
		end
	end

	-- click lands once the row is highlighted
	local pc = Mouse_ui.plist_click
	if pc ~= nil then
		local pd = data[pc.pane]
		if m.frame > pc.expire then
			Mouse_ui.plist_click = nil
		elseif can_act and data.active_pane == pd and pd.highlighted_item == pc.item then
			Mouse_ui.plist_click = nil
			mouse_ui_inject("select", 0)
		end
	end

	-- wheel scrolls the pane under the cursor, or the active one
	if can_act and m.wheel ~= 0 and mouse_ui_take("wheel_frame", m) then
		if name == nil then
			name = "edit"
			if data.active_pane == data.playlist then
				name = "playlist"
			end
		end
		local n = m.wheel
		if n > 4 then
			n = 4
		elseif n < -4 then
			n = -4
		end
		local pd = data[name]
		if pd.num_items > mui_get("PLAYLIST_MAX_ITEMS") then
			mouse_ui_call("plist_view", name, pd.first_vis_item - n)
			audio_play(Menu_sound_item_nav)
		end
	end
	return item >= 0
end

-- like pause_menu_control_playlist_update_scroll_bar, but with the given first row
function mouse_ui_plist_set_first(lab, data, pd, first)
	local max = mui_get("PLAYLIST_MAX_ITEMS")
	local top = pd.num_items - max
	if top < 0 then
		top = 0
	end
	if first > top then
		first = top
	end
	if first < 0 then
		first = 0
	end
	pd.first_vis_item = first
	pause_menu_playlist_update_labels(lab, data)
	pause_menu_control_playlist_update_arrow(lab, data)
	if top > 0 then
		pause_menu_playlist_set_thumb_pos_scroll_bar(pd, first / top)
	end
end

-- like pause_menu_control_playlist_update_selectbar without re-centring
function mouse_ui_plist_refresh(lab, data)
	local pd = data.active_pane
	local pl = lab.active_pane
	if pd.highlighted_item >= pd.num_items then
		pd.highlighted_item = pd.num_items - 1
	end
	if pd.highlighted_item < 0 then
		pd.highlighted_item = 0
	end
	mouse_ui_plist_set_first(lab, data, pd, pd.first_vis_item)

	local opt = pl.option_labels[pd.highlighted_item - pd.first_vis_item]
	if opt == nil or pd[pd.highlighted_item] == nil then
		return
	end
	local clip = lab.edit_clip_h
	if pd == data.playlist then
		clip = lab.playlist_clip_h
	end
	local cx, cy = vint_get_property(clip, "anchor")
	local ax, ay = vint_get_property(opt.label_h, "anchor")
	vint_set_property(Menu_option_labels.hl_bar, "anchor", ax + cx + 6, ay + cy + 100)
	vint_set_property(vint_object_find("menu_sel_bar_label", Menu_option_labels.hl_bar), "text_tag", pd[pd.highlighted_item].label)
end

function mouse_ui_plist_footer(pd)
	local it = pd[pd.highlighted_item]
	if it ~= nil then
		pause_menu_playlist_update_footer(it.artist_name, it.track_name)
	end
end

-- highlight a row, switching panes if needed
function mouse_ui_plist_set(name, item)
	local lab, data = mouse_ui_plist_get()
	if lab == nil or menu_input_is_blocked() or Menu_transition_in_progress == true then
		return
	end
	local pd = data[name]
	if pd == nil or item < 0 or item >= pd.num_items then
		return
	end
	local switch = data.active_pane ~= pd
	if switch == false and pd.highlighted_item == item then
		return
	end
	pd.highlighted_item = item
	if switch then
		lab.active_pane = lab[name]
		data.active_pane = pd
		Menu_active.btn_tips = pd.btn_tips
		btn_tips_update()
	end
	mouse_ui_plist_refresh(lab, data)
	if switch then
		pause_menu_playlist_resize_select_bar()
		pause_menu_playlist_update_clip_region()
	end
	mouse_ui_plist_footer(pd)
	audio_play(Menu_sound_item_nav)
end

-- scroll a pane, the highlight stays inside it
function mouse_ui_plist_view(name, first)
	local lab, data = mouse_ui_plist_get()
	if lab == nil or menu_input_is_blocked() or Menu_transition_in_progress == true then
		return
	end
	local pd = data[name]
	local max = mui_get("PLAYLIST_MAX_ITEMS")
	if pd == nil or pd.num_items == nil or pd.num_items <= max then
		return
	end
	local top = pd.num_items - max
	if first > top then
		first = top
	end
	if first < 0 then
		first = 0
	end
	if first == pd.first_vis_item then
		return
	end
	local hl = pd.highlighted_item
	if hl < first then
		hl = first
	elseif hl > first + max - 1 then
		hl = first + max - 1
	end
	local moved = hl ~= pd.highlighted_item
	pd.highlighted_item = hl
	pd.first_vis_item = first
	if pd == data.active_pane then
		mouse_ui_plist_refresh(lab, data)
		if moved then
			mouse_ui_plist_footer(pd)
		end
	else
		mouse_ui_plist_set_first(lab, data, pd, first)
	end
end

-- weapon wheel (hud.lua, hold Q)
-- cursor starts on the stick in the middle, hover a weapon / food to pick it,
-- release Q or click to equip, right click closes without changing anything, WASD keeps moving you

function mouse_ui_radial_start()
	local hi = mui_get("Hud_inventory")
	if hi == nil or hi.thread == nil then
		return	-- inventory disabled, nothing opened
	end
	-- Input/MouseWeaponWheel off, or opened with a controller: vanilla wheel
	local wheel_on = mui_get("mouse_ui_wheel_enabled")
	if wheel_on ~= nil and wheel_on() == false then
		return
	end
	local pad = mui_get("mouse_ui_pad")
	if pad ~= nil and pad() == true then
		return
	end
	if Mouse_ui.radial ~= nil and Mouse_ui.radial.open == true then
		return
	end
	Mouse_ui.radial = { open = true, warped = false, mouse = false }
	-- Juiced skips the wheel's stick / d-pad inputs now so WASD still moves you
	local radial = mui_get("mouse_ui_radial")
	if radial ~= nil then
		radial(true)
	end
	mouse_ui_start("radial")
end

function mouse_ui_radial_stop()
	local r = Mouse_ui.radial
	if r == nil or r.open ~= true then
		return
	end
	r.open = false
	local radial = mui_get("mouse_ui_radial")
	if radial ~= nil then
		radial(false)
	end
	mouse_ui_stop("radial")
end

-- the game also turns mouse movement into inventory_x / y, ignore that once the cursor is in use
function mouse_ui_radial_hook_input()
	local orig = mui_get("hud_inventory_input")
	if type(orig) ~= "function" or Mouse_ui_wrapped["hud_inventory_input"] == orig then
		return
	end
	local wrapper = function(event, value)
		local r = Mouse_ui.radial
		if r ~= nil and r.open == true and r.mouse == true and (event == "inventory_x" or event == "inventory_y") then
			return
		end
		return orig(event, value)
	end
	Mouse_ui_wrapped["hud_inventory_input"] = wrapper
	Mouse_ui_G["hud_inventory_input"] = wrapper
end

-- centre and radius of the stick base
function mouse_ui_radial_centre(hr)
	local stick = vint_object_find("control_stick", hr.radial_grp_h)
	local x0, y0, x1, y1 = mouse_ui_bbox(vint_object_find("base", stick))
	if x0 == nil or x1 - x0 < 1 then
		return nil
	end
	return (x0 + x1) * 0.5, (y0 + y1) * 0.5, (x1 - x0) * 0.5
end

-- same angle -> slot table as hud_inventory_process (x right, y up)
function mouse_ui_radial_slot(x, y)
	local mag = sqrt(x * x + y * y)
	local pi = 3.14159
	local radians = acos(x / mag)
	if y < 0 then
		radians = pi + (pi - radians)
	end
	local eighth = pi / 8
	local fourth = pi / 4
	if radians < eighth then
		return 2
	elseif radians < fourth + eighth then
		return 1
	elseif radians < 2 * fourth + eighth then
		return 0
	elseif radians < 3 * fourth + eighth then
		return 7
	elseif radians < 4 * fourth + eighth then
		return 6
	elseif radians < 5 * fourth + eighth then
		return 5
	elseif radians < 6 * fourth + eighth then
		return 4
	elseif radians < 7 * fourth + eighth then
		return 3
	end
	return 2
end

function mouse_ui_radial_handler()
	local hr = mui_get("Hud_radial_menu")
	local hi = mui_get("Hud_inventory")
	local r = Mouse_ui.radial
	if hr == nil or hi == nil or hi.thread == nil or r == nil or r.open ~= true then
		return
	end
	local m = mouse_ui_state()
	local cx, cy, rad = mouse_ui_radial_centre(hr)
	if cx == nil then
		mui_request(true)
		return
	end

	-- ultrawide / 16:10: cvint.dat draws radial_grp shifted, its elements don't know about it
	local ox = 0
	local oy = 0
	local cvint = mui_get("mouse_ui_cvint_offset")
	if cvint ~= nil then
		ox, oy = cvint("hud", "radial_grp")
	end
	local mm = { x = m.x - ox, y = m.y - oy }

	-- cursor to the middle first, the poll after the request moves it
	if r.warped ~= true then
		r.warped = true
		r.wait = 2
		local warp = mui_get("mouse_ui_warp")
		if warp ~= nil then
			warp(cx + ox, cy + oy)
		end
		mui_request(true)
		return
	end
	if r.wait > 0 then
		r.wait = r.wait - 1
		mui_request(true)
		return
	end

	if m.active == true and (m.moved == true or m.lpress == true) then
		r.mouse = true
	end

	if r.mouse == true and m.active == true then
		local dx = mm.x - cx
		local dy = mm.y - cy
		local dist = sqrt(dx * dx + dy * dy)

		-- slots under the cursor (food on the d-pad too), otherwise the direction from the middle
		local pick = -1
		for i = 0, 11 do
			local s = hr.slots[i]
			if s ~= nil and s.grp_h ~= -1 and mouse_ui_hit_visible(s.grp_h, mm, 0) then
				pick = i
				break
			end
		end
		if pick < 0 and dist > rad then
			pick = mouse_ui_radial_slot(dx, -dy)
		end
		if pick >= 0 and pick ~= hr.slot_selected then
			Hud_radial_menu_change_select(pick)
		end

		-- the little stick leans towards the cursor
		local lean = dist / (rad * 3)
		if lean > 1 then
			lean = 1
		end
		if dist > 0.5 then
			vint_set_property(hr.stick_grp_h, "anchor", dx / dist * lean * 12, dy / dist * lean * 12)
		end
		hr.stick_mag = 0	-- keeps the d-pad food events working

		if m.lpress == true then
			-- same as letting go of Q
			mui_get("hud_inventory_input")("inventory", 0)
			return
		end
	end

	if m.rpress == true then
		hud_inventory_hide()
		return
	end
	mui_request(true)
end
Mouse_ui_handlers["radial"] = mouse_ui_radial_handler

-- main menu top list (the game calls main_menu_nav itself, no vint input)

function mouse_ui_main_menu_item_h(i)
	local it = Main_menu_top[i]
	if it == nil then
		return nil
	end
	return it.text_h
end

function mouse_ui_main_menu_handler()
	if mui_get("Main_menu_can_has_input") ~= true or mui_get("Menu_active") ~= 0 then
		return
	end
	local top = mui_get("Main_menu_top")
	if top == nil or top.num_items == nil or top.num_items < 1 or top[0] == nil or top[0].text_h == nil then
		return
	end
	if mouse_ui_visible(top[0].item_h) == false then
		return
	end

	local m = mouse_ui_state()
	if mouse_ui_dialog_open() then
		mui_request(false)
		return
	end

	local can_act = m.active == true and menu_input_is_blocked() == false
	local hit = mouse_ui_pick_list(m, top.num_items, mouse_ui_main_menu_item_h, 40)
	local hover = hit >= 0

	if can_act then
		if hover then
			if hit ~= top.current_selection and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
				mouse_ui_call("main_menu_hover", hit)
			end
			if m.lpress == true then
				mouse_ui_call("main_menu_nav", "select")
				mouse_ui_lock(m, 10)
			end
		end
		if m.wheel ~= 0 and mouse_ui_take("wheel_frame", m) then
			local n = m.wheel
			local ev = "nav_up"
			if n < 0 then
				n = -n
				ev = "nav_down"
			end
			if n > 4 then
				n = 4
			end
			for i = 1, n do
				mouse_ui_call("main_menu_nav", ev)
			end
		end
	end

	mui_request(hover)
end
Mouse_ui_handlers["main_menu"] = mouse_ui_main_menu_handler

-- dialog boxes

function mouse_ui_dialog_handler()
	local d = mui_get("Dialog_box")
	if d == nil or d.input_subs == nil or d.options == nil or d.handles == nil then
		return
	end
	local n = d.options.num_options
	if n == nil or n < 1 or d.cur_idx == nil then
		return
	end
	if mouse_ui_visible(d.handles["dialog"]) == false then
		return
	end

	local m = mouse_ui_state()
	local can_act = m.active == true and Dialog_input_block ~= true
	local hover = false

	-- select bar is on the current option, others are DIALOG_BOX_OPTION_OFFSET apart
	local bx0, by0, bx1, by1 = mouse_ui_bbox(d.handles["sel_bar_w"])
	if bx0 ~= nil then
		local scale = (by1 - by0) / 34
		if scale > 0.01 then
			local right = bx1
			local ex0, ey0, ex1, ey1 = mouse_ui_bbox(d.handles["sel_bar_e"])
			if ex1 ~= nil and ex1 > right then
				right = ex1
			end
			if m.x >= bx0 - 4 and m.x <= right + 4 then
				local cy_cur = (by0 + by1) * 0.5
				local step = DIALOG_BOX_OPTION_OFFSET * scale
				for i = 0, n - 1 do
					local cy = cy_cur + (i - d.cur_idx) * step
					if mui_abs(m.y - cy) <= step * 0.5 then
						hover = true
						if can_act then
							if i ~= d.cur_idx and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
								mouse_ui_dialog_set(i)
							end
							if m.lpress == true then
								mouse_ui_inject("select", 0)
								mouse_ui_lock(m, 10)
							end
						end
						break
					end
				end
			end
		end
	end

	if can_act then
		mouse_ui_wheel(m, "nav_up", "nav_down")
	end
	mui_request(hover)
end
Mouse_ui_handlers["dialog"] = mouse_ui_dialog_handler

-- dialog_box_input nav but straight to an option
function mouse_ui_dialog_set(i)
	local d = Dialog_box
	local old_idx = d.cur_idx
	if old_idx == i or d.options[i] == nil or d.options[old_idx] == nil then
		return
	end
	d.cur_idx = i

	local ax, ay = vint_get_property(d.handles["option"], "anchor")
	ax = vint_get_property(d.handles["select"], "anchor")
	vint_set_property(d.options[old_idx].label_h, "tint", DIALOG_BOX_UNSELECT_TEXT.R, DIALOG_BOX_UNSELECT_TEXT.G, DIALOG_BOX_UNSELECT_TEXT.B)
	vint_set_property(d.handles["select"], "anchor", ax, ay + (i * DIALOG_BOX_OPTION_OFFSET))
	vint_set_property(d.options[i].label_h, "tint", DIALOG_BOX_HIGHLIGHT_TEXT.R, DIALOG_BOX_HIGHLIGHT_TEXT.G, DIALOG_BOX_HIGHLIGHT_TEXT.B)
	dialog_box_set_result(d.handle, i, DIALOG_ACTION_NAVIGATE)
end

-- blackjack

function mouse_ui_blackjack_handler()
	local g = mui_get("Gmb_blackjack")
	if g == nil or g.input_subs == nil or g.handles == nil then
		return
	end
	local m = mouse_ui_state()
	if mouse_ui_dialog_open() then
		mui_request(false)
		return
	end

	local can_act = m.active == true and gmb_blackjack_input_is_bocked() == false
	local hover = false
	local betting = g.input_state == 0

	if betting then
		-- bet: arrows (or left / right half of the bet button) and the wheel change it
		local hd = g.handles
		if mouse_ui_hit_visible(hd.btn_bet_arrow_left, m, 6) then
			hover = true
			if can_act and m.lpress == true then
				mouse_ui_inject("nav_left", 0)
			end
		elseif mouse_ui_hit_visible(hd.btn_bet_arrow_right, m, 6) then
			hover = true
			if can_act and m.lpress == true then
				mouse_ui_inject("nav_right", 0)
			end
		elseif mouse_ui_hit_visible(hd.btn_bet, m, 0) then
			hover = true
			if can_act and m.lpress == true then
				local x0, y0, x1, y1 = mouse_ui_bbox(hd.btn_bet)
				if m.x < (x0 + x1) * 0.5 then
					mouse_ui_inject("nav_left", 0)
				else
					mouse_ui_inject("nav_right", 0)
				end
			end
		end
		if can_act then
			mouse_ui_wheel(m, "nav_right", "nav_left")
		end
	elseif can_act then
		mouse_ui_wheel(m, "nav_up", "nav_down")
	end

	-- Deal / Hit / Stand / Split / Double down / New game / Quit
	if g.btns ~= nil and g.num_btns ~= nil then
		for i = 0, g.num_btns - 1 do
			local b = g.btns[i]
			if b ~= nil and mouse_ui_hit(b.btn_h, m.x, m.y, 0) then
				hover = true
				if can_act then
					if betting == false and i ~= g.cur_idx and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
						gmb_blackjack_btn_highlight(i)
						g.cur_idx = i
						audio_play(GMB_BLACKJACK_AUDIO.NAV)
					end
					if m.lpress == true then
						mouse_ui_inject("select", 0)
						mouse_ui_lock(m, 10)
					end
				end
				break
			end
		end
	end

	if can_act then
		mouse_ui_back(m)
	end
	mui_request(hover)
end
Mouse_ui_handlers["blackjack"] = mouse_ui_blackjack_handler

-- poker

function mouse_ui_poker_handler()
	local p = mui_get("Gmb_poker")
	if p == nil or p.input_subs == nil or p.handles == nil then
		return
	end
	local m = mouse_ui_state()
	if mouse_ui_dialog_open() then
		mui_request(false)
		return
	end

	local can_act = m.active == true and gmb_poker_input_is_bocked() == false
	local hover = false
	local state = p.input_state	-- 0 = bet, 10 = holding, 20 = complete
	local hd = p.handles

	if state == 0 then
		-- bet button: upper half raises, lower half lowers the bet
		if mouse_ui_hit_visible(hd.btn_bet, m, 0) then
			hover = true
			if can_act and m.lpress == true then
				local x0, y0, x1, y1 = mouse_ui_bbox(hd.btn_bet)
				if m.y < (y0 + y1) * 0.5 then
					mouse_ui_inject("nav_up", 0)
				else
					mouse_ui_inject("nav_down", 0)
				end
			end
		end
		if can_act then
			mouse_ui_wheel(m, "nav_up", "nav_down")
		end
	elseif state == 10 then
		-- cards: hover to pick, click to hold / release
		if p.cards ~= nil then
			for i = 0, 4 do
				local card = p.cards[i]
				if card ~= nil and card.grp_h ~= nil and mouse_ui_hit(card.grp_h, m.x, m.y, 0) then
					hover = true
					if can_act then
						if p.cur_idx ~= i + 1 and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
							gmb_poker_hold_nav(i + 1)
							audio_play(GMB_POKER_AUDIO.NAV)
						end
						if m.lpress == true then
							mouse_ui_inject("select", 0)
							mouse_ui_lock(m, 8)
						end
					end
					break
				end
			end
		end
		if can_act then
			mouse_ui_wheel(m, "nav_left", "nav_right")
		end
	end

	-- Deal / Draw / Play again
	if hover == false and mouse_ui_hit_visible(hd.btn_action, m, 0) then
		hover = true
		if can_act then
			if state == 10 and p.cur_idx ~= 0 and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
				gmb_poker_hold_nav(0)
				audio_play(GMB_POKER_AUDIO.NAV)
			end
			if m.lpress == true then
				mouse_ui_inject("select", 0)
				mouse_ui_lock(m, 10)
			end
		end
	end

	if can_act then
		mouse_ui_back(m)
	end
	mui_request(hover)
end
Mouse_ui_handlers["poker"] = mouse_ui_poker_handler

-- phone lists (contacts, homies, garage, cheats)
-- hovering highlights without moving the list, only the wheel and scroll bar scroll it

function mouse_ui_phone_open()
	local c = mui_get("Cellphone")
	return c ~= nil and c.input_subs ~= nil
end

function mouse_ui_cell_alive(it)
	return it ~= nil and it.grp_h ~= nil and it.grp_h ~= -1 and it.grp_h ~= 0
end

function mouse_ui_cell_is_book(ma)
	return ma.menu_type == "phonebook" and ma.handles ~= nil and ma.handles.grp ~= nil and ma.num_items ~= nil and ma.num_items > 0 and ma.cur_idx ~= nil
end

-- view state, rebuilt when the game moved the list itself (keys, new list)
function mouse_ui_cell_state(md)
	local s = Mouse_ui.cell
	local ex, ey = vint_get_property(Cellphone.handles.book_slider_tween, "end_value")
	if s ~= nil and s.md == md and s.grp == md.handles.grp and s.num == md.num_items and s.cur == md.cur_idx then
		if s.slide == nil or ey == nil or mui_abs(ey - s.slide) < 0.5 then
			return s
		end
	end

	s = { md = md, grp = md.handles.grp, num = md.num_items, cur = md.cur_idx }
	-- same numbers as cellphone_phonebook_update_layout
	if md.image_peg == -1 then
		s.tl = 4
		s.bl = 6
		s.disp = 10.5
	else
		s.tl = 2
		s.bl = 5
		s.disp = 7
	end
	s.rows = s.tl + s.bl
	s.scrolls = s.num >= s.disp
	s.maxtop = s.num - s.bl - s.tl
	if s.maxtop < 0 then
		s.maxtop = 0
	end
	s.top = s.cur - s.tl
	if s.top < 0 then
		s.top = 0
	elseif s.top > s.maxtop then
		s.top = s.maxtop
	end
	if s.scrolls then
		s.slide = ey
	end
	-- items that exist around the current one
	s.lo = s.cur
	while s.lo > 0 and mouse_ui_cell_alive(md[s.lo - 1]) do
		s.lo = s.lo - 1
	end
	s.hi = s.cur
	while s.hi < s.num - 1 and mouse_ui_cell_alive(md[s.hi + 1]) do
		s.hi = s.hi + 1
	end
	Mouse_ui.cell = s
	return s
end

function mouse_ui_cell_text_bottom(it)
	if mouse_ui_cell_alive(it) == false then
		return nil
	end
	local x0, y0, x1, y1 = mouse_ui_bbox(vint_object_find("book_item_text", it.grp_h))
	return y1
end

-- create an item next to an existing one and line it up
function mouse_ui_cell_add(md, i, below)
	if mouse_ui_cell_alive(md[i]) then
		return
	end
	cellphone_phonebook_add_to_list(i)
	local h = md[i].grp_h
	if below then
		local p = md[i - 1].grp_h
		local t = vint_object_find("book_item_text", p)
		local tx, ty = vint_get_property(t, "anchor")
		local w, th = element_get_actual_size(t)
		local x, y = vint_get_property(p, "anchor")
		vint_set_property(h, "anchor", x, y + ty + th)
	elseif i > 0 then
		local n = md[i + 1].grp_h
		local w, th = element_get_actual_size(vint_object_find("book_item_text", h))
		local x, y = vint_get_property(n, "anchor")
		vint_set_property(h, "anchor", x, y - th)
	end
end

-- grow / shrink the range of created items one step at a time so each new one has a neighbour
function mouse_ui_cell_range(s, lo, hi)
	local md = s.md
	if lo < 0 then
		lo = 0
	end
	if hi > s.num - 1 then
		hi = s.num - 1
	end
	while s.hi < hi do
		s.hi = s.hi + 1
		mouse_ui_cell_add(md, s.hi, true)
	end
	while s.lo > lo do
		s.lo = s.lo - 1
		mouse_ui_cell_add(md, s.lo, false)
	end
	while s.lo < lo do
		if mouse_ui_cell_alive(md[s.lo]) then
			cellphone_phonebook_remove_from_list(s.lo)
		end
		s.lo = s.lo + 1
	end
	while s.hi > hi do
		if mouse_ui_cell_alive(md[s.hi]) then
			cellphone_phonebook_remove_from_list(s.hi)
		end
		s.hi = s.hi - 1
	end
end

-- what the select / deselect animations end on, set straight away for the mouse
function mouse_ui_cell_item_look(md, grp, selected)
	local text_h = vint_object_find("book_item_text", grp)
	vint_set_property(text_h, "alpha", 1)
	if selected then
		vint_set_property(text_h, "tint", .529, .725, .815)
	else
		vint_set_property(text_h, "tint", 0.1294, 0.1765, 0.204)
	end
	if vint_get_property(text_h, "text_tag") ~= nil then
		if selected then
			cellphone_phonebook_resize_static_text(text_h, 1.2, true)
		else
			cellphone_phonebook_resize_static_text(text_h, 1.0, false)
		end
	end
	if md.image_peg ~= -1 then
		-- homies: the picture opens above the selected name
		local bmp_h = vint_object_find("book_image", grp)
		local w, h = vint_get_property(bmp_h, "source_se")
		local tx, ty = vint_get_property(text_h, "anchor")
		if selected then
			vint_set_property(bmp_h, "source_se", w, 98)
			vint_set_property(bmp_h, "alpha", 1)
			vint_set_property(text_h, "anchor", tx, 98)
		else
			vint_set_property(bmp_h, "source_se", w, 0)
			vint_set_property(bmp_h, "alpha", 0)
			vint_set_property(text_h, "anchor", tx, 0)
		end
	end
end

-- cellphone_phonebook_nav for the mouse: no fade, no scrolling, any distance
function mouse_ui_cell_select(md, idx, old)
	local c = Cellphone
	local hd = c.handles
	c.cur_idx = idx
	md.cur_idx = idx

	-- a fade from the keyboard could still be running on one of these
	vint_set_property(hd.book_text_deselect_anim, "is_paused", true)
	vint_set_property(hd.book_image_select_anim, "is_paused", true)
	vint_set_property(hd.book_image_deselect_anim, "is_paused", true)

	if old ~= idx and mouse_ui_cell_alive(md[old]) then
		mouse_ui_cell_item_look(md, md[old].grp_h, false)
	end
	local grp = md[idx].grp_h
	mouse_ui_cell_item_look(md, grp, true)
	cellphone_phonebook_align()

	vint_set_property(hd.book_item_pulse_anim, "target_handle", grp)
	lua_play_anim(hd.book_item_pulse_anim, 0)
	audio_play(Cellphone_audio["nav"])
end

-- slide the list and move the scroll bar
function mouse_ui_cell_slide(s, top)
	local md = s.md
	local text_h = vint_object_find("book_item_text", md[md.cur_idx].grp_h)
	local w, lh = vint_get_property(text_h, "unscaled_size")
	if lh == nil then
		return
	end
	local y_to = -top * lh
	local box = vint_object_find("book_slider", md.handles.grp)
	local x, y = vint_get_property(box, "anchor")
	local tw = Cellphone.handles.book_slider_tween
	vint_set_property(tw, "start_value", x, y)
	vint_set_property(tw, "end_value", 0, y_to)
	vint_set_property(tw, "target_handle", box)
	lua_play_anim(Cellphone.handles.book_slider_anim, 0)
	s.slide = y_to

	local h = 304 * s.disp / s.num
	local pos = 0
	if s.maxtop > 0 then
		pos = top * (304 - h) / s.maxtop
	end
	element_set_actual_size(md.handles.scrollbar, 8, h)
	vint_set_property(md.handles.scrollbar, "anchor", 0, pos)
end

-- move the view to top, want = item to highlight (nil keeps it, pulled into view)
function mouse_ui_cell_apply(s, top, want)
	local md = s.md
	if top < 0 then
		top = 0
	elseif top > s.maxtop then
		top = s.maxtop
	end
	local old = md.cur_idx
	local cur = old
	if cellphone_nav_is_blocked() == false then
		if want ~= nil then
			cur = want
		end
		local last = top + s.rows - 1
		if last > s.num - 1 then
			last = s.num - 1
		end
		if cur < top then
			cur = top
		elseif cur > last then
			cur = last
		end
	end

	-- keep the view and both highlights (+-10 like the game) created
	local lo = top + s.tl
	local hi = lo
	if cur < lo then
		lo = cur
	end
	if old < lo then
		lo = old
	end
	if cur > hi then
		hi = cur
	end
	if old > hi then
		hi = old
	end
	mouse_ui_cell_range(s, lo - 10, hi + 10)

	if cur ~= old then
		mouse_ui_cell_select(md, cur, old)
	end
	s.cur = cur
	if s.scrolls and top ~= s.top then
		mouse_ui_cell_slide(s, top)
	end
	s.top = top
end

-- screen pixels per phone unit, from an unscaled item's text
-- (the scroll bar bitmap is sized with "scale", which its screen box doesn't include)
function mouse_ui_cell_scale(s)
	local md = s.md
	local fallback = nil
	for i = s.lo, s.hi do
		local it = md[i]
		if mouse_ui_cell_alive(it) then
			local t = vint_object_find("book_item_text", it.grp_h)
			local x0, y0, x1, y1 = mouse_ui_bbox(t)
			local w, uh = vint_get_property(t, "unscaled_size")
			local sx, sy = vint_get_property(t, "scale")
			if x0 ~= nil and uh ~= nil and uh > 1 and y1 - y0 > 1 then
				if sy ~= nil and sy > 0.99 and sy < 1.01 then
					return (y1 - y0) / uh
				end
				if fallback == nil and sy ~= nil and sy > 0.01 then
					fallback = (y1 - y0) / (uh * sy)
				end
			end
		end
	end
	return fallback
end

-- scroll bar thumb / track on screen, nil if there's none
function mouse_ui_cell_track(s)
	if s.scrolls == false then
		return nil
	end
	local sb = s.md.handles.scrollbar
	local x0, y0, x1, y1 = mouse_ui_bbox(sb)
	local sc = mouse_ui_cell_scale(s)
	if x0 == nil or sc == nil then
		return nil
	end
	local h = 304 * s.disp / s.num
	local ax, ay = vint_get_property(sb, "anchor")
	return { x0 = x0, x1 = x0 + 8 * sc, ty0 = y0, ty1 = y0 + h * sc, top = y0 - ay * sc, sc = sc, h = h }
end

function mouse_ui_cell_drag(m, s, tr, can)
	local d = Mouse_ui.cdrag
	local travel = (304 - tr.h) * tr.sc
	if can == false or travel <= 1 then
		return
	end
	local pos = (m.y - d.grab - tr.top) / travel
	if pos < 0 then
		pos = 0
	elseif pos > 1 then
		pos = 1
	end
	local top = floor(pos * s.maxtop + 0.5)
	if top ~= s.top then
		mouse_ui_cell_apply(s, top, nil)
	end
end

function mouse_ui_cell_book(m, c, ma)
	local s = mouse_ui_cell_state(ma)
	local can = m.active == true and c.cur_idx ~= nil
	local blocked = cellphone_nav_is_blocked()
	local tr = mouse_ui_cell_track(s)

	-- scroll bar
	local d = Mouse_ui.cdrag
	if d ~= nil then
		if m.ldown ~= true or tr == nil or d.md ~= ma then
			Mouse_ui.cdrag = nil
		else
			mouse_ui_cell_drag(m, s, tr, can)
			return true
		end
	end
	if tr ~= nil and m.x >= tr.x0 - 6 and m.x <= tr.x1 + 6 and m.y >= tr.top and m.y <= tr.top + 304 * tr.sc then
		if can and m.lpress == true then
			local grab = (tr.ty1 - tr.ty0) * 0.5
			if m.y >= tr.ty0 and m.y <= tr.ty1 then
				grab = m.y - tr.ty0
			end
			Mouse_ui.cdrag = { md = ma, grab = grab }
			mouse_ui_cell_drag(m, s, tr, can)
		end
		return true
	end

	-- items in view
	local hit = -1
	local gx0, gy0, gx1, gy1 = mouse_ui_bbox(ma.handles.grp)
	local vy0 = -100000
	local vy1 = 100000
	if tr ~= nil and s.maxtop > 0 then
		vy0 = tr.top
		vy1 = tr.top + 310 * tr.sc
	end
	if gx0 ~= nil and m.x >= gx0 and m.x <= gx1 and m.y >= vy0 and m.y <= vy1 then
		local last = s.top + s.rows - 1
		if last > s.num - 1 then
			last = s.num - 1
		end
		-- rows go from the previous item's text bottom to this item's text bottom
		-- (the group's bbox includes the hidden picture, so use the text)
		local prev_y1 = nil
		if s.top > 0 then
			prev_y1 = mouse_ui_cell_text_bottom(ma[s.top - 1])
		end
		for i = s.top, last do
			local it = ma[i]
			if mouse_ui_cell_alive(it) then
				local x0, y0, x1, y1 = mouse_ui_bbox(vint_object_find("book_item_text", it.grp_h))
				if y0 ~= nil then
					if prev_y1 ~= nil and prev_y1 < y0 then
						y0 = prev_y1
					elseif i == 0 then
						y0 = -100000
					end
					if m.y >= y0 and m.y <= y1 then
						hit = i
						break
					end
				end
				prev_y1 = y1
			else
				prev_y1 = nil
			end
		end
	end

	if hit >= 0 and can then
		if hit ~= ma.cur_idx and blocked == false and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
			mouse_ui_cell_apply(s, s.top, hit)
		end
		if m.lpress == true and mouse_ui_locked(m) == false then
			Mouse_ui.cell_click = { md = ma, idx = hit, expire = m.frame + 60 }
			mouse_ui_lock(m, 8)
		end
	end

	-- click lands once the phone is done animating to that item
	local pc = Mouse_ui.cell_click
	if pc ~= nil then
		if pc.md ~= ma or m.frame > pc.expire then
			Mouse_ui.cell_click = nil
		elseif cellphone_nav_is_blocked() == false then
			if ma.cur_idx == pc.idx then
				Mouse_ui.cell_click = nil
				mouse_ui_inject("select", 0)
			elseif pc.idx >= s.top and pc.idx <= s.top + s.rows - 1 then
				mouse_ui_cell_apply(s, s.top, pc.idx)
			else
				Mouse_ui.cell_click = nil
			end
		end
	end

	if can and m.wheel ~= 0 and mouse_ui_take("wheel_frame", m) then
		local n = m.wheel
		if n > 4 then
			n = 4
		elseif n < -4 then
			n = -4
		end
		if s.scrolls then
			mouse_ui_cell_apply(s, s.top - n, nil)
		elseif blocked == false then
			if n > 0 then
				mouse_ui_inject("nav_up", 0)
			else
				mouse_ui_inject("nav_down", 0)
			end
		end
	end

	-- highlight got left behind while the phone was busy
	if can and cellphone_nav_is_blocked() == false and (ma.cur_idx < s.top or ma.cur_idx > s.top + s.rows - 1) then
		mouse_ui_cell_apply(s, s.top, nil)
	end
	return hit >= 0
end

-- phone

function mouse_ui_cellphone_handler()
	local c = mui_get("Cellphone")
	if c == nil or c.input_subs == nil or c.menu_active == nil then
		return
	end
	local m = mouse_ui_state()
	if mouse_ui_dialog_open() then
		mui_request(false)
		return
	end

	local can_act = m.active == true and cellphone_nav_is_blocked() == false and c.cur_idx ~= nil
	local hover = false
	local book = false
	local ma = c.menu_active

	if ma == Cellphone_menu_main and ma.num_items ~= nil then
		-- app list
		for i = 0, ma.num_items - 1 do
			local it = ma[i]
			if it ~= nil and it.grp_h ~= nil and it.grp_h ~= 0 and it.grp_h ~= -1 and mouse_ui_hit(it.grp_h, m.x, m.y, 0) then
				hover = true
				if can_act then
					if i ~= c.cur_idx and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
						mouse_ui_call("cell_main", i)
					end
					if m.lpress == true then
						mouse_ui_inject("select", 0)
						mouse_ui_lock(m, 10)
					end
				end
				break
			end
		end
	elseif ma == Cellphone_menu_dial and ma.num_items ~= nil and ma.num_cols ~= nil then
		-- keypad (items 0 and 1 are both the call button)
		for i = 0, ma.num_items - 1 do
			local it = ma[i]
			if i ~= 1 and it ~= nil and it.h ~= nil and mouse_ui_hit(it.h, m.x, m.y, 0) then
				hover = true
				if can_act then
					local cur = ma.cur_idx
					if i ~= cur and (i ~= 0 or cur ~= 1) and mouse_ui_locked(m) == false and (m.moved == true or m.lpress == true) then
						local row = 0
						local col = i
						while col >= ma.num_cols do
							col = col - ma.num_cols
							row = row + 1
						end
						cellphone_dial_update_highlighted(col, row)
						audio_play(Cellphone_audio["nav"])
					end
					if m.lpress == true then
						mouse_ui_inject("select", 0)
						mouse_ui_lock(m, 8)
					end
				end
				break
			end
		end
	elseif mouse_ui_cell_is_book(ma) then
		book = true
		hover = mouse_ui_cell_book(m, c, ma)
	elseif ma.num_items ~= nil then
		-- anything else, only clicking the current entry selects
		local it = ma[c.cur_idx]
		if it ~= nil and it.grp_h ~= nil and it.grp_h ~= 0 and it.grp_h ~= -1 and mouse_ui_hit(it.grp_h, m.x, m.y, 0) then
			hover = true
			if can_act and m.lpress == true then
				mouse_ui_inject("select", 0)
				mouse_ui_lock(m, 8)
			end
		end
	end

	if can_act then
		mouse_ui_back(m)
		if book == false then
			mouse_ui_wheel(m, "nav_up", "nav_down")
		end
	end
	mui_request(hover)
end
Mouse_ui_handlers["cellphone"] = mouse_ui_cellphone_handler
