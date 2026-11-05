#include "stream_settings.hpp"

#include "app_paths.hpp"
#include "atomic_file_replace.hpp"

#include <jansson.h>

#include <algorithm>
#include <memory>

namespace lunanx
{
namespace
{

using JsonPtr = std::unique_ptr<json_t, decltype(&json_decref)>;

std::string SettingsPath()
{
    return AppHomePath() + "/stream_settings.json";
}

std::string JsonField(json_t* object, const char* key, const std::string& fallback)
{
    json_t* value = json_object_get(object, key);
    return json_is_string(value) ? json_string_value(value) : fallback;
}

int JsonInt(json_t* object, const char* key, int fallback)
{
    json_t* value = json_object_get(object, key);
    return json_is_integer(value) ? static_cast<int>(json_integer_value(value)) : fallback;
}

bool JsonBool(json_t* object, const char* key, bool fallback)
{
    json_t* value = json_object_get(object, key);
    return json_is_boolean(value) ? json_is_true(value) : fallback;
}

bool OneOf(const std::string& value, std::initializer_list<const char*> allowed)
{
    return std::any_of(allowed.begin(), allowed.end(),
                       [&value](const char* item) { return value == item; });
}

bool OneOfInt(int value, std::initializer_list<int> allowed)
{
    return std::find(allowed.begin(), allowed.end(), value) != allowed.end();
}

StreamSettings Normalize(StreamSettings settings)
{
    // La résolution et le décodeur restent fixes (720p, logiciel) : c'est la combinaison
    // la plus fluide sur Switch. Le nombre d'images et le plafond de débit sont réglables
    // pour les connexions plus lentes.
    settings.preset_id = "custom";
    settings.video_backend = "Software";
    settings.image_quality_mode = "Adaptive";
    settings.width = 1280;
    settings.height = 720;
    if (!OneOfInt(settings.fps, {30, 60}))
        settings.fps = 60;
    if (settings.bitrate_kbps != 0 &&
        !OneOfInt(settings.bitrate_kbps, {3000, 4000, 6000, 8000, 10000, 15000}))
        settings.bitrate_kbps = 10000;
    settings.label = "720p · " + std::to_string(settings.fps) + " fps";
    settings.audio_volume = std::clamp(settings.audio_volume, 100, 1600);
    settings.audio_buffer_ms = std::clamp(settings.audio_buffer_ms, 30, 100);
    if (!OneOf(settings.controller_layout, {"Xbox", "Switch"}))
        settings.controller_layout = "Switch";
    if (!OneOf(settings.library_layout, {"Covers", "List"}))
        settings.library_layout = "Covers";
    if (!OneOf(settings.language, {"de", "en", "es", "fr", "it", "nl", "pl", "pt", "ru", "sv", "zh"}))
        settings.language = "en";
    if (!OneOf(settings.amazon_market, {"fr", "de", "co.uk", "es", "it", "pl", "nl", "se", "com.be", "at", "pt", "lu", "com", "ca"}))
        settings.amazon_market.clear();
    if (settings.region.empty())
        settings.region = "Auto";
    return settings;
}

} // namespace

StreamBitrateBudget ResolveStreamBitrateBudget(int bitrate_kbps)
{
    // Automatique : Luna peut monter jusqu'à 10 Mb/s, mais démarre prudemment à 6 Mb/s.
    if (bitrate_kbps <= 0)
        return {2'000'000u, 6'000'000u, 10'000'000u};

    const auto cap = static_cast<std::uint32_t>(bitrate_kbps) * 1000u;
    const std::uint32_t floor = 1'000'000u;
    const std::uint32_t ceiling = 6'000'000u;
    const std::uint32_t minimum = std::min(cap, std::clamp(cap / 2, floor, ceiling));
    return {minimum, cap, cap};
}

StreamSettings LoadStreamSettings()
{
    StreamSettings settings;
    json_error_t error {};
    JsonPtr root(json_load_file(SettingsPath().c_str(), 0, &error), &json_decref);
    if (!root || !json_is_object(root.get()))
        return Normalize(settings);

    json_t* r = root.get();
    settings.preset_id = JsonField(r, "preset", settings.preset_id);
    settings.fps = JsonInt(r, "fps", settings.fps);
    settings.bitrate_kbps = JsonInt(r, "bitrate_kbps", settings.bitrate_kbps);
    settings.region = JsonField(r, "region", settings.region);
    settings.audio_enabled = JsonBool(r, "audio_enabled", settings.audio_enabled);
    settings.audio_volume = JsonInt(r, "audio_volume", settings.audio_volume);
    settings.audio_buffer_ms = JsonInt(r, "audio_buffer_ms", settings.audio_buffer_ms);
    settings.video_backend = JsonField(r, "video_backend", settings.video_backend);
    settings.debug_diagnostics = JsonBool(r, "debug_diagnostics", settings.debug_diagnostics);
    settings.controller_layout = JsonField(r, "controller_layout", settings.controller_layout);
    settings.image_quality_mode = JsonField(r, "image_quality_mode", settings.image_quality_mode);
    settings.library_layout = JsonField(r, "library_layout", settings.library_layout);
    settings.amazon_market = JsonField(r, "amazon_market", settings.amazon_market);
    settings.language = JsonField(r, "language", settings.language);
    return Normalize(settings);
}

bool SaveStreamSettings(const StreamSettings& settings)
{
    PrepareAppStorage();
    const StreamSettings clean = Normalize(settings);
    JsonPtr root(json_object(), &json_decref);
    json_t* r = root.get();
    json_object_set_new(r, "preset", json_string(clean.preset_id.c_str()));
    json_object_set_new(r, "fps", json_integer(clean.fps));
    json_object_set_new(r, "bitrate_kbps", json_integer(clean.bitrate_kbps));
    json_object_set_new(r, "region", json_string(clean.region.c_str()));
    json_object_set_new(r, "audio_enabled", json_boolean(clean.audio_enabled));
    json_object_set_new(r, "audio_volume", json_integer(clean.audio_volume));
    json_object_set_new(r, "audio_buffer_ms", json_integer(clean.audio_buffer_ms));
    json_object_set_new(r, "video_backend", json_string(clean.video_backend.c_str()));
    json_object_set_new(r, "debug_diagnostics", json_boolean(clean.debug_diagnostics));
    json_object_set_new(r, "controller_layout", json_string(clean.controller_layout.c_str()));
    json_object_set_new(r, "image_quality_mode", json_string(clean.image_quality_mode.c_str()));
    json_object_set_new(r, "library_layout", json_string(clean.library_layout.c_str()));
    json_object_set_new(r, "language", json_string(clean.language.c_str()));
    json_object_set_new(r, "amazon_market", json_string(clean.amazon_market.c_str()));

    char* text = json_dumps(r, JSON_INDENT(2));
    if (!text)
        return false;
    const bool ok = WriteFileAtomically(SettingsPath(), text);
    free(text);
    return ok;
}

std::string FormatStreamSettings(const StreamSettings& settings)
{
    const std::string bitrate = settings.bitrate_kbps > 0
        ? std::to_string(settings.bitrate_kbps / 1000) + " Mb/s"
        : std::string("auto");
    return settings.label + " · " + bitrate + " · région " + settings.region;
}

} // namespace lunanx
