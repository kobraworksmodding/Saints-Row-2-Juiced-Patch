module;

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <functional>
#include <cstdint>
#include <utility>

#include "../GameConfig.h"

export module OptionsManager;

export struct Option {
    enum class Type { None, Bool, Int16, Int32, Int64, Float, Double };

    std::string appName;
    std::string keyName;
    Type        type = Type::None;
    void*       valuePtr = nullptr;

    // Optional callbacks. readCallback overrides direct read; writeCallback runs
    // *after* the backing variable (if any) has been updated, acting as a "post" hook.
    std::function<double()>      readCallback;
    std::function<void(double)>  writeCallback;

    Option(std::string app, std::string key, Type t, void* p)
        : appName(std::move(app)), keyName(std::move(key)), type(t), valuePtr(p) {}

    bool isFloat() const { return type == Type::Float || type == Type::Double; }

    double getValue() const {
        if (readCallback) return readCallback();
        if (!valuePtr)    return 0.0;
        switch (type) {
            case Type::Bool:   return *static_cast<bool*>(valuePtr) ? 1.0 : 0.0;
            case Type::Int16:  return static_cast<double>(*static_cast<int16_t*>(valuePtr));
            case Type::Int32:  return static_cast<double>(*static_cast<int32_t*>(valuePtr));
            case Type::Int64:  return static_cast<double>(*static_cast<int64_t*>(valuePtr));
            case Type::Float:  return static_cast<double>(*static_cast<float*>(valuePtr));
            case Type::Double: return *static_cast<double*>(valuePtr);
            default:           return 0.0;
        }
    }

    void setValue(double v) const {
        if (valuePtr) {
            switch (type) {
                case Type::Bool:   *static_cast<bool*>(valuePtr)    = (v != 0.0); break;
                case Type::Int16:  *static_cast<int16_t*>(valuePtr) = static_cast<int16_t>(v); break;
                case Type::Int32:  *static_cast<int32_t*>(valuePtr) = static_cast<int32_t>(v); break;
                case Type::Int64:  *static_cast<int64_t*>(valuePtr) = static_cast<int64_t>(v); break;
                case Type::Float:  *static_cast<float*>(valuePtr)   = static_cast<float>(v); break;
                case Type::Double: *static_cast<double*>(valuePtr)  = v; break;
                default: break;
            }
        }
        if (writeCallback) writeCallback(v);
    }

    void syncToIni() const {
        if (isFloat()) {
            GameConfig::SetDoubleValue(appName.c_str(), keyName.c_str(), getValue());
        } else {
            GameConfig::SetValue(appName.c_str(), keyName.c_str(), static_cast<uint32_t>(getValue()));
        }
    }

    void loadFromIni(double defaultValue) {
        if (isFloat()) {
            double v = GameConfig::GetDoubleValue(appName.c_str(), keyName.c_str(), defaultValue);
            setValue(v);
        } else {
            uint32_t v = GameConfig::GetValue(appName.c_str(), keyName.c_str(), static_cast<uint32_t>(defaultValue));
            setValue(static_cast<double>(v));
        }
    }
};

export class OptionsManager {
private:
    static std::unordered_map<std::string, std::unordered_map<std::string, Option*>>& getOptionsMap() {
        static std::unordered_map<std::string, std::unordered_map<std::string, Option*>> optionsMap;
        return optionsMap;
    }
    OptionsManager() {}

    static Option* registerImpl(const std::string& app, const std::string& key, Option::Type t, void* p) {
        auto& optionsMap = getOptionsMap();
        Option*& slot = optionsMap[app][key];
        if (slot) {
            slot->type     = t;
            slot->valuePtr = p;
        } else {
            slot = new Option(app, key, t, p);
        }
        return slot;
    }

public:
    static void cleanup() {
        auto& optionsMap = getOptionsMap();
        for (auto& appPair : optionsMap)
            for (auto& keyPair : appPair.second)
                delete keyPair.second;
        optionsMap.clear();
    }

    // ---- Typed registration ----
    static void registerOption(const std::string& app, const std::string& key, int* var, int def = 0) {
        Option* opt = registerImpl(app, key, Option::Type::Int32, var);
        opt->loadFromIni(static_cast<double>(def));
    }
    static void registerOption(const std::string& app, const std::string& key, bool* var, bool def = false) {
        Option* opt = registerImpl(app, key, Option::Type::Bool, var);
        opt->loadFromIni(def ? 1.0 : 0.0);
    }
    static void registerOption(const std::string& app, const std::string& key, int16_t* var, int16_t def = 0) {
        Option* opt = registerImpl(app, key, Option::Type::Int16, var);
        opt->loadFromIni(static_cast<double>(def));
    }
    static void registerOption(const std::string& app, const std::string& key, int64_t* var, int64_t def = 0) {
        Option* opt = registerImpl(app, key, Option::Type::Int64, var);
        opt->loadFromIni(static_cast<double>(def));
    }
    static void registerOption(const std::string& app, const std::string& key, float* var, float def = 0.0f) {
        Option* opt = registerImpl(app, key, Option::Type::Float, var);
        opt->loadFromIni(static_cast<double>(def));
    }
    static void registerOption(const std::string& app, const std::string& key, double* var, double def = 0.0) {
        Option* opt = registerImpl(app, key, Option::Type::Double, var);
        opt->loadFromIni(def);
    }

