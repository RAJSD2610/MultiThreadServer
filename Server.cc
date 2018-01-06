/**********************************************************************************************/
/* webServer.cpp  03-12-2017  version-1

   Group Members: Disha Siddappana Palya Eshwarappa
                  Mohanish Panchal
                  Rajvardhan Deshmukh
                  Shamanth Kumar Parameshwar

   Purpose: A web server with threaded approach.

   Variables Nomenclature: Variables start with small letter of their data type and then capital
                           letters for each new word.
                           Example - int iVariableName;                                       */
/**********************************************************************************************/

#include <iostream>
#include <cstdio>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <cstdlib>
#include <string>
#include <pthread.h>
#include <sstream>
#include <vector>
#include <string.h>
#include <fstream>
#include <ctime>


#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>
#include <map>
#include <list>
#include <errno.h>
#include <math.h> 

using namespace std;

#define handle_error_en(en, msg) \
do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define DEFAULT 10

#define STAT_HTTP_200 " 200 OK\n"
#define STAT_HTTP_404 " 404 Not Found\n"
#define STAT_HTTP_403 " 403 Forbidden\n"
#define STAT_HTTP_400 " 400 Bad Request\n"

#define GIF "Content-Type: image/gif\n"
#define TXT_HTML "Content-Type: text/html\n"
#define JPEG "Content-Type: image/jpeg\n"
#define JPG "Content-Type: image/jpg\n"

#define MSG_404 "\n\n<html>\n <body>\n  <h1>Not Found</h1>\n\n  <p>The requested URL was not found on this server.</p>\n </body>\n</html>\n"
#define MSG_403 "\n\n<html>\n <body>\n  <h1>Forbidden</h1>\n  <p>403 Forbidden You don't have permission to access it on this server.</p>\n </body>\n</html>\n"
#define MSG_400 "\n\n<html>\n <body>\n  <h1>Bad Request</h1>\n  <p>This server did not understand your request.</p>\n </body>\n</html>\n"


typedef enum { gif=0, html=1, jpeg=2, jpg=3, txt=4} ext;

ext get_ext(char const *file) {
  if (strstr(file, ".gif") != NULL)
    return gif;
  else if (strstr(file, ".html") != NULL)
    return html;
  else if (strstr(file, ".jpeg") != NULL)
    return jpeg;
  else if (strstr(file, ".jpg") != NULL)
    return jpg;
  else if (strstr(file, ".txt") != NULL)
    return txt;
  else
    return txt;
}
//use lock on global variables 
int DELAY;
string sRootPath;
pthread_mutex_t mutexsum;//locks
int iActiveThreads=0;

