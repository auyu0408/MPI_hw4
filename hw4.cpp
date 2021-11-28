#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

#include <ctime>
#include <pthread.h>
using namespace std;

#define NSmooth 1000//smooth time

/*
Variable：
bmpHeader    ： BMP file header
bmpInfo      ： BMP file info
**BMPSaveData： The pixel going to be save儲存要被寫入的像素資料
**BMPData    ： pixel temp
*/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;                                               
RGBTRIPLE **BMPData = NULL;                                                   

/*
function：
readBMP    ： Read BMP file and save data in "BMPSaveData"
saveBMP    ： write BMP file, content is BMP
swap       ： swap two RGBTRIPLE content
**alloc_memory： allocate a Y * X matrix
*/
int readBMP( char *fileName);//read file
int saveBMP( char *fileName);//save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X );//allocate memory

int total_thread;//total # of thread
int remainder;
int *cal_size, *cal_start, *barrier_counter, *swap_counter;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* thread_routine(void* param)
{
    int id = (long) param;
    //calculate smooth
    for(int count = 0; count < NSmooth ; count ++){
        //change BMPSaveData and BMPData
        if(id == 0)
        {
            swap(BMPSaveData,BMPData);
            pthread_mutex_lock(&mutex);
            swap_counter[count]++;
            pthread_mutex_unlock(&mutex);
        }
        else
        {
            pthread_mutex_lock(&mutex);
            swap_counter[count]++;
            pthread_mutex_unlock(&mutex);
        }
        while(swap_counter[count] < total_thread);
        //doing smooth
        for(int i = cal_start[id]; i < cal_start[id] + cal_size[id] ; i++)
        {
            for(int j =0; j<bmpInfo.biWidth ; j++){
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
        pthread_mutex_lock(&mutex);
        barrier_counter[count]++;
        pthread_mutex_unlock(&mutex);
        while(barrier_counter[count] < total_thread);
    }
    return NULL;
}

int main(int argc,char *argv[])
{
/*
Variable：
*infileName  ： input filename
*outfileName ： output filename
startwtime   ： start time
endwtime     ： end time
*/
    char *infileName = "input.bmp";
    char *outfileName = "output.bmp";
    double startwtime = 0.0, endwtime=0;
    int thread_id;//count in open and join all thread

    //record starttime
    startwtime = clock();

    //readfile
    if ( readBMP( infileName) )
        cout << "Read file successfully!!" << endl;
    else 
        cout << "Read file fails!!" << endl;

    //allocate memory to BMPData
    BMPData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
    
    //thread initial
    pthread_t* thread_handles;
    total_thread = strtol(argv[1], NULL, 10);
    remainder = bmpInfo.biHeight % total_thread;
    thread_handles = (pthread_t *) malloc(total_thread * sizeof(pthread_t));
    //calculate start point
    cal_size = (int *)malloc( total_thread * sizeof(int));
    cal_start = (int *)malloc( total_thread * sizeof(int));
    cal_start[0] = 0;
    for(thread_id = 0; thread_id < total_thread; thread_id ++)
    {
        cal_size[thread_id] = bmpInfo.biHeight / total_thread;
        if(thread_id < remainder)
            cal_size[thread_id]++;
    }
    for(thread_id = 1; thread_id < total_thread; thread_id ++)
    {
        cal_start[thread_id] = cal_start[thread_id-1] + cal_size[thread_id-1]; 
    }
    //initial counter
    barrier_counter = (int *)malloc(NSmooth * sizeof(int));
    swap_counter = (int *)malloc(NSmooth * sizeof(int));
    for(int i = 0; i<NSmooth; i++)
    {
        barrier_counter[i] = 0;
        swap_counter[i] = 0;
    }
    //create thread
    for(thread_id = 0; thread_id < total_thread; thread_id++)
        pthread_create(&thread_handles[thread_id], NULL, thread_routine, (void *)thread_id);
    //wait all thread end
    for(thread_id=0; thread_id < total_thread; thread_id++)
        pthread_join(thread_handles[thread_id], NULL);
    //save file
    if ( saveBMP( outfileName ) )
        cout << "Save file successfully!!" << endl;
    else
        cout << "Save file fails!!" << endl;

    //get endtime and print
    endwtime = clock();;
    cout<<"We have "<<total_thread<<" thread"<<endl;
    cout << "The execution time = "<< (endwtime-startwtime)/CLOCKS_PER_SEC <<" s"<<endl;
    //free memory
    free(BMPData[0]);
    free(BMPSaveData[0]);
    free(BMPData);
    free(BMPSaveData);
    free(cal_size);
    free(cal_start);
    free(barrier_counter);
    free(swap_counter);

    return 0;
}

/*
read BMP file
*/
int readBMP(char *fileName)
{
    //build input object	
    ifstream bmpFile( fileName, ios::in | ios::binary );

    //if file can't open
    if ( !bmpFile ){
        cout << "It can't open file!!" << endl;
        return 0;
    }

    //read BMP header file
    bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );

    //check if it is BMP file
    if( bmpHeader.bfType != 0x4d42 ){
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }

    //read BMP info
    bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );
    
    //check bit depth is right
    if ( bmpInfo.biBitCount != 24 ){
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
/*
儲存圖檔
*/
int saveBMP( char *fileName)
{
    //check if it is BMP file
    if( bmpHeader.bfType != 0x4d42 ){
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }

    //build output file object
    ofstream newFile( fileName,  ios:: out | ios::binary );
    //if file can't build
    if ( !newFile ){
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


/*
allocate memory
*/
RGBTRIPLE **alloc_memory(int Y, int X )
{        
    //build array's length is y
    RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
    RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
    memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
    memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

    //for each y array build a array lenth's X
    for( int i = 0; i < Y; i++){
        temp[ i ] = &temp2[i*X];
    }
    return temp;
}
/*
change two RGBTRIPLE pointer
*/
void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
    RGBTRIPLE *temp;
    temp = a;
    a = b;
    b = temp;
    return;
}