
-- Global Variables --

PLAYER_TIMER_INDEX	= {[LOCAL_PLAYER] = 0, [REMOTE_PLAYER] = 1}
PLAYER_SYNC				= {[LOCAL_PLAYER] = SYNC_LOCAL, [REMOTE_PLAYER] = SYNC_REMOTE}
IN_CHURCH = { [LOCAL_PLAYER] = true; [REMOTE_PLAYER] = true; }

Em01_assault_smoke				= {"em01_$n073", "em01_$n074", "em01_$n075", "em01_$n076"}
Em01_smoke_effect					= "smoke_grenade"
Em01_smoke_1					=	INVALID_THREAD_HANDLE
Em01_smoke_2					=	INVALID_THREAD_HANDLE
Em01_smoke_3					=	INVALID_THREAD_HANDLE
Em01_smoke_4					=	INVALID_THREAD_HANDLE

Em01_ultor_assault_01			= {"em01_$c000", "em01_$c001"}
Num_em01_ultor_assault_01		= sizeof_table(Em01_ultor_assault_01)

Em01_ultor_assault_02			= {"em01_$c002", "em01_$c003", "em01_$c004"}
Num_em01_ultor_assault_02		= sizeof_table(Em01_ultor_assault_02)

Num_em01_assault_masako			= Num_em01_ultor_assault_01 + Num_em01_ultor_assault_02
Assault_masako_kill_count		= 0

Em01_ultor_defend_01				= {"em01_$c005", "em01_$c006", "em01_$c007", "em01_$c008"}
Num_em01_ultor_defend_01		= sizeof_table(Em01_ultor_defend_01)
Defend_1_masako_kill_count		= 0

Em01_ultor_defend_02_a			= {"em01_$c009", "em01_$c010", "em01_$c011"}
Num_em01_ultor_defend_02_a		= sizeof_table(Em01_ultor_defend_02_a)

Em01_ultor_defend_02_b			= "em01_$c012"
Em01_ultor_defend_02_c			= "em01_$c013"

Num_em01_defend_masako			= Num_em01_ultor_defend_02_a + 2
Defend_masako_kill_count		= 0

Em01_ultor_west_a					= {"em01_$c014", "em01_$c015", "em01_$c016", "em01_$c017"}
Num_em01_ultor_west_a			= sizeof_table(Em01_ultor_west_a)

Em01_ultor_west_b					= {"em01_$c018", "em01_$c019", "em01_$c020", "em01_$c021"}
Num_em01_ultor_west_b			= sizeof_table(Em01_ultor_west_b)

Num_em01_west_masako				= Num_em01_ultor_west_a + Num_em01_ultor_west_b
West_masako_kill_count			= 0

Em01_ultor_east					= {"em01_$c022", "em01_$c023", "em01_$c024", "em01_$c025"}
Num_em01_ultor_east				= sizeof_table(Em01_ultor_east)

East_masako_kill_count			= 0

Em01_ultor_south_a				= {"em01_$c026", "em01_$c027", "em01_$c028", "em01_$c029"}
Num_em01_ultor_south_a			= sizeof_table(Em01_ultor_south_a)

Em01_ultor_south_b				= {"em01_$c030", "em01_$c031", "em01_$c032", "em01_$c033"}
Num_em01_ultor_south_b			= sizeof_table(Em01_ultor_south_b)

Num_em01_south_masako			= Num_em01_ultor_south_a + Num_em01_ultor_south_b
South_masako_kill_count			= 0

Em01_ultor_heli					= {"em01_$c034", "em01_$c035"}
Heli_masako_killed				= false

Em01_heli_path						= {"em01_$n040", "em01_$n039", "em01_$n040", "em01_$n041"}
Num_em01_heli_path				= sizeof_table(Em01_heli_path)
Em01_heli_can_jump				= {false, true, false, true}
Em01_heli_jump_church			= {["em01_$n039"] = {{"em01_$n131", "em01_$n130", "em01_$n129", "em01_$n041"}, 4},
											["em01_$n041"] = {{"em01_$n129", "em01_$n130", "em01_$n131", "em01_$n039"}, 2}}
HELI_JUMP_PATTERN					= 1
HELI_JUMP_INDEX					= 2

Num_em01_required_kills			= Num_em01_assault_masako + Num_em01_ultor_defend_01 + Num_em01_defend_masako + Num_em01_west_masako + Num_em01_ultor_east + Num_em01_south_masako
Num_em01_required_count			= 0

Em01_siren_lights					= {"em01_$v010", "em01_$v011", "em01_$v012", "em01_$v013", "em01_$v014", "em01_$v017", "em01_$v018"}
Num_em01_siren_lights			= sizeof_table(Em01_siren_lights)

Em01_weak_sauce_heli				= {"em01_$c040", "em01_$c041"}
Em01_lamb_apc_a					= {"em01_$c042", "em01_$c043"}
Em01_lamb_apc_b					= {"em01_$c044", "em01_$c045"}

Em01_oh_shit_heli_a				= {"em01_$c036", "em01_$c037"}
Em01_oh_shit_heli_b				= {"em01_$c038", "em01_$c039"}
Em01_oh_shit_fire_now			= false

Em01_last_stand_heli_a			= {"em01_$c047", "em01_$c048"}
Em01_last_stand_heli_b			= {"em01_$c049", "em01_$c050"}
Em01_last_stand_heli_killed	= 0

Em01_ls_table_a_ccw				= {{"em01_$ccw100_107", "em01_$ccw100_102"}, 
											{"em01_$ccw102_100", "em01_$ccw102_107"}, 
											{"em01_$ccw107_102", "em01_$ccw107_100"}}
Em01_ls_table_a_ccw_idx			= {{["em01_$ccw100_107"] = 3, ["em01_$ccw100_102"] = 2}, 
											{["em01_$ccw102_100"] = 1, ["em01_$ccw102_107"] = 3}, 
											{["em01_$ccw107_102"] = 2, ["em01_$ccw107_100"] = 1}}
Em01_ls_table_a_cw				= {{"em01_$cw100_102", "em01_$cw100_107"}, 
											{"em01_$cw102_107", "em01_$cw102_100"}, 
											{"em01_$cw107_100", "em01_$cw107_102"}}
Em01_ls_table_a_cw_idx			= {{["em01_$cw100_102"] = 2, ["em01_$cw100_107"] = 3}, 
											{["em01_$cw102_107"] = 3, ["em01_$cw102_100"] = 1}, 
											{["em01_$cw107_100"] = 1, ["em01_$cw107_102"] = 2}}
Em01_ls_table_a					= {Em01_ls_table_a_ccw, Em01_ls_table_a_cw}
Em01_ls_table_a_idx				= {Em01_ls_table_a_ccw_idx, Em01_ls_table_a_cw_idx}

Em01_ls_table_b_ccw				= {{"em01_$ccw110_116", "em01_$ccw110_114"}, 
											{"em01_$ccw114_110", "em01_$ccw114_116"}, 
											{"em01_$ccw116_114", "em01_$ccw116_110"}}
Em01_ls_table_b_ccw_idx			= {{["em01_$ccw110_116"] = 3, ["em01_$ccw110_114"] = 2}, 
											{["em01_$ccw114_110"] = 1, ["em01_$ccw114_116"] = 3}, 
											{["em01_$ccw116_114"] = 2, ["em01_$ccw116_110"] = 1}}
Em01_ls_table_b_cw				= {{"em01_$cw110_114", "em01_$cw110_116"}, 
											{"em01_$cw114_116", "em01_$cw114_110"}, 
											{"em01_$cw116_110", "em01_$cw116_114"}}
Em01_ls_table_b_cw_idx			= {{["em01_$cw110_114"] = 2, ["em01_$cw110_116"] = 3}, 
											{["em01_$cw114_116"] = 3, ["em01_$cw114_110"] = 1}, 
											{["em01_$cw116_110"] = 1, ["em01_$cw116_114"] = 2}}
Em01_ls_table_b					= {Em01_ls_table_b_ccw, Em01_ls_table_b_cw}
Em01_ls_table_b_idx				= {Em01_ls_table_b_ccw_idx, Em01_ls_table_b_cw_idx}

Em01_ls_table_julius				= {"em01_$n123", "em01_$n124", "em01_$n140", "em01_$n125", 
											"em01_$n126", "em01_$n127", "em01_$n139", "em01_$n128"}
Num_em01_ls_table_julius		= sizeof_table(Em01_ls_table_julius)

Em01_church_doors					= {"em01_SRCh_dr01MeshMoverA010", "em01_SRCh_dr01MeshMoverA020",
											"em01_SRCh_dr01MeshMoverA030", "em01_SRCh_dr01MeshMoverA040"}
Num_em01_church_doors			= sizeof_table(Em01_church_doors)

--A random set of numbers for a helicopter firing sequence.  Each time the sequence is called it picks from one set of lines, fires a rocket then waits the rand time, then fires the next rocket until the end of the sequence.  The final value is how long the heli waits at the location after firing the last rocket before moving to the next location.
--Changing this table will increase/decrease the randomness of the firing sequence

Em01_missile_sequence			= {{0.0},
											{rand_float(2.0, 3.0), 10.0},
											{rand_float(2.0, 2.5), 10.0},
											{rand_float(1.5, 2.5), rand_float(1.0, 2.0), 10.0},
											{rand_float(2.0, 2.5), rand_float(1.0, 2.0), 10.0},
											{rand_float(1.5, 2.0), rand_float(2.0, 2.5), 10.0},
								}
								
Num_em01_missile_sequence		= sizeof_table(Em01_missile_sequence)

HELI_FIRE_MULTIPLIER 			= 3.0		--Helicopter missle accuracy.  If set to 1 then missle will hit within 1 meter of the target
-- Current number of the path chase vehicles that are chasing the player
Num_path_chase_vehicles_chasing = 0

CHURCH_IS_BEING_ABANDONED		= false
CHURCH_ABANDON_DIST_WARN		= 80.0
CHURCH_ABANDON_TIME				= 15000

-- Max number of path chase vehicles that should chase the player
MAX_NUM_PATH_CHASERS          = 3

CHURCH_SCRIPT_PAUSED				= false

SMOKE_DEFEND_ACTIVE				= true
SMOKE_ASSAULT_ACTIVE				= true

IN_COOP								= false

LAST_STAND_SHOW_PERFORMED		= false
LAST_STAND_ATTACK_STARTED     = false
Controls_disabled = false
Players_have_temp_weapons		= false

LOCATION_START_LOCAL				= "em01_$local_player_start"
LOCATION_START_REMOTE			= "em01_$remote_player_start"

GROUP_START							= "em01_$start"
GROUP_ASSAULT_1					= "em01_$assault_1"
GROUP_ASSAULT_2					= "em01_$assault_2"
GROUP_DEFEND_1						= "em01_$defend_1"
GROUP_DEFEND_2_A					= "em01_$defend_2_a"
GROUP_DEFEND_2_B					= "em01_$defend_2_b"
GROUP_DEFEND_2_C					= "em01_$defend_2_c"
GROUP_WEST_VEHICLES				= "em01_$west_vehicles"
GROUP_WEST_A						= "em01_$west_a"
GROUP_WEST_B						= "em01_$west_b"
GROUP_EAST_VEHICLE				= "em01_$east_vehicle"
GROUP_EAST							= "em01_$east"
GROUP_SOUTH_VEHICLES				= "em01_$south_vehicles"
GROUP_SOUTH_A						= "em01_$south_a"
GROUP_SOUTH_B						= "em01_$south_b"
GROUP_HELI_VEHICLE				= "em01_$heli_vehicle"
GROUP_HELI							= "em01_$heli"
GROUP_JULIUS_VEHICLE				= "em01_$julius_vehicle"
GROUP_WEAK_VEHICLE				= "em01_$weak_vehicle"
GROUP_WEAK							= "em01_$weak"
GROUP_LAMB							= "em01_$lamb"
GROUP_LAMB_A						= "em01_$lamb_a"
GROUP_LAMB_B						= "em01_$lamb_b"
GROUP_OH_SHIT_VEH_A				= "em01_$oh_shit_vehicle_a"
GROUP_OH_SHIT_VEH_B				= "em01_$oh_shit_vehicle_b"
GROUP_OH_SHIT_A					= "em01_$oh_shit_a"
GROUP_OH_SHIT_B					= "em01_$oh_shit_b"
GROUP_ROADBLOCK					= "em01_$roadblock"
GROUP_STAND_VEH_A					= "em01_$last_stand_veh_a"
GROUP_STAND_VEH_B					= "em01_$last_stand_veh_b"
GROUP_STAND_A						= "em01_$last_stand_a"
GROUP_STAND_B						= "em01_$last_stand_b"

PATH_CHASE_GROUPS             = { "em01_$Path_Chase_V1", "em01_$Path_Chase_V2",
                                  "em01_$Path_Chase_V3", "em01_$Path_Chase_V4",
                                  "em01_$Path_Chase_V5", "em01_$Path_Chase_V6" }

