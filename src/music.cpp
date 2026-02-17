#include "music.hpp"
#include <SDL2/SDL.h>
#include <sstream>
#include <SDL2/SDL_mixer.h>

namespace audio {

    MusicSystem::~MusicSystem() {
        shutdown();
    }

        
    bool MusicSystem::init(const Config& cfg) {
        if(m_initialized) return true;

        if((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0) {
            if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
                setError(std::string("SDL_InitSubSystem(AUDIO): ") + SDL_GetError());
                return false;
            }
        }

        if(Mix_OpenAudio(cfg.frecuencia, MIX_DEFAULT_FORMAT, cfg.channels, cfg.chunkSize) != 0) {
            setError(std::string("Mix_OpenAudio: ") + Mix_GetError());
            return false;
        }

        Mix_AllocateChannels(cfg.sfxChannels);

        Mix_VolumeMusic(m_musicVolume);
        Mix_Volume(-1, m_SFXVolume);

        m_initialized = true;
        m_lastError.clear();
        return true;
    }

    void MusicSystem::shutdown() {
        if(!m_initialized) return;

        stopMusic();

        if(m_music) {
            Mix_FreeMusic(m_music);
            m_music = nullptr;
        }

        m_musicPath.clear();

        clearSFXCache();

        Mix_CloseAudio();

        m_initialized = false;
    }

    bool MusicSystem::loadMusic(const std::string& path) {
        if (!m_initialized) {
            setError("MusicSystem not initialized");
            return false;
        }

        if (m_music && m_musicPath == path) return true;

        stopMusic();

        if (m_music) {
            Mix_FreeMusic(m_music);
            m_music = nullptr;
        }

        m_music = Mix_LoadMUS(path.c_str());
        if (!m_music) {
            setError(std::string("Mix_LoadMUS: ") + Mix_GetError());
            return false;
        }

        m_musicPath = path;
        m_lastError.clear();
        return true;
    }

    bool MusicSystem::playMusic(int loops) {
    if (!m_initialized) {
        setError("MusicSystem not initialized");
        return false;
    }
    if (!m_music) {
        setError("No music loaded");
        return false;
    }

    if (Mix_PlayMusic(m_music, loops) != 0) {
        setError(std::string("Mix_PlayMusic: ") + Mix_GetError());
        return false;
    }
    m_lastError.clear();
    return true;
}

void MusicSystem::pauseMusic() {
    if (m_initialized && Mix_PlayingMusic()) Mix_PauseMusic();
}

void MusicSystem::resumeMusic() {
    if (m_initialized && Mix_PausedMusic()) Mix_ResumeMusic();
}

void MusicSystem::stopMusic() {
    if (!m_initialized) return;
    if (Mix_PlayingMusic()) Mix_HaltMusic();
}

bool MusicSystem::isMusicPlaying() const {
    return m_initialized && Mix_PlayingMusic() != 0 && Mix_PausedMusic() == 0;
}

bool MusicSystem::isMusicPaused() const {
    return m_initialized && Mix_PausedMusic() != 0;
}

bool MusicSystem::preloadSFX(const std::string& path) {
    if (!m_initialized) {
        setError("MusicSystem not initialized");
        return false;
    }

    auto it = m_sfxCache.find(path);
    if (it != m_sfxCache.end() && it->second) return true;

    Mix_Chunk* c = Mix_LoadWAV(path.c_str());
    if (!c) {
        setError(std::string("Mix_LoadWAV: ") + Mix_GetError());
        return false;
    }

    m_sfxCache[path] = c;
    m_lastError.clear();
    return true;
}

int MusicSystem::playSFX(const std::string& path, int loops, int channel) {
    if (!m_initialized) {
        setError("MusicSystem not initialized");
        return -1;
    }

    if (!preloadSFX(path)) return -1;

    Mix_Chunk* c = m_sfxCache[path];
    // volumen global SFX ya aplicado con Mix_Volume(-1, ...)
    int ch = Mix_PlayChannel(channel, c, loops);
    if (ch == -1) {
        setError(std::string("Mix_PlayChannel: ") + Mix_GetError());
        return -1;
    }

    m_lastError.clear();
    return ch;
}

void MusicSystem::stopAllSfx() {
    if (!m_initialized) return;
    Mix_HaltChannel(-1);
}

void MusicSystem::setMusicVolume(int v) {
    if (v < 0) v = 0;
    if (v > 128) v = 128;
    m_musicVolume = v;
    if (m_initialized) Mix_VolumeMusic(m_musicVolume);
}

void MusicSystem::setSFXVolume(int v) {
    if (v < 0) v = 0;
    if (v > 128) v = 128;
    m_SFXVolume = v;
    if (m_initialized) Mix_Volume(-1, m_SFXVolume);
}

void MusicSystem::clearSFXCache() {
    for (auto& kv : m_sfxCache) {
        if (kv.second) Mix_FreeChunk(kv.second);
    }
    m_sfxCache.clear();
}

void MusicSystem::setError(const std::string& msg) {
    m_lastError = msg;
}

}
