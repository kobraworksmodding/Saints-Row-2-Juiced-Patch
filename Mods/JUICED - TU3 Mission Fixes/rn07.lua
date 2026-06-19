-- rn07.lua
-- SR2 mission script
-- 3/28/07

	LTNT_DEBUGGED = false

	-- Coop mission?
	IN_COOP	= false

	-- MISC CONSTANTS
	
		-- Stage 1 Hospital Escape
		GURNEY_SPRINT_USE_OVERRIDE_MS			= 15000	-- How long the player can sprint while holding Gat's gurney.
		GURNEY_SPRINT_DELAY_OVERRIDE_MS		= 5000	-- Delay after a sprint before meter refill starts while holding gurney.
		GURNEY_SPRINT_RECHARGE_OVERRIDE_MS	= 5000	-- Time for sprint meter to refill while holding gurney.
		GAT_HIT_POINTS								= 7000	-- Gat's total hit points
		GAT_HIT_POINTS_HELI								= 10000  --Gat's total HP while in the helicopter
		GAT_PERCENT_HEALTH						= 1.0		-- Ratio of Gat's starting health to full health.
		GAT_MIN_RESTART_PERCENT_HEALTH		= 0.5		-- Min health ratio of Gat upon restart.
		GAT_PADDLE_HEALTH_RESTORE				= .40		-- Fraction of Gat's health returned when using the shock paddles.
		LIEUTENANT_RESPAWN_DISTANCE			= 1		-- Min distance from player to allow respawn
		LIEUTENANT_RESPAWN_TIME_MS				= 250		-- Time in MS before respawning
		LIEUTENANT_SPRINT_CHANCE				= 50		-- % of the time that the Lieutenants will sprint to the player
		GAT_REVIVE_TIME_S							= 45		-- Amount of time player has to revive Gat after he goes into cardiac arrest
		GAT_REVIVE_HUD_INDEX						= 0
		GAT_DISTANCE_ALLOWED						= 25		-- Maximum distance the player can be from Gat 
		GAT_DISTANCE_TIME_S						= 20		-- If player out of range this long, mission fails
		GAT_DISTANCE_HUD_INDEX					= 1 
		GAT_HEALTH_LOSS_TIME_S					= 210		-- Time (in s.) that it will take for Gat to die assuming that he
																	-- he takes no additional damage from Ronin soldiers.
		GAT_HELI_DAMAGE_RATIO					= .05		-- When Gat is in the helicopter, this fraction of damage dealt to
																	-- the copter is applied to Gat as well.
		PLAYER_HELI_HEALTH_MULTIPLIER			= 3			--Multiplier used to increase hp of player's helicopter for the last stage of the mission
		
	-- GROUPS --
 
		GROUP_AMBULANCE				= "rn07_$Gambulance"
		GROUP_GAT						= "rn07_$Ggat"
		GROUP_GAT_NORMAL				= "rn07_$Ggat_normal"
		GROUP_HELICOPTER				= "rn07_$Ghelicopter"
		GROUP_OUTSIDE_HOSPITAL		= "rn07_$Goutside_hospital" -- Ronin outside the front door
		GROUP_RONIN_HELICOPTER		= "rn07_$Gronin_helicopter"
		GROUP_HELI_ATTACK_RONIN		= "rn07_$Gheli_attack_ronin_1" -- Ronin that attack the player when they enter the helicopter
		GROUP_RONIN_STATIONARY		= "rn07_$Gronin_stationary"
		GROUP_RONIN_STATIONARY_COOP= "rn07_$Gronin_stationary_coop"

		-- Patrol groups
		FLOOR_PATROL_GROUPS		= {	["floor_1"] = "rn07_$Gronin_floor_1",	["floor_2"] = "rn07_$Gronin_floor_2", 
												["floor_3"] = "rn07_$Gronin_floor_3",	["floor_4"] = "rn07_$Gronin_floor_4"}

		-- List of all groups, makes cleaning up more convenient
		TABLE_ALL_GROUPS			= {	GROUP_AMBULANCE, GROUP_GAT, GROUP_OUTSIDE_HOSPITAL, 
												GROUP_RONIN_HELICOPTER, GROUP_HELI_ATTACK_RONIN, 
												GROUP_RONIN_STATIONARY}

		TABLE_ALL_COOP_GROUPS	= {	GROUP_RONIN_STATIONARY_COOP}
	
	-- GROUP MEMBER TABLES -- 
		PATROL_GROUP_MEMBERS		= 
			{
				["floor_1"] =	{	"rn07_$c007", "rn07_$c008", "rn07_$c009", "rn07_$c010", "rn07_$c011", "rn07_$c012"};
				["floor_2"] =	{	"rn07_$c001", "rn07_$c002", "rn07_$c003", "rn07_$c004", "rn07_$c005", "rn07_$c006"};
				["floor_3"] =	{	"rn07_$c013", "rn07_$c014", "rn07_$c015", "rn07_$c016", "rn07_$c017", "rn07_$c018"};
				["floor_4"] =	{	"rn07_$c019", "rn07_$c020", "rn07_$c021", "rn07_$c022", "rn07_$c023", "rn07_$c024"};
			}
		
		STATIONARY_GROUP_MEMBERS 		={"rn07_$c032", "rn07_$c033", "rn07_$c034", "rn07_$c035", "rn07_$c036", "rn07_$c037", "rn07_$c038", "rn07_$c039",
										  "rn07_$c040", "rn07_$c041", "rn07_$c042", "rn07_$c043", "rn07_$c044", "rn07_$c045"}
						
		STATIONARY_GROUP_MEMBERS_COOP 	= {"rn07_$c046", "rn07_$c047", "rn07_$c048", "rn07_$c049", "rn07_$c050", "rn07_$c051", 
										   "rn07_$c052", "rn07_$c053", "rn07_$c054", "rn07_$c055", "rn07_$c056", "rn07_$c057", 
										   "rn07_$c058", "rn07_$c059", "rn07_$c060", "rn07_$c061"}
		