PATH_CHASE_PASSENGERS         = { { "em01_$Path_Chase_V1_M01", "em01_$Path_Chase_V1_M02" },
                                  { "em01_$Path_Chase_V2_M01", "em01_$Path_Chase_V2_M02" },
                                  { "em01_$Path_Chase_V3_M01", "em01_$Path_Chase_V3_M02" },
                                  { "em01_$Path_Chase_V4_M01", "em01_$Path_Chase_V4_M02" },
                                  { "em01_$Path_Chase_V5_M01", "em01_$Path_Chase_V5_M02" },
                                  { "em01_$Path_Chase_V6_M01", "em01_$Path_Chase_V6_M02" } }

CHARACTER_JULIUS					= "em01_$julius"

CHURCH_LEASH_INSIDE				= "em01_$inside"
CHURCH_LEASH_EAST					= "em01_$east"
CHURCH_LEASH_WEST					= "em01_$west"
CHURCH_LEASH_SOUTH				= "em01_$south"

HELI_ATTACK_POINT_1				= "em01_$n044"
HELI_ATTACK_POINT_2				= "em01_$n045"

LAST_STAND_POINT_A				= "em01_$n083"
LAST_STAND_POINT_B				= "em01_$n091"
LAST_STAND_CHECK_A				= "em01_$n100"
LAST_STAND_CHECK_B				= "em01_$n110"

DEFEND_CHURCH_TRIGGER			= "em01_$defend_trigger"
APC_RAM1_TRIGGER					= "em01_$ram1"
APC_RAM2_TRIGGER					= "em01_$ram2"
PEEL_OFF_TRIGGER					= "em01_$call_off"
GOTCHA_TRIGGER						= "em01_$gotcha"
LAST_STAND_SHOW_TRIGGER			= "em01_$last_stand_show"
LAST_STAND_EXECUTE_TRIGGER		= "em01_$last_stand_execute"

TRIGGER_BLAST_CHURCH				= "em01_$blast_church"
TRIGGER_FALL_BACK					= "em01_$fall_back"
TRIGGER_BLAST_OH_SHIT			= "em01_$blast_oh_shit"
TRIGGER_BLAST_VEHICLE			= "em01_$blast_vehice"
TRIGGER_BLAST_CANCEL				= "em01_$blast_cancel"

PATH_CHASE_TRIGGERS           =  { "em01_$Path_Chase_V1_T", "em01_$Path_Chase_V2_T",
                                   "em01_$Path_Chase_V3_T", "em01_$Path_Chase_V4_T",
                                   "em01_$Path_Chase_V5_T", "em01_$Path_Chase_V6_T" }

WEST_SIDE_VEHICLE_1				= "em01_$v000"
WEST_SIDE_VEHICLE_2				= "em01_$v001"
EAST_SIDE_VEHICLE					= "em01_$v002"
SOUTH_SIDE_VEHICLE_1				= "em01_$v003"
SOUTH_SIDE_VEHICLE_2				= "em01_$v004"
HELI_ATTACK_VEHICLE				= "em01_$v005"
JULIUS_VEHICLE						= "em01_$v006"
OH_SHIT_HELI_A						= "em01_$v007"
OH_SHIT_HELI_B						= "em01_$v008"
WEAK_HELI_VEHICLE					= "em01_$v009"
LAMB_A_VEHICLE						= "em01_$v015"
LAMB_B_VEHICLE						= "em01_$v016"
LAST_STAND_HELI_A					= "em01_$v019"
LAST_STAND_HELI_B					= "em01_$v020"

PATH_CHASE_VEHICLES           = { "em01_$Path_Chase_V1", "em01_$Path_Chase_V2",
                                  "em01_$Path_Chase_V3", "em01_$Path_Chase_V4",
                                  "em01_$Path_Chase_V5", "em01_$Path_Chase_V6" }

WEST_SIDE_ARRIVE_1				= "em01_$path000"
WEST_SIDE_ARRIVE_2				= "em01_$path001"
EAST_SIDE_ARRIVE					= "em01_$path002"
SOUTH_SIDE_ARRIVE_1				= "em01_$path003"
SOUTH_SIDE_ARRIVE_2				= "em01_$path004"
HELI_ATTACK_ARRIVE				= "em01_$path005"
ESCAPE_ROUTE_PATH_A				= "em01_$path006a"
ESCAPE_ROUTE_PATH_B				= "em01_$path006b"
CRASH_ROUTE_PATH					= "em01_$path007"
OH_SHIT_PATH_A						= "em01_$path008"
OH_SHIT_PATH_B						= "em01_$path009"
APC_RAM_PATH_A						= "em01_$path010"
APC_RAM_PATH_B						= "em01_$path011"
LAST_STAND_PATH_A					= "em01_$path012"
LAST_STAND_PATH_B					= "em01_$path013"

CHURCH_ABANDON_INTERIOR_THREAD	= INVALID_THREAD_HANDLE
CHURCH_ABANDON_PERIMETER_THREAD	= INVALID_THREAD_HANDLE
LAMB_TO_THE_SLAUGHTER_THREAD		= INVALID_THREAD_HANDLE
JULIUS_MOVE_TO_POINT_THREAD		= INVALID_THREAD_HANDLE
JULIUS_VEHICLE_HEALTH_WATCH		= INVALID_THREAD_HANDLE

CT_INTRO = "BON01"
CT_OUTRO = "BON02"

-- This needs to come after Julius being defined...
Em01_heliwave1_exchange = {{"JULIUS_BONUS_HELICOPTER_01",	CHARACTER_JULIUS,	0.0},
									{"PLAYER_BONUS_HELICOPTER_01",	LOCAL_PLAYER,		rand_float(0.25, 0.5)}}

Em01_helideath_exchange = {{"JULIUS_BONUS_HELIDEATH_01",		CHARACTER_JULIUS,	0.0},
									{"PLAYER_BONUS_HELIDEATH_01",		LOCAL_PLAYER,		rand_float(0.25, 0.5)}}

Em01_vehicle_exchange	= {{"JULIUS_BONUS_CARCHAT_01",		CHARACTER_JULIUS,	0.0},
									{"PLAYER_BONUS_CARCHAT_01",		LOCAL_PLAYER,		rand_float(0.25, 0.5)},
									{"JULIUS_BONUS_CARCHAT_02",		CHARACTER_JULIUS,	rand_float(0.25, 0.5)}}

Em01_heliwave2_exchange = {{"JULIUS_BONUS_HELIWAVE2_01",		CHARACTER_JULIUS,	0.0},
									{"PLAYER_BONUS_HELIWAVE2_01",		LOCAL_PLAYER,		rand_float(0.25, 0.5)}}


function em01_start(checkpoint, is_restart)

	set_mission_author("Ryan Spencer")

	-- Check for coop being active
	if (coop_is_active()) then
		IN_COOP	= true
	end

	mission_start_fade_out()

	-- Starting from the beginning?
	if (checkpoint == MISSION_START_CHECKPOINT) then
		-- No checkpoint
	if (not is_restart) then
      cutscene_play( CT_INTRO )
	end
      fade_out( 0 )

		-- Have smoke bombs go off!!!
		thread_new("em01_smoke_it_up")

		-- Create the starting group
		group_create(GROUP_START, true)

		-- Teleport the player(s)
		teleport_coop(LOCATION_START_LOCAL, LOCATION_START_REMOTE)

		group_create_hidden(GROUP_ASSAULT_1)
		group_create_hidden(GROUP_ASSAULT_2)

		-- Julius received a beating
		set_current_hit_points(CHARACTER_JULIUS, get_max_hit_points(CHARACTER_JULIUS) * 0.5)
		-- Give Julius a handgun
		inv_item_remove_all(CHARACTER_JULIUS)
		inv_item_add("desert eagle", 1, CHARACTER_JULIUS)

		-- Make sure the church is not abandoned
		CHURCH_ABANDON_INTERIOR_THREAD = thread_new("em01_church_abandonment_interior")

		-- Proceed with the assault
		thread_new("em01_church_assault")

	elseif (checkpoint == "perimeter") then

		group_create(GROUP_JULIUS_VEHICLE, true)

		-- Load our ultor strike scenario
		group_create_hidden(GROUP_WEAK_VEHICLE)
		group_create_hidden(GROUP_WEAK)
		group_create_hidden(GROUP_LAMB)
		group_create_hidden(GROUP_LAMB_A)
		group_create_hidden(GROUP_LAMB_B)

		thread_new("em01_make_a_run_for_it")
	else-- checkpoint == "last stand"
      -- Give the player(s) the rocket launcher
      inv_weapon_add_temporary(LOCAL_PLAYER, "rpg_annihilator", 1, true)
      inv_weapon_add_temporary(LOCAL_PLAYER, "AR50_launcher", 1 ,true )
      inv_weapon_add_temporary(LOCAL_PLAYER, "grenade", 1, true )
      if (IN_COOP) then
         inv_weapon_add_temporary(REMOTE_PLAYER, "rpg_annihilator", 1, true)
         inv_weapon_add_temporary(REMOTE_PLAYER, "AR50_launcher", 1 ,true )
         inv_weapon_add_temporary(REMOTE_PLAYER, "grenade", 1, true )
      end
		Players_have_temp_weapons = true

		-- Create our groups for the final stand off
		group_create_hidden(GROUP_STAND_VEH_A, true)
		group_create_hidden(GROUP_STAND_VEH_B, true)
		group_create(GROUP_STAND_A, true)
		group_create(GROUP_STAND_B, true)
		group_create(GROUP_ROADBLOCK, true)

		thread_new("em01_last_stand_show")
		thread_new("em01_last_stand_attack")
	end

	--Turn off museum kiosk triggers
	trigger_enable("julius_kiosk_$01", false)
	trigger_enable("julius_kiosk_$02", false)
	trigger_enable("julius_kiosk_$03", false)
	trigger_enable("julius_kiosk_$04", false)
	trigger_enable("julius_kiosk_$05", false)
	trigger_enable("julius_kiosk_$06", false)
	trigger_enable("julius_kiosk_$07", false)
	
	-- Open Church doors
	for i = 1, Num_em01_church_doors, 1 do
      mesh_mover_reset(Em01_church_doors[i])
		door_open(Em01_church_doors[i])
	end

	-- Add Julius to the party
	party_add(CHARACTER_JULIUS, LOCAL_PLAYER)
   set_team(CHARACTER_JULIUS, "Playas")

	on_dismiss("em01_julius_abondoned", CHARACTER_JULIUS)
	on_death("em01_julius_died", CHARACTER_JULIUS)

	-- Have Julius attack enemies
	set_attack_enemies_flag(CHARACTER_JULIUS, true)

	-- Persona overrides
	persona_override_character_stop_all(CHARACTER_JULIUS)
	persona_override_character_start(CHARACTER_JULIUS, POT_SITUATIONS[POT_ATTACK], "JULIUS_BONUS_ATTACK")
	persona_override_character_start(CHARACTER_JULIUS, POT_SITUATIONS[POT_TAKE_DAMAGE], "JULIUS_BONUS_TAKEDAM")

	-- Disable Ultor spawning
	notoriety_force_no_spawn("ultor", true)
	notoriety_force_no_spawn("police", true)
	notoriety_set_min("police", 4)
	roadblocks_enable(false)

	-- Fade in
	mission_start_fade_in()
end

