-------------
-- CREDITS --
-------------
Credits = {}
Credits.data = { }
--[[	[0] = {type = 0, str1="Volition"},
	[1] = {type = 1, str1="Leads"},
	[2] = {type = 3, str2="Producer", str1="Gred Donovan"},
	[3] = {type = 3, str2="Product Technical Director", str1="Nick Lee"},
	[4] = {type = 3, str1="David Absug", str2="Programmer"},
	[5] = {type = 3, str1="Jim Brennan", str2="Programmer"},
	[6] = {type = 3, str1="John Buckley", str2="Programmer"},
	[7] = {type = 3, str1="Aaron Canary", str2="Programmer"},
	[8] = {type = 3, str1="Justin Christofoli", str2="Programmer"},
	[9] = {type = 3, 	str1="Mike Hawkins", str2="Professor"},
	[10] = {type = 1, str1="General"},
	[11] = {type = 3, str1="Jeffrey Harris", str2="The master mind"},
	[12] = {type = 3, str1="", str2="Programmer"},
	[13] = {type = 3, str1="", str2="Programmer", },
	[14] = {type = 3, str1="", str2="Programmer", },
	[15] = {type = 3, str1="Darryl Davis", str2="Programmer"},
	[16] = {type = 3, str1="Darryl Davis", str2="Title 1"},
	[17] = {type = 3, str1="Darryl Davis", str2="Title 1"},
	[18] = {type = 3, str1="Eric Braun", str2="Programmer"},
	[19] = {type = 3, str1="Jon Breuer", str2="Title 1"},
	[20] = {type = 0, str1="Volition"},
	[21] = {type = 1, str1="Leads"},
	[22] = {type = 3, str2="Producer", str1="Gred Donovan"},
	[23] = {type = 3, str2="Product Technical Director", str1="Nick Lee"},
	[24] = {type = 3, str1="David Absug", str2="Programmer"},
	[25] = {type = 3, str1="Jim Brennan", str2="Programmer"},
	[26] = {type = 3, str1="John Buckley", str2="Programmer"},
	[27] = {type = 3, str1="Aaron Canary", str2="Programmer"},
	[28] = {type = 3, str1="Justin Christofoli", str2="Programmer"},
	[29] = {type = 3, str1="Mike Hawkins", str2="Professor"},
	[30] = {type = 1, str1="Music"},
	[31] = {type = 2, str1="This is how we do it - Jesus Christ Superstar", str2="This covers any SR2 specific technical details that need to be documented. Things pertaining to how parts of the"},
	[32] = {type = 2, str1="Dear Maria, Count Me In - All Time Low", str2="Concerts, album reviews, pop, jazz and classical music news from the Los Angeles Times."},
	[33] = {type = 2, str1="Sorry - Buck Cherry", str2="Check out new bands and artists on MySpace Music: Watch music videos, check out concerts and tour dates, music, news and more.", },
	[34] = {type = 2, str1="Stick it in Slowly - DEGC", str2="New This Week: Ashlee Simpson;Plus new ... Alan Light gets Keith Urban's candid take on success, sobriety and music ...", },
	[35] = {type = 2, str1="Doin it from the back - Lucky Pierre", str2="Music is an art form in which the medium is sound. Elements of music are pitch (which governs melody and harmony), rhythm (and its associated concepts tempo ..."},
	[36] = {type = 2, str1="Darryl Davis", str2="Title 1"},
	[37] = {type = 2, str1="Darryl Davis", str2="Title 1"},
	[38] = {type = 2, str1="Eric Braun", str2="Programmer"},
	[39] = {type = 2, str1="Jon Breuer", str2="Title 1"},
} ]]--
--[[
0 = Flag (1 -  Section Title, 2 � Header (i.e., programming), 3 � Role � Name, 4 � Music)
1 = Section Title text, Header Text, or Name Text, or Song Title
2 = Null, Null, Role, Music Description
]]




--General Constants
CREDITS_HALF_SCREEN_HEIGHT = 360
CREDITS_SCREEN_LEEWAY = 250


--Spacing/Size constants
if get_language() == "JP" then
	CREDITS_SPACING_STANDARD = 4
	CREDITS_TITLE_GAP = 30
	CREDITS_EMPTY_GAP = 0
elseif get_language() == "SK" then
	CREDITS_SPACING_STANDARD = 10		
	CREDITS_TITLE_GAP = 25			
	CREDITS_EMPTY_GAP = 4			
