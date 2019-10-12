#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <android/log.h>

//#include "libavformat/avformat.h"
//#include "libavcodec/avcodec.h"
//#include "libswscale/swscale.h"
//#include "libavutil/imgutils.h"
//#include "libavutil/avutil.h"

#include "SDL.h"

#define LOG_TAG    "JNILOG"
#undef LOG
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG_TAG,__VA_ARGS__)


typedef struct Sprite
{
    SDL_Texture* texture;
    Uint16 w;
    Uint16 h;
} Sprite;

/* Adapted from SDL's testspriteminimal.c */
Sprite LoadSprite(const char* file, SDL_Renderer* renderer)
{
    Sprite result;
    result.texture = NULL;
    result.w = 0;
    result.h = 0;

    SDL_Surface* temp;

    /* Load the sprite image */
    temp = SDL_LoadBMP(file);
    if (temp == NULL)
    {
        fprintf(stderr, "Couldn't load %s: %s\n", file, SDL_GetError());
        return result;
    }
    result.w = temp->w;
    result.h = temp->h;

    /* Create texture from the image */
    result.texture = SDL_CreateTextureFromSurface(renderer, temp);
    if (!result.texture) {
        fprintf(stderr, "Couldn't create texture: %s\n", SDL_GetError());
        SDL_FreeSurface(temp);
        return result;
    }
    SDL_FreeSurface(temp);

    return result;
}

void draw(SDL_Window* window, SDL_Renderer* renderer, const Sprite sprite)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    SDL_Rect destRect = {w/2 - sprite.w/2, h/2 - sprite.h/2, sprite.w, sprite.h};
    /* Blit the sprite onto the screen */
    SDL_RenderCopy(renderer, sprite.texture, NULL, &destRect);
}

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;

    LOGI("SDL HelloWorld");
    /*av_register_all();*/
    //LOGI("%s\n", avcodec_configuration());

    if(SDL_CreateWindowAndRenderer(0, 0, 0, &window, &renderer) < 0)
        exit(2);

    Sprite sprite = LoadSprite("test.bmp", renderer);
    if(sprite.texture == NULL)
        exit(2);

    /* Main render loop */
    Uint8 done = 0;
    SDL_Event event;
    while(!done)
    {
        /* Check for events */
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT )
            {
                done = 1;
            }
            else if(event.type == SDL_KEYDOWN || event.type == SDL_FINGERDOWN)
            {
                LOGI("SDL event type=%d(0x%x)",event.type,event.type);
            }
            else
            {
                LOGI("SDL event type=%d(0x%x)",event.type,event.type);
                if(event.type == SDL_APP_TERMINATING)
                {
                    continue;
                }
            }
        }

        /* Draw a gray background */
        SDL_SetRenderDrawColor(renderer, 0xA0, 0xA0, 0xA0, 0xFF);
        SDL_RenderClear(renderer);

        draw(window, renderer, sprite);

        /* Update the screen! */
        SDL_RenderPresent(renderer);

        SDL_Delay(10);
    }

    exit(0);
}