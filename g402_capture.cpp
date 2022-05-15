/*
    Made by : Ankit Sharma
    maglash64@gmail.com
*/

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define ARRAY_SIZE 26
#define PIXEL_SIZE 5  

uint8_t buffer[64];

struct __attribute__((__packed__)) FRAME
{
    uint8_t start;
    uint8_t line;
    uint8_t linebuff[ARRAY_SIZE * 2];   //squeze all we can out of the 64 byte FS packet..
} * frame;

int main()
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    
    int fd = open("/dev/usbd", O_RDWR);

    if (fd < 0)
    {
        printf("cannot open device!\n");
        return 0;
    }

    frame = (struct FRAME *)buffer;

    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("G402 Capture", 100, 100, ARRAY_SIZE * PIXEL_SIZE, ARRAY_SIZE * PIXEL_SIZE, SDL_WINDOW_SHOWN);

    renderer =  SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED);

    bool quit = 0;
    SDL_Event e;
    SDL_Rect r;

    uint8_t arr[ARRAY_SIZE * ARRAY_SIZE];

    while (!quit)
    {
        //clear the screen
        SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        read(fd, buffer, 64);

        //if new frame(technically parts of it) has arrived read it and store it in the frame buffer..
        if(frame->start == 1)
        {   
            for (uint32_t y = 0; y < ARRAY_SIZE * 2; y++)
                arr[y] = frame->linebuff[y];

            for (uint32_t x = 1; x < (ARRAY_SIZE / 2); x++)
            {   
                read(fd, buffer, 64);
                for (uint32_t y = 0; y < ARRAY_SIZE * 2; y++)
                {
                    arr[2*(ARRAY_SIZE * x) + y] = frame->linebuff[y];
                }
            }

        }

        //display the stored frame buffer..
        for (uint32_t x = 0; x < ARRAY_SIZE; x++)
        {
            for (uint32_t y = 0; y < ARRAY_SIZE; y++)
            {
                SDL_SetRenderDrawColor( renderer, arr[ARRAY_SIZE * x + y] * 2, arr[ARRAY_SIZE * x + y] * 2, arr[ARRAY_SIZE * x + y] * 2, 255 );
                r.x = y * PIXEL_SIZE;
                r.y = x * PIXEL_SIZE;
                r.w = PIXEL_SIZE;
                r.h = PIXEL_SIZE;
                SDL_RenderFillRect( renderer, &r );
            }
        }

        SDL_RenderPresent(renderer);

        SDL_UpdateWindowSurface(window);

        while (SDL_PollEvent(&e) != 0)
        {
            // User requests quit
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
        }

        
    }

    close(fd);

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
