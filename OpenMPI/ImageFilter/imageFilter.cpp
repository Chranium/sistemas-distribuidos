// g++ prueba.cpp -o prueba.exe
// Video processing example using FFmpeg
// Written by Ted Burke - last updated 12-2-2017
//

#include <stdio.h>
#include <time.h>

// Video resolution
#define W 1280
#define H 720

// Allocate a buffer to store one frame
unsigned char frame[H][W][3] = { 0 };

int main() {
    int x, y, count;

    // Open an input pipe from ffmpeg and an output pipe to a second instance of ffmpeg
    FILE* pipein = popen("ffmpeg -i teapot_input.mp4 -f image2pipe -vcodec rawvideo -pix_fmt rgb24 -", "rb+");
    FILE* pipeout = popen("ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 -s 1280x720 -r 25 -i - -f mp4 -q:v 5 -an -vcodec mpeg4 teapot_output.mp4", "wb+");

    clock_t begin = clock();

    // Process video frames
    while (1) {
        // Read a frame from the input pipe into the buffer
        count = fread(frame, 1, H * W * 3, pipein);

        // If we didn't get a frame of video, we're probably at the end
        if (count != H * W * 3) break;

        // Process this frame
        for (y = 0; y < H; ++y) for (x = 0; x < W; ++x) {
            // Invert each colour component in every pixel
            frame[y][x][0] = 255 - frame[y][x][0]; // red
            frame[y][x][1] = 255 - frame[y][x][1]; // green
            frame[y][x][2] = 255 - frame[y][x][2]; // blue
        }

        // Write this frame to the output pipe
        fwrite(frame, 1, H * W * 3, pipeout);
    }

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    printf("\n==============================================");
    printf("\nTime elapsed: %f second(s)", time_spent);
    printf("\n==============================================\n");

    // Flush and close input and output pipes
    fflush(pipein);
    pclose(pipein);
    fflush(pipeout);
    pclose(pipeout);
    return 0;
}