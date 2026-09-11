#ifndef __TEST_H
#define __TEST_H

#include "oled.h"

// 这些函数分别测试不同功能，主程序里调用它们就能看到效果
void TEST_MainPage(void);          // 开机主界面
void Test_Color(void);             // 刷屏（黑/白切换）
void Test_Rectangular(void);       // 画矩形并填充
void Test_Circle(void);            // 画圆
void Test_Triangle(void);          // 画三角形并填充
void TEST_English(void);           // 显示英文（大小写）
void TEST_Number_Character(void);  // 显示数字和符号
void TEST_Chinese(void);           // 显示中文（不同字号）
void TEST_BMP(void);               // 显示 BMP 图片
void TEST_Menu1(void);             // 模拟菜单1
void TEST_Menu2(void);             // 模拟菜单2（天气界面）

#endif
