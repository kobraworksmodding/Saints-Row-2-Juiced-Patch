Hud_btnmash = {	

	handles = {},
	shock_thread = -1
}


function hud_btnmash_init()
	--get the language
	Hud_btnmash.handles.language = get_language()
	
	local language = Hud_btnmash.handles.language
	
	--get the platform
	Hud_btnmash.handles.platform = get_platform()
		
	local platform = Hud_btnmash.handles.platform
	
	
	if platform == "XBOX360" then
		Hud_btnmash.handles.platform = "xbox2"
	elseif platform == "PS3" then
		Hud_btnmash.handles.platform = "ps3"
	elseif platform == "PC" then
		Hud_btnmash.handles.platform = "pc"
	end
	
	
	local SDL_type = vint_get_avg_processing_time("INPUT_PROMPT")
	
	if SDL_type == 1 then
	Hud_btnmash.handles.platform = "sdk"
	elseif SDL_type == 2 then
	Hud_btnmash.handles.platform = "xsx"
	elseif SDL_type == 3 then
	Hud_btnmash.handles.platform = "nx"
	elseif SDL_type == 4 then
	Hud_btnmash.handles.platform = "ps5"
	end
	
	platform = Hud_btnmash.handles.platform	
	
	
	--[[
	***********************
	ATTENTION MASS MEDIA
	SIX AXIS DETECTION HERE
	***********************
	]]
	--write code to detect sixaxis option and make this variable true or false
	Hud_btnmash.handles.sixaxis_h = false
	
	
	
	
	--Load Pegs
	peg_load("ui_btnmash_" .. platform)
	peg_load("ui_btnmash_t_" .. language )
	
	

	--meter
	Hud_btnmash.handles.meter_grp_h = vint_object_find("meter_grp")
	local h = Hud_btnmash.handles.meter_grp_h
	Hud_btnmash.handles.meter_fill_1_h = vint_object_find("fill_1", h)
	Hud_btnmash.handles.meter_fill_2_h = vint_object_find("fill_2", h)
	Hud_btnmash.handles.meter_border_h = vint_object_find("border", h)
	Hud_btnmash.handles.meter_grit_h = vint_object_find("grit", h)
	vint_set_property(Hud_btnmash.handles.meter_fill_2_h, "alpha", 0)
	vint_set_property(h, "visible", false)
	
	--status word
	Hud_btnmash.handles.status_grp_h = vint_object_find("status_grp")
	h = Hud_btnmash.handles.status_grp_h
	Hud_btnmash.handles.status_1_h = vint_object_find("status_1", h)
	Hud_btnmash.handles.status_2_h = vint_object_find("status_2", h)
	vint_set_property(h, "visible", false)

	
	--triggers left
	Hud_btnmash.handles.trigger_l_grp_h = vint_object_find("trigger_l_grp")
	h = Hud_btnmash.handles.trigger_l_grp_h
	Hud_btnmash.handles.trigger_l_dn_h = vint_object_find("trg_l_dn", h)
	Hud_btnmash.handles.trigger_l_up_h = vint_object_find("trg_l_up", h)
	Hud_btnmash.handles.trigger_l2_up_h = vint_object_find("trg_l2_up", h)
	Hud_btnmash.handles.trigger_l_lbl_h = vint_object_find("trg_l_lbl", h)
	vint_set_property(h, "visible", false)
	
	--triggers right
	Hud_btnmash.handles.trigger_r_grp_h = vint_object_find("trigger_r_grp")
	h = Hud_btnmash.handles.trigger_r_grp_h
	Hud_btnmash.handles.trigger_r_dn_h = vint_object_find("trg_r_dn", h)
	Hud_btnmash.handles.trigger_r_up_h = vint_object_find("trg_r_up", h)
	Hud_btnmash.handles.trigger_r2_up_h = vint_object_find("trg_r2_up", h)
	Hud_btnmash.handles.trigger_r_lbl_h = vint_object_find("trg_r_lbl", h)
	vint_set_property(h, "visible", false)
	
	--sticks
	Hud_btnmash.handles.sticks_grp_h = vint_object_find("sticks_grp")
	h = Hud_btnmash.handles.sticks_grp_h
	Hud_btnmash.handles.sticks_l_base_h = vint_object_find("st_l_base", h)
	Hud_btnmash.handles.sticks_l_pad_h = vint_object_find("st_l_pad", h)
	Hud_btnmash.handles.sticks_l_lbl_h = vint_object_find("st_l_lbl", h)
	Hud_btnmash.handles.sticks_r_base_h = vint_object_find("st_r_base", h)
	Hud_btnmash.handles.sticks_r_pad_h = vint_object_find("st_r_pad", h)
	Hud_btnmash.handles.sticks_r_lbl_h = vint_object_find("st_r_lbl", h)
	vint_set_property(h, "visible", false)
	
	--giant buttons 1
	Hud_btnmash.handles.giant_buttons_grp_1_h = vint_object_find("giant_buttons_grp_1")
	h = Hud_btnmash.handles.giant_buttons_grp_1_h
	Hud_btnmash.handles.btn_dn_1_h = vint_object_find("btn_1_dn", h)
	Hud_btnmash.handles.btn_up_1_h = vint_object_find("btn_1_up", h)
	vint_set_property(h, "visible", false)
	
	--giant buttons 2
	Hud_btnmash.handles.giant_buttons_grp_2_h = vint_object_find("giant_buttons_grp_2")
	h = Hud_btnmash.handles.giant_buttons_grp_2_h
	Hud_btnmash.handles.btn_dn_2_h = vint_object_find("btn_2_dn", h)
	Hud_btnmash.handles.btn_up_2_h = vint_object_find("btn_2_up", h)
	vint_set_property(h, "visible", false)
	
	--sixaxis fb
	Hud_btnmash.handles.sixaxis_fb_h = vint_object_find("sixaxis_fb")
	h = Hud_btnmash.handles.sixaxis_fb_h
	Hud_btnmash.handles.sixaxis_fb_back_h = vint_object_find("sixaxis_fb_back", h)
	Hud_btnmash.handles.sixaxis_fb_fwd_h = vint_object_find("sixaxis_fb_fwd", h)
	Hud_btnmash.handles.sixaxis_fb_center_h = vint_object_find("sixaxis_fb_center", h)
	vint_set_property(h, "visible", false)
	
	--sixaxis lr
	Hud_btnmash.handles.sixaxis_lr_h = vint_object_find("sixaxis_lr")
	h = Hud_btnmash.handles.sixaxis_lr_h
	Hud_btnmash.handles.sixaxis_lr_left_h = vint_object_find("sixaxis_lr_left", h)
	Hud_btnmash.handles.sixaxis_lr_right_h = vint_object_find("sixaxis_lr_right", h)
	Hud_btnmash.handles.sixaxis_lr_center_h = vint_object_find("sixaxis_lr_center", h)
	vint_set_property(h, "visible", false)
	
	--btnmash_grp
	Hud_btnmash.handles.btn_mash_h = vint_object_find("btn_mash")
	vint_set_property(Hud_btnmash.handles.btn_mash_h, "alpha", 0)
	
	
	--if not ps3, don't load the graphics
	if Hud_btnmash.handles.platform == "xbox2" then
		vint_set_property(Hud_btnmash.handles.sixaxis_lr_left_h, "image", "")
		vint_set_property(Hud_btnmash.handles.sixaxis_lr_right_h, "image", "")
		vint_set_property(Hud_btnmash.handles.sixaxis_lr_center_h, "image", "")
		vint_set_property(Hud_btnmash.handles.sixaxis_fb_back_h, "image", "")
		vint_set_property(Hud_btnmash.handles.sixaxis_fb_fwd_h, "image", "")
		vint_set_property(Hud_btnmash.handles.sixaxis_fb_center_h, "image", "")
	end
	
	--load the meter graphics
	--[[
	vint_set_property(Hud_btnmash.handles.meter_border_h, "image", "ui_btnmash_meter_border_" .. platform  )
	vint_set_property(Hud_btnmash.handles.meter_fill_1_h, "image", "ui_btnmash_meter_fill_" .. platform  )
	vint_set_property(Hud_btnmash.handles.meter_fill_2_h, "image", "ui_btnmash_meter_fill_" .. platform  )
	vint_set_property(Hud_btnmash.handles.meter_grit_h, "image", "ui_btnmash_meter_grit_" .. platform  )
	]]
	--Load platform specific images
	vint_set_property(Hud_btnmash.handles.btn_dn_1_h, "image", "ui_btnmash_b1_dn_" .. platform )
	vint_set_property(Hud_btnmash.handles.btn_up_1_h, "image", "ui_btnmash_b1_up_" .. platform )
	vint_set_property(Hud_btnmash.handles.btn_dn_2_h, "image", "ui_btnmash_b2_dn_" .. platform  )
	vint_set_property(Hud_btnmash.handles.btn_up_2_h, "image", "ui_btnmash_b2_up_" .. platform  )
	vint_set_property(Hud_btnmash.handles.sticks_l_base_h, "image", "ui_btnmash_base_" .. platform  )
	vint_set_property(Hud_btnmash.handles.sticks_l_pad_h, "image", "ui_btnmash_stick_" .. platform  )
	if platform == "pc" then
		vint_set_property(Hud_btnmash.handles.sticks_r_base_h, "image", "ui_btnmash_base_" .. platform )
		vint_set_property(Hud_btnmash.handles.sticks_r_pad_h, "image", "ui_btnmash_base_" .. platform  )
	else
		vint_set_property(Hud_btnmash.handles.sticks_r_base_h, "image", "ui_btnmash_base_" .. platform )
		vint_set_property(Hud_btnmash.handles.sticks_r_pad_h, "image", "ui_btnmash_stick_" .. platform  )
	end
	vint_set_property(Hud_btnmash.handles.trigger_l_dn_h, "image", "ui_btnmash_tr_dn_" .. platform  )
	vint_set_property(Hud_btnmash.handles.trigger_l_up_h, "image", "ui_btnmash_tr_up_" .. platform  )

	vint_set_property(Hud_btnmash.handles.trigger_r_dn_h, "image", "ui_btnmash_tr_dn_" .. platform  )
	vint_set_property(Hud_btnmash.handles.trigger_r_up_h, "image", "ui_btnmash_tr_up_" .. platform  )
	
	vint_set_property(Hud_btnmash.handles.trigger_r_dn_h, "scale", -1, 1 )
	vint_set_property(Hud_btnmash.handles.trigger_r_up_h, "scale", -1, 1 )	

	
	
	--LABELS
	
	if platform == "ps3" then
		vint_set_property(Hud_btnmash.handles.trigger_l2_up_h, "visible", true)
		vint_set_property(Hud_btnmash.handles.trigger_r2_up_h, "visible", true)
		end
	
	if platform == "ps3" or platform == "ps5" or platform == "sdk" then
		vint_set_property(Hud_btnmash.handles.trigger_l_lbl_h, "text_tag", "L2"  )
		vint_set_property(Hud_btnmash.handles.trigger_r_lbl_h, "text_tag", "R2"  )
		vint_set_property(Hud_btnmash.handles.sticks_l_lbl_h, "text_tag", "L"  )
		vint_set_property(Hud_btnmash.handles.sticks_r_lbl_h, "text_tag", "R"  )
		vint_set_property(Hud_btnmash.handles.trigger_l2_up_h, "visible", false)
		vint_set_property(Hud_btnmash.handles.trigger_r2_up_h, "visible", false)
	elseif platform == "pc" then
		vint_set_property(Hud_btnmash.handles.trigger_l_lbl_h, "text_tag", ""  )
		vint_set_property(Hud_btnmash.handles.trigger_r_lbl_h, "text_tag", ""  )
		vint_set_property(Hud_btnmash.handles.sticks_l_lbl_h, "text_tag", ""  )
		vint_set_property(Hud_btnmash.handles.sticks_r_lbl_h, "text_tag", ""  )
		vint_set_property(Hud_btnmash.handles.trigger_l2_up_h, "visible", false)
		vint_set_property(Hud_btnmash.handles.trigger_r2_up_h, "visible", false)
	else
		vint_set_property(Hud_btnmash.handles.trigger_l_lbl_h, "text_tag", "LT"  )
		vint_set_property(Hud_btnmash.handles.trigger_r_lbl_h, "text_tag", "RT"  )
		vint_set_property(Hud_btnmash.handles.sticks_l_lbl_h, "text_tag", "LS"  )
		vint_set_property(Hud_btnmash.handles.sticks_r_lbl_h, "text_tag", "RS"  )
		vint_set_property(Hud_btnmash.handles.trigger_l2_up_h, "visible", false)
		vint_set_property(Hud_btnmash.handles.trigger_r2_up_h, "visible", false)
	end
	


	
	
	
	--animations and the loops
	local twn_h = 0
	
		--regular controls
		Hud_btnmash.handles.lt_rt_alt_h = vint_object_find("lt_rt_alt")			--Find animation
		twn_h = vint_object_find("alt_lt_2_dn", Hud_btnmash.handles.lt_rt_alt_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "hud_btnmash_lt_rt_alt_loop")	--Set end event callback
		vint_set_property(Hud_btnmash.handles.lt_rt_alt_h , "is_paused", true)
		
		Hud_btnmash.handles.lt_rt_tog_h = vint_object_find("lt_rt_tog")			--Find animation
		twn_h = vint_object_find("tog_lt_2_dn", Hud_btnmash.handles.lt_rt_tog_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "hud_btnmash_lt_rt_tog_loop")							--Set end event callback
		vint_set_property(Hud_btnmash.handles.lt_rt_tog_h, "is_paused", true)
		
		Hud_btnmash.handles.lt_only_h = vint_object_find("lt_only")			--Find animation
		local twn_h = vint_object_find("lt_2_dn", Hud_btnmash.handles.lt_only_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "hud_btnmash_lt_only_loop")
		vint_set_property(Hud_btnmash.handles.lt_only_h, "is_paused", true)
		
		Hud_btnmash.handles.rt_only_h = vint_object_find("rt_only")			--Find animation
		twn_h = vint_object_find("rt_2_dn", Hud_btnmash.handles.rt_only_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "hud_btnmash_rt_only_loop")		
		vint_set_property(Hud_btnmash.handles.rt_only_h, "is_paused", true)
		
		Hud_btnmash.handles.giant_btn_1_anim_h = vint_object_find("giant_btn_1_anim")			--Find animation
		twn_h = vint_object_find("btn_1_2_dn", Hud_btnmash.handles.giant_btn_1_anim_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "hud_btnmash_giant_btn_1_loop")							--Set end event callback
		vint_set_property(Hud_btnmash.handles.giant_btn_1_anim_h, "is_paused", true)
		
		Hud_btnmash.handles.giant_btn_2_anim_h = vint_object_find("giant_btn_2_anim")			--Find animation
		twn_h = vint_object_find("btn_2_2_dn", Hud_btnmash.handles.giant_btn_2_anim_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "hud_btnmash_giant_btn_2_loop")							--Set end event callback
		vint_set_property(Hud_btnmash.handles.giant_btn_2_anim_h, "is_paused", true)
		
		Hud_btnmash.handles.status_anim_h = vint_object_find("status_anim")					--Find animation
		twn_h = vint_object_find("status_alpha_dwn", Hud_btnmash.handles.status_anim_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "hud_btnmash_status_anim_loop")						--Set end event callback
		vint_set_property(Hud_btnmash.handles.status_anim_h , "is_paused", true)

		Hud_btnmash.handles.bar_flash_anim_h = vint_object_find("bar_flash_anim")					--Find animation
		vint_set_property(Hud_btnmash.handles.bar_flash_anim_h, "is_paused", true)
		
		Hud_btnmash.handles.bar_blink_anim_h = vint_object_find("bar_blink_anim")					--Find animation
		twn_h = vint_object_find("blink_2", Hud_btnmash.handles.bar_blink_anim_h)	
		vint_set_property(twn_h, "end_event", "hud_btnmash_bar_blink_anim_loop")						--Set end event callback
		vint_set_property(Hud_btnmash.handles.bar_blink_anim_h, "is_paused", true)
		
		Hud_btnmash.handles.sticks_alt_h = vint_object_find("sticks_alt")			--Find animation
		twn_h = vint_object_find("alt_ls_2", Hud_btnmash.handles.sticks_alt_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "hud_btnmash_sticks_alt_loop")							--Set end event callback
		vint_set_property(Hud_btnmash.handles.sticks_alt_h, "is_paused", true)
		
		Hud_btnmash.handles.sticks_tog_h = vint_object_find("sticks_tog")			--Find animation
		twn_h = vint_object_find("tog_ls_2", Hud_btnmash.handles.sticks_tog_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "hud_btnmash_sticks_tog_loop")							--Set end event callback
		vint_set_property(Hud_btnmash.handles.sticks_tog_h, "is_paused", true)

		Hud_btnmash.handles.btnmash_fade_in_h = vint_object_find("btnmash_fade_in")			--Find animation
		vint_set_property(Hud_btnmash.handles.btnmash_fade_in_h, "is_paused", true)
		
		Hud_btnmash.handles.btnmash_fade_out_h = vint_object_find("btnmash_fade_out")
		twn_h = vint_object_find("fade_out", Hud_btnmash.handles.btnmash_fade_out_h)
		vint_set_property(twn_h, "end_event", "hud_btnmash_fade_out_end")	
		vint_set_property(Hud_btnmash.handles.btnmash_fade_out_h, "is_paused", true)
		
		
		
		--SIXAXIS I FUGGIN LOVE YOU
		--LR
		Hud_btnmash.handles.sixaxis_lr_anim_h = vint_object_find("sixaxis_lr_anim")			--Find animation
		twn_h = vint_object_find("sixaxis_lr_center_alpha_twn_4", Hud_btnmash.handles.sixaxis_lr_anim_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "sixaxis_lr_anim_loop")							--Set end event callback
		vint_set_property(Hud_btnmash.handles.sixaxis_lr_anim_h, "is_paused", true)
		--FB
		Hud_btnmash.handles.sixaxis_fb_anim_h = vint_object_find("sixaxis_fb_anim")			--Find animation
		twn_h = vint_object_find("sixaxis_fb_center_alpha_twn_4", Hud_btnmash.handles.sixaxis_fb_anim_h)				--Find tween inside anim
		vint_set_property(twn_h, "end_event", "sixaxis_fb_anim_loop")							--Set end event callback
		vint_set_property(Hud_btnmash.handles.sixaxis_fb_anim_h, "is_paused", true)
	
	--subscription
	vint_dataitem_add_subscription("button_mashing_minigame", "update", "hud_btnmash_update")
	

	Hud_btnmash.handles.last_index = -1
	