--		RONIN_HELICOPTER_MEMBERS = {"rn07_$c029",	"rn07_$c030", "rn07_$c031"}
		RONIN_HELICOPTER_MEMBERS = {"rn07_$c029",	"rn07_$c030"}

	-- TRIGGERS -- 
		
		TRIGGER_BED						= "rn07_$Tbed"
		TRIGGER_ELEVATOR_1			= "rn07_$Televator_1"
		TRIGGER_ELEVATOR_2			= "rn07_$Televator_2"
		TRIGGER_ELEVATOR_3			= "rn07_$Televator_3"
		TRIGGER_ELEVATOR_4			= "rn07_$Televator_4"
		TRIGGER_FRONT_DOOR			= "rn07_$Tfront_door"
		TRIGGER_AMBULANCE				= "rn07_$Tambulance"
		TRIGGER_HELICOPTER			= "rn07_$Thelicopter"
		TRIGGER_SAINTS_HQ				= "rn07_$Tsaints_hq"
		TRIGGER_SAINTS_HQ_LOCAL		= "rn07_$Tsaints_hq_local"
		TRIGGER_SAINTS_HQ_REMOTE	= "rn07_$Tsaints_hq_remote"
		TRIGGER_PARKING_FOOT			= "rn07_$Tparking_foot"
		TRIGGER_PARKING_HELICOPTER = "rn07_$Tparking_helicopter"
		TRIGGER_STAIRWELL				= "rn07_$Tstairwell" -- 2nd floor stairwell

		TRIGGER_BREADCRUMB_1			= "rn07_$t000"
		TRIGGER_BREADCRUMB_2			= "rn07_$t001"
		TRIGGER_BREADCRUMB_3			= "rn07_$t002"
		TRIGGER_BREADCRUMB_4			= "rn07_$t003"
		TRIGGER_BREADCRUMB_5			= "rn07_$t004"
		TRIGGER_BREADCRUMB_6			= "rn07_$t005"
		TRIGGER_BREADCRUMB_7			= "rn07_$t006"
		TRIGGER_BREADCRUMB_8			= "rn07_$t007"

		-- List of all triggers, makes cleaning up more convenient
		TABLE_ALL_TRIGGERS		= {	TRIGGER_BED, TRIGGER_ELEVATOR_1, TRIGGER_ELEVATOR_2, TRIGGER_ELEVATOR_3,
												TRIGGER_ELEVATOR_4, TRIGGER_FRONT_DOOR, TRIGGER_AMBULANCE, TRIGGER_HELICOPTER,
												TRIGGER_SAINTS_HQ, TRIGGER_PARKING_FOOT, TRIGGER_PARKING_HELICOPTER,
												TRIGGER_STAIRWELL, TRIGGER_BREADCRUMB_1, TRIGGER_BREADCRUMB_2, TRIGGER_BREADCRUMB_3,
												TRIGGER_BREADCRUMB_4, TRIGGER_BREADCRUMB_5, TRIGGER_BREADCRUMB_6,
												TRIGGER_BREADCRUMB_7, TRIGGER_BREADCRUMB_8}		

	-- VEHICLES --
		VEHICLE_AMBULANCE			= "rn07_$Vambulance"
		VEHICLE_HELICOPTER		= "rn07_$Vhelicopter"
		VEHICLE_RONIN_HELI		= "rn07_$Vronin_heli"

	-- CHARACTERS --
		CHARACTER_GAT				= "rn07_$Cgat"
		CHARACTER_GAT_NORMAL		= "rn07_$Cgat_normal"
		CHARACTER_RPG_GUY			= "rn07_$c000"

	-- ITEMS --
		SHOCK_PADDLES				= {	"rn07_$Ishock_paddles_floor_1", "rn07_$Ishock_paddles_floor_2",
												"rn07_$Ishock_paddles_floor_3", "rn07_$Ishock_paddles_floor_4"}

	-- NAVPOINTS

		-- the mission start locations
		NAVPOINT_LOCAL_PLAYER_START					= "rn07_$Nlocal_player_start"
		NAVPOINT_REMOTE_PLAYER_START					= "rn07_$Nremote_player_start"

		NAVPOINT_ROOFTOP									= "rn07_$Nrooftop" -- Rooftop near helipad
		NAVPOINT_ROOFTOP_GAT								= "rn07_$Nrooftop_gat" -- Next to NAVPOINT_ROOFTOP

		NAVPOINT_CHECKPOINT_GAT							= "rn07_$n089"

		NAVPOINT_LOCAL_MISSION_END						= "rn07_$Nlocal_mission_end"
		NAVPOINT_REMOTE_MISSION_END					= "rn07_$Nremote_mission_end"
		NAVPOINT_HOSPITAL_CENTER						= "rn07_$Nhospital_center"

	-- PATROL NAVPOINTS

		PATROL_NAVPOINTS_FLOOR_1	= {	"rn07_$n043",	"rn07_$n044",	"rn07_$n045", "rn07_$n046", 
												"rn07_$n047",	"rn07_$n048"}

		PATROL_NAVPOINTS_FLOOR_2	= {	"rn07_$n018",	"rn07_$n019",	"rn07_$n020", "rn07_$n021", 
												"rn07_$n022",	"rn07_$n023",	"rn07_$n024" }

		PATROL_NAVPOINTS_FLOOR_3	= {	"rn07_$n067",	"rn07_$n068",	"rn07_$n069", "rn07_$n070", 
												"rn07_$n071",	"rn07_$n072",	"rn07_$n073" }

		PATROL_NAVPOINTS_FLOOR_4	= {	"rn07_$n086",	"rn07_$n087",	"rn07_$n088"}

		PATROL_NAVPOINT_TABLES	= {	["floor_1"]	=	PATROL_NAVPOINTS_FLOOR_1,
												["floor_2"]	=	PATROL_NAVPOINTS_FLOOR_2,
												["floor_3"] =	PATROL_NAVPOINTS_FLOOR_3,
												["floor_4"] =	PATROL_NAVPOINTS_FLOOR_4}

		-- The navpoints where we can spawn Ronin lieutenants in the key area
		SPAWN_NAVPOINT_TABLES	=	
			{
				["floor_2_area_1"] = {	"rn07_$n000", "rn07_$n001", "rn07_$n002", "rn07_$n003", "rn07_$n004", "rn07_$n005"};

				["floor_2_area_2"] = {	"rn07_$n006", "rn07_$n007", "rn07_$n008", "rn07_$n009", "rn07_$n010", "rn07_$n011"};
	
				["floor_2_area_5"] = {	"rn07_$n012", "rn07_$n013", "rn07_$n014", "rn07_$n015", "rn07_$n016", "rn07_$n017"};

				["floor_1_area_1"] = {	"rn07_$n025", "rn07_$n026", "rn07_$n027", "rn07_$n028", "rn07_$n029", "rn07_$n030"};

				["floor_1_area_2"] = {	"rn07_$n037", "rn07_$n038", "rn07_$n039", "rn07_$n040", "rn07_$n041", "rn07_$n042"};
	
				["floor_1_area_4"] = {	"rn07_$n031", "rn07_$n032", "rn07_$n033", "rn07_$n034", "rn07_$n035", "rn07_$n036"};

				["floor_3_area_1"] = {	"rn07_$n049", "rn07_$n050", "rn07_$n051", "rn07_$n052", "rn07_$n053", "rn07_$n054"};

				["floor_3_area_2"] = {	"rn07_$n055", "rn07_$n056", "rn07_$n057", "rn07_$n058", "rn07_$n059", "rn07_$n060"};
	
				["floor_3_area_5"] = {	"rn07_$n061", "rn07_$n062", "rn07_$n063", "rn07_$n064", "rn07_$n065", "rn07_$n066"};

				["floor_4_area_1"] = {	"rn07_$n080", "rn07_$n081", "rn07_$n082", "rn07_$n083", "rn07_$n084", "rn07_$n085"};

				["floor_4_area_3"] = {	"rn07_$n074", "rn07_$n075", "rn07_$n076", "rn07_$n077", "rn07_$n078", "rn07_$n079"};
				
			}

	-- MOVERS
		
			-- Two doors exiting onto rooftop near the heli pad
			DOOR_FLOOR_4_EXIT		= "rn07_$Dfloor4_exit"
			DOOR_FLOOR_4_EXIT_2	= "rn07_$Dfloor4_exit2"		

			-- Main exit on first floor
			DOOR_MAIN_1					= "rn07_$Dmain1"
			DOOR_MAIN_2					= "rn07_$Dmain2"

	-- HELPTEXT

		-- localized helptext messages

			HELPTEXT_FAILURE_GAT_DIED					= "rn07_failure_gat_died"
			HELPTEXT_FAILURE_GAT_ABANDONED			= "rn07_failure_gat_abandoned"
			HELPTEXT_FAILURE_HELICOPTER_DESTROYED	= "rn07_failure_helicopter_destroyed"

			HELPTEXT_HUD_GAT_HEALTH						= "rn07_hud_gat_health"		-- "Johnny Gat's Health"

			HELPTEXT_PROMPT_BED					= "rn07_prompt_bed"					-- "Go to the foot of Gat's bed." 
			-- "Press Y to wheel Gat's bed around. L Stick steers." (Helptext or hospital system?)
			HELPTEXT_PROMPT_BED_SPRINT			= "rn07_prompt_sprint_gurney"		-- "Hold {SPRINT_IMG} to sprint with the gurney"
			HELPTEXT_PROMPT_EXIT_HOSPITAL		= "rn07_prompt_exit_hospital"		-- "Get Gat out of the hospital!"
			HELPTEXT_PROMPT_FRONT_DOOR			= "rn07_prompt_front_door"			-- "Head to the front door."
			HELPTEXT_PROMPT_AMBULANCE			= "rn07_prompt_ambulance"			-- "Head to the ambulance!"
			HELPTEXT_PROMPT_RETURN_ELEVATOR	= "rn07_prompt_return_elevator"	-- "Head back to the elevator!" 
			HELPTEXT_PROMPT_ENTER_HELICOPTER = "rn07_prompt_enter_helicopter" -- "Get in the helicopter!" 
			HELPTEXT_PROMPT_BASE					= "rn07_prompt_base"					-- "Head back to base!" 
			HELPTEXT_PROMPT_RETURN_GAT			= "rn07_prompt_return_gat"			-- "Get back to Gat before the Yakuza get to him!"
			HELPTEXT_PROMPT_SHOCK_PADDLES		= "rn07_prompt_shock_paddles"		-- "Find some shock paddles and revive Gat!" 
			HELPTEXT_PROMPT_PARKING				= "rn07_prompt_parking"				-- "Wait for your partner in the parking lot"
			HELPTEXT_PROMPT_ROOFTOP_ELEVATOR = "rn07_prompt_rooftop_elevator" -- "Take the elevator to the rooftop"
			HELPTEXT_PROMPT_CARDIAC_ARREST	= "rn07_prompt_cardiac_arrest"	-- "Gat's gone into cardiac arrest!"
			HELPTEXT_PROMPT_USE_PADDLES		= "rn07_prompt_use_paddles"		-- "Use the shock paddles to revive Gat."
			HELPTEXT_PROMPT_HAVE_PULSE			= "rn07_prompt_have_pulse"			-- "We have a pulse!"
			HELPTEXT_PROMPT_RELEASE_BED		= "rn07_prompt_release_bed"		-- ## Press <btn action> to release the bed.
			HELPTEXT_PROMPT_ELEVATOR			= "rn07_prompt_elevator"			-- Go to the elevator.
			HELPTEXT_PROMPT_HELICOPTER_MED	= "rn07_prompt_helicopter_med"	-- You found some medicine for Gat!
			HELPTEXT_PROMPT_NO_GAT_ELEVATOR	= "rn07_prompt_no_gat_elevator"	-- "Don't leave the floor without Gat"

			HELPTEXT_PROMPT_GAT_HEALTH_1		= "rn07_prompt_gat_health_1"
			HELPTEXT_PROMPT_GAT_HEALTH_2		= "rn07_prompt_gat_health_2"
			HELPTEXT_PROMPT_GAT_HEALTH_3		= "rn07_prompt_gat_health_3"

			HELPTEXT_PROMPT_REENTER_HELI		= "rn07_prompt_reenter_heli"		-- Get back in the helicopter.

			-- coop only prompts
			HELPTEXT_PROMPT_STAIRWELL			= "rn07_prompt_stairwell"			-- "Take the stairs to the rooftop"
			HELPTEXT_PROMPT_PICKUP				= "rn07_prompt_pickup"				-- "Pickup your partner and Gat in the parking lot"
			HELPTEXT_PROMPT_HOLD_GROUND		= "rn07_prompt_hold_ground"		-- "Hold your ground until your homie picks you up!"
			HELPTEXT_PROMPT_DEFEND_HELICOPTER= "rn07_prompt_defend_helicopter"-- "Defend the helicopter until your friends arrive!"

	-- CHECKPOINTS
		
		CHECKPOINT_START						= MISSION_START_CHECKPOINT -- defined in ug_lib.lua
		CHECKPOINT_HOSPITAL					= "rn07_checkpoint_hospital" -- 

	-- HOSPITAL AREAS

	--[[ - Floors listed here for convenience, not needed by code
		FLOOR_1_AREA_1							= "floor_1_area_1"	-- Elevator from 2nd floor, operating room.
		FLOOR_1_AREA_2							= "floor_1_area_2"	-- Elevator to roof.
		FLOOR_1_AREA_3							= "floor_1_area_3"	-- Reception area/ waiting room  
		FLOOR_1_AREA_4							= "floor_1_area_4"	-- Outdoors

		FLOOR_2_AREA_1							= "floor_2_area_1"	-- Gat's hospital room
		FLOOR_2_AREA_2							= "floor_2_area_2"	-- Nurses station, hallway loop 
		FLOOR_2_AREA_3							= "floor_2_area_3"	-- Two adjacent operating rooms
		FLOOR_2_AREA_4							= "floor_2_area_4"	-- Large operating room
		FLOOR_2_AREA_5							= "floor_2_area_5"	-- Elevator to 1st floor
	]]

		-- Index of first navpoint that the Ronin whould pathfind to in the floor's patrol path
		AREA_PATROL_START	= 
			{	["floor_2_area_1"] = 1,	["floor_2_area_2"] = 3, ["floor_2_area_5"] = 7,
				["floor_1_area_1"] = 1, ["floor_1_area_2"] = 6, ["floor_1_area_4"] = 3,
				["floor_3_area_1"] = 1,	["floor_3_area_2"] = 3, ["floor_3_area_5"] = 7,
				["floor_4_area_1"] = 1,	["floor_4_area_3"] = 3
			}

		-- Direction along the patrol route that the Ronin need to follow to get to the target area
		AREA_FLOOR_PATROL_DIRECTION = 
			{
				["floor_2_area_1"] = {	["floor_2_area_2"] = "forward",	["floor_2_area_3"] = "forward",
												["floor_2_area_4"] = "forward",	["floor_2_area_5"] = "forward"};

				["floor_2_area_2"] = {	["floor_2_area_1"] = "reverse",	["floor_2_area_3"] = "forward",
												["floor_2_area_4"] = "forward",	["floor_2_area_5"] = "forward"};

				["floor_2_area_3"] = {	["floor_2_area_1"] = "reverse",	["floor_2_area_2"] = "reverse",
												["floor_2_area_4"] = "forward",	["floor_2_area_5"] = "forward"};

				["floor_2_area_4"] = {	["floor_2_area_1"] = "reverse",	["floor_2_area_2"] = "reverse",
												["floor_2_area_3"] = "reverse",	["floor_2_area_5"] = "forward"};

				["floor_2_area_5"] = {	["floor_2_area_1"] = "reverse",	["floor_2_area_2"] = "reverse",
												["floor_2_area_3"] = "reverse",	["floor_2_area_4"] = "reverse"};

				["floor_1_area_1"] = {	["floor_1_area_2"] = "forward",	["floor_1_area_3"] = "forward",
												["floor_1_area_4"] = "forward"};

				["floor_1_area_2"] = {	["floor_1_area_1"] = "reverse",	["floor_1_area_3"] = "reverse",
												["floor_1_area_4"] = "reverse"};

				["floor_1_area_3"] = {	["floor_1_area_1"] = "reverse",	["floor_1_area_2"] = "forward",
												["floor_1_area_4"] = "forward"};

				["floor_1_area_4"] = {	["floor_1_area_1"] = "reverse",	["floor_1_area_2"] = "forward",
												["floor_1_area_3"] = "forward"};

				["floor_3_area_1"] = {	["floor_3_area_2"] = "forward",	["floor_3_area_3"] = "forward",
												["floor_3_area_4"] = "forward",	["floor_3_area_5"] = "forward"};

				["floor_3_area_2"] = {	["floor_3_area_1"] = "reverse",	["floor_3_area_3"] = "forward",
												["floor_3_area_4"] = "forward",	["floor_3_area_5"] = "forward"};

				["floor_3_area_3"] = {	["floor_3_area_1"] = "reverse",	["floor_3_area_2"] = "reverse",
												["floor_3_area_4"] = "forward",	["floor_3_area_5"] = "forward"};

				["floor_3_area_4"] = {	["floor_3_area_1"] = "reverse",	["floor_3_area_2"] = "reverse",
												["floor_3_area_3"] = "reverse",	["floor_3_area_5"] = "forward"};

				["floor_3_area_5"] = {	["floor_3_area_1"] = "reverse",	["floor_3_area_2"] = "reverse",
												["floor_3_area_3"] = "reverse",	["floor_3_area_4"] = "reverse"};

				["floor_4_area_1"] = {	["floor_4_area_2"] = "forward",	["floor_4_area_3"] = "forward"};

				["floor_4_area_2"] = {	["floor_4_area_1"] = "reverse",	["floor_4_area_3"] = "forward"};

				["floor_4_area_3"] = {	["floor_4_area_1"] = "reverse",	["floor_4_area_2"] = "reverse"};
			}

		-- Areas where Ronin can safely spawn without popping
		FLOOR_1_SAFE_SPAWN_AREAS =
			{
				["floor_1_area_1"] = {	"floor_1_area_2",	"floor_1_area_4"};

				["floor_1_area_2"] = {	"floor_1_area_1",	"floor_1_area_4"};

				["floor_1_area_3"] = {	"floor_1_area_2",	"floor_1_area_4"};

				["floor_1_area_4"] = {	"floor_1_area_1",	"floor_1_area_2"};
			}
		FLOOR_2_SAFE_SPAWN_AREAS =
			{
				["floor_2_area_1"] = {	"floor_2_area_2",	"floor_2_area_5"};

				["floor_2_area_2"] = {	"floor_2_area_1",	"floor_2_area_5"};

				["floor_2_area_3"] = {	"floor_2_area_1",	"floor_2_area_2",	"floor_2_area_5"};

				["floor_2_area_4"] = {	"floor_2_area_1",	"floor_2_area_2"};

				["floor_2_area_5"] = {	"floor_2_area_1",	"floor_2_area_2"};
			}
		FLOOR_3_SAFE_SPAWN_AREAS =
			{
				["floor_3_area_1"] = {	"floor_3_area_2",	"floor_3_area_5"};

				["floor_3_area_2"] = {	"floor_3_area_1",	"floor_3_area_5"};

				["floor_3_area_3"] = {	"floor_3_area_1",	"floor_3_area_2",	"floor_3_area_5"};

				["floor_3_area_4"] = {	"floor_3_area_1",	"floor_3_area_2"};

				["floor_3_area_5"] = {	"floor_3_area_1",	"floor_3_area_2"};
			}
		FLOOR_4_SAFE_SPAWN_AREAS =
		{
				["floor_4_area_1"] = {	"floor_4_area_3"};

				["floor_4_area_2"] = {	"floor_4_area_3"};

				["floor_4_area_3"] = {	"floor_4_area_1"};
		}
		
		-- Maps from floor -> safe spawn area table
		FLOOR_SAFE_SPAWN_TABLE	= {["floor_1"] = FLOOR_1_SAFE_SPAWN_AREAS, ["floor_2"]=FLOOR_2_SAFE_SPAWN_AREAS,
											["floor_3"] = FLOOR_3_SAFE_SPAWN_AREAS, ["floor_4"]=FLOOR_4_SAFE_SPAWN_AREAS}

		-- Area in which each patrol navpoint is located
		FLOOR_1_PATROL_NAVPOINT_AREAS =	{	"floor_1_area_1",	"floor_1_area_3",	"floor_1_area_3",
														"floor_1_area_3", "floor_1_area_2", "floor_1_area_2"}

		FLOOR_2_PATROL_NAVPOINT_AREAS =	{	"floor_2_area_1",	"floor_2_area_2",	"floor_2_area_2",
														"floor_2_area_3", "floor_2_area_3", "floor_2_area_4",
														"floor_2_area_5"}

		FLOOR_3_PATROL_NAVPOINT_AREAS =	{	"floor_3_area_1",	"floor_3_area_2",	"floor_3_area_2",
														"floor_3_area_3", "floor_3_area_3", "floor_3_area_4",
														"floor_3_area_5"}

		FLOOR_4_PATROL_NAVPOINT_AREAS =	{	"floor_4_area_1",	"floor_4_area_2",	"floor_4_area_3"}

		FLOOR_PATROL_NAVPOINT_TABLES = {	["floor_1"] = FLOOR_1_PATROL_NAVPOINT_AREAS,
													["floor_2"] = FLOOR_2_PATROL_NAVPOINT_AREAS,
													["floor_3"] = FLOOR_3_PATROL_NAVPOINT_AREAS,
													["floor_4"] = FLOOR_4_PATROL_NAVPOINT_AREAS}

		-- Number of areas on each floor
		AREAS_PER_FLOOR				= {["floor_1"]=4, ["floor_2"]=5, ["floor_3"]=5, ["floor_4"]=3 }

	-- Areas associated with each entrance (built in rn07_initialize_common)
		ENTRANCE_TARGET_AREAS	=	{}

	-- MINIMAP MODES, highest to lowest priority
	MINIMAP_MODE_PADDLES		= 0	-- Only displaying paddle icons
	MINIMAP_MODE_GAT			= 1	-- Only displaying Gat's icon
	MINIMAP_MODE_HELI			= 2	-- Only displaying the helicopter's icon
	MINIMAP_MODE_LOCATIONS	= 3	-- Only displaying location icons
	MINIMAP_MODE_NONE			= 4	-- Not displaying any icons on the map

	MINIMAP_MODE_FIRST		= MINIMAP_MODE_PADDLES
	MINIMAP_MODE_LAST			= MINIMAP_MODE_LOCATIONS

	PLAYER_SYNC	=	
	{
		[LOCAL_PLAYER]		= SYNC_LOCAL,
		[REMOTE_PLAYER]	= SYNC_REMOTE,
	}

	RN01_RONIN_ATTACK_PERSONAS	= {
		["AM_Ron2"]	=	"AMRON2",

		["AF_Ron1"]	=	"AFRON1",

		["WM_Ron1"]	=	"WMRON1",

		["WF_Ron1"]	=	"WFRON1",
		["WF_Ron2"]	=	"WFRON2",
	}

	PARKING_SPAWN_AMBULANCE = "parking_spots_$parking_896"

	-- Dialogue lines
	Rn07_bicker_conversations = 
	{
		{
			{ "PLAYER_RON7_BICKER1_L1", LOCAL_PLAYER, 0.0 },
			{ "RON7_BICKER1_L2", CHARACTER_GAT, 0.0 },
		};
		{
			{ "RON7_BICKER2_L1", CHARACTER_GAT, 0.0 },
			{ "PLAYER_RON7_BICKER2_L2", LOCAL_PLAYER, 0.0 },
		};
		{
			{ "PLAYER_RON7_BICKER3_L1", LOCAL_PLAYER, 0.0 },
			{ "RON7_BICKER3_L2", CHARACTER_GAT, 0.0 },
		};
		{
			{ "RON7_BICKER4_L1", CHARACTER_GAT, 0.0 },
			{ "PLAYER_RON7_BICKER4_L2", LOCAL_PLAYER, 0.0 },
		};
		{
			{ "PLAYER_RON7_BICKER5_L1", LOCAL_PLAYER, 0.0 },
			{ "RON7_BICKER5_L2", CHARACTER_GAT, 0.0 },
		};
	}

	Rn07_mission_conversations = 
	{
		["ambulance"] = 
			{
				{ "PLAYER_RON7_AMBULANCE_L1", LOCAL_PLAYER, 1 },
				{ "RON7_AMBULANCE_L2", CHARACTER_GAT, 1 },
			};
		["heli"] = 
			{
				{ "GAT_RON7_HELI_01", CHARACTER_GAT, 1 },
			};
	}

