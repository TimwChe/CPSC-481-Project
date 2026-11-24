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
//Sample catalog for immediate testing and demonstration
    static std::vector<Movie> loadSampleCatalog(){

    }

//===================== Content-Based Filtering and Cosine Similarity Engine ====================================
class ContentBasedEngine {
private:
    std::unordered_map<std::string, size_t> featureVocab;

    void addToken(const std::string& prefix, const std::string& value) {
        std::string key = prefix + ":" + value;
        if (featureVocab.find(key) == featureVocab.end()) {
            size_t nextIdx = featureVocab.size();
            featureVocab[key] = nextIdx;
        }
    }

public:
    // Builds vocabulary across all genres, directors, and actors in the dataset
    void buildVocabulary(const std::vector<Movie>& movies) {
        featureVocab.clear();
        for (const auto& m : movies) {
            for (const auto& g : m.genres) addToken("GENRE", g);
            for (const auto& d : m.directors) addToken("DIR", d);
            for (const auto& a : m.actors) addToken("ACT", a);
        }
    }

    // Converts an individual Movie into a weighted feature vector
    std::vector<double> vectorizeMovie(const Movie& movie) const {
        std::vector<double> vec(featureVocab.size(), 0.0);

        // Feature weights: Genres (1.0), Directors (0.8), Actors (0.6)
        for (const auto& g : movie.genres) {
            auto it = featureVocab.find("GENRE:" + g);
            if (it != featureVocab.end()) vec[it->second] = 1.0;
        }
        for (const auto& d : movie.directors) {
            auto it = featureVocab.find("DIR:" + d);
            if (it != featureVocab.end()) vec[it->second] = 0.8;
        }
        for (const auto& a : movie.actors) {
            auto it = featureVocab.find("ACT:" + a);
            if (it != featureVocab.end()) vec[it->second] = 0.6;
        }
        return vec;
    }
//Builds the User Preference Vector
std::vector<double> buildUserVector(const UserProfile& user, const std::vector<Movie>& catalog) const {
        std::vector<double> userVec(featureVocab.size(), 0.0);

        //This loop states the user's base favorite genres
        for (const auto& g : user.baseFavoriteGenres) {
            auto it = featureVocab.find("GENRE:" + g);
            if (it != featureVocab.end()) {
                userVec[it->second] += 1.5;
            }
        }

        //The 2nd loop finds the historical movie ratings (centered around neutral rating 2.5)
        for (const auto& rated : user.ratingHistory) {
            for (const auto& m : catalog) {
                if (m.title == rated.movieTitle) {
                    auto movieVec = vectorizeMovie(m);
                    double weight = (rated.rating - 2.0) / 3.0; // Positive boost for >2.0, penalty for <2.0
                    for (size_t i = 0; i < userVec.size(); ++i) {
                        userVec[i] += movieVec[i] * weight;
                    }
                    break;
                }
            }
        }

        //This loop brings negative values to 0 for standard non-negative cosine comparison
        for (double& val : userVec) {
            if (val < 0.0) val = 0.0;
        }
        return userVec;
    }

    //This will computes the Cosine Similarity between two feature vectors
    static double computeCosineSimilarity(const std::vector<double>& vecA, const std::vector<double>& vecB) {
        double dotProduct = 0.0;
        double normA = 0.0;
        double normB = 0.0;

        for (size_t i = 0; i < vecA.size(); ++i) {
            dotProduct += vecA[i] * vecB[i];
            normA += vecA[i] * vecA[i];
            normB += vecB[i] * vecB[i];
        }

        if (normA == 0.0 || normB == 0.0) return 0.0;
        return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
    }
};

//========================= Context-Aware Scoring Engine =========================================
class ContextAwareScorer {
private:
    // Maps user mood to associated film genres
    static std::vector<std::string> getMoodGenres(const std::string& mood) {
        std::string m = mood;
        std::transform(m.begin(), m.end(), m.begin(), ::tolower);

        if (m == "happy" || m == "upbeat" || m == "fun")
            return {"Comedy", "Animation", "Adventure", "Music"};
        if (m == "tense" || m == "excited" || m == "thrilled")
            return {"Action", "Thriller", "Horror", "Crime", "Mystery"};
        if (m == "thoughtful" || m == "intellectual" || m == "serious")
            return {"Sci-Fi", "Drama", "Mystery"};
        if (m == "relaxed" || m == "chill" || m == "cozy")
            return {"Comedy", "Animation", "Romance", "Adventure"};
        if (m == "adventurous")
            return {"Adventure", "Action", "Sci-Fi"};

        return {};
    }

public:
    static RecommendationResult evaluateMovie(
        const Movie& movie,
        double baseCosineSim,
        const ViewingContext& ctx,
        std::ostringstream& reasonStream
    ) {
        double multiplier = 1.0;

        // 1. time check for availability length
        if (movie.runtimeMinutes > ctx.availableTimeMinutes) {
            // Filter out movies that exceed available viewing time
            return {movie, baseCosineSim, 0.0, -1.0, "Exceeds available time window."};
        } else {
            // A bonus for utilizing available time well (within 35 mins of limit)
            int diff = ctx.availableTimeMinutes - movie.runtimeMinutes;
            if (diff <= 35) {
                multiplier += 0.15;
                reasonStream << "[Fits " << ctx.availableTimeMinutes << "m limit well (" << movie.runtimeMinutes << "m)] ";
            } else {
                reasonStream << "[Runtime: " << movie.runtimeMinutes << "m] ";
            }
        }

//Mood Alignment

int main()
{

  return 0;
}