function em01_cleanup()

	-- Kill any running threads
	thread_kill(CHURCH_ABANDON_INTERIOR_THREAD)
	thread_kill(CHURCH_ABANDON_PERIMETER_THREAD)
	thread_kill(LAMB_TO_THE_SLAUGHTER_THREAD)
	thread_kill(JULIUS_MOVE_TO_POINT_THREAD)
	thread_kill(JULIUS_VEHICLE_HEALTH_WATCH)

	-- Stop timers
	em01_church_abandonment_interior_cleanup()

	--stop the smoke effect
	effect_stop(Em01_smoke_1)
	effect_stop(Em01_smoke_2)
	effect_stop(Em01_smoke_3)
	effect_stop(Em01_smoke_4)
	
	--Turn on museum kiosk triggers
	trigger_enable("julius_kiosk_$01", true)
	trigger_enable("julius_kiosk_$02", true)
	trigger_enable("julius_kiosk_$03", true)
	trigger_enable("julius_kiosk_$04", true)
	trigger_enable("julius_kiosk_$05", true)
	trigger_enable("julius_kiosk_$06", true)
	trigger_enable("julius_kiosk_$07", true)

	-- Reset persona overrides...
	if (character_exists(CHARACTER_JULIUS)) then
      set_team(CHARACTER_JULIUS, "Neutral Gang")
		persona_override_character_stop_all(CHARACTER_JULIUS)
	end

	-- Fix hud
	damage_indicator_off(0)
	objective_text_clear(0)

	-- Reset notoriety
	notoriety_force_no_spawn("police", false)
	notoriety_force_no_spawn("ultor", false)
	notoriety_reset("police")
	roadblocks_enable()

	-- Remove callbacks
	on_vehicle_enter("", JULIUS_VEHICLE)
	on_vehicle_exit("", JULIUS_VEHICLE)
	on_vehicle_destroyed("", JULIUS_VEHICLE)
	on_vehicle_destroyed("", WEAK_HELI_VEHICLE)
	on_trigger_exit("", DEFEND_CHURCH_TRIGGER)
	on_trigger("", DEFEND_CHURCH_TRIGGER)

	set_player_can_enter_exit_vehicles(LOCAL_PLAYER, true)
	if ( coop_is_active() ) then
		set_player_can_enter_exit_vehicles(REMOTE_PLAYER, true)
	end

	if ( Players_have_temp_weapons ) then
		inv_weapon_remove_temporary(LOCAL_PLAYER, "rpg_annihilator")
		inv_weapon_remove_temporary(LOCAL_PLAYER, "AR50_launcher")
		inv_weapon_remove_temporary(LOCAL_PLAYER, "grenade")
		if ( coop_is_active() ) then
			inv_weapon_remove_temporary(REMOTE_PLAYER, "rpg_annihilator")
			inv_weapon_remove_temporary(REMOTE_PLAYER, "AR50_launcher")
			inv_weapon_remove_temporary(REMOTE_PLAYER, "grenade")
		end
	end

	-- Remove the markers
	marker_remove_group(GROUP_ASSAULT_1, SYNC_ALL)
	marker_remove_group(GROUP_ASSAULT_2, SYNC_ALL)
	
	marker_remove_group(GROUP_DEFEND_1, SYNC_ALL)
	marker_remove_group(GROUP_DEFEND_2_A, SYNC_ALL)
	marker_remove_group(GROUP_DEFEND_2_B, SYNC_ALL)
	marker_remove_group(GROUP_DEFEND_2_C, SYNC_ALL)

	marker_remove_group(GROUP_WEST_A, SYNC_ALL)
	marker_remove_group(GROUP_WEST_B, SYNC_ALL)

	marker_remove_group(GROUP_EAST, SYNC_ALL)

	marker_remove_group(GROUP_SOUTH_A, SYNC_ALL)
	marker_remove_group(GROUP_SOUTH_B, SYNC_ALL)

	marker_remove_group(GROUP_HELI_VEHICLE, SYNC_ALL)

	marker_remove_group(GROUP_JULIUS_VEHICLE, SYNC_ALL)

	-- Release these groups to the world
	release_to_world(GROUP_ASSAULT_1)
	release_to_world(GROUP_ASSAULT_2)
	
	release_to_world(GROUP_DEFEND_1)
	release_to_world(GROUP_DEFEND_2_A)
	release_to_world(GROUP_DEFEND_2_B)
	release_to_world(GROUP_DEFEND_2_C)

	release_to_world(GROUP_WEST_VEHICLES)
	release_to_world(GROUP_WEST_A)
	release_to_world(GROUP_WEST_B)

	release_to_world(GROUP_EAST_VEHICLE)
	release_to_world(GROUP_EAST)

	release_to_world(GROUP_SOUTH_VEHICLES)
	release_to_world(GROUP_SOUTH_A)
	release_to_world(GROUP_SOUTH_B)

	release_to_world(GROUP_HELI_VEHICLE)
	release_to_world(GROUP_HELI)

	release_to_world(GROUP_WEAK_VEHICLE)
	release_to_world(GROUP_WEAK)
	release_to_world(GROUP_LAMB)
	release_to_world(GROUP_LAMB_A)
	release_to_world(GROUP_LAMB_B)

	release_to_world(GROUP_OH_SHIT_VEH_A)
	release_to_world(GROUP_OH_SHIT_VEH_B)
	release_to_world(GROUP_OH_SHIT_A)
	release_to_world(GROUP_OH_SHIT_B)

   release_to_world(GROUP_STAND_A)
	release_to_world(GROUP_STAND_B)
	
	-- Destroy these groups
	group_destroy(GROUP_ROADBLOCK)
	group_destroy(GROUP_START)
	group_destroy(GROUP_JULIUS_VEHICLE)

	-- Close the Church doors
	for i = 1, Num_em01_church_doors, 1 do
		door_close(Em01_church_doors[i])
	end

	if ( Controls_disabled ) then
		player_controls_enable( LOCAL_PLAYER )
		if ( coop_is_active() ) then
			player_controls_enable( REMOTE_PLAYER )
		end
	end

   turn_vulnerable(LOCAL_PLAYER)
   if (coop_is_active()) then
      turn_vulnerable(REMOTE_PLAYER)
   end
end

function em01_success()

	-- Post the news event
	radio_post_event("JANE_NEWS_BONUS", true)

end

function em01_complete()
	-- Mission is complete with success
	mission_end_success("em01", CT_OUTRO)
end

function em01_church_abandoned()
	mission_end_failure("em01", "em01_church_abandoned")
end

function em01_julius_died()
	mission_end_failure("em01", "em01_julius_died")
end

function em01_julius_abondoned()
	mission_end_failure("em01", "em01_julius_abandoned")
end

function em01_vehicle_destroyed()
	mission_end_failure("em01", "em01_vehicle_destroyed")
end

function em01_smoke_it_up()
	while (SMOKE_DEFEND_ACTIVE) do
		if (SMOKE_ASSAULT_ACTIVE) then
			-- Play the two effects near the front
			Em01_smoke_1 = effect_play(Em01_smoke_effect, Em01_assault_smoke[1], false)
			delay(1)
			Em01_smoke_2 = effect_play(Em01_smoke_effect, Em01_assault_smoke[2], false)
			
			-- Delay so there is smoke in the church for an extended period of time
			delay(4)
		end

		-- Play the two effects near the sides
		Em01_smoke_3 = effect_play(Em01_smoke_effect, Em01_assault_smoke[3], false)
		delay(1)
		Em01_smoke_4 = effect_play(Em01_smoke_effect, Em01_assault_smoke[4], false)

		-- Delay and do it again...smoke effects lasts about 30 seconds
		delay(26)
	end
end

function em01_church_assault()
	-- Setup the defense trigger
	trigger_enable(DEFEND_CHURCH_TRIGGER, true)
	on_trigger_exit("em01_exited_church", DEFEND_CHURCH_TRIGGER)
	on_trigger("em01_entered_church", DEFEND_CHURCH_TRIGGER)
	trigger_set_delay_between_activations(DEFEND_CHURCH_TRIGGER, 0)

	-- Delay before Julius speaks and the objective is given
	delay(2.0)
	audio_play_for_character("JULIUS_BONUS_MISSIONSTART", CHARACTER_JULIUS, "voice", false, true)

	-- Instruct the player to defend the church
	mission_help_table("em01_defend_church")

	-- Delay before the church assault begins
	delay(2.0)

	-- Add the objective
	objective_text(0, "em01_defend_objective", Assault_masako_kill_count, Num_em01_assault_masako)

	-- Assault the church...wave one attack
	group_show(GROUP_ASSAULT_1)
	marker_add_group(GROUP_ASSAULT_1, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)
	for i = 1, Num_em01_ultor_assault_01, 1 do
		on_death("em01_assault_killed", Em01_ultor_assault_01[i])
		attack(Em01_ultor_assault_01[i])
	end

	-- Assault the church...wave two prepare
	group_show(GROUP_ASSAULT_2)
	marker_add_group(GROUP_ASSAULT_2, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)
	for i = 1, Num_em01_ultor_assault_02, 1 do
		on_death("em01_assault_killed", Em01_ultor_assault_02[i])
	end

	-- Wait a little while before sending in ground troops
	
	delay(3.0)

	-- Assault the church...wave two attack
	for i = 1, Num_em01_ultor_assault_02, 1 do
		attack(Em01_ultor_assault_02[i])
	end

	-- Create the attackers in the defend church phase
	group_create_hidden(GROUP_DEFEND_1)
	group_create_hidden(GROUP_DEFEND_2_A)
	group_create_hidden(GROUP_DEFEND_2_B)
	group_create_hidden(GROUP_DEFEND_2_C)
end

function em01_exited_church(human, trigger)

	IN_CHURCH[human] = false;

end

function em01_entered_church(human, trigger)

	IN_CHURCH[human] = true;

end

-- Handle prompts/failure when players leave the church interior during the inital Masako attacks
function em01_church_abandonment_interior()

	-- Were players in church?
	local player_was_inside = { [LOCAL_PLAYER] = true; [REMOTE_PLAYER] = true; }

	-- Called when a player has left the church.
	local function em01_player_left_church(player)

		-- Enable notoriety spawns
		notoriety_force_no_spawn("ultor", false)
		notoriety_force_no_spawn("police", false)

		-- Pause the script
		CHURCH_SCRIPT_PAUSED = true

		-- Show the abandon message
		local sync = PLAYER_SYNC[player]
		mission_help_table_nag("em01_abandoning_church", nil, nil, sync)

		-- Start a timer
		hud_timer_set(PLAYER_TIMER_INDEX[player], CHURCH_ABANDON_TIME, "em01_church_abandoned", true, sync)

	end

	-- Called a player reenter the church.
	local function em01_player_reentered_church(player)

		-- Remove timer
		hud_timer_stop(PLAYER_TIMER_INDEX[player])

		-- If both players are now in the church
		if ( IN_CHURCH[LOCAL_PLAYER] and IN_CHURCH[REMOTE_PLAYER] ) then

			-- Disable notoriety spawns
			notoriety_force_no_spawn("ultor", true)
			notoriety_force_no_spawn("police", true)

			-- Unpause the script
			CHURCH_SCRIPT_PAUSED = false
		
		end

	end

	-- Update a player's prompts
	local function update_prompts(player)

		-- A player exited
		if (player_was_inside[player] and (not IN_CHURCH[player]) ) then
			em01_player_left_church(player)
		end

		-- A player entered
		if	( (not player_was_inside[player]) and IN_CHURCH[player] ) then
			em01_player_reentered_church(player)
		end

	end

	while(1) do

		-- Don't hang
		thread_yield()

		-- Update player prompts
		update_prompts(LOCAL_PLAYER)
		if ( coop_is_active() ) then
			update_prompts(REMOTE_PLAYER)
		end

		-- Remember player locations
		player_was_inside[LOCAL_PLAYER] = IN_CHURCH[LOCAL_PLAYER]
		player_was_inside[REMOTE_PLAYER] = IN_CHURCH[REMOTE_PLAYER]

	end
end

function em01_church_abandonment_interior_cleanup()

	thread_kill(CHURCH_ABANDON_INTERIOR_THREAD)

   -- Remove any failure timers
   hud_timer_stop(0)
   hud_timer_stop(1)
	
	-- Unpause
	CHURCH_SCRIPT_PAUSED = false
	
end

function em01_clear_objective(show_help_text)
	-- Delay so that the objective can be seen updated
	delay(1)
	-- Clear objective
	objective_text_clear(0)
	--show help text for when first sets of masako are killed
	if show_help_text then
		delay(3)
		while(CHURCH_SCRIPT_PAUSED) do
			thread_yield()
		end
		mission_help_table("em01_more_masako")
	end
	
	-- Delay a couple of seconds so the objective is seen as cleared
	delay(2)
end

function em01_assault_killed(human)
	-- Remove the marker
	marker_remove_npc(human, SYNC_ALL)

	-- Add to the count
	Assault_masako_kill_count = Assault_masako_kill_count + 1

	-- Track total kills
	Num_em01_required_count = Num_em01_required_count + 1

	-- Update objective
	objective_text(0, "em01_defend_objective", Assault_masako_kill_count, Num_em01_assault_masako)

	-- Start the side attacks
	if (Assault_masako_kill_count == Num_em01_assault_masako) then
		em01_clear_objective(true)
		em01_defend_church_part1()
	end
end

function em01_defend_church_part1()

	while (CHURCH_SCRIPT_PAUSED) do
		thread_yield()
	end

	-- The smoke assault is finished
	SMOKE_ASSAULT_ACTIVE = false

	-- Show the group
	group_show(GROUP_DEFEND_1)
	-- Have the group attack from the west side
	marker_add_group(GROUP_DEFEND_1, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)
	for i = 1, Num_em01_ultor_defend_01, 1 do
		on_death("em01_defend_1_killed", Em01_ultor_defend_01[i])
		attack(Em01_ultor_defend_01[i])
	end

	-- Add the objective
	objective_text(0, "em01_defend_objective", Defend_1_masako_kill_count, Num_em01_ultor_defend_01)
end

function em01_defend_1_killed(human)
	-- Remove the marker
	marker_remove_npc(human, SYNC_ALL)

	-- Add to the count
	Defend_1_masako_kill_count = Defend_1_masako_kill_count + 1

	-- Keep track of the total kills
	Num_em01_required_count = Num_em01_required_count + 1

	-- Update the objective
	objective_text(0, "em01_defend_objective", Defend_1_masako_kill_count, Num_em01_ultor_defend_01)

	-- One assault human left so start the perimeter attack
	if (Defend_1_masako_kill_count == Num_em01_ultor_defend_01) then
		em01_clear_objective(true)
		em01_defend_church_part2()
	end
end