else	
	CREDITS_SPACING_STANDARD = 7		--Spacing between credits
	CREDITS_TITLE_GAP = 15				--Title gap
	CREDITS_EMPTY_GAP = 18				--Gap for things like role-name
end	
CREDITS_HEADER_GAP = 15					--Header gap
				

--init text size globals
Credits_header_text_height = -1
Credits_role_text_height = -1
Credits_name_text_height = -1

	
--Movement Constants
CREDITS_PIXELS_PER_SEC_MAX = 200 --2200
CREDITS_PIXELS_PER_SEC_NORMAL = 50 --50
CREDITS_PIXELS_PER_SEC_MIN = 5 --5

--Movement Constants

Credits_thread = -1

function credits_init()
	Credits.handles = {}
	
	--Template Elements
	Credits.handles.credits_base = vint_object_find("credits_base_grp")
	Credits.handles.credits_slide = vint_object_find("credits_slide")
	Credits.handles.header = vint_object_find("credit_header")
	
	--header text
	Credits.handles.header_text = vint_object_find("header", Credits.handles.header)

	--music grp
	Credits.handles.music = vint_object_find("credit_music")
	
	--role name
	Credits.handles.role_name = vint_object_find("credit_role_name")
	Credits.handles.role_text = vint_object_find("role", Credits.handles.name_text)
	Credits.handles.name_text = vint_object_find("name", Credits.handles.name_text)
	
	--Text size globals
	local w, h = element_get_actual_size(Credits.handles.header_text)
	Credits_header_text_height = h

	w, h = element_get_actual_size(Credits.handles.role_text)
	Credits_role_text_height = h
		
	w, h = element_get_actual_size(Credits.handles.name_text)
	Credits_name_text_height = h
	
	--logo img placeholder
	Credits.handles.logo = vint_object_find("credit_logo")
	
	vint_set_property(Credits.handles.role_name, "visible", false)
	vint_set_property(Credits.handles.music, "visible", false)
	vint_set_property(Credits.handles.header, "visible", false)
	vint_set_property(Credits.handles.logo, "visible", false)
	
--	vint_set_property(Credits.handles.credits_base, "alpha", 0)
	Credits.handles.bg = vint_object_find("bg", Credits.handles.credits_base)
	Credits.handles.lower_bitmap = vint_object_find("lower_bitmap", Credits.handles.credits_base)
	Credits.handles.upper_bitmap = vint_object_find("upper_bitmap", Credits.handles.credits_base)
	vint_set_property(Credits.handles.bg, "alpha", 0)
	vint_set_property(Credits.handles.lower_bitmap, "alpha", 0)
	vint_set_property(Credits.handles.upper_bitmap, "alpha", 0)
	

	vint_set_property(Credits.handles.credits_slide, "alpha", 0)
	
	--Animations
	Credits.anims = {}
	Credits.anims.fade_in = vint_object_find("fade_in_anim")
	Credits.anims.fade_in_bg = vint_object_find("fade_in_bg_anim")
	Credits.anims.fade_out = vint_object_find("fade_out_anim")
	Credits.anims.fade_out_twn = vint_object_find("fade_out_twn", Credits.anims.fade_out)
	
	vint_set_property(Credits.anims.fade_in, "is_paused", true)
	vint_set_property(Credits.anims.fade_in_bg, "is_paused", true)
	vint_set_property(Credits.anims.fade_out, "is_paused", true)
	
	Credits.cur_idx = -1				--initialize current idx
	Credits.oldest_idx = 0
	Credits.cur_slide_pos = 365	
	Credits.num_items = 0
	Credits.scroll_speed = CREDITS_PIXELS_PER_SEC_NORMAL
	Credits.skip_available  = true
	
	--Load peg
	Credits.peg_data = "ui_credits"
	peg_load(Credits.peg_data)
	peg_load("ui_juiced_credits")
	
	--Download all credits
	vint_dataresponder_request("credits_grab_credits", "credits_populate", 0)
	credits_populate_logos()
	Credits_thread = thread_new("credits_process")
	
	--Fade in animation
	lua_play_anim(Credits.anims.fade_in, 0)
	
	--Is the main menu loaded? if so do not delay background fade in.
	if vint_document_find("main_menu") == 0 then
		lua_play_anim(Credits.anims.fade_in_bg, 9)
	else
		--We are in main menu, do not delay bg fade in and speed up animation
		lua_play_anim(Credits.anims.fade_in_bg, 0)
		vint_set_property(vint_object_find("bg_alpha_twn_1", Credits.anims.fade_in_bg), "duration", .9)
		vint_set_property(vint_object_find("lower_bitmap_alpha_twn_1", Credits.anims.fade_in_bg), "duration", .9)
		vint_set_property(vint_object_find("upper_bitmap_alpha_twn_1", Credits.anims.fade_in_bg), "duration", .9)
		
	end
	
	
	credits_input_subscribe()
