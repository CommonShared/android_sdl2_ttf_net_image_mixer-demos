// Five.c
// SDL2 五子棋
// gcc -mwindows -o Five Five.c FiveData.c FiveData.h -lSDL2 -lSDL2main -lSDL2_image -lSDL2_ttf -lSDL2_mixer

//#define _DEBUG_

#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <fivedata.h>

#include <android/log.h>

#define LOG_TAG    "JNILOG"
#undef LOG
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL,LOG_TAG,__VA_ARGS__)

// 资源文件
char szBackGroundFile[] = "background.jpg";    // 棋盘背景图文件
char szBlackFile[]      = "blackpiece.jpg";    // 黑棋子图文件（背景色：白色）
char szWhiteFile[]      = "whitepiece.jpg";    // 白棋子图文件（背景色：白色）
int  BackColor          = 0xFFFFFF;                     // 棋子图片的背景色
char szFontFile[]       = "lazy.ttf";  // 字体文件
char szMusicFile[]      = "luozi.mp3";         // 落子音效文件
// 字符串常量
char szTitle[]    = "五子棋";
char szBlack[]    = "黑方";
char szWhite[]    = "白方";
char szGameTips[] = "第 %d 手，轮到 %s 落子";
char szGameOver[] = "%s 取得本局胜利，请按键继续";

int OnKeyUp(int x, int y, int nSpacing);
void PrintString(SDL_Renderer *pRenderer, int nSpacing, char *szString, TTF_Font *pFont, int color);
void DrawBoard(SDL_Renderer *pRenderer, int nSpacing, int color);
void DrawPieces(SDL_Renderer *pRenderer, int nSpacing, SDL_Texture *pBlackTexture, SDL_Texture *pWhiteTexture);
void FillCircle(SDL_Renderer *pRenderer, int x, int y, int r, int color);
SDL_Texture *GetImageTexture(SDL_Renderer *pRenderer, char *szFile, int bTransparent, int color);
SDL_Texture *GetStringTexture(SDL_Renderer *pRenderer, char *szString, TTF_Font *pFont, int color);

#define _DEBUG_ 1