function em01_defend_church_part2()

	while (CHURCH_SCRIPT_PAUSED) do
		thread_yield()
	end

	-- Show the group
	group_show(GROUP_DEFEND_2_A)
	-- Have the group attack from the west side
	marker_add_group(GROUP_DEFEND_2_A, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)
	for i = 1, Num_em01_ultor_defend_02_a, 1 do
		on_death("em01_defend_2_killed", Em01_ultor_defend_02_a[i])
		attack(Em01_ultor_defend_02_a[i])
	end

	-- Wait until we can show both the masako from upstairs...
	em01_defend_church_part2_reveal_human(GROUP_DEFEND_2_B, Em01_ultor_defend_02_b)
	em01_defend_church_part2_reveal_human(GROUP_DEFEND_2_C, Em01_ultor_defend_02_c)

	-- Create the perimeter west group
	group_create_hidden(GROUP_WEST_VEHICLES)
	group_create_hidden(GROUP_WEST_A)
	group_create_hidden(GROUP_WEST_B)

	-- The smoke defend church sequence if finished
	SMOKE_DEFEND_ACTIVE = false

	-- Add the objective
	objective_text(0, "em01_defend_objective", Defend_masako_kill_count, Num_em01_defend_masako)
end

function em01_defend_church_part2_reveal_human(group, human)
	-- Wait until group is shown
	while (1) do
		thread_yield()

		-- Only attempted if the script is not paused
		if (not CHURCH_SCRIPT_PAUSED) then
			local local_can_show		= not npc_in_player_fov(human, LOCAL_PLAYER)
			local remote_can_show	= true

			if (IN_COOP) then
				local	remote_can_show = not npc_in_player_fov(human, REMOTE_PLAYER)
			end

			if (local_can_show and remote_can_show) then
				-- Show the human
				group_show(group)
				-- Have the human attack
				marker_add_group(group, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)
				on_death("em01_defend_2_killed", human)
				attack(human)
				return
			end
		end
	end
end

function em01_defend_2_killed(human)
	-- Remove the marker
	marker_remove_npc(human, SYNC_ALL)

	-- Add to the count
	Defend_masako_kill_count = Defend_masako_kill_count + 1

	-- Keep track of the total kills
	Num_em01_required_count = Num_em01_required_count + 1

	-- Update the objective
	objective_text(0, "em01_defend_objective", Defend_masako_kill_count, Num_em01_defend_masako)

	-- One assault human left so start the perimeter attack
	if (Defend_masako_kill_count == Num_em01_defend_masako) then

		-- Cleanup interior abandonment thread
		em01_church_abandonment_interior_cleanup()

		em01_clear_objective(false)
		em01_perimeter_west()
	end
end

function em01_church_abandon_perimeter_cleanup()
   CHURCH_IS_BEING_ABANDONED = false

   -- Disable notoriety spawning
   notoriety_force_no_spawn("ultor", true)
   notoriety_force_no_spawn("police", true)

   CHURCH_SCRIPT_PAUSED = false

   -- Remove timer
   hud_timer_stop(0)

   -- Remove radius
   minimap_icon_remove_radius(CHURCH_LEASH_INSIDE, SYNC_ALL)
end

-- When player is too far from the church, pause script and start failure timers.
function em01_church_abandonment_perimeter()

	while(1) do
		thread_yield()

		local local_player = get_dist_char_to_nav(LOCAL_PLAYER, CHURCH_LEASH_INSIDE)
		local remote_player = 0.0

		if (IN_COOP) then
			remote_player = get_dist_char_to_nav(REMOTE_PLAYER, CHURCH_LEASH_INSIDE)
		end

		local		local_abandoning = (local_player > CHURCH_ABANDON_DIST_WARN)
		local		remote_abandoning = (remote_player > CHURCH_ABANDON_DIST_WARN)

		if (local_abandoning or remote_abandoning) then
			if (CHURCH_IS_BEING_ABANDONED == false) then

				-- Abandon message
				mission_help_table_nag("em01_abandoning_church")

				-- Spawn ultor
				notoriety_force_no_spawn("ultor", false)
				notoriety_force_no_spawn("police", false)

				CHURCH_SCRIPT_PAUSED = true

				-- Fail timer
				hud_timer_set(0, CHURCH_ABANDON_TIME, "em01_church_abandoned")

				-- Add radius
				minimap_icon_add_radius(CHURCH_LEASH_INSIDE, CHURCH_ABANDON_DIST_WARN, SYNC_ALL)
			end
			
			CHURCH_IS_BEING_ABANDONED = true
		else
			CHURCH_IS_BEING_ABANDONED = false

			-- Disable notoriety spawning
			notoriety_force_no_spawn("ultor", true)
			notoriety_force_no_spawn("police", true)

			CHURCH_SCRIPT_PAUSED = false

			-- Remove timer
			hud_timer_stop(0)

			-- Remove radius
			minimap_icon_remove_radius(CHURCH_LEASH_INSIDE, SYNC_ALL)
		end
	end
end

function em01_west_perimeter_vehicle_pathfind()
	-- Do the pathfind...this function yields
	vehicle_pathfind_to(WEST_SIDE_VEHICLE_2, WEST_SIDE_ARRIVE_2, true, false)
	vehicle_stop(WEST_SIDE_VEHICLE_2)

	-- The vehicle has reached its destination have the humans exit
	for i = 1, Num_em01_ultor_west_b, 1 do
		vehicle_exit(Em01_ultor_west_b[i])
		attack(Em01_ultor_west_b[i], CHARACTER_JULIUS)
	end
	
	-- Do the pathfind...this function yields
	vehicle_pathfind_to(WEST_SIDE_VEHICLE_1, WEST_SIDE_ARRIVE_1, true, false)
	vehicle_stop(WEST_SIDE_VEHICLE_1)

	-- The vehicle has reached its destination have the humans exit
	for i = 1, Num_em01_ultor_west_a, 1 do
		vehicle_exit(Em01_ultor_west_a[i])
		attack(Em01_ultor_west_a[i], CHARACTER_JULIUS)
	end
end

function em01_move_julius_to_point(navpoint)
	-- Change Julius's state to idle before pathfinding
	npc_go_idle(CHARACTER_JULIUS)
	move_to_safe(CHARACTER_JULIUS, navpoint, 3, true, true)

	-- Make sure Julius isn't killed during the first move to
	if (follower_is_unconscious(CHARACTER_JULIUS)) then
		thread_yield()
		
		-- Pause the script from proceeding until Julius is revived
		while (follower_is_unconscious(CHARACTER_JULIUS)) do
			thread_yield()
		end

		-- Change Julius's state to idle before pathfinding
		npc_go_idle(CHARACTER_JULIUS)
		move_to_safe(CHARACTER_JULIUS, navpoint, 3, true, true)
	end
end

function em01_perimeter_west()
	-- Make sure notoriety spawning isn't happening
	notoriety_force_no_spawn("ultor", true)
	notoriety_force_no_spawn("police", true)

   -- Make sure the church is not abandoned
	CHURCH_ABANDON_PERIMETER_THREAD = thread_new("em01_church_abandonment_perimeter")

	-- Disable the defense trigger
	trigger_enable(DEFEND_CHURCH_TRIGGER, false)

	-- Make Julius independent
	follower_make_independent(CHARACTER_JULIUS, true)

	while (CHURCH_SCRIPT_PAUSED) do
		thread_yield()
	end

	-- Pause the script from proceeding until Julius is revived
	while (follower_is_unconscious(CHARACTER_JULIUS)) do
		thread_yield()
	end

	-- Show the groups
	group_show(GROUP_WEST_VEHICLES)
	group_show(GROUP_WEST_A)
	group_show(GROUP_WEST_B)

	-- Setup the callbacks
	for i = 1, Num_em01_ultor_west_a, 1 do
		on_death("em01_west_killed", Em01_ultor_west_a[i])
	end

	for i = 1, Num_em01_ultor_west_b, 1 do
		on_death("em01_west_killed", Em01_ultor_west_b[i])
	end

	-- Add the markers
	marker_add_group(GROUP_WEST_A, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)
	marker_add_group(GROUP_WEST_B, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)

	-- Teleport the humans into the vehicles
	vehicle_enter_group_teleport(Em01_ultor_west_a, WEST_SIDE_VEHICLE_1)
	vehicle_enter_group_teleport(Em01_ultor_west_b, WEST_SIDE_VEHICLE_2)

	-- Pathfind to their locations
	thread_new("em01_west_perimeter_vehicle_pathfind")

	-- Create the perimeter east group
	group_create_hidden(GROUP_EAST_VEHICLE)
	group_create_hidden(GROUP_EAST)

	-- Spawn the thread to move Julius to his destination
	thread_kill(JULIUS_MOVE_TO_POINT_THREAD)
	JULIUS_MOVE_TO_POINT_THREAD = thread_new("em01_move_julius_to_point", CHURCH_LEASH_WEST)

	-- Play Julius' help line
	audio_play_for_character("JULIUS_BONUS_REINFORCE", CHARACTER_JULIUS, "voice", false, true, 1, 2)

	-- Julius call out for the player
	mission_help_table("em01_defend_west")

	-- Add the objective
	objective_text(0, "em01_defend_objective", West_masako_kill_count, Num_em01_west_masako)
end

function em01_west_killed(human)
	-- Remove marker
	marker_remove_npc(human, SYNC_ALL)

	-- Add to the count
	West_masako_kill_count = West_masako_kill_count + 1

	-- Keep track of the total kills
	Num_em01_required_count = Num_em01_required_count + 1

	-- Update objective
	objective_text(0, "em01_defend_objective", West_masako_kill_count, Num_em01_west_masako)

	-- One assault human left so start the perimeter attack
	if (West_masako_kill_count == Num_em01_west_masako) then
		em01_clear_objective(false)
		em01_perimeter_east()
	end
end

function em01_east_perimeter_vehicle_pathfind()
	-- Do the pathfind...this function yields
	vehicle_pathfind_to(EAST_SIDE_VEHICLE, EAST_SIDE_ARRIVE, true, false)
	vehicle_stop(EAST_SIDE_VEHICLE)

	-- Vehicle has reached its destination then have the humans exit
	for i = 1, Num_em01_ultor_east, 1 do
		vehicle_exit(Em01_ultor_east[i])
		attack(Em01_ultor_east[i], CHARACTER_JULIUS)
	end
end

function em01_perimeter_east()

	while (CHURCH_SCRIPT_PAUSED) do
		thread_yield()
	end

	-- Pause the script from proceeding until Julius is revived
	while (follower_is_unconscious(CHARACTER_JULIUS)) do
		thread_yield()
	end

	-- Show groups
	group_show(GROUP_EAST_VEHICLE)
	group_show(GROUP_EAST)

	-- Setup callbacks
	for i = 1, Num_em01_ultor_east, 1 do
		on_death("em01_east_killed", Em01_ultor_east[i])
	end

	-- Add markers
	marker_add_group(GROUP_EAST, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)

	-- Teleport humans into the vehicles
	vehicle_enter_group_teleport(Em01_ultor_east, EAST_SIDE_VEHICLE)

	-- Pathfind
	thread_new("em01_east_perimeter_vehicle_pathfind")

	-- Create the perimeter west group
	group_create_hidden(GROUP_SOUTH_VEHICLES)
	group_create_hidden(GROUP_SOUTH_A)
	group_create_hidden(GROUP_SOUTH_B)

	-- Move Julius to his destination
	thread_kill(JULIUS_MOVE_TO_POINT_THREAD)
	JULIUS_MOVE_TO_POINT_THREAD = thread_new("em01_move_julius_to_point", CHURCH_LEASH_EAST)

	-- Play Julius' help line
	audio_play_for_character("JULIUS_BONUS_REINFORCE", CHARACTER_JULIUS, "voice", false, true, 0, 2)

	-- Julius call out for the player
	mission_help_table("em01_defend_east")

	-- Add objective
	objective_text(0, "em01_defend_objective", East_masako_kill_count, Num_em01_ultor_east)
end

function em01_east_killed(human)
	-- Remove marker
	marker_remove_npc(human, SYNC_ALL)

	-- Add to the count
	East_masako_kill_count = East_masako_kill_count + 1

	-- Keep track of the total kills
	Num_em01_required_count = Num_em01_required_count + 1

	-- Update objective
	objective_text(0, "em01_defend_objective", East_masako_kill_count, Num_em01_ultor_east)

	-- One assault human left so start the perimeter attack
	if (East_masako_kill_count == Num_em01_ultor_east) then
		em01_clear_objective(false)
		em01_perimeter_south()
	end
end

function em01_south_perimeter_vehicle_pathfind()
	-- Do the pathfind...this function yields
	vehicle_pathfind_to(SOUTH_SIDE_VEHICLE_2, SOUTH_SIDE_ARRIVE_2, true, false)
	vehicle_stop(SOUTH_SIDE_VEHICLE_2)

	-- Vehicle has reached its destination then have the humans exit
	for i = 1, Num_em01_ultor_south_b, 1 do
		vehicle_exit(Em01_ultor_south_b[i])
		attack(Em01_ultor_south_b[i], CHARACTER_JULIUS)
	end
	
	-- Do the pathfind...this function yields
	vehicle_pathfind_to(SOUTH_SIDE_VEHICLE_1, SOUTH_SIDE_ARRIVE_1, true, false)
	vehicle_stop(SOUTH_SIDE_VEHICLE_1)

	-- Vehicle has reached its destination then have the humans exit
	for i = 1, Num_em01_ultor_south_a, 1 do
		vehicle_exit(Em01_ultor_south_a[i])
		attack(Em01_ultor_south_a[i], CHARACTER_JULIUS)
	end
