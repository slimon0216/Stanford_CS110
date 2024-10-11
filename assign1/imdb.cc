#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
using namespace std;

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";
imdb::imdb(const string& directory) {
    const string actorFileName = directory + "/" + kActorFileName;
    const string movieFileName = directory + "/" + kMovieFileName;  
    actorFile = acquireFileMap(actorFileName, actorInfo);
    movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const {
    return !( (actorInfo.fd == -1) || 
            (movieInfo.fd == -1) ); 
}

imdb::~imdb() {
    releaseFileMap(actorInfo);
    releaseFileMap(movieInfo);
}

bool imdb::getCredits(const string& player, vector<film>& films) const { 
    int numPlayers = *(int*)actorFile;
    // cout << numPlayers << endl;
    //for (int i = 0; i < numPlayers; ++i) {
    //	int offset = *((int*)actorFile + 1 + i); 
    //	string name = (char*)actorFile + offset ;
    //	cout << name  << '\n';
    //}

    // Find player with binary search;
    // Create a pointer to the beginning of actor offsets
    int* offsetsBegin = (int*)actorFile + 1;
    int* offsetsEnd = offsetsBegin + numPlayers;

    // Use std::lower_bound to find the player
    int* result = std::lower_bound(offsetsBegin, offsetsEnd, player, [&](int offset, const std::string& key) {
            std::string name = (char*)actorFile + offset;
            return name < key;
            });

    if (result == offsetsEnd) {
        return false; // Player not found
    }

    int offset = *result;
    char* actor = (char*)actorFile + offset;
    std::string name = actor;
    if (name != player) return false; // Ensure the name matches exactly



    // cout << "Find " << name << endl;

    // Find credits
    char* ptr = actor + name.size()+1;
    if (!(name.size() &1)) ptr++;
    // next 2 bytes (short) represent # of movies
    short numFilms = *(short*)ptr;
    // cout << name << " has " << numFilms << '\n';

    /* 
       If the number of bytes dedicated to the actor’s name (always even) and the short (always 2)
       isn’t a multiple of four, then two additional '\0'’s appear after the two bytes storing the
       number of movies.
       */
    ptr += 2;
    while ((ptr-actor)%4 != 0) ptr++;
    for (int i = 0; i < numFilms; ++i) {
        int movieOffset = *((int*)ptr + i);
        char *ptr = (char*)movieFile + movieOffset;
        string filmName = ptr;
        ptr += filmName.size()+1;
        int year = *ptr + 1900;
        // cout << "Film: " << filmName << ' ' << year << "\n";
        film f;
        f.title = std::move(filmName);
        f.year = year;
        films.push_back(std::move(f));
    }

    return true;
}

bool imdb::getCast(const film& movie, vector<string>& players) const { 
    int numMovies = *(int*)movieFile;	
    //
    // Use std::lower_bound to find the movie
    int* offsetsBegin = (int*)movieFile + 1;
    int* offsetsEnd = (int*)movieFile + numMovies;
    int* result = std::lower_bound(offsetsBegin, offsetsEnd, movie, [&] (int offset, const film& key) {
            char* ptr = (char*)movieFile + offset;
            film cur;
            cur.title = ptr;
            ptr += cur.title.size()+1;
            cur.year = *ptr + 1900;
            return cur < key;
            });
    if (result == offsetsEnd) 
        return false; 
    int offset = *result;
    char* ptr = (char*)movieFile + offset;
    char* movieStart = ptr;
    film movieFound;
    movieFound.title = ptr;	
    ptr += movieFound.title.size()+1;
    movieFound.year = *ptr + 1900;
    if (!(movieFound == movie)) return false;
    // cout << movieFound.title << ' ' << movieFound.year << endl;
    // find players
    // If the total number of bytes used to encode the name and year of the movie is odd, then an extra '\0' sits in between the one-byte year and the data that follows.
    char *movie_ptr = (char *) movieFile + offset;
    long titleSpaceUsed = movie.title.length() + 1;
    movie_ptr += titleSpaceUsed;

    // Skip year
    long yearSpaceUsed = ((titleSpaceUsed + 1) % 2 == 0) ? 1 : 2;
    movie_ptr += yearSpaceUsed;

    // Skip number of actors
    short numActors = *(short *) movie_ptr;
    movie_ptr += ((titleSpaceUsed + yearSpaceUsed + 2) % 4 == 0) ? 2 : 4;

    // Store all players into vector
    for (short i=0; i<numActors; i++) {
        players.push_back((char*)actorFile + (*(int *) movie_ptr));
        movie_ptr += 4;
    }
    return true;
}

const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info) {
    struct stat stats;
    stat(fileName.c_str(), &stats);
    info.fileSize = stats.st_size;
    info.fd = open(fileName.c_str(), O_RDONLY);
    return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info) {
    if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
    if (info.fd != -1) close(info.fd);
}