void *handle_request(void *arg)
{
  int iSockId = (intptr_t)arg;
  char buffer[1024];
  char out_buf[1024];
  char *pFileBuff;
  string sTemp,sTemp1;
  ssize_t len;
  string line;
  char *con_len;
  bool bIsPersistent;
   //keep persistant connection and unlock
   //timer
  clock_t startTime = clock(); //Start timer

  pthread_detach(pthread_self());
  if ((len = recv(iSockId, buffer, sizeof(buffer), 0)) < 0)//failed recv
  {
    cout << "ERROR: recv() failed for " << pthread_self() << endl;
    pthread_exit((void *)1);
  }

  do
  {
     buffer[len] = 0;
     string s(buffer), token;
     stringstream ss(s);
     vector<string> token_list;
     for(int i = 0; i < 3 && ss; i++)
     {
        ss >> token;
        token_list.push_back(token);
     }

     if (token_list.size() == 3 && token_list[0] == "GET" && (token_list[2] == "HTTP/1.0" || token_list[2] == "HTTP/1.1"))
     {
        string filename;

        if (!(token_list[1].compare("/")))
           filename = sRootPath + token_list[1] + "index.html";
        else
           filename = sRootPath + token_list[1];

        ifstream ifs;
        ifs.open(filename.c_str(), std::fstream::in | std::fstream::binary);
        if (ifs.is_open()) //If file Exists
        {
           ext f_ext = get_ext(filename.c_str());
           struct stat fileStat;
           stat(filename.c_str(), &fileStat);

           if (fileStat.st_mode & S_IROTH)  //If file has read permissions
           {
              strcpy (out_buf, token_list[2].c_str());
              strcat (out_buf, STAT_HTTP_200);

              time_t t = time(NULL);
              char dt[100];
              strftime(dt, sizeof(dt), "%c", localtime(&t));
              stringstream p;
              p << dt;
              sTemp1 = "Date: " + p.str() + "\n";
              strcat (out_buf, sTemp1.c_str());

              if (f_ext == txt ||  f_ext==html){
                 strcat (out_buf, TXT_HTML);
              }
              else if (f_ext==jpg) {
                 strcat(out_buf,JPG );
              }
              else if (f_ext==jpeg){
                 strcat(out_buf,JPEG);
              }
              else if (f_ext == gif) {strcat(out_buf,GIF);}
              else {
                 strcat (out_buf, TXT_HTML);
              }

              if (token_list[2] == "HTTP/1.1")
              {
                 ss.str("");
                 ss << DELAY;
                 strcat(out_buf, "Connection: Keep-Alive\nKeep-Alive: timeout=");
                 strcat(out_buf, (ss.str()).c_str());
                 strcat(out_buf, ", max=5\n");
              }

              ss.str("");
              ss << fileStat.st_size;
              sTemp = "Content-Length: " + ss.str() + "\n\n";
              strcat (out_buf, sTemp.c_str());

              send (iSockId, out_buf, strlen(out_buf), 0);
              pFileBuff = new char[fileStat.st_size];
              ifs.read(pFileBuff, fileStat.st_size);
              send(iSockId, pFileBuff, fileStat.st_size, 0);

              delete[] pFileBuff;
              ifs.close();

              if (token_list[2] == "HTTP/1.1")
              {
                 bIsPersistent = true;
								 //startTime= clock();
              }
           }
           else //No Read permission-Forbidden for access
           {
              strcpy (out_buf, token_list[2].c_str());
              strcat (out_buf, STAT_HTTP_403);

              time_t t = time(NULL);
              char dt[100];
              strftime(dt, sizeof(dt), "%c", localtime(&t));
              stringstream p;
              p << dt;
              sTemp1 = "Date: " + p.str() + "\n";
              strcat(out_buf, sTemp1.c_str());
              strcat(out_buf, "Connection: keep-Alive\nContent-Type: text/html\nContent-Length: ");
              ss.str("");
              ss << strlen(MSG_403) - 2;
              strcat(out_buf, (ss.str()).c_str());
              strcat(out_buf, MSG_403);
              send(iSockId, out_buf, strlen(out_buf), 0);
           }
        }
        else // File Not found
        {
           strcpy (out_buf, token_list[2].c_str());
           strcat (out_buf, STAT_HTTP_404);

           time_t t = time(NULL);
           char dt[100];
           strftime(dt, sizeof(dt), "%c", localtime(&t));
           stringstream p;
           p << dt;
           sTemp1 = "Date: " + p.str() + "\n";
           strcat(out_buf, sTemp1.c_str());
           strcat(out_buf, "Connection: keep-Alive\nContent-Type: text/html\nContent-Length: ");
           ss.str("");
           ss << strlen(MSG_404) - 2;
           strcat(out_buf, (ss.str()).c_str());
           strcat(out_buf, MSG_404);
           send(iSockId, out_buf, strlen(out_buf), 0);
        }
     }
     else //Bad Request
     {
        strcpy (out_buf, token_list[2].c_str());
        strcat (out_buf, STAT_HTTP_400);

        time_t t = time(NULL);
        char dt[100];
        strftime(dt, sizeof(dt), "%c", localtime(&t));
        stringstream p;
        p << dt;
        sTemp1 = "Date: " + p.str() + "\n";
        strcat(out_buf, sTemp1.c_str());
        strcat(out_buf, "Connection: keep-Alive\nContent-Type: text/html\nContent-Length: ");
        ss.str("");
        ss << strlen(MSG_400) - 2;
        strcat(out_buf, (ss.str()).c_str());
        strcat(out_buf, MSG_400);
        send (iSockId, out_buf, strlen(out_buf), 0);
     }
  }while ((bIsPersistent) &&
          (((clock() - startTime)/CLOCKS_PER_SEC)<= DELAY)&&((len = recv(iSockId, buffer, sizeof(buffer), 0)) > 0));

  pthread_mutex_lock (&mutexsum);
  close(iSockId);
  iActiveThreads--;
  pthread_mutex_unlock (&mutexsum);
  pthread_exit((void *)0);
}