end

function credits_cleanup()
	if Credits.peg_data ~= nil then
		peg_unload(Credits.peg_data)
	end
	credits_input_unsubscribe()
end


function credits_populate(item_type, str1, str2)
	Credits.data[Credits.num_items] = { type = item_type, str1 = str1, str2 = str2, y = -1}
	Credits.num_items = Credits.num_items + 1
end

function credits_populate_logos()
	--Add logos to credits
	Credits.data[Credits.num_items] = { type = 4, logo_bmp = "ui_credits_chris_allen", y = -1}
	Credits.num_items = Credits.num_items + 1
	Credits.data[Credits.num_items] = { type = 4, logo_bmp = "ui_credits_logo_volition", y = -1}
	Credits.num_items = Credits.num_items + 1
	Credits.data[Credits.num_items] = { type = 4, logo_bmp = "ui_credits_logo_thq", y = -1}
	Credits.num_items = Credits.num_items + 1
	Credits.data[0] = { type = 4, logo_bmp = "ui_credits_mike_watson", y = -1}
	Credits.data[27] = { type = 4, logo_bmp = "ui_credits_logo_kobraworks", y = -1}
end

function credits_process()
	
	local move_amount = 0
	
	local prev_item, new_item, credit_h, credit_height, h, item_width, item_height, prev_item_height, prev_item_y, item_y
	local create_new_item
	local credits_play = true
	while credits_play do

			
		--move credits
		move_amount = Credits.scroll_speed * get_frame_time_real()
		
		--Slide Credits up
		Credits.cur_slide_pos = Credits.cur_slide_pos - move_amount
		vint_set_property(Credits.handles.credits_slide, "anchor", 0, Credits.cur_slide_pos)
	
		--Is the first top item off the screen?	
		if Credits.cur_idx < 0 then
			--Initialize
			create_new_item = true
		else
			--Remove first item?
			local actual_slide_pos = Credits.cur_slide_pos + CREDITS_HALF_SCREEN_HEIGHT 
			
			if Credits.data[Credits.oldest_idx] ~= nil then
							
				if actual_slide_pos <  (Credits.data[Credits.oldest_idx].y + CREDITS_SCREEN_LEEWAY) * -1 then
				
					--Lets kill it
					if Credits.data[Credits.oldest_idx].credit_h ~= nil then
						vint_object_destroy(Credits.data[Credits.oldest_idx].credit_h)
					end
					
					Credits.data[Credits.oldest_idx] = nil
					Credits.oldest_idx = Credits.oldest_idx + 1
					
					--Are credits complete?
					if Credits.num_items == Credits.oldest_idx then
						--Credits complete
						credits_complete()
						credits_play = false
					end
				end
			end	
			if Credits.data[Credits.cur_idx] ~= nil then		
				--Build next item
				local next_item_build_at = (Credits.data[Credits.cur_idx].y + Credits.data[Credits.cur_idx].height) * -1
				actual_slide_pos = Credits.cur_slide_pos - CREDITS_HALF_SCREEN_HEIGHT 
				if actual_slide_pos < next_item_build_at + CREDITS_SCREEN_LEEWAY then
					create_new_item = true 
				end
			end
		end
	
		--Should I create an item on the bottom?
		if create_new_item == true then
			--What is the new item going to be?
			Credits.cur_idx = Credits.cur_idx + 1
			new_item = Credits.data[Credits.cur_idx]
			
			if new_item ~= nil and Credits.cur_idx < Credits.num_items then
				--Ok so there is another item to physically build, lets build it...
				
				--What type is it, do special processing and build item
				if new_item.type == 0 then
					--Header
					credit_h = vint_object_clone(Credits.handles.header)
					h = vint_object_find("header", credit_h)
					vint_set_property(h, "text_tag", new_item.str1)
					vint_set_property(h, "tint", .89, .74, .05) 
					local text_x, text_y = vint_get_property(h, "anchor")
					local text_width, text_height = element_get_actual_size(h)
					vint_set_property(h, "anchor", 0, text_y + 40)
					vint_set_property(h, "text_scale", .65, .65)
					credit_height = text_height + CREDITS_HEADER_GAP + 40
				elseif new_item.type == 1 then
					--Title
					credit_h = vint_object_clone(Credits.handles.header)
					h = vint_object_find("header", credit_h)
					vint_set_property(h, "text_tag", new_item.str1)
					local text_x, text_y = vint_get_property(h, "anchor")
					local text_width, text_height = element_get_actual_size(h)
					vint_set_property(h, "anchor", 0, text_y + 10)
					credit_height = text_height + CREDITS_TITLE_GAP + 10
					
				elseif new_item.type == 3 then
					--Music
					credit_h = vint_object_clone(Credits.handles.music)
					--first text
					h = vint_object_find("song_name", credit_h)
					
					vint_set_property(h, "text_tag", new_item.str1)
					
					local text_x, text_y = vint_get_property(h, "anchor")
					local text_width_a, text_height_a = element_get_actual_size(h)
					vint_set_property(h, "anchor", 0, text_y )
					
					--second text
					h = vint_object_find("description", credit_h)
					vint_set_property(h, "text_tag", new_item.str2)
					
					local text_x, text_y = vint_get_property(h, "anchor")
					local text_width_b, text_height_b = element_get_actual_size(h)
					vint_set_property(h, "anchor", 0, text_y)

					if get_language() == "JP" then
						if new_item.str2 ~= "" then
							text_height_b = text_height_b + 10
						end
					elseif get_language() == "SK" then
						if new_item.str2 ~= "" then
							text_height_b = text_height_b + 12
						end
					else
						if new_item.str2 ~= "" then
							text_height_b = text_height_b + 14
						end					
					end
					
					--Special height calculation for wrapping text
					item_width, item_height = element_get_actual_size(h)
					credit_height = text_height_a + text_height_b

				elseif new_item.type == 2 then
					--Role\Name
					credit_h = vint_object_clone(Credits.handles.role_name)
					
					--Find names
					local name_h = vint_object_find("name", credit_h)
					local role_h = vint_object_find("role", credit_h)
					
					--Change Tags
					vint_set_property(name_h, "text_tag", new_item.str1)
					vint_set_property(role_h, "text_tag", new_item.str2)
					
					--Get Sizes
					local text_width1, name_height = element_get_actual_size(name_h)		
					local role_width, role_height = element_get_actual_size(role_h)
					
					local text_height = Credits_name_text_height + 3
					
					--init the text orientation
					vint_set_property(name_h, "auto_offset", "nw")
					vint_set_property(role_h, "auto_offset", "ne")
					vint_set_property(name_h, "anchor", 5, 3)
					vint_set_property(role_h, "anchor", role_width - 10, 6)

					if get_language() == "JP" then
						--adjust the roles for JP
						if role_height > Credits_role_text_height then
							vint_set_property(name_h, "auto_offset", "w")
							vint_set_property(role_h, "auto_offset", "e")
							vint_set_property(name_h, "anchor", 5, 29)
							vint_set_property(role_h, "anchor", role_width - 10, 29)
							text_height = role_height + 12
						else
							text_height = name_height
						end
						--is the name 2 lines tall?
						if name_height > Credits_name_text_height then
							text_height = name_height * 2
						end	
					elseif get_language() == "SK" then
						--adjust the roles for SK
						if role_height > Credits_role_text_height then
							vint_set_property(name_h, "auto_offset", "w")
							vint_set_property(role_h, "auto_offset", "e")
							vint_set_property(name_h, "anchor", 5, 27)
							vint_set_property(role_h, "anchor", role_width - 10, 27)
							text_height = role_height + 10
						else
							text_height = name_height
						end
						--is the name 2 lines tall?
						if name_height > Credits_name_text_height then
							text_height = name_height * 2
						end	
					else
				
						--adjust the roles for US
						if role_height > Credits_role_text_height then
							vint_set_property(name_h, "auto_offset", "w")
							vint_set_property(role_h, "auto_offset", "e")
							vint_set_property(name_h, "anchor", 5, 29)
							vint_set_property(role_h, "anchor", role_width - 10, 29)
							text_height = role_height + 6
						else
							text_height = name_height
						end
						--is the name 2 lines tall?
						if name_height > Credits_name_text_height then
							text_height = name_height * 2
						end
					end
					credit_height = text_height
				elseif new_item.type == 4 then
					--Logo
					credit_h = vint_object_clone(Credits.handles.logo)
					h = vint_object_find("logo_bmp", credit_h)
					vint_set_property(h, "image", new_item.logo_bmp)
					local logo_width, logo_height = element_get_actual_size(h)
					credit_height = logo_height + 40
				end
				
				--Reassign parent
				vint_object_set_parent(credit_h, Credits.handles.credits_slide)
				vint_set_property(credit_h, "visible", true)
				
				--Calculate y from previous item data
				prev_item = Credits.data[Credits.cur_idx - 1]
				
				if prev_item == nil then
					item_y = 0
				else
					if new_item.str1 == "" then
						credit_height = CREDITS_EMPTY_GAP
					end
					item_y = prev_item.y + prev_item.height + CREDITS_SPACING_STANDARD
					
					--but if this is japanese, we need more space
					if get_language() == "JP" then
						item_y = item_y + 10 
					end
				end
				

				
				--Move object
				vint_set_property(credit_h,"anchor", 0, item_y)
				
				--Store data
				new_item.credit_h = credit_h
				new_item.height = credit_height 
				new_item.y = item_y 
				
			end
			
			--Do not create the next item automatically
			create_new_item = false
		end
		thread_yield()
	end
