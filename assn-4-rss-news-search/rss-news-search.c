#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "url.h"
#include "bool.h"
#include "urlconnection.h"
#include "streamtokenizer.h"
#include "html-utils.h"
#include "hashset.h"

#define DEBUG_HTML 0

static void Welcome(const char *welcomeTextFileName);
static void BuildIndices(const char *feedsFileName, hashset* index, hashset* stopWords);
static void ProcessFeed(const char *remoteDocumentName, hashset* index, hashset* stopWords);
static void PullAllNewsItems(urlconnection *urlconn, hashset* index, hashset* stopWords);
static bool GetNextItemTag(streamtokenizer *st);
static void ProcessSingleNewsItem(streamtokenizer *st, hashset* index, hashset* stopWords, hashset* alreadyParesedCheck);
static void ExtractElement(streamtokenizer *st, const char *htmlTag, char dataBuffer[], int bufferLength);
static void ParseArticle(const char *articleTitle, const char *articleDescription, const char *articleURL, hashset* index, hashset* stopWords);
static void ScanArticle(streamtokenizer *st, const char *articleTitle, const char *unused, const char *articleURL, hashset* index, hashset* stopWords);
static void QueryIndices(hashset *index);
static void ProcessResponse(const char *word, hashset *index);
static bool WordIsWellFormed(const char *word);

/**
 * Function: main
 * --------------
 * Serves as the entry point of the full application.
 * You'll want to update main to declare several hashsets--
 * one for stop words, another for previously seen urls, etc--
 * and pass them (by address) to BuildIndices and QueryIndices.
 * In fact, you'll need to extend many of the prototypes of the
 * supplied helpers functions to take one or more hashset *s.
 *
 * Think very carefully about how you're going to keep track of
 * all of the stop words, how you're going to keep track of
 * all the previously seen articles, and how you're going to 
 * map words to the collection of news articles where that
 * word appears.
 */

// My extra functions
static const signed long kHashMultiplier = -1664117991L;
static int StringHash(const char *s, int numBuckets)  
{            
  int i;
  unsigned long hashcode = 0;
  
  for (i = 0; i < strlen(s); i++)  
    hashcode = hashcode * kHashMultiplier + tolower(s[i]);  
  return hashcode % numBuckets;                                
}


// required structs that saves all the words and articles that contain that word
struct Word{
    char* word;		                  // a particular word
    struct vector articles;      // vector with information about articles in which the word ocurs
};

static int WordHash(const void *elem1, const int numBuckets)
{
  struct Word *map1 = (struct Word *)elem1;
  return StringHash(map1->word, numBuckets);
}

static int compareWord(const void *elem1, const void *elem2)
{
  struct Word *map1 = (struct Word *)elem1;
  struct Word *map2 = (struct Word *)elem2;
  return strcmp(map1->word, map2->word);
}


static void freeFnWordIndex(struct Word* elemAddr)
{
  free(elemAddr->word);
  VectorDispose(&elemAddr->articles);
}



// struct to save al the info of an atricle that contains a particular word
struct articleInformation{
  char* title;
  char* link;
  int wordCount;
};

static int CompareArticleInformation(const void *elem1, const void *elem2){
  struct articleInformation *info1 = (struct articleInformation *)elem1;
  struct articleInformation *info2 = (struct articleInformation *)elem2;
  // from large to small
  if (info1->wordCount > info2->wordCount) return -1; 
  else if (info1->wordCount < info2->wordCount) return 1;
  else return 0;
}



static void sortArticles(struct Word* word, void* auxData)
{
  VectorSort(&word->articles, CompareArticleInformation);
}

static void freeArticleInformation(struct articleInformation* elemAddr)
{
  free(elemAddr->link);
  free(elemAddr->title);
}




// to check if we already indexed an article
struct articleInfoStorage{
  char* title;		      
  char* URL;
  char* server;      
};

static void freeArticleInfoStorage(struct articleInfoStorage* elemAddr)
{
  free(elemAddr->title);
  free(elemAddr->URL);
  free(elemAddr->server);
}

static int ArticleInfoStorageHash(const void *elem1, const int numBuckets)
{
  struct articleInfoStorage *map1 = (struct Word *)elem1;
  return StringHash(map1->URL, numBuckets);
}