end


function sixaxis_lr_anim_loop()
	lua_play_anim(Hud_btnmash.handles.sixaxis_lr_anim_h, 0)
end

function sixaxis_fb_anim_loop()
	lua_play_anim(Hud_btnmash.handles.sixaxis_fb_anim_h, 0)
end

function hud_btnmash_fade_out_end()
		
		vint_set_property(Hud_btnmash.handles.meter_grp_h, "visible", false)
		vint_set_property(Hud_btnmash.handles.status_grp_h, "visible", false)
		vint_set_property(Hud_btnmash.handles.giant_buttons_grp_1_h, "visible", false)
		vint_set_property(Hud_btnmash.handles.giant_buttons_grp_2_h, "visible", false)
		vint_set_property(Hud_btnmash.handles.trigger_l_grp_h, "visible", false)
		vint_set_property(Hud_btnmash.handles.trigger_r_grp_h, "visible", false)
		vint_set_property(Hud_btnmash.handles.sticks_grp_h, "visible", false)

end

function hud_btnmash_fade_out_pause_all()
		vint_set_property(Hud_btnmash.handles.lt_rt_alt_h , "is_paused", true)
		vint_set_property(Hud_btnmash.handles.lt_rt_tog_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.lt_only_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.rt_only_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.giant_btn_1_anim_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.giant_btn_2_anim_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.status_anim_h , "is_paused", true)
		vint_set_property(Hud_btnmash.handles.bar_flash_anim_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.bar_blink_anim_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.sticks_alt_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.sticks_tog_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.btnmash_fade_in_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.btnmash_fade_out_h, "is_paused", true)
		vint_set_property(Hud_btnmash.handles.sixaxis_lr_anim_h , "is_paused", true)
		vint_set_property(Hud_btnmash.handles.sixaxis_fb_anim_h , "is_paused", true)
		