end

function em01_perimeter_south()

	while (CHURCH_SCRIPT_PAUSED) do
		thread_yield()
	end

	-- Pause the script from proceeding until Julius is revived
	while (follower_is_unconscious(CHARACTER_JULIUS)) do
		thread_yield()
	end

	-- Show the groups
	group_show(GROUP_SOUTH_VEHICLES)
	group_show(GROUP_SOUTH_A)
	group_show(GROUP_SOUTH_B)

	-- Setup the callbacks
	for i = 1, Num_em01_ultor_south_a, 1 do
		on_death("em01_south_killed", Em01_ultor_south_a[i])
	end

	for i = 1, Num_em01_ultor_south_b, 1 do
		on_death("em01_south_killed", Em01_ultor_south_b[i])
	end

	-- Add the markers
	marker_add_group(GROUP_SOUTH_A, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)
	marker_add_group(GROUP_SOUTH_B, MINIMAP_ICON_KILL, INGAME_EFFECT_KILL, SYNC_ALL)

	-- Teleport the humans into the vehicles
	vehicle_enter_group_teleport(Em01_ultor_south_a, SOUTH_SIDE_VEHICLE_1)
	vehicle_enter_group_teleport(Em01_ultor_south_b, SOUTH_SIDE_VEHICLE_2)

	-- Pathfind to their locations
	thread_new("em01_south_perimeter_vehicle_pathfind")

	-- Create the perimeter east group
	group_create_hidden(GROUP_HELI_VEHICLE)
	group_create_hidden(GROUP_HELI)

	-- Spawn the thread to move Julius to his destination
	thread_kill(JULIUS_MOVE_TO_POINT_THREAD)
	JULIUS_MOVE_TO_POINT_THREAD = thread_new("em01_move_julius_to_point", CHURCH_LEASH_SOUTH)

	-- Play Julius' help line
	audio_play_for_character("JULIUS_BONUS_REINFORCE", CHARACTER_JULIUS, "voice", false, true, 2, 2)

	-- Julius call out for the player
	mission_help_table("em01_defend_south")

	-- Add the objective
	objective_text(0, "em01_defend_objective", South_masako_kill_count, Num_em01_south_masako)
end

function em01_south_killed(human)
	-- Remove the marker
	marker_remove_npc(human, SYNC_ALL)

	-- Add to the count
	South_masako_kill_count = South_masako_kill_count + 1

	-- Keep track of the total kills
	Num_em01_required_count = Num_em01_required_count + 1

	-- Update the objective
	objective_text(0, "em01_defend_objective", South_masako_kill_count, Num_em01_south_masako)

	-- One assault human left so start the perimeter attack
	if (South_masako_kill_count == Num_em01_south_masako) then
		em01_perimeter_attack_heli()
		-- Do this after and not before since we want the helicopter to arrive ASAP
		em01_clear_objective(false)
	end
end

function em01_they_have_helicopters()
	-- Disable the trigger
	trigger_enable(TRIGGER_FALL_BACK, false)

	-- Play the helicopter exchange
	audio_play_conversation(Em01_heliwave1_exchange)

	-- Julius call out for the player to fall back
	mission_help_table_nag("em01_fall_back")

	delay(6.0)

	-- Give the objective
	mission_help_table("em01_heli")
end

function em01_fire_missile_at_church()
	-- Get the driver of the heli
	local	driver = vehicle_get_driver(HELI_ATTACK_VEHICLE)

   if ( driver == nil ) then
      return
   end

	-- We want this guy to have perfect aim
	if (not character_is_dead(driver)) then
		set_perfect_aim(driver, true)
	end
	
	-- Shoot at designated spots
	helicopter_shoot_navpoint(HELI_ATTACK_VEHICLE, "em01_$n141")	
	delay(0.25)
	helicopter_shoot_navpoint(HELI_ATTACK_VEHICLE, "em01_$n142")
	delay(0.25)
		helicopter_shoot_navpoint(HELI_ATTACK_VEHICLE, "em01_$n143")
	
	-- We want this guy to not have perfect aim
	if (not character_is_dead(driver)) then
		set_perfect_aim(driver, false)
	end
end

function em01_missile_attack_sequence(heli, target_humans, missile_sequence)
	-- Find the closest human...
	local		human_table = sizeof_table(target_humans)
	local		target_human = target_humans[1]
	local		human_dist = get_dist(target_human, heli)

	for i = 2, human_table, 1 do
		local		dist = get_dist(target_humans[i], heli)

		if (dist < human_dist) then
			target_human = target_humans[i]
			human_dist = dist
		end
	end

	-- Set the target and let the helicopter fire at them
	helicopter_go_to_set_target(heli, target_human)

	-- Wait for the heli to be facing the target human
	while (vehicle_exists(heli) and character_exists(target_human) and not fov_check_xz_plane(heli, target_human, 10.0)) do
		thread_yield()
	end

	-- Peform missile sequence
	local		seq_table = sizeof_table(missile_sequence)

	-- Get pilot
	local		driver = vehicle_get_driver(heli)
   if ( driver == nil ) then
      return
   end

	for i = 1, seq_table, 1 do
		-- If the heli is not destroyed and the driver is not dead then perform a missile fire
		if (not vehicle_is_destroyed(heli) and not character_is_dead(driver)) then

			-- Fire a missile at the human now
			set_perfect_aim(driver, true)
			helicopter_shoot_human(heli, target_human, true, HELI_FIRE_MULTIPLIER)
			set_perfect_aim(driver, false)
		end

		-- Delay before firing again
		delay(missile_sequence[i])
	end
end

function em01_heli_perimeter_vehicle_pathfind()
	-- Change the amount of damage the vehicle can take
	if (not IN_COOP) then
		set_max_hit_points(HELI_ATTACK_VEHICLE, get_max_hit_points(HELI_ATTACK_VEHICLE) * 4, true)
	else
		set_max_hit_points(HELI_ATTACK_VEHICLE, get_max_hit_points(HELI_ATTACK_VEHICLE) * 8, true)
	end

	-- Setup the fallback trigger
	trigger_enable(TRIGGER_FALL_BACK, true)
	on_trigger("em01_they_have_helicopters", TRIGGER_FALL_BACK)

	-- Setup the shoot navpoint trigger
	trigger_enable(TRIGGER_BLAST_CHURCH, true)
	on_trigger("em01_fire_missile_at_church", TRIGGER_BLAST_CHURCH)

	-- Teleport the heli to the start path point
	teleport_vehicle(HELI_ATTACK_VEHICLE, "em01_$n003")

	-- Do the pathfind...this function yields
	helicopter_fly_to_direct(HELI_ATTACK_VEHICLE, 40.0, HELI_ATTACK_ARRIVE)

	-- Just shoot guns, rockets are scripted
	helicopter_set_missile_chance(HELI_ATTACK_VEHICLE, 0.01)

	-- Maintain a pattern and attack a various points
	em01_execute_pattern()
end

function em01_execute_pattern()
	-- Keep track of what index we are at
	local		pattern_idx = 1
	local		use_jump_path = false

	while (1) do
      
      thread_yield()

		-- Exit if the vehicle is destroyed
		if (vehicle_is_destroyed(HELI_ATTACK_VEHICLE)) then
			return
		end

		local		smoke, fire = vehicle_get_smoke_and_fire_state(HELI_ATTACK_VEHICLE)

		-- If the helicopter is on fire then just exit
		if (fire) then
			return
		end

		-- Have the helicopter fly to the next point in the list
		if (not use_jump_path) then
			helicopter_fly_to_direct(HELI_ATTACK_VEHICLE, 50.0, Em01_heli_path[pattern_idx])
		else
			helicopter_fly_to_direct(HELI_ATTACK_VEHICLE, 35.0, Em01_heli_jump_church[Em01_heli_path[pattern_idx]][HELI_JUMP_PATTERN])
			-- Update the pattern idx to the correct position
			pattern_idx = Em01_heli_jump_church[Em01_heli_path[pattern_idx]][HELI_JUMP_INDEX]
		end

		smoke, fire = vehicle_get_smoke_and_fire_state(HELI_ATTACK_VEHICLE)

		-- If the helicopter is on fire then just exit
		if (fire) then
			return
		else
			-- Fire a sequence of missiles
			if (not IN_COOP) then
				em01_missile_attack_sequence(HELI_ATTACK_VEHICLE, {LOCAL_PLAYER, CHARACTER_JULIUS}, Em01_missile_sequence[rand_int(1, Num_em01_missile_sequence)])
			else
				em01_missile_attack_sequence(HELI_ATTACK_VEHICLE, {LOCAL_PLAYER, CHARACTER_JULIUS, REMOTE_PLAYER}, Em01_missile_sequence[rand_int(1, Num_em01_missile_sequence)])
			end
		end

		local		heli_can_jump_church = Em01_heli_can_jump[pattern_idx]
		local		heli_should_jump_church = ((rand_int(0, 2) == 0) and not use_jump_path)

		if (heli_can_jump_church and heli_should_jump_church) then
			-- We want to use the alternate path...
			use_jump_path = true
		else
			-- We want the pattern to circle around
			pattern_idx = pattern_idx + 1
			if (pattern_idx > Num_em01_heli_path) then
				pattern_idx = 1
			end

			use_jump_path = false
		end
	end
end

function em01_perimeter_attack_heli()
	-- Julius is no longer independent
	follower_make_independent(CHARACTER_JULIUS, false)

	-- Wait until we can actually execute the script
	while (CHURCH_SCRIPT_PAUSED) do
		thread_yield()
	end

	-- Show the groups
	group_show(GROUP_HELI_VEHICLE)
	group_show(GROUP_HELI)

	-- Setup the callbacks
	on_vehicle_destroyed("em01_heli_killed", HELI_ATTACK_VEHICLE)

	-- Add the marker
	marker_add_group(GROUP_HELI_VEHICLE, MINIMAP_ICON_KILL, INGAME_EFFECT_VEHICLE_KILL, SYNC_ALL)

	-- Teleport the humans into the vehicle
	vehicle_enter_group_teleport(Em01_ultor_heli, HELI_ATTACK_VEHICLE)

	-- Pathfind to their locations
	thread_new("em01_heli_perimeter_vehicle_pathfind")

	-- Create the next group
	group_create_hidden(GROUP_JULIUS_VEHICLE)

	-- Spawn off a thread to check if the player and Julius can make a run for it
	thread_new("em01_can_run_for_it")

	-- Load our ultor strike scenario
	group_create_hidden(GROUP_WEAK_VEHICLE)
	group_create_hidden(GROUP_WEAK)
	group_create_hidden(GROUP_LAMB)
	group_create_hidden(GROUP_LAMB_A)
	group_create_hidden(GROUP_LAMB_B)
end

function em01_heli_killed(vehicle)
	-- Remove the marker
	marker_remove_vehicle(vehicle, SYNC_ALL)

	Heli_masako_killed = true
end

function em01_can_run_for_it()

	while (1) do
		thread_yield()

      local masako_killed = (Num_em01_required_count == Num_em01_required_kills)
      local julius_alive = (not character_is_dead(CHARACTER_JULIUS))

      -- The heli has to be destroyed, the masako must be killed, and Julius can't be dead.
      if (Heli_masako_killed and masako_killed and julius_alive) then

         -- Spawn off a thread to release the previous groups.
         thread_new("em01_defend_release_to_world")

         -- Checkpoint
         mission_set_checkpoint("perimeter")

         -- We no longer need to protect the church
         thread_kill(CHURCH_ABANDON_PERIMETER_THREAD)

         em01_church_abandon_perimeter_cleanup()

         -- Delay
         delay(2.0)

         em01_make_a_run_for_it()
         return

		end
	end
end

function em01_defend_release_to_world()

	release_to_world(GROUP_HELI_VEHICLE)
	release_to_world(GROUP_WEST_VEHICLES)
	release_to_world(GROUP_EAST_VEHICLE)
	release_to_world(GROUP_SOUTH_VEHICLES)

	-- Wait a frame
	thread_yield()

	release_to_world(GROUP_ASSAULT_1)
	release_to_world(GROUP_ASSAULT_2)

	thread_yield()

	release_to_world(GROUP_DEFEND_1)
	release_to_world(GROUP_DEFEND_2_A)
	release_to_world(GROUP_DEFEND_2_B)
	release_to_world(GROUP_DEFEND_2_C)

	thread_yield()

	release_to_world(GROUP_WEST_A)
	release_to_world(GROUP_WEST_B)
	release_to_world(GROUP_EAST)

	thread_yield()

	release_to_world(GROUP_SOUTH_A)
	release_to_world(GROUP_SOUTH_B)
	release_to_world(GROUP_HELI)
end

