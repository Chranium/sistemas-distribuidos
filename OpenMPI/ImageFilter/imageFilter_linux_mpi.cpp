// g++ imageFilter_windows.cpp -o imageFilter_windows.exe
// Video processing example using FFmpeg
// Written by Ted Burke - last updated 12-2-2017
//

#include <stdio.h>
#include <stdlib.h>

// Video resolution
#define W 1280
#define H 720
#define F 820

int offset, nFrames, endFrame = F;
unsigned char frame[H][W][3];

int main() {
   int iam = 0;
   int a = 0, count;
   const int tasks = 8;
   nFrames = F / tasks;
   offset = iam * nFrames;
   char commandIn[78], commandOut[130];

   sprintf(
      commandOut,
      "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 25 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 teapot_output%i.mp4",
      iam
   );

   FILE* pipein = popen("ffmpeg -i teapot_input.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "rb+");
   FILE* pipeout = popen(commandOut, "wb+");

   // Determina el número de frames a modificar de cada proceso.
   if (iam != tasks - 1) { endFrame = nFrames + offset; }
   
   while (1) {
      // Read a frame from the input pipe into the buffer
      count = fread(frame, 1, H * W * 3, pipein);

      // If we didn't get a frame of video, we're probably at the end
      if (count != H * W * 3) break;

      // Write this frame to the output pipe
      if (a >= offset && a < endFrame) {
         // Process this frame
        for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
            // Invert each colour component in every pixel
            frame[y][x][0] = 255 - frame[y][x][0]; // red
            frame[y][x][1] = 255 - frame[y][x][1]; // green
            frame[y][x][2] = 255 - frame[y][x][2]; // blue
        }

         fwrite(frame, 1, H * W * 3, pipeout);
      }

      a++;
   }

   fflush(pipein);
   pclose(pipein);
   fflush(pipeout);
   pclose(pipeout);

   // MPI Barrier

   if(iam == 0) {
      pipeout = popen("ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 25 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 teapot_output.mp4", "wb+");

      for (int i = 0; i < tasks; i++) {
         sprintf(
            commandIn,
            "ffmpeg -i teapot_output%i.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -",
            i
         );

         pipein = popen(commandIn, "rb+");

         while (1) {
            // Read a frame from the input pipe into the buffer
            count = fread(frame, 1, H * W * 3, pipein);

            // If we didn't get a frame of video, we're probably at the end
            if (count != H * W * 3) break;

            fwrite(frame, 1, H * W * 3, pipeout);
         }

         fflush(pipein);
         pclose(pipein);
      }

      fflush(pipeout);
      pclose(pipeout);
   }
  
   return 0;
}