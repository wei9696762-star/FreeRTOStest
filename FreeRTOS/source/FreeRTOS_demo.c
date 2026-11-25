/**
  ******************************************************************************
  * @file    FreeRTOS_demo.c
  * @brief   STM32 智能手表 —— FreeRTOS 任务创建与页面调度实现
  ******************************************************************************
  * 任务结构说明：
  *   1. Start_Task       启动任务，负责创建其余全部任务，完成后删除自身；
  *   2. Key_Task         按键扫描任务（优先级最高），把键值写入消息队列；
  *   3. ShowTime_Task    时间显示任务（默认运行的界面）；
  *   4. Menu_Task        菜单任务；
  *   5. StopWatch_Task   秒表任务；
  *   6. LED_Task         手电筒任务；
  *   7. MPU6050_Task     MPU6050 传感器演示任务；
  *   8. Game_Task        游戏任务；
  *   9. Emoji_Task       表情任务；
  *  10. Gradienter_Task  水平仪任务；
  *  11. SetTime_Task     时间设置任务。
  *
  * 页面切换方式（挂起 / 恢复）：
  *   - 开机后只有时间显示任务和菜单任务处于就绪状态，其余功能任务
  *     创建完成后立即被挂起；
  *   - 菜单任务根据用户选择恢复对应的功能任务，然后挂起自己；
  *   - 功能任务退出时恢复菜单任务，再挂起自己。
  *   这样任意时刻只有一个界面任务在运行，避免多个任务同时操作
  *   LCD 造成显示冲突。
  ******************************************************************************
  */

#include "FreeRTOS_demo.h"   /* FreeRTOS 头文件及任务声明 */
#include "gpio.h"            /* GPIO 初始化 */
#include "menu.h"            /* 菜单与各界面函数声明 */
#include "key.h"             /* 按键相关声明 */
#include "adc.h"             /* ADC（电池电压采集等） */
#include "tim.h"             /* 定时器 */

/* 按键消息队列句柄：Key_Task 写入键值，各界面任务通过 Key_GetNum_RTOS() 读取 */
QueueHandle_t Key_Queue;

/* 各任务句柄：用于任务的挂起、恢复和删除 */
TaskHandle_t Start_Task_Handle;        /* 启动任务 */
TaskHandle_t Key_Task_Handle;          /* 按键扫描任务 */
TaskHandle_t ShowTime_Task_Handle;     /* 时间显示任务 */
TaskHandle_t Menu_Task_Handle;         /* 菜单任务 */
TaskHandle_t StopWatch_Task_Handle;    /* 秒表任务 */
TaskHandle_t LED_Task_Handle;          /* 手电筒任务 */
TaskHandle_t MPU6050_Task_Handle;      /* MPU6050 演示任务 */
TaskHandle_t Game_Task_Handle;         /* 游戏任务 */
TaskHandle_t Emoji_Task_Handle;        /* 表情任务 */
TaskHandle_t Gradienter_Task_Handle;   /* 水平仪任务 */
TaskHandle_t SetTime_Task_Handle;      /* 时间设置任务 */

/* ---------------- 任务优先级 ----------------
 * FreeRTOS 中数值越大优先级越高。
 * 按键扫描需要及时响应，设为 5；
 * 其余功能任务统一为 3，优先级相同，靠挂起/恢复来切换。
 */
#define KEY_TASK_PRIORITY       5
#define KEY_STK_SIZE            128   /* 栈大小，单位为字(Word)，下同 */

#define StopWatch_TASK_PRIORITY   3
#define LED_TASK_PRIORITY         3
#define ShowTime_TASK_PRIORITY    3
#define Menu_TASK_PRIORITY        3
#define Game_TASK_PRIORITY        3
#define Emoji_TASK_PRIORITY       3
#define MPU6050_TASK_PRIORITY     3
#define Gradienter_TASK_PRIORITY  3
#define SetTime_TASK_PRIORITY     3

/* ---------------- 各任务栈大小（单位：字） ---------------- */
#define StopWatch_STK_SIZE      128
#define LED_STK_SIZE            128
#define ShowTime_STK_SIZE       256   /* 时间显示涉及较多绘制，栈稍大 */
#define Menu_STK_SIZE           256
#define Game_STK_SIZE           256
#define Emoji_STK_SIZE          256
#define MPU6050_STK_SIZE        350   /* 传感器运算占用较多局部变量 */
#define Gradienter_STK_SIZE     350
#define SetTime_STK_SIZE        350

/* 启动任务优先级最低(1)，创建完其他任务后即删除自己 */
#define START_TASK_PRIORITY     1
#define START_STK_SIZE          128

/* 任务函数声明 */
void Start_Task(void *pvParameters);
void Key_Task(void *pvParameters);
void ShowTime_Task(void *pvParameters);
void Menu_Task(void *pvParameters);
void StopWatch_Task(void *pvParameters);
void LED_Task(void *pvParameters);
void MPU6050_Task(void *pvParameters);
void Game_Task(void *pvParameters);
void Emoji_Task(void *pvParameters);
void Gradienter_Task(void *pvParameters);
void SetTime_Task(void *pvParameters);

