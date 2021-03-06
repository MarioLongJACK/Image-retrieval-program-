#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <math.h>
#include <float.h>
#include "worker.h"


/*
 * Read an image from a file and create a corresponding 
 * image struct 
 */

Image* read_image(char *filename)
{   
        //Create and allocate space for the return image
        Image *img;
        img = malloc(sizeof(Image));
        //Set a buffer for checking the magic P3
        char buff[10];
        //Create vars for storing width height and maximum value of image
        int width;
        int height;
        int maxVal;
        int maxSize;
        // Create vars for red blue green values of each pixel
        int r;
        int g;
        int b;
        // Open the file being processed
        FILE *f = fopen(filename, "r");
        // Check if the starting image text is P3 to ensure we're reading the right file else just return NULL
        fscanf(f,"%s", buff);
        if(strcmp(buff,"P3") != 0){
            return NULL;
        }
        //Grab the width height and maximum value and store to created vars
        fscanf(f, "%d %d %d", &width, &height, &maxVal);
        //Get total number of pixels and store into maximum size var
        maxSize = width*height;
        //Allocate space for the Pixel Array to store individual pixels into
        Pixel *pArray = malloc(sizeof(Pixel)*maxSize);
        //Loop Through each pixel and create Pixel struct for each and store into array
        for(int i = 0; i < maxSize; i++){
            fscanf(f, "%d %d %d", &r, &g, &b);
            Pixel pixel;
            pixel.red = r;
            pixel.green = g;
            pixel.blue = b;
            pArray[i] = pixel;

        }
        //Store needed values to the image struct
        img->width = width;
        img->height = height;
        img->max_value = maxVal;
        img->p = pArray;
        // Properly close opened file
        fclose(f);
        //Return image
        return img;
}

/*
 * Print an image based on the provided Image struct 
 */

void print_image(Image *img){
        printf("P3\n");
        printf("%d %d\n", img->width, img->height);
        printf("%d\n", img->max_value);
       
        for(int i=0; i<img->width*img->height; i++)
           printf("%d %d %d  ", img->p[i].red, img->p[i].green, img->p[i].blue);
        printf("\n");
}

/*
 * Compute the Euclidian distance between two pixels 
 */
float eucl_distance (Pixel p1, Pixel p2) {
        return sqrt( pow(p1.red - p2.red,2 ) + pow( p1.blue - p2.blue, 2) + pow(p1.green - p2.green, 2));
}

/*
 * Compute the average Euclidian distance between the pixels 
 * in the image provided by img1 and the image contained in
 * the file filename
 */

float compare_images(Image *img1, char *filename) {
        //read the given file
        Image *img2 = read_image(filename);
        //create vars for calculation
        float average;
        float total = 0;
        int maxPix = img1->width*img1->height;
        if(img2==NULL){
            average =  -1;
            return average;
        }
        //loop through each pixel and calculate euclidian distance and get total
        for(int i = 0; i < maxPix; i++){
            total = total+eucl_distance(img1->p[i],img2->p[i]);
        }
        //get average distance
        average = total/maxPix;
        //return average
        return average;
}

/* process all files in one directory and find most similar image among them
* - open the directory and find all files in it 
* - for each file read the image in it 
* - compare the image read to the image passed as parameter 
* - keep track of the image that is most similar 
* - write a struct CompRecord with the info for the most similar image to out_fd
*/
CompRecord process_dir(char *dirname, Image *img, int out_fd){
    // initialize the path of sub directory 
    char path[PATHLENGTH];
    char temp_path[PATHLENGTH] = "";
    DIR *dirp;
    // initialize the max_distance in the directory
    float max_distance = FLT_MAX;
    float temp = FLT_MAX;
    // if open sub directory fails, exit with error report
    if((dirp = opendir(dirname)) == NULL) {
        perror("opendir");
        exit(1);
    }
    // loop through each file in the sub directory
    struct dirent *dp;
        CompRecord CRec;
    while((dp = readdir(dirp)) != NULL){
        // strncpy the path of the file to the root 
        strncpy(path, dirname, PATHLENGTH);
        strncat(path, "/", PATHLENGTH - strlen(path) - 1);
        strncat(path, dp->d_name, PATHLENGTH - strlen(path) - 1);
        // check if the path is correct
        struct stat sbuf;
        if(stat(path, &sbuf) == -1){
        perror("stat");
        exit(1);
        }
        // check if the file is a directory or it is regular file
        if((S_ISDIR(sbuf.st_mode)) != 1) {
                // if it is a regular file, call compare_image 
                temp = compare_images(img, path);
                // let max_distance stores the smallest distance
                if(temp != -1 && temp < max_distance){
                    max_distance = temp;
            strcpy(temp_path, path);
                }
        }
                
        }
        // write the file name and max_distance to the return result
        strcpy(CRec.filename, temp_path);
        CRec.distance = max_distance;
	// write the result to the pipe
	write(out_fd, &CRec, sizeof(CompRecord));
         
        return CRec;
}
