/*
 * core/RankingSystem.hpp
 * Gestiona el top 5 de puntuaciones con nombre (3 letras estilo recreativa).
 * Persiste en scores.txt junto al ejecutable. Formato por línea: "AAA 12345\n"
 */
#pragma once
#include <string>
#include <array>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

struct ScoreEntry {
    char name[4] = "AAA";  // 3 letras + null terminator
    int  score   = 0;
};

class RankingSystem {
public:
    static constexpr int MAX_ENTRIES = 5;

    RankingSystem() { load(); }

    /* Devuelve true si score entraría en el top 5. */
    bool isHighScore(int score) const {
        if ((int)m_entries.size() < MAX_ENTRIES) return score > 0;
        return score > m_entries.back().score;
    }

    /* Inserta un nuevo score, ordena desc y recorta a MAX_ENTRIES. */
    void insertScore(const char name[4], int score) {
        ScoreEntry e;
        e.name[0] = name[0]; e.name[1] = name[1];
        e.name[2] = name[2]; e.name[3] = '\0';
        e.score = score;
        m_entries.push_back(e);
        std::sort(m_entries.begin(), m_entries.end(),
                  [](const ScoreEntry& a, const ScoreEntry& b){ return a.score > b.score; });
        if ((int)m_entries.size() > MAX_ENTRIES)
            m_entries.resize(MAX_ENTRIES);
        save();
    }

    const std::vector<ScoreEntry>& entries() const { return m_entries; }

    /* Rango (1-based) que ocuparía este score; MAX_ENTRIES+1 si no entra. */
    int rankOf(int score) const {
        int rank = 1;
        for (const auto& e : m_entries)
            if (e.score > score) rank++;
        return rank;
    }

private:
    std::vector<ScoreEntry> m_entries;
    const char* SCORES_FILE = "../scores.txt";

    void load() {
        m_entries.clear();
        std::ifstream f(SCORES_FILE);
        if (!f.is_open()) return;
        std::string line;
        while (std::getline(f, line) && (int)m_entries.size() < MAX_ENTRIES) {
            std::istringstream ss(line);
            std::string name; int score;
            if (ss >> name >> score && name.size() == 3) {
                ScoreEntry e;
                e.name[0] = name[0]; e.name[1] = name[1];
                e.name[2] = name[2]; e.name[3] = '\0';
                e.score = score;
                m_entries.push_back(e);
            }
        }
    }

    void save() const {
        std::ofstream f(SCORES_FILE);
        for (const auto& e : m_entries)
            f << e.name << " " << e.score << "\n";
    }
};