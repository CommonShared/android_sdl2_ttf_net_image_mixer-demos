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
extern int Five_isFive(void);

// 重置对局数据
extern void Five_ResetData(void);

// 记录一个落子数据
// 参数：x，y = 棋子坐标，c = 棋子颜色
extern void Five_AddPiece(int x, int y, enum en_COLOR c);


#endif