end

function hud_btnmash_lt_rt_alt_loop()
	vint_set_property(Hud_btnmash.handles.lt_only_h, "is_paused", true)
	vint_set_property(Hud_btnmash.handles.rt_only_h, "is_paused", true)
	vint_set_property(Hud_btnmash.handles.lt_rt_tog_h, "is_paused", true)
	lua_play_anim(Hud_btnmash.handles.lt_rt_alt_h, 0)
end

function hud_btnmash_lt_rt_tog_loop()
	vint_set_property(Hud_btnmash.handles.lt_only_h, "is_paused", true)
	vint_set_property(Hud_btnmash.handles.rt_only_h, "is_paused", true)
	vint_set_property(Hud_btnmash.handles.lt_rt_alt_h, "is_paused", true)	
	lua_play_anim(Hud_btnmash.handles.lt_rt_tog_h, 0)
end

function hud_btnmash_lt_only_loop()
	vint_set_property(Hud_btnmash.handles.rt_only_h, "is_paused", true)
	vint_set_property(Hud_btnmash.handles.lt_rt_alt_h, "is_paused", true)
	vint_set_property(Hud_btnmash.handles.lt_rt_tog_h, "is_paused", true)
	lua_play_anim(Hud_btnmash.handles.lt_only_h, 0)