//#undef main
int main(int argc, char **argv)
{
    int nWindowWidth  = 640;            // 屏幕尺寸（默认640*480）
    int nWindowHeight = 480;
    int nSpacing;                       // 棋盘线距
    SDL_Window   *pWindow       = NULL; // 主窗口
    SDL_Renderer *pRenderer     = NULL; // 主窗口渲染器
    SDL_Texture  *pBackTexture  = NULL; // 棋盘背景图纹理
    SDL_Texture  *pBlackTexture = NULL; // 黑棋子图纹理
    SDL_Texture  *pWhiteTexture = NULL; // 白棋子图纹理
    TTF_Font     *pFont         = NULL; // 提示文字字体
    Mix_Music    *pMusic        = NULL; // 音效
    SDL_Event    event;                 // 事件
    int bRun = 1;                     // 持续等待事件控制循环标识
    char  szString[256];

    LOGI("enter main");

    /* Enable standard application logging */
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    {
        const char *devname = nullptr;
        SDL_AudioSpec spec;
        SDL_AudioSpec wanted;
        int devcount;
        int i;
        SDL_AudioDeviceID devid_in = 0;
        SDL_AudioDeviceID devid_out = 0;

        //SDL_Log("Using audio driver: %s\n", SDL_GetCurrentAudioDriver());
        LOGE("Using audio driver: %s\n", SDL_GetCurrentAudioDriver());

        devcount = SDL_GetNumAudioDevices(SDL_TRUE);
        for (i = 0; i < devcount; i++) {
            SDL_Log(" Capture device #%d: '%s'\n", i, SDL_GetAudioDeviceName(i, SDL_TRUE));
            LOGE(" Capture device #%d: '%s'\n", i, SDL_GetAudioDeviceName(i, SDL_TRUE));
        }

        SDL_zero(wanted);
        wanted.freq = 44100;
        wanted.format = AUDIO_F32SYS;
        wanted.channels = 1;
        wanted.samples = 4096;
        wanted.callback = NULL;

        SDL_zero(spec);

        /* DirectSound can fail in some instances if you open the same hardware
           for both capture and output and didn't open the output end first,
           according to the docs, so if you're doing something like this, always
           open your capture devices second in case you land in those bizarre
           circumstances. */

        SDL_Log("Opening default playback device...\n");
        LOGE("Opening default playback device...\n");
        devid_out = SDL_OpenAudioDevice(NULL, SDL_FALSE, &wanted, &spec, SDL_AUDIO_ALLOW_ANY_CHANGE);
        if (!devid_out) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open an audio device for playback: %s!\n", SDL_GetError());
            LOGE("Couldn't open an audio device for playback: %s!\n", SDL_GetError());
            SDL_Quit();
            exit(1);
        }

        SDL_Log("Opening capture device %s%s%s...\n",
                devname ? "'" : "",
                devname ? devname : "[[default]]",
                devname ? "'" : "");
        LOGE("Opening capture device %s%s%s...\n",
             devname ? "'" : "",
             devname ? devname : "[[default]]",
             devname ? "'" : "");
        devid_in = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &spec, &spec, 0);
        if (!devid_in) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't open an audio device for capture: %s!\n", SDL_GetError());
            LOGE("Couldn't open an audio device for capture: %s!\n", SDL_GetError());
            SDL_Quit();
            exit(1);
        }

        SDL_Log("Ready! Hold down mouse or finger to record!\n");
        LOGE("Ready! Hold down mouse or finger to record!\n");
    }
    // 初始化：SDL2、SDL_Image（jpg）、SDL_ttf、SDL_Mixer
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) == -1 ||
        IMG_Init(IMG_INIT_JPG) == -1 ||
        TTF_Init() == -1 ||
        Mix_Init(MIX_INIT_MP3) == -1 //||
        //Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) == -1
        //Mix_OpenAudio(2 * MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 4096) == -1
        )
    {
#ifdef _DEBUG_
        fprintf(stderr, "1 %s", SDL_GetError());

        LOGE("1 %s", SDL_GetError());
#endif
        return 1;
    }
    LOGI("enter main 1");

    // 创建主窗口及其渲染器
    if(SDL_CreateWindowAndRenderer(nWindowWidth, nWindowHeight, SDL_WINDOW_RESIZABLE, &pWindow, &pRenderer)==-1)
    //if(SDL_CreateWindowAndRenderer(0, 0, 0, &pWindow, &pRenderer)==-1)
    {
#ifdef _DEBUG_
        fprintf(stderr, "2 %s", SDL_GetError());
        LOGE("2 %s", SDL_GetError());
#endif
        goto label_error;
    }
    LOGI("enter main 2");
    SDL_SetWindowTitle(pWindow, szTitle);
    // 加载图片文件
    if(NULL==(pBackTexture = GetImageTexture(pRenderer, szBackGroundFile, 0, 0))
       || NULL==(pBlackTexture = GetImageTexture(pRenderer, szBlackFile, 1, BackColor))
       || NULL==(pWhiteTexture = GetImageTexture(pRenderer, szWhiteFile, 1, BackColor)))
    {
#ifdef _DEBUG_
        fprintf(stderr, "3 %s", IMG_GetError());
        LOGE("3 %s", SDL_GetError());
#endif
        goto label_error;
    }
    LOGI("enter main 3");
    // 加载字体文件
    if(NULL == (pFont = TTF_OpenFont(szFontFile, 20)))
    {
#ifdef _DEBUG_
        fprintf(stderr, "4 %s", TTF_GetError());
        LOGE("4 %s", SDL_GetError());
#endif
        goto label_error;
    }
    LOGI("enter main 4");
    // 加载声音文件
    /*if((pMusic = Mix_LoadMUS(szMusicFile)) == NULL)
    {
#ifdef _DEBUG_
        fprintf(stderr, "5 %s", Mix_GetError());
        LOGE("5 %s", SDL_GetError());
#endif
        goto label_error;
    }
    LOGI("enter main 5");*/

    // 重置棋局数据，等待事件
    Five_ResetData();
    while((bRun == 1) && SDL_WaitEvent(&event))
    {
        switch(event.type)
        {
            case SDL_MOUSEBUTTONUP :    // 鼠标按键弹起
                if(g_iWho != NONE)
                {
                    if(OnKeyUp(event.button.x, event.button.y, nSpacing) == 1)
                    {
                        //Mix_PlayMusic(pMusic, 0);
                        if(Five_isFive() == 1)
                            g_iWho = NONE;
                    }
                }
                else
                    Five_ResetData();
                // 这里没有 break; 往下坠落重绘窗口

            case SDL_WINDOWEVENT :      //  有窗口消息需重绘窗口
                SDL_GetWindowSize(pWindow, &nWindowWidth, &nWindowHeight);
                nSpacing = SDL_min(nWindowWidth, nWindowHeight)/(MAX_LINES+2);

                SDL_RenderClear(pRenderer);
                SDL_RenderCopyEx(pRenderer, pBackTexture, NULL, NULL, 0, NULL, SDL_FLIP_NONE);
                DrawBoard(pRenderer, nSpacing, 0);
                DrawPieces(pRenderer, nSpacing, pBlackTexture, pWhiteTexture);
                if(g_iWho == NONE)
                    sprintf(szString, szGameOver, g_nHands%2==1 ? szBlack : szWhite);
                else
                    sprintf(szString, szGameTips, g_nHands+1, g_iWho==BLACK ? szBlack : szWhite);
                PrintString(pRenderer, nSpacing, szString, pFont, 0);
                SDL_RenderPresent(pRenderer);
                break;

            case SDL_QUIT :
                bRun = 0;
                break;

            default :
                break;
        }
    }

    label_error:
