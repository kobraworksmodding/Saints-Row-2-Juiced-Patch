// MPStorage.cpp (clippy95)
// --------------------
// Created: 1/5/2026
#include "../FileLogger.h"
#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "Gamespy.h"
#include "../UtilsGlobal.h"
#include "STUNNat.h"
#include <Wininet.h>
#include "safetyhook.hpp"
#include "../Hooker.h"

#include <filesystem>
#include <fstream>
#include <cstring>
#include <cstddef>

namespace MPStorage
{

	struct sr2_mp_player_data
	{
		int data[0xEE8C / 4];
	};
	static_assert(sizeof(sr2_mp_player_data) == 0xEE8C,"Wrong size for sr2_mp_player_data");

	struct sr2_mp_data
	{
		uint16_t version;
		int current_cash;
		int lifetime_cash;
		uint8_t mp_rank;
		int mp_badge;
		uint32_t server_upstream;
		bool has_been_server;
		sr2_mp_player_data creation_data;
		uint8_t term_byte;
	};
	static_assert(sizeof(sr2_mp_data) == 0xEEAC, "Wrong size for sr2_mp_data");

	struct MPDT
	{
		uint32_t signature;
		uint32_t version;
		uint32_t offset_for_where_data_starts; // this points to sr2_mp_data in the file, maybe we can add like index multiple chars and stuff so a pointer to this is useful
		uint32_t sr2_mp_data_size;
		sr2_mp_data data;
	};

	sr2_mp_data* mp_data = (sr2_mp_data*)DynAddress(0x02A18A08);
	const char* where_to_save = (const char*)0x0144E650;

	void SetOnlineStuff(int thing1, int thing2) 
	{
		cdecl_call<void>(0x81A3F0, thing1, thing2);
		*(bool*)(0x2528C2C) = true;
	}

	void SUCESSTHING()
	{
		SetOnlineStuff(1, 1);
	}

	constexpr uint32_t MPDT_SIGNATURE = 'TDPM'; // "MPDT" little-endian
	constexpr uint32_t MPDT_VERSION = 1;

	static std::filesystem::path GetMPStoragePath()
	{
		std::filesystem::path base(where_to_save);

		// C:\Users\...\Saints Row 2\<FOLDER_NAME>\mp
		return base / FOLDER_NAME / "mp" / "MPStorage.mpdt_pc";
	}

	bool get_data_from_server()
	{
		auto path = GetMPStoragePath();
		*(bool*)(0x2528C2C) = true;
		if (!std::filesystem::exists(path))
			return false;

		std::ifstream file(path, std::ios::binary);
		if (!file)
			return false;

		MPDT header{};
		file.read(reinterpret_cast<char*>(&header), sizeof(MPDT));

		if (!file)
			return false;

		if (header.signature != MPDT_SIGNATURE)
			return false;

		if (header.version != MPDT_VERSION)
			return false;

		if (header.offset_for_where_data_starts != offsetof(MPDT, data))
			return false;

		if (header.sr2_mp_data_size != sizeof(sr2_mp_data))
			return false;


		std::memcpy(mp_data, &header.data, sizeof(sr2_mp_data));

		SUCESSTHING();

		return true;
	}

	bool wtf_get_data_from_server()
	{
		auto result = get_data_from_server();
		Logger::TypedLog("FUCKYOU", "THE THING IS UH {}\n", result);
		return result;
	}

	bool save_data_to_server(char console_command)
	{

		auto path = GetMPStoragePath();

		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		if (ec)
			return false;

		MPDT out{};
		out.signature = MPDT_SIGNATURE;
		out.version = MPDT_VERSION;
		out.offset_for_where_data_starts = offsetof(MPDT, data);
		out.sr2_mp_data_size = sizeof(sr2_mp_data);

		std::memcpy(&out.data, mp_data, sizeof(sr2_mp_data));

		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		if (!file)
			return false;

		file.write(reinterpret_cast<const char*>(&out), sizeof(MPDT));
		SUCESSTHING();
		return file.good();
	}

	void Init()
	{
		InjectHook(0x81A7A3, wtf_get_data_from_server);
		InjectHook(0x81A590, save_data_to_server, HookType::Jump);
	}

}