-- Global Variables (First letter caps == Global Variable)

	-- Threads
	Thread_remote_player				= -1	-- Main mission thread for remote player in Coop
	Thread_floor_patrols				= {	["floor_1"] = -1,	["floor_2"] = -1, ["floor_3"] = -1, ["floor_4"] = -1}
	Thread_syphon						= INVALID_THREAD_HANDLE

	-- Player locations
	Local_player_location			= "other dimension"
	Remote_player_location			= "other dimension"

	-- Progress flags
	Cutscene_played					= false
	Bed_foot_reached					= false
	Helicopter_entered				= {[LOCAL_PLAYER] = false, [REMOTE_PLAYER] = false}
	Local_reached_parking			= false -- Local player arrived at parking lot
	Remote_reached_parking			= false -- Remote player arrived at parking lot

	-- Misc
	Gat_in_cardiac_arrest			= false -- Is Gat in cardiac arrest?
	Gat_too_far							= false -- Is Gat too far from the player?
	Pushing_gat							= false -- Should the local player be pushing Gat around?

	Next_navpoint_index				= 1 -- Index into current spawn target where next lieutenant will spawn
	Min_wave_size						= 4
	Max_wave_size						= 5	

		-- Number of lieutenants alive on each floor
	Floor_living_lieutenants				= {	["floor_1"] = 0,	["floor_2"] = 0, ["floor_3"] = 0, ["floor_4"] = 0}
		-- First navpoint that the lieutenants should be pathfinding to
	Lieutenant_first_navpoint				= {}
	-- Path that the lieutenants should be using
	Lieutenant_path							= {}
		-- Floor that the lieutenant is on
	Lieutenant_floor							= {}
		-- Player the Lieutenant is pathfinding to
	Lieutenant_target							= {}

-- Called when mission starts
function rn07_start(rn07_checkpoint, is_restart)

	if (rn07_checkpoint == CHECKPOINT_START) then

		-- Determine which groups we'll need to load immediately
		local start_groups = {GROUP_RONIN_STATIONARY, GROUP_GAT, GROUP_AMBULANCE, GROUP_OUTSIDE_HOSPITAL}
		if (IN_COOP) then
			local start_groups = {GROUP_RONIN_STATIONARY, GROUP_GAT, GROUP_RONIN_STATIONARY_COOP, GROUP_OUTSIDE_HOSPITAL}
		end
		
		
		if (is_restart) then

			-- Load starting groups. Will position the players inside of rn07_initialize_checkpoint
			for i,group in pairs(start_groups) do
				group_create_hidden(group, true)
			end
			
		else

			-- Play the cutscene. Use it to create groups and position the players
			cutscene_play("ro07-01", start_groups, {NAVPOINT_LOCAL_PLAYER_START, NAVPOINT_REMOTE_PLAYER_START}, false)
		end

		for i,group in pairs(start_groups) do
			if (group ~= GROUP_OUTSIDE_HOSPITAL) then
				group_show(group)
			end
		end

	end

	rn07_initialize(rn07_checkpoint, is_restart)

	-- Start floor patrols
	rn07_start_hospital_patrols()

	if(rn07_checkpoint == CHECKPOINT_START) then

		thread_new("rn07_load_later_groups")

		-- Local player has to get to the first floor
		rn07_local_player_to_first_floor()
		
		if (not IN_COOP) then

			-- Player is sent to the ambulance which is destroyed by the Ronin
			rn07_local_player_the_ambulance()

			-- Back into the hospital checkpoint
			Rn07_gat_health = get_current_hit_points(CHARACTER_GAT)
			mission_set_checkpoint(CHECKPOINT_HOSPITAL)
			rn07_checkpoint = CHECKPOINT_HOSPITAL

		else -- Else, do the Coop ending

			-- Meet partner in the parking lot
			rn07_local_player_get_to_parking_lot()

		end

	end -- end of CHECKPOINT_START

	-- Single player only
	if (rn07_checkpoint == CHECKPOINT_HOSPITAL) then

		-- Player has to ge to the roof and take the Hospital's heli back to the Saint's base.
		rn07_single_player_ending()

	end -- end of CHECKPOINT_HOSPITAL

	rn07_complete()
end

function rn07_load_later_groups()

	group_create_hidden(GROUP_GAT_NORMAL, true)
	group_create(GROUP_HELICOPTER, true)
	group_create(GROUP_OUTSIDE_HOSPITAL, true)

	--Increase HP of player's helicopter using multiplier constant defined at top of script
	set_max_hit_points(VEHICLE_HELICOPTER, get_max_hit_points(VEHICLE_HELICOPTER)*PLAYER_HELI_HEALTH_MULTIPLIER)
	
end

function rn07_initialize(checkpoint, is_restart)

	mission_start_fade_out()

	rn07_initialize_common()
	rn07_initialize_checkpoint(checkpoint, is_restart)
	rn07_initialize_common_post()

	mission_start_fade_in()

end

-- Do initialization that is required for all checkpoints
function rn07_initialize_common()

	set_mission_author("Phillip Alexander")
	
	-- Check for coop being active
	if (coop_is_active()) then
		IN_COOP	= true
	end

	-- Swap the chunk state
	city_chunk_swap("SR2_IntDTMisHospital", "blackout", true, true, true)
	city_chunk_swap("SR2_IntDTMisHospital02", "blackout", false, true, true)
	city_chunk_swap("SR2_IntDTMisHospital03", "blackout", true, true, true)

	-- Disable the ambient ambulance parking spawn in front of the hospital
	if (not IN_COOP) then
		parking_spot_enable(PARKING_SPAWN_AMBULANCE, false)
	end

	door_open(DOOR_MAIN_1)
	door_open(DOOR_MAIN_2)

	-- Build mapping from hospital area entrances to the entrance triggers
	for floor,num_areas in pairs(AREAS_PER_FLOOR) do
		for area_number = 1, num_areas, 1 do
			local area_name = floor .. "_area_" .. area_number								-- e.g. area_name = "floor_1_area_1"
			local area_entrance_prefix = "rn07_$T" .. area_name							-- e.g. area_entrance_prefix = "rn07_$Tfloor_1_area_1"
			local area_entrances = { trigger_get_all_with_prefix(area_entrance_prefix) }
			if (area_entrances ~= nil) then
				for i,entrance in pairs(area_entrances) do
					ENTRANCE_TARGET_AREAS[entrance] = area_name
				end
			end
		end
	end

	-- Overload Ronin personas
	persona_override_group_start(RN01_RONIN_ATTACK_PERSONAS,POT_SITUATIONS[POT_ATTACK], "RO07_ATTACK")
	
	--Dismiss the player's party
	party_dismiss_all()
	party_set_recruitable(false)

	-- Don't let the players knock the heli off of the roof
	vehicle_prevent_explosion_fling(VEHICLE_HELICOPTER,true)

	-- Enable failure conditions
	set_unjackable_flag(VEHICLE_HELICOPTER, true)
	turn_invulnerable(VEHICLE_HELICOPTER, false)
	on_vehicle_destroyed("rn07_failure_helicopter_destroyed", VEHICLE_HELICOPTER)

	-- Override the npc respawn distance
	npc_respawn_dist_override(LIEUTENANT_RESPAWN_DISTANCE)

	-- Enable the area-entrance triggers for player tracking
	rn07_setup_player_tracking_triggers()

	-- Set Ronin notoriety to 3
	notoriety_set_min("Ronin",3)
	notoriety_set_max("Ronin",3)
	notoriety_set("Ronin",3)

	-- Start processing the minimap
	thread_new("rn07_process_minimap", LOCAL_PLAYER)
	if (IN_COOP) then
		thread_new("rn07_process_minimap", REMOTE_PLAYER)
	end

	thread_new("rn07_play_dialogue")

	thread_new("rn07_control_grenade_slot")

end

function rn07_initialize_common_post()

	-- Enable the Gat died failure conditions
	on_death("rn07_failure_gat_died", CHARACTER_GAT)

	-- Handle gat taking damage in script
	on_take_damage("rn07_handle_gat_damaged",CHARACTER_GAT)
	turn_invulnerable(CHARACTER_GAT, false)
	set_max_hit_points(CHARACTER_GAT, GAT_HIT_POINTS)
	hud_bar_on(0, "Health", HELPTEXT_HUD_GAT_HEALTH, GAT_HIT_POINTS)

	-- Don't let the gurney do things that a gurney shouldn't do
	character_allow_ragdoll(CHARACTER_GAT,false)
	character_prevent_flinching(CHARACTER_GAT,true)
	inv_item_remove_all(CHARACTER_GAT)
	npc_combat_enable(CHARACTER_GAT,false)
	set_attack_enemies_flag(CHARACTER_GAT, false) 
	set_attack_peds_flag(CHARACTER_GAT, false) 
	set_attack_player_flag(CHARACTER_GAT, false)
	set_cant_flee_flag(CHARACTER_GAT, true)

	-- Slowly leech away Gat's health
	Thread_syphon = thread_new("rn07_health_syphon_gat", CHARACTER_GAT)

end

function rn07_start_hospital_patrols()
	Thread_floor_patrols["floor_1"] = thread_new("rn07_patrol_floor","floor_1")
	Thread_floor_patrols["floor_2"] = thread_new("rn07_patrol_floor","floor_2")
	Thread_floor_patrols["floor_3"] = thread_new("rn07_patrol_floor","floor_3")
	Thread_floor_patrols["floor_4"] = thread_new("rn07_patrol_floor","floor_4")
end

