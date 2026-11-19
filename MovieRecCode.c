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


int main()
{

  return 0;
}