end

function credits_complete()
	--Unsubscribe from controls
	credits_input_unsubscribe()
	
	--Tell game that credits are done rolling
	credits_are_finished()
end

function credits_input_subscribe()
		--	Subscribe to the input
	Credits.input_subs = {	
		y = vint_subscribe_to_raw_input("left_joy_y",			"credits_input_stick", -100),
		up = vint_subscribe_to_raw_input("inventory_up",		"credits_input_stick", -100),
		down = vint_subscribe_to_raw_input("inventory_down",		"credits_input_stick", -100),
		select = vint_subscribe_to_input_event(nil, "select",		"credits_input_btn", -100),
		back = vint_subscribe_to_input_event(nil, "back",		"credits_input_btn", -100),
		skip = vint_subscribe_to_input_event(nil, "exit",		"credits_input_btn", -100),
		all = vint_subscribe_to_input_event(nil, "all_unassigned",	"credits_input_btn", -100),
	}
end

function credits_input_unsubscribe()
	vint_unsubscribe_to_raw_input(Credits.input_subs.y)
	vint_unsubscribe_to_raw_input(Credits.input_subs.up)
	vint_unsubscribe_to_raw_input(Credits.input_subs.down)
	vint_unsubscribe_to_input_event(Credits.input_subs.select)
	vint_unsubscribe_to_input_event(Credits.input_subs.back)
	vint_unsubscribe_to_input_event(Credits.input_subs.skip)
	vint_unsubscribe_to_input_event(Credits.input_subs.all)