// 清理
    if(pBackTexture  != NULL) SDL_DestroyTexture(pBackTexture);
    if(pBlackTexture != NULL) SDL_DestroyTexture(pBlackTexture);
    if(pWhiteTexture != NULL) SDL_DestroyTexture(pWhiteTexture);
    if(pFont != NULL)         TTF_CloseFont(pFont);
    if(pMusic != NULL)        Mix_FreeMusic(pMusic);
    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}

// 响应落子按键
// 参数：(x，y) = 被点击的窗口坐标；nSpacing = 棋盘线距
int OnKeyUp(int x, int y, int nSpacing)
{
    // 计算落点棋盘坐标
    int m = (x - 0.5*nSpacing)/nSpacing;
    int n = (y - 0.5*nSpacing)/nSpacing;
    // 处理有效落点
    if(m>=0 && m<MAX_LINES && n>=0 && n<MAX_LINES && g_iBoard[m][n]==NONE)
    {
        Five_AddPiece(m, n, g_iWho);
        return 1;
    }
    return 0;
}

// 画棋盘
// 参数：pRenderer = 渲染器；nSpacing = 棋盘线距；color = 线及星颜色
void DrawBoard(SDL_Renderer *pRenderer, int nSpacing, int color)
{
    int r, x, y, z;

    // 棋盘线
    SDL_SetRenderDrawColor(pRenderer, color>>16, (color>>8)&0xFF, color&0xFF, SDL_ALPHA_OPAQUE);
    for(int i = 1; i <= MAX_LINES; i++)
    {
        SDL_RenderDrawLine(pRenderer, nSpacing, i*nSpacing, MAX_LINES*nSpacing, i*nSpacing);
        SDL_RenderDrawLine(pRenderer, i*nSpacing, nSpacing, i*nSpacing, MAX_LINES*nSpacing);
    }

    // 星位
    r = nSpacing*0.2;               // 星半径
    x = nSpacing*4;                 // 第四线
    y = nSpacing*(MAX_LINES+1)/2;   // 中线
    z = nSpacing*(MAX_LINES-3);     // 倒数第四线
    FillCircle(pRenderer, x, x, r, color);
    FillCircle(pRenderer, y, x, r, color);
    FillCircle(pRenderer, z, x, r, color);
    FillCircle(pRenderer, x, y, r, color);
    FillCircle(pRenderer, y, y, r, color);
    FillCircle(pRenderer, z, y, r, color);
    FillCircle(pRenderer, x, z, r, color);
    FillCircle(pRenderer, y, z, r, color);
    FillCircle(pRenderer, z, z, r, color);
}

// 画棋子
// 参数：pRenderer = 渲染器；nSpacing = 棋盘线距；pBlackTexture = 黑子纹理；pWhiteTexture = 白子纹理
void DrawPieces(SDL_Renderer *pRenderer, int nSpacing, SDL_Texture *pBlackTexture, SDL_Texture *pWhiteTexture)
{
    int r = 0.4*nSpacing;  // 棋子半径
    SDL_Rect rt = {0, 0, 2*r, 2*r};

    if(g_nHands <= 0)
        return;

    for(int i=0; i<MAX_LINES; i++)
    {
        for(int j=0; j<MAX_LINES; j++)
        {
            rt.x = (i+1)*nSpacing - r;
            rt.y = (j+1)*nSpacing - r;
            if(g_iBoard[i][j] == BLACK)
                SDL_RenderCopyEx(pRenderer, pBlackTexture, NULL, &rt, 0, NULL, SDL_FLIP_NONE);
            else if(g_iBoard[i][j] == WHITE)
                SDL_RenderCopyEx(pRenderer, pWhiteTexture, NULL, &rt, 0, NULL, SDL_FLIP_NONE);
        }
    }
}