-- Do checkpoint-specific initialization
function rn07_initialize_checkpoint(checkpoint, is_restart)		

	if (checkpoint == CHECKPOINT_START) then

		set_current_hit_points(CHARACTER_GAT, GAT_HIT_POINTS * GAT_PERCENT_HEALTH)
		hud_bar_set_value(0, GAT_HIT_POINTS * GAT_PERCENT_HEALTH, SYNC_LOCAL )		

		-- Put players in the initial area for player tracking. This causes enemy
		-- waves to begin spawning.
		Local_player_location			= "floor_2_area_1"
		if (IN_COOP) then
			Remote_player_location		= "floor_2_area_1"
		end
		
		rn07_setup_rpg_guy()

		for i, npc in pairs(STATIONARY_GROUP_MEMBERS) do
			attack(npc)
		end
		
		if (IN_COOP) then
			for i, npc in pairs(STATIONARY_GROUP_MEMBERS_COOP) do
				attack(npc)
			end
		end			

		if (not IN_COOP) then
			thread_new("rn07_check_ambulance_distance")
		end
		
		-- Teleport players to the start location
		if (is_restart) then
			teleport_coop(NAVPOINT_LOCAL_PLAYER_START, NAVPOINT_REMOTE_PLAYER_START, true)
		end


	elseif (checkpoint == CHECKPOINT_HOSPITAL) then

		-- Start loading groups
		rn07_load_later_groups()

		group_create(GROUP_GAT, true)

		local min_gat_hit_points = GAT_HIT_POINTS * GAT_MIN_RESTART_PERCENT_HEALTH
		local gat_restart_hit_points = max(min_gat_hit_points, Rn07_gat_health)

		set_current_hit_points(CHARACTER_GAT, gat_restart_hit_points)
		hud_bar_set_value(0, gat_restart_hit_points, SYNC_LOCAL )		
		teleport(CHARACTER_GAT, NAVPOINT_CHECKPOINT_GAT)

		Local_player_location			= "floor_1_area_4"

		Pushing_gat = true

	end

end

-- Handles enabling/disabling of the grenade slot for the local player depending on whether
-- or not they hodling Gat.
function rn07_control_grenade_slot()

	local grenades_disabled = false

	local function should_disable_grenades()
		if ( (not character_exists(CHARACTER_GAT)) or character_is_dead(CHARACTER_GAT)) then
			return false
		end
		return character_has_specific_human_shield(LOCAL_PLAYER, CHARACTER_GAT)
	end

	while(true) do
		thread_yield()

		local should_disable = should_disable_grenades()
		if ( should_disable and (not grenades_disabled) ) then
			inv_weapon_disable_slot(WEAPON_SLOT_THROWN, true, SYNC_LOCAL)
			grenades_disabled = true
			sprint_set_local_use_override(GURNEY_SPRINT_USE_OVERRIDE_MS)
			sprint_set_local_recharge_override(GURNEY_SPRINT_RECHARGE_OVERRIDE_MS)
		end

		if ( (not should_disable) and grenades_disabled) then
			inv_weapon_disable_slot(WEAPON_SLOT_THROWN, false, SYNC_LOCAL)
			grenades_disabled = false
			sprint_reset_local_overrides()
		end

	end

end

function rn07_local_player_to_first_floor()

	-- Start coop player's thread
	Thread_remote_player = thread_new("rn07_start_coop")

	-- Send local player to the foot of Gat's bed and have him start pushing gat
	rn07_send_to_location(TRIGGER_BED, INGAME_EFFECT_LOCATION, HELPTEXT_PROMPT_BED, nil, nil, SYNC_LOCAL)
	Pushing_gat = true
	
	--Wait till the player grabs the bed
	while (not character_has_specific_human_shield(LOCAL_PLAYER, CHARACTER_GAT)) do
		thread_yield()
	end

	-- Tell the player he can sprint when he's holding the bed
	mission_help_table_nag(HELPTEXT_PROMPT_BED_SPRINT, "", "", SYNC_LOCAL)
	delay(5.0)
	
	-- Send local player to the 2nd floor elevator
	rn07_send_to_location({TRIGGER_BREADCRUMB_4, TRIGGER_ELEVATOR_2}, INGAME_EFFECT_LOCATION, HELPTEXT_PROMPT_EXIT_HOSPITAL, nil, nil, SYNC_LOCAL)

	if(not character_has_specific_human_shield(LOCAL_PLAYER, CHARACTER_GAT)) then

		-- If the player isn't pushing Gat, tell them not to leave w/out him.
		mission_help_table_nag(HELPTEXT_PROMPT_NO_GAT_ELEVATOR)
		
		local close_enough = get_dist(LOCAL_PLAYER, TRIGGER_ELEVATOR_2) < 1.5
		local has_gat = character_has_specific_human_shield(LOCAL_PLAYER, CHARACTER_GAT)
		while( (not has_gat) or (not close_enough) ) do
			thread_yield()
		end
	end
	
	-- Elevator was reached, warp to next floor
	player_controls_disable(LOCAL_PLAYER)
	fade_out(1.0,{0,0,0}, SYNC_LOCAL)
	fade_out_block()
	teleport(LOCAL_PLAYER, TRIGGER_ELEVATOR_1)

	rn07_set_hit_points_gat(GAT_HIT_POINTS, CHARACTER_GAT)
	fade_in(1.0, SYNC_LOCAL)
	player_controls_enable(LOCAL_PLAYER)

	Local_player_location = "floor_1_area_2"

	-- Tell the player that Medicine was found for Gat.
	mission_help_table_nag(HELPTEXT_PROMPT_HELICOPTER_MED, "", "", SYNC_LOCAL)
	delay(3.0)

	-- Send local player to the front door
	group_show(GROUP_OUTSIDE_HOSPITAL)

end

function rn07_local_player_the_ambulance()

	-- Send player to the ambulance, make the rpg guy and ambulance invulnerable for now
	local thread_send_to_ambulance = thread_new("rn07_send_to_ambulance")
	turn_invulnerable(CHARACTER_RPG_GUY, false)
	turn_invulnerable(VEHICLE_AMBULANCE, false)

	on_vehicle_destroyed("rn07_ambulance_destroyed",VEHICLE_AMBULANCE)

	while(not vehicle_is_destroyed(VEHICLE_AMBULANCE)) do
		thread_yield()
	end

	-- Clean-up leftovers from send-to-front-door, send-to-ambulance
	thread_kill(thread_send_to_ambulance)
	rn07_toggle_location_reached(LOCAL_PLAYER, TRIGGER_FRONT_DOOR, SYNC_LOCAL)
	rn07_toggle_location_reached(LOCAL_PLAYER, TRIGGER_AMBULANCE, SYNC_LOCAL)
	rn07_location_reached[SYNC_LOCAL] = true
	delay(1.0)

end

function rn07_local_player_get_to_parking_lot()

	-- Send to the front door
	rn07_send_to_location(TRIGGER_FRONT_DOOR, INGAME_EFFECT_LOCATION, HELPTEXT_PROMPT_FRONT_DOOR, nil, nil, SYNC_LOCAL)
	delay(1.0)

	-- Send player to the parking lot
	rn07_send_to_location(TRIGGER_PARKING_FOOT, INGAME_EFFECT_LOCATION, HELPTEXT_PROMPT_PARKING, nil, nil, SYNC_LOCAL)
	Local_reached_parking = true

	if(not Remote_reached_parking) then
		-- Tell player to hold their ground, wait for the helicopter
		thread_new("mission_help_table",HELPTEXT_PROMPT_HOLD_GROUND,"","",SYNC_LOCAL)	

		while(not Remote_reached_parking) do
			thread_yield()
		end
	end

	-- Stop pushing Gat now.
	Pushing_gat = false

	-- Stop overloading Ronin personas
	persona_override_group_stop(RONIN_PERSONAS,POT_SITUATIONS[POT_ATTACK])

	-- Place players,Gat in helicopter
	fade_out(1.0,{0,0,0}, SYNC_ALL)
	fade_out_block()

	rn07_swap_gats()

	-- TODO... need this to be bed-less gat model
	set_unjackable_flag(VEHICLE_HELICOPTER, false)
	vehicle_enter_teleport(CHARACTER_GAT_NORMAL, VEHICLE_HELICOPTER, 2)
	vehicle_enter_teleport(REMOTE_PLAYER, VEHICLE_HELICOPTER, 0)
	vehicle_enter_teleport(LOCAL_PLAYER, VEHICLE_HELICOPTER, 1)
	delay(1.0)
	fade_in(1.0, SYNC_ALL)
	fade_in_block()

	-- Player finds some medicine in the copter, heals Gat
	rn07_gat_entered_helicopter()

	Helicopter_entered[LOCAL_PLAYER] = true

	-- Send players to the Saints' HQ
	trigger_enable(TRIGGER_SAINTS_HQ)
	on_trigger("rn07_hq_reached", TRIGGER_SAINTS_HQ)
	thread_new("rn07_send_to_location",TRIGGER_SAINTS_HQ_LOCAL, INGAME_EFFECT_VEHICLE_LOCATION, HELPTEXT_PROMPT_BASE, nil, nil, SYNC_LOCAL)
	rn07_send_to_location(TRIGGER_SAINTS_HQ_REMOTE, INGAME_EFFECT_VEHICLE_LOCATION, HELPTEXT_PROMPT_BASE, nil, nil, SYNC_REMOTE)
	while(true) do
		thread_yield()
	end
end

function rn07_single_player_ending()

	-- Send player back to the 1st floor elevator
	rn07_send_to_location({TRIGGER_BREADCRUMB_7, TRIGGER_ELEVATOR_1 }, INGAME_EFFECT_LOCATION, HELPTEXT_PROMPT_RETURN_ELEVATOR, nil, nil, SYNC_LOCAL)

	if(not character_has_specific_human_shield(LOCAL_PLAYER, CHARACTER_GAT)) then

		-- If the player isn't pushing Gat, tell them not to leave w/out him.
		mission_help_table_nag(HELPTEXT_PROMPT_NO_GAT_ELEVATOR)
		
		local close_enough = get_dist(LOCAL_PLAYER, TRIGGER_ELEVATOR_1) < 1.5
		local has_gat = character_has_specific_human_shield(LOCAL_PLAYER, CHARACTER_GAT)
		while( (not has_gat) or (not close_enough) ) do
			thread_yield()
		end
	end

	-- Elevator was reached, warp to the rooftop
	fade_out(1.0,{0,0,0}, SYNC_LOCAL)
	player_controls_disable(LOCAL_PLAYER)
	fade_out_block()
	teleport(LOCAL_PLAYER, NAVPOINT_ROOFTOP)
	door_open(DOOR_FLOOR_4_EXIT)
	door_open(DOOR_FLOOR_4_EXIT_2)

	fade_in(1.0, SYNC_LOCAL)
	player_controls_enable(LOCAL_PLAYER)

	delay(2.0)

	Local_player_location = "floor_4_area_3"

	rn07_send_to_location(	TRIGGER_BREADCRUMB_8, 
									INGAME_EFFECT_LOCATION,
									nil, 
									nil, 
									nil, 
									SYNC_LOCAL)

	-- Stop pushing Gat
	Pushing_gat = false

	-- Stop overloading Ronin personas
	persona_override_group_stop(RONIN_PERSONAS,POT_SITUATIONS[POT_ATTACK])

	trigger_enable(TRIGGER_HELICOPTER,false)
	on_trigger("",TRIGGER_HELICOPTER)
	marker_remove_vehicle(VEHICLE_HELICOPTER)

	thread_new("rn07_heli_chase_player", LOCAL_PLAYER)

	-- Place player,Gat in helicopter
	fade_out(1.0,{0,0,0}, SYNC_LOCAL)
	fade_out_block()

	rn07_swap_gats()
	
	-- TODO... need this to be bed-less gat model
	set_unjackable_flag(VEHICLE_HELICOPTER, false)
	vehicle_enter_group_teleport({LOCAL_PLAYER, CHARACTER_GAT_NORMAL}, VEHICLE_HELICOPTER)
	vehicle_prevent_explosion_fling(VEHICLE_HELICOPTER, false)
	delay(1.0)
	fade_in(1.0, SYNC_LOCAL)

	-- Player finds some medicine in the copter, heals Gat
	rn07_gat_entered_helicopter()
	Helicopter_entered[LOCAL_PLAYER] = true

	-- Bump notoriety
	notoriety_set_max("Ronin",4)
	notoriety_set("Ronin",4)
	notoriety_set_min("Ronin",4)

	-- Send player back to the Saints' HQ
	rn07_send_to_location(TRIGGER_SAINTS_HQ, INGAME_EFFECT_VEHICLE_LOCATION, HELPTEXT_PROMPT_BASE, nil, nil, SYNC_LOCAL)

end

function rn07_send_to_ambulance()
	rn07_send_to_location(TRIGGER_FRONT_DOOR, INGAME_EFFECT_LOCATION, HELPTEXT_PROMPT_FRONT_DOOR, nil, nil, SYNC_LOCAL)
	delay(1.0)
	rn07_send_to_location(TRIGGER_AMBULANCE, INGAME_EFFECT_LOCATION, HELPTEXT_PROMPT_AMBULANCE, nil, nil, SYNC_LOCAL)

	-- Have the rpg guy blow up the ambulance if he hasn't already
	rn07_rpg_guy_destroy_ambulance()

end

