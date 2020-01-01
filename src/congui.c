#define _CRT_SECURE_NO_WARNINGS
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Windows.h>

int main()
{
    setlocale(LC_ALL, "");

    /*
    //定义句柄类型的变量
    HANDLE hOut = NULL;
    //获取标准输出句柄
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    PCONSOLE_SCREEN_BUFFER_INFO screenInfo;
    GetConsoleScreenBufferInfo(hOut, screenInfo);
    WORD oldAttr = screenInfo->wAttributes;
    wprintf(L"oldAttr: %d\n", oldAttr);

    LPCWSTR title = L"这是一个标题。";
    SetConsoleTitleW(title);

    //设置文本属性为青色
    SetConsoleTextAttribute(hOut, 10);
    wprintf(L"黑马程序员黑马程序员黑马程序员\n");

    //设置文本属性为红色
    SetConsoleTextAttribute(hOut, 12);
    wprintf(L"黑马程序员黑马程序员黑马程序员\n");

    //设置文本属性为黄色
    SetConsoleTextAttribute(hOut, 14);
    wprintf(L"黑马程序员黑马程序员黑马程序员\n");

    SetConsoleTextAttribute(hOut, 3);
    wprintf(L"黑马程序员黑马程序员黑马程序员\n");

    //SetConsoleTextAttribute(hOut, oldAttr);
    wprintf(L"-------------------\n");
    system("pause");

    //开始坐标
    COORD pos0 = { 0, };
    
    //获取窗口信息结构体
    GetConsoleScreenBufferInfo(hOut, screenInfo);

    //将整个缓冲区填充字符'B'效果
    //FillConsoleOutputCharacter(hOut, 'B', screenInfo->dwSize.X * 4, pos0, NULL);

    COORD size = { 80, };
    //设置控制台缓冲区大小
    SetConsoleScreenBufferSize(hOut, size);

    SMALL_RECT rect = { 0, 0, - 1, - };
    SetConsoleWindowInfo(hOut, 1, &rect);

    //开始位置
    COORD posShadow = { 4, };
    //BACKGROUND_INTENSITY 灰色属性

    WriteConsoleOutputCharacter(hOut, "中国人中国人", 14, posShadow, NULL);

    FillConsoleOutputAttribute(hOut, BACKGROUND_INTENSITY, 10, posShadow, NULL);
    getchar();

    //*/
    /*
    //定义输出信息
    char* str = "Hello World!";
    int i;
    int len = strlen(str);

    //阴影属性
    WORD shadow = BACKGROUND_INTENSITY;
    //文本属性
    WORD text = BACKGROUND_GREEN | BACKGROUND_INTENSITY;

    //获得标准输出设备句柄
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    //定义窗口缓冲区信息结构体
    CONSOLE_SCREEN_BUFFER_INFO screenInfo;

    //获得窗口缓冲区信息
    GetConsoleScreenBufferInfo(hOut, &screenInfo);

    //定义一个文本框输出区域
    SMALL_RECT rc;

    //定义文本框的起始坐标
    COORD posText;

    //定义阴影框的起始坐标
    COORD posShadow;

    //确定区域的边界
    rc.Top = 8; //上边界
    rc.Bottom = rc.Top + 4; //下边界
    rc.Left = (screenInfo.dwSize.X - len) / 2 - 2; //左边界，为了让输出的字符串居中
    rc.Right = rc.Left + len + 4; //右边界
    //确定文本框起始坐标
    posText.X = rc.Left;
    posText.Y = rc.Top;
    //确定阴影框的起始坐标
    posShadow.X = posText.X + 1;
    posShadow.Y = posText.Y + 1;
    for (i = 0; i < 5; ++i) //先输出阴影框
    {
        FillConsoleOutputAttribute(hOut, shadow, len + 4, posShadow, NULL);
        posShadow.Y++;
    }
    for (i = 0; i < 5; ++i) //在输出文本框，其中与阴影框重合的部分会被覆盖掉
    {
        FillConsoleOutputAttribute(hOut, text, len + 4, posText, NULL);
        posText.Y++;
    }
    //设置文本输出处的坐标
    posText.X = rc.Left + 2;
    posText.Y = rc.Top + 2;
    WriteConsoleOutputCharacter(hOut, str, len, posText, NULL); //输出字符串

    getchar();
    SetConsoleTextAttribute(hOut, screenInfo.wAttributes); // 恢复原来的属性
    CloseHandle(hOut);

    system("pause");
    return 0;
    //*/
    /*
颜色函数SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),前景色 | 背景色 | 前景加强 | 背景加强);

前景色：数字0-15 或 FOREGROUND_XXX 表示	（其中XXX可用BLUE、RED、GREEN表示） 
前景加强：数字8 或 FOREGROUND_INTENSITY 表示
背景色：数字16 32 64 或 BACKGROUND_XXX 三种颜色表示 
背景加强： 数字128 或 BACKGROUND_INTENSITY 表示
主要应用：
改变指定区域字体与背景的颜色
前景颜色对应值：
0=黑色                8=灰色　
1=蓝色                9=淡蓝色        十六进制        　
2=绿色                10=淡绿色       0x0a
3=湖蓝色             11=淡浅绿色     0x0b　
4=红色                12=淡红色       0x0c　　　　
5=紫色                13=淡紫色       0x0d        　　　
6=黄色                14=淡黄色       0x0e        　　　
7=白色                15=亮白色       0x0f 

//该函数主要为了更改字体颜色，若想更改背景颜色可进行修改
void Set_Color(short color)	//自定义函根据参数改变字体颜色 
{
	if(x>=0 && x<=15)	//参数在0-15的范围颜色    	
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);	
	else	//默认的颜色白色 
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}
*/

    HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE); //获得标准输入设备句柄
    INPUT_RECORD keyrec; //定义输入事件结构体
    DWORD res; //定义返回记录
    for (;;) {
        ReadConsoleInput(handle_in, &keyrec, 1, &res); //读取输入事件
        if (keyrec.EventType == KEY_EVENT) //如果当前事件是键盘事件
        {
            if (keyrec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE //当前事件的虚拟键为Esc键
                && keyrec.Event.KeyEvent.bKeyDown == true) //表示当前为键按下而不是键释放
            {
                printf("Esc ");
            }
        }

        ReadConsoleInput(handle_in, &keyrec, 1, &res); //读取输入事件
        if (keyrec.EventType == KEY_EVENT) //如果当前事件是键盘事件
        {
            if (keyrec.Event.KeyEvent.wVirtualKeyCode == 'A' && keyrec.Event.KeyEvent.bKeyDown == true) //当按下字母A键时
            {
                if (keyrec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) //Shift键为按下状态
                {
                    printf("Shift+");
                }
                if (keyrec.Event.KeyEvent.dwControlKeyState & CAPSLOCK_ON) //大写锁定为打开状态
                {
                    printf("A ");
                } else //大写锁定关闭状态
                {
                    printf("a ");
                }
            }
        }
    }
    return 0;
}