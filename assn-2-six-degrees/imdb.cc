using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include <string.h>
#include <stdio.h>


const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

// you should be implementing these two methods right here... 
bool imdb::getCredits(const string& player, vector<film>& films) const 
{ 
  int low = 1;
  int high = *(int*)actorFile;
  int mid = (*(int*)actorFile)/2;

  bool Bool = false; 
  while(true)
  {
    char* address = (char*)actorFile + *((int*)actorFile + mid);
    string Try(address);

    if(Try == player)
    {
      Bool = true; 
      int num_movies_idx = ((Try.length())%2 == 0) ? Try.length()+2: Try.length()+1;

      short num_movies = *(short*)((char*)address + num_movies_idx);
      int first_movie = (num_movies_idx%4 == 0) ? num_movies_idx+4: num_movies_idx+2;

      for(int i=0; i<num_movies; i++)
      {
        int movie_idx = *(int*)(address + first_movie + i*4);

        film tmp;
        string str((char*)movieFile + movie_idx);
        tmp.title = str;
        tmp.year = (int) *((char*)movieFile + movie_idx + str.length()+1);
        films.push_back(tmp);
      }
      break;
    }

    if(Try>player)
    {
      if(mid==low) {break;}
      high = mid-1;
      mid = (low-1) + ((mid-low)+1)/2;
    }
    else
    {
      if(mid==high) {break;}
      low = mid+1;
      mid = mid + ((high-mid)+1)/2;
    }
  }
  return Bool; 
}

bool imdb::getCast(const film& movie, vector<string>& players) const
{
  int low = 1;
  int high = *(int*)movieFile;
  int mid = (*(int*)movieFile)/2;

  bool Bool = false; 
  film Try;

  while(true)
  {
    char* address = (char*)movieFile + *((int*)movieFile + mid);
    //*(((char**)actorFile) + mid);
    //movie_revo  = *address;
    string str(address);
    Try.title = str;
    Try.year = *(address + str.length()+1);

    if(Try == movie)
    {
      Bool = true;
      int num_players_idx = (str.length()%2 == 0) ? str.length()+2: str.length()+3;
      short num_players = *(short*)(address + num_players_idx);

      int first_player = (num_players_idx%4 == 0) ? num_players_idx+4: num_players_idx+2;
      for(int i=0; i<num_players; i++)
      {
        int actor_idx = *(int*)(address + first_player + i*4);
        string tmp((char*)actorFile + actor_idx);
        players.push_back(tmp);
      }
      break;
    }

    if(movie<Try)
    {
      if(mid==low) {break;}
      high = mid-1;
      mid = (low-1) + ((mid-low)+1)/2;
    }
    else
    {
      if(mid==high) {break;}
      low = mid+1;
      mid = mid + ((high-mid)+1)/2;
    }
  }
  return Bool;
}


imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}