static int CompareArticleInfoStorage(const void *elem1, const void *elem2)
{
  char* empty = "";
  struct articleInfoStorage *info1 = (struct articleInfoStorage *)elem1;
  struct articleInfoStorage *info2 = (struct articleInfoStorage *)elem2;
  // check if the article s already parsed
  // can only be used with lineary search
  if(strcmp(info1->URL, info2->URL)==0)
  {
    return 0;
  }
  else if (strcmp(info1->title, info2->title)==0 && strcmp(info1->server, info2->server)==0)
  {
    return 0;
  }
  return -1;
}




static int CompareStringPointer(const char **string1, const char **string2)
{
  return strcmp(*string1, *string2);
} 

static void StringHashPointer(const char** elemAddr, int numBuckets)
{
  StringHash(*elemAddr, numBuckets);
}

static void freeStopWords(char** elemAddr)
{
  free(*elemAddr);
}

///// build stopWords hash set
static void buildStopWord(hashset* stopWords)
{
  FILE* fp;
  char* line = NULL;
  size_t len = 0;
  ssize_t read;

  fp = fopen("assn-4-rss-news-search-data/stop-words.txt", "r");
  if(fp == NULL) 

  {exit(EXIT_FAILURE);}

  while((read = getline(&line, &len, fp)) != -1) 
  {
    char* tmp = strdup(line);
    HashSetEnter(stopWords, &tmp);  
  }

  fclose(fp);
  if (line)
  {
   free(line);
  }
}





static const char *const kWelcomeTextFile = "assn-4-rss-news-search-data/welcome.txt";
static const char *const kDefaultFeedsFile = "assn-4-rss-news-search-data/rss-feeds-techy.txt";
int main(int argc, char **argv)
{
  Welcome(kWelcomeTextFile);
  
  hashset stopWords;
  HashSetNew(&stopWords, sizeof(char*), 32, StringHashPointer, CompareStringPointer, freeStopWords);
  buildStopWord(&stopWords);

  hashset index;
  HashSetNew(&index, sizeof(struct Word), 100, WordHash, compareWord, freeFnWordIndex);
  
  BuildIndices((argc == 1) ? kDefaultFeedsFile : argv[1], &index, &stopWords);
  HashSetMap(&index, sortArticles, NULL);

  QueryIndices(&index);

  HashSetDispose(&index);
  HashSetDispose(&stopWords);
  return 0;
}



/** 
 * Function: Welcome
 * -----------------
 * Displays the contents of the specified file, which
 * holds the introductory remarks to be printed every time
 * the application launches.  This type of overhead may
 * seem silly, but by placing the text in an external file,
 * we can change the welcome text without forcing a recompilation and
 * build of the application.  It's as if welcomeTextFileName
 * is a configuration file that travels with the application.
 */
 
static const char *const kNewLineDelimiters = "\r\n";
static void Welcome(const char *welcomeTextFileName)
{
  FILE *infile;
  streamtokenizer st;
  char buffer[1024];
  
  infile = fopen(welcomeTextFileName, "r");
  assert(infile != NULL);    
  
  STNew(&st, infile, kNewLineDelimiters, true);
  while (STNextToken(&st, buffer, sizeof(buffer))){
    printf("%s\n", buffer);
  }
  printf("\n");
  STDispose(&st); // remember that STDispose doesn't close the file, since STNew doesn't open one.. 
  fclose(infile);
}



/**
 * Function: BuildIndices
 * ----------------------
 * As far as the user is concerned, BuildIndices needs to read each and every
 * one of the feeds listed in the specied feedsFileName, and for each feed parse
 * content of all referenced articles and store the content in the hashset of indices.
 * Each line of the specified feeds file looks like this:
 *
 *   <feed name>: <URL of remore xml document>
 *
 * Each iteration of the supplied while loop parses and discards the feed name (it's
 * in the file for humans to read, but our aggregator doesn't care what the name is)
 * and then extracts the URL.  It then relies on ProcessFeed to pull the remote
 * document and index its content.
 */

static void BuildIndices(const char *feedsFileName, hashset* index, hashset* stopWords)
{
  ///// process the articles
  FILE *infile;
  streamtokenizer st;
  char remoteFileName[1024];
  
  infile = fopen(feedsFileName, "r");
  assert(infile != NULL);
  STNew(&st, infile, kNewLineDelimiters, true);
  while (STSkipUntil(&st, ":") != EOF) { // ignore everything up to the first selicolon of the line
    STSkipOver(&st, ": ");		 // now ignore the semicolon and any whitespace directly after it
    STNextToken(&st, remoteFileName, sizeof(remoteFileName)); 
    
    ProcessFeed(remoteFileName, index, stopWords);
  }
  STDispose(&st);
  fclose(infile);
  printf("\n");
}