-- Called whenever Gat enters the helicopter
function rn07_gat_entered_helicopter()

	-- Handle the helicopter taking damage in script so that we can apply a 
	-- portion of that damage to Gat.
	on_take_damage("rn07_handle_helicopter_damaged",VEHICLE_HELICOPTER)
	
	-- Play the Heli entered conversation
	Rn07_dialogue_mission = "heli"

	mission_help_table_nag(HELPTEXT_PROMPT_HELICOPTER_MED, "", "", SYNC_LOCAL)
	delay(3.0)
end

function rn07_setup_rpg_guy()

	-- Add the rocket launcher
	inv_item_add("rpg_launcher",0,CHARACTER_RPG_GUY)
	inv_item_equip("rpg_launcher", CHARACTER_RPG_GUY)

	-- Handle him taking damage in script
	on_take_damage("rn07_rpg_guy_damaged",CHARACTER_RPG_GUY)
	on_take_damage("rn07_ambulance_damaged",VEHICLE_AMBULANCE)

	-- Rocket d00d never misses
	set_perfect_aim(CHARACTER_RPG_GUY,true)

	-- Make the rpg guy invulnerable to everything
	turn_invulnerable(CHARACTER_RPG_GUY, false)
	character_disallow_ragdoll(CHARACTER_RPG_GUY)
	character_set_cannot_be_grabbed(CHARACTER_RPG_GUY, true)
	character_set_cannot_be_grabbed_by_player(CHARACTER_RPG_GUY, true)
	character_set_only_scripted_grabs(CHARACTER_RPG_GUY, true)
	character_prevent_explosion_fling(CHARACTER_RPG_GUY, true)
	character_prevent_flinching(CHARACTER_RPG_GUY, true)
	character_prevent_kneecapping(CHARACTER_RPG_GUY, true)
	character_set_combat_ready(CHARACTER_RPG_GUY, true, 600 * 1000)

end

function rn07_check_ambulance_distance()

	local function should_destroy_ambulance()

		if (get_dist(LOCAL_PLAYER, VEHICLE_AMBULANCE) < 20) then
			return true
		end
		if (	character_exists(CHARACTER_RPG_GUY) 
				and (not character_is_dead(CHARACTER_RPG_GUY)) 
				and (get_dist(LOCAL_PLAYER, CHARACTER_RPG_GUY) < 20)
			) then
			return true
		end
		
		return false

	end

	while(not should_destroy_ambulance() ) do
		thread_yield()
	end

	rn07_rpg_guy_destroy_ambulance()

end

function rn07_rpg_guy_damaged(rpg_guy, attacker, damage_percent, explosion)

	if(damage_percent < 0.1) then
		damage_percent = 0.1
	end

	local new_hit_points = damage_percent * get_max_hit_points(CHARACTER_RPG_GUY)
	set_current_hit_points(CHARACTER_RPG_GUY, new_hit_points)

	if ( (attacker == LOCAL_PLAYER) or (attacker == REMOTE_PLAYER) or explosion) then
		rn07_rpg_guy_destroy_ambulance()
	end

end

function rn07_ambulance_damaged(ambulance, attacker, damage_percent)
		
	if	(not vehicle_is_destroyed(ambulance) ) then

		-- If someone else attacked the ambulance, have the rpg guy destroy it
		if (attacker ~= CHARACTER_RPG_GUY) then
			rn07_rpg_guy_destroy_ambulance()
		-- If the rpg guy damaged the ambulance, detonate it
		else
			rn07_ambulance_detonate()
		end

	end

end

Rn07_ambulance_detonated = false
function rn07_ambulance_detonate()

	if (Rn07_ambulance_detonated or vehicle_is_destroyed(VEHICLE_AMBULANCE)) then
		return
	end
	Rn07_ambulance_detonated = true

	vehicle_prevent_explosion_fling(VEHICLE_AMBULANCE,false)
	turn_vulnerable(VEHICLE_AMBULANCE)
	vehicle_detonate(VEHICLE_AMBULANCE)
end

Rn07_rpg_guy_destroying_ambulance = false
Rn07_rpg_guy_destruction_attempts = 0
function rn07_rpg_guy_destroy_ambulance()

	-- If the rpg guy is already trying to destroy the ambulance, just return
	if (Rn07_rpg_guy_destroying_ambulance) then
		return
	end

	Rn07_rpg_guy_destroying_ambulance = true

	-- Eliminate the on_take_damage callback
	on_take_damage("", CHARACTER_RPG_GUY)

	-- Try to blow up the ambulance 3 times. If we're not successful by the third time, just blow it up
	while(	(not vehicle_is_destroyed(VEHICLE_AMBULANCE)) 
				and (not character_is_dead(CHARACTER_RPG_GUY))
				and (Rn07_rpg_guy_destruction_attempts < 3)
			) do

		-- Blow up the ambulance
		force_fire(CHARACTER_RPG_GUY, VEHICLE_AMBULANCE, true)

		-- If we're on the last attempt, make sure that the ambulance detonates
		if (Rn07_rpg_guy_destruction_attempts == 3) then
			rn07_ambulance_detonate()
		end

		delay(1.0)
	end

	-- Allow Rocket man to miss
	set_perfect_aim(CHARACTER_RPG_GUY,false)

	-- Make the rocket guy vulnerable again.	
	turn_vulnerable(CHARACTER_RPG_GUY)
	character_allow_ragdoll(CHARACTER_RPG_GUY, true)
	character_set_cannot_be_grabbed(CHARACTER_RPG_GUY, false)
	character_set_cannot_be_grabbed_by_player(CHARACTER_RPG_GUY, false)
	character_set_only_scripted_grabs(CHARACTER_RPG_GUY, false)
	character_prevent_explosion_fling(CHARACTER_RPG_GUY, false)
	character_prevent_flinching(CHARACTER_RPG_GUY, false)
	character_prevent_kneecapping(CHARACTER_RPG_GUY, false)

end

function rn07_ambulance_destroyed()

	on_take_damage("",VEHICLE_AMBULANCE)

	Rn07_dialogue_mission = "ambulance"

	-- Send player back to the 1st floor elevator
	rn07_send_to_location({TRIGGER_BREADCRUMB_7, TRIGGER_ELEVATOR_1 }, INGAME_EFFECT_LOCATION, HELPTEXT_PROMPT_RETURN_ELEVATOR, nil, nil, SYNC_LOCAL)

end

-- Called when mission starts in coop
function rn07_start_coop()

	-- Send player to the stairwell
	rn07_send_to_location(TRIGGER_STAIRWELL, INGAME_EFFECT_LOCATION, HELPTEXT_PROMPT_STAIRWELL, nil, nil, SYNC_REMOTE)

	-- Send player to the elevator
	rn07_send_to_location(TRIGGER_ELEVATOR_3, INGAME_EFFECT_LOCATION, HELPTEXT_PROMPT_ROOFTOP_ELEVATOR, nil, nil, SYNC_REMOTE)

	mission_debug("rn07_start_coop: elevator arrival",-1)

	-- Elevator was reached, warp to the rooftop
	fade_out(1.0,{0,0,0}, SYNC_REMOTE)
	delay(1.0)
	teleport(REMOTE_PLAYER, NAVPOINT_ROOFTOP)
	door_open(DOOR_FLOOR_4_EXIT)
	door_open(DOOR_FLOOR_4_EXIT_2)
	fade_in(1.0, SYNC_REMOTE)

	-- Prompt the player to enter the helicopter, add minimap and in-game icon to helicopter
	set_unjackable_flag(VEHICLE_HELICOPTER, false)
	thread_new("mission_help_table",HELPTEXT_PROMPT_ENTER_HELICOPTER,"","",SYNC_REMOTE)
	marker_add_vehicle(VEHICLE_HELICOPTER, MINIMAP_ICON_PROTECT_ACQUIRE, INGAME_EFFECT_VEHICLE_PROTECT_ACQUIRE, SYNC_REMOTE)

	-- TODO, when we can hide stuff again in COOP, reenable this
	thread_new("rn07_heli_chase_player", REMOTE_PLAYER)

	-- Wait for a player to enter the helicopter
	on_vehicle_enter("rn07_helicopter_entered_remote", VEHICLE_HELICOPTER)

	while (not Helicopter_entered[REMOTE_PLAYER]) do
		thread_yield()
	end
	vehicle_prevent_explosion_fling(VEHICLE_HELICOPTER, false)

	-- Prompt to fly down to the parking lot, pick up partner
	rn07_send_to_location(TRIGGER_PARKING_HELICOPTER, INGAME_EFFECT_VEHICLE_LOCATION, HELPTEXT_PROMPT_PICKUP, nil, nil, SYNC_REMOTE)
	Remote_reached_parking = true

	if(not Local_reached_parking) then
		-- Tell player to defend the helicopter while waiting for their partner to arrive
		thread_new("mission_help_table",HELPTEXT_PROMPT_DEFEND_HELICOPTER,"","",SYNC_REMOTE)

		--rn07_yield_until(Local_reached_parking)
		while(not Local_reached_parking) do
			thread_yield()
		end
	end
		
end

-- Keep track of the distance between the local player and Gat
function rn07_monitor_distance()
	while (not Helicopter_entered[LOCAL_PLAYER]) do
		-- calculate distances to current dealer's trigger
		local dist_to_gat = get_dist(LOCAL_PLAYER, CHARACTER_GAT)

		if (not Gat_too_far and dist_to_gat > GAT_DISTANCE_ALLOWED) then
			Gat_too_far = true

			-- Tell the player to get back to Gat
			mission_help_table_nag(HELPTEXT_PROMPT_RETURN_GAT, "", "", SYNC_LOCAL)

			-- Start the timer
			hud_timer_set(GAT_DISTANCE_HUD_INDEX, GAT_DISTANCE_TIME_S*1000,"rn07_failure_gat_abandoned")

		elseif (Gat_too_far and dist_to_gat < GAT_DISTANCE_ALLOWED) then
			Gat_too_far = false

			-- Disable timer
			hud_timer_stop(GAT_DISTANCE_HUD_INDEX)
			objective_text_clear(GAT_DISTANCE_HUD_INDEX)
		end
	thread_yield()
	end
end

-- Handle lieutenant spawning for a floor
--
function rn07_patrol_floor(floor)

	-- Create the group, hidden
	group_create_hidden(FLOOR_PATROL_GROUPS[floor])

	-- Build a list of areas associated with this floor
	local num_areas = AREAS_PER_FLOOR[floor]
	local patrol_areas = {}
	for area_number = 1, num_areas, 1 do
		local area = floor .. "_area_" .. area_number	-- e.g. area= "floor_1_area_1"
		patrol_areas[area] = true
	end

	-- Wait for the group to load
	while (not group_is_loaded(FLOOR_PATROL_GROUPS[floor])) do
		thread_yield()
	end

	-- Monitor current number of Ronin on this floor, add more when needed
	while(true) do
		
		-- Determine which players are on this floor
		local local_present, remote_present = false, false
		if ( patrol_areas[Local_player_location] == true) then
			local_present = true;
		end
		if ( IN_COOP and patrol_areas[Remote_player_location] == true) then
			remote_present = true;
		end
		local player_present = local_present or remote_present

		-- Spawn a new set of Ronin if the current wave is dead and there is a player on this floor
		if (player_present and Floor_living_lieutenants[floor] == 0) then

			local lieutenants_to_spawn = rand_int(Min_wave_size, Max_wave_size)

			for i=1, lieutenants_to_spawn, 1 do

				-- Determine the target player
				local target = ""
				if (local_present and remote_present) then
					local coin_toss = rand_int(1,2)
					if(coin_toss == 1) then
						target = LOCAL_PLAYER
					else
						target = REMOTE_PLAYER
					end
				elseif (local_present) then
					target = LOCAL_PLAYER
				elseif (remote_present) then
					target = REMOTE_PLAYER
				end

				-- If we have a target
				if (target ~= "") then

					-- Select a new spawning site for each lieutenant. Makes the mission play better,
					-- and needs to be done in case the player(s) move between spawns
					local spawn_site = rn07_get_safe_spawn_area(FLOOR_SAFE_SPAWN_TABLE[floor])	

					-- There should always be a valid spawn area, but just in case
					if (not spawn_site) then
						break
					end
	
					-- Get the next lieutenant we'll be spawning
					local lieutenant = PATROL_GROUP_MEMBERS[floor][i]

					-- Determine the lieutenant's spawn navpoint
					local spawn_navpoint = SPAWN_NAVPOINT_TABLES[spawn_site][i]

					-- Teleport the lieutenant to the spawn navpoint
					teleport(lieutenant, spawn_navpoint)
					
					-- Tell the Lieutenant to respawn, override default respawn time
					npc_respawn_after_death(lieutenant, true)
					npc_respawn_after_death_time_override(lieutenant, LIEUTENANT_RESPAWN_TIME_MS, true)

					-- Add respawn callback... needs to be done after each death
					on_respawn("rn07_lieutenant_respawn",lieutenant)

					-- Show the lieutenant
					character_show(lieutenant)
	
					-- Store the target
					Lieutenant_target[lieutenant] = target

					-- Store the first navpoint that the lieutenant should pathfind to
					Lieutenant_first_navpoint[lieutenant] = AREA_PATROL_START[spawn_site]

					-- Store the path that the lieutenant should be using
					Lieutenant_path[lieutenant] = PATROL_NAVPOINT_TABLES[floor]

					-- Store the floor that the lieutenant is on
					Lieutenant_floor[lieutenant] = floor

					-- Start the patrol
					patrol("rn07_lieutenant_patrol",lieutenant)

					-- Increment living lieutenant count
					Floor_living_lieutenants[floor] = Floor_living_lieutenants[floor] + 1

					-- Delay just a tad between spawns
					delay(rand_float(.1,.4))

				end
			end
		end
		thread_yield()
	end
