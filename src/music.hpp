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

                Config(int frecuencia_ = 44100, int channels_ = 2, int chunkSize_ = 2048, int sfxChannels_ = 16)
                    : frecuencia(frecuencia_), channels(channels_), chunkSize(chunkSize_), sfxChannels(sfxChannels_) {}
            };


            MusicSystem() = default;
            ~MusicSystem();

            MusicSystem(const MusicSystem&) = delete;
            MusicSystem& operator=(const MusicSystem&) = delete;
            MusicSystem(MusicSystem&&) = delete;
            MusicSystem& operator=(MusicSystem&&) = delete;

            bool init(const Config& cfg = Config());
            void shutdown();

            bool loadMusic(const std::string& path);
            bool playMusic(int loops = -1);
            void pauseMusic();
            void resumeMusic();
            void stopMusic();

            bool isMusicPlaying() const;
            bool isMusicPaused() const;
            

            bool preloadSFX(const std::string& path);
            int playSFX(const std::string& path, int loops = 0, int channel = -1);
            void stopAllSfx();

            void setMusicVolume(int v);
            void setSFXVolume(int v);

            int musicVolume() const { return m_musicVolume;}
            int SFXVolume() const {return m_SFXVolume;}

            void clearSFXCache();

            const std:: string& lastError() const {return m_lastError;}

        private:
            void setError(const std::string& msg);

            bool m_initialized = false;

            Mix_Music* m_music = nullptr;
            std::string m_musicPath;

            std::unordered_map<std::string, Mix_Chunk*> m_sfxCache;

            int m_musicVolume = 20;
            int m_SFXVolume = 20;

            std::string m_lastError;
    };
}
