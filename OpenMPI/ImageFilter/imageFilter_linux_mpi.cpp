// compile: mpic++ imageFilter_linux_mpi.cpp -o imageFilter_linux_mpi
// run: time mpirun -np 8 --hostfile /home/mpi/mpi_hosts /home/mpi/src/OpenMPI/ImageFilter/imageFilter_linux_mpi

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

// Video resolution
#define W 1280
#define H 720
#define F 820
#define MSG_LENGTH 30

int offset, nFrames, endFrame = F;
unsigned char frame[H][W][3];

int main (int argc, char *argv[]) {
   int iam = 0;
   int _tasks, a = 0, count, namelen;

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &_tasks);
   MPI_Comm_rank(MPI_COMM_WORLD, &iam);

   const int tasks = _tasks;
   nFrames = F / tasks;
   offset = iam * nFrames;

   char commandIn[78], commandOut[130];
   // char processor_name[MPI_MAX_PROCESSOR_NAME];
   // char message[MSG_LENGTH + 2];

   sprintf(
      commandOut,
      "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 25 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 teapot_output%i.mp4",
      iam
   );

   FILE *pipein = popen("ffmpeg -i teapot_input.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "r");
   FILE* pipeout = popen(commandOut, "w");

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

   fflush(pipein); pclose(pipein);
   fflush(pipeout); pclose(pipeout);

   // strcpy(message, "Aplicando Filtro a video");
   // MPI_Bcast(message, MSG_LENGTH, MPI_CHAR, 0, MPI_COMM_WORLD);

   // if (iam == 0) { printf("\nMensaje enviado"); fflush(stdout); }
   
   // else {
   //    MPI_Get_processor_name(processor_name, &namelen);
   //    printf("\nnodo %d %s ", iam, message);                        
   //    printf("procesador %s", processor_name); fflush(stdout);
   // }

   MPI_Barrier(MPI_COMM_WORLD);

   if(iam == 0) {
      pipeout = popen("ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 25 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 teapot_output.mp4", "w");

      for (int i = 0; i < tasks; i++) {
         sprintf(
            commandIn,
            "ffmpeg -i teapot_output%i.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -",
            i
         );

         pipein = popen(commandIn, "r");

         while (1) {
            // Read a frame from the input pipe into the buffer
            count = fread(frame, 1, H * W * 3, pipein);

            // If we didn't get a frame of video, we're probably at the end
            if (count != H * W * 3) break;

            fwrite(frame, 1, H * W * 3, pipeout);
         }

         fflush(pipein); pclose(pipein);
      }

      fflush(pipeout); pclose(pipeout);
   }
   
   MPI_Finalize();
   return 0;
}