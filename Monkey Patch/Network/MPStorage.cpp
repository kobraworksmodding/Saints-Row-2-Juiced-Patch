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
	constexpr uint16_t GAME_MP_DATA_VERSION = 12;
	constexpr size_t MP_STORAGE_DATA_SIZE = 0xEE8C;
	constexpr int MP_STORAGE_STATUS_BLANK = 1;
	constexpr int MP_STORAGE_STATUS_VALID = 2;
	constexpr int MP_ASYNC_RESULT_OK = 0;
	constexpr int MP_ASYNC_RESULT_FAILED = 2;
	constexpr int MP_CALLBACK_STATUS_OK = 0;
	constexpr int MP_CALLBACK_STATUS_BLANK = 1;

	struct sr2_mp_data
	{
		uint16_t version;
		std::byte raw[MP_STORAGE_DATA_SIZE - sizeof(version)];
	};
	static_assert(sizeof(sr2_mp_data) == MP_STORAGE_DATA_SIZE, "Wrong size for sr2_mp_data");

	struct MPDT
	{
		uint32_t signature;
		uint32_t version;
		uint32_t offset_for_where_data_starts; // this points to sr2_mp_data in the file, maybe we can add like index multiple chars and stuff so a pointer to this is useful
		uint32_t sr2_mp_data_size;
		sr2_mp_data data;
	};

	sr2_mp_data* mp_data = (sr2_mp_data*)DynAddress(0x02A18A08);
	int* mp_storage_status = (int*)DynAddress(0x02528C24);
	bool* mp_storage_async_pending = (bool*)DynAddress(0x02528C2C);
	int* mp_async_callback_status = (int*)DynAddress(0x0212F46C);
	int* mp_async_result = (int*)DynAddress(0x0212F470);
	const char* where_to_save = (const char*)0x0144E650;

	void FinishAsyncRequest(int result, int callback_status)
	{
		*mp_async_result = result;
		*mp_async_callback_status = callback_status;
		*mp_storage_async_pending = false;
	}

	void MarkBlankRead()
	{
		*mp_storage_status = MP_STORAGE_STATUS_BLANK;
		FinishAsyncRequest(MP_ASYNC_RESULT_OK, MP_CALLBACK_STATUS_BLANK);
	}

	void MarkSuccessfulRead()
	{
		mp_data->version = GAME_MP_DATA_VERSION;
		*mp_storage_status = MP_STORAGE_STATUS_VALID;
		FinishAsyncRequest(MP_ASYNC_RESULT_OK, MP_CALLBACK_STATUS_OK);
	}

	void MarkSuccessfulWrite()
	{
		mp_data->version = GAME_MP_DATA_VERSION;
		*mp_storage_status = MP_STORAGE_STATUS_VALID;
		FinishAsyncRequest(MP_ASYNC_RESULT_OK, MP_CALLBACK_STATUS_OK);
	}

	void MarkWriteFailure()
	{
		FinishAsyncRequest(MP_ASYNC_RESULT_FAILED, MP_CALLBACK_STATUS_OK);
	}

	constexpr uint32_t MPDT_SIGNATURE = 'TDPM'; // "MPDT" little-endian
	constexpr uint32_t MPDT_VERSION = 1;

	static std::filesystem::path GetMPStoragePath()
	{
		std::filesystem::path base(where_to_save);

		// C:\Users\...\Saints Row 2\<FOLDER_NAME>\mp
		return base / FOLDER_NAME / "mp" / "MPStorage.mpdt_pc";
	}

	int __cdecl get_data_from_server(...)
	{
		auto path = GetMPStoragePath();
		std::error_code ec;
		if (!std::filesystem::exists(path, ec) || ec)
		{
			MarkBlankRead();
			Logger::TypedLog(CHN_NET, "MPStorage: no local storage found at {}, using blank storage.\n", path.string());
			return 1;
		}

		std::ifstream file(path, std::ios::binary);
		if (!file) {
			Logger::TypedLog(CHN_NET, "MPStorage: failed to open {} for reading.\n", path.string());
			return MP_ASYNC_RESULT_FAILED;
		}

		MPDT header{};
		file.read(reinterpret_cast<char*>(&header), sizeof(MPDT));

		if (!file
			|| header.signature != MPDT_SIGNATURE
			|| header.version != MPDT_VERSION
			|| header.offset_for_where_data_starts != offsetof(MPDT, data)
			|| header.sr2_mp_data_size != sizeof(sr2_mp_data))
		{
			MarkBlankRead();
			Logger::TypedLog(CHN_NET, "MPStorage: {} was invalid, using blank storage.\n", path.string());
			return 1;
		}

		std::memcpy(mp_data, &header.data, sizeof(sr2_mp_data));
		MarkSuccessfulRead();
		Logger::TypedLog(CHN_NET, "MPStorage: loaded local player data from {}.\n", path.string());

		return 1;
	}

	bool save_data_to_server(char /*console_command*/)
	{
		auto path = GetMPStoragePath();

		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);
		if (ec) {
			MarkWriteFailure();
			Logger::TypedLog(CHN_NET, "MPStorage: failed to create directory for {}.\n", path.string());
			return false;
		}

		MPDT out{};
		out.signature = MPDT_SIGNATURE;
		out.version = MPDT_VERSION;
		out.offset_for_where_data_starts = offsetof(MPDT, data);
		out.sr2_mp_data_size = sizeof(sr2_mp_data);

		mp_data->version = GAME_MP_DATA_VERSION;
		std::memcpy(&out.data, mp_data, sizeof(sr2_mp_data));

		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		if (!file) {
			MarkWriteFailure();
			Logger::TypedLog(CHN_NET, "MPStorage: failed to open {} for writing.\n", path.string());
			return false;
		}

		file.write(reinterpret_cast<const char*>(&out), sizeof(MPDT));
		if (!file.good()) {
			MarkWriteFailure();
			Logger::TypedLog(CHN_NET, "MPStorage: failed to write {}.\n", path.string());
			return false;
		}

		MarkSuccessfulWrite();
		Logger::TypedLog(CHN_NET, "MPStorage: saved local player data to {}.\n", path.string());
		return true;
	}

	void Init()
	{
		InjectHook(0x81A7A3, get_data_from_server);
		InjectHook(0x81A590, save_data_to_server, HookType::Jump);
	}

}