end

function rn07_lieutenant_respawn(lieutenant)

	-- Hide the lieutenant for now 
	character_hide(lieutenant)

	-- Update the count of living lieutenants on this floor
	Floor_living_lieutenants[Lieutenant_floor[lieutenant]] = Floor_living_lieutenants[Lieutenant_floor[lieutenant]] - 1

end

function rn07_lieutenant_patrol(lieutenant)

	-- Get the patrol path to follow
	local patrol_navpoints = Lieutenant_path[lieutenant]

	-- Get the index of the first navpoint to pathfind to
	local patrol_navpoint_index = Lieutenant_first_navpoint[lieutenant]

	-- Determine move_to speed. 2==run, 3== sprint
	local roll = rand_int(1,100)
	local move_speed = 2
	if (roll <= LIEUTENANT_SPRINT_CHANCE) then
		move_speed = 3		
	end

	-- Send to the first navpoint
	move_to(lieutenant, patrol_navpoints[patrol_navpoint_index], move_speed, false, false)

	-- Get the number of navpoints in this path
	local number_navpoints = sizeof_table(patrol_navpoints)

	-- Get the floor that the lieutenant is on
	local floor = Lieutenant_floor[lieutenant]

	-- Get the player target for this lieutenant
	local player_target = Lieutenant_target[lieutenant]

	-- function to get the current target's location
	local function get_target_location(target)

		if (not IN_COOP) then
			return Local_player_location
		end

		if (target == LOCAL_PLAYER) then
			return Local_player_location
		else
			return Remote_player_location
		end
	end

	-- Start patrolling
	while (1) do

		-- Get the current lieutenant's current location
		local current_area = FLOOR_PATROL_NAVPOINT_TABLES[floor][patrol_navpoint_index]

		local target_location = get_target_location(player_target)

		-- If the target is in the same location, wander around the current location for a bit,
		-- then move back to current navpoint
		if (current_area == target_location) then
			wander_start(lieutenant, PATROL_NAVPOINT_TABLES[floor][patrol_navpoint_index], rand_float(8,16))
			delay(rand_float(8,16))
			wander_stop(lieutenant)
			move_to(lieutenant, PATROL_NAVPOINT_TABLES[floor][patrol_navpoint_index], 2, false, false)
		else
			-- Get the direction we need to travel to get to the player
			local patrol_direction = AREA_FLOOR_PATROL_DIRECTION[current_area][target_location]

			if (player_target == REMOTE_PLAYER and patrol_direction and not LTNT_DEBUGGED) then
					mission_debug(lieutenant .. " initial target location chosen: " .. target_location,-1)
					mission_debug(lieutenant .. " initial patrol direction: " .. patrol_direction,-1)
			end

			-- If we couldn't find a direction, and the target isn't in the lieutenant's area,
			-- then the target is no longer on this floor. Check to see if we can switch to the other player
			if (not patrol_direction and current_area ~= target_location and IN_COOP) then
				if(player_target == LOCAL_PLAYER) then
					player_target = REMOTE_PLAYER
					if (not LTNT_DEBUGGED) then
						mission_debug(lieutenant .. " switching to remote player target ",-1)
					end
				else
					player_target = LOCAL_PLAYER
					if (not LTNT_DEBUGGED) then
						mission_debug(lieutenant .. " switching to local player target ", -1)
					end
				end
				target_location = get_target_location(player_target)
				patrol_direction = AREA_FLOOR_PATROL_DIRECTION[current_area][target_location]
			end

			if (player_target == REMOTE_PLAYER and patrol_direction and not LTNT_DEBUGGED) then
					mission_debug(lieutenant .. " new target location chosen: " .. target_location,-1)
					mission_debug(lieutenant .. " new patrol direction: " .. patrol_direction,-1)
			end

			-- If we still don't have a direction listed, no players are on this floor... don't process
			if (patrol_direction) then

				-- Get the next navpoint along the path in the correct direction
				if (patrol_direction == "forward") then
					patrol_navpoint_index = patrol_navpoint_index + 1
					if (patrol_navpoint_index > number_navpoints) then
						patrol_navpoint_index = patrol_navpoint_index - number_navpoints
					end
				else
					patrol_navpoint_index = patrol_navpoint_index - 1
					if (patrol_navpoint_index < 1) then
						patrol_navpoint_index = patrol_navpoint_index + number_navpoints
					end
				end

				if (player_target == REMOTE_PLAYER and not LTNT_DEBUGGED) then
						mission_debug(lieutenant .. " target navpoint: " .. FLOOR_PATROL_NAVPOINT_TABLES[floor][patrol_navpoint_index] .. target_location,-1)
				end

				-- Move to the next navpoint along the path
				move_to(lieutenant, PATROL_NAVPOINT_TABLES[floor][patrol_navpoint_index], 2, false, false)
		
			-- Else, no player on this floor. Wander around a bit and return.
			else
				wander_start(lieutenant, PATROL_NAVPOINT_TABLES[floor][patrol_navpoint_index], rand_float(10,25))
				delay(rand_float(5,12))
				wander_stop(lieutenant)
				move_to(lieutenant, PATROL_NAVPOINT_TABLES[floor][patrol_navpoint_index], 2, false, false)
			end
		
			LTNT_DEBUGGED = true
		end -- Ends else for if(in same areas)

		-- I don't think this is required, but I'm being safe
		thread_yield()
	end -- ends While(1)
end

-- Returns a random popping-safe spawn area
function rn07_get_safe_spawn_area(safe_spawn_areas)

	local valid_local_player_spawn_areas = safe_spawn_areas[Local_player_location]
	local valid_remote_player_spawn_areas = safe_spawn_areas[Remote_player_location]
	local valid_areas = {}

	-- If neither player is on the floor, return nil
	if (not valid_local_player_spawn_areas and not valid_remote_player_spawn_areas) then
		return nil
	end
	
	-- Only the local player is present, use their entire list
	if (valid_local_player_spawn_areas and not valid_remote_player_spawn_areas) then
		for i,area in pairs(valid_local_player_spawn_areas) do
			valid_areas[area] = area
		end
	end

	-- Only remote player present, use their entire list
	if (not valid_local_player_spawn_areas and valid_remote_player_spawn_areas) then
		for i,area in pairs(valid_remote_player_spawn_areas) do
			valid_areas[area] = area
		end
	end

	-- Both players present .. only add those areas valid for both players
	if (valid_local_player_spawn_areas and valid_remote_player_spawn_areas) then

		-- First grab all of the local players safe areas
		local local_valid_areas = {}
		for i,area in pairs(valid_local_player_spawn_areas) do
			local_valid_areas[area] = area
		end

		-- If the are is also in the remote player's list, then it is safe for both
		for i,area in pairs(valid_remote_player_spawn_areas) do
			if(local_valid_areas[area]) then
				valid_areas[area] = area
			end
		end

	end

	-- Get the total number of valid spawn areas
	local num_valid_spawn_areas = sizeof_table(valid_areas)

	-- If there aren't any valid spawn areas, return nil
	if (num_valid_spawn_areas == 0) then
		return nil
	end

	-- Select a random valid spawn area index
	local spawn_index = rand_int(1,num_valid_spawn_areas)

	-- Find the valid spawn area corresponding to the random index
	local count = 1
	for index,value in pairs(valid_areas) do
		if (count == spawn_index) then
			return value
		end
		count = count + 1
	end

	-- Shouldn't get here
	return nil

end

function rn07_setup_player_tracking_triggers()
	-- enable area-entrance triggers
	for entrance, area in pairs(ENTRANCE_TARGET_AREAS) do
		trigger_enable(entrance, true)
		on_trigger("rn07_area_entered",entrance)
		trigger_set_delay_between_activations(entrance,0)
	end
end

function rn07_area_entered(triggerer, trigger)
	if (triggerer == LOCAL_PLAYER) then
		Local_player_location = ENTRANCE_TARGET_AREAS[trigger]
	else 
		Remote_player_location = ENTRANCE_TARGET_AREAS[trigger]
	end
end

-- Helicopter has been entered
function rn07_helicopter_entered_remote()
	Helicopter_entered[REMOTE_PLAYER] = true;
	
	-- remove the vehicle callback
	marker_remove_vehicle(VEHICLE_HELICOPTER,SYNC_REMOTE)
	on_vehicle_enter("",VEHICLE_HELICOPTER)
end

-- Gat slowly loses health.
--
function rn07_health_syphon_gat(gat)

	local hit_points_per_tick = GAT_HIT_POINTS / GAT_HEALTH_LOSS_TIME_S

	while (true) do
		if ( (not Gat_in_cardiac_arrest) and (fade_get_percent() == 0.0)) then
			local old_hit_points = get_current_hit_points(gat)
			rn07_set_hit_points_gat(old_hit_points - hit_points_per_tick, gat)
			delay(1.0)
		end
		thread_yield()
	end
end

-- Called when Gat is damaged
-- damage_percent is Gat's health percentage after the attack
function rn07_handle_gat_damaged(gat, attacker, damage_percent)

	if (not Gat_in_cardiac_arrest) then

		-- Set Gat's new hit point total
		local new_hit_points = damage_percent * get_max_hit_points(CHARACTER_GAT)
		rn07_set_hit_points_gat(new_hit_points, gat)

		-- Maybe have Gat say something
		if(rand_float(0,1) < .5) then
			Rn07_dialogue_gat = "GAT_RON07_TAKEDAM"
		end

	else

		-- Already in Cardiac arrest, see if the player caused the damage w/ shock paddles.
		-- if so, reset Gats health
		if(attacker == LOCAL_PLAYER and inv_item_is_equipped("shock_paddles",LOCAL_PLAYER)) then
		
			rn07_set_hit_points_gat(GAT_HIT_POINTS * GAT_PADDLE_HEALTH_RESTORE, CHARACTER_GAT)

			-- Tell the player that Gat was revived
			mission_help_table_nag(HELPTEXT_PROMPT_HAVE_PULSE,"","",SYNC_LOCAL)

		end

	end

end

function rn07_set_hit_points_gat(new_hit_points, gat)

	if (Gat_in_cardiac_arrest and new_hit_points > 0) then
		-- Disable timer
		hud_timer_stop(GAT_REVIVE_HUD_INDEX)
		objective_text_clear(GAT_REVIVE_HUD_INDEX)
		Gat_in_cardiac_arrest = false
	end
	
	-- Play a warning sound/message when Gat's health drops below certain levels
	if (new_hit_points >= 1) then
		local warning_levels		= {0.50, 0.30, 0.10}

		local warning_messages	= {HELPTEXT_PROMPT_GAT_HEALTH_1, HELPTEXT_PROMPT_GAT_HEALTH_2,
											HELPTEXT_PROMPT_GAT_HEALTH_3}
		local old_hit_points		= get_current_hit_points(gat)

		for i,health_ratio in pairs(warning_levels) do
			if (	old_hit_points > health_ratio * GAT_HIT_POINTS and
					new_hit_points <= health_ratio * GAT_HIT_POINTS
				) then
				audio_play("SYS_HEALTH_WARNING")
				mission_help_table_nag(warning_messages[i], "", "", SYNC_LOCAL)
			end
		end
	end

	--[[
	-- If this attack would normally kill Gat, make him go into cardiac arrest instead
	if (	(new_hit_points < 1) and (not Helicopter_entered)) then

		new_hit_points = 1
		set_current_hit_points(CHARACTER_GAT, new_hit_points)
		hud_bar_set_value(0,new_hit_points, SYNC_ALL)
		Gat_in_cardiac_arrest	= true

		-- Tell the player that Gat has gone into cardiac arrest
		mission_help_table_nag(HELPTEXT_PROMPT_CARDIAC_ARREST, "", "", SYNC_LOCAL)

		-- Add a timer
		hud_timer_set(GAT_REVIVE_HUD_INDEX, GAT_REVIVE_TIME_S*1000,"rn07_failure_gat_died")
		--objective_text(GAT_REVIVE_HUD_INDEX, HELPTEXT_PROMPT_SHOCK_PADDLES)
		delay(3)

		-- Tell the player to find some shock paddles if he doesn't have any
		if (not inv_has_item("shock_paddles", LOCAL_PLAYER)) then
			mission_help_table(HELPTEXT_PROMPT_SHOCK_PADDLES,"","",SYNC_LOCAL)

			-- Wait for the player to obtain shock paddles
			while(not inv_has_item("shock_paddles", LOCAL_PLAYER)) do
				thread_yield()
			end

		end

		-- Tell the player to use the shock paddles to revive Gat
		mission_help_table(HELPTEXT_PROMPT_USE_PADDLES,"","",SYNC_LOCAL)
	elseif (new_hit_points < 1) then
		new_hit_points = 0
		rn07_failure_gat_died()
	end
	]]


	-- When Gat actually dies, animation badness sometimes ensues... don't let that happen!
	local gat_died = (new_hit_points <= 0)
	new_hit_points = max(new_hit_points, 1)
	set_current_hit_points(gat, new_hit_points)
	hud_bar_set_value(0,new_hit_points, SYNC_ALL)

	if (gat_died) then
		rn07_failure_gat_died()
	end
end