end

function credits_input_stick(event, value)
	if event == "left_joy_y" or event == "inventory_up" or event == "inventory_down" then
		--Adjust credit speed
		if  event == "inventory_up" and value ~= 0 then value = 1 end
		if  event == "inventory_down" and value ~= 0 then value = -1 end
		if value < 0 then
			Credits.scroll_speed = CREDITS_PIXELS_PER_SEC_NORMAL + (CREDITS_PIXELS_PER_SEC_MIN - CREDITS_PIXELS_PER_SEC_NORMAL ) * value * -1 
			
		elseif value > 0 then
		
			Credits.scroll_speed = 	CREDITS_PIXELS_PER_SEC_NORMAL + (CREDITS_PIXELS_PER_SEC_MAX - CREDITS_PIXELS_PER_SEC_NORMAL) * value 
		else
			Credits.scroll_speed = CREDITS_PIXELS_PER_SEC_NORMAL 
		end
	end
end

function credits_input_btn(target, event, accelleration)
	
	if Credits.skip_available == true then
		if event == "exit" then
			credits_were_skipped()
		elseif credits_in_cutscene() == false then
			credits_complete()
		end
	end
end

function credits_force_closed()
	if credits_in_cutscene() == true then		
		credits_were_skipped()
	else 	
		credits_complete()
	end
end
