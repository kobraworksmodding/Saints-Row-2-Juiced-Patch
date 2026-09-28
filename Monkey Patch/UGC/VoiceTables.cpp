#include "../stdafx.h"
#include "VoiceTables.h"
#include "../Game/Game.h"
#include "../Hooker.h"
#include "../FileLogger.h"
#include "../loose files.h"
#include <map>
#include <set>
#include <string>

namespace VoiceTables {
namespace {
struct Variant { uint32_t handle; uint8_t extra[8]; };
struct Voice { uint32_t name_hash; Variant* variants; uint32_t flags; const wchar_t* subtitle; };
static_assert(sizeof(Voice) == 16 && sizeof(Variant) == 12);
SafetyHookMid AppendVoices;

void __cdecl Load() {
    auto& count = *reinterpret_cast<uint16_t*>(0x2526AF8_g);
    auto* voices = reinterpret_cast<Voice*>(0x2574360_g);
    std::set<uint32_t> names;
    for (uint32_t i = 0; i < count && i < Capacity; ++i) names.insert(voices[i].name_hash);
    // Match the loose loader's precedence; sort filenames for deterministic IDs.
    std::map<std::string, std::string> files;
    constexpr std::string_view suffix = ".voices.xtbl";
    for (auto* cache : {&DirCache, &DLCCache}) {
        for (const auto& [name, data] : *cache) {
            if (name.size() >= suffix.size() && Equal(std::string_view(name).substr(name.size()-suffix.size()), suffix))
                files.try_emplace(name, data.FilePath);
        }
    }
    for (const auto& [filename, path] : files) {
        auto* table = Game::xml::parse_table_node(filename.c_str(), nullptr);
        if (!table) {
            if (*reinterpret_cast<int*>(0xE87134_g) != -1) Game::xml::xtbl_free();
            Logger::TypedLog(CHN_AUDIO,"Voice table: could not parse {}\n",path); continue;
        }
        uint32_t added = 0;
        for (auto* row = table->elements; row; row = row->next) {
            if (!row->name || !Equal(row->name, "Voice")) continue;
            const char* name = Text(row,"Name");
            if (auto* error = Validate(row,count)) {
                Logger::TypedLog(CHN_AUDIO,"Voice table {}: rejected {}: {}\n",path,name ? name : "<unnamed>",error); continue;
            }
            const uint32_t hash = Game::utils::str_to_hash(name);
            if (names.contains(hash)) {
                Logger::TypedLog(CHN_AUDIO,"Voice table {}: duplicate name/hash {} skipped\n",path,name); continue;
            }
            voices[count] = {};
            if (!cdecl_call<bool>(0x47A770_g,row)) continue;
            const auto& voice = voices[count]; const uint32_t n = (voice.flags >> 12) & 0x7f;
            bool valid = voice.variants && n;
            for (uint32_t i = 0; valid && i < n; ++i) valid = voice.variants[i].handle != UINT32_MAX;
            if (!valid) {
                Logger::TypedLog(CHN_AUDIO,"Voice table {}: {} references an unavailable bank/cue; skipped\n",path,name);
                continue;
            }
            ++count; ++added; names.insert(hash);
        }
        Game::xml::xtbl_free();
        loaded_files_push_filename(path.c_str());
        Logger::TypedLog(CHN_AUDIO,"Voice table {}: added {} entries (total {})\n",path,added,count);
    }
}
}
void Init() {
    AppendVoices = safetyhook::create_mid(0x465159_g, [](SafetyHookContext&) { Load(); });
}
}
