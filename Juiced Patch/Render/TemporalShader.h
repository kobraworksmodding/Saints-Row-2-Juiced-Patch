#pragma once
#include <cstdint>
#include <span>
#include <vector>

// Transform the effective (BlingGFX/Juiced/vanilla) shader, preserving its inputs,
// scale, skin weights and morph calculations. Unsupported programs fail closed.
namespace TemporalAA::Shader
{
    using Token = uint32_t;
    constexpr unsigned Type(Token t) { return ((t >> 28) & 7) | ((t >> 8) & 24); }
    constexpr unsigned Reg(Token t) { return t & 2047; }
    constexpr Token Register(unsigned type, unsigned reg)
    {
        return 0x80000000u | ((type & 7) << 28) | ((type & 24) << 8) | reg;
    }
    constexpr Token Dest(unsigned type, unsigned reg, unsigned mask = 15)
    {
        return Register(type, reg) | (mask << 16);
    }
    constexpr Token Source(unsigned type, unsigned reg, unsigned swizzle = 0xE4)
    {
        return Register(type, reg) | (swizzle << 16);
    }
    inline void Emit(std::vector<Token>& out, unsigned op, std::initializer_list<Token> args)
    {
        out.push_back(op | (static_cast<Token>(args.size()) << 24));
        out.insert(out.end(), args);
    }
    struct Program
    {
        std::vector<Token> tokens;
        bool skinned = false;
    };
    // Native c244-c247 are SR2's world matrix; keep them intact. The replay
    // reserves c248-c251 for previous MVP, c252-c254 for bone/jitter parameters.
    inline Program MakeVelocity(std::span<const Token> code)
    {
        Program result;
        if (code.empty() || code[0] != 0xFFFE0300u) return result;
        std::vector<std::vector<Token>> body;
        std::vector<Token> declarations, definitions;
        unsigned position = ~0u, mvpMask = 0;
        bool ended = false;
        for (size_t p = 1; p < code.size();)
        {
            const unsigned op = code[p] & 0xFFFF;
            if (op == 0xFFFF) { ended = true; break; }
            const size_t length = op == 0xFFFE ? 1 + ((code[p] >> 16) & 0x7FFF) : 1 + ((code[p] >> 24) & 15);
            if (length > code.size() - p) return {};
            if (op == 0xFFFE) { p += length; continue; }
            if (code[p] & 0xF0000000u) return {}; // Predication/coissue.
            if (op == 31)
            {
                if (length != 3) return {};
                const auto t = code[p + 2];
                if (Type(t) == 6)
                {
                    if ((code[p + 1] & 15) == 0) position = Reg(t);
                }
                else if (Type(t) == 1)
                    declarations.insert(declarations.end(), code.begin() + p, code.begin() + p + length);
                else return {}; // Existing vertex texture shaders.
            }
            else if (op == 81)
            {
                if (length != 6 || Type(code[p + 1]) != 2 || Reg(code[p + 1]) >= 248) return {};
                definitions.insert(definitions.end(), code.begin() + p, code.begin() + p + length);
            }
            else
            {
                // Straight-line arithmetic; reject control flow and matrix macros.
                if (!(op <= 19 || op == 35 || op == 36 || op == 46 || op == 88 || op == 90)) return {};
                if (op != 0 && length < 3) return {};
                unsigned indirectSources = 0;
                for (size_t j = 1; j < length; ++j)
                {
                    const auto t = code[p + j];
                    const auto type = Type(t), reg = Reg(t);
                    if (type == 0 && reg >= 24) return {};
                    if (type == 2)
                    {
                        if (reg >= 248) return {};
                        if (reg >= 4 && reg <= 7) mvpMask |= 1u << (reg - 4);
                        if (t & 0x2000)
                        {
                            if (++indirectSources > 1 || j == 1 || reg < 52 || reg > 54 || j + 1 >= length || Type(code[p + j + 1]) != 3 || Reg(code[p + j + 1]) != 0) return {};
                            result.skinned = true;
                            ++j;
                        }
                    }
                    else if (t & 0x2000) return {};
                    if (j > 1 && type == 6) return {};
                }
                body.emplace_back(code.begin() + p, code.begin() + p + length);
            }
            p += length;
        }
        if (!ended || position == ~0u || mvpMask != 15) return {};
        auto& out = result.tokens;
        out.push_back(code[0]);
        out.insert(out.end(), declarations.begin(), declarations.end());
        Emit(out, 31, {0x80000000u, Dest(6, 0)});
        Emit(out, 31, {0x80000005u, Dest(6, 1)});
        Emit(out, 31, {0x80010005u, Dest(6, 2)});
        if (result.skinned) Emit(out, 31, {0x90000000u, Dest(10, 0)});
        out.insert(out.end(), definitions.begin(), definitions.end());
        for (unsigned pass = 0; pass != 2; ++pass)
        {
            for (const auto& original : body)
            {
                const auto op = original[0] & 0xFFFF;
                if (op == 0) { out.push_back(original[0]); continue; }
                auto instruction = original;
                auto& dst = instruction[1];
                if (Type(dst) == 6)
                    dst = (dst & ~(0x70001800u | 2047u)) | Register(0, Reg(dst) == position ? 28 + pass : 26);
                if (pass && op == 46)
                    Emit(out, 1, {Dest(0, 25, (original[1] >> 16) & 15), original[2]});
                for (size_t j = 2; j < instruction.size(); ++j)
                {
                    auto& t = instruction[j];
                    if (Type(t) != 2) continue;
                    const auto reg = Reg(t);
                    if (t & 0x2000)
                    {
                        if (!pass) { ++j; continue; }
                        const unsigned component = (instruction[j + 1] >> 16) & 3;
                        // c52 + rounded bone index * 3. One texel per float4.
                        Emit(out, 5, {Dest(0, 24, 1), Source(0, 25, component * 0x55), Source(2, 252, 0)});
                        Emit(out, 2, {Dest(0, 24, 1), Source(0, 24, 0), Source(2, 253, (reg - 52) * 0x55)});
                        Emit(out, 1, {Dest(0, 24, 2), Source(2, 252, 0x55)});
                        Emit(out, 1, {Dest(0, 24, 12), Source(2, 252, 0xAA)});
                        Emit(out, 95, {Dest(0, 24), Source(0, 24), Source(10, 0)});
                        t = (t & ~(0x70001800u | 2047u | 0x2000u)) | Register(0, 24);
                        instruction.erase(instruction.begin() + j + 1);
                    }
                    else if (pass && reg >= 4 && reg <= 7)
                        t = (t & ~2047u) | (248 + reg - 4);
                }
                instruction[0] = (instruction[0] & ~0x0F000000u) | (static_cast<Token>(instruction.size() - 1) << 24);
                out.insert(out.end(), instruction.begin(), instruction.end());
            }
        }
        Emit(out, 1, {Dest(6, 0), Source(0, 28)});
        Emit(out, 4, {Dest(6, 1, 3), Source(0, 28, 0xFF), Source(2, 254), Source(0, 28)});
        Emit(out, 1, {Dest(6, 1, 12), Source(0, 28)});
        Emit(out, 1, {Dest(6, 2), Source(0, 29)});
        out.push_back(0x0000FFFFu);
        return result;
    }
    // Rotate only the Bayer coverage calculation in the effective PS. Later
    // VPOS reads still address the native light/reflection buffers exactly.
    // c190 and r31 are reserved only when this recognizer proves them unused.
    inline std::vector<Token> MakeTemporalDither(std::span<const Token> code)
    {
        if (code.empty() || code[0] != 0xFFFF0300u) return {};
        size_t firstPosition = 0, kill = 0, end = 0;
        unsigned frc = 0, positionReads = 0, fractions = 0;
        for (size_t p = 1; p < code.size();)
        {
            const unsigned op = code[p] & 0xFFFF;
            if (op == 0xFFFF) { end = p; break; }
            const size_t n = op == 0xFFFE ? 1 + ((code[p] >> 16) & 0x7FFF) : 1 + ((code[p] >> 24) & 15);
            if (n > code.size() - p || (op != 0xFFFE && (code[p] & 0xF0000000u))) return {};
            if (op == 81)
            {
                if (n != 6 || Type(code[p+1]) != 2 || Reg(code[p+1]) == 190) return {};
                for (size_t j = 2; j < n; ++j)
                {
                    if (code[p+j] == 0x3E800000u) fractions |= 1; // .25
                    if (code[p+j] == 0x3F000000u) fractions |= 2; // .50
                    if (code[p+j] == 0x3F400000u) fractions |= 4; // .75
                }
            }
            else if (op != 0xFFFE)
            {
                // DEFI/DEFB, branching and relative addressing are not part of
                // the supported straight-line Bayer shaders.
                if (op == 47 || op == 48 || (op >= 25 && op <= 30) ||
                    (op >= 38 && op <= 45) || (op >= 96)) return {};
                for (size_t j = op == 31 ? 2 : 1; j < n; ++j)
                {
                    const Token t = code[p+j];
                    if ((t & 0x2000) || (Type(t) == 0 && Reg(t) == 31) ||
                        (Type(t) == 2 && Reg(t) == 190)) return {};
                    if (!kill && op != 31 && j >= 2 && Type(t) == 17 && Reg(t) == 0)
                    {
                        if (!firstPosition) firstPosition = p;
                        ++positionReads;
                    }
                }
                if (!kill && op == 19) ++frc;
                if (!kill && op == 65) kill = p;
            }
            p += n;
        }
        if (!end || !kill || !firstPosition || firstPosition >= kill ||
            frc < 3 || positionReads < 3 || fractions != 7) return {};
        std::vector<Token> result(code.begin(), code.begin()+firstPosition);
        Emit(result, 2, {Dest(0,31,3), Source(17,0), Source(2,190)});
        for (size_t p = firstPosition; p <= end;)
        {
            const unsigned op = code[p] & 0xFFFF;
            const size_t n = op == 0xFFFF ? 1 : op == 0xFFFE ? 1 + ((code[p] >> 16) & 0x7FFF) : 1 + ((code[p] >> 24) & 15);
            const size_t start = result.size();
            result.insert(result.end(), code.begin()+p, code.begin()+p+n);
            if (p < kill && op != 31 && op != 81 && op != 0xFFFE)
                for (size_t j = 2; j < n; ++j)
                    if (Type(result[start+j]) == 17 && Reg(result[start+j]) == 0)
                        result[start+j] = (result[start+j] & ~(0x70001800u | 2047u)) | Register(0,31);
            p += n;
        }
        return result;
    }

}
