#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lunanx
{

struct StreamSettings
{
    std::string preset_id = "custom";
    std::string label     = "720p · 60 fps";
    int width             = 1280;
    int height            = 720;
    // 60 ou 30 images par seconde (voir Normalize).
    int fps               = 60;
    // Plafond du débit vidéo en kbit/s ; 0 = automatique (adaptatif jusqu'à 10 Mb/s).
    int bitrate_kbps      = 10000;
    // "Auto" ou un code de région Luna (ex. "eu-west-3").
    std::string region    = "Auto";
    bool audio_enabled    = true;
    int audio_volume      = 1200;
    int audio_buffer_ms   = 40;
    // Fixé à "Software" (voir Normalize) ; "Auto" et "NVDEC" restent gérés par le décodeur.
    std::string video_backend = "Software";
    bool debug_diagnostics = false;
    // "Switch" : A Switch = A Xbox (étiquette, par défaut) ; "Xbox" : A Switch = B Xbox (position).
    std::string controller_layout = "Switch";
    std::string image_quality_mode = "Adaptive";
    // Boutique Amazon utilisée pour la connexion directe (« fr », « de », « com »...) ; vide
    // tant qu'elle n'a pas été choisie au premier lancement.
    std::string amazon_market;
    // Bibliothèque : "Covers" (jaquettes, Top 10, Reprendre) ou "List" (liste simple).
    std::string library_layout = "Covers";
    // Langue de l'interface (code de UiLanguageOptions), anglais au premier lancement.
    std::string language = "en";
};

// Budget de débit envoyé à Luna et annoncé en REMB. Les trois valeurs sont en bit/s.
struct StreamBitrateBudget
{
    std::uint32_t minimum;
    std::uint32_t start;
    std::uint32_t maximum;
};

StreamBitrateBudget ResolveStreamBitrateBudget(int bitrate_kbps);
StreamSettings LoadStreamSettings();
bool SaveStreamSettings(const StreamSettings& settings);
std::string FormatStreamSettings(const StreamSettings& settings);

} // namespace lunanx
