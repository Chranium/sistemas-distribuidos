//  g++ prueba3.cpp -lstdc++ -lvfw32 -o prueba3.exe
#define UNICODE
#include <vfw.h>
#include <stdio.h>

BOOL CreateFromPackedDIBPointer(LPBYTE pDIB, int iFrame) {
   //Creates a full-color (no palette) DIB from a pointer to a
   //full-color memory DIB

   //get the BitmapInfoHeader
   BITMAPINFOHEADER bih;
   printf("OK");
   RtlMoveMemory(&bih.biSize, pDIB, sizeof(BITMAPINFOHEADER));

   // Now get the bitmap bits
   if (bih.biSizeImage < 1) { return FALSE; }

   BYTE* Bits = new BYTE[bih.biSizeImage];
   RtlMoveMemory(Bits, pDIB + sizeof(BITMAPINFOHEADER), bih.biSizeImage);

   //and BitmapInfo variable-length UDT
   BYTE memBitmapInfo[40];
   RtlMoveMemory(memBitmapInfo, &bih, sizeof(bih));

   BITMAPFILEHEADER bfh;
   bfh.bfType = 19778;    //BM header
   bfh.bfSize=55 + bih.biSizeImage;
   bfh.bfReserved1 = 0;
   bfh.bfReserved2 = 0;
   bfh.bfOffBits = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER); //54
   
   FILE* fp = fopen("prueba3.bmp", "wb");
   if (fp != NULL) {
      fwrite(&bfh, sizeof(bfh), 1, fp);
      fwrite(&memBitmapInfo, sizeof(memBitmapInfo), 1, fp);
      fwrite(Bits, bih.biSizeImage, 1, fp);
      fclose(fp);
   }
    
   else {
      printf("Error writing the bitmap file");
      return FALSE;
   }

   delete [] Bits;
   return TRUE;
}

int main() {
   LONG res; 
   PAVIFILE avi; 
   wchar_t filename[] = L"prueba.avi";

   AVIFileInit();
   res = AVIFileOpen(&avi, filename, OF_SHARE_DENY_WRITE, 0L);
   if (res != 0) {
      // Handle failure.
      printf("Error en archivo AVI\n");
      return 1;
   }

   AVIFILEINFO avi_info;
   AVIFileInfo(avi, &avi_info, sizeof(AVIFILEINFO));

   printf(
      "Dimention: %dx%d\nLength: %d frames\nMax bytes per second: %d\nSamples per second: %d\nStreams: %d\nFile Type: %d",
      avi_info.dwWidth,
      avi_info.dwHeight,
      avi_info.dwLength,
      avi_info.dwMaxBytesPerSec,
      (DWORD) (avi_info.dwRate / avi_info.dwScale),
      avi_info.dwStreams,
      avi_info.szFileType
   );

   PAVISTREAM pStream;
   res = AVIFileGetStream(avi, &pStream, streamtypeVIDEO /*video stream*/, 0 /*first stream*/);

   if (res != AVIERR_OK) {
      if (pStream != NULL) { AVIStreamRelease(pStream); }
      AVIFileExit();
   }

   // Do some task with the stream
   int iNumFrames;
   int iFirstFrame;

   iFirstFrame = AVIStreamStart(pStream);

   if (iFirstFrame == -1) {
      // Error getteing the frame inside the stream
      if (pStream != NULL) { AVIStreamRelease(pStream); }
      AVIFileExit();
   }

   iNumFrames = AVIStreamLength(pStream);
   if (iNumFrames == -1) {
      // Error getteing the number of frames inside the stream
      if (pStream != NULL) { AVIStreamRelease(pStream); }
      AVIFileExit();
   }

   // Getting bitmap from frame
   BITMAPINFOHEADER bih;
   ZeroMemory(&bih, sizeof(BITMAPINFOHEADER));

   bih.biBitCount = 24;    //24 bit per pixel
   bih.biClrImportant = 0;
   bih.biClrUsed = 0;
   bih.biCompression = BI_RGB;
   bih.biPlanes = 1;
   bih.biSize = 40;
   bih.biXPelsPerMeter = 0;
   bih.biYPelsPerMeter = 0;
   // calculate total size of RGBQUAD scanlines (DWORD aligned)
   bih.biSizeImage = (((bih.biWidth * 3) + 3) & 0xFFFC) * bih.biHeight ;

   PGETFRAME pFrame;
   pFrame = AVIStreamGetFrameOpen(pStream, NULL/*(BITMAPINFOHEADER*) AVIGETFRAMEF_BESTDISPLAYFMT*/ /*&bih*/);

   if (pFrame == NULL) { printf("\nNULL"); return 1; }

   // Get the first frame
   int index = 0;
   for (int i = iFirstFrame; i < iNumFrames; i++) {
      index = i - iFirstFrame;
      BYTE* pDIB = (BYTE*) AVIStreamGetFrame(pFrame, index);       
      CreateFromPackedDIBPointer(pDIB, index);
   }

   AVIStreamGetFrameClose(pFrame);
   //close the stream after finishing the task
   if (pStream!=NULL) { AVIStreamRelease(pStream); }

   AVIFileRelease(avi);
   AVIFileExit();
   return 0;
}