int main(int argc, char *argv[])
{
  int iServerSocket, iNewSocket, iTcpPort, iClientAddLen,rc,iOpt = 1;
  struct sockaddr_in stServerAdd, stClientAdd;
  pthread_t *threadId;
  int iThreadState;
  pthread_t *DelThread;
  DELAY=DEFAULT;
  if (argc != 3)
  {
    cout << "Invalid number of arguments\n"
    "usage: " << argv[0] << " <Root Path> <Port>" << endl;
    exit(1);
  }

  iTcpPort = atoi(argv[2]);
  if (iTcpPort < 8000 || iTcpPort > 9999)
  {
    cout << "Port value exceeding the range 8000 to 9999\n";
    exit(1);
  }

  sRootPath = argv[1];
    //if (*sRootPath.rbegin() != '/') /* Keeping this '/' at the end of root path if not passsed in argument */
        //sRootPath.push_back('/');
  struct stat* pstTemp = new struct stat; /* stat struct is used to check the status of path */
  if ((stat(sRootPath.c_str(), pstTemp) < 0) && !(S_ISDIR(pstTemp->st_mode)))
  {
    cout << "Direcory '" << sRootPath << "' does not exist\n";
    exit(1);
  }
  delete pstTemp; /* No longer needed */

    /* Creating Socket File Descriptor */
  if ((iServerSocket = socket(AF_INET,     /* AF_INET - IPv4, DARPA Internet addresses communication domain */
                                SOCK_STREAM, /* SOCK_STREAM - Support for TCP internet transport protocol */
                                0)) < 0)     /* 0 - Automatically takes default protocol for that address family and socket type
                                                -1 is returned on Failure */
  {
    perror("socket failed");
    exit(1);
  }

  stServerAdd.sin_family = AF_INET;
  stServerAdd.sin_addr.s_addr = INADDR_ANY; /* Local Host address is assigned */
  stServerAdd.sin_port = htons(iTcpPort);
  if (setsockopt(iServerSocket, SOL_SOCKET, SO_REUSEADDR, &iOpt, sizeof(int)) == -1) /* Set socket option to avoid address already in use error while binding */
  {
    perror("setsockopt failed");
    exit(1);
  }

    /* Bind socket to a port and address in structure stServerAdd */
  if (bind(iServerSocket, (struct sockaddr *)&stServerAdd, sizeof(stServerAdd)) < 0)
  {
    perror("bind failed");
    exit(1);
  }

    /* Listen on socket */
  if (listen(iServerSocket, SOMAXCONN) < 0) /* SOMAXCONN - parameter queuelen specifies the maximum number of queued connections
                                                 that will be allowed. The maximum queue length is currently limited by the system to 5.
                                                 The manifest constant SOMAXCONN from <sys/socket.h> defines this maximum */
  {
    perror("listen failed");
    exit(1);
  }

  while (1)
  {

    iClientAddLen = sizeof(stClientAdd);
    threadId=new pthread_t;
        /* Accept a connection from client. Client's address is stored in structure stClientAdd */
        /* accept blocks if there are no more connection from clients and thus waits infinitely */
        //threads=new thread_info;
    if ((iNewSocket = accept(iServerSocket, (struct sockaddr *)&stClientAdd, (socklen_t*)&iClientAddLen)) < 0)
    {
      perror("accept failed");
      exit(1);
    }

        /* Create a new thread to process a the incoming request from each client */

    pthread_mutex_lock (&mutexsum);
    if(iActiveThreads>1)
    {
		   DELAY=DEFAULT-(int)(2*log10(iActiveThreads)); //(iack/#)<=5
		}
		if (rc = pthread_create(threadId, NULL, handle_request, (void *) (intptr_t)iNewSocket))
    {
      perror("pthread_create failed");
			pthread_mutex_unlock (&mutexsum);      
			continue;
           //exit(1);
    }
    delete threadId;
    iActiveThreads++;
    pthread_mutex_unlock (&mutexsum);
        /* After thread creation, server will be free to accept another socket request */
  }

  pthread_exit(NULL);
}
