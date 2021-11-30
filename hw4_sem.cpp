#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

#include <ctime>
#include <pthread.h>
#include <semaphore.h>
using namespace std;

#define NSmooth 1000//smooth time

BMPHEADER bmpHeader;//BMP file header
BMPINFO bmpInfo;// BMP file info
RGBTRIPLE **BMPSaveData = NULL;//Pixel going to be save
RGBTRIPLE **BMPData = NULL;//pixel temp

int readBMP( char *fileName);//Read BMP file and save data in "BMPSaveData"
int saveBMP( char *fileName);//write BMP file, content is BMP
void swap(RGBTRIPLE *a, RGBTRIPLE *b);//swap two RGBTRIPLE content
RGBTRIPLE **alloc_memory( int Y, int X );//allocate a Y * X matrix

int total_thread;//total # of thread
int counter;
//semaphore
sem_t count_sem;
sem_t barrier[2];

//thread entry
void* thread_routine(void* param)
{
    int id = (long) param;//thread_id
    int remainder = bmpInfo.biHeight%total_thread;//variable that help correct calculate size
    int size = bmpInfo.biHeight/total_thread;//# of line each thread should calculate
    int start_pos;//position start calculate
    if(id >= remainder)
        start_pos = remainder*(size + 1) + (id - remainder)*(size);
    else
    {
        start_pos = id*(size + 1);
        size++;
    }

    //smooth
    for(int count = 0; count < NSmooth ; count ++)
    {
        //change BMPSaveData and BMPData
        //barriers with semaphores
        sem_wait(&count_sem);
        if(counter == total_thread-1)
        {
            swap(BMPSaveData, BMPData);
            counter = 0;
            sem_post(&count_sem);
            for(int k=0;k<total_thread-1; k++)
                sem_post(&barrier[count%2]);//when one is release, the other will open 
        }
        else
        {
            counter++;
            sem_post(&count_sem);
            sem_wait(&barrier[count%2]);
        }
        //doing smooth
        for(int i = start_pos; i < start_pos + size ; i++)
        {
            for(int j =0; j<bmpInfo.biWidth ; j++)
            {
                //setting Top, Down, Left, Right location
                int Top = i>0 ? i-1 : bmpInfo.biHeight-1;
                int Down = i<bmpInfo.biHeight-1 ? i+1 : 0;
                int Left = j>0 ? j-1 : bmpInfo.biWidth-1;
                int Right = j<bmpInfo.biWidth-1 ? j+1 : 0;
                //average pixel
                BMPSaveData[i][j].rgbBlue =  (double) (BMPData[i][j].rgbBlue+BMPData[Top][j].rgbBlue+BMPData[Top][Left].rgbBlue+BMPData[Top][Right].rgbBlue+BMPData[Down][j].rgbBlue+BMPData[Down][Left].rgbBlue+BMPData[Down][Right].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/9+0.5;
                BMPSaveData[i][j].rgbGreen =  (double) (BMPData[i][j].rgbGreen+BMPData[Top][j].rgbGreen+BMPData[Top][Left].rgbGreen+BMPData[Top][Right].rgbGreen+BMPData[Down][j].rgbGreen+BMPData[Down][Left].rgbGreen+BMPData[Down][Right].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/9+0.5;
                BMPSaveData[i][j].rgbRed =  (double) (BMPData[i][j].rgbRed+BMPData[Top][j].rgbRed+BMPData[Top][Left].rgbRed+BMPData[Top][Right].rgbRed+BMPData[Down][j].rgbRed+BMPData[Down][Left].rgbRed+BMPData[Down][Right].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/9+0.5;
            }
        }
        //Barrier
    }
    return NULL;
}

int main(int argc,char *argv[])
{

    char *infileName = "input.bmp";//input filename
    char *outfileName = "output7.bmp";//output filename
    struct timespec starttime, endtime;//start and endtime, use clock_gettime()
    double total;//total execution time
    int thread_id;//thread counter

    //record starttime
    clock_gettime(CLOCK_MONOTONIC, &starttime);

    //readfile
    if ( readBMP( infileName) )
        cout << "Read file successfully!!" << endl;
    else
        cout << "Read file fails!!" << endl;

    //allocate memory to BMPData and initialize BMPData
    BMPData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);

    //thread initial & semaphore setting
    pthread_t* thread_handles;
    total_thread = strtol(argv[1], NULL, 10);
    thread_handles = (pthread_t *) malloc(total_thread * sizeof(pthread_t));
    counter = 0;
    sem_init(&count_sem, 0, 1);
    sem_init(&barrier[0], 0, 0);//semaphore = 0 represent that it is lock in the beginning
    sem_init(&barrier[1], 0, 0);

    //initial barrier and create thread
    for(thread_id=0; thread_id < total_thread; thread_id++)
    {
        pthread_create(&thread_handles[thread_id], NULL, thread_routine, (void *)thread_id);
    }

    //wait thread finished
    for(thread_id=0; thread_id < total_thread; thread_id++)
        pthread_join(thread_handles[thread_id], NULL);

    //save file
    if ( saveBMP( outfileName ) )
        cout << "Save file successfully!!" << endl;
    else
        cout << "Save file fails!!" << endl;

    //get endtime and print
    clock_gettime(CLOCK_MONOTONIC, &endtime);
    total = (endtime.tv_sec - starttime.tv_sec);
    cout<<"We have "<<total_thread<<" thread, the execution time = "<< total <<" s"<<endl;

    //free memory
    sem_destroy(&barrier[1]);
    sem_destroy(&barrier[0]);
    sem_destroy(&count_sem);
    free(BMPData[0]);
    free(BMPSaveData[0]);
    free(BMPData);
    free(BMPSaveData);

    return 0;
}

//read BMP file
int readBMP(char *fileName)
{
    //build input object
    ifstream bmpFile( fileName, ios::in | ios::binary );

    //if file can't open
    if ( !bmpFile )
    {
        cout << "It can't open file!!" << endl;
        return 0;
    }

    //read BMP header file
    bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );

    //check if it is BMP file
    if( bmpHeader.bfType != 0x4d42 )
    {
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }

    //read BMP info
    bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );

    //check bit depth is right
    if ( bmpInfo.biBitCount != 24 )
    {
        cout << "The file is not 24 bits!!" << endl;
        return 0;
    }

    //modified image file can modified 4
    while( bmpInfo.biWidth % 4 != 0 )
        bmpInfo.biWidth++;

    //allocate memory
    BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);

    //read pixel file
    bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);

    //close file
    bmpFile.close();

    return 1;
}
//save picture
int saveBMP( char *fileName)
{
    //check if it is BMP file
    if( bmpHeader.bfType != 0x4d42 )
    {
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }

    //build output file object
    ofstream newFile( fileName,  ios:: out | ios::binary );
    //if file can't build
    if ( !newFile )
    {
        cout << "The File can't create!!" << endl;
        return 0;
    }

    //input BMP file header
    newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

    //write BMP file info
    newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

    //write pixel
    newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

    //write input file
    newFile.close();

    return 1;
}


//allocate memory
RGBTRIPLE **alloc_memory(int Y, int X )
{
    //build array's length is y
    RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
    RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
    memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
    memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

    //for each y array build a array lenth's X
    for( int i = 0; i < Y; i++)
    {
        temp[ i ] = &temp2[i*X];
    }
    return temp;
}
//change two RGBTRIPLE pointer
void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
    RGBTRIPLE *temp;
    temp = a;
    a = b;
    b = temp;
    return;
}