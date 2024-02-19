#include <vector>
#include <list>
#include <set>
#include <string>
#include <iostream>
#include <iomanip>
#include "imdb.h"
#include "path.h"
using namespace std;

/**
 * Using the specified prompt, requests that the user supply
 * the name of an actor or actress.  The code returns
 * once the user has supplied a name for which some record within
 * the referenced imdb existsif (or if the user just hits return,
 * which is a signal that the empty string should just be returned.)
 *
 * @param prompt the text that should be used for the meaningful
 *               part of the user prompt.
 * @param db a reference to the imdb which can be used to confirm
 *           that a user's response is a legitimate one.
 * @return the name of the user-supplied actor or actress, or the
 *         empty string.
 */
    
static string promptForActor(const string& prompt, const imdb& db)
{
  string response;
  while (true) {
    cout << prompt << " [or <enter> to quit]: ";
    getline(cin, response);
    if (response == "") return "";
    vector<film> credits;
    if (db.getCredits(response, credits)) return response;
    cout << "We couldn't find \"" << response << "\" in the movie database. "
	 << "Please try again." << endl;
  }
}


static path searchPath(const imdb& IMDB, path& startPath, const string& target)
{
  list<path> queue;
  queue.insert(queue.end(), startPath);
  set<pair<string, int> > seenMovies;
  set<string> seenPlayers;

  while(queue.size()!=0)
  {
    path front = queue.front();
    if(front.getLength()>5){break;}
    queue.pop_front();

    vector<film> movies;
    bool check = IMDB.getCredits(front.getLastPlayer(), movies);
    for(int i=0; i<movies.size(); i++)
    {
      if(seenMovies.count(make_pair(movies[i].title, movies[i].year))==0)
      {
        seenMovies.insert(make_pair(movies[i].title, movies[i].year));

        vector<string> players;
        bool check = IMDB.getCast(movies[i], players);
        for(int j=0; j<players.size(); j++)
        { 
          if(seenPlayers.count(players[j])==0)
          {
            seenPlayers.insert(players[j]);
            front.addConnection(movies[i], players[j]);
            if(players[j]==target){return front;} 
            queue.insert(queue.end(), front);
            front.undoConnection();
          }
        } 
      }
    }
  }
  return  startPath;
}

static path generateShortestPath(const string& start, const string& target)
{
  imdb IMDB(determinePathToData());
  path Path(start);
  Path = searchPath(IMDB, Path, target);
  Path.reverse();
  if(Path.getLength()==0){ printf("could not find a path of lenght 6 or less"); }
  else{std::cout << Path;}
  return Path;
}

/**
 * Serves as the main entry point for the six-degrees executable.
 * There are no parameters to speak of.
 *
 * @param argc the number of tokens passed to the command line to
 *             invoke this executable.  It's completely ignored
 *             here, because we don't expect any arguments.
 * @param argv the C strings making up the full command line.
 *             We expect argv[0] to be logically equivalent to
 *             "six-degrees" (or whatever absolute path was used to
 *             invoke the program), but otherwise these are ignored
 *             as well.
 * @return 0 if the program ends normally, and undefined otherwise.
 */

int main(int argc, const char *argv[])
{
  imdb db(determinePathToData(argv[1])); // inlined in imdb-utils.h
  if (!db.good()) {
    cout << "Failed to properly initialize the imdb database." << endl;
    cout << "Please check to make sure the source files exist and that you have permission to read them." << endl;
    exit(1);
  }
  
  while (true) {
    string source = promptForActor("Actor or actress", db);
    //"Bradley Cooper (I)"
    //"Leonardo DiCaprio"
    if (source == "") break;
    string target = promptForActor("Another actor or actress", db);
    //"Brad Pitt"
    // "Hans Teeuwen"
    if (target == "") break;
    if (source == target) {
      cout << "Good one.  This is only interesting if you specify two different people." << endl;
    } 
    else { path Path = generateShortestPath(source, target); }
  }
  cout << "Thanks for playing!" << endl;
  return 0;
}

