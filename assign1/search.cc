#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <queue>
#include "imdb.h"
using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <actor> " << "<actor>" << endl;
        return 1;
    }
    imdb db(kIMDBDataDirectory);
    if (!db.good()) {
        cerr << "Data file not found!  Exiting..." << endl;
        return 1;
    }
    // do the bfs to find the relation between the two actors
    string source = argv[1];
    string target = argv[2];
    std::queue<string> q;
    std::map<string, string> parent; // can be player or film
    std::set<string> visited;
    parent[source] = "";
    visited.insert(source);
    q.push(source);
    while (q.size()) {
        string cur = q.front();
        q.pop();
        if (cur == target) {
            vector<string> path;
            while (cur != "") {
                path.push_back(cur);
                cur = parent[cur];
            }
            reverse(path.begin(), path.end());
            for (int i = 0; i+2 < path.size(); i+=2) {
                cout << path[i] << " was in \"" << path[i+1] << "\" with " << path[i+2] << '.' << endl;
            }
            return 0;
        }
        vector<film> films;
        if (!db.getCredits(cur, films))
            continue;
        for (const film &f : films) {
            if (visited.count(f.title))
                continue;
            visited.insert(f.title);
            parent[f.title] = cur;
            vector<string> actors;
            if (!db.getCast(f, actors))
                continue;
            for (string &a : actors) {
                if (visited.count(a))
                    continue;
                parent[a] = f.title;
                visited.insert((a));
                q.push(std::move(a));
            }
        }
    }
    return 0;
}