    // Var-less registration: no backing variable. Use this when you want to handle
    // get/set entirely via callbacks (or just route everything through GLuaWrapper(F)
    // and read/write the ini yourself). Type still matters for ini representation
    // (int vs double) and for how the lua menu treats the value.
    // Optionally pass callbacks here, or attach them later via setCallbacks().
    static void registerOption(const std::string& app, const std::string& key,
                               Option::Type type, double def = 0.0,
                               std::function<double()> read = nullptr,
                               std::function<void(double)> write = nullptr) {
        Option* opt = registerImpl(app, key, type, nullptr);
        if (read)  opt->readCallback  = std::move(read);
        if (write) opt->writeCallback = std::move(write);
        // Seed: if a write callback exists, push the ini-stored value into it.
        // Otherwise nothing to seed (no var, no setter) — value is purely produced
        // by readCallback at query time.
    }

    // Backwards-compat alias for the var-less form.
    static void registerCallbackOption(const std::string& app, const std::string& key,
                                       std::function<double()> read,
                                       std::function<void(double)> write,
                                       Option::Type type = Option::Type::Int32,
                                       double def = 0.0) {
        registerOption(app, key, type, def, std::move(read), std::move(write));
    }

    // Attach get/post callbacks to an already-registered option.
    static void setCallbacks(const std::string& app, const std::string& key,
                             std::function<double()> read,
                             std::function<void(double)> write) {
        Option* opt = getOption(app, key);
        if (!opt) return;
        if (read)  opt->readCallback  = std::move(read);
        if (write) opt->writeCallback = std::move(write);
    }
    static void setCallbacks(const std::string& key,
                             std::function<double()> read,
                             std::function<void(double)> write) {
        Option* opt = getOption(key);
        if (!opt) return;
        if (read)  opt->readCallback  = std::move(read);
        if (write) opt->writeCallback = std::move(write);
    }

    // ---- Set / Get ----
    static bool setOptionValue(const std::string& app, const std::string& key, double value) {
        Option* opt = getOption(app, key);
        if (!opt) return false;
        opt->setValue(value);
        if (!opt->readCallback) {
            opt->syncToIni();
        }
        return true;
    }
    static bool setOptionValue(const std::string& key, double value) {
        Option* opt = getOption(key);
        if (!opt) return false;
        opt->setValue(value);
        if (!opt->readCallback) {
            opt->syncToIni();
        }
        return true;
    }

    static Option* getOption(const std::string& app, const std::string& key) {
        auto& m = getOptionsMap();
        auto a = m.find(app);
        if (a == m.end()) return nullptr;
        auto b = a->second.find(key);
        return b == a->second.end() ? nullptr : b->second;
    }
    static Option* getOption(const std::string& key) {
        for (auto& appPair : getOptionsMap()) {
            auto it = appPair.second.find(key);
            if (it != appPair.second.end()) return it->second;
        }
        return nullptr;
    }

    static int getOptionValue(const std::string& app, const std::string& key, int def = 0) {
        Option* opt = getOption(app, key);
        if (opt) return static_cast<int>(opt->getValue());
        return GameConfig::GetValue(app.c_str(), key.c_str(), def);
    }
    static int getOptionValue(const std::string& key, int def = 0) {
        Option* opt = getOption(key);
        return opt ? static_cast<int>(opt->getValue()) : def;
    }

    static double getOptionValueD(const std::string& app, const std::string& key, double def = 0.0) {
        Option* opt = getOption(app, key);
        if (opt) return opt->getValue();
        return GameConfig::GetDoubleValue(app.c_str(), key.c_str(), def);
    }
    static double getOptionValueD(const std::string& key, double def = 0.0) {
        Option* opt = getOption(key);
        return opt ? opt->getValue() : def;
    }

    // ---- Ini sync ----
    static bool refreshFromIni(const std::string& app, const std::string& key) {
        Option* opt = getOption(app, key);
        if (!opt) return false;
        if (opt->isFloat()) {
            opt->setValue(GameConfig::GetDoubleValue(app.c_str(), key.c_str(), opt->getValue()));
        } else {
            uint32_t v = GameConfig::GetValue(app.c_str(), key.c_str(), static_cast<uint32_t>(opt->getValue()));
            opt->setValue(static_cast<double>(v));
        }
        return true;
    }
    static void refreshAllFromIni() {
        for (auto& appPair : getOptionsMap())
            for (auto& keyPair : appPair.second)
                if (keyPair.second) refreshFromIni(keyPair.second->appName, keyPair.second->keyName);
    }
    static void saveAllToIni() {
        for (auto& appPair : getOptionsMap())
            for (auto& keyPair : appPair.second)
                if (keyPair.second) keyPair.second->syncToIni();
    }

    static std::vector<Option*> getAllOptionsForApp(const std::string& app) {
        std::vector<Option*> result;
        auto& m = getOptionsMap();
        auto it = m.find(app);
        if (it != m.end()) {
            result.reserve(it->second.size());
            for (auto& kp : it->second) result.push_back(kp.second);
        }
        return result;
    }
    static std::vector<Option*> getAllOptions() {
        std::vector<Option*> result;
        for (auto& appPair : getOptionsMap())
            for (auto& kp : appPair.second) result.push_back(kp.second);
        return result;
    }
};
