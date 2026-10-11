-- mouse_ui.lua (Clippy95)
-- Mouse support for vint menus, loaded by Juiced (Player/MouseUI.cpp).
-- Juiced appends mouse_ui_attach() to menu_base, main_menu, dialog_box, gmb_* and cellphone.

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

Mouse_ui_kinds = { "menu", "main_menu", "dialog", "blackjack", "poker", "cellphone" }
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
	for i = 1, 6 do
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
	mouse_ui_stop("menu")
end

-- menu_close() animates out before releasing input, don't wait for it
function mouse_ui_menu_closing()
	Mouse_ui.drag = nil
	Mouse_ui.sdrag = nil
	Mouse_ui.menu_closing = true
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
	end
end

-- menu_base (pause menu, sub menus, stores, garage, cribs...)

function mouse_ui_menu_handler()
	local m = mouse_ui_state()
	local active = mui_get("Menu_active")
	if active == nil or active == 0 or Mouse_ui.menu_closing == true then
		Mouse_ui.drag = nil
		Mouse_ui.sdrag = nil
		return
	end
	if mouse_ui_dialog_open() then
		Mouse_ui.drag = nil
		mui_request(false)
		return
	end

	local can_act = m.active == true and menu_input_is_blocked() == false and Menu_transition_in_progress ~= true
	local hover = false

	if mouse_ui_menu_slider_drag(m) or mouse_ui_menu_scroll_drag(m, can_act) then
		hover = true
	else
		if mouse_ui_menu_horz(m, can_act) then
			hover = true
		end
		if Menu_active.custom_show ~= true and Menu_option_labels ~= nil and mouse_ui_visible(Menu_option_labels.frame) then
			if mouse_ui_menu_scroll_bar(m, can_act) then
				hover = true
			elseif mouse_ui_menu_rows(m, can_act) then
				hover = true
			end
		end
		if can_act then
			mouse_ui_back(m)
			mouse_ui_wheel(m, "nav_up", "nav_down")
		end
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
		if m.lpress == true and Menu_active == menu and menu.highlighted_item == item then
			mouse_ui_menu_click(m, r, item, data)
		end
	end
	return true
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

	if control ~= nil and control.grp_h ~= nil and (t == MENU_ITEM_TYPE_NUM_SLIDER or t == MENU_ITEM_TYPE_TEXT_SLIDER or t == MENU_ITEM_TYPE_PALETTE) then
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
function mouse_ui_scroll_target()
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
	local sd = Menu_option_labels.scroll_bar
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
			Mouse_ui.sdrag = { grab = grab, top = ty0 - pos * travel, travel = travel, sound_frame = 0 }
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
	local kind, total, vis, first = mouse_ui_scroll_target()
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
		mouse_ui_call("menu_scroll", target)
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
	elseif ma.num_items ~= nil then
		-- lists scroll themselves, only clicking the current entry selects
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
		mouse_ui_wheel(m, "nav_up", "nav_down")
	end
	mui_request(hover)
end
Mouse_ui_handlers["cellphone"] = mouse_ui_cellphone_handler
