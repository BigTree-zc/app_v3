#if 0
#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>

int main(void)
{
    lv_init();

    /*Linux frame buffer device init*/
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    /*Create a Demo*/
    lv_demo_widgets();
    lv_demo_widgets_start_slideshow();

    /*Handle LVGL tasks*/
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}
#endif

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#define DISP_BUF_SIZE (800 * 480)

//void msleep(unsigned int n);
//void sleep(unsigned int n);

int main(void)
{
    int fd;
	int val;
    int i;
    unsigned int keyCount;
    pid_t status;

#if 0
    for(i=0;i<1;i++)
    {
        sleep(1);
        printf("i=%d\n",i);
    }

	/* 2. 打开文件 */
	fd = open("/dev/keys-key", O_RDWR);
	if (fd == -1)
	{
		printf("can not open file /dev/keys-key\n");
		return -1;
	}
    else
    {
        printf("open file /dev/keys-key ok\n");
    }

    keyCount = 0;
#endif

#if 0
	for(i=0;i<10;i++)
	{
		read(fd, &val, 4);
        printf("val=%d\n",val);
        if(val == 33280)
        {
            keyCount++;
            if(keyCount >=4)
            {
                status = system("ts_calibrate");
                if (-1 == status)
                {
                    printf("system error\n");
                    return -1;
                }
 
                //正确退出并操作成功
                if (WIFEXITED(status)&& 0 == WEXITSTATUS(status))
                {
                    printf("run successfully.\n");
                }
                else
                {
                    printf("fail, exit code: [%d]\n", WEXITSTATUS(status));
                    return -1;
                }          
            }
        }
        else 
        {
            break;
        }
        sleep(1);
	}
#endif

    lv_init();

    /*Linux frame buffer device init*/
    fbdev_init();

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;
    disp_drv.hor_res    = 480;
    disp_drv.ver_res    = 272;
    lv_disp_drv_register(&disp_drv);

    /* Linux input device init */
    evdev_init();

    /* Initialize and register a display input driver */
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);      /*Basic initialization*/

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = evdev_read;
    lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv); 

    /*Create a Demo*/
    lv_demo_widgets();

    /*Handle LVGL tasks*/
    while(1) 
    {
        lv_timer_handler();
        usleep(5000);
    }
    
    return 0;
}

void msleep(unsigned int n)
{
    unsigned int i;
    for(i=0;i<n;i++)
    {
        usleep(1000);
    }
}
/*
void sleep(unsigned int n)
{
    unsigned int i;
    for(i=0;i<n;i++)
    {
        msleep(1000);
    }
}
*/


/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}