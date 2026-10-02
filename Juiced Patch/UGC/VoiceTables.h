#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <string_view>

namespace VoiceTables {
inline constexpr uint32_t Capacity = 0x8000;
inline bool Equal(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        auto lower = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
        if (lower(a[i]) != lower(b[i])) return false;
    }
    return true;
}
template<class Node> Node* Find(Node* parent, const char* name) {
    for (auto* n = parent ? parent->elements : nullptr; n; n = n->next)
        if (n->name && Equal(n->name, name)) return n;
    return nullptr;
}
template<class Node> const char* Text(Node* parent, const char* name) {
    auto* n = Find(parent, name); return n ? n->text : nullptr;
}
inline bool ValidString(const char* s, size_t maximum) { return s && *s && std::strlen(s) <= maximum; }
inline bool Radius(const char* s, float& value) {
    if (!ValidString(s, 32)) return false;
    char* end = nullptr; value = std::strtof(s, &end);
    if (end == s || !std::isfinite(value) || value < 0) return false;
    while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    return !*end;
}
// Validate before the native parser reaches its unchecked filename copy,
// 7-bit variation count and fixed 32768-entry voice array.
template<class Node> const char* Validate(Node* voice, uint32_t count) {
    if (count >= Capacity) return "native voice table is full";
    if (!ValidString(Text(voice, "Name"), 63)) return "Name must contain 1..63 bytes";
    if (!ValidString(Text(voice, "AudioBanks"), 63)) return "AudioBanks must contain 1..63 bytes";
    if (Find(voice, "Focused") && !ValidString(Text(voice, "Focused"), 63))
        return "Focused must name a focus profile when present";
    auto* variants = Find(voice, "Variations"); uint32_t n = 0;
    for (auto* v = variants ? variants->elements : nullptr; v; v = v->next) {
        if (!v->name || !Equal(v->name, "Variation")) continue;
        ++n; auto* file = Text(Find(v, "Var"), "Filename");
        if (!ValidString(file, 63)) return "each Variation needs Var/Filename (1..63 bytes)";
        if (std::strpbrk(file, "/\\")) return "cue filenames must not contain paths";
    }
    if (!n || n > 127) return "Variations must contain 1..127 entries";
    auto* sphere = Find(voice, "SphereProperties");
    float minimum = 0, maximum = 0;
    if (sphere && (!Radius(Text(sphere,"MinRadius"),minimum) || !Radius(Text(sphere,"MaxRadius"),maximum) || maximum <= minimum))
        return "SphereProperties requires finite radii with 0 <= MinRadius < MaxRadius";
    return nullptr;
}
void Init();
}