// 提示文字
// 参数：pRenderer = 渲染器；nSpacing = 棋盘线距；szString = 文字内容；pFont = 字体；color = 文字颜色
void PrintString(SDL_Renderer *pRenderer, int nSpacing, char *szString, TTF_Font *pFont, int color)
{
    SDL_Texture *pTextTexture;
    SDL_Rect rt;

    rt.x = nSpacing;
    rt.y = nSpacing*(MAX_LINES+1);
    rt.w = nSpacing*strlen(szString)/4;     // 这个 4 和字体大小有关
    rt.h = nSpacing;

    if((pTextTexture = GetStringTexture(pRenderer, szString, pFont, color)) != NULL)
    {
        SDL_RenderCopyEx(pRenderer, pTextTexture, NULL, &rt, 0, NULL, SDL_FLIP_NONE);
        SDL_DestroyTexture(pTextTexture);
    }
}

// 取得图片文件纹理
// 参数：pRenderer = 渲染器；szFile = 图片文件名；bTransparent = 是否透明处理；color = 背景色
// 返回值：纹理指针
SDL_Texture *GetImageTexture(SDL_Renderer *pRenderer, char *szFile, int bTransparent, int color)
{
    SDL_Texture *pTexture;
    SDL_Surface *pSurface;

    if((pSurface = IMG_Load(szFile)) == NULL)
        return NULL;
    if(bTransparent)
        SDL_SetColorKey(pSurface, 1, SDL_MapRGB(pSurface->format, color>>16, (color>>8)&0xFF, color&0xFF));
    pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
    SDL_FreeSurface(pSurface);

    return pTexture;
}

// 取得字符串纹理
// 参数：pRenderer = 渲染器；szString = 字符串内容；pFont = 字体；color = 文字颜色
// 返回值：纹理指针
SDL_Texture *GetStringTexture(SDL_Renderer *pRenderer, char *szString, TTF_Font *pFont, int color)
{
    SDL_Texture *pTexture;
    SDL_Surface *pSurface;
    SDL_Color c = {(Uint8)(color>>16), (Uint8)((color>>8)&0xFF), (Uint8)(color&0xFF)};

    if((pSurface = TTF_RenderUTF8_Blended(pFont, szString, c)) == NULL)
        return NULL;
    pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
    SDL_FreeSurface(pSurface);

    return pTexture;
}

// 画圆（SDL2 没有画圆的函数，先用矩形框代替吧）
// 参数：pRenderer = 渲染器；(x，y) = 圆心坐标；r = 半径；color = 填充色
void FillCircle(SDL_Renderer *pRenderer, int x, int y, int r, int color)
{
    SDL_Rect rt = {x-r, y-r, 2*r, 2*r};

    SDL_SetRenderDrawColor(pRenderer, color>>16, (color>>8)&0xFF, color&0xFF, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(pRenderer, &rt);
}
// FiveData.h
// 五子棋：数据处理模块对外接口

#ifndef _FIVE_DATA_H
#define _FIVE_DATA_H


enum en_COLOR   // 棋子颜色
{
    NONE = 0,       // 无子
    BLACK,          // 黑子
    WHITE           // 白子
};

// 棋局
#define MAX_LINES      15       // 棋盘线数
extern int  g_nHands;           // 总手数
extern int  g_nLastCrossing;    // 100*x+y，（x，y）为最后一手的坐标
extern enum en_COLOR g_iWho;    // 轮到哪方落子：0 不可落子状态，1 黑方，2 白方
extern int  g_iBoard[MAX_LINES][MAX_LINES]; // 棋盘交叉点数据：0 无子，1 黑子，2 白子


// 判断最后一手棋是否形成五子连珠
// 返回值：1 = 形成五子连珠， 0 = 未形成五子连珠
extern _Bool Five_isFive(void);

// 重置对局数据
extern void Five_ResetData(void);

// 记录一个落子数据
// 参数：x，y = 棋子坐标，c = 棋子颜色
extern void Five_AddPiece(int x, int y, enum en_COLOR c);


#endif