/**
  * @brief  FreeRTOS 启动函数
  * @note   由 main() 调用：先创建按键消息队列和启动任务，
  *         再开启任务调度器。正常情况下不会返回。
  */
void FreeRTOS_Start(void)
{
    /* 创建按键消息队列：深度 5，每个元素为 1 字节的键值 */
    Key_Queue = xQueueCreate(5, sizeof(uint8_t));

    /* 创建启动任务，由它负责创建其余所有任务 */
    xTaskCreate((TaskFunction_t)Start_Task,
                (const char*   )"Start_Task",
                (uint16_t      )START_STK_SIZE,
                (void*         )NULL,
                (UBaseType_t   )START_TASK_PRIORITY,
                (TaskHandle_t* )&Start_Task_Handle);

    /* 启动任务调度器，开始多任务运行 */
    vTaskStartScheduler();
}

/**
  * @brief  启动任务
  * @note   创建全部功能任务，并设置初始的挂起状态，最后删除自身。
  */
void Start_Task(void *pvParameters)
{
    /* 进入临界区：任务创建期间不允许被其他任务打断 */
    taskENTER_CRITICAL();

    /* 创建按键扫描任务 */
    xTaskCreate(Key_Task, "Key_Task", KEY_STK_SIZE, NULL, KEY_TASK_PRIORITY, &Key_Task_Handle);

    /* 创建各功能界面任务：时间显示、菜单、游戏、表情、秒表、
       手电筒、MPU6050、水平仪、时间设置 */
    xTaskCreate(ShowTime_Task, "Time_Task", ShowTime_STK_SIZE, NULL, ShowTime_TASK_PRIORITY, &ShowTime_Task_Handle);
    xTaskCreate(Menu_Task,     "Menu_Task", Menu_STK_SIZE, NULL, Menu_TASK_PRIORITY, &Menu_Task_Handle);
    xTaskCreate(Game_Task,     "Game",      Game_STK_SIZE, NULL, Game_TASK_PRIORITY, &Game_Task_Handle);
    xTaskCreate(Emoji_Task,    "Emoji",     Emoji_STK_SIZE, NULL, Emoji_TASK_PRIORITY, &Emoji_Task_Handle);
    xTaskCreate(StopWatch_Task, "StopWatch", StopWatch_STK_SIZE, NULL, StopWatch_TASK_PRIORITY, &StopWatch_Task_Handle);
    xTaskCreate(LED_Task,       "LED",       LED_STK_SIZE, NULL, LED_TASK_PRIORITY, &LED_Task_Handle);
    xTaskCreate(MPU6050_Task,   "MPU6050",   MPU6050_STK_SIZE, NULL, MPU6050_TASK_PRIORITY, &MPU6050_Task_Handle);
    xTaskCreate(Gradienter_Task,"Gradienter",Gradienter_STK_SIZE, NULL, Gradienter_TASK_PRIORITY, &Gradienter_Task_Handle);
    xTaskCreate(SetTime_Task,   "SetTime",   SetTime_STK_SIZE, NULL, SetTime_TASK_PRIORITY, &SetTime_Task_Handle);

    /* 除时间显示任务外，其余界面任务创建后先挂起，
       等待菜单任务根据用户选择逐个恢复 */
    vTaskSuspend(Menu_Task_Handle);
    vTaskSuspend(StopWatch_Task_Handle);
    vTaskSuspend(LED_Task_Handle);
    vTaskSuspend(MPU6050_Task_Handle);
    vTaskSuspend(Game_Task_Handle);
    vTaskSuspend(Emoji_Task_Handle);
    vTaskSuspend(Gradienter_Task_Handle);
    vTaskSuspend(SetTime_Task_Handle);

    /* 启动任务使命完成，删除自身；随后退出临界区 */
    vTaskDelete(Start_Task_Handle);
    taskEXIT_CRITICAL();
}

/**
  * @brief  按键扫描任务
  * @note   每 20ms 检查一次全局变量 Key_Num（由按键扫描程序更新），
  *         非 0 表示有新按键，将其写入队列后清零，避免重复发送。
  */
extern uint8_t Key_Num;
void Key_Task(void *pvParameters)
{
    uint8_t key = 0;
    while(1)
    {
        if(Key_Num != 0)                 /* 检测到新的按键值 */
        {
            key = Key_Num;
            Key_Num = 0;                 /* 清零，防止同一按键被发送多次 */
            xQueueSend(Key_Queue, &key, 0);  /* 非阻塞方式写入队列 */
        }
        vTaskDelay(20); /* 20ms 轮询一次，把键值发送到队列 */
    }
}


/**
  * @brief  从按键队列读取键值（非阻塞）
  * @retval 读取到的键值；队列为空时返回 0
  */