/**
 * Function: ProcessFeed
 * ---------------------
 * ProcessFeed locates the specified RSS document, and if a (possibly redirected) connection to that remote
 * document can be established, then PullAllNewsItems is tapped to actually read the feed.  Check out the
 * documentation of the PullAllNewsItems function for more information, and inspect the documentation
 * for ParseArticle for information about what the different response codes mean.
 */

static void ProcessFeed(const char *remoteDocumentName, hashset* index, hashset* stopWords)
{
  url u;
  urlconnection urlconn;
  
  URLNewAbsolute(&u, remoteDocumentName);
  URLConnectionNew(&urlconn, &u);
  
  switch (urlconn.responseCode) {
      case 0: printf("Unable to connect to \"%s\".  Ignoring...", u.serverName);
              break;
      case 200: PullAllNewsItems(&urlconn, index, stopWords);
                break;
      case 301: 
      case 302: ProcessFeed(urlconn.newUrl, index, stopWords);
                break;
      default: printf("Connection to \"%s\" was established, but unable to retrieve \"%s\". [response code: %d, response message:\"%s\"]\n",
		      u.serverName, u.fileName, urlconn.responseCode, urlconn.responseMessage);
	       break;
  };
  URLConnectionDispose(&urlconn);
  URLDispose(&u);
}

/**
 * Function: PullAllNewsItems
 * --------------------------
 * Steps though the data of what is assumed to be an RSS feed identifying the names and
 * URLs of online news articles.  Check out "datafiles/sample-rss-feed.txt" for an idea of what an
 * RSS feed from the www.nytimes.com (or anything other server that syndicates is stories).
 *
 * PullAllNewsItems views a typical RSS feed as a sequence of "items", where each item is detailed
 * using a generalization of HTML called XML.  A typical XML fragment for a single news item will certainly
 * adhere to the format of the following example:
 *
 * <item>
 *   <title>At Installation Mass, New Pope Strikes a Tone of Openness</title>
 *   <link>http://www.nytimes.com/2005/04/24/international/worldspecial2/24cnd-pope.html</link>
 *   <description>The Mass, which drew 350,000 spectators, marked an important moment in the transformation of Benedict XVI.</description>
 *   <author>By IAN FISHER and LAURIE GOODSTEIN</author>
 *   <pubDate>Sun, 24 Apr 2005 00:00:00 EDT</pubDate>
 *   <guid isPermaLink="false">http://www.nytimes.com/2005/04/24/international/worldspecial2/24cnd-pope.html</guid>
 * </item>
 *
 * PullAllNewsItems reads and discards all characters up through the opening <item> tag (discarding the <item> tag
 * as well, because once it's read and indentified, it's been pulled,) and then hands the state of the stream to
 * ProcessSingleNewsItem, which handles the job of pulling and analyzing everything up through and including the </item>
 * tag. PullAllNewsItems processes the entire RSS feed and repeatedly advancing to the next <item> tag and then allowing
 * ProcessSingleNewsItem do process everything up until </item>.
 */

static const char *const kTextDelimiters = " \t\n\r\b!@$%^*()_+={[}]|\\'\":;/?.>,<~`";
static void PullAllNewsItems(urlconnection *urlconn, hashset* index, hashset* stopWords)
{
  streamtokenizer st;
  STNew(&st, urlconn->dataStream, kTextDelimiters, false);

  hashset alreadyParesedCheck;
  HashSetNew(&alreadyParesedCheck, sizeof(struct articleInfoStorage), 12, ArticleInfoStorageHash, CompareArticleInfoStorage, freeArticleInfoStorage);

  while (GetNextItemTag(&st)) { // if true is returned, then assume that <item ...> has just been read and pulled from the data stream
    ProcessSingleNewsItem(&st, index, stopWords, &alreadyParesedCheck);
  }

  HashSetDispose(&alreadyParesedCheck);
  STDispose(&st);
}

/**
 * Function: GetNextItemTag
 * ------------------------
 * Works more or less like GetNextTag below, but this time
 * we're searching for an <item> tag, since that marks the
 * beginning of a block of HTML that's relevant to us.  
 * 
 * Note that each tag is compared to "<item" and not "<item>".
 * That's because the item tag, though unlikely, could include
 * attributes and perhaps look like any one of these:
 *
 *   <item>
 *   <item rdf:about="Latin America reacts to the Vatican">
 *   <item requiresPassword=true>
 *
 * We're just trying to be as general as possible without
 * going overboard.  (Note that we use strncasecmp so that
 * string comparisons are case-insensitive.  That's the case
 * throughout the entire code base.)
 */

