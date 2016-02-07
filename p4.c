#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>

int N, b, c, F, B, P, S, T;
struct hostent *host;
struct sockaddr_in *node;
pthread_barrier_t bt;
int loc_time = 0;
int seed = 0;
char myip[20];
int myi;
int *snodelist;
int *cnodelist;
int down = 0;
int *sendlist;
int *lifetime;
int *deadlist;
int *heartbeat;

FILE *bfp;

void * serverFunc ( void *arg )
{
  FILE *fp;
  int i;
  char hostname[500]; 
  char ipaddr[20];

  // find out the hostname of your own machine.
  gethostname (hostname, 500);
  fp = fopen ("endpoints", "a+");
  
  int len;
  int count;

  // find the IP address of your own machine
  host = gethostbyname (hostname);
  
  struct in_addr * addr;
 
  // extract the IP address from the host structure
  addr = ( struct in_addr *) host->h_addr; 
//  printf ("%s", inet_ntoa (*addr));
 
  struct sockaddr_in serverSock, tmp;

  // create a UDP server socket
  int serverSocket = socket (AF_INET, SOCK_DGRAM, 0);

  serverSock.sin_family = AF_INET;
  serverSock.sin_port = 0;
  serverSock.sin_addr.s_addr = inet_addr (inet_ntoa (*addr));
  memset (serverSock.sin_zero, 0, sizeof (serverSock.sin_zero));  

  strcpy (myip, inet_ntoa (*addr));

  /*Bind socket with address struct*/
  bind( serverSocket, (struct sockaddr *) &serverSock, sizeof(serverSock));

  // write the IP and port details into endpoints file
  fwrite (inet_ntoa (*addr), strlen(inet_ntoa (*addr)), 1, fp);

  len = sizeof (tmp);

  // get the socket details we just created and extract the port number from it
  getsockname (serverSocket, &tmp, &len);
  int port = ntohs (tmp.sin_port);

  char portstr[10];


  sprintf (portstr, " %d\n", port);

  // write the port to endpoints file
  fwrite (&portstr[0], strlen (portstr), 1, fp);
  
  fclose (fp);

  char buf[3];

   // We need the barrier to make sure the control doesn't move on to next section of code files before the server has written 
  // its IP and port in endpoints file.
    pthread_barrier_wait (&bt);
  
  int tmp_port; 
  
  // listen for hellos
  while (1)
  {
    recvfrom (serverSocket, buf, 3, 0, (struct sockaddr_in *) &tmp, &len);
//    printf ("%s\n", buf);
    break;  
  }

  fp = fopen ("endpoints", "rb");
  i = 0;

  while ( fp && (fscanf (fp, "%s %d\n", ipaddr, &tmp_port) == 2))
  {

    node[i].sin_family = AF_INET;
    node[i].sin_port = htons (port);
    node[i].sin_addr.s_addr = inet_addr (ipaddr);
    if ((strcmp (ipaddr, myip) == 0) && (tmp_port == port))
      {  
        myi = i;
      }

    memset (node[i].sin_zero, 0, sizeof (node[i].sin_zero));  

    i++;
  }

  fclose (fp);

  int recv_bytes = 0;

  while (1)
  {
 
  //  printf ("inside server while\n");   
    if (deadlist[myi] || down)
      break;

    if (loc_time >= T)
      break;  

    memset (snodelist, 0, 4*N);

 //   printf ("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX nodeID = %d, calling recv\n ", myi);
    recv_bytes =  recvfrom (serverSocket, snodelist, 4*N, 0, (struct sockaddr_in *) &tmp, &len);

    if (recv_bytes != 4*N)
     {
    //   printf ("recv_bytes = %d\n", recv_bytes);
 //      sleep (1);
       continue;
     }

   // printf ("------------myi = %d, bytes recvd : %d-----------------\n", myi, recv_bytes);

    // if you receive a message from your neighburs, increment the hearbeat counter for that node as well make a note of time.
    for (i = 0; i < N; i++)
    {
  //    printf ("myi : %d recv from server: %d\n", myi, snodelist[i]);
      if ((snodelist[i] == 1))
        {
          lifetime[myi] = loc_time;
          lifetime[i] = loc_time;
          heartbeat[i]++;
          
        }
    }

  // if the neighbours time is past more than F, declare the neighbour as dead. 

    for (i = 0; i < N; i++)
      if (((loc_time - lifetime[i]) >= F) && (i != myi))
        deadlist[i] = 1;
  }

  return NULL; 
}

