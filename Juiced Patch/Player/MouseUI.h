// MouseUI.h (Clippy95)
// --------------------
// Created: 11/10/2026

#pragma once
#include <string>

namespace MouseUI
{
	void Init();

	// true if this vint script should get mouse_ui_attach() appended
	bool WantsScript(const char* filename);

	// appends mouse_ui_attach("<script>"), called from General::VINT_DOC_luaLoadBuff
	bool PatchVintScript(const char* filename, std::string& content);

	bool IsActive();
}
