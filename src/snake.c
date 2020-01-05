//Linux下用C实现贪吃蛇（多线程）多线程实现

//操纵蛇是通过键盘的上下左右键

// gcc snake.c -lpthread
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#define KEYCODE_U 0x41   // 向上按键
#define KEYCODE_D 0x42   // 向下按键
#define KEYCODE_L 0x44   // 向左按键
#define KEYCODE_R 0x43   // 向右按键

int kfd = 0;
struct termios cooked, raw;
char dir = KEYCODE_U;    // 当前蛇的移动方向

// 获取键盘响应：上、下、左、右键
void* get_dir(void *a)
{
	while(1)
	{
		char c;                                                              
		tcgetattr(kfd, &cooked); // 得到 termios 结构体保存，然后重新配置终端
		memcpy(&raw, &cooked, sizeof(struct termios));
		raw.c_lflag &=~ (ICANON | ECHO);                       
		raw.c_cc[VEOL] = 1;
		raw.c_cc[VEOF] = 2;
		tcsetattr(kfd, TCSANOW, &raw);

		if(read(kfd, &c, 1) < 0)
		{
			perror("read():");
			exit(-1);
		}
		tcsetattr(kfd, TCSANOW, &cooked);//在程序结束时在恢复原来的配置
		dir = c;
	}
}

unsigned char map[17][17] = {0};   // 游戏地图
unsigned char snake[50] = {133};   // 初始化蛇坐标，游戏开始的时候蛇在（8,5）这个位置
unsigned char food = 67;       // 食物的坐标，游戏开始的时候在（4，3）这个位置
int len = 1;                   // 保存蛇的当前长度

// 将 数字 转化为坐标系
void num_to_xy(unsigned char num, unsigned char *x, unsigned char *y)
{
	*x = num >> 4;
	*y = (unsigned char)(num << 4) >> 4;
}

// 更新地图数据
void update_map()
{	
	int i, j;
	// 先初始化边界
	for (i = 0; i < 17; i++)
	{
		for (j = 0; j < 17; j++)
		{
			if (i == 0 || i == 16 || j == 0 || j == 16)  // 代表地图的边界
			{
				map[i][j] = '#';
			}
			else
			{
				map[i][j] = ' ';
			}
			
		}
	}
	// 初始化食物
	unsigned char  x,y;      // 坐标
	num_to_xy(food, &x, &y); // 获取食物的坐标   （4，3）
	map[y][x] = '!';         // 设置食物
	
	
	// 初始化蛇
	for (i = 0; i < len; i++)
	{
		num_to_xy(snake[i], &x, &y);  // 获取蛇的第 i+1 节身体的坐标
		map[y][x] = '*';
	}
}

// 打印地图
void print_map()
{
	int i, j;
	for (i = 0; i < 17; i++)
	{
		for (j = 0; j < 17; j++)
		{
			printf ("%c", map[i][j]);
		}
		printf ("\n");
	}
	
	usleep(500000/(len/4+1));           // 让程序睡眠1秒
	system("clear");    // 清屏
}

// 生成食物
unsigned char generate_food()
{
	srand ((unsigned int)time(NULL));   // 生成一个随机数的种子
	unsigned char new_food;
	unsigned char x,y;
	int  insnake = 0;  // 标志生成的食物有没有在蛇的身体，默认是不在
	do
	{
		insnake = 0;
		new_food = rand() % 256;    // 产生一个新的食物
		num_to_xy(new_food, &x, &y);
		
		int i;
		for (i = 0; i < len; i++)
		{
			if (snake[i] == new_food)
			{
				insnake = 1;    // 1代表在蛇的身体上
				break;
			}
		}	
	}while (x == 0 || x == 16 || y == 0 || y == 16 || insnake);
	
	return new_food;
}

// 移动蛇
void move_snake()
{
	unsigned char x,y;  // 坐标
	num_to_xy(snake[0], &x, &y);   // 获取蛇头的坐标
	
	// 判断移动方向
	switch (dir)
	{
		case KEYCODE_U:   // 向上移动
			y--;
			break;
		case KEYCODE_D:   // 向下移动
			y++;
			break;
		case KEYCODE_L:   // 向左移动
			x--;
			break;
		case KEYCODE_R:   // 向右移动
			x++;
			break;
	}
	unsigned char last = snake[0];    // 保存原蛇头的值
	snake[0] = (unsigned char)(x << 4) + y;  // 更新蛇头坐标
	
	int i;
	// 移动蛇的身体
	unsigned char current;
	for (i = 1; i < len; i++)
	{
		current  = snake[i];
		snake[i] = last;
		last = current;
	}
	
	// 如果蛇头和食物的坐标重合，证明蛇吃到食物
	// len 是蛇的长度 0 -- len-1 snake[len]
	if (snake[0] == food)
	{
		// 蛇要长一节身体
		snake[len] = last;
		len++;
		
		// 重新生成食物
		food = generate_food();
	}
}

// 判断蛇是否应该存活，如果返回值是0代表应该存活，1代表不应该存活
int isalive()
{
	unsigned char x,y;
	num_to_xy(snake[0], &x, &y);   // 获取蛇头的坐标
	
	// 判断蛇头有没有碰到边界
	if (x == 0 || x == 16 || y == 0 || y == 16)
	{
		return 1;
	}
	
	// 判断蛇头有没有咬到它自己
	int i;
	for (i = 1; i < len; i++)
	{
		if (snake[0] == snake[i])
		{
			return 1;
		}
	}
	
	return 0;
}

int main()
{	
	pthread_t tid1;
	pthread_create(&tid1,NULL,get_dir,NULL);
	while (1)
	{
		update_map();
		print_map();
		move_snake();
		if (isalive() == 1)
		break;
	}
	tcsetattr(kfd, TCSANOW, &cooked);//在程序结束时在恢复原来的配置
	printf("GG\n");
	return 0;
}
