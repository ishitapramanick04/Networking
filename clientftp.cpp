#include <iostream>
#include <string>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <fstream>
#include<bits/stdc++.h>
#include<sys/time.h>
#include <time.h>
using namespace std;

#define LEN 516
#define port 69

#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERR 5

static const char* mode="octet"; //binary mode

struct sockaddr_in sendSockAddr;
int clientSd;
socklen_t from_length;
struct sockaddr_in from;

void receive_file(string filename)
{
  FILE *fp;
  int size = filename.length();
  char fname[size+1];
  strcpy(fname, filename.c_str());
	fp = fopen(fname, "wb+");
  if(fp==NULL)
  {
    cout<<"not opened"<<endl;
    exit(1);
  }
  unsigned char msg[LEN],arr[LEN];
  // read request
  bzero(msg,LEN);
  // format: opcode | filename | 0 | mode | 0
  //short op=(short)1;
  msg[0]=0;
	msg[1]=RRQ; //the RRQ opcode
  memcpy(msg+2,fname,strlen(fname));
  memcpy(msg+2+size+1,mode,strlen(mode));
  int tots=2+size+1+strlen(mode)+1;
  int n=sendto(clientSd,msg,tots,0,(struct sockaddr *)&sendSockAddr,from_length);
    if(n<0)
    {
        //perror("send");
        cout<<"error in sending"<<endl;
    }
  int blockno=0;
    while(1)
  {
    //bzero((char*)&from, sizeof(from));

    //receiving
    bzero(arr,LEN);
    n = recvfrom(clientSd,arr,LEN,0,(struct sockaddr *)&from,&from_length);
    if(n<0)
    {
        cout<<n<<endl;
        perror("error ");
        //<<"error in receiving"<<endl;
        exit(1);
    }
  //  cout<<(int)arr[1];
    n -= 4;
    if(arr[1]==ERR)
      perror("Error Packet");

    fwrite(&arr[4],1,n,fp);
    cout<<"blocksize: "<<n<<" block no: "<<blockno<<" ";
    //sending acknowledgement
    bzero(msg,LEN);
    msg[0]=0;
		msg[1]=ACK;
		msg[2]=arr[2];
		msg[3]=arr[3];
    //cout<<msg<<endl;
    //cout<<strlen(msg);
    blockno++;
    int m=sendto(clientSd,msg,4,0,(struct sockaddr *)&from,from_length);
      if(m<0)
      {
          cout<<"error in sending"<<endl;
          exit(1);
      }
      cout<<"blockno:"<<blockno<<" m:"<<m<<endl;
      if(n<512)
      break;
  }
  fclose(fp);
  cout<<"*******************got the file*******************"<<endl;

}
void send_file(string filename)
{
  FILE *fp;
  int size = filename.length();
  char fname[size+1];
  strcpy(fname, filename.c_str());
	fp = fopen(fname, "rb+");
  if (fp == NULL)
    {
       cout<<"npot opened"<<endl;
       exit(1);
     }
  unsigned char msg[LEN],arr[LEN];
  bzero(msg,sizeof(msg));
  msg[0]=0;
  msg[1]=WRQ; //the RRQ opcode
  //memcpy(msg,&op,2);
  memcpy(msg+2,fname,strlen(fname));
  memcpy(msg+2+strlen(fname)+1,mode,strlen(mode));
  int tots=2+strlen(fname)+1+strlen(mode)+1;
  int n=sendto(clientSd,msg,tots,0,(struct sockaddr *)&sendSockAddr,from_length);
    if(n<0)
    {
        cout<<"error in sending"<<endl;
    }
  //  cout<<"tots: "<<tots<<"send :"<<n<<endl;
    int blockno=0,blockno2=0;
    while(1)
    {
      bzero(arr,LEN);
      n = recvfrom(clientSd,arr,LEN,0,(struct sockaddr *)&from,&from_length);
      if(n<0)
      {
          //cout<<"error in receiving"<<endl;
          perror("error ");
      }
      blockno=arr[3]+1;
      if(blockno==256)
      {
        blockno2++;
        blockno=0;
      }
      cout<<"arr size: "<<n<<endl;
      bzero(msg,LEN);
      msg[0]=0;
    	msg[1]=DATA;
    	msg[2]=blockno2;
		  msg[3]=blockno;

      //cout<<"size: "<<s;
      //cout<<s;
    /*  char ch=fgetc(fp);
      while(ch!=EOF)
      {
        msg[i]=ch;
        i++;
        ch=fgetc(fp);
        if(i==516)
        break;
      }*/
      int s=fread(msg+4,1,512,fp);
      perror("error");
      //cout<<endl;
    	cout<<"blockno: "<< blockno<<endl<<"size: "<<s<<endl;
      blockno++;
      int m=sendto(clientSd,msg,s+4,0,(struct sockaddr *)&from,from_length);
        if(m<0)
        {
            cout<<"error in sending"<<endl;
            //perror("error ");
        }
        cout<<"m: " <<m<<endl;
    	if(m<512)
    		{
          bzero(arr,LEN);
          n = recvfrom(clientSd,arr,LEN,0,(struct sockaddr *)&from,&from_length);
          if(n<0)
          {
              cout<<"error in receiving"<<endl;
              //perror("error ");
          }
          break;
        }
    }
    fclose(fp);
    cout<<"*******************sent the file*******************"<<endl;
}

int main(int argc,char *argv[])
{
  if(argc < 2)
  {
    cout << "Usage: ip_address " << endl;
    exit(0);
  }
  char *serverIp = argv[1];
  clientSd = socket(AF_INET, SOCK_DGRAM, 0);
  if(clientSd < 0)
  {
      cout<<"Error connecting to socket!"<<endl; exit(0);
  }
  //sockaddr_in from;
  struct hostent* host = gethostbyname(serverIp);
  bzero((char*)&sendSockAddr, sizeof(sendSockAddr));
  sendSockAddr.sin_family = AF_INET;
  sendSockAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
  sendSockAddr.sin_port = htons(port);
  from_length= sizeof(sendSockAddr);

  struct timeval timer;
  timer.tv_sec = 10;
  timer.tv_usec = 0;

  if (setsockopt (clientSd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timer,sizeof(timer)) < 0)
        {
          perror("setsockopt failed\n");
          //exit(1);
        }
  if (setsockopt (clientSd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timer,sizeof(timer)) < 0)
        {
          perror("setsockopt failed\n");
          //exit(1);
        }
  //cout<<"SOL_SOCKET "<<SOL_SOCKET<<endl;
  while(1)
  {
    cout<<"client:";

    string op="",filename="";
    cin>>op;
    if(op=="quit")
    {
      cout<<"ending.."<<endl;
      return 0;
    }
    else if(op=="get")    //receive
    {
      cout<<"enter filename: ";
      cin>>filename;
      cout<<endl;
      receive_file(filename);
    }
    else if(op=="put")     //send
    {
      cout<<"enter filename: ";
      cin>>filename;
      cout<<endl;
      send_file(filename);
    }
    else
    continue;
  }
  return 0;
}
