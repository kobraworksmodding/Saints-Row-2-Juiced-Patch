#pragma once

// returns local string from ID, if not found will just return pointer to default_fallback
extern const wchar_t* display_request_string(const char* id, const wchar_t* default_fallback = nullptr);

namespace MStrings
{
	extern void Init();
}