static const char *const kItemTagPrefix = "<item";
static bool GetNextItemTag(streamtokenizer *st)
{
  char htmlTag[1024];
  while (GetNextTag(st, htmlTag, sizeof(htmlTag))) {
    if (strncasecmp(htmlTag, kItemTagPrefix, strlen(kItemTagPrefix)) == 0) {
      return true;
    }
  }	 
  return false;
}

/**
 * Function: ProcessSingleNewsItem
 * -------------------------------
 * Code which parses the contents of a single <item> node within an RSS/XML feed.
 * At the moment this function is called, we're to assume that the <item> tag was just
 * read and that the streamtokenizer is currently pointing to everything else, as with:
 *   
 *      <title>Carrie Underwood takes American Idol Crown</title>
 *      <description>Oklahoma farm girl beats out Alabama rocker Bo Bice and 100,000 other contestants to win competition.</description>
 *      <link>http://www.nytimes.com/frontpagenews/2841028302.html</link>
 *   </item>
 *
 * ProcessSingleNewsItem parses everything up through and including the </item>, storing the title, link, and article
 * description in local buffers long enough so that the online new article identified by the link can itself be parsed
 * and indexed.  We don't rely on <title>, <link>, and <description> coming in any particular order.  We do asssume that
 * the link field exists (although we can certainly proceed if the title and article descrption are missing.)  There
 * are often other tags inside an item, but we ignore them.
 */

static const char *const kItemEndTag = "</item>";
static const char *const kTitleTagPrefix = "<title";
static const char *const kDescriptionTagPrefix = "<description";
static const char *const kLinkTagPrefix = "<link";
static void ProcessSingleNewsItem(streamtokenizer *st, hashset* index, hashset* stopWords, hashset* alreadyParesedCheck)
{
  char htmlTag[1024];
  char articleTitle[1024];
  char articleDescription[1024];
  char articleURL[1024];
  articleTitle[0] = articleDescription[0] = articleURL[0] = '\0';
  
  while (GetNextTag(st, htmlTag, sizeof(htmlTag)) && (strcasecmp(htmlTag, kItemEndTag) != 0)) {
    if (strncasecmp(htmlTag, kTitleTagPrefix, strlen(kTitleTagPrefix)) == 0) ExtractElement(st, htmlTag, articleTitle, sizeof(articleTitle));
    if (strncasecmp(htmlTag, kDescriptionTagPrefix, strlen(kDescriptionTagPrefix)) == 0) ExtractElement(st, htmlTag, articleDescription, sizeof(articleDescription));
    if (strncasecmp(htmlTag, kLinkTagPrefix, strlen(kLinkTagPrefix)) == 0) ExtractElement(st, htmlTag, articleURL, sizeof(articleURL));
  }
  
  if (strncmp(articleURL, "", sizeof(articleURL)) == 0) return;     // punt, since it's not going to take us anywhere
  
  // Find the start of the server part
  const char* prefix = "http://";
  char* serverStart =  articleURL + strlen(prefix)+1;
  
  // Find the end of the server part
  char* serverEnd = strchr(serverStart, '/');

  int serverLength = serverEnd - serverStart;
  char* server = malloc(serverLength + 1);
  strncpy(server, serverStart, serverLength);
  
  server[serverLength] = '\0'; 
  
  struct articleInfoStorage articleInfo;
  articleInfo.title = strdup(articleTitle);
  articleInfo.URL = strdup(articleURL);
  articleInfo.server = strdup(server);


  //printf("URL: %s\n", articleURL);
  //printf("title: %s\n", articleTitle);
  //printf("server: %s\n", server);
  char* empty= "";
  if(strcmp(articleInfo.title, empty)==0)
  {
    //printf("unknown\n");
    HashSetEnter(alreadyParesedCheck, &articleInfo);
    ParseArticle(articleTitle, articleDescription, articleURL, index, stopWords);
  }
  else if(HashSetLookup(alreadyParesedCheck, &articleInfo)==NULL)
  {
    //printf("new\n");
    HashSetEnter(alreadyParesedCheck, &articleInfo);
    ParseArticle(articleTitle, articleDescription, articleURL, index, stopWords);
  }
  //else{printf("done\n");}
  
}