function em01_make_a_run_for_it()
	-- Play the helicopter death exchange
	audio_play_conversation(Em01_helideath_exchange)

	-- Julius says lets make a run for it
	mission_help_table("em01_run_for_it")

	-- Add waypoint to Julius' car
	waypoint_add(JULIUS_VEHICLE, SYNC_ALL)
	-- Show Julius' car
	group_show(GROUP_JULIUS_VEHICLE)
	-- Add a marker
	marker_add_vehicle(JULIUS_VEHICLE, MINIMAP_ICON_LOCATION, INGAME_EFFECT_VEHICLE_INTERACT, SYNC_ALL)

	-- Setup the enter/exit callback
	on_vehicle_enter("em01_vehicle_entered", JULIUS_VEHICLE)
	on_vehicle_exit("em01_vehicle_exited", JULIUS_VEHICLE)

	-- The vehicle cannot be destroyed
	on_vehicle_destroyed("em01_vehicle_destroyed", JULIUS_VEHICLE)

   em01_setup_julius_vehicle()

	-- Spawn off a thread to watch when Julius can start to drive
	thread_new("em01_wait_for_car_load")

	-- Don't let npcs exit until we say so
	vehicle_suppress_npc_exit(JULIUS_VEHICLE, true) 
	-- Don't let the vehicle move until we say so
	vehicle_speed_override(JULIUS_VEHICLE, 0.1)
	
	-- Delay for a little while to allow the player some time to make the run for it
	delay(20.0)

	if (LAMB_TO_THE_SLAUGHTER_THREAD == INVALID_THREAD_HANDLE) then
		thread_new("em01_exploring_is_weak_sauce")
	end
end

function em01_setup_julius_vehicle()
	-- Make sure the vehicle is not entered...we just want to teleport everyone
	set_unjackable_flag(JULIUS_VEHICLE, true)
   vehicle_prevent_explosion_fling( JULIUS_VEHICLE, true )
	-- Give Julius' car infinite mass
	vehicle_infinite_mass(JULIUS_VEHICLE, true)
	-- Don't let weapon fire flatten the tires
	vehicle_never_flatten_tires(JULIUS_VEHICLE, true)
end

-- Wait for the turret sequence to begin.
function em01_wait_for_car_load()

	-- Conditions which must be met before turret sequence begins:
	local		local_close_enough = false
	local		remote_close_enough = false
   local    julius_alive = false
   repeat
      thread_yield()
		local_close_enough = (get_dist_char_to_vehicle(LOCAL_PLAYER, JULIUS_VEHICLE) <= 10.0)
		if (IN_COOP) then
			remote_close_enough = (get_dist_char_to_vehicle(REMOTE_PLAYER, JULIUS_VEHICLE) <= 10.0)
		end
      julius_alive = not character_is_dead( CHARACTER_JULIUS )
   until ( julius_alive and local_close_enough and ( coop_is_active() == false or remote_close_enough ) )

	-- Don't do anything while either player is dead
	while( character_is_dead(LOCAL_PLAYER) or (IN_COOP and character_is_dead(REMOTE_PLAYER)) ) do
		thread_yield()
	end

   -- Don't let Julius die while controls are disabled
   turn_invulnerable( CHARACTER_JULIUS)

	-- Fade out and disable player controls
	fade_out(1.0,{0,0,0}, SYNC_ALL)
	fade_out_block()
	player_controls_disable(LOCAL_PLAYER)
	if (IN_COOP) then
		player_controls_disable(REMOTE_PLAYER)
	end
   Controls_disabled = true
	delay(0.5)

   -- Eliminate waypoint
   waypoint_remove( SYNC_ALL)

	-- Place the people in the vehicle
	em01_setup_turret_occupants()

	-- Don't allow the player(s) to exit
	set_player_can_enter_exit_vehicles(LOCAL_PLAYER, false)
	if (IN_COOP) then
		set_player_can_enter_exit_vehicles(REMOTE_PLAYER, false)
	end

	-- Fade back into the game
	fade_in(1.0, SYNC_ALL)
	fade_in_block()

	-- Reenable player controls
	player_controls_enable(LOCAL_PLAYER)
	if (IN_COOP) then
		player_controls_enable(REMOTE_PLAYER)
	end
	Controls_disabled = false

   -- Now that Julius is in the car, make him vulnerable again
   turn_vulnerable( CHARACTER_JULIUS )

	-- Proceed to the next phase
	em01_protect_julius_car()

end

function em01_setup_turret_occupants()

	-- Make sure that the players aren't in a vehicle
	character_evacuate_from_all_vehicles(LOCAL_PLAYER)
   character_evacuate_from_all_vehicles(CHARACTER_JULIUS)
	character_evacuate_from_all_vehicles(REMOTE_PLAYER)
	delay(0.5)

	-- Teleport the players to a staging area, removing them from vehicles along the way.
	teleport_coop("em01_$c028", "em01_$c029", true)

   -- Make players invulnerable
   turn_invulnerable(LOCAL_PLAYER, false)
   if (IN_COOP) then
      turn_invulnerable(REMOTE_PLAYER, false)
   end

	-- Dismiss any extra followers and put Julius in the player's party
	party_dismiss_all() 
   party_add(CHARACTER_JULIUS,LOCAL_PLAYER)
   follower_make_independent(CHARACTER_JULIUS, true)
   character_set_unrevivable(CHARACTER_JULIUS, true)

	-- Place characters in the vehicle
	set_unjackable_flag(JULIUS_VEHICLE, false)
   vehicle_enter_teleport(CHARACTER_JULIUS,JULIUS_VEHICLE,0)
	vehicle_enter_teleport(LOCAL_PLAYER,JULIUS_VEHICLE,1)
	if IN_COOP then
		vehicle_enter_teleport(REMOTE_PLAYER,JULIUS_VEHICLE,2)
	end

	-- Wait for characters to enter the vehicle
   em01_wait_for_vehicle_entry(CHARACTER_JULIUS, JULIUS_VEHICLE, 0)
	em01_wait_for_vehicle_entry(LOCAL_PLAYER, JULIUS_VEHICLE, 1)
	if IN_COOP then
		em01_wait_for_vehicle_entry(REMOTE_PLAYER, JULIUS_VEHICLE, 2)
		delay(1.0)
	end

   -- Make the vehicle unjackable again
	set_unjackable_flag(JULIUS_VEHICLE, true)

   -- Make players vulnerable
   turn_vulnerable(LOCAL_PLAYER)
   if (coop_is_active()) then
      turn_vulnerable(REMOTE_PLAYER)
   end

end

-- Wait for a character to enter a vehicle. Blocks 
function em01_wait_for_vehicle_entry(character, vehicle, seat_index)

	local delay_length = 0.25
	if (character == REMOTE_PLAYER) then
		delay_length = 0.5
	end

	local teleport_attempts = 1
	while( (not character_is_in_vehicle(character, vehicle)) and (teleport_attempts < 5) ) do
		teleport_attempts = teleport_attempts + 1
		vehicle_enter_teleport(character, vehicle, seat_index)
		delay(delay_length)
	end

end


function em01_vehicle_entered(human)
	-- Need to know who we are dealing with
	local		sync = SYNC_LOCAL

	if (human == REMOTE_PLAYER) then
		sync = SYNC_REMOTE
	end

	-- Remove the vehicle
	marker_remove_vehicle(JULIUS_VEHICLE, sync)
end

function em01_vehicle_exited(human)
	-- Need to know who we are dealing with
	local		sync = SYNC_LOCAL

	if (human == REMOTE_PLAYER) then
		sync = SYNC_REMOTE
	end

	-- Add a marker
	marker_add_vehicle(JULIUS_VEHICLE, MINIMAP_ICON_LOCATION, INGAME_EFFECT_VEHICLE_INTERACT, sync)
end

function em01_exploring_is_weak_sauce()
	-- Show our groups
	group_show(GROUP_WEAK_VEHICLE)
	group_show(GROUP_WEAK)

	-- Teleport the humans into the vehicle
	vehicle_enter_group_teleport(Em01_weak_sauce_heli, WEAK_HELI_VEHICLE)

	--Make sure chase helis don't fire rockets at the player
	helicopter_set_missile_chance(WEAK_HELI_VEHICLE, 0.001)
	
	-- Setup a callback for death
	on_vehicle_destroyed("em01_weak_destroyed", WEAK_HELI_VEHICLE)

	-- Go into chase mode
	vehicle_chase(WEAK_HELI_VEHICLE, LOCAL_PLAYER)
end

function em01_weak_destroyed()
	-- Release to the world
	release_to_world(GROUP_WEAK_VEHICLE)
	release_to_world(GROUP_WEAK)

	-- Invalidate the destroyed callback
	on_vehicle_destroyed("", WEAK_HELI_VEHICLE)

	-- Re-create the groups
	group_create_hidden(GROUP_WEAK_VEHICLE)
	group_create_hidden(GROUP_WEAK)
end

function em01_lamb_to_the_slaughter()
	-- Show our groups
	group_show(GROUP_LAMB)
	group_show(GROUP_LAMB_A)
	group_show(GROUP_LAMB_B)

	-- Turn on the siren lights for all the road blocks...
	for i = 1, Num_em01_siren_lights, 1 do
		vehicle_set_sirenlights(Em01_siren_lights[i], true)
	end

	-- Teleport the humans into the vehicle
	vehicle_enter_group_teleport(Em01_lamb_apc_a, LAMB_A_VEHICLE)
	vehicle_enter_group_teleport(Em01_lamb_apc_b, LAMB_B_VEHICLE)

	-- Setup the ram scenario
	trigger_enable(APC_RAM1_TRIGGER, true)
	trigger_enable(APC_RAM2_TRIGGER, true)
	on_trigger("em01_apc_ram1", APC_RAM1_TRIGGER)
	on_trigger("em01_apc_ram2", APC_RAM2_TRIGGER)

	-- Remove the callback for the helicopter
	on_vehicle_destroyed("", WEAK_HELI_VEHICLE)

end

function em01_apc_ram1()
	-- Disable the trigger
	trigger_enable(APC_RAM1_TRIGGER, false)
	-- Change to navmesh pathfinding
	vehicle_pathfind_to(LAMB_A_VEHICLE, APC_RAM_PATH_A, true, false)
	-- Go into chase mode
	vehicle_chase(LAMB_A_VEHICLE, LOCAL_PLAYER, false, true, false, true)
end

function em01_apc_ram2()
	-- Disable the trigger
	trigger_enable(APC_RAM2_TRIGGER, false)
	-- Change to navmesh pathfinding
	vehicle_pathfind_to(LAMB_B_VEHICLE, APC_RAM_PATH_B, true, false)
	-- Go into chase mode
	vehicle_chase(LAMB_B_VEHICLE, LOCAL_PLAYER, false, true, false, true)
end

function em01_julius_car_health_watch()
	local		last_percentage = get_current_hit_points(JULIUS_VEHICLE) / get_max_hit_points(JULIUS_VEHICLE)
	local		percentage

	-- Only watch the vehicle while it is not destroyed
	while (not vehicle_is_destroyed(JULIUS_VEHICLE)) do
		thread_yield()

		percentage = get_current_hit_points(JULIUS_VEHICLE) / get_max_hit_points(JULIUS_VEHICLE)

		if ((percentage <= 0.75) and (last_percentage > 0.75)) then
			-- Play audio line for Julius
			audio_play_for_character("JULIUS_BONUS_CAR75_01", CHARACTER_JULIUS, "voice")
		elseif ((percentage <= 0.25) and (last_percentage > 0.25)) then
			-- Play audio line for Julius
			audio_play_for_character("JULIUS_BONUS_CAR25_01", CHARACTER_JULIUS, "voice")
			-- This thread is no longer needed...
			return
		end

		-- Update the last_percentage variable
		last_percentage = percentage
	end
end

function em01_protect_vehicle_exchange()
	-- Play the vehicle exchange
	audio_play_conversation(Em01_vehicle_exchange)

	-- Instruct player to protect the car
	mission_help_table("em01_protect_car")
end

function em01_path_chase_setup()
   for index, group_name in pairs( PATH_CHASE_GROUPS ) do
      group_create_hidden( group_name )
   end

   for index, trigger_name in pairs( PATH_CHASE_TRIGGERS ) do
      trigger_enable( trigger_name, true )
      on_trigger( "em01_hit_path_chase_trigger", trigger_name )
   end
end

function em01_index_from_trigger_name( trigger_name_to_match )
   for index, trigger_name in pairs( PATH_CHASE_TRIGGERS ) do
      if ( trigger_name == trigger_name_to_match ) then
         return index
      end
   end
   return -1
end

function em01_hit_path_chase_trigger( triggerer_name, trigger_name )
   trigger_enable( trigger_name, false )
   if ( Num_path_chase_vehicles_chasing < MAX_NUM_PATH_CHASERS ) then
      local trigger_index = em01_index_from_trigger_name( trigger_name )

      script_assert( trigger_index ~= -1, "failed to find trigger's index" )

      if ( trigger_index ~= nil ) then
         mission_debug( "trigger index = "..trigger_index.."." )
      else
         mission_debug( "trigger index = nil." )
      end

      group_show( PATH_CHASE_GROUPS[trigger_index] )

      thread_new( "em01_path_chase_vehicle_chase", trigger_index )
   end
