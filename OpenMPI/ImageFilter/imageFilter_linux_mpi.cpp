// Compile:
// mpic++ imageFilter_linux_mpi.cpp -o imageFilter_linux_mpi

// Run:
// time mpirun -np 8 --hostfile /home/mpi/mpi_hosts /home/mpi/src/OpenMPI/ImageFilter/imageFilter_linux_mpi

//
// Video processing example using FFmpeg
// Written by Ted Burke - last updated 12-2-2017
//
 
#include <stdio.h>
#include <stdlib.h>
// #include <mpi.h>
 
// Video resolution
#define W 1280
#define H 720
#define F 820
 
// Allocate a buffer to store one frame
unsigned char *frame;
unsigned char *frames[F];
 
int main() {
    int a = 0, x, y, count;
   //  MPI_Comm comm;
     
    // Open an input pipe from ffmpeg and an output pipe to a second instance of ffmpeg
    FILE *pipein = popen("ffmpeg -i teapot_input.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "r");
    FILE *pipeout = popen("ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 25 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 teapot_output.mp4", "w");
     
   
    // Process video frames
    while(1)
    {
      frame = (unsigned char*) malloc(H * W * 3);

      // Read a frame from the input pipe into the buffer
      count = fread(frame, 1, H * W * 3, pipein);
 
      // If we didn't get a frame of video, we're probably at the end
      if (count != H * W * 3) break;
      frames[a++] = frame;
    }

    for (int i = 0; i < a; i++) {
       fwrite(frames[i], 1, H * W * 3, pipeout);
    }
     
    // Flush and close input and output pipes
    fflush(pipein);
    pclose(pipein);
    fflush(pipeout);
    pclose(pipeout);
    return 0;
}