rn07_old_helicopter_health = 1.0
function rn07_handle_helicopter_damaged(helicopter, attacker, damage_percent)

	-- Determine how much damage was done to the helicopter, apply the damage
	local helicopter_new_hit_points = damage_percent * get_max_hit_points(helicopter)
	local helicopter_old_hit_points = rn07_old_helicopter_health * get_max_hit_points(helicopter)
	local helicopter_damage = helicopter_old_hit_points - helicopter_new_hit_points
	set_current_hit_points(helicopter, helicopter_new_hit_points)
	rn07_old_helicopter_health = damage_percent

	-- Determine how much damage to apply to Gat, apply the damage
	local gat_damage = GAT_HELI_DAMAGE_RATIO * helicopter_damage
	local gat_current_hit_points = get_current_hit_points(CHARACTER_GAT_NORMAL)
	rn07_set_hit_points_gat(gat_current_hit_points - gat_damage, CHARACTER_GAT_NORMAL)
end

Current_minimap_mode = {[LOCAL_PLAYER] = MINIMAP_MODE_NONE, [REMOTE_PLAYER] = MINIMAP_MODE_NONE}
function rn07_process_minimap(player)

	Current_minimap_mode[player]	= MINIMAP_MODE_NONE

	while(true) do

		-- Determine the next minimap mode
		local next_minimap_mode = MINIMAP_MODE_NONE
		for mode = MINIMAP_MODE_FIRST, MINIMAP_MODE_LAST, 1 do
			if (MINIMAP_MODE_FUNCTIONS[mode]["mode_is_active"](player)) then
				next_minimap_mode = mode
				break
			end
		end

		-- If the mode has changed, exit the old mode and enter the new
		if (next_minimap_mode ~= Current_minimap_mode[player]) then

			if (Current_minimap_mode[player] ~= MINIMAP_MODE_NONE) then
				MINIMAP_MODE_FUNCTIONS[Current_minimap_mode[player]]["exit"](player)
			end

			Current_minimap_mode[player] = next_minimap_mode
		
			if (next_minimap_mode ~= MINIMAP_MODE_NONE) then
				MINIMAP_MODE_FUNCTIONS[next_minimap_mode]["enter"](player)
			end

		end

		Current_minimap_mode[player] = next_minimap_mode

		thread_yield()
	
	end

end

-- Shock paddle marker functions
function rn07_should_display_paddle_markers(player)

	return false

	--[[
	if (player == REMOTE_PLAYER) then
		return false
	else
		return (Gat_in_cardiac_arrest and (not inv_has_item("shock_paddles", LOCAL_PLAYER)))
	end
	]]
end

function rn07_add_paddle_markers()

	-- Show the player where to find the shock paddles
	for i,shock_paddle in pairs(SHOCK_PADDLES) do
		marker_add_item(shock_paddle, MINIMAP_ICON_PROTECT_ACQUIRE, INGAME_EFFECT_PROTECT_ACQUIRE, SYNC_LOCAL)
	end

end

function rn07_remove_paddle_markers()

	-- Remove minimap icons, effects from shock paddles
	for i,shock_paddle in pairs(SHOCK_PADDLES) do
		marker_remove_item(shock_paddle, SYNC_LOCAL)
	end
end

-- Gat marker functions
function rn07_should_display_gat_markers(player)
	if (player == REMOTE_PLAYER) then
		return false
	else
		return (Pushing_gat and (not character_has_specific_human_shield(LOCAL_PLAYER, CHARACTER_GAT)))
	end
end

function rn07_add_gat_markers(player)
	marker_add_npc(CHARACTER_GAT, MINIMAP_ICON_PROTECT_ACQUIRE, INGAME_EFFECT_PROTECT_ACQUIRE, SYNC_LOCAL)
end

function rn07_remove_gat_markers(player)
	marker_remove_npc(CHARACTER_GAT, SYNC_LOCAL)
end

-- Heli marker functions
function rn07_should_display_heli_markers(player)

	-- If the player has not yet entered the helicopter, don't tell them to reenter it.
	if ( not Helicopter_entered[player] ) then
		return false
	end

	if	( character_is_in_vehicle(player, VEHICLE_HELICOPTER) ) then
		return false
	end

	return true

end

function rn07_add_heli_markers(player)
	mission_help_table_nag(HELPTEXT_PROMPT_REENTER_HELI, "", "", PLAYER_SYNC[player])
	marker_add_vehicle(VEHICLE_HELICOPTER, MINIMAP_ICON_PROTECT_ACQUIRE, INGAME_EFFECT_VEHICLE_PROTECT_ACQUIRE, PLAYER_SYNC[player])
end

function rn07_remove_heli_markers(player)
	marker_remove_vehicle(VEHICLE_HELICOPTER, PLAYER_SYNC[player])
end


-- Location marker functions
function rn07_should_display_location_markers(player)
	if (not rn07_location_reached[SYNC_ALL]) then
		return true
	end
	if (not rn07_location_reached[PLAYER_SYNC[player]]) then
		return true
	end
end

Rn07_last_location_message = {[LOCAL_PLAYER] = "", [REMOTE_PLAYER] = ""}
function rn07_add_location_markers(player)

	local sync = PLAYER_SYNC[player]
	if (not rn07_location_reached[SYNC_ALL]) then
		sync = SYNC_ALL
	end

	if (rn07_breadcrumb_effect[sync]) then

		local num_breadcrumbs = sizeof_table(rn07_breadcrumb_triggers[sync])

		-- First traverse the triggers in reverse. If we're close enough, count them as visited.
		local current_trigger = rn07_current_trigger[sync]
		for i = num_breadcrumbs, current_trigger, -1 do
			local trigger = rn07_breadcrumb_triggers[sync][i]
			if (get_dist(player, trigger) < 5) then
				if (i == num_breadcrumbs) then
					rn07_toggle_location_reached(player, trigger, sync)
				else
					rn07_breadcrumb_reached(player, trigger, sync)
				end
			end
		end

		-- If we triggered the final destination during our traversal, then we can return
		if (rn07_location_reached[sync]) then
			return
		end

		for i = rn07_current_trigger[sync], num_breadcrumbs, 1 do			
			trigger_enable(rn07_breadcrumb_triggers[sync][i],true)
		end
		local minimap_icon = MINIMAP_ICON_LOCATION
		if( rn07_breadcrumb_effect[sync] == INGAME_EFFECT_VEHICLE_PROTECT_ACQUIRE) then
			minimap_icon = MINIMAP_ICON_PROTECT_ACQUIRE
		end
		marker_add_trigger(rn07_breadcrumb_triggers[sync][rn07_current_trigger[sync]],minimap_icon,rn07_breadcrumb_effect[sync],sync)

		-- Display helptext
		if (rn07_breadcrumb_helptext[sync]) then
			-- If we've already played the message for this location, make it a nag
			if(Rn07_last_location_message[player] == rn07_breadcrumb_helptext[sync]) then		
				mission_help_table_nag(rn07_breadcrumb_helptext[sync], "", "", sync)
			else
			-- Otherwise, it is an objective
				mission_help_table(rn07_breadcrumb_helptext[sync], "", "", sync)
				Rn07_last_location_message[player] = rn07_breadcrumb_helptext[sync]
			end
		end

	end

end

function rn07_remove_location_markers(player)

	local sync = PLAYER_SYNC[player]
	if (not rn07_location_reached[SYNC_ALL]) then
		sync = SYNC_ALL
	end

	if (rn07_breadcrumb_effect[sync]) then

		marker_remove_trigger(rn07_breadcrumb_triggers[sync][rn07_current_trigger[sync]])

	end

end

	-- Minimap_mode_functions
	MINIMAP_MODE_FUNCTIONS	=
		{
			[MINIMAP_MODE_PADDLES]		= 
				{
					["mode_is_active"]	= rn07_should_display_paddle_markers,
					["enter"]				= rn07_add_paddle_markers,
					["exit"]					= rn07_remove_paddle_markers,	
				};
			[MINIMAP_MODE_GAT]			= 
				{
					["mode_is_active"]	= rn07_should_display_gat_markers,
					["enter"]				= rn07_add_gat_markers,
					["exit"]					= rn07_remove_gat_markers,	
				};
			[MINIMAP_MODE_HELI]			= 
				{
					["mode_is_active"]	= rn07_should_display_heli_markers,
					["enter"]				= rn07_add_heli_markers,
					["exit"]					= rn07_remove_heli_markers,	
				};
			[MINIMAP_MODE_LOCATIONS]	= 
				{
					["mode_is_active"]	= rn07_should_display_location_markers,
					["enter"]				= rn07_add_location_markers,
					["exit"]					= rn07_remove_location_markers,	
				};
		}

function rn07_heli_chase_player(player)
	-- Create the heli group
	group_create_hidden(GROUP_RONIN_HELICOPTER)

	vehicle_enter_group_teleport(RONIN_HELICOPTER_MEMBERS, VEHICLE_RONIN_HELI)

	-- Wait for loading
	while (not group_is_loaded(GROUP_RONIN_HELICOPTER)) do
		thread_yield()
	end

	-- give the back seat passenger a rifle
	inv_item_add("AR200",0,RONIN_HELICOPTER_MEMBERS[2])
	inv_item_equip("AR200", RONIN_HELICOPTER_MEMBERS[2])

	group_show(GROUP_RONIN_HELICOPTER)
	character_hide("rn07_$c031")

	vehicle_lights_on(VEHICLE_RONIN_HELI, true)
	helicopter_set_missile_chance(VEHICLE_RONIN_HELI, 0.001)
	vehicle_chase(VEHICLE_RONIN_HELI, player)
end

rn07_location_reached			= {[SYNC_ALL] = true, [SYNC_LOCAL] = true, [SYNC_REMOTE] = true}
rn07_current_trigger				= {[SYNC_ALL] = 1, [SYNC_LOCAL] = 1, [SYNC_REMOTE] = 1}
rn07_breadcrumb_triggers		= {[SYNC_ALL] = {}, [SYNC_LOCAL] = {}, [SYNC_REMOTE] = {}}
rn07_breadcrumb_effect			= {[SYNC_ALL] = "", [SYNC_LOCAL] = "", [SYNC_REMOTE] = ""}
rn07_breadcrumb_helptext		= {[SYNC_ALL] = nil, [SYNC_LOCAL] = nil, [SYNC_REMOTE] = nil}
-- Send a player to a particular location
-- 
-- Handles enabling/disabling of trigger effects when Gat goes into cardiac arrest or is revived.
-- Currently, assumes that breadcrumbing only happens for the local player.
--
--	trigger:						If a table, then the final trigger is the destination and other triggers are breadcrumbs
--									If scalar, then this is the destination.
-- effect:						Effect to be placed on the trigger(s).
-- helptext:					Helptext to display when the function is called, or when triggers are reenabled after Gat
--										comes out of cardiac arrest.
-- time:							Time constraint if there is one.
-- time_failure_function:	Function called if time runs out.
-- sync:							Sync type.
--
function rn07_send_to_location(trigger, effect, helptext, time, time_failure_function, sync)

	rn07_location_reached[sync] = false
	rn07_breadcrumb_effect[sync] = false
	if (effect) then
		rn07_breadcrumb_effect[sync] = effect
	end
	rn07_current_trigger[sync] = 1
	rn07_breadcrumb_triggers[sync] = {}
	rn07_breadcrumb_helptext[sync] = nil

	-- Place triggers into a global table variable
	if ((type(trigger) == "table")) then
		rn07_breadcrumb_triggers[sync] = trigger
	else
		rn07_breadcrumb_triggers[sync][1] = trigger
	end

	local final_trigger_function = 
		{	
			[SYNC_ALL] = "rn07_toggle_location_reached_all", 
			[SYNC_LOCAL] = "rn07_toggle_location_reached_local", 
			[SYNC_REMOTE] = "rn07_toggle_location_reached_remote"
		}
	local breadcrumb_trigger_function = 
		{	
			[SYNC_ALL] = "rn07_breadcrumb_reached_all", 
			[SYNC_LOCAL] = "rn07_breadcrumb_reached_local", 
			[SYNC_REMOTE] = "rn07_breadcrumb_reached_remote"
		}

	-- Add callbacks for all triggers, special callback for final trigger
	local num_breadcrumbs = sizeof_table(rn07_breadcrumb_triggers[sync])
	for i,breadcrumb in pairs(rn07_breadcrumb_triggers[sync]) do
		trigger_enable(breadcrumb,true)
		if (i == num_breadcrumbs) then
			on_trigger(final_trigger_function[sync],breadcrumb)					
		else
			on_trigger(breadcrumb_trigger_function[sync], breadcrumb)
		end
	end

	-- Setup the time constraint if there is one.
	if (time and time_failure_function) then
		hud_timer_set(GAT_REVIVE_HUD_INDEX, time, time_failure_function)
	end

	-- Set helptext
	if (helptext) then
		--mission_help_table(helptext, "", "", sync)
		rn07_breadcrumb_helptext[sync] = helptext
	end

	-- wait for player(s) to arrive
	while (not rn07_location_reached[sync]) do

		thread_yield()

	end

	--hud_timer_stop(GAT_REVIVE_HUD_INDEX)
end

function rn07_breadcrumb_reached_local(triggerer, trigger)
	if (triggerer == LOCAL_PLAYER) then
		rn07_breadcrumb_reached(triggerer, trigger, SYNC_LOCAL)
	end
end

function rn07_breadcrumb_reached_remote(triggerer, trigger)
	if (triggerer == REMOTE_PLAYER) then
		rn07_breadcrumb_reached(triggerer, trigger, SYNC_REMOTE)
	end