/**
 * Function: ExtractElement
 * ------------------------
 * Potentially pulls text from the stream up through and including the matching end tag.  It assumes that
 * the most recently extracted HTML tag resides in the buffer addressed by htmlTag.  The implementation
 * populates the specified data buffer with all of the text up to but not including the opening '<' of the
 * closing tag, and then skips over all of the closing tag as irrelevant.  Assuming for illustration purposes
 * that htmlTag addresses a buffer containing "<description" followed by other text, these three scanarios are
 * handled:
 *
 *    Normal Situation:     <description>http://some.server.com/someRelativePath.html</description>
 *    Uncommon Situation:   <description></description>
 *    Uncommon Situation:   <description/>
 *
 * In each of the second and third scenarios, the document has omitted the data.  This is not uncommon
 * for the description data to be missing, so we need to cover all three scenarious (I've actually seen all three.)
 * It would be quite unusual for the title and/or link fields to be empty, but this handles those possibilities too.
 */
 
static void ExtractElement(streamtokenizer *st, const char *htmlTag, char dataBuffer[], int bufferLength)
{
  assert(htmlTag[strlen(htmlTag) - 1] == '>');
  if (htmlTag[strlen(htmlTag) - 2] == '/') return;    // e.g. <description/> would state that a description is not being supplied
  STNextTokenUsingDifferentDelimiters(st, dataBuffer, bufferLength, "<");
  RemoveEscapeCharacters(dataBuffer);
  if (dataBuffer[0] == '<') strcpy(dataBuffer, "");  // e.g. <description></description> also means there's no description
  STSkipUntil(st, ">");
  STSkipOver(st, ">");
}

/** 
 * Function: ParseArticle
 * ----------------------
 * Attempts to establish a network connect to the news article identified by the three
 * parameters.  The network connection is either established of not.  The implementation
 * is prepared to handle a subset of possible (but by far the most common) scenarios,
 * and those scenarios are categorized by response code:
 *
 *    0 means that the server in the URL doesn't even exist or couldn't be contacted.
 *    200 means that the document exists and that a connection to that very document has
 *        been established.
 *    301 means that the document has moved to a new location
 *    302 also means that the document has moved to a new location
 *    4xx and 5xx (which are covered by the default case) means that either
 *        we didn't have access to the document (403), the document didn't exist (404),
 *        or that the server failed in some undocumented way (5xx).
 *
 * The are other response codes, but for the time being we're punting on them, since
 * no others appears all that often, and it'd be tedious to be fully exhaustive in our
 * enumeration of all possibilities.
 */

static void ParseArticle(const char *articleTitle, const char *articleDescription, const char *articleURL, hashset* index, hashset* stopWords)
{
  url u;
  urlconnection urlconn;
  streamtokenizer st;

  URLNewAbsolute(&u, articleURL);
  URLConnectionNew(&urlconn, &u);
  
  switch (urlconn.responseCode) {
      case 0: printf("Unable to connect to \"%s\".  Domain name or IP address is nonexistent.\n", articleURL);
	      break;
      case 200: printf("Scanning \"%s\" from \"http://%s\"\n", articleTitle, u.serverName);
	        STNew(&st, urlconn.dataStream, kTextDelimiters, false);
		ScanArticle(&st, articleTitle, articleDescription, articleURL, index, stopWords);
		STDispose(&st);
		break;
      case 301:
      case 302: // just pretend we have the redirected URL all along, though index using the new URL and not the old one...
                ParseArticle(articleTitle, articleDescription, urlconn.newUrl, index, stopWords);
		break;
      default: printf("Unable to pull \"%s\" from \"%s\". [Response code: %d] Punting...\n", articleTitle, u.serverName, urlconn.responseCode);
	       break;
  }
  
  URLConnectionDispose(&urlconn);
  URLDispose(&u);
}

/**
 * Function: ScanArticle
 * ---------------------
 * Parses the specified article, skipping over all HTML tags, and counts the numbers
 * of well-formed words that could potentially serve as keys in the set of indices.
 * Once the full article has been scanned, the number of well-formed words is
 * printed, and the longest well-formed word we encountered along the way
 * is printed as well.
 *
 * This is really a placeholder implementation for what will ultimately be
 * code that indexes the specified content.
 */


