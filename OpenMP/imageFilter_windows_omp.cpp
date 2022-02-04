// compile: gcc imageFilter_windows_omp.c -o imageFilter_windows_omp -fopenmp
// run: ./imageFilter_windows_omp.exe

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

// Video resolution
#define W 1280
#define H 720
#define F 820

unsigned char frame[64][H][W][3];

int main (int argc, char *argv[]) {
   int nThreads;
   if(argc > 0) { nThreads = atoi(argv[1]); }
   else { printf("\nPor favor ingrese como parametro el numero de hilos.\n"); return 1; }
   omp_set_num_threads(nThreads);
   double time = omp_get_wtime();

   #pragma omp parallel
   {
      int iam = omp_get_thread_num();
      const int tasks = omp_get_num_threads();
      
      int a = 0, count;
      int nFrames = F / tasks;
      int offset = iam * nFrames;
      int endFrame = F;
      char commandIn[78], commandOut[130];

      sprintf(
         commandOut,
         "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 25 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 teapot_output%i.mp4",
         iam
      );

      FILE* pipein = _popen("ffmpeg -i teapot_input.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "rb+");
      FILE* pipeout = _popen(commandOut, "wb+");

      // Determina el número de frames a modificar de cada proceso.
      if (iam != tasks - 1) { endFrame = nFrames + offset; }
      
      while (1) {
         // Read a frame from the input pipe into the buffer
         count = fread(frame[iam], 1, H * W * 3, pipein);

         // If we didn't get a frame of video, we're probably at the end
         if (count != H * W * 3) break;

         // Write this frame to the output pipe
         if (a >= offset && a < endFrame) {
            // Process this frame
            for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
               // Invert each colour component in every pixel
               frame[iam][y][x][0] = 255 - frame[iam][y][x][0]; // red
               frame[iam][y][x][1] = 255 - frame[iam][y][x][1]; // green
               frame[iam][y][x][2] = 255 - frame[iam][y][x][2]; // blue
            }

            fwrite(frame[iam], 1, H * W * 3, pipeout);
         }

         a++;
      }

      fflush(pipein); _pclose(pipein);
      fflush(pipeout); _pclose(pipeout);
      #pragma omp barrier

      if(iam == 0 && tasks > 1) {
         pipeout = _popen("ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 25 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 teapot_output.mp4", "wb+");

         for (int i = 0; i < tasks; i++) {
            sprintf(
               commandIn,
               "ffmpeg -i teapot_output%i.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -",
               i
            );

            pipein = _popen(commandIn, "rb+");

            while (1) {
               // Read a frame from the input pipe into the buffer
               count = fread(frame[iam], 1, H * W * 3, pipein);

               // If we didn't get a frame of video, we're probably at the end
               if (count != H * W * 3) break;

               fwrite(frame[iam], 1, H * W * 3, pipeout);
            }

            fflush(pipein); _pclose(pipein);
         }

         fflush(pipeout); _pclose(pipeout);
      }
   }
   
   printf("\nelapse time: %f seconds\n ", omp_get_wtime() - time);

   return 0;
}