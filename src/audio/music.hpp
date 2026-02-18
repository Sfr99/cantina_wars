/*
 * audio/music.hpp
 * Sistema de audio basado en SDL2_mixer. Gestiona música de fondo y efectos
 * de sonido (SFX) con caché, control de volumen y canales independientes.
 */
#pragma once
#include <string>
#include <unordered_map>

struct Mix_Music;
struct Mix_Chunk;

namespace audio {

class MusicSystem {
public:
    struct Config {
        int frecuencia;
        int channels;
        int chunkSize;
        int sfxChannels;

        Config(int frecuencia_ = 44100, int channels_ = 2,
               int chunkSize_ = 2048,  int sfxChannels_ = 16)
            : frecuencia(frecuencia_), channels(channels_),
              chunkSize(chunkSize_),   sfxChannels(sfxChannels_) {}
    };

    MusicSystem() = default;
    ~MusicSystem();

    MusicSystem(const MusicSystem&)            = delete;
    MusicSystem& operator=(const MusicSystem&) = delete;
    MusicSystem(MusicSystem&&)                 = delete;
    MusicSystem& operator=(MusicSystem&&)      = delete;

    /* Inicializa el subsistema de audio con la configuración dada. */
    bool init(const Config& cfg = Config());

    /* Libera todos los recursos de audio y cierra el subsistema. */
    void shutdown();

    /* Carga un fichero de música; reutiliza el ya cargado si la ruta coincide. */
    bool loadMusic(const std::string& path);

    /* Reproduce la música cargada; loops=-1 para repetición infinita. */
    bool playMusic(int loops = -1);

    void pauseMusic();
    void resumeMusic();
    void stopMusic();

    bool isMusicPlaying() const;
    bool isMusicPaused()  const;

    /* Carga un SFX en caché sin reproducirlo todavía. */
    bool preloadSFX(const std::string& path);

    /* Reproduce un SFX (lo carga si aún no está en caché). Devuelve el canal usado o -1. */
    int  playSFX(const std::string& path, int loops = 0, int channel = -1);

    void stopAllSfx();

    void setMusicVolume(int v);
    void setSFXVolume(int v);

    int musicVolume() const { return m_musicVolume; }
    int SFXVolume()   const { return m_SFXVolume;   }

    /* Libera todos los chunks SFX del caché. */
    void clearSFXCache();

    const std::string& lastError() const { return m_lastError; }

private:
    void setError(const std::string& msg);

    bool        m_initialized = false;
    Mix_Music*  m_music       = nullptr;
    std::string m_musicPath;

    std::unordered_map<std::string, Mix_Chunk*> m_sfxCache;

    int m_musicVolume = 20;
    int m_SFXVolume   = 20;

    std::string m_lastError;
};

} // namespace audio