int main( int argc, char *argv[])
{
  N = atoi(argv[1]);      
  b = atoi(argv[2]);      
  c = atoi(argv[3]);      
  F = atoi(argv[4]);      
  B = atoi(argv[5]);      
  P = atoi(argv[6]);      
  S = atoi(argv[7]);    
  T = atoi(argv[8]);    

  snodelist = (int *) calloc (N, sizeof (int));
  deadlist = (int *) calloc (N, sizeof (int));
  lifetime = (int *) calloc (N, sizeof (int));
  cnodelist = (int *) calloc (N, sizeof (int));
  heartbeat = (int *) calloc (N, sizeof (int));

  struct random_data rdbuf1, rdbuf2;
  int rand1, rand2;

 
  int fail = 0;
  bfp = fopen ("bfile", "w+");
  fwrite (&fail, sizeof(int), 1, bfp);
  fclose (bfp);

  int len;
  pthread_t server;
  node = (struct sockaddr_in *) calloc (N, sizeof (struct sockaddr_in));
  pthread_barrier_init (&bt, NULL, 2);

 // printf ("after calloc\n");
 // create new thread for the server
  pthread_create (&server, NULL, serverFunc, NULL );


  FILE *fp;
  fp = fopen ("endpoints", "a+");
  
   
  int i = 0, k = 0;
  int port;
  char ipaddr[20];

  int bytes_sent = 0;

  int client;
  
  // create client socket
  client = socket (AF_INET, SOCK_DGRAM, 0);

  pthread_barrier_wait (&bt);

  // populate the neighbour list from the endpoints file
  while ( fp && (fscanf (fp, "%s %d\n", ipaddr, &port) == 2))
  {
    //printf ("%s %d\n", ipaddr, port);
    node[i].sin_family = AF_INET;
    node[i].sin_port = htons (port);
    node[i].sin_addr.s_addr = inet_addr (ipaddr);
    memset (node[i].sin_zero, 0, sizeof (node[i].sin_zero));  

    i++;
  }

  //  printf (" i = %d\n", i);  
  // if number of nodes == N, this is last node, send OK to everyone else. 
  if (i == N)
  {
//    printf ("%d %d\n", i, N);  
    for (k = 0; k < N; k++)
    {
      len = sizeof (node[k]);
      sendto (client, "OK", 3, 0, (struct sockaddr *)&node[k], len);
    }     
  }
 
  int m;
  int j;

  sleep (1);
  fseek (fp, 0L, SEEK_SET);

  i = 0;

  // copy all the neighbours into node array and identify your own line number. 
  while ( fp && (fscanf (fp, "%s %d\n", ipaddr, &port) == 2))
  {

//    printf ("#################################%s %d\n", ipaddr, port);
    node[i].sin_family = AF_INET;
    node[i].sin_port = htons (port);
    node[i].sin_addr.s_addr = inet_addr (ipaddr);
    memset (node[i].sin_zero, 0, sizeof (node[i].sin_zero));  

    i++;
  }

  fclose (fp);

 //   pthread_barrier_wait (&bt);

 sleep (1);

 // create and initialize the statebuf with seed

 char *randbuf1 = (char *) calloc(32, sizeof(char));
 char *randbuf2 = (char *) calloc(32, sizeof(char));

 memset (&rdbuf1, 0, sizeof (struct random_data));
 memset (&rdbuf2, 0, sizeof (struct random_data));

 //printf ("Using myi now : %d\n", myi);

 initstate_r (S, randbuf1, 32, &rdbuf1);
 initstate_r (S + myi, randbuf2, 32, &rdbuf2);

 srandom_r (S, &rdbuf1);
 srandom_r (S+myi, &rdbuf2);

 // printf ("after using myi\n");

 while (loc_time <= T)
 {
 
  if (loc_time >= T)
    break;  

  
  for (k = 0; k < c; k++)
    {
  
      sleep (1);
      loc_time ++;

  
  
    if (loc_time >= T)
      break;  

      if (down)
        break;

      int inp;

        // if the local time mod P is 0, its time to fail a node.
      if ((loc_time % P) == 0)
      {
         bfp = fopen ("bfile", "r+");
         
         random_r (&rdbuf1, &rand1);

         rand1 = rand1 % N;

 //        printf ("rand1 = %d, myi = %d\n", rand1, myi);

         double mesleep;
         if (rand1 == myi)
           {
             mesleep = rand1/10.0;
             sleep (mesleep);
             if ((fread (&fail, sizeof(int), 1, bfp) == 1) && (fail < B))
               {
  //               printf ("fail : %d\n", fail);                 
                 down = 1;
                 fail++;

                 deadlist[myi] = 1;
                 fseek (bfp, 0L, SEEK_SET);
                 fwrite (&fail, sizeof(int), 1, bfp);
                 fclose (bfp);
                 break;
               }
           }
        fclose (bfp); 
      }
    memset (cnodelist, 0, 4*N);

 //   printf ("--------reached_here------------\n");
    int max_tries = b * 100;

    int z = 0;

    // pick b nodes randomly and send heartbeats
    for (j = 0; (j < b) && (j < (N - fail));)
      {
        
        if (loc_time >= T)
          break;

        z++;

        if (z == max_tries)
          break;

        random_r (&rdbuf2, &rand2);

        rand2 = rand2 % N;

        if ((rand2 != myi) && (deadlist[rand2] == 0) && (cnodelist[rand2] == 0))
          {
   //         printf ("Setting neighbours : rand2 = %d, myi = %d\n", rand2, myi);

            cnodelist[rand2] = 1;
            j++;     
          }
       }

    
    if (z == max_tries)
      break;
    
    cnodelist[myi] = 1;

    int client1;

    sleep (1);
    for (m = 0; m < N; m++)
      if ((cnodelist[m] == 1) && (m != myi))
        {
          client1 = socket (AF_INET, SOCK_DGRAM, 0); 

          len = sizeof (node[m]);
    //      bytes_sent =  sendto (client1, "OK", 3, 0, (struct sockaddr *)&node[m], len);
    //      printf ("Sending to neighbours1 : myi %d, bytes sent %d, sendto ID : %d\n", myi, bytes_sent, m );
          bytes_sent =  sendto (client1, cnodelist, 4*N, 0, (struct sockaddr *)&node[m], len);
   //       printf ("Sending to neighbours : myi %d, bytes sent %d, sendto ID : %d\n", myi, bytes_sent, m );
        }
    memset (cnodelist, 0, 4*N);


    }
//    if (loc_time >= T)
 //     break;  
     // sleep (1);
    //  loc_time ++;
 
  } 
 // fclose (fp);


// end of program, 
// dump the neighbourlist contents to listX file and return

  char opname[20];
  char opline[50];

  myi;

  sprintf (opname, "%s%d", "list", myi+1);
//  printf ("#####################################opname : %s\n", opname);

  FILE *ofp;
  ofp = fopen (opname, "w+");
  
  char okay[] = "OK\n";
  char strfail[] = "FAIL\n";

//  printf ("NodeId %d\n");

//  for (k = 0; k < N; k++)
  //  printf ("Nodeid:%d Neighbour:%d State:%d\n", myi, k, deadlist[k]);

  if (deadlist[myi])
    fwrite (strfail, strlen(strfail), 1, ofp);
  else
    fwrite (okay, strlen (okay), 1, ofp);

  for (i = 0; i < N; i++)
   {
     if (i != myi)
     {  
       sprintf (opline, "%d %d\n", i+1, heartbeat[i]);
       fwrite (opline, strlen(opline), 1, ofp); 
     }
 
   }  

  fclose (ofp);
  return 1; 
}
