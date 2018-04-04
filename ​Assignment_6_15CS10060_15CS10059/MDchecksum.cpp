#include <stdio.h>
#include <iostream>
#include <openssl/md5.h>
#include <string.h>
#define BUFSIZE 1024

using namespace std;


void MD5_checksum(char *filename,char *output)
{
    int n;
    MD5_CTX c;
    char buf[BUFSIZE];
    char temp[7];
    ssize_t bytes;
    unsigned char out[MD5_DIGEST_LENGTH];
    FILE *fp  = fopen(filename,"r");
    MD5_Init(&c);
    bytes=fread(buf,1, BUFSIZE, fp);
    while(!feof(fp))
    {
        MD5_Update(&c, buf, bytes);
        bytes=fread(buf,1, BUFSIZE, fp);
    }

    MD5_Final(out, &c);

    for(n=0; n<MD5_DIGEST_LENGTH; n++)
    {
        snprintf(temp, 6, "%02x", out[n]);
        strcat(output,temp);
    }
    printf("\n");
    fclose(fp);
}









int main()
{
	char client[128],server[128],output1[128],output2[128];
	strcpy(client,"abc.mp4");
	strcpy(server,"out.mp4");
	MD5_checksum(client,output1);
	cout<<"\n"<<output1;
    strcpy(output2,"");
	MD5_checksum(server,output2);
    cout<<"\n"<<output2;
    if(strcmp(output1,output2) == 0)
        cout<<"\nMD5 matched";
    else
        cout<<"\nMD5 did not match";
	

}