end

function hud_btnmash_rt_only_loop()
	vint_set_property(Hud_btnmash.handles.lt_only_h, "is_paused", true)
	vint_set_property(Hud_btnmash.handles.lt_rt_alt_h, "is_paused", true)
	vint_set_property(Hud_btnmash.handles.lt_rt_tog_h, "is_paused", true)
	lua_play_anim(Hud_btnmash.handles.rt_only_h, 0)
end

			
function hud_btnmash_giant_btn_1_loop()
	lua_play_anim(Hud_btnmash.handles.giant_btn_1_anim_h, 0)
end

function hud_btnmash_giant_btn_2_loop()
	lua_play_anim(Hud_btnmash.handles.giant_btn_2_anim_h, 0)
end

function hud_btnmash_sticks_alt_loop()
	lua_play_anim(Hud_btnmash.handles.sticks_alt_h, 0)
end

function hud_btnmash_sticks_tog_loop()
	lua_play_anim(Hud_btnmash.handles.sticks_tog_h, 0)
end

function hud_btnmash_bar_blink_anim_loop()
	lua_play_anim(Hud_btnmash.handles.bar_blink_anim_h, 0)
end

function hud_btnmash_status_anim_loop()
	lua_play_anim(Hud_btnmash.handles.status_anim_h, 0)