end

function em01_path_chase_vehicle_chase( path_chase_index )
   Num_path_chase_vehicles_chasing = Num_path_chase_vehicles_chasing + 1

   local path_chase_vehicle = PATH_CHASE_VEHICLES[path_chase_index]
   local path_chase_passengers = PATH_CHASE_PASSENGERS[path_chase_index]

   mission_debug( "group teleport for group index "..path_chase_index )
   vehicle_enter_group_teleport( path_chase_passengers, path_chase_vehicle )

   vehicle_chase( path_chase_vehicle, LOCAL_PLAYER, false, true, false, true)

   -- Keep checking this vehicle for destruction
   local driver = "none"
   local vehicle_destroyed = false
   repeat
      driver = vehicle_get_driver( path_chase_vehicle )
      vehicle_destroyed = vehicle_is_destroyed( path_chase_vehicle )
      thread_yield()
   until ( driver == nil or vehicle_destroyed )

   Num_path_chase_vehicles_chasing = Num_path_chase_vehicles_chasing - 1
end

function em01_protect_julius_car()
	-- Spawn off a thread to do the audio exchange and show the message
	thread_new("em01_protect_vehicle_exchange")

	-- Create our groups for when we are almost across the bridge
	group_create_hidden(GROUP_OH_SHIT_VEH_A)
	group_create_hidden(GROUP_OH_SHIT_VEH_B)
	group_create_hidden(GROUP_OH_SHIT_A)
	group_create_hidden(GROUP_OH_SHIT_B)
	

	-- Give the player the rocket launcher
	inv_weapon_add_temporary(LOCAL_PLAYER, "rpg_annihilator", 1, true)
   inv_weapon_add_temporary(LOCAL_PLAYER, "AR50_launcher", 1 ,true )
   inv_weapon_add_temporary(LOCAL_PLAYER, "grenade", 1, true )
	inv_item_equip("AR50_launcher", LOCAL_PLAYER)
	if (IN_COOP) then
		inv_weapon_add_temporary(REMOTE_PLAYER, "rpg_annihilator", 1, true)
      inv_weapon_add_temporary(REMOTE_PLAYER, "AR50_launcher", 1 ,true )
      inv_weapon_add_temporary(REMOTE_PLAYER, "grenade", 1, true )
		inv_item_equip("AR50_launcher", REMOTE_PLAYER)
	end
	Players_have_temp_weapons = true

	-- Enable the trigger to disable notoriety spawning
	trigger_enable(PEEL_OFF_TRIGGER, true)
	on_trigger("em01_peel_off", PEEL_OFF_TRIGGER)

	-- Enable the trigger to re-enable notoriety behaviors
	trigger_enable(GOTCHA_TRIGGER, true)
	on_trigger("em01_gotcha_heli", GOTCHA_TRIGGER)

   em01_path_chase_setup()

	-- The massive ultor strike
	LAMB_TO_THE_SLAUGHTER_THREAD = thread_new("em01_lamb_to_the_slaughter")

	-- Play Julius lines about this car health
	JULIUS_VEHICLE_HEALTH_WATCH = thread_new("em01_julius_car_health_watch")

	-- Drive like a mad man
	vehicle_speed_override(JULIUS_VEHICLE, 40.0 )
	-- Change the amount of damage the vehicle can take
	set_max_hit_points(JULIUS_VEHICLE, get_max_hit_points(JULIUS_VEHICLE) * 3, true)

	-- Show the damage bar
	damage_indicator_on(0, JULIUS_VEHICLE, 0.0, "em01_bar_label", get_max_hit_points(JULIUS_VEHICLE), SYNC_ALL)

	-- Drive to the museum
	vehicle_turret_base_to(JULIUS_VEHICLE, ESCAPE_ROUTE_PATH_A, false)
	vehicle_turret_base_to(JULIUS_VEHICLE, ESCAPE_ROUTE_PATH_B, false)

	-- Kill speed override
	vehicle_speed_cancel(JULIUS_VEHICLE)
	-- Change to navmesh pathfinding
	vehicle_pathfind_to(JULIUS_VEHICLE, CRASH_ROUTE_PATH, true, false)

	-- Turn the damage bar off
	damage_indicator_off(0)

	-- Remove callbacks on Julius' car
	on_vehicle_enter("", JULIUS_VEHICLE)
	on_vehicle_exit("", JULIUS_VEHICLE)
	on_vehicle_destroyed("", JULIUS_VEHICLE)

	-- Julius can exit the vehicle
	vehicle_suppress_npc_exit(JULIUS_VEHICLE, false) 

	-- Let the player(s) exit
	set_player_can_enter_exit_vehicles(LOCAL_PLAYER, true)
	if (IN_COOP) then
		set_player_can_enter_exit_vehicles(REMOTE_PLAYER, true)
	end

	-- Wait until the vehicle is not moving very fast
	while (get_vehicle_speed(JULIUS_VEHICLE) > 5.0) do
		thread_yield()
	end

	-- Make Julius exit the car
	vehicle_exit(CHARACTER_JULIUS)

	-- Kill the health watch thread for Julius's car
	thread_kill(JULIUS_VEHICLE_HEALTH_WATCH)

	-- Checkpoint
	mission_set_checkpoint("last stand")

	-- Show the objective to the player
	em01_last_stand_objective()
end

function em01_blast_oh_shit()
	-- Disable this trigger as it is no longer needed
	trigger_enable(TRIGGER_BLAST_OH_SHIT, false)

	Em01_oh_shit_fire_now = true
end

function em01_peel_off()
	-- Disable this trigger
	trigger_enable(PEEL_OFF_TRIGGER, false)

	-- Peel off the scripted attacks
	vehicle_flee(LAMB_A_VEHICLE, LOCAL_PLAYER)
	vehicle_flee(LAMB_B_VEHICLE, LOCAL_PLAYER)

	local		smoke, fire = vehicle_get_smoke_and_fire_state(WEAK_HELI_VEHICLE)

	if (not fire and not vehicle_is_destroyed(WEAK_HELI_VEHICLE)) then
		-- Can't make a helicopter flee like this...so just have him chase a fleeing apc
		vehicle_chase(WEAK_HELI_VEHICLE, Em01_lamb_apc_a[1], false, true, false, true)
	end

	-- Show our groups for the "Oh Shit" moment
	group_show(GROUP_OH_SHIT_VEH_A)
	group_show(GROUP_OH_SHIT_VEH_B)
	group_show(GROUP_OH_SHIT_A)
	group_show(GROUP_OH_SHIT_B)

	-- Teleport the humans into the vehicle
	vehicle_enter_group_teleport(Em01_oh_shit_heli_a, OH_SHIT_HELI_A)
	vehicle_enter_group_teleport(Em01_oh_shit_heli_b, OH_SHIT_HELI_B)

	trigger_enable(TRIGGER_BLAST_OH_SHIT, true)
	on_trigger("em01_blast_oh_shit", TRIGGER_BLAST_OH_SHIT)
	
	--Make sure chase helis don't fire rockets at the player
	helicopter_set_missile_chance(OH_SHIT_HELI_A, 0.001)
	helicopter_set_missile_chance(OH_SHIT_HELI_B, 0.001)

	-- Spawn off threads to run the pathfinding
	thread_new("em01_oh_shit_a_execute")
	thread_new("em01_oh_shit_b_execute")

	-- Delay for a short period of time
	delay(1.0)

	-- Play a line for the local player
	audio_play_for_character("PLAYER_BONUS_GOONS_01", LOCAL_PLAYER, "voice")
end

function em01_gotcha_heli()
	-- Disable this trigger
	trigger_enable(GOTCHA_TRIGGER, false)

	-- Once we have gotten to this point, flattening of the tires is acceptable
	vehicle_never_flatten_tires(JULIUS_VEHICLE, false) 

	-- Release these groups to the world now...
	release_to_world(GROUP_WEAK_VEHICLE)
	release_to_world(GROUP_WEAK)
	release_to_world(GROUP_LAMB)
	release_to_world(GROUP_LAMB_A)
	release_to_world(GROUP_LAMB_B)

	-- Create our groups for the final showdown
	group_create_hidden(GROUP_STAND_VEH_A)
	group_create_hidden(GROUP_STAND_VEH_B)
	group_create_hidden(GROUP_STAND_A)
	group_create_hidden(GROUP_STAND_B)
	group_create_hidden(GROUP_ROADBLOCK)

	-- Enable the last stand trigger
	trigger_enable(LAST_STAND_SHOW_TRIGGER, true)
	on_trigger("em01_last_stand_show", LAST_STAND_SHOW_TRIGGER)

	trigger_enable(LAST_STAND_EXECUTE_TRIGGER, true)
	on_trigger("em01_last_stand_attack", LAST_STAND_EXECUTE_TRIGGER)
end

function em01_oh_shit_a_execute()
	-- Setup a callback for death
	on_vehicle_destroyed("em01_heli_destroyed", OH_SHIT_HELI_A)

	-- Teleport the helicopter to the starting location
	teleport_vehicle(OH_SHIT_HELI_A, HELI_ATTACK_POINT_1)

	-- Pathfind above the heightmap
	helicopter_fly_to_direct(OH_SHIT_HELI_A, 30.0, OH_SHIT_PATH_A)

	-- Wait until the player is where we want them to be
	while (not Em01_oh_shit_fire_now) do
		thread_yield()
	end

	local driver = vehicle_get_driver(OH_SHIT_HELI_A)

   if ( driver == nil ) then
      return
   end

	if (not vehicle_is_destroyed(OH_SHIT_HELI_A) and not character_is_dead(driver)) then
		-- We want this guy to have perfect aim
		set_perfect_aim(driver, true)
		-- Fire a rocket at the player(s)
		helicopter_shoot_navpoint(OH_SHIT_HELI_A, "em01_$n137")
		-- We want this guy to not have perfect aim
		set_perfect_aim(driver, false)

		-- Go into chase mode
		vehicle_chase(OH_SHIT_HELI_A, LOCAL_PLAYER)
	end
end

function em01_oh_shit_b_execute()
	-- Setup a callback for death
	on_vehicle_destroyed("em01_heli_destroyed", OH_SHIT_HELI_B)

	-- Teleport the helicopter to the starting location
	teleport_vehicle(OH_SHIT_HELI_B, HELI_ATTACK_POINT_2)

	-- Pathfind above the heightmap
	helicopter_fly_to_direct(OH_SHIT_HELI_B, 15.0, OH_SHIT_PATH_B)

	-- Wait until the player is where we want them to be
	while (not Em01_oh_shit_fire_now) do
		thread_yield()
	end

	local driver = vehicle_get_driver(OH_SHIT_HELI_B)

   if ( driver == nil ) then
      return
   end

	if (not vehicle_is_destroyed(OH_SHIT_HELI_B) and not character_is_dead(driver)) then
		-- We want this guy to have perfect aim
		set_perfect_aim(driver, true)
		-- Fire a rocket at the player(s)
		helicopter_shoot_navpoint(OH_SHIT_HELI_B, "em01_$n138")
		-- We want this guy to not have perfect aim
		set_perfect_aim(driver, false)

		-- Go into chase mode
		vehicle_chase(OH_SHIT_HELI_B, LOCAL_PLAYER)
	end
end

function em01_heli_destroyed(heli)
	-- Need to know who we are dealing with
	if (heli == OH_SHIT_HELI_A) then
		-- Release to the world
		release_to_world(GROUP_OH_SHIT_A)
		release_to_world(GROUP_OH_SHIT_VEH_A)

	else
		-- Release to the world
		release_to_world(GROUP_OH_SHIT_B)
		release_to_world(GROUP_OH_SHIT_VEH_B)

	end
end

function em01_last_stand_show(human)
	-- Disable the trigger...it is no longer needed
	trigger_enable(LAST_STAND_SHOW_TRIGGER, false)

	-- Make Julius independent...he might be already but do this for safety
	follower_make_independent(CHARACTER_JULIUS, true)

	-- Teleport the vehicles first...
   teleport_vehicle(LAST_STAND_HELI_A, LAST_STAND_POINT_A)
   teleport_vehicle(LAST_STAND_HELI_B, LAST_STAND_POINT_B)

   -- Show the groups
   group_show(GROUP_STAND_VEH_A)
   group_show(GROUP_STAND_VEH_B)

   if ( human ~= nil ) then
      group_show(GROUP_STAND_A)
      group_show(GROUP_STAND_B)
      group_show(GROUP_ROADBLOCK)
   end

	-- Teleport the humans into the vehicle
	vehicle_enter_group_teleport(Em01_last_stand_heli_a, LAST_STAND_HELI_A)
	vehicle_enter_group_teleport(Em01_last_stand_heli_b, LAST_STAND_HELI_B)
	
	--Make sure chase helis don't fire rockets at the player
	helicopter_set_missile_chance(LAST_STAND_HELI_A, 0.001)
	helicopter_set_missile_chance(LAST_STAND_HELI_B, 0.001)

	-- Setup the death callbacks
	on_vehicle_destroyed("em01_last_stand_heli_destroyed", LAST_STAND_HELI_A)
	on_vehicle_destroyed("em01_last_stand_heli_destroyed", LAST_STAND_HELI_B)

	-- Change the amount of damage the vehicles can take
	if (not IN_COOP) then
		set_max_hit_points(LAST_STAND_HELI_A, get_max_hit_points(LAST_STAND_HELI_A) * 4, true)
		set_max_hit_points(LAST_STAND_HELI_B, get_max_hit_points(LAST_STAND_HELI_B) * 4, true)
	else
		set_max_hit_points(LAST_STAND_HELI_A, get_max_hit_points(LAST_STAND_HELI_A) * 8, true)
		set_max_hit_points(LAST_STAND_HELI_B, get_max_hit_points(LAST_STAND_HELI_B) * 8, true)
	end

   -- Set the show variable
	LAST_STAND_SHOW_PERFORMED = true
   
   if ( human == nil ) then
      mission_debug( "objective!" )
      -- We are continuing from a checkpoint, show the objective to the player
      em01_last_stand_objective()
   end