static void ScanArticle(streamtokenizer *st, const char *articleTitle, const char *unused, const char *articleURL, hashset* index, hashset* stopWords)
{
  int numWords = 0;
  char word[1024];
  char longestWord[1024] = {'\0'};
  

  while (STNextToken(st, word, sizeof(word))) {
    if (strcasecmp(word, "<") == 0) 
    {
      //I had to increase the buffer of this fuction to make it work: 
      SkipIrrelevantContent(st); // in html-utls.h
    } 
    else 
    {
      RemoveEscapeCharacters(word);
      char* Word = word;

      if (WordIsWellFormed(word) && HashSetLookup(stopWords, &Word) == NULL)
      {
        // set up the index tables
        //printf("word: %s\n", word);
  	    numWords++;

        struct Word localWord;
        localWord.word = strdup(Word);
        struct Word *found = HashSetLookup(index, &localWord);	
      
        if(found==NULL)
        {
          HashSetEnter(index, &localWord);

          struct Word *address = HashSetLookup(index , &localWord);
          VectorNew(&address->articles, sizeof(struct articleInformation), freeArticleInformation, 10);
          
          struct articleInformation localInfo;
          localInfo.link = strdup(articleURL);
          localInfo.title = strdup(articleTitle);
          localInfo.wordCount = 1;
          VectorAppend(&address->articles, &localInfo);
        }
        else
        {
          // if the localWord is not added to the Hash set, localWord.word has to be freed. 
          free(localWord.word);

          struct articleInformation *currentInfo;
          currentInfo = VectorNth(&found->articles, VectorLength(&found->articles)-1);

          if(strcmp(currentInfo->link, articleURL)==0)
          {
            currentInfo->wordCount = (currentInfo->wordCount)+1;
          }
          else
          {
            struct articleInformation localInfo;
            localInfo.link = strdup(articleURL);
            localInfo.title = strdup(articleTitle);
            localInfo.wordCount = 1;
            VectorAppend(&found->articles, &localInfo);
          }
        }
        if (strlen(word) > strlen(longestWord)){strcpy(longestWord, word);}
      }
    }
  }
  printf("\tWe counted %d well-formed words [including duplicates].\n", numWords);
  printf("\tThe longest word scanned was \"%s\".", longestWord);
  if (strlen(longestWord) >= 15 && (strchr(longestWord, '-') == NULL)) 
    printf(" [Ooooo... long word!]");
  printf("\n");
}

/** 
 * Function: QueryIndices
 * ----------------------
 * Standard query loop that allows the user to specify a single search term, and
 * then proceeds (via ProcessResponse) to list up to 10 articles (sorted by relevance)
 * that contain that word.
 */

static void QueryIndices(hashset *index)
{
  char response[1024];
  while (true) {
    printf("Please enter a single query term that might be in our set of indices [enter to quit]: ");
    fgets(response, sizeof(response), stdin);
    response[strlen(response) - 1] = '\0';
    if (strcasecmp(response, "") == 0) break;
    ProcessResponse(response, index);
  }
}

/** 
 * Function: ProcessResponse
 * -------------------------
 * Placeholder implementation for what will become the search of a set of indices
 * for a list of web documents containing the specified word.
 */

static void ProcessResponse(const char *word, hashset *index)
{
  if(WordIsWellFormed(word)) 
  {
    //printf("\tWell, we don't have the database mapping words to online news articles yet, but if we DID have\n");
    char* Word = word;
    struct Word tmp;
    tmp.word = strdup(Word);

    struct Word* Articles = HashSetLookup(index, &tmp);
    if(Articles == NULL){printf("\t this word is not included in any of the articles\n");}
    else
    {
      for(int i=0; i < VectorLength(&Articles->articles); i++)
      {
        struct articleInformation* inforamtion = VectorNth(&Articles->articles, i);

        printf("%i.)  %s [Search term occured %i times]\n", i+1, inforamtion->title, inforamtion->wordCount);
        printf("%s\n", inforamtion->link);
        printf("\n");
        
        if(i==9){break;}
      }
    }
    free(tmp.word);
  } 
  else {
    printf("\tWe won't be allowing words like \"%s\" into our set of indices.\n", word);
  }
}

/**
 * Predicate Function: WordIsWellFormed
 * ------------------------------------
 * Before we allow a word to be inserted into our map
 * of indices, we'd like to confirm that it's a good search term.
 * One could generalize this function to allow different criteria, but
 * this version hard codes the requirement that a word begin with 
 * a letter of the alphabet and that all letters are either letters, numbers,
 * or the '-' character.  
 */

static bool WordIsWellFormed(const char *word)
{
  int i;
  if (strlen(word) == 0) return true;
  if (!isalpha((int) word[0])) return false;
  for (i = 1; i < strlen(word); i++)
    if (!isalnum((int) word[i]) && (word[i] != '-')) return false; 

  return true;
}
