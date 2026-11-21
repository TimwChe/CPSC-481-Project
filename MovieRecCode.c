#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

//==================== Data Strutures =================================
struct Movie {
    std::string IMDb_ID;
    std::string title;
    int releaseYear;
    int runtimeMinutes;
    std::vector<std::string> genres;
    std::vector<std::string> directors;
    std::vector<std::string> actors;
    
    // Crowd-appeal heuristic for group viewing (1.0 = niche, 5.0 = crowd-pleaser)
    double crowdAppealScore = 3.5; 
};
struct UserRating {
    std::string movieTitle;
    double rating;
};
struct UserProfile {
    std::string username;
    std::vector<std::string> FavoriteGenres;
    std::vector<UserRating> ratingHistory;
};

struct RecommendationResult {
    Movie movie;
    double baseCosineSimilarity;
    double contextMultiplier;
    double finalScore;
    std::string explanation;
};

//===================== Dataset Processing ====================================
class DatasetManager()
{
public:
    static std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delimiter)) {
            if (!token.empty() && token != "\\N") {
                tokens.push_back(token);
            }
        }
        return tokens;
    }
static std::vector<Movie> loadFromIMDbTSV(const std::string& basicsPath, const std::string& crewPath, size_t maxLimit = 5000) {
        std::unordered_map<std::string, Movie> movieMap;
        std::ifstream basicsFile(basicsPath);
        std::string line;

        if (!basicsFile.is_open()) {
            return {};
        }

        // Skip header
        std::getline(basicsFile, line);
        size_t count = 0;

        while (std::getline(basicsFile, line) && count < maxLimit) {
            auto cols = split(line, '\t');
            // IMDb basics columns: tconst(0), titleType(1), primaryTitle(2), originalTitle(3),
            // isAdult(4), startYear(5), endYear(6), runtimeMinutes(7), genres(8)
            if (cols.size() >= 9 && cols[1] == "movie") {
                Movie m;
                m.tconst = cols[0];
                m.title = cols[2];
                try {
                    m.releaseYear = std::stoi(cols[5]);
                    m.runtimeMinutes = std::stoi(cols[7]);
                } catch (...) {
                    continue;
                }
                m.genres = split(cols[8], ',');
                movieMap[m.tconst] = m;
                count++;
            }
        }
        basicsFile.close();

        // Load crew (directors & writers/actors)
        std::ifstream crewFile(crewPath);
        if (crewFile.is_open()) {
            std::getline(crewFile, line); // Skip header
            while (std::getline(crewFile, line)) {
                auto cols = split(line, '\t');
                if (cols.size() >= 2 && movieMap.count(cols[0])) {
                    movieMap[cols[0]].directors = split(cols[1], ',');
                    if (cols.size() >= 3) {
                        movieMap[cols[0]].actors = split(cols[2], ',');
                    }
                }
            }
            crewFile.close();
        }

        std::vector<Movie> results;
        for (auto& pair : movieMap) {
            results.push_back(pair.second);
        }
        return results;
    }

int main()
{

  return 0;
}