end

function em01_last_stand_attack(human)
	-- Disable the trigger...it is no longer needed
	trigger_enable(LAST_STAND_EXECUTE_TRIGGER, false)

	-- Wait until the show has been performed...
	while (not LAST_STAND_SHOW_PERFORMED) do
		thread_yield()
	end
	-- If the "oh shit" helicopters still exist then have them flee
	if (vehicle_exists(OH_SHIT_HELI_A)) then
		vehicle_flee(OH_SHIT_HELI_A)
	end

	if (vehicle_exists(OH_SHIT_HELI_B)) then
		vehicle_flee(OH_SHIT_HELI_B)
	end

   -- Allow some time for the players to find cover before attacking with the helicopters
   if ( human ~= nil ) then
      delay( 15.0 )
   end

	-- Add the marker
	marker_add_vehicle(LAST_STAND_HELI_A, MINIMAP_ICON_KILL, INGAME_EFFECT_VEHICLE_KILL, SYNC_ALL)
	marker_add_vehicle(LAST_STAND_HELI_B, MINIMAP_ICON_KILL, INGAME_EFFECT_VEHICLE_KILL, SYNC_ALL)

	-- Fire off the threads to execute the helicopter patterns
	thread_new("em01_last_stand_a", human)
	thread_new("em01_last_stand_b", human)

   -- Mark the last stand attack as started
   LAST_STAND_ATTACK_STARTED = true
end

function em01_last_stand_objective()
   -- Wait until the attack has started before giving the objective
   repeat
      thread_yield()
   until ( LAST_STAND_ATTACK_STARTED )
   mission_debug( "objective done delaying" )

	-- Play the helicopter wave2 exchange
	audio_play_conversation(Em01_heliwave2_exchange)

	-- Give the directive and show the objective
	mission_help_table("em01_last_stand")
	objective_text(0, "em01_ls_objective", Em01_last_stand_heli_killed, 2, SYNC_ALL)

	-- Spawn off a thread to move Julius around
	thread_new("em01_last_stand_julius")
end

function em01_last_stand_heli_destroyed(vehicle)
	-- Remove the marker
	marker_remove_vehicle(vehicle, SYNC_ALL)

	-- Increment the number of helicopters killed
	Em01_last_stand_heli_killed = Em01_last_stand_heli_killed + 1

	-- Update the objective
	objective_text(0, "em01_ls_objective", Em01_last_stand_heli_killed, 2, SYNC_ALL)

	if (Em01_last_stand_heli_killed == 2) then
		-- Mission can end with success
		em01_complete()
	end
end

function em01_fire_missiles_at_vehicle()
	-- Disable the trigger
	trigger_enable(TRIGGER_BLAST_VEHICLE, false)
	-- Disable the cancel trigger too
	trigger_enable(TRIGGER_BLAST_CANCEL, false)

	-- Play Julius' audio line
	audio_play_for_character("JULIUS_BONUS_CRASHCAR_01", CHARACTER_JULIUS, "voice")

	-- Get the driver of this heli
	local driver = vehicle_get_driver(LAST_STAND_HELI_A)

   -- If the driver can't be found, the helicopter's not going to be
   -- doing much attacking
   if ( driver == nil ) then
      return
   end
	
	-- We want this guy to have perfect aim
	if (not character_is_dead(driver)) then
		set_perfect_aim(driver, true)
	end

	-- Shoot at a designated spot
	helicopter_shoot_navpoint(LAST_STAND_HELI_A, "em01_$n132")

	delay(0.1)
	
	-- Shoot at a designated spot
	helicopter_shoot_navpoint(LAST_STAND_HELI_A, "em01_$n133")

	delay(0.1)
	
	-- Shoot at a designated spot
	helicopter_shoot_navpoint(LAST_STAND_HELI_A, "em01_$n134")

	-- We want this guy to not have perfect aim
	if (not character_is_dead(driver)) then
		set_perfect_aim(driver, false)
	end
end

function em01_fire_rpg_at_vehicle()
	-- Disable the trigger
	trigger_enable(TRIGGER_BLAST_VEHICLE, false)
	-- Disable the cancel trigger too
	trigger_enable(TRIGGER_BLAST_CANCEL, false)

	-- Play Julius' audio line
	audio_play_for_character("JULIUS_BONUS_CRASHCAR_01", CHARACTER_JULIUS, "voice")

	-- We want this guys to have perfect aim
	if (not character_is_dead("em01_$c054")) then
		set_perfect_aim("em01_$c054", true)
	end

	if (not character_is_dead("em01_$c055")) then
		set_perfect_aim("em01_$c055", true)
	end

	if (not character_is_dead("em01_$c056")) then
		set_perfect_aim("em01_$c056", true)
	end

	-- Shoot at a designated spot
	force_fire("em01_$c054", "em01_$n132", true)

	delay(0.1)
	
	-- Shoot at a designated spot
	force_fire("em01_$c055", "em01_$n133", true)

	delay(0.1)
	
	-- Shoot at a designated spot
	force_fire("em01_$c056", "em01_$n134", true)

	-- We want this guys to not have perfect aim
	if (not character_is_dead("em01_$c054")) then
		set_perfect_aim("em01_$c054", false)
	end

	if (not character_is_dead("em01_$c055")) then
		set_perfect_aim("em01_$c055", false)
	end

	if (not character_is_dead("em01_$c056")) then
		set_perfect_aim("em01_$c056", false)
	end

	-- Delay and then remove the rpg from these guys inventory
	delay(0.1)
	inv_item_remove("rpg_launcher", 1, "em01_$c054")
	inv_item_remove("rpg_launcher", 1, "em01_$c055")
	inv_item_remove("rpg_launcher", 1, "em01_$c056")
end

function em01_fire_missile_at_vehicle_cancel()
	-- Disable the trigger
	trigger_enable(TRIGGER_BLAST_CANCEL, false)
	-- Setup the shoot navpoint trigger
	trigger_enable(TRIGGER_BLAST_VEHICLE, true)
	on_trigger("em01_fire_rpg_at_vehicle", TRIGGER_BLAST_VEHICLE)

	-- Give a couple of guys rpgs since the heli won't be firing missiles
	inv_item_add("rpg_launcher", 1, "em01_$c054", true)
	inv_item_add("rpg_launcher", 1, "em01_$c055", true)
	inv_item_add("rpg_launcher", 1, "em01_$c056", true)
end

function em01_last_stand_a()

	trigger_enable(TRIGGER_BLAST_VEHICLE, true)
	on_trigger("em01_fire_missiles_at_vehicle", TRIGGER_BLAST_VEHICLE)

	trigger_enable(TRIGGER_BLAST_CANCEL, true)
	on_trigger("em01_fire_missile_at_vehicle_cancel", TRIGGER_BLAST_CANCEL)

	-- Pathfind to the location
	helicopter_fly_to_direct(LAST_STAND_HELI_A, 45.0, LAST_STAND_PATH_A)

	-- Current direction and pattern being executed
	local		direction = 1		-- 1 == CCW
	local		pattern_idx = 1
	local		dir_executed = 0
	local		new_pattern = ""

	-- We want this helicopter to just machine gun fire
	helicopter_set_missile_chance(LAST_STAND_HELI_A, 0.01)

	-- Fly the pattern...
	while (1) do
		-- Delay for a couple of seconds
		delay(1.0)

		-- Exit if the vehicle is destroyed
		if (vehicle_is_destroyed(LAST_STAND_HELI_A)) then
			return
		end

		-- Randomly choose the new pattern that can be used
		new_pattern = Em01_ls_table_a[direction][pattern_idx][rand_int(1, 2)]

		-- Have the helicopter fly to the next point in the list
		helicopter_fly_to_direct(LAST_STAND_HELI_A, 40.0, new_pattern)
		
		-- Fire a sequence of missiles
		if (not IN_COOP) then
			em01_missile_attack_sequence(LAST_STAND_HELI_A, {LOCAL_PLAYER, CHARACTER_JULIUS}, Em01_missile_sequence[rand_int(1, Num_em01_missile_sequence)])
		else
			em01_missile_attack_sequence(LAST_STAND_HELI_A, {LOCAL_PLAYER, CHARACTER_JULIUS, REMOTE_PLAYER}, Em01_missile_sequence[rand_int(1, Num_em01_missile_sequence)])
		end

		-- Set the pattern idx to use for the next choice
		pattern_idx = Em01_ls_table_a_idx[direction][pattern_idx][new_pattern]

		-- Increment the number of times a pattern has been executed in the current direction (cw, or ccw)
		dir_executed = dir_executed + 1
		if (dir_executed == 2) then
			-- Reset the number of times this direction has been executed
			dir_executed = 0
			-- Switch the pattern direction
			if (direction == 1) then
				direction = 2
			else
				direction = 1
			end
		end
	end
end

function em01_last_stand_b(human)
	-- Only pathfind to the desired area if we are not restarting from a checkpoint
	--if (not (human == nil)) then
		-- Pathfind to the location
		helicopter_fly_to_direct(LAST_STAND_HELI_B, 40.0, LAST_STAND_PATH_B)
	--end

	-- Variables to store the current direction and pattern being executed
	local		direction = 2		-- Start this heli going cw
	local		pattern_idx = 1
	local		dir_executed = 0
	local		new_pattern = ""

	-- We want this helicopter to just machine gun fire
	helicopter_set_missile_chance(LAST_STAND_HELI_B, 0.01)

	-- Fly the pattern...
	while (1) do
		-- Delay for a couple of seconds
		delay(1.0)

		-- Exit if the vehicle is destroyed
		if (vehicle_is_destroyed(LAST_STAND_HELI_B)) then
			return
		end

		-- Randomly choose the new pattern that can be used
		new_pattern = Em01_ls_table_b[direction][pattern_idx][rand_int(1, 2)]

		-- Have the helicopter fly to the next point in the list
		helicopter_fly_to_direct(LAST_STAND_HELI_B, 40.0, new_pattern)
		
		-- Fire a sequence of missiles
		if (not IN_COOP) then
			em01_missile_attack_sequence(LAST_STAND_HELI_B, {LOCAL_PLAYER, CHARACTER_JULIUS}, Em01_missile_sequence[rand_int(1, Num_em01_missile_sequence)])
		else
			em01_missile_attack_sequence(LAST_STAND_HELI_B, {LOCAL_PLAYER, CHARACTER_JULIUS, REMOTE_PLAYER}, Em01_missile_sequence[rand_int(1, Num_em01_missile_sequence)])
		end

		-- Set the pattern idx to use for the next choice
		pattern_idx = Em01_ls_table_b_idx[direction][pattern_idx][new_pattern]

		-- Increment the number of times a pattern has been executed in the current direction (cw, or ccw)
		dir_executed = dir_executed + 1
		if (dir_executed == 2) then
			-- Reset the number of times this direction has been executed
			dir_executed = 0
			-- Switch the pattern direction
			if (direction == 1) then
				direction = 2
			else
				direction = 1
			end
		end
	end
end

function em01_last_stand_julius()
	-- Store our current location
	local		current_location = 0

	-- Do this until the mission fails or succeeds
	while (1) do
		-- Find a new location
		local		location = rand_int(1, Num_em01_ls_table_julius)

		while (location == current_location) do
			location = rand_int(1, Num_em01_ls_table_julius)
		end

		current_location = location

		-- Move to a navpoint...attack...repeat
		move_to_safe(CHARACTER_JULIUS, Em01_ls_table_julius[current_location], 3, true, true)

		-- Leash Julius to this navpoint
		npc_leash_to_nav(CHARACTER_JULIUS, Em01_ls_table_julius[current_location], 2.0)

		-- Have Julius attack enemies
		set_attack_enemies_flag(CHARACTER_JULIUS, true)

		-- Delay a random amount for this location
		delay(rand_float(2.0, 6.0))

		-- Remove the leash
		npc_leash_remove(CHARACTER_JULIUS)
	end
end