end

function rn07_breadcrumb_reached_all(triggerer, trigger)
	rn07_breadcrumb_reached(triggerer, trigger, SYNC_ALL)
end

function rn07_breadcrumb_reached(triggerer, trigger, sync)

	if ( Current_minimap_mode[triggerer] ~= MINIMAP_MODE_LOCATIONS ) then
		return
	end

	for i = rn07_current_trigger[sync], sizeof_table(rn07_breadcrumb_triggers[sync]), 1 do
		local breadcrumb = rn07_breadcrumb_triggers[sync][i]
		trigger_enable(breadcrumb,false)
		on_trigger("",trigger)
		marker_remove_trigger(trigger, sync)
		rn07_current_trigger[sync] = i+1
		if(rn07_breadcrumb_triggers[sync][i] == trigger) then
			break
		end
	end

	-- Add an effect to the next trigger
	if (rn07_breadcrumb_effect[sync]) then
		local minimap_icon = MINIMAP_ICON_LOCATION
		if( rn07_breadcrumb_effect[sync] == INGAME_EFFECT_VEHICLE_PROTECT_ACQUIRE) then
			minimap_icon = MINIMAP_ICON_PROTECT_ACQUIRE
		end

		marker_add_trigger(	rn07_breadcrumb_triggers[sync][rn07_current_trigger[sync]],
									minimap_icon,
									rn07_breadcrumb_effect[sync],
									sync)

	end
		
end

function rn07_toggle_location_reached_all(triggerer,trigger)
	rn07_toggle_location_reached(triggerer, trigger, SYNC_ALL)
end

function rn07_toggle_location_reached_local(triggerer,trigger)
	if (triggerer == LOCAL_PLAYER) then
		rn07_toggle_location_reached(triggerer, trigger, SYNC_LOCAL)
	end
end

function rn07_toggle_location_reached_remote(triggerer,trigger)
	if (triggerer == REMOTE_PLAYER) then
		rn07_toggle_location_reached(triggerer, trigger, SYNC_REMOTE)
	end
end

function rn07_toggle_location_reached(triggerer, trigger, sync)

	if ( Current_minimap_mode[triggerer] ~= MINIMAP_MODE_LOCATIONS ) then
		return
	end

	rn07_location_reached[sync] = true		

	-- Disable all breadcrumbs
	for i = rn07_current_trigger[sync], sizeof_table(rn07_breadcrumb_triggers[sync]), 1 do
		local breadcrumb = rn07_breadcrumb_triggers[sync][i]
		trigger_enable(breadcrumb,false)
		on_trigger("",trigger)
		marker_remove_trigger(trigger, sync)
	end
	
end

function rn07_yield_until(condition)
	while(not condition) do
		thread_yield()
	end
end

-- Play dialogue lines. Lines are divided into three types: mission conversations,
-- random gat lines, and bicker conversations. Mission conversations have the highest
-- priority.
Rn07_dialogue_mission = ""
Rn07_dialogue_gat = ""
Rn07_dialogue_bicker = 1
function rn07_play_dialogue()

	local dialogue_delay = 0.25	-- The delay between dialogue checks
	local bicker_chance = .05		-- Chance that a bicker is played under valid conditions
	local min_bicker_ticks = 40	-- Min number ticks to skip between bicker conversations 
	local cur_bicker_ticks = 0		-- Current number of ticks
	
	--Randomize the order of the bicker conversations
	local rand_order = {}
	local num_bickers = sizeof_table(Rn07_bicker_conversations)
	for i = 1, num_bickers, 1 do
		rand_order[i] = i
	end
	for i = 1, num_bickers, 1 do
		local rand1 = rand_int(1,num_bickers)
		local rand2 = rand_int(1,num_bickers)
		rand_order[rand1],rand_order[rand2] = rand_order[rand2],rand_order[rand1]
	end

	-- Wait a few seconds before we start talking
	delay(8.0)

	-- Play the highest priority line that's available.
	while(true) do

		-- Tick down
		if (cur_bicker_ticks > 0) then
			cur_bicker_ticks = cur_bicker_ticks - 1
		end

		-- Play missions lines if available.
		if(Rn07_dialogue_mission ~= "") then

			audio_play_conversation(Rn07_mission_conversations[Rn07_dialogue_mission])		
			Rn07_dialogue_mission = ""

		-- Play a random Gat line if one is available
		elseif( (Rn07_dialogue_gat ~= "") and (not Gat_in_cardiac_arrest) ) then

			audio_play_for_character(Rn07_dialogue_gat, CHARACTER_GAT, "voice", false, true)

		-- Maybe play a bicker conversation if any are left and Gat isn't in cardiac arrest.
		elseif( (Rn07_dialogue_bicker ~= -1) and (not Gat_in_cardiac_arrest) and (cur_bicker_ticks == 0) ) then

			-- Make sure that the player and Gat are near each other
			local dist = get_dist(LOCAL_PLAYER, CHARACTER_GAT)
			local roll = rand_float(0,1)
			if( (dist <= 8) and (roll <= bicker_chance) ) then
				audio_play_conversation(Rn07_bicker_conversations[rand_order[Rn07_dialogue_bicker]])

				-- Make sure to wait a ceratin amount of time before the next bicker conversation
				cur_bicker_ticks = min_bicker_ticks

				-- Move to the next bicker conversation if there is one
				Rn07_dialogue_bicker = Rn07_dialogue_bicker + 1
				if(Rn07_dialogue_bicker > num_bickers) then
					Rn07_dialogue_bicker = -1
				end

			end
		end

		-- Gat's lines are triggered by events such as Gat taking damage. We don't want
		-- to play any of these if much time has passed since the event.
		Rn07_dialogue_gat = ""

		delay(dialogue_delay)
		thread_yield()
	end
end

-- Cleanup mission
function rn07_cleanup()

	IN_COOP = coop_is_active()

	-- Make sure that the players don't end up stuck in the hospital
	local local_maybe_stuck = true
	local remote_maybe_stuck = IN_COOP

	-- Not stuck if we're in the heli
	if (vehicle_exists(VEHICLE_HELICOPTER) and (not vehicle_is_destroyed(VEHICLE_HELICOPTER)) ) then
		if (character_is_in_vehicle(LOCAL_PLAYER, VEHICLE_HELICOPTER)) then
			local_maybe_stuck = false
		end
		if (IN_COOP and character_is_in_vehicle(REMOTE_PLAYER, VEHICLE_HELICOPTER)) then
			remote_maybe_stuck = false
		end
	end

	-- Not stuck if we're far from the hospital
	local hospital_radius = 70
	if (get_dist(LOCAL_PLAYER, NAVPOINT_HOSPITAL_CENTER) > hospital_radius) then
		local_maybe_stuck = false
	end
	if (IN_COOP and get_dist(REMOTE_PLAYER, NAVPOINT_HOSPITAL_CENTER) > hospital_radius) then
		remote_maybe_stuck = false
	end

	-- Teleport players who might be stuck to spots in front of the hospital
	if (remote_maybe_stuck) then
		teleport(REMOTE_PLAYER, NAVPOINT_REMOTE_MISSION_END)
	end
	if (local_maybe_stuck) then
		teleport(LOCAL_PLAYER, NAVPOINT_REMOTE_MISSION_END)
	end

	-- Enable the ambient ambulance parking spawn in front of the hospital
	if (not IN_COOP) then
		parking_spot_enable(PARKING_SPAWN_AMBULANCE, true)
	end

	-- Swap out the interior chunks
	city_chunk_swap("SR2_IntDTMisHospital", "normal", true, true, true)
	city_chunk_swap("SR2_IntDTMisHospital02", "normal", false, true, true)
	city_chunk_swap("SR2_IntDTMisHospital03", "normal", true, true, true)

	sprint_reset_local_overrides()

	-- Make sure that the player can use grenades
	inv_weapon_disable_slot(WEAPON_SLOT_THROWN, false, SYNC_LOCAL)

	if(vehicle_exists(VEHICLE_HELICOPTER) and (not vehicle_is_destroyed(VEHICLE_HELICOPTER))) then
		turn_vulnerable(VEHICLE_HELICOPTER)
	end

	if(character_has_specific_human_shield(LOCAL_PLAYER, CHARACTER_GAT)) then
		character_release_human_shield(LOCAL_PLAYER, true)
		clear_animation_state(LOCAL_PLAYER)
	end

	-- Stop overloading Ronin personas
	persona_override_group_stop(RONIN_PERSONAS,POT_SITUATIONS[POT_ATTACK])

	-- Reset the respawn distance
	npc_respawn_dist_reset()

	-- Reset Ronin notoriety
	notoriety_set_min("Ronin",0)
	notoriety_set_max("Ronin",5)
	notoriety_set("Ronin",0)
	
	--Reenable recruiting
	party_set_recruitable(true)

	-- Remove mnimap icons, ingame effects
	marker_remove_vehicle(VEHICLE_HELICOPTER)
	marker_remove_npc(CHARACTER_GAT)
	for i,shock_paddle in pairs(SHOCK_PADDLES) do
		marker_remove_item(shock_paddle, SYNC_LOCAL)
	end

	-- Remove callbacks

		-- Remove failure callbacks
		on_death("", CHARACTER_GAT)
		on_vehicle_destroyed("", VEHICLE_HELICOPTER)

		-- Remove damage callbacks
		on_take_damage("",CHARACTER_GAT)
		on_take_damage("",VEHICLE_HELICOPTER)

		-- Remove vehicle triggers
		on_vehicle_enter("",VEHICLE_HELICOPTER)

		-- disable all triggers, remove callbacks
		for i, trigger in pairs(TABLE_ALL_TRIGGERS) do
			on_trigger("",trigger)
			trigger_enable(trigger,false)
		end

		-- disable area-entrance triggers, remove callbacks
		for entrance, area in pairs(ENTRANCE_TARGET_AREAS) do
			on_trigger("",entrance)
			trigger_enable(entrance, false)
		end

		-- disable all area entrances, remove callbacks
		for entrance, area in pairs(ENTRANCE_TARGET_AREAS) do
			on_trigger("",entrance)
			trigger_enable(entrance,false)
		end

		-- Remove lieutenant callbacks
		for i,lieutenant in pairs(PATROL_GROUP_MEMBERS["floor_1"]) do
			on_death("",lieutenant)
			on_respawn("",lieutenant)
		end
		for i,lieutenant in pairs(PATROL_GROUP_MEMBERS["floor_2"]) do
			on_death("",lieutenant)			
			on_respawn("",lieutenant)
		end
		for i,lieutenant in pairs(PATROL_GROUP_MEMBERS["floor_3"]) do
			on_death("",lieutenant)
			on_respawn("",lieutenant)
		end
		for i,lieutenant in pairs(PATROL_GROUP_MEMBERS["floor_4"]) do
			on_death("",lieutenant)
			on_respawn("",lieutenant)
		end

	-- destroy all groups
	for i, group in pairs(TABLE_ALL_GROUPS) do
		group_destroy(group)
	end

	-- release all coop-only groups
	if (IN_COOP) then
		for i, group in pairs(TABLE_ALL_COOP_GROUPS) do
			group_destroy(group)
		end
	end
end

function rn07_swap_gats()

	-- Release old gat
	if(character_has_specific_human_shield(LOCAL_PLAYER, CHARACTER_GAT)) then
		character_release_human_shield(LOCAL_PLAYER, true)
	end
	delay(1.0)

	-- Set non gurney-gat's hit points
	set_max_hit_points(CHARACTER_GAT_NORMAL, GAT_HIT_POINTS_HELI)
	set_current_hit_points(CHARACTER_GAT_NORMAL, GAT_HIT_POINTS)

	-- Hide old gat, show new one
	character_hide(CHARACTER_GAT)
	character_show(CHARACTER_GAT_NORMAL)

	-- Eliminate callbacks from the old gat, add to new one (Normals gat's damage handled by rn07_handle_helicopter_damaged)
	on_death("", CHARACTER_GAT)
	on_take_damage("", CHARACTER_GAT)
	on_death("rn07_failure_gat_died", CHARACTER_GAT_NORMAL)

	--remove inventory from new Gat
	inv_item_remove_all(CHARACTER_GAT_NORMAL)

	-- Kill off the old syphon thread and start a new one
	thread_kill(Thread_syphon)
	Thread_syphon = thread_new("rn07_health_syphon_gat", CHARACTER_GAT_NORMAL)


end

function rn07_hq_reached(triggerer)
	if(vehicle_exists(VEHICLE_HELICOPTER) and character_is_in_vehicle(triggerer, VEHICLE_HELICOPTER)) then
		rn07_complete()
	end
end

function rn07_complete()
	--bink_play_movie("ro07-2.bik")

	-- End the mission with success
	mission_end_success("rn07", "ro07-02")
end


function rn07_success()
	-- Called when the mission has ended with success
end

-- MISSION FAILURE FUNCTIONS --------------------------------

function rn07_failure_gat_died()
	-- End the mission, Gat died
	mission_end_failure("rn07", HELPTEXT_FAILURE_GAT_DIED)
end

function rn07_failure_gat_abandoned()
	-- End the mission, Gat abandoned
	mission_end_failure("rn07", HELPTEXT_FAILURE_GAT_ABANDONED)
end

function rn07_failure_helicopter_destroyed()
	-- End the mission, Helicopter destroyed
	delay(2.0)
	mission_end_failure("rn07", HELPTEXT_FAILURE_HELICOPTER_DESTROYED)
end