uint8_t Key_GetNum_RTOS(void)
{
    uint8_t key_val = 0;
    BaseType_t xStatus;

    /* 等待 0 个节拍：不等待，立即返回 */
    xStatus = xQueueReceive(Key_Queue, &key_val, 0);

    if(xStatus == pdPASS)
    {
        return key_val;      /* 成功读到键值 */
    }
    else
    {
        return 0;            /* 队列为空，没有按键 */
    }
}

/**
  * @brief  时间显示任务（开机默认界面）
  * @note   First_Page_Clock() 显示时间并返回用户操作结果：
  *         1 -> 进入菜单；2 -> 进入时间设置。
  *         切换界面前先恢复目标任务，再挂起自己。
  */
void ShowTime_Task(void *pvParameters)
{
    int flag;
    while(1)
    {
        flag = First_Page_Clock();

        if(flag == 1) /* 进入菜单 */
        {
            vTaskResume(Menu_Task_Handle);
            vTaskSuspend(NULL);          /* 挂起当前（时间显示）任务 */
        }
        else if(flag == 2) /* 进入时间设置 */
        {
             vTaskResume(SetTime_Task_Handle);
             vTaskSuspend(NULL);
        }
        vTaskDelay(5); /* 让出 CPU，避免独占处理器 */
    }
}

/**
  * @brief  菜单任务
  * @note   Menu() 阻塞运行菜单界面，并返回用户选中的功能编号：
  *         0 返回时间页，2 秒表，3 手电筒，4 MPU6050，
  *         5 游戏，6 表情，7 水平仪。
  *         根据编号恢复对应功能任务，然后挂起菜单自身。
  */
void Menu_Task(void *pvParameters)
{
    int selection;
    while(1)
    {
        selection = Menu(); /* Menu() 内部负责界面显示与按键处理 */

        switch(selection)
        {
            case 0: /* 返回时间页 */
                vTaskResume(ShowTime_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 2: /* 秒表 */
                vTaskResume(StopWatch_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 3: /* 手电筒 */
                vTaskResume(LED_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 4: /* MPU6050 */
                vTaskResume(MPU6050_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 5: /* 游戏 */
                vTaskResume(Game_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 6: /* 表情 */
                vTaskResume(Emoji_Task_Handle);
                vTaskSuspend(NULL);
                break;
            case 7: /* 水平仪 */
                vTaskResume(Gradienter_Task_Handle);
                vTaskSuspend(NULL);
                break;
            default:  /* 非法选择，返回时间页 */
                vTaskResume(ShowTime_Task_Handle);
                vTaskSuspend(NULL);
                break;
        }
        vTaskDelay(5);
    }
}

/**
  * @brief  秒表任务
  * @note   MenuToFunction() 完成进入界面前的初始化；
  *         StopWatch() 会一直阻塞运行，直到用户退出该界面；
  *         退出后恢复菜单任务，再挂起自己。
  */
void StopWatch_Task(void *pvParameters)
{
    while(1)
    {
        MenuToFunction();
		StopWatch(); //除非退出此任务，否则一直会阻塞在这，所以MenuToFunction();不会被反复执行
        vTaskResume(Menu_Task_Handle);//一旦退出此任务，在裸机中需要重新初始化菜单各参数
        vTaskSuspend(NULL);//在FreeRTOS中直接回到菜单任务就好了
        vTaskDelay(5);
    }
}

/**
  * @brief  手电筒（LED）任务
  * @note   LED() 阻塞运行，退出后返回菜单界面。
  */
void LED_Task(void *pvParameters)
{
    while(1)
    {
        MenuToFunction();
		LED();
        vTaskResume(Menu_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

/**
  * @brief  MPU6050 传感器演示任务
  * @note   MPU6050() 阻塞运行，退出后返回菜单界面。
  */
void MPU6050_Task(void *pvParameters)
{
    while(1)
    {
		MenuToFunction();
        MPU6050();
        vTaskResume(Menu_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

/**
  * @brief  游戏任务
  * @note   Game() 阻塞运行，退出后返回菜单界面。
  */
void Game_Task(void *pvParameters)
{
    while(1)
    {
        MenuToFunction();
		Game();
        vTaskResume(Menu_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

/**
  * @brief  表情任务
  * @note   Emoji() 阻塞运行，退出后返回菜单界面。
  */
void Emoji_Task(void *pvParameters)
{
    while(1)
    {
        MenuToFunction();
		Emoji();
        vTaskResume(Menu_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

/**
  * @brief  水平仪任务
  * @note   Gradienter() 阻塞运行，退出后返回菜单界面。
  */
void Gradienter_Task(void *pvParameters)
{
    while(1)
    {
        MenuToFunction();
		Gradienter();
        vTaskResume(Menu_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}

/**
  * @brief  时间设置任务
  * @note   SettingPage() 阻塞运行时间设置界面，退出后返回时间显示页。
  */
void SetTime_Task(void *pvParameters)
{
    while(1)
    {
        SettingPage();
        vTaskResume(ShowTime_Task_Handle);
        vTaskSuspend(NULL);
        vTaskDelay(5);
    }
}
