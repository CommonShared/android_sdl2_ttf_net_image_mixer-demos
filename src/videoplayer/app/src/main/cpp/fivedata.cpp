// FiveData.c
// 五子棋：数据处理模块

#include "FiveData.h"


// 公共变量
int  g_nHands;           // 总手数
int  g_nLastCrossing;    // 100*x+y，（x，y）为最后一手的坐标
enum en_COLOR g_iWho;    // 轮到哪方落子：0 不可落子状态，1 黑方，2 白方
int  g_iBoard[MAX_LINES][MAX_LINES];    // 棋盘交叉点数据：0 无子，1 黑子，2 白子


// 判断最后一手棋是否形成五子连珠
// 返回值：1 = 形成五子连珠， 0 = 未形成五子连珠
int Five_isFive(void)
{
    int i, j, nCount, x, y;

    if(g_nLastCrossing < 0)
        return 0;
    x = g_nLastCrossing/100;
    y = g_nLastCrossing%100;

    // 横线计数
    nCount = 1;
    i = x - 1;      // 左
    while(i>=0 && g_iBoard[x][y]==g_iBoard[i][y])
    {
        nCount++;
        i--;
    }
    i = x + 1;      // 右
    while(i<MAX_LINES && g_iBoard[x][y]==g_iBoard[i][y])
    {
        nCount++;
        i++;
    }
    if(nCount >= 5)
        return 1;

    // 竖线计数
    nCount = 1;
    j = y - 1;      // 上
    while(j>=0 && g_iBoard[x][y]==g_iBoard[x][j])
    {
        nCount++;
        j--;
    }
    j = y + 1;      // 下
    while(j<MAX_LINES && g_iBoard[x][y]==g_iBoard[x][j])
    {
        nCount++;
        j++;
    }
    if(nCount >= 5)
        return 1;

    // 左斜线计数
    nCount = 1;
    i = x - 1;      // 左上
    j = y - 1;
    while(i>=0 && j>=0 && g_iBoard[x][y]==g_iBoard[i][j])
    {
        nCount++;
        i--;
        j--;
    }
    i = x + 1;      // 右下
    j = y + 1;
    while(i<MAX_LINES && j<MAX_LINES && g_iBoard[x][y]==g_iBoard[i][j])
    {
        nCount++;
        i++;
        j++;
    }
    if(nCount >= 5)
        return 1;

    // 右斜线计数
    nCount = 1;
    i = x + 1;      // 右上
    j = y - 1;
    while(i<MAX_LINES && j>=0 && g_iBoard[x][y]==g_iBoard[i][j])
    {
        nCount++;
        i++;
        j--;
    }
    i = x - 1;      // 左下
    j = y + 1;
    while(i>=0 && j<MAX_LINES && g_iBoard[x][y]==g_iBoard[i][j])
    {
        nCount++;
        i--;
        j++;
    }
    if(nCount >= 5)
        return 1;

    return 0;
}

// 重置对局数据
void Five_ResetData(void)
{
    for(int i=0; i<MAX_LINES; i++)
        for(int j=0; j<MAX_LINES; j++)
            g_iBoard[i][j] = NONE;
    g_nHands = 0;
    g_nLastCrossing = -1;
    g_iWho = BLACK;
}

// 记录一个落子数据
// 参数：x，y = 棋子坐标，c = 棋子颜色
void Five_AddPiece(int x, int y, enum en_COLOR c)
{
    g_iBoard[x][y] = c;
    g_nHands++;
    g_nLastCrossing = 100*x + y;
    g_iWho = (g_iWho == BLACK ? WHITE : BLACK);
}