end



function hud_btnmash_update(di_h, event)

	local pct_done, button_set_index, is_ps3, player_input_this_frame = vint_dataitem_get(di_h)

		
	--language to local var
	local language = Hud_btnmash.handles.language
	
	--stop blinking the bar if the player didnt shock the victim
	if Hud_btnmash.handles.last_index == 5 and button_set_index ~= 5 then
		lua_play_anim(Hud_btnmash.handles.bar_blink_anim_h, 0)
		vint_set_property(Hud_btnmash.handles.bar_blink_anim_h, "is_paused", true)
	end
	
	--shock shaking
	if button_set_index == 5 then
		local randrot = -0.05 + rand_float(0.00, 0.1)
		local randsx = rand_int(90,120)/100
		local randsy = rand_int(85,110)/100
		local randalpha = rand_float(0.2, 0.6)
		local randanchor = rand_int(0,8)
		vint_set_property(Hud_btnmash.handles.status_grp_h, "scale", randsx, randsy)
		vint_set_property(Hud_btnmash.handles.status_grp_h, "rotation", randrot)
		vint_set_property(Hud_btnmash.handles.status_2_h, "alpha", randalpha)
		vint_set_property(Hud_btnmash.handles.status_grp_h, "anchor", randanchor, randanchor + 15)
	else
		vint_set_property(Hud_btnmash.handles.status_grp_h, "scale", 1, 1)
		vint_set_property(Hud_btnmash.handles.status_grp_h, "rotation", 0)
		vint_set_property(Hud_btnmash.handles.status_grp_h, "anchor", 0, 15)
	end
	
	
	
	if button_set_index ~= -1 then
		--show meter
		vint_set_property(Hud_btnmash.handles.meter_grp_h, "visible", true)
		
				
		--override randomly generated health pct for SHOCK with 1
		if button_set_index == 5 then
			pct_done = 1
		end
		
		--meter colors
		if button_set_index >= 0 and button_set_index <= 4 then
			--fight club colors
			local color_low = {["r"]=0.8, ["g"]=0, ["b"]=0}
			local color_high = {["r"]=0.92, ["g"]=0.85, ["b"]=0.01}
			local color_mid = {["r"]=((color_high.r - color_low.r) * pct_done) + color_low.r, ["g"]=((color_high.g - color_low.g) * pct_done) + color_low.g ,  ["b"]=((color_high.b - color_low.b) * pct_done) + color_low.b}
			vint_set_property(Hud_btnmash.handles.meter_fill_1_h, "tint", color_mid.r, color_mid.g, color_mid.b)
			vint_set_property(Hud_btnmash.handles.meter_fill_2_h, "tint", color_mid.r, color_mid.g, color_mid.b)
		elseif button_set_index == 7 then
			--chest compressions
			local color_low = {["r"]=0.70, ["g"]=0.55, ["b"]=0.06}
			local color_high = {["r"]=0.93, ["g"]=0.86, ["b"]=0.23}
			local color_mid = {["r"]=((color_high.r - color_low.r) * pct_done) + color_low.r, ["g"]=((color_high.g - color_low.g) * pct_done) + color_low.g ,  ["b"]=((color_high.b - color_low.b) * pct_done) + color_low.b}	
			vint_set_property(Hud_btnmash.handles.meter_fill_1_h, "tint", color_mid.r, color_mid.g, color_mid.b)
			vint_set_property(Hud_btnmash.handles.meter_fill_2_h, "tint", color_mid.r, color_mid.g, color_mid.b)
		else
			--shock paddles colors
			local color_low = {["r"]=0.28, ["g"]=0.34, ["b"]=0.80}
			local color_high = {["r"]=0.40, ["g"]=0.53, ["b"]=0.92}
			local color_mid = {["r"]=((color_high.r - color_low.r) * pct_done) + color_low.r, ["g"]=((color_high.g - color_low.g) * pct_done) + color_low.g ,  ["b"]=((color_high.b - color_low.b) * pct_done) + color_low.b}	
			vint_set_property(Hud_btnmash.handles.meter_fill_1_h, "tint", color_mid.r, color_mid.g, color_mid.b)
			vint_set_property(Hud_btnmash.handles.meter_fill_2_h, "tint", color_mid.r, color_mid.g, color_mid.b)
		end
		
		--flash bar on user input
		if player_input_this_frame == true then
			lua_play_anim(Hud_btnmash.handles.bar_flash_anim_h, 0)
		end

		--if the buttons change, swap em out
		if button_set_index ~= Hud_btnmash.handles.last_index then

			--fade in if btn_mash_grp is transparent
			if vint_get_property(Hud_btnmash.handles.btn_mash_h, "alpha") == 0 then
				lua_play_anim(Hud_btnmash.handles.btnmash_fade_in_h, 0)
			end
				
			--button 1	
			if button_set_index == 0 then	
				if Hud_btnmash.handles.sixaxis_h == false then
					vint_set_property(Hud_btnmash.handles.giant_buttons_grp_1_h, "visible", true)
					hud_btnmash_giant_btn_1_loop()
				end
			else
				vint_set_property(Hud_btnmash.handles.giant_buttons_grp_1_h, "visible", false)
				
			end
			
			--button 2
			if button_set_index == 1 then
				if Hud_btnmash.handles.sixaxis_h == false then
					vint_set_property(Hud_btnmash.handles.giant_buttons_grp_2_h, "visible", true)
					hud_btnmash_giant_btn_2_loop()
				end
			else
				vint_set_property(Hud_btnmash.handles.giant_buttons_grp_2_h, "visible", false)
			end

			--triggers
			if button_set_index == 2 then
				vint_set_property(Hud_btnmash.handles.status_grp_h, "visible", false)	
				--left trigger
				vint_set_property(Hud_btnmash.handles.trigger_l_grp_h, "visible", true)
				vint_set_property(Hud_btnmash.handles.trigger_r_grp_h, "visible", true)
				--do this shit
				hud_btnmash_lt_only_loop()
				--Hud_btnmash.handles.last_index = button_set_index	
			elseif button_set_index == 3 then
				vint_set_property(Hud_btnmash.handles.status_grp_h, "visible", false)
				--right trigger
				vint_set_property(Hud_btnmash.handles.trigger_l_grp_h, "visible", true)
				vint_set_property(Hud_btnmash.handles.trigger_r_grp_h, "visible", true)
				--do this shit
				hud_btnmash_rt_only_loop()				
				--Hud_btnmash.handles.last_index = button_set_index
			elseif button_set_index == 4 then
				vint_set_property(Hud_btnmash.handles.status_grp_h, "visible", false)
				--both triggers, alt
				vint_set_property(Hud_btnmash.handles.trigger_l_grp_h, "visible", true)
				vint_set_property(Hud_btnmash.handles.trigger_r_grp_h, "visible", true)				
				--do this shit
				hud_btnmash_lt_rt_alt_loop()
				--Hud_btnmash.handles.last_index = button_set_index
			elseif button_set_index == 5 then
				vint_set_property(Hud_btnmash.handles.status_grp_h, "visible", true)
				--shock!
				vint_set_property(Hud_btnmash.handles.status_1_h, "image", "ui_btnmash_t_" .. language .. "_shock")
				vint_set_property(Hud_btnmash.handles.status_2_h, "image", "ui_btnmash_t_" .. language .. "_shock")
				--both triggers, together
				vint_set_property(Hud_btnmash.handles.trigger_l_grp_h, "visible", true)
				vint_set_property(Hud_btnmash.handles.trigger_r_grp_h, "visible", true)
				--do this shit
				hud_btnmash_lt_rt_tog_loop()
				hud_btnmash_bar_blink_anim_loop()			
			else 
				vint_set_property(Hud_btnmash.handles.trigger_l_grp_h, "visible", false)
				vint_set_property(Hud_btnmash.handles.trigger_r_grp_h, "visible", false)	
			end
			
			--STICKS
			if button_set_index == 6 then
				--sticks, alternating
				vint_set_property(Hud_btnmash.handles.status_1_h, "image", "ui_btnmash_t_" .. language .. "_charge")
				vint_set_property(Hud_btnmash.handles.status_2_h, "image", "ui_btnmash_t_" .. language .. "_charge")	
				hud_btnmash_status_anim_loop()
				if Hud_btnmash.handles.sixaxis_h == false then	
					vint_set_property(Hud_btnmash.handles.sticks_grp_h, "visible", true)	
					hud_btnmash_sticks_alt_loop()								
				end
				
			elseif button_set_index == 7 then
				--sticks, together
				vint_set_property(Hud_btnmash.handles.status_1_h, "image", "ui_btnmash_t_" .. language .. "_chest")
				vint_set_property(Hud_btnmash.handles.status_2_h, "image", "ui_btnmash_t_" .. language .. "_chest")				
				hud_btnmash_status_anim_loop()
				if Hud_btnmash.handles.sixaxis_h == false then		
					vint_set_property(Hud_btnmash.handles.sticks_grp_h, "visible", true)		
					hud_btnmash_sticks_tog_loop()								
				end
			else
				vint_set_property(Hud_btnmash.handles.sticks_grp_h, "visible", false)
			end
			
			
			--SIXAXIS 	
			if button_set_index == 1 or button_set_index == 7  then
				--if all this is true, then do the sixaxis fwd-back tilt
				if Hud_btnmash.handles.sixaxis_h == true then
					vint_set_property(Hud_btnmash.handles.sixaxis_fb_h, "visible", true)
					sixaxis_fb_anim_loop()
					Hud_btnmash.handles.last_index = button_set_index
				end
			else
				vint_set_property(Hud_btnmash.handles.sixaxis_fb_h, "visible", false)
			end
				

			if button_set_index == 0 or button_set_index == 6 then
				--if all this is true, then do the sixaxis left-right tilt
				if Hud_btnmash.handles.sixaxis_h == true then
					vint_set_property(Hud_btnmash.handles.sixaxis_lr_h, "visible", true)
					sixaxis_lr_anim_loop()
					Hud_btnmash.handles.last_index = button_set_index
				end
			else
				vint_set_property(Hud_btnmash.handles.sixaxis_lr_h, "visible", false)
			end			
			
			--STATUS TEXT
			if button_set_index > 4 and button_set_index < 8 then
				vint_set_property(Hud_btnmash.handles.status_grp_h, "visible", true)
			else
				vint_set_property(Hud_btnmash.handles.status_grp_h, "visible", false)
			end
			
			Hud_btnmash.handles.last_index = button_set_index
			
		end
			
	else
		hud_btnmash_fade_out_pause_all()
		lua_play_anim(Hud_btnmash.handles.btnmash_fade_out_h, 0)
		Hud_btnmash.handles.last_index = -1
	end
	
		--update the pct_done bar
		vint_set_property(Hud_btnmash.handles.meter_fill_1_h, "source_se", 288 * pct_done, 10)
		vint_set_property(Hud_btnmash.handles.meter_fill_2_h, "source_se", 288 * pct_done, 10)
		vint_set_property(Hud_btnmash.handles.meter_border_h, "source_se", (288 * pct_done) + 4, 14)
		
		
	
end


function hud_btnmash_cleanup()
	--Unload Pegs
	vint_set_property(Hud_btnmash.handles.btn_mash_h, "visible", false)
	peg_unload("ui_btnmash_" .. Hud_btnmash.handles.platform)
	peg_unload("ui_btnmash_t_" .. Hud_btnmash.handles.language)
end

