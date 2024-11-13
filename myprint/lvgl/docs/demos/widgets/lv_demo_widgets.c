/**
 * @file lv_demo_widgets.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_widgets.h"
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include <sys/mman.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>   
#include <sys/resource.h>

#if LV_USE_DEMO_WIDGETS

#if LV_MEM_CUSTOM == 0 && LV_MEM_SIZE < (38ul * 1024ul)
    #error Insufficient memory for lv_demo_widgets. Please set LV_MEM_SIZE to at least 38KB (38ul * 1024ul).  48KB is recommended.
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef enum {
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void profile_create(lv_obj_t * parent);
static void analytics_create(lv_obj_t * parent);
static void shop_create(lv_obj_t * parent);
static void color_changer_create(lv_obj_t * parent);

static lv_obj_t * create_meter_box(lv_obj_t * parent, const char * title, const char * text1, const char * text2,
                                   const char * text3);
static lv_obj_t * create_shop_item(lv_obj_t * parent, const void * img_src, const char * name, const char * category,
                                   const char * price);

static void color_changer_event_cb(lv_event_t * e);
static void color_event_cb(lv_event_t * e);
static void ta_event_cb(lv_event_t * e);
static void birthday_event_cb(lv_event_t * e);
static void calendar_event_cb(lv_event_t * e);
static void slider_event_cb(lv_event_t * e);
static void chart_event_cb(lv_event_t * e);
static void shop_chart_event_cb(lv_event_t * e);
static void meter1_indic1_anim_cb(void * var, int32_t v);
static void meter1_indic2_anim_cb(void * var, int32_t v);
static void meter1_indic3_anim_cb(void * var, int32_t v);
static void meter2_timer_cb(lv_timer_t * timer);
static void meter3_anim_cb(void * var, int32_t v);

/**********************
 *  STATIC VARIABLES
 **********************/
static disp_size_t disp_size;

static lv_obj_t * tv;
static lv_obj_t * calendar;
static lv_style_t style_text_muted;
static lv_style_t style_title;
static lv_style_t style_icon;
static lv_style_t style_bullet;

static lv_obj_t * meter1;
static lv_obj_t * meter2;
static lv_obj_t * meter3;

static lv_obj_t * chart1;
static lv_obj_t * chart2;
static lv_obj_t * chart3;

static lv_chart_series_t * ser1;
static lv_chart_series_t * ser2;
static lv_chart_series_t * ser3;
static lv_chart_series_t * ser4;

static const lv_font_t * font_large;
static const lv_font_t * font_normal;

static uint32_t session_desktop = 1000;
static uint32_t session_tablet = 1000;
static uint32_t session_mobile = 1000;

static lv_timer_t * meter2_timer;

static lv_obj_t * Head_background_image;
static lv_obj_t * Total_output_label;
static lv_obj_t * Date_label;
static lv_obj_t * Time_label;
static lv_obj_t * Battery_label;
static lv_obj_t * Battery_image;
static lv_obj_t * win_content;

#define CANVAS_WIDTH      1024
#define CANVAS_HEIGHT     150

#define PRINT_KEY_NUM     130   // PE2
#define ENCODER_KEY_NUM   132   // PE4
#define PH45_164_CLK_NUM  136   // PE8
#define PH45_164_AB_NUM   135   // PE7
#define PH45_595_SER1_NUM 138   // PE10
#define PH45_595_SER2_NUM 137   // PE9
#define PH45_595_CLK_NUM  134   // PE6
#define PH45_595_RCK_NUM  133   // PE5

#define PH45_GPIO_STATUS_HIGH  1
#define PH45_GPIO_STATUS_LOW   0

#define GPIO_REG_BASE   0x01C20800      //GPIO物理基地址 (小页4kb)
#define MAP_SIZE        0x400 //MMU页大小
#define GPIO_BASE_OFFSET (GPIO_REG_BASE & 0X00000FFF) //GPIO基地址偏移计算
#define GPIO_PAGE_OFFSET (GPIO_REG_BASE & 0XFFFFF000) //获得页偏移

#define rPE_CFG0 0X90  //PE_CFG0寄存器地址偏移  PE0~PE7
#define rPE_CFG1 0X94  //PE_CFG0寄存器地址偏移  PE8~PE15
#define rPE_DAT 0XA0    //PE_DAT寄存器地址偏移
#define rPE_PULL0 0XAC  //PE_PULL0寄存器地址偏移

typedef struct _PRINT_DATA_
{
    unsigned int data_len;
    unsigned int data_hight;
    unsigned int data_width;
    unsigned int nozzle; // 0:left;1:right
    unsigned char data[CANVAS_WIDTH * CANVAS_HEIGHT];
}PRINT_DATA;          

/* 第一步：为画布申请缓冲区内存 */
static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_WIDTH,CANVAS_HEIGHT)];
static unsigned int buzzer_enable = 0;
static unsigned int buzzer_count = 0;
static int fd_buzzer;
static int fd_mp1484_en;
static int fd_bm8563;
static int fd_adc;
static int print_width[CANVAS_HEIGHT] = {0};
static int print_button_enable = 0;
static int fd_gpio_in;
static unsigned int key_value;
static unsigned int encoder_enalbe = 0;
static unsigned int encoder_low_flag = 0;
static unsigned int encoder_count = 0;
//static unsigned int encoder_count_last = 0;
static int fd_ph45;
//static unsigned char g_odd_even = 1; //jishu
//static unsigned char *map_base;
static PRINT_DATA print_data;
static char g_time_data_string[16]={0};
static char g_time_time_string[16]={0};
static lv_obj_t * label_print;
static lv_obj_t * imgbtn_print;
unsigned int g_total_output = 0;
const char* config_file = "/root/config.conf";


//文件管理
lv_obj_t* file_management_win;
lv_obj_t* file_management_head_bg;
lv_obj_t* label_file_management_total_output;
lv_obj_t* label_file_management;
lv_obj_t* label_file_management_date;
lv_obj_t* label_file_management_time;
lv_obj_t* img_file_management_battery_level;
lv_obj_t* label_file_management_label;
lv_obj_t* file_management_middle_bg;
lv_obj_t* img_file_management_text;
lv_obj_t* label_file_management_text;
lv_obj_t* label_file_management_text_label;
lv_obj_t* img_file_management_text_time;
lv_obj_t* label_file_management_text_time;
lv_obj_t* label_file_management_text_time_label;
lv_obj_t* img_file_management_typeface;
lv_obj_t* label_file_management_typeface;
lv_obj_t* label_file_management_typeface_label;
lv_obj_t* img_file_management_word_space;
lv_obj_t* label_file_management_word_space;
lv_obj_t* label_file_management_word_space_label;
lv_obj_t* img_file_management_word_size;
lv_obj_t* label_file_management_word_size;
lv_obj_t* label_file_management_word_size_label;
lv_obj_t* img_file_management_spin;
lv_obj_t* label_file_management_spin;
lv_obj_t* label_file_management_spin_label;
lv_obj_t* img_file_management_width;
lv_obj_t* label_file_management_width;
lv_obj_t* label_file_management_width_label;
lv_obj_t* img_file_management_bold;
lv_obj_t* label_file_management_bold;
lv_obj_t* sw_file_management_bold;
lv_obj_t* img_file_management_line_increment;
lv_obj_t* label_file_management_line_increment;
lv_obj_t* label_file_management_line_increment_label;
lv_obj_t* img_management_line_italic;
lv_obj_t* label_management_line_italic;
lv_obj_t* sw_management_line_italic;
lv_obj_t* file_management_bottom_bg;
lv_obj_t* label_file_management_return;
lv_obj_t* imgbtn_file_management_get_back;
lv_obj_t* imgbtn_file_management_confirm;
lv_obj_t* label_file_management_confirm;
lv_obj_t* btn_file_management_text;
lv_obj_t* label_keyboard_return;
lv_obj_t* btn_keyboard_return;
lv_obj_t* label_keyboard_confirm;
lv_obj_t* btn_keyboard_confirm;
lv_obj_t* keyboard_cand_panel;
lv_obj_t* text_edit_keyboard;
lv_obj_t* textarea_text_keyboard;
lv_obj_t* pinyin_ime;
lv_obj_t* textarea_bg_keyboard;
lv_obj_t* keyboard_bg;
static unsigned char print_status = 0;
lv_obj_t* main_canvas;
lv_ft_info_t main_info;
lv_draw_label_dsc_t main_label_dsc;
lv_color_t canvasBuf[(32 * 1024) / 8 * 150];
lv_timer_t * timer_100ms;

unsigned short get_print_data_for_each_column(unsigned int line_number,unsigned int A_number,unsigned char odd_even);
void set_mp1484_en(unsigned char gpio_status);
void delay_100ns(unsigned char ns);
void p_74hc595d(unsigned short data);
void initialize(void);
void function_20240522(void);
void set_PH45_GPIO(unsigned char gpio_num,unsigned char gpio_status);
void set_mp1484_en(unsigned char gpio_status);
int get_max(int buf[],int len);
void print_event_cb(lv_event_t *e);
void slider_show_2(void);


//获取1列的打印数据
unsigned short get_print_data_for_each_column(unsigned int line_number,unsigned int A_number,unsigned char odd_even)
{
    unsigned int nozzle_number;
    unsigned short ret = 0;

    //偶数
    if (odd_even == 0)
    {
        nozzle_number = line_number * 2 + 2;
    }
    //奇数
    else
    {
        nozzle_number = line_number * 2 + 1;
    }
    switch (A_number)
    {
        case 1:
            switch (nozzle_number)
            {
                case 1:
                    ret = 0x0001;
                    break;
                case 45:
                    ret = 0x0004;
                    break;
                case 42:
                    ret = 0x0008;
                    break;
                case 89:
                    ret = 0x0010;
                    break;
                case 86:
                    ret = 0x0020;
                    break;
                case 133:
                    ret = 0x0040;
                    break;
                case 130:
                    ret = 0x0080;
                    break;
                case 177:
                    ret = 0x0100;
                    break;
                case 174:
                    ret = 0x0200;
                    break;
                case 221:
                    ret = 0x0400;
                    break;
                case 218:
                    ret = 0x0800;
                    break;
                case 265:
                    ret = 0x1000;
                    break;
                case 262:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;
        case 2:
            switch (nozzle_number)
            {
                case 7:
                    ret = 0x0001;
                    break;
                case 4:
                    ret = 0x0002;
                    break;
                case 51:
                    ret = 0x0004;
                    break;
                case 48:
                    ret = 0x0008;
                    break;
                case 95:
                    ret = 0x0010;
                    break;
                case 92:
                    ret = 0x0020;
                    break;
                case 139:
                    ret = 0x0040;
                    break;
                case 136:
                    ret = 0x0080;
                    break;
                case 183:
                    ret = 0x0100;
                    break;
                case 180:
                    ret = 0x0200;
                    break;
                case 227:
                    ret = 0x0400;
                    break;
                case 224:
                    ret = 0x0800;
                    break;
                case 271:
                    ret = 0x1000;
                    break;
                case 268:
                    ret = 0x2000;
                    break;
                default:
                        ret = 0x00;
                        break;
            }
            break;
        case 3:
            switch (nozzle_number)
            {
                case 13:
                    ret = 0x0001;
                    break;
                case 10:
                    ret = 0x0002;
                    break;
                case 57:
                    ret = 0x0004;
                    break;
                case 54:
                    ret = 0x0008;
                    break;
                case 101:
                    ret = 0x0010;
                    break;
                case 98:
                    ret = 0x0020;
                    break;
                case 145:
                    ret = 0x0040;
                    break;
                case 142:
                    ret = 0x0080;
                    break;
                case 189:
                    ret = 0x0100;
                    break;
                case 186:
                    ret = 0x0200;
                    break;
                case 233:
                    ret = 0x0400;
                    break;
                case 230:
                    ret = 0x0800;
                    break;
                case 277:
                    ret = 0x1000;
                    break;
                case 274:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 4:
            switch (nozzle_number)
            {
                case 19:
                    ret = 0x0001;
                    break;
                case 16:
                    ret = 0x0002;
                    break;
                case 63:
                    ret = 0x0004;
                    break;
                case 60:
                    ret = 0x0008;
                    break;
                case 107:
                    ret = 0x0010;
                    break;
                case 104:
                    ret = 0x0020;
                    break;
                case 151:
                    ret = 0x0040;
                    break;
                case 148:
                    ret = 0x0080;
                    break;
                case 195:
                    ret = 0x0100;
                    break;
                case 192:
                    ret = 0x0200;
                    break;
                case 239:
                    ret = 0x0400;
                    break;
                case 236:
                    ret = 0x0800;
                    break;
                case 283:
                    ret = 0x1000;
                    break;
                case 280:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 5:
            switch (nozzle_number)
            {
                case 25:
                    ret = 0x0001;
                    break;
                case 22:
                    ret = 0x0002;
                    break;
                case 69:
                    ret = 0x0004;
                    break;
                case 66:
                    ret = 0x0008;
                    break;
                case 113:
                    ret = 0x0010;
                    break;
                case 110:
                    ret = 0x0020;
                    break;
                case 157:
                    ret = 0x0040;
                    break;
                case 154:
                    ret = 0x0080;
                    break;
                case 201:
                    ret = 0x0100;
                    break;
                case 198:
                    ret = 0x0200;
                    break;
                case 245:
                    ret = 0x0400;
                    break;
                case 242:
                    ret = 0x0800;
                    break;
                case 289:
                    ret = 0x1000;
                    break;
                case 286:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 6:
            switch (nozzle_number)
            {
                case 31:
                    ret = 0x0001;
                    break;
                case 28:
                    ret = 0x0002;
                    break;
                case 75:
                    ret = 0x0004;
                    break;
                case 72:
                    ret = 0x0008;
                    break;
                case 119:
                    ret = 0x0010;
                    break;
                case 116:
                    ret = 0x0020;
                    break;
                case 163:
                    ret = 0x0040;
                    break;
                case 160:
                    ret = 0x0080;
                    break;
                case 207:
                    ret = 0x0100;
                    break;
                case 204:
                    ret = 0x0200;
                    break;
                case 251:
                    ret = 0x0400;
                    break;
                case 248:
                    ret = 0x0800;
                    break;
                case 295:
                    ret = 0x1000;
                    break;
                case 292:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 7:
            switch (nozzle_number)
            {
                case 37:
                    ret = 0x0001;
                    break;
                case 34:
                    ret = 0x0002;
                    break;            
                case 81:
                    ret = 0x0004;
                    break;
                case 78:
                    ret = 0x0008;
                    break;
                case 125:
                    ret = 0x0010;
                    break;
                case 122:
                    ret = 0x0020;
                    break;
                case 169:
                    ret = 0x0040;
                    break;
                case 166:
                    ret = 0x0080;
                    break;
                case 213:
                    ret = 0x0100;
                    break;
                case 210:
                    ret = 0x0200;
                    break;
                case 257:
                    ret = 0x0400;
                    break;
                case 254:
                    ret = 0x0800;
                    break;
                case 298:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;


        case 8:
            switch (nozzle_number)
            {
                case 40:
                    ret = 0x0002;
                    break;
                case 43:
                    ret = 0x0004;
                    break;
                case 84:
                    ret = 0x0008;
                    break;
                case 87:
                    ret = 0x0010;
                    break;
                case 128:
                    ret = 0x0020;
                    break;
                case 131:
                    ret = 0x0040;
                    break;
                case 172:
                    ret = 0x0080;
                    break;
                case 175:
                    ret = 0x0100;
                    break;
                case 216:
                    ret = 0x0200;
                    break;
                case 219:
                    ret = 0x0400;
                    break;
                case 260:
                    ret = 0x0800;
                    break;
                case 263:
                    ret = 0x1000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 9:
            switch (nozzle_number)
            {
                case 5:
                    ret = 0x0001;
                    break;
                case 2:
                    ret = 0x0002;
                    break;
                case 49:
                    ret = 0x0004;
                    break;
                case 46:
                    ret = 0x0008;
                    break;
                case 93:
                    ret = 0x0010;
                    break;
                case 90:
                    ret = 0x0020;
                    break;
                case 137:
                    ret = 0x0040;
                    break;
                case 134:
                    ret = 0x0080;
                    break;
                case 181:
                    ret = 0x0100;
                    break;
                case 178:
                    ret = 0x0200;
                    break;
                case 225:
                    ret = 0x0400;
                    break;
                case 222:
                    ret = 0x0800;
                    break;
                case 269:
                    ret = 0x1000;
                    break;
                case 266:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 10:
            switch (nozzle_number)
            {
                case 11:
                    ret = 0x0001;
                    break;
                case 8:
                    ret = 0x0002;
                    break;
                case 55:
                    ret = 0x0004;
                    break;
                case 52:
                    ret = 0x0008;
                    break;
                case 99:
                    ret = 0x0010;
                    break;
                case 96:
                    ret = 0x0020;
                    break;
                case 143:
                    ret = 0x0040;
                    break;
                case 140:
                    ret = 0x0080;
                    break;
                case 187:
                    ret = 0x0100;
                    break;
                case 184:
                    ret = 0x0200;
                    break;
                case 231:
                    ret = 0x0400;
                    break;
                case 228:
                    ret = 0x0800;
                    break;
                case 275:
                    ret = 0x1000;
                    break;
                case 272:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 11:
            switch (nozzle_number)
            {
                case 17:
                    ret = 0x0001;
                    break;
                case 14:
                    ret = 0x0002;
                    break;
                case 61:
                    ret = 0x0004;
                    break;
                case 58:
                    ret = 0x0008;
                    break;
                case 105:
                    ret = 0x0010;
                    break;
                case 102:
                    ret = 0x0020;
                    break;
                case 149:
                    ret = 0x0040;
                    break;
                case 146:
                    ret = 0x0080;
                    break;
                case 193:
                    ret = 0x0100;
                    break;
                case 190:
                    ret = 0x0200;
                    break;
                case 237:
                    ret = 0x0400;
                    break;
                case 234:
                    ret = 0x0800;
                    break;
                case 281:
                    ret = 0x1000;
                    break;
                case 278:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 12:
            switch (nozzle_number)
            {
                case 23:
                    ret = 0x0001;
                    break;
                case 20:
                    ret = 0x0002;
                    break;
                case 67:
                    ret = 0x0004;
                    break;
                case 64:
                    ret = 0x0008;
                    break;
                case 111:
                    ret = 0x0010;
                    break;
                case 108:
                    ret = 0x0020;
                    break;
                case 155:
                    ret = 0x0040;
                    break;
                case 152:
                    ret = 0x0080;
                    break;
                case 199:
                    ret = 0x0100;
                    break;
                case 196:
                    ret = 0x0200;
                    break;
                case 243:
                    ret = 0x0400;
                    break;
                case 240:
                    ret = 0x0800;
                    break;
                case 287:
                    ret = 0x1000;
                    break;
                case 284:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 13:
            switch (nozzle_number)
            {
                case 29:
                    ret = 0x0001;
                    break;
                case 26:
                    ret = 0x0002;
                    break;
                case 73:
                    ret = 0x0004;
                    break;
                case 70:
                    ret = 0x0008;
                    break;
                case 117:
                    ret = 0x0010;
                    break;
                case 114:
                    ret = 0x0020;
                    break;
                case 161:
                    ret = 0x0040;
                    break;
                case 158:
                    ret = 0x0080;
                    break;
                case 205:
                    ret = 0x0100;
                    break;
                case 202:
                    ret = 0x0200;
                    break;
                case 249:
                    ret = 0x0400;
                    break;
                case 246:
                    ret = 0x0800;
                    break;
                case 293:
                    ret = 0x1000;
                    break;
                case 290:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 14:
            switch (nozzle_number)
            {
                case 35:
                    ret = 0x0001;
                    break;
                case 32:
                    ret = 0x0002;
                    break;
                case 79:
                    ret = 0x0004;
                    break;
                case 76:
                    ret = 0x0008;
                    break;
                case 123:
                    ret = 0x0010;
                    break;
               case 120:
                    ret = 0x0020;
                    break;
                case 167:
                    ret = 0x0040;
                    break;
                case 164:
                    ret = 0x0080;
                    break;
                case 211:
                    ret = 0x0100;
                    break;
                case 208:
                    ret = 0x0200;
                    break;
                case 255:
                    ret = 0x0400;
                    break;
                case 252:
                    ret = 0x0800;
                    break;
                case 299:
                    ret = 0x1000;
                    break;
                case 296:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 15:
            switch(nozzle_number)
            {
                case 38:
                    ret = 0x0002;
                    break;
                case 41:
                    ret = 0x0004;
                    break;
                case 82:
                    ret = 0x0008;
                    break;
                case 85:
                    ret = 0x0010;
                    break;
                case 126:
                    ret = 0x0020;
                    break;
                case 129:
                    ret = 0x0040;
                    break;
                case 170:
                    ret = 0x0080;
                    break;
                case 173:
                    ret = 0x0100;
                    break;
                case 214:
                    ret = 0x0200;
                    break;
                case 217:
                    ret = 0x0400;
                    break;
                case 258:
                    ret = 0x0800;
                    break;
                case 261:
                    ret = 0x1000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 16:
            switch (nozzle_number)
            {
                case 3:
                    ret = 0x0001;
                    break;
                case 47:
                    ret = 0x0004;
                    break;
                case 44:
                    ret = 0x0008;
                    break;
                case 91:
                    ret = 0x0010;
                    break;
                case 88:
                    ret = 0x0020;
                    break;
                case 135:
                    ret = 0x0040;
                    break;
                case 132:
                    ret = 0x0080;
                    break;
                case 179:
                    ret = 0x0100;
                    break;
                case 176:
                    ret = 0x0200;
                    break;
                case 223:
                    ret = 0x0400;
                    break;
                case 220:
                    ret = 0x0800;
                    break;
                case 267:
                    ret = 0x1000;
                    break;
                case 264:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 17:
            switch (nozzle_number)
            {
                case 9:
                    ret = 0x0001;
                    break;
                case 6:
                    ret = 0x0002;
                    break;
                case 53:
                    ret = 0x0004;
                    break;
                case 50:
                    ret = 0x0008;
                    break;
                case 97:
                    ret = 0x0010;
                    break;
                case 94:
                    ret = 0x0020;
                    break;
                case 141:
                    ret = 0x0040;
                    break;
                case 138:
                    ret = 0x0080;
                    break;
                case 185:
                    ret = 0x0100;
                    break;
                case 182:
                    ret = 0x0200;
                    break;
                case 229:
                    ret = 0x0400;
                    break;
                case 226:
                    ret = 0x0800;
                    break;
                case 273:
                    ret = 0x1000;
                    break;
                case 270:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x00;
                    break;
            }
            break;

        case 18:
            switch (nozzle_number)
            {
                case 15:
                    ret = 0x0001;
                    break;
                case 12:
                    ret = 0x0002;
                    break;
                case 59:
                    ret = 0x0004;
                    break;
                case 56:
                    ret = 0x0008;
                    break;
                case 103:
                    ret = 0x0010;
                    break;
                case 100:
                    ret = 0x0020;
                    break;
                case 147:
                    ret = 0x0040;
                    break;
                case 144:
                    ret = 0x0080;
                    break;
                case 191:
                    ret = 0x0100;
                    break;
                case 188:
                    ret = 0x0200;
                    break;
                case 235:
                    ret = 0x0400;
                    break;
                case 232:
                    ret = 0x0800;
                    break;
                case 279:
                    ret = 0x1000;
                    break;
                case 276:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 19:
            switch (nozzle_number)
            {
                case 21:
                    ret = 0x0001;
                    break;
                case 18:
                    ret = 0x0002;
                    break;
                case 65:
                    ret = 0x0004;
                    break;
                case 62:
                    ret = 0x0008;
                    break;
                case 109:
                    ret = 0x0010;
                    break;
                case 106:
                    ret = 0x0020;
                    break;
                case 153:
                    ret = 0x0040;
                    break;
                case 150:
                    ret = 0x0080;
                    break;
                case 197:
                    ret = 0x0100;
                    break;
                case 194:
                    ret = 0x0200;
                    break;
                case 241:
                    ret = 0x0400;
                    break;
                case 238:
                    ret = 0x0800;
                    break;
                case 285:
                    ret = 0x1000;
                    break;
                case 282:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;


        case 20:
            switch (nozzle_number)
            {
                case 27:
                    ret = 0x0001;
                    break;
                case 24:
                    ret = 0x0002;
                    break;
                case 71:
                    ret = 0x0004;
                    break;
                case 68:
                    ret = 0x0008;
                    break;
                case 115:
                    ret = 0x0010;
                    break;
                case 112:
                    ret = 0x0020;
                    break;
                case 159:
                    ret = 0x0040;
                    break;
                case 156:
                    ret = 0x0080;
                    break;
                case 203:
                    ret = 0x0100;
                    break;
                case 200:
                    ret = 0x0200;
                    break;
                case 247:
                    ret = 0x0400;
                    break;
                case 244:
                    ret = 0x0800;
                    break;
                case 291:
                    ret = 0x0100;
                    break;
                case 288:
                    ret = 0x0200;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;

        case 21:
            switch (nozzle_number)
            {
                case 33:
                    ret = 0x0001;
                    break;
                case 30:
                    ret = 0x0002;
                    break;
                case 77:
                    ret = 0x0004;
                    break;
                case 74:
                    ret = 0x0008;
                    break;
                case 121:
                    ret = 0x0010;
                    break;
                case 118:
                    ret = 0x0020;
                    break;
                case 165:
                    ret = 0x0040;
                    break;
                case 162:
                    ret = 0x0080;
                    break;
                case 209:
                    ret = 0x0100;
                    break;
                case 206:
                    ret = 0x0200;
                    break;
                case 253:
                    ret = 0x0400;
                    break;
                case 250:
                    ret = 0x0800;
                    break;
                case 297:
                    ret = 0x1000;
                    break;
                case 294:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;


        case 22:
            switch (nozzle_number)
            {
                case 39:
                    ret = 0x0001;
                    break;
                case 36:
                    ret = 0x0002;
                    break;
                case 83:
                    ret = 0x0004;
                    break;
                case 80:
                    ret = 0x0008;
                    break;
                case 127:
                    ret = 0x0010;
                    break;
                case 124:
                    ret = 0x0020;
                    break;
                case 171:
                    ret = 0x0040;
                    break;
                case 168:
                    ret = 0x0080;
                    break;
                case 215:
                    ret = 0x0100;
                    break;
                case 212:
                    ret = 0x0200;
                    break;
                case 259:
                    ret = 0x0400;
                    break;
                case 256:
                    ret = 0x0800;
                    break;
                case 300:
                    ret = 0x2000;
                    break;
                default:
                    ret = 0x0000;
                    break;
            }
            break;
    }
 
    return ret;
}

void set_PH45_GPIO(unsigned char gpio_num,unsigned char gpio_status)
{
    unsigned char databuf[2];
    int retvalue;

    databuf[0] = gpio_status;
    databuf[1] = gpio_num;
    retvalue = write(fd_ph45, databuf, sizeof(databuf));      
    if(retvalue < 0)
    {
        printf("fd_ph45 Control Failed!\r\n");
        close(fd_ph45);
    }    
}

void set_mp1484_en(unsigned char gpio_status)
{
    unsigned char databuf[2];
    int retvalue;

    databuf[0] = gpio_status;
    retvalue = write(fd_mp1484_en, databuf, 1);      
    if(retvalue < 0)
    {
        printf("set_mp1484_en Control Failed!\r\n");
        close(fd_mp1484_en);
    }    
}

void delay_100ns(unsigned char ns)
{
    unsigned char i;
    for(i=0;i<ns;i++);
}
#if 0
void p_74hc595d(unsigned short data)
{
    int i;
    unsigned char ser1;
    unsigned char ser2;
    unsigned int PE_DAT;
/*
    PS1 = Q20 = SER2
    PS2 = Q21 = SER2
    PS3 = Q22 = SER2
    PS4 = Q23 =SER2
    PS5 = Q24 = SER2
    PS6 = Q25 = SER2
    PS7 = Q26 = SER2
    PS8 = Q10 = SER1
    PS9 = Q11 = SER1
    PS10 = Q12 = SER1
    PS11 = Q13 = SER1
    PS12 = Q14 = SER1
    PS13 = Q15 = SER1
    PS14 = Q16 = SER1
*/
#if 0
    set_PH45_GPIO(PH45_595_SER1_NUM,PH45_GPIO_STATUS_LOW);
    set_PH45_GPIO(PH45_595_SER2_NUM,PH45_GPIO_STATUS_LOW);
    set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_LOW);
    delay_100ns(1);
    set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_HIGH);
    set_PH45_GPIO(PH45_595_SER1_NUM,PH45_GPIO_STATUS_HIGH);
    set_PH45_GPIO(PH45_595_SER2_NUM,PH45_GPIO_STATUS_HIGH);  
#endif
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFF9FF)); //PE9,PE10,LOW
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF)); //PE6,LOW
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF) | 0X00000040); //PE6,HIGI
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFF9FF) | 0x00000600); //PE9,PE10,HIGH

    ser1 = (data >> 8) & 0xff;
    ser2 = data & 0xff;
  
    for(i=0;i<7;i++)
    {
        //delay_100ns(1);
        //set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_LOW);
        PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
        *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF)); //PE6,LOW       
        //delay_100ns(1);
        if((ser1 >> i) & 0x01)
        {
            //set_PH45_GPIO(PH45_595_SER1_NUM,PH45_GPIO_STATUS_LOW);
            PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFBFF)); //PE10,LOW                
        }
        else
        {
            //set_PH45_GPIO(PH45_595_SER1_NUM,PH45_GPIO_STATUS_HIGH);
            PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFBFF) | 0X00000400); //PE10,HIGH
        }

        if((ser2 >> i) & 0x01)
        {
            //set_PH45_GPIO(PH45_595_SER2_NUM,PH45_GPIO_STATUS_LOW);
            PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFDFF)); //PE9,LOW    
        }
        else
        {
            //set_PH45_GPIO(PH45_595_SER2_NUM,PH45_GPIO_STATUS_HIGH);
            PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFDFF) | 0x00000200); //PE9,HIGH    
        }
        
        //set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_HIGH);
        PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
        *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF) | 0x00000040); //PE6,high   
    }

#if 0
    delay_100ns(1);
    set_PH45_GPIO(PH45_595_RCK_NUM,PH45_GPIO_STATUS_LOW);
    delay_100ns(1);
    set_PH45_GPIO(PH45_595_RCK_NUM,PH45_GPIO_STATUS_HIGH);
    delay_100ns(1);
#endif
    delay_100ns(1);
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFDF)); //PE5,LOW
    delay_100ns(1);
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFDF) | 0X00000020); //PE5,high

#if 0
    set_PH45_GPIO(PH45_595_SER1_NUM,PH45_GPIO_STATUS_LOW);
    set_PH45_GPIO(PH45_595_SER2_NUM,PH45_GPIO_STATUS_LOW);
    set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_LOW);
    delay_100ns(1);
    set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_HIGH);
    set_PH45_GPIO(PH45_595_SER1_NUM,PH45_GPIO_STATUS_HIGH);
    set_PH45_GPIO(PH45_595_SER2_NUM,PH45_GPIO_STATUS_HIGH);  
#endif
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFF9FF)); //PE9,PE10,LOW
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF)); //PE6,LOW
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF) | 0X00000040); //PE6,HIGH
    //delay_100ns(1);
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFF9FF) | 0x00000600); //PE9,PE10,HIGH


    for(i=0;i<7;i++)
    {
        //delay_100ns(1);
        //set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_LOW);
        PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
        *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF)); //PE6,LOW   
        //delay_100ns(1);
        //set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_HIGH);
        PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
        *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF) | 0x00000040); //PE6,high  
    }

#if 0
    delay_100ns(1);
    set_PH45_GPIO(PH45_595_RCK_NUM,PH45_GPIO_STATUS_LOW);
    delay_100ns(1);
    set_PH45_GPIO(PH45_595_RCK_NUM,PH45_GPIO_STATUS_HIGH);
    delay_100ns(1);
#endif
    //delay_100ns(1);
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFDF)); //PE5,LOW
    //delay_100ns(1);
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFDF) | 0X00000020); //PE5,high

    //set_PH45_GPIO(PH45_164_AB_NUM,PH45_GPIO_STATUS_LOW);
    //set_PH45_GPIO(PH45_164_CLK_NUM,PH45_GPIO_STATUS_LOW);
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFF7F)); //PE7,LOW
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFEFF)); //PE8,LOW
    //delay_100ns(1);
    //set_PH45_GPIO(PH45_164_CLK_NUM,PH45_GPIO_STATUS_HIGH);
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFEFF) | 0X00000100); //PE8,HIGH
}
#endif

void initialize(void)
{

    fd_ph45 = open("/dev/hp45",O_RDWR | O_SYNC);
	if (fd_ph45 < 0)
	{
		printf("can not open file hp45\n");
	}
    else
    {
        printf("initialize hp45 ok\n");
    }

#if 0
    unsigned int PE_CFG0;
    unsigned int PE_CFG1;
    unsigned int PE_PULL0;
#endif


    fd_gpio_in = open("/dev/key_print", O_RDWR);
	if (fd_gpio_in < 0)
	{
		printf("can not open file gpio_in\n");
	}


#if 0
    fd_ph45 = open("/dev/hp45",O_RDWR | O_SYNC);
	if (fd_ph45 < 0)
	{
		printf("can not open file hp45\n");
	}
    else
    {
        printf("initialize\n");
        set_PH45_GPIO(PH45_595_SER1_NUM,PH45_GPIO_STATUS_HIGH);
        set_PH45_GPIO(PH45_595_SER2_NUM,PH45_GPIO_STATUS_HIGH);
        set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_HIGH);
        set_PH45_GPIO(PH45_595_RCK_NUM,PH45_GPIO_STATUS_HIGH);
        set_PH45_GPIO(PH45_164_CLK_NUM,PH45_GPIO_STATUS_HIGH);
        set_PH45_GPIO(PH45_164_AB_NUM,PH45_GPIO_STATUS_HIGH);
    }
#endif
#if 0
    // 设置当前进程的优先级为最高
    if (setpriority(PRIO_PROCESS, 0, -20) == -1) 
    {
        printf("setpriority error \n");
    }
 
    // 输出当前进程的优先级
    int priority = getpriority(PRIO_PROCESS, 0);
    printf("Current priority: %d\n", priority);
#endif
 
    // 这里可以执行你的代码，进程将不会被抢占

#if 0
    fd_ph45 = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd_ph45 < 0) 
    {
        printf("open error\n");
    }
    map_base = (unsigned char *)mmap(NULL, 0x400,PROT_READ | PROT_WRITE, MAP_SHARED,fd_ph45, GPIO_PAGE_OFFSET); //把物理地址映射到虚拟地址
    if(*map_base)  
    {
        printf("mmap_fail!\n"); //是否映射成功
    }

    PE_CFG0=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_CFG0);
    PE_CFG1=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_CFG1);
    PE_PULL0=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_PULL0);

    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_CFG0)=((PE_CFG0 & 0X9998F8FF)|0X00080000);//PE2,PE4,PE5,PE6,PE7
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_CFG1)=((PE_CFG1 & 0XFFFFF000)|0X00000111);//PE8,PE9,PE10
    *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_PULL0)=((PE_PULL0 & 0XFFFFFCCF)|0X00000220);//
#endif

}


void function_20240522(void)
{
    int retvalue;

    if(print_button_enable == 1)
    {
        print_button_enable = 0;
        sleep(1);

        lv_timer_pause(timer_100ms);

        retvalue = write(fd_ph45,(unsigned char *)&print_data, sizeof(print_data));      
        if(retvalue < 0)
        {
            printf("fd_ph45 Control Failed!\r\n");
            close(fd_ph45);
        }

        lv_timer_resume(timer_100ms);
    }   
}

#if 0
void function(void)
{
    unsigned short data;
    int i,j,k;
    static int column = 0;
    unsigned int PE_CFG0;
    unsigned int PE_CFG1;
    unsigned int PE_DAT;
    unsigned char writeData;

    read(fd_gpio_in, &key_value, 4); 
    if((key_value >> 8) == PRINT_KEY_NUM)
    {
        if((key_value & 0xff)== 0x00)
        {
            printf("PRINT_KEY_NUM\n");
            buzzer_enable = 1;
            if((print_button_enable == 1)&&(encoder_enalbe == 0))
            {
                encoder_enalbe = 1;
                encoder_count = 0;
                encoder_low_flag = 0;
            }
        }
    }

#if 0
    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);

    if((PE_DAT & 0x00000004) == 0x00000000)
    {
         msleep(100);
        if((PE_DAT & 0x00000004) == 0x00000000)
        {
            printf("PRINT_KEY_NUM\n");
            buzzer_enable = 1;
            if((print_button_enable == 1)&&(encoder_enalbe == 0))
            {
                encoder_enalbe = 1;
                encoder_count = 0;
                encoder_low_flag = 0;
            }
        }
    }

    if(encoder_enalbe == 2)
    {
        
        if((PE_DAT & 0x00000010) == 0x00000010)
        {
            if(encoder_low_flag == 0)
            {
                encoder_low_flag = 1;
            }
        }
        else if((PE_DAT & 0x00000010) == 0x00000000)
        {
            if(encoder_low_flag == 1)
            {
                encoder_low_flag = 0;
                encoder_count++;
            }
        }
#endif

    if(print_button_enable == 1)
    {
        //printf("key_value=0x%x\n",key_value);
        //if((key_value >> 8) == ENCODER_KEY_NUM)
        {
            encoder_count++;
            //printf("%u\n",encoder_count);
            if(encoder_count> 1000)
            {
                //printf("ENCODER_KEY_NUM\n");

                writeData = 0;
                write(fd_gpio_in, &writeData, 1); 

                PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=(PE_DAT & 0XFFFFFF7F); //PE5,PE6,PE8,PE9,PE10,HIGH,PE7,LOW

                //一列打印150个像素点
                //一次上升沿打印7个地址位
                //7*22=154
                //打印第一列，获取A1所有的地址位
                //单数喷嘴
                //unsigned char print_data[22] = {0};
                data = 0;
        
                //printf("column=%d\n",column);
#if 0
                set_PH45_GPIO(PH45_595_SER1_NUM,PH45_GPIO_STATUS_LOW);
                set_PH45_GPIO(PH45_595_SER2_NUM,PH45_GPIO_STATUS_LOW);
                set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_LOW);
                delay_100ns(1);
                set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_HIGH);
                set_PH45_GPIO(PH45_595_SER1_NUM,PH45_GPIO_STATUS_HIGH);
                set_PH45_GPIO(PH45_595_SER2_NUM,PH45_GPIO_STATUS_HIGH);
#endif

                PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFF9FF)); //PE9,PE10,LOW
                PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF)); //PE6,LOW
                PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF) | 0X00000040); //PE6,high
                PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
                *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFF9FF) | 0x00000600); //PE9,PE10,HIGH
            
#if 0
                for(i=0;i<7;i++)
                {
                    delay_100ns(1);
                    set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_LOW);
                    delay_100ns(1);
                    set_PH45_GPIO(PH45_595_CLK_NUM,PH45_GPIO_STATUS_HIGH);
                }
#endif
                for(i=0;i<7;i++)
                {
                    delay_100ns(1);
                    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	                *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF)); //PE6,LOW
                    delay_100ns(1);
                    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	                *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFBF) | 0X00000040); //PE6,high
                }

#if 0
                set_PH45_GPIO(PH45_595_RCK_NUM,PH45_GPIO_STATUS_LOW);
                delay_100ns(1);
                set_PH45_GPIO(PH45_595_RCK_NUM,PH45_GPIO_STATUS_HIGH);
#endif

                PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
                *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFDF)); //PE5,LOW
                delay_100ns(1);
                PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
                *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFFDF) | 0X00000020); //PE5,high

#if 0
                for(i=0;i<21;i++)
                {
                    set_PH45_GPIO(PH45_164_CLK_NUM,PH45_GPIO_STATUS_LOW);
                    delay_100ns(1);
                    set_PH45_GPIO(PH45_164_CLK_NUM,PH45_GPIO_STATUS_HIGH);
                    delay_100ns(1);
                }   
#endif
                for(i=0;i<21;i++)
                {
                    delay_100ns(1);
                    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	                *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFEFF)); //PE8,LOW
                    delay_100ns(1);
                    PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	                *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFEFF) | 0X00000100); //PE8,high
                }   
#if 0
                set_PH45_GPIO(PH45_164_AB_NUM,PH45_GPIO_STATUS_HIGH);
                set_PH45_GPIO(PH45_164_CLK_NUM,PH45_GPIO_STATUS_LOW);
                delay_100ns(1);
                set_PH45_GPIO(PH45_164_CLK_NUM,PH45_GPIO_STATUS_HIGH);
#endif
                PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFF7F) | 0X00000080); //PE7,high
                PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFEFF)); //PE8,LOW  
                PE_DAT=*(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT);
	            *(volatile unsigned int *)(map_base+GPIO_BASE_OFFSET+rPE_DAT)=((PE_DAT & 0XFFFFFEFF) | 0X00000100); //PE8,HIGH             

                for(k=0;k<22;k++)
                { 
                    for (j = 0; j < printHeight; j++)
                    {
                        if (printBuffer[j * printWidth + column] == 1)
                        {
                            //printf("j=%d i=%d\n", j, i);
                            data |= get_print_data_for_each_column(j, k, g_odd_even);
                        }
                    }
                    //printf("A=%d data=%u\n", k+1,data);
                    p_74hc595d(data);
                    
                    data = 0x00;
                }
#if 0
                if(fd_ph45) 
                {
                    close(fd_ph45);
                }
                munmap(map_base,MAP_SIZE);//解除映射关系
#endif
                writeData = 1;
                write(fd_gpio_in, &writeData, 1); 

                column++;
                if(column >= printWidth)
                {
                    encoder_enalbe = 0;
                    buzzer_enable = 1;
                    printf("end\n");
                    lv_timer_enable(1);
                    print_button_enable = 0;
                    encoder_count = 0;
                }
            }
            else
            {
                column = 0;
            }

        }      
    }
}
#endif

int get_max(int buf[],int len)
{
    int i;
    int ret;
    ret = buf[0];
    for(i=0;i<len;i++)
    {
        if(ret < buf[i])
        {
            ret = buf[i];
        }
    }

    return ret;
}


void print_event_cb(lv_event_t *e)
{
    unsigned char *pdata = NULL;
    unsigned char *pdata1 = NULL;
    unsigned char *pdata2 = NULL;
    unsigned char data = 0;
    int i,j;

    lv_event_code_t code = lv_event_get_code(e); /* 获取事件类型 */
  
    if (code == LV_EVENT_CLICKED)
    {
        printf("print_event_cb\n");
        buzzer_enable = 1;
        //get image
        //Gets the width you want to print
        if(print_button_enable == 0)
        {
            pdata = (unsigned char *)cbuf;

            for(i=0;i<CANVAS_HEIGHT;i++)
            {
                for(j=CANVAS_WIDTH-1;j>=0;j--)
                {
                    data = *(pdata + (i*1024+j) * 4);
                    if(data != 0xff)
                    {
                        print_width[i] = j;
                        break;
                    }
                }
            }
    #if 0
            for(i=0;i<CANVAS_HEIGHT;i++)
            {
                printf("%d\n",print_width[i]);
            }
    #endif
            
            print_data.data_width = get_max(print_width,CANVAS_HEIGHT);

            pdata1 = (unsigned char *)cbuf;
            print_data.data_hight = 0;
            for(i=CANVAS_HEIGHT-1;i>=0;i--)
            {
                for(j=0;j<CANVAS_WIDTH;j++)
                {
                    data = *(pdata1 + (i*1024+j) * 4);
                    if(data != 0xff)
                    {
                        print_data.data_hight = i;
                        break;
                    }
                }
                if(print_data.data_hight != 0)
                {
                    break;
                }
            }

            printf("printWidth=%d printHeight=%d\n",print_data.data_width,print_data.data_hight);
            print_data.nozzle = 0;
            pdata2 = (unsigned char *)cbuf;
            for(i=0;i<print_data.data_hight;i++)
            {
                for(j=0;j<print_data.data_width;j++)
                {
                    data = *(pdata2 + (i * 1024 + j)*4);
                    if (data > 127)
                    {
                        print_data.data[i*print_data.data_width + j] = 0;
                    }
                    else
                    {
                        print_data.data[i*print_data.data_width + j] = 1;
                    }
                    //printf("%u ", print_data.data[i*print_data.data_width + j]);
                }
                //printf("\n");
            }
            print_button_enable  = 1;
        }
        else if(print_button_enable == 1)
        {
            print_button_enable = 0;
        }
    }
}

static void timer_100ms_cb(lv_timer_t * t)
{
    int retvalue;
    char buf[2]={0};
    unsigned char databuf[16] = {0};
    static int time_count_1s = 0;
    static int adc_count_5s = 0;
    int voltage = 0;

    time_count_1s++;
    if(time_count_1s >= 10)
    {
        time_count_1s = 0;
        memset(databuf,0,sizeof(databuf));
        databuf[0] = 0x02;
        
        retvalue = read(fd_bm8563, databuf, 6);
        if(retvalue < 0)
        {
            printf("fd_bm8563 Failed!\r\n");
            close(fd_bm8563);
        }
        
        //printf("%u %u %u %u %u %u\n\r",databuf[0],databuf[1],databuf[2],databuf[3],databuf[4],databuf[5]);
        sprintf(g_time_data_string,"%04u/%02u/%02u",2000+databuf[0],databuf[1],databuf[2]);
        sprintf(g_time_time_string,"%02u:%02u:%02u",databuf[3],databuf[4],databuf[5]);
        lv_label_set_text(Date_label, g_time_data_string);
        lv_label_set_text(Time_label, g_time_time_string);
    }

    adc_count_5s++;
    if(adc_count_5s >= 500)
    {
        adc_count_5s = 0;
    }

    if(adc_count_5s == 5)
    {
        memset(databuf,0,sizeof(databuf));
        retvalue = read(fd_adc, databuf, sizeof(databuf));
		if(retvalue < 0)
		{
			printf("BEEP Control Failed!\r\n");
			close(fd_adc);
		}
		else
		{
			memcpy((unsigned char *)&voltage,databuf,4);
			printf("voltage=%u\n",voltage);

            if(voltage == 63)
            {
                lv_label_set_text(Battery_label, "100%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_FULL);
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 395, 5);
            }
            else if(voltage == 62)
            {
                lv_label_set_text(Battery_label, "90%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_FULL);      
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 5);         
            }
            else if(voltage == 61)
            {
                lv_label_set_text(Battery_label, "80%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_3);
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 5);
            }
            else if(voltage == 60)
            {
                lv_label_set_text(Battery_label, "70%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_3);   
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 5);            
            }
            else if(voltage == 59)
            {
                lv_label_set_text(Battery_label, "60%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_2);      
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 5);         
            }
            else if(voltage == 58)
            {
                lv_label_set_text(Battery_label, "50%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_2);
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 5);
            }
            else if(voltage == 57)
            {
                lv_label_set_text(Battery_label, "40%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_1);     
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 5);          
            }
            else if(voltage == 56)
            {
                lv_label_set_text(Battery_label, "30%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_1);     
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 5);          
            }
            else if(voltage == 55)
            {
                lv_label_set_text(Battery_label, "20%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_EMPTY);
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 5);
            }
            else if(voltage == 54)
            {
                lv_label_set_text(Battery_label, "10%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_EMPTY);     
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 5);          
            }
            else if(voltage < 54)
            {
                lv_label_set_text(Battery_label, "0%");
                lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_EMPTY);
                lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 5);
            }
		}
    }


    if(buzzer_enable == 1)
    {
        buzzer_count++;
        if(buzzer_count == 2)
        {
            buf[0] = 1;
	        retvalue = write(fd_buzzer, buf, 1);
	        if(retvalue < 0)
            {
		        printf("BEEP Control Failed!\r\n");
		        close(fd_buzzer);
            }
        }
        if(buzzer_count > 50)
        {
            buzzer_enable = 0;
            buf[0] = 0;
            retvalue = write(fd_buzzer, buf, 1);
	        if(retvalue < 0)
            {
		        printf("BEEP Control Failed!\r\n");
		        close(fd_buzzer);
            }

            if(encoder_enalbe == 1)
            {
                lv_timer_enable(0);
                encoder_enalbe = 2;
                printf("encoder_enalbe = 2\n");
            }
        }
    }
    else
    {
        buzzer_count = 0;
    }

    read(fd_gpio_in, &key_value, 4); 
    if((key_value >> 8) == PRINT_KEY_NUM)
    {
        if((key_value & 0xff)== 0x00)
        {
            printf("PRINT_KEY_NUM\n");
            buzzer_enable = 1;
            if((print_button_enable == 1)&&(encoder_enalbe == 0))
            {
                encoder_enalbe = 1;
                encoder_count = 0;
                encoder_low_flag = 0;
            }
        }
    }
}

static void main_slider_event_cb(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e); /* 获取触发源 */
    lv_event_code_t code = lv_event_get_code(e); /* 获取事件类型 */
    int32_t temp;
    if ((code == LV_EVENT_VALUE_CHANGED)&&(print_status == 0))
    {
        temp =  lv_slider_get_value(target); 
        //printf("temp=%d\n", temp);
        lv_obj_scroll_to_x(win_content, temp*7, LV_ANIM_ON);
    }
}

/*************************************************
 *  函数名称 :  slider_show_2
 *  参    数 ： 无
 *  函数功能 ： slider显示
 *************************************************/
void slider_show_2(void)
{
    static const lv_style_prop_t props[] = { LV_STYLE_BG_COLOR,0 };  //设置位置参数
    static lv_style_transition_dsc_t transition_dsc;               //设置转换变量
    lv_style_transition_dsc_init(&transition_dsc, props, lv_anim_path_linear, 480, 0, NULL); //初始化转换

    static lv_style_t style_main;                                  //创建样式
    static lv_style_t style_indicator;                             //创建样式
    static lv_style_t style_knob;                                  //创建样式
    static lv_style_t style_pressed_color;                         //创建样式
    lv_style_init(&style_main);                                    //初始化样式
    lv_style_set_bg_opa(&style_main, LV_OPA_TRANSP);                 //设置背景透明度
    lv_style_set_bg_color(&style_main, lv_color_hex3(0xffffff));        //设置背景颜色
    //lv_style_set_radius(&style_main,LV_RADIUS_CIRCLE);             //设置圆角
    lv_style_set_size_A(&style_main, 400, 15);
    lv_style_set_pad_ver(&style_main, 0);                          //设置上下边距

    lv_style_init(&style_indicator);                               //初始化样式
    lv_style_set_bg_opa(&style_indicator, LV_OPA_TRANSP);            //设置背景透明度
    lv_style_set_bg_color(&style_indicator, lv_color_hex3(0xffffff));//设置背景颜色
    // lv_style_set_radius(&style_indicator,LV_RADIUS_CIRCLE);        //设置圆角
    lv_style_set_transition(&style_indicator, &transition_dsc);     //设置转化

    LV_IMG_DECLARE(img_slider);

    lv_style_init(&style_knob);                                    //初始化样式
    lv_style_set_bg_opa(&style_knob, LV_OPA_TRANSP);                 //设置背景透明度
    //lv_style_set_bg_color(&style_knob,lv_palette_main(LV_PALETTE_GREY));//设置背景颜色
    lv_style_set_bg_img_src(&style_knob, &img_slider);
    lv_style_set_border_color(&style_knob, lv_palette_darken(LV_PALETTE_GREY, 3));//设置边框背景颜色
    lv_style_set_border_width(&style_knob, 0);                      //设置边框宽度
    // lv_style_set_radius(&style_knob,LV_RADIUS_CIRCLE);             //设置圆角
    lv_style_set_pad_all(&style_knob, 20);                           //设置边距
    lv_style_set_size_A(&style_knob, 20, 10);
    lv_style_set_transition(&style_knob, &transition_dsc);          //设置转化

    lv_style_init(&style_pressed_color);                           //初始化样式
    lv_style_set_bg_color(&style_pressed_color, lv_color_hex3(0xffffff));//设置背景颜色

    lv_obj_t* slider = lv_slider_create(lv_scr_act());            //创建滑块
    lv_obj_remove_style_all(slider);                               //移除所有样式

    lv_obj_add_style(slider, &style_main, LV_PART_MAIN);             //添加样式
    lv_obj_add_style(slider, &style_indicator, LV_PART_INDICATOR);   //添加样式 指示器
    lv_obj_add_style(slider, &style_pressed_color, LV_PART_INDICATOR | LV_STATE_PRESSED); //添加样式 指示器 按压状态
    lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);             //添加样式 圆头
    lv_obj_add_style(slider, &style_pressed_color, LV_PART_KNOB | LV_STATE_PRESSED);      //添加样式 圆头 按压状态
    lv_obj_align(slider, LV_ALIGN_TOP_LEFT, 39, 186);
    //lv_obj_center(slider);                                         //居中显示
    lv_obj_add_event_cb(slider, main_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void button_keyboard_return_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        printf("button_keyboard_return_event_cb\n");
        lv_obj_del(textarea_bg_keyboard);
        lv_obj_del(pinyin_ime);
        lv_obj_del(textarea_text_keyboard);
        lv_obj_del(keyboard_bg);
        lv_obj_del(label_keyboard_confirm);
        lv_obj_del(btn_keyboard_confirm);
        lv_obj_del(label_keyboard_return);
        lv_obj_del(btn_keyboard_return);

        lv_timer_resume(timer_100ms);
    }
}

static void button_keyboard_confirm_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        printf("button_keyboard_confirm_event_cb\n");
        //获取输入框数据给到文本显示框
        lv_label_set_text(label_file_management_text_label, lv_textarea_get_text(textarea_text_keyboard));
        lv_label_set_text(label_file_management_label, lv_textarea_get_text(textarea_text_keyboard));
        lv_obj_del(textarea_bg_keyboard);
        lv_obj_del(pinyin_ime);
        lv_obj_del(textarea_text_keyboard);
        lv_obj_del(keyboard_bg);
        lv_obj_del(label_keyboard_confirm);
        lv_obj_del(btn_keyboard_confirm);
        lv_obj_del(label_keyboard_return);
        lv_obj_del(btn_keyboard_return);

        lv_timer_resume(timer_100ms);
    }
}
static void button_left_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("left\n");
    }
}

static void button_down_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("down\n");
    }
}

static void button_add_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("add\n");
    }
}

static void button_reduce_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("reduce\n");
    }
}

static void button_print_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        printf("print\n");
        if (print_status == 0)
        {
            print_status = 1;
            LV_IMG_DECLARE(print_2);
            lv_imgbtn_set_src(imgbtn_print, LV_IMGBTN_STATE_RELEASED, NULL, &print_2, NULL);
            lv_obj_set_size(imgbtn_print, print_2.header.w, print_2.header.h);
            lv_obj_align(imgbtn_print, LV_ALIGN_TOP_LEFT, 425, 207);

            LV_FONT_DECLARE(heiFont16_1);
            lv_obj_set_style_text_font(label_print, &heiFont16_1, LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(label_print, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
            lv_label_set_text(label_print, "打印中");
            lv_obj_align(label_print, LV_ALIGN_TOP_LEFT, 426, 251);
        }
        else
        {
            print_status = 0;
            LV_IMG_DECLARE(print);
            lv_imgbtn_set_src(imgbtn_print, LV_IMGBTN_STATE_RELEASED, NULL, &print, NULL);
            lv_obj_set_size(imgbtn_print, print.header.w, print.header.h);
            lv_obj_align(imgbtn_print, LV_ALIGN_TOP_LEFT, 425, 207);

            LV_FONT_DECLARE(heiFont16_1);
            lv_obj_set_style_text_font(label_print, &heiFont16_1, LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(label_print, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
            lv_label_set_text(label_print, "打印");
            lv_obj_align(label_print, LV_ALIGN_TOP_LEFT, 428, 251);
        }
    }
}

static void button_right_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("right\n");
    }
}

static void button_up_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("up\n");
    }
}


static void button_file_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("file\n");
    }
}

static void button_save_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("save\n");
    }
}

static void button_file_management_return_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        printf("get_back\n");
        lv_obj_del(file_management_win);
        lv_obj_del(file_management_head_bg);
        lv_obj_del(label_file_management_total_output);
        lv_obj_del(label_file_management);
        lv_obj_del(label_file_management_date);
        lv_obj_del(label_file_management_time);
        lv_obj_del(img_file_management_battery_level);
        lv_obj_del(label_file_management_label);
        lv_obj_del(file_management_middle_bg);
        lv_obj_del(img_file_management_text);
        lv_obj_del(label_file_management_text);
        lv_obj_del(label_file_management_text_label);
        lv_obj_del(btn_file_management_text);
        //lv_obj_del(img_file_management_text_time);
        //lv_obj_del(label_file_management_text_time);
        //lv_obj_del(label_file_management_text_time_label);
        lv_obj_del(img_file_management_typeface);
        lv_obj_del(label_file_management_typeface);
        lv_obj_del(label_file_management_typeface_label);
        lv_obj_del(img_file_management_word_space);
        lv_obj_del(label_file_management_word_space);
        lv_obj_del(label_file_management_word_space_label);
        lv_obj_del(img_file_management_word_size);
        lv_obj_del(label_file_management_word_size);
        lv_obj_del(label_file_management_word_size_label);
        lv_obj_del(img_file_management_spin);
        lv_obj_del(label_file_management_spin);
        lv_obj_del(label_file_management_spin_label);
        lv_obj_del(img_file_management_width);
        lv_obj_del(label_file_management_width);
        lv_obj_del(label_file_management_width_label);
        lv_obj_del(img_file_management_bold);
        lv_obj_del(label_file_management_bold);
        lv_obj_del(sw_file_management_bold);
        //lv_obj_del(img_file_management_line_increment);
        //lv_obj_del(label_file_management_line_increment);
        //lv_obj_del(label_file_management_line_increment_label);
        lv_obj_del(img_management_line_italic);
        lv_obj_del(label_management_line_italic);
        lv_obj_del(sw_management_line_italic);
        lv_obj_del(file_management_bottom_bg);
        lv_obj_del(label_file_management_return);
        lv_obj_del(imgbtn_file_management_get_back);
        lv_obj_del(imgbtn_file_management_confirm);
        lv_obj_del(label_file_management_confirm);
    }
}


static void button_file_management_confirm_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        printf("button_file_management_confirm_event_cb\n");
        //lv_label_set_text(main_test_label, lv_label_get_text(label_file_management_label));
        //lv_label_t* label = (lv_label_t *)main_test_label;
        lv_canvas_draw_text(main_canvas, 0, 0, CANVAS_WIDTH, &main_label_dsc, lv_label_get_text(label_file_management_label));
        lv_obj_del(file_management_win);
        lv_obj_del(file_management_head_bg);
        lv_obj_del(label_file_management_total_output);
        lv_obj_del(label_file_management);
        lv_obj_del(label_file_management_date);
        lv_obj_del(label_file_management_time);
        lv_obj_del(img_file_management_battery_level);
        lv_obj_del(label_file_management_label);
        lv_obj_del(file_management_middle_bg);
        lv_obj_del(img_file_management_text);
        lv_obj_del(label_file_management_text);
        lv_obj_del(label_file_management_text_label);
        //lv_obj_del(img_file_management_text_time);
        //lv_obj_del(label_file_management_text_time);
        //lv_obj_del(label_file_management_text_time_label);

        lv_obj_del(img_file_management_typeface);
        lv_obj_del(label_file_management_typeface);
        lv_obj_del(label_file_management_typeface_label);
        lv_obj_del(img_file_management_word_space);
        lv_obj_del(label_file_management_word_space);
        lv_obj_del(label_file_management_word_space_label);
        lv_obj_del(img_file_management_word_size);
        lv_obj_del(label_file_management_word_size);
        lv_obj_del(label_file_management_word_size_label);
        lv_obj_del(img_file_management_spin);
        lv_obj_del(label_file_management_spin);
        lv_obj_del(label_file_management_spin_label);
        lv_obj_del(img_file_management_width);
        lv_obj_del(label_file_management_width);
        lv_obj_del(label_file_management_width_label);
        lv_obj_del(img_file_management_bold);
        lv_obj_del(label_file_management_bold);
        lv_obj_del(sw_file_management_bold);
        //lv_obj_del(img_file_management_line_increment);
        //lv_obj_del(label_file_management_line_increment);
        //lv_obj_del(label_file_management_line_increment_label);
        lv_obj_del(img_management_line_italic);
        lv_obj_del(label_management_line_italic);
        lv_obj_del(sw_management_line_italic);
        lv_obj_del(file_management_bottom_bg);
        lv_obj_del(label_file_management_return);
        lv_obj_del(imgbtn_file_management_get_back);
        lv_obj_del(imgbtn_file_management_confirm);
        lv_obj_del(label_file_management_confirm);
        lv_obj_del(btn_file_management_text);
    }
}


static void button_confirm_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        printf("confirm\n");
    }
}


static void textarea_text_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    //lv_obj_t* target = lv_event_get_target(e);

    printf("code=%d\n",code);

    if (code == LV_EVENT_FOCUSED)
    {
        printf("lv_keyboard_set_textarea\n");
    }
    else if (code == LV_EVENT_VALUE_CHANGED)
    {
        printf("LV_EVENT_VALUE_CHANGED\n");
        //const char* txt = lv_textarea_get_text(target);
    }
    else if (code == LV_EVENT_CANCEL)
    {
        printf("LV_EVENT_CANCEL\n");
        //lv_obj_set_style_text_font(target,&myFont,LV_PART_MAIN);
    }
}

static void btn_file_management_text_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        printf("btn_file_management_text_event_cb\n");

        textarea_bg_keyboard = lv_obj_create(lv_scr_act());
        /* 设置大小 */
        lv_obj_set_size(textarea_bg_keyboard, 480, 272);
        lv_obj_align(textarea_bg_keyboard, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_style_border_width(textarea_bg_keyboard, 0, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_border_color(textarea_bg_keyboard, lv_color_hex(0x9bcd9b), LV_STATE_DEFAULT); /* 设置边框颜色 */
        //lv_obj_set_style_border_opa(file_management_win, 100, LV_STATE_DEFAULT); /* 设置边框透明度 */
        lv_obj_set_style_radius(textarea_bg_keyboard, 0, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_color(textarea_bg_keyboard, lv_color_hex(0x1fadd3), LV_STATE_DEFAULT);

        //输入框+键盘
        LV_FONT_DECLARE(heiFont7);
        pinyin_ime = lv_ime_pinyin_create(lv_scr_act());
        lv_obj_set_style_text_font(pinyin_ime, &heiFont7, 0);
        //lv_obj_set_style_text_font(pinyin_ime, &lv_font_simsun_16_cjk, 0);
        //lv_ime_pinyin_set_dict(pinyin_ime, your_dict); // Use a custom dictionary. If it is not set, the built-in dictionary will be used.

        textarea_text_keyboard = lv_textarea_create(lv_scr_act());
        //lv_textarea_set_one_line(textarea_text_keyboard, true);
        lv_obj_set_size(textarea_text_keyboard, 480, 95);
        lv_obj_set_style_text_font(textarea_text_keyboard, &heiFont7, 0);
        lv_obj_align(textarea_text_keyboard, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_textarea_set_max_length(textarea_text_keyboard, 100);
        lv_textarea_set_placeholder_text(textarea_text_keyboard, "Smart printer");
        //lv_obj_add_event_cb(textarea_text_keyboard, textarea_text_event_cb, LV_EVENT_ALL, NULL);
        lv_obj_set_style_border_width(textarea_text_keyboard, 1, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_border_color(textarea_text_keyboard, lv_color_hex(0xAAAAAA), LV_STATE_DEFAULT); /* 设置边框颜色 */
        lv_obj_set_style_border_opa(textarea_text_keyboard, 100, LV_STATE_DEFAULT); /* 设置边框透明度 */
        lv_obj_set_style_radius(textarea_text_keyboard, 0, LV_STATE_DEFAULT); /* 设置圆角 */
     


        /*Create a keyboard and add it to ime_pinyin*/
        text_edit_keyboard = lv_keyboard_create(lv_scr_act());
        lv_ime_pinyin_set_keyboard(pinyin_ime, text_edit_keyboard);
        lv_keyboard_set_textarea(text_edit_keyboard, textarea_text_keyboard);
        //lv_obj_set_style_bg_color(text_edit_keyboard, lv_color_hex(0xff4040), LV_PART_MAIN);
        lv_obj_set_style_bg_color(text_edit_keyboard, lv_color_hex(0x1fadd3), LV_PART_MAIN);
        lv_obj_add_event_cb(textarea_text_keyboard, textarea_text_event_cb, LV_EVENT_ALL, text_edit_keyboard);
        lv_obj_align(text_edit_keyboard, LV_ALIGN_TOP_LEFT, 0, 115);
        lv_obj_set_size(text_edit_keyboard, 480, 133);

        /*Get the cand_panel, and adjust its size and position*/
        keyboard_cand_panel = lv_ime_pinyin_get_cand_panel(pinyin_ime);
        lv_obj_set_size(keyboard_cand_panel, LV_PCT(100), LV_PCT(10));
        lv_obj_align_to(keyboard_cand_panel, text_edit_keyboard, LV_ALIGN_OUT_TOP_MID, 0, 5);
        lv_obj_set_style_bg_color(keyboard_cand_panel, lv_color_hex(0x8B1A1A), LV_PART_MAIN);
    

        keyboard_bg = lv_obj_create(lv_scr_act());
        lv_obj_set_size(keyboard_bg, 480, 24);
        lv_obj_align(keyboard_bg, LV_ALIGN_TOP_LEFT, 0, 248);
        lv_obj_set_style_border_width(keyboard_bg, 0, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_radius(keyboard_bg, 0, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_color(keyboard_bg, lv_color_hex(0x28536a), LV_STATE_DEFAULT);
        lv_obj_remove_style(keyboard_bg, 0, LV_PART_SCROLLBAR);


        LV_FONT_DECLARE(heiFont16_1);
        label_keyboard_return = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_keyboard_return, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_keyboard_return, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_keyboard_return, "返回");
        lv_obj_align(label_keyboard_return, LV_ALIGN_TOP_LEFT, 140, 252);

        LV_IMG_DECLARE(get_back);
        btn_keyboard_return = lv_imgbtn_create(lv_scr_act());
        lv_imgbtn_set_src(btn_keyboard_return, LV_IMGBTN_STATE_RELEASED, NULL, &get_back, NULL);
        lv_obj_set_size(btn_keyboard_return, get_back.header.w, get_back.header.h);
        lv_obj_align(btn_keyboard_return, LV_ALIGN_TOP_LEFT, 180, 250);
        lv_obj_add_event_cb(btn_keyboard_return, button_keyboard_return_event_cb, LV_EVENT_CLICKED, NULL);

        LV_IMG_DECLARE(confirm);
        btn_keyboard_confirm = lv_imgbtn_create(lv_scr_act());
        lv_imgbtn_set_src(btn_keyboard_confirm, LV_IMGBTN_STATE_RELEASED, NULL, &confirm, NULL);
        lv_obj_set_size(btn_keyboard_confirm, get_back.header.w, get_back.header.h);
        lv_obj_align(btn_keyboard_confirm, LV_ALIGN_TOP_LEFT, 250, 250);
        lv_obj_add_event_cb(btn_keyboard_confirm, button_keyboard_confirm_event_cb, LV_EVENT_CLICKED, NULL);

        LV_FONT_DECLARE(heiFont16_1);
        label_keyboard_confirm = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_keyboard_confirm, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_keyboard_confirm, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_keyboard_confirm, "确定");
        lv_obj_align(label_keyboard_confirm, LV_ALIGN_TOP_LEFT, 277, 252);

        lv_timer_pause(timer_100ms);
    }
}

static void button_addition_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        printf("addition\n");
        file_management_win = lv_obj_create(lv_scr_act());
        /* 设置大小 */
        lv_obj_set_size(file_management_win, 480, 272);
        lv_obj_align(file_management_win, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_style_border_width(file_management_win, 0, LV_STATE_DEFAULT); /* 设置边框宽度 */
        //lv_obj_set_style_border_color(file_management_win, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT); /* 设置边框颜色 */
        //lv_obj_set_style_border_opa(file_management_win, 100, LV_STATE_DEFAULT); /* 设置边框透明度 */
        lv_obj_set_style_radius(file_management_win, 0, LV_STATE_DEFAULT); /* 设置圆角 */

        file_management_head_bg = lv_obj_create(lv_scr_act());
        /* 设置大小 */
        lv_obj_set_size(file_management_head_bg, 480, 24);
        lv_obj_align(file_management_head_bg, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_style_border_width(file_management_head_bg, 0, LV_STATE_DEFAULT); /* 设置边框宽度 */
        //lv_obj_set_style_border_color(file_management_win_1, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT); /* 设置边框颜色 */
        //lv_obj_set_style_border_opa(file_management_win_1, 100, LV_STATE_DEFAULT); /* 设置边框透明度 */
        lv_obj_set_style_radius(file_management_head_bg, 0, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_color(file_management_head_bg, lv_color_hex(0x28536a), LV_STATE_DEFAULT);
        lv_obj_remove_style(file_management_head_bg, 0, LV_PART_SCROLLBAR);


        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_total_output = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_total_output, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_total_output, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_total_output, "总产量:99999");
        lv_obj_align(label_file_management_total_output, LV_ALIGN_TOP_LEFT, 0, 4);


        LV_FONT_DECLARE(heiFont16_1);
        label_file_management = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management, "文件管理");
        lv_obj_align(label_file_management, LV_ALIGN_TOP_LEFT, 200, 4);


        LV_FONT_DECLARE(heiFont12);
        label_file_management_date = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_date, &heiFont12, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_date, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_date, "2024-09-09");
        lv_obj_align(label_file_management_date, LV_ALIGN_TOP_LEFT, 375, 2);


        LV_FONT_DECLARE(heiFont12);
        label_file_management_time = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_time, &heiFont12, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_time, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_time, "09:52:32");
        lv_obj_align(label_file_management_time, LV_ALIGN_TOP_LEFT, 380, 12);


        LV_IMG_DECLARE(battery_level);
        img_file_management_battery_level = lv_img_create(lv_scr_act());
        lv_img_set_src(img_file_management_battery_level, &battery_level);
        lv_obj_align(img_file_management_battery_level, LV_ALIGN_TOP_LEFT, 450, 2);


        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_label = lv_label_create(lv_scr_act());
        lv_obj_set_width(label_file_management_label, 480);
        lv_obj_set_height(label_file_management_label, 108);
        lv_obj_set_style_text_font(label_file_management_label, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_label, lv_color_hex(0x1C1C1C), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_label, "欢迎使用");
        lv_obj_align(label_file_management_label, LV_ALIGN_TOP_LEFT, 0, 24);

        
        file_management_middle_bg = lv_obj_create(lv_scr_act());
        lv_obj_set_size(file_management_middle_bg, 480, 140);
        lv_obj_align(file_management_middle_bg, LV_ALIGN_TOP_LEFT, 0, 132);
        lv_obj_set_style_border_width(file_management_middle_bg, 0, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_radius(file_management_middle_bg, 0, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_color(file_management_middle_bg, lv_color_hex(0x1fadd3), LV_STATE_DEFAULT);

        LV_IMG_DECLARE(text);
        img_file_management_text = lv_img_create(lv_scr_act());
        lv_img_set_src(img_file_management_text, &text);
        lv_obj_align(img_file_management_text, LV_ALIGN_TOP_LEFT, 10, 134);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_text = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_text, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_text, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_text, "文本");
        lv_obj_align(label_file_management_text, LV_ALIGN_TOP_LEFT, 45, 139);

        LV_FONT_DECLARE(heiFont16_1);
        btn_file_management_text = lv_btn_create(lv_scr_act());
        lv_obj_set_size(btn_file_management_text, 125, 24);
        lv_obj_align(btn_file_management_text, LV_ALIGN_TOP_LEFT, 95, 136);
        lv_obj_set_style_border_width(btn_file_management_text, 0, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_radius(btn_file_management_text, 0, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_color(btn_file_management_text, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        label_file_management_text_label = lv_label_create(btn_file_management_text);
        lv_obj_set_style_text_font(label_file_management_text_label, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_text_label, lv_color_hex(0x1C1C1C), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_text_label, "");
        lv_obj_align(label_file_management_text_label, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_border_width(label_file_management_text_label, 0, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_border_color(label_file_management_text_label, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT); /* 设置边框颜色 */
        lv_obj_set_style_border_opa(label_file_management_text_label, 100, LV_STATE_DEFAULT); /* 设置边框透明度 */
        lv_obj_set_style_radius(label_file_management_text_label, 0, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_opa(label_file_management_text_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(label_file_management_text_label, lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_obj_add_event_cb(btn_file_management_text, btn_file_management_text_event_cb, LV_EVENT_CLICKED, NULL);

/*
        LV_IMG_DECLARE(time);
        img_file_management_text_time = lv_img_create(lv_scr_act());
        lv_img_set_src(img_file_management_text_time, &time);
        lv_obj_align(img_file_management_text_time, LV_ALIGN_TOP_LEFT, 250, 106);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_text_time = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_text_time, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_text_time, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_text_time, "时间");
        lv_obj_align(label_file_management_text_time, LV_ALIGN_TOP_LEFT, 285, 111);


        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_text_time_label = lv_label_create(lv_scr_act());
        lv_obj_set_width(label_file_management_text_time_label, 125);
        lv_obj_set_height(label_file_management_text_time_label, 24);
        lv_obj_set_style_text_font(label_file_management_text_time_label, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_text_time_label, lv_color_hex(0x1C1C1C), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_text_time_label, "2024/09/20");
        lv_obj_align(label_file_management_text_time_label, LV_ALIGN_TOP_LEFT, 338, 108);
        lv_obj_set_style_border_width(label_file_management_text_time_label, 1, LV_STATE_DEFAULT); //设置边框宽度 
        lv_obj_set_style_border_color(label_file_management_text_time_label, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT); //设置边框颜色 
        lv_obj_set_style_border_opa(label_file_management_text_time_label, 100, LV_STATE_DEFAULT); //设置边框透明度
        lv_obj_set_style_radius(label_file_management_text_time_label, 2, LV_STATE_DEFAULT); //设置圆角 
        lv_obj_set_style_bg_opa(label_file_management_text_time_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(label_file_management_text_time_label, lv_color_hex(0xffffff), LV_PART_MAIN);
*/

        LV_IMG_DECLARE(typeface);
        img_file_management_typeface = lv_img_create(lv_scr_act());
        lv_img_set_src(img_file_management_typeface, &typeface);
        lv_obj_align(img_file_management_typeface, LV_ALIGN_TOP_LEFT, 10, 162);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_typeface = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_typeface, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_typeface, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_typeface, "字体");
        lv_obj_align(label_file_management_typeface, LV_ALIGN_TOP_LEFT, 45, 167);


        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_typeface_label = lv_label_create(lv_scr_act());
        lv_obj_set_width(label_file_management_typeface_label, 125);
        lv_obj_set_height(label_file_management_typeface_label, 24);
        lv_obj_set_style_text_font(label_file_management_typeface_label, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_typeface_label, lv_color_hex(0x1C1C1C), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_typeface_label, "ABCDEF");
        lv_obj_align(label_file_management_typeface_label, LV_ALIGN_TOP_LEFT, 95, 164);
        lv_obj_set_style_border_width(label_file_management_typeface_label, 1, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_border_color(label_file_management_typeface_label, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT); /* 设置边框颜色 */
        lv_obj_set_style_border_opa(label_file_management_typeface_label, 100, LV_STATE_DEFAULT); /* 设置边框透明度 */
        lv_obj_set_style_radius(label_file_management_typeface_label, 2, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_opa(label_file_management_typeface_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(label_file_management_typeface_label, lv_color_hex(0xffffff), LV_PART_MAIN);


        LV_IMG_DECLARE(word_space);
        img_file_management_word_space = lv_img_create(lv_scr_act());
        lv_img_set_src(img_file_management_word_space, &word_space);
        lv_obj_align(img_file_management_word_space, LV_ALIGN_TOP_LEFT, 250, 134);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_word_space = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_word_space, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_word_space, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_word_space, "间隔");
        lv_obj_align(label_file_management_word_space, LV_ALIGN_TOP_LEFT, 285, 139);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_word_space_label = lv_label_create(lv_scr_act());
        lv_obj_set_width(label_file_management_word_space_label, 125);
        lv_obj_set_height(label_file_management_word_space_label, 24);
        lv_obj_set_style_text_font(label_file_management_word_space_label, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_word_space_label, lv_color_hex(0x1C1C1C), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_word_space_label, "ABCDEF");
        lv_obj_align(label_file_management_word_space_label, LV_ALIGN_TOP_LEFT, 338, 136);
        lv_obj_set_style_border_width(label_file_management_word_space_label, 1, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_border_color(label_file_management_word_space_label, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT); /* 设置边框颜色 */
        lv_obj_set_style_border_opa(label_file_management_word_space_label, 100, LV_STATE_DEFAULT); /* 设置边框透明度 */
        lv_obj_set_style_radius(label_file_management_word_space_label, 2, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_opa(label_file_management_word_space_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(label_file_management_word_space_label, lv_color_hex(0xffffff), LV_PART_MAIN);

        LV_IMG_DECLARE(word_size);
        img_file_management_word_size = lv_img_create(lv_scr_act());
        lv_img_set_src(img_file_management_word_size, &word_size);
        lv_obj_align(img_file_management_word_size, LV_ALIGN_TOP_LEFT, 10, 190);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_word_size = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_word_size, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_word_size, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_word_size, "字号");
        lv_obj_align(label_file_management_word_size, LV_ALIGN_TOP_LEFT, 45, 195);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_word_size_label = lv_label_create(lv_scr_act());
        lv_obj_set_width(label_file_management_word_size_label, 125);
        lv_obj_set_height(label_file_management_word_size_label, 24);
        lv_obj_set_style_text_font(label_file_management_word_size_label, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_word_size_label, lv_color_hex(0x1C1C1C), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_word_size_label, "ABCDEF");
        lv_obj_align(label_file_management_word_size_label, LV_ALIGN_TOP_LEFT, 95, 192);
        lv_obj_set_style_border_width(label_file_management_word_size_label, 1, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_border_color(label_file_management_word_size_label, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT); /* 设置边框颜色 */
        lv_obj_set_style_border_opa(label_file_management_word_size_label, 100, LV_STATE_DEFAULT); /* 设置边框透明度 */
        lv_obj_set_style_radius(label_file_management_word_size_label, 2, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_opa(label_file_management_word_size_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(label_file_management_word_size_label, lv_color_hex(0xffffff), LV_PART_MAIN);


        LV_IMG_DECLARE(spin);
        img_file_management_spin = lv_img_create(lv_scr_act());
        lv_img_set_src(img_file_management_spin, &spin);
        lv_obj_align(img_file_management_spin, LV_ALIGN_TOP_LEFT, 250, 190);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_spin = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_spin, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_spin, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_spin, "旋转");
        lv_obj_align(label_file_management_spin, LV_ALIGN_TOP_LEFT, 285, 195);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_spin_label = lv_label_create(lv_scr_act());
        lv_obj_set_width(label_file_management_spin_label, 125);
        lv_obj_set_height(label_file_management_spin_label, 24);
        lv_obj_set_style_text_font(label_file_management_spin_label, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_spin_label, lv_color_hex(0x1C1C1C), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_spin_label, "ABCDEF");
        lv_obj_align(label_file_management_spin_label, LV_ALIGN_TOP_LEFT, 338, 192);
        lv_obj_set_style_border_width(label_file_management_spin_label, 1, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_border_color(label_file_management_spin_label, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT); /* 设置边框颜色 */
        lv_obj_set_style_border_opa(label_file_management_spin_label, 100, LV_STATE_DEFAULT); /* 设置边框透明度 */
        lv_obj_set_style_radius(label_file_management_spin_label, 2, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_opa(label_file_management_spin_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(label_file_management_spin_label, lv_color_hex(0xffffff), LV_PART_MAIN);

        LV_IMG_DECLARE(width);
        img_file_management_width = lv_img_create(lv_scr_act());
        lv_img_set_src(img_file_management_width, &width);
        lv_obj_align(img_file_management_width, LV_ALIGN_TOP_LEFT, 250, 162);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_width = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_width, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_width, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_width, "宽度");
        lv_obj_align(label_file_management_width, LV_ALIGN_TOP_LEFT, 285, 167);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_width_label = lv_label_create(lv_scr_act());
        lv_obj_set_width(label_file_management_width_label, 125);
        lv_obj_set_height(label_file_management_width_label, 24);
        lv_obj_set_style_text_font(label_file_management_width_label, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_width_label, lv_color_hex(0x1C1C1C), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_width_label, "ABCDEF");
        lv_obj_align(label_file_management_width_label, LV_ALIGN_TOP_LEFT, 338, 164);
        lv_obj_set_style_border_width(label_file_management_width_label, 1, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_border_color(label_file_management_width_label, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT); /* 设置边框颜色 */
        lv_obj_set_style_border_opa(label_file_management_width_label, 100, LV_STATE_DEFAULT); /* 设置边框透明度 */
        lv_obj_set_style_radius(label_file_management_width_label, 2, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_opa(label_file_management_width_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(label_file_management_width_label, lv_color_hex(0xffffff), LV_PART_MAIN);


        LV_IMG_DECLARE(bold);
        img_file_management_bold = lv_img_create(lv_scr_act());
        lv_img_set_src(img_file_management_bold, &bold);
        lv_obj_align(img_file_management_bold, LV_ALIGN_TOP_LEFT, 250, 218);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_bold = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_bold, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_bold, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_bold, "粗体");
        lv_obj_align(label_file_management_bold, LV_ALIGN_TOP_LEFT, 285, 223);

        sw_file_management_bold = lv_switch_create(lv_scr_act());
        lv_obj_set_pos(sw_file_management_bold, 343, 220);
        lv_obj_set_width(sw_file_management_bold, 48);
        lv_obj_set_height(sw_file_management_bold, 23);
        lv_obj_set_style_bg_color(sw_file_management_bold, lv_color_hex(0x00ff00), LV_STATE_CHECKED | LV_PART_INDICATOR);


/*
        LV_IMG_DECLARE(line_increment);
        img_file_management_line_increment = lv_img_create(lv_scr_act());
        lv_img_set_src(img_file_management_line_increment, &line_increment);
        lv_obj_align(img_file_management_line_increment, LV_ALIGN_TOP_LEFT, 10, 218);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_line_increment = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_line_increment, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_line_increment, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_line_increment, "增行");
        lv_obj_align(label_file_management_line_increment, LV_ALIGN_TOP_LEFT, 45, 223);


        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_line_increment_label = lv_label_create(lv_scr_act());
        lv_obj_set_width(label_file_management_line_increment_label, 125);
        lv_obj_set_height(label_file_management_line_increment_label, 24);
        lv_obj_set_style_text_font(label_file_management_line_increment_label, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_line_increment_label, lv_color_hex(0x1C1C1C), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_line_increment_label, "ABCDEF");
        lv_obj_align(label_file_management_line_increment_label, LV_ALIGN_TOP_LEFT, 95, 220);
        lv_obj_set_style_border_width(label_file_management_line_increment_label, 1, LV_STATE_DEFAULT); // 设置边框宽度
        lv_obj_set_style_border_color(label_file_management_line_increment_label, lv_color_hex(0x8a8a8a), LV_STATE_DEFAULT); // 设置边框颜色 
        lv_obj_set_style_border_opa(label_file_management_line_increment_label, 100, LV_STATE_DEFAULT); // 设置边框透明度 
        lv_obj_set_style_radius(label_file_management_line_increment_label, 2, LV_STATE_DEFAULT); // 设置圆角 
        lv_obj_set_style_bg_opa(label_file_management_line_increment_label, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(label_file_management_line_increment_label, lv_color_hex(0xffffff), LV_PART_MAIN);

*/
        LV_IMG_DECLARE(italic);
        img_management_line_italic = lv_img_create(lv_scr_act());
        lv_img_set_src(img_management_line_italic, &italic);
        lv_obj_align(img_management_line_italic, LV_ALIGN_TOP_LEFT, 10, 218);

        LV_FONT_DECLARE(heiFont16_1);
        label_management_line_italic = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_management_line_italic, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_management_line_italic, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_management_line_italic, "斜体");
        lv_obj_align(label_management_line_italic, LV_ALIGN_TOP_LEFT, 45, 223);

        sw_management_line_italic = lv_switch_create(lv_scr_act());
        lv_obj_set_pos(sw_management_line_italic, 95, 220);
        lv_obj_set_width(sw_management_line_italic, 48);
        lv_obj_set_height(sw_management_line_italic, 23);
        //lv_obj_set_style_bg_color(bold_sw, lv_color_hex(0x00ff00), LV_PART_MAIN);
        lv_obj_set_style_bg_color(sw_management_line_italic, lv_color_hex(0x00ff00), LV_STATE_CHECKED | LV_PART_INDICATOR);

        file_management_bottom_bg = lv_obj_create(lv_scr_act());
        lv_obj_set_size(file_management_bottom_bg, 480, 24);
        lv_obj_align(file_management_bottom_bg, LV_ALIGN_TOP_LEFT, 0, 248);
        lv_obj_set_style_border_width(file_management_bottom_bg, 0, LV_STATE_DEFAULT); /* 设置边框宽度 */
        lv_obj_set_style_radius(file_management_bottom_bg, 0, LV_STATE_DEFAULT); /* 设置圆角 */
        lv_obj_set_style_bg_color(file_management_bottom_bg, lv_color_hex(0x28536a), LV_STATE_DEFAULT);
        lv_obj_remove_style(file_management_bottom_bg, 0, LV_PART_SCROLLBAR);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_return = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_return, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_return, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_return, "返回");
        lv_obj_align(label_file_management_return, LV_ALIGN_TOP_LEFT, 140, 252);

        LV_IMG_DECLARE(get_back);
        imgbtn_file_management_get_back = lv_imgbtn_create(lv_scr_act());
        lv_imgbtn_set_src(imgbtn_file_management_get_back, LV_IMGBTN_STATE_RELEASED, NULL, &get_back, NULL);
        lv_obj_set_size(imgbtn_file_management_get_back, get_back.header.w, get_back.header.h);
        lv_obj_align(imgbtn_file_management_get_back, LV_ALIGN_TOP_LEFT, 180, 250);
        lv_obj_add_event_cb(imgbtn_file_management_get_back, button_file_management_return_event_cb, LV_EVENT_CLICKED, NULL);

        LV_IMG_DECLARE(confirm);
        imgbtn_file_management_confirm = lv_imgbtn_create(lv_scr_act());
        lv_imgbtn_set_src(imgbtn_file_management_confirm, LV_IMGBTN_STATE_RELEASED, NULL, &confirm, NULL);
        lv_obj_set_size(imgbtn_file_management_confirm, get_back.header.w, get_back.header.h);
        lv_obj_align(imgbtn_file_management_confirm, LV_ALIGN_TOP_LEFT, 250, 250);
        lv_obj_add_event_cb(imgbtn_file_management_confirm, button_file_management_confirm_event_cb, LV_EVENT_CLICKED, NULL);

        LV_FONT_DECLARE(heiFont16_1);
        label_file_management_confirm = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(label_file_management_confirm, &heiFont16_1, LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label_file_management_confirm, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
        lv_label_set_text(label_file_management_confirm, "确定");
        lv_obj_align(label_file_management_confirm, LV_ALIGN_TOP_LEFT, 277, 252); 

    }
}

static void button_delete_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("delete\n");
    }
}

static void button_modification_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("modification\n");
    }
}

static void button_set_event_cb(lv_event_t* e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    if ((code == LV_EVENT_CLICKED)&&(print_status == 0))
    {
        printf("set\n");
    }
}


void lv_demo_widgets(void)
{
    //int retvalue;
	//char *filename;
	char databuf[32];

    if(LV_HOR_RES <= 320) 
    {
        disp_size = DISP_SMALL;
    }
    else if(LV_HOR_RES < 720) 
    {
        disp_size = DISP_MEDIUM;
    }
    else 
    {
        disp_size = DISP_LARGE;
    }
    
    font_large = LV_FONT_DEFAULT;
    font_normal = LV_FONT_DEFAULT;

#if 0
    lv_coord_t tab_h;
    if(disp_size == DISP_LARGE) {
        tab_h = 70;
#if LV_FONT_MONTSERRAT_24
        font_large     = &lv_font_montserrat_24;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_24 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_16
        font_normal    = &lv_font_montserrat_16;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_16 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }
    else if(disp_size == DISP_MEDIUM) {
        tab_h = 45;
#if LV_FONT_MONTSERRAT_20
        font_large     = &lv_font_montserrat_20;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_20 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_14
        font_normal    = &lv_font_montserrat_14;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_14 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }
    else {   /* disp_size == DISP_SMALL */
        tab_h = 45;
#if LV_FONT_MONTSERRAT_18
        font_large     = &lv_font_montserrat_18;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_18 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_12
        font_normal    = &lv_font_montserrat_12;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_12 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }

#if LV_USE_THEME_DEFAULT
    lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK,
                          font_normal);
#endif
#endif

#if 0

    lv_fs_file_t lv_file;
  	lv_fs_res_t  lv_res;
    char buf[20] = {0};
    int num;

  	lv_res = lv_fs_open( &lv_file, "A:/root/1.txt", LV_FS_MODE_RD );
  	if ( lv_res != LV_FS_RES_OK ) 
    {
    	printf( "LVGL FS open error. (%d)\n", lv_res );
  	} 
    else 
    {
        printf( "LVGL FS open Ok\n" );
    }

    lv_res = lv_fs_read(&lv_file,buf,5,&num);
    if(lv_res != LV_FS_RES_OK )
    {
        printf( "LVGL f_read error. (%d)\n", lv_res);
    }
    else
    {
        printf( "%s\n", buf);
    }
    
  	
	lv_fs_close(&lv_file);

#if 0
    lv_draw_rect_dsc_t rect_dsc;
    /* 初始化绘画矩形 */
    lv_draw_rect_dsc_init(&rect_dsc);
    /* 设置圆角 */
    rect_dsc.radius = 10;
    /* 设置透明度 */
    rect_dsc.bg_opa = LV_OPA_COVER;
    /* 设置颜色渐变方向 */
    rect_dsc.bg_grad.dir = LV_GRAD_DIR_HOR;
    /* 设置开始颜色 */
    rect_dsc.bg_grad.stops[0].color = lv_palette_main(LV_PALETTE_RED);
    /* 设置结束颜色 */
    rect_dsc.bg_grad.stops[1].color = lv_palette_main(LV_PALETTE_BLUE);
    /* 设置边缘宽度 */
    rect_dsc.border_width = 2;
    /* 设置边缘透明度 */
    rect_dsc.border_opa = LV_OPA_90;
    /* 设置边缘颜色 */
    rect_dsc.border_color = lv_color_white();
    /* 在画布上绘制矩形 */
    lv_canvas_draw_rect(canvas,0,0,50,50,&rect_dsc);
#endif

#if 0

    /* 定义绘制标签 */
    lv_draw_label_dsc_t label_dsc;
    /* 初始化绘制标签 */
    lv_draw_label_dsc_init(&label_dsc);
    /* 设置标签颜色 */
    label_dsc.color = lv_color_black();
    /* 在画布上绘制标签 */
    lv_canvas_draw_text(canvas,0,0,1024,&label_dsc,"Some text on text canvas");
#endif

/* 字体测试*/
    lv_font_t *my_font;
    my_font = lv_font_load("A:/usr/font/songFont36.bin");
    if (my_font == NULL)
    {
        printf("font load failed\n");
    }
    else
    {
       printf("font load ok\n");
    }
    

    static lv_style_t font_style;
    lv_style_init(&font_style);
    lv_style_set_text_font(&font_style, my_font);


    lv_obj_t *label_zh = lv_label_create(lv_scr_act());
    lv_obj_align(label_zh, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(label_zh, &font_style, 0);
    lv_label_set_text(label_zh, "中国智造");

    lv_font_free(my_font);

#endif


    LV_IMG_DECLARE(head_background_image);
    Head_background_image = lv_img_create(lv_scr_act()); /* 创建图片部件 */
    lv_img_set_src(Head_background_image, &head_background_image); /* 设置图片源 */
    /* 设置图片位置 */
    lv_obj_align(Head_background_image, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_update_layout(Head_background_image); /* 更新图片参数 */

    LV_FONT_DECLARE(heiFont16_1);
    Total_output_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(Total_output_label, (const lv_font_t *)&heiFont16_1, 0);
extern int LoadConfigFile(const char* pconfName);
extern int GetConfigIntDefault(const char* p_itemname, const int def);
    LoadConfigFile(config_file);
    g_total_output = GetConfigIntDefault("g_total_output",0);
    sprintf(databuf,"总产量:%u",g_total_output);
    lv_label_set_text(Total_output_label, databuf);
    lv_obj_align(Total_output_label, LV_ALIGN_TOP_LEFT, 0, 5);
    lv_obj_set_style_text_color(Total_output_label, lv_color_hex(0xffffff),LV_STATE_DEFAULT);



    Date_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(Date_label, (const lv_font_t *)&heiFont16_1, 0);
    //lv_label_set_text(Date_label, "2024/05/01");
    lv_label_set_text(Date_label, "");
    lv_obj_align(Date_label, LV_ALIGN_TOP_LEFT, 160, 5);
    lv_obj_set_style_text_color(Date_label, lv_color_hex(0xffffff),LV_STATE_DEFAULT);

    Time_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(Time_label, (const lv_font_t *)&heiFont16_1, 0);
    //lv_label_set_text(Time_label, "12:44:30");
    lv_label_set_text(Time_label, "");
    lv_obj_align(Time_label, LV_ALIGN_TOP_LEFT, 280, 5);
    lv_obj_set_style_text_color(Time_label, lv_color_hex(0xffffff),LV_STATE_DEFAULT);

    Battery_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(Battery_label, (const lv_font_t *)&heiFont16_1, 0);
    //lv_label_set_text(Battery_label, "80%");
    //lv_obj_align(Battery_label, LV_ALIGN_TOP_LEFT, 405, 0);
    lv_obj_set_style_text_color(Battery_label, lv_color_hex(0xffffff),LV_STATE_DEFAULT);

    //LV_IMG_DECLARE(lv_font_montserrat_14);
    Battery_image = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(Battery_image, (const lv_font_t *)&lv_font_montserrat_28, 0);
    //lv_label_set_text(Battery_image, LV_SYMBOL_BATTERY_3);
    lv_label_set_text(Battery_image, "");
    lv_obj_align(Battery_image, LV_ALIGN_TOP_LEFT, 440, -3);
    lv_obj_set_style_text_color(Battery_image, lv_color_hex(0x00ff00),LV_STATE_DEFAULT);


    lv_freetype_init(64, 1, 0);
    ///*Create a font*/
    //static lv_ft_info_t info;
    //info.name = "./lvgl/src/extra/libs/freetype/arial.ttf";
    //info.name = "./lvgl/src/extra/libs/freetype/simsun.ttc";
    main_info.name = "/media/SourceHanSerifCN-Regular-1.otf";
    main_info.weight = 80;
    //info.style = FT_FONT_STYLE_BOLD | FT_FONT_STYLE_ITALIC;
    //info.style = FT_FONT_STYLE_BOLD;
    main_info.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&main_info);


    //static lv_style_t style;
    //lv_style_init(&style);
    //lv_style_set_text_font(&style, main_info.font);


    //lv_obj_t* label123 = lv_label_create(lv_scr_act());
    //lv_obj_add_style(label123, &style, 0);
    //lv_label_set_text(label123, "欢迎使用");


    /* 第二步：创建一个画布*/
    win_content = lv_obj_create(lv_scr_act());
    lv_obj_set_size(win_content, 480, 160);
    lv_obj_align(win_content, LV_ALIGN_TOP_LEFT, 0, 24);
    lv_obj_set_style_bg_color(win_content, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(win_content, 1, LV_STATE_DEFAULT); /* 设置边框宽度 */
    lv_obj_set_style_border_color(win_content, lv_color_hex(0xcccccc), LV_STATE_DEFAULT); /* 设置边框颜色 */
    lv_obj_remove_style(win_content, 0, LV_PART_SCROLLBAR);
    lv_obj_set_scrollbar_mode(win_content, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_radius(win_content, 1, LV_STATE_DEFAULT); /* 设置圆角 */

    //LV_FONT_DECLARE(heiFont16_1);
    lv_draw_label_dsc_init(&main_label_dsc);
    main_label_dsc.color = lv_color_black();
    //main_label_dsc.font = &heiFont16_1;
    main_label_dsc.font = main_info.font;
    main_canvas = lv_canvas_create(win_content);
    lv_obj_align(main_canvas, LV_ALIGN_TOP_LEFT, 0, 0);

    /* 第三步：为画布设置缓冲区 */
    lv_canvas_set_buffer(main_canvas, canvasBuf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    lv_canvas_fill_bg(main_canvas, lv_color_hex(0xffffff), LV_OPA_COVER);

    lv_canvas_set_palette(main_canvas, 0, lv_color_hex3(0x33f));
    lv_canvas_set_palette(main_canvas, 1, lv_color_hex3(0xeef));
    lv_obj_align(main_canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_fill_bg(main_canvas, lv_color_hex(0xffffff), LV_OPA_COVER);
    lv_canvas_draw_text(main_canvas, 0, 0, CANVAS_WIDTH, &main_label_dsc, "12345678");


    LV_IMG_DECLARE(Slider_base_color);
    lv_obj_t* Slider_base_color_img = lv_img_create(lv_scr_act());
    lv_img_set_src(Slider_base_color_img, &Slider_base_color);
    lv_obj_align(Slider_base_color_img, LV_ALIGN_TOP_LEFT, 0, 183);

    //左滑块停止
    LV_IMG_DECLARE(left_stop);
    lv_obj_t* left_stop_img = lv_img_create(lv_scr_act());
    lv_img_set_src(left_stop_img, &left_stop);
    lv_obj_align(left_stop_img, LV_ALIGN_TOP_LEFT, 0, 183);

    slider_show_2();

    //右滑块停止
    LV_IMG_DECLARE(right_stop);
    lv_obj_t* right_stop_img = lv_img_create(lv_scr_act());
    lv_img_set_src(right_stop_img, &right_stop);
    lv_obj_align(right_stop_img, LV_ALIGN_TOP_LEFT, 459, 183);


    //底部背景
    LV_IMG_DECLARE(Bottom_background);
     lv_obj_t* Bottom_background_img = lv_img_create(lv_scr_act());
    lv_img_set_src(Bottom_background_img, &Bottom_background);
    lv_obj_align(Bottom_background_img, LV_ALIGN_TOP_LEFT, 0, 204);

    //文件
    LV_IMG_DECLARE(file);
    lv_obj_t* imgbtn_file = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_file, LV_IMGBTN_STATE_RELEASED, NULL, &file, NULL);
    lv_obj_set_size(imgbtn_file, file.header.w, file.header.h);
    lv_obj_align(imgbtn_file, LV_ALIGN_TOP_LEFT, 15, 207);
    lv_obj_add_event_cb(imgbtn_file, button_file_event_cb, LV_EVENT_CLICKED, NULL);

    LV_FONT_DECLARE(heiFont16_1);
    lv_obj_t* label_file = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_file, &heiFont16_1, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_file, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_label_set_text(label_file, "文件");
    lv_obj_align(label_file, LV_ALIGN_TOP_LEFT, 18, 251);

    //保存
    LV_IMG_DECLARE(save);
    lv_obj_t* imgbtn_save = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_save, LV_IMGBTN_STATE_RELEASED, NULL, &save, NULL);
    lv_obj_set_size(imgbtn_save, save.header.w, save.header.h);
    lv_obj_align(imgbtn_save, LV_ALIGN_TOP_LEFT, 70, 207);
    lv_obj_add_event_cb(imgbtn_save, button_save_event_cb, LV_EVENT_CLICKED, NULL);

    LV_FONT_DECLARE(heiFont16_1);
    lv_obj_t* label_save = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_save, &heiFont16_1, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_save, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_label_set_text(label_save, "保存");
    lv_obj_align(label_save, LV_ALIGN_TOP_LEFT, 73, 251);

    //添加
    LV_IMG_DECLARE(addition);
    lv_obj_t* imgbtn_addition = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_addition, LV_IMGBTN_STATE_RELEASED, NULL, &addition, NULL);
    lv_obj_set_size(imgbtn_addition, addition.header.w, addition.header.h);
    lv_obj_align(imgbtn_addition, LV_ALIGN_TOP_LEFT, 125, 207);
    lv_obj_add_event_cb(imgbtn_addition, button_addition_event_cb, LV_EVENT_CLICKED, NULL);

    LV_FONT_DECLARE(heiFont16_1);
    lv_obj_t* label_addition = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_addition, &heiFont16_1, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_addition, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_label_set_text(label_addition, "添加");
    lv_obj_align(label_addition, LV_ALIGN_TOP_LEFT, 128, 251);

    //删除
    LV_IMG_DECLARE(delete);
    lv_obj_t* imgbtn_delete = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_delete, LV_IMGBTN_STATE_RELEASED, NULL, &delete, NULL);
    lv_obj_set_size(imgbtn_delete, delete.header.w, delete.header.h);
    lv_obj_align(imgbtn_delete, LV_ALIGN_TOP_LEFT, 180, 207);
    lv_obj_add_event_cb(imgbtn_delete, button_delete_event_cb, LV_EVENT_CLICKED, NULL);

    LV_FONT_DECLARE(heiFont16_1);
    lv_obj_t* label_delete = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_delete, &heiFont16_1, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_delete, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_label_set_text(label_delete, "删除");
    lv_obj_align(label_delete, LV_ALIGN_TOP_LEFT, 183, 251);

    //修改
    LV_IMG_DECLARE(modification);
    lv_obj_t* imgbtn_modification = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_modification, LV_IMGBTN_STATE_RELEASED, NULL, &modification, NULL);
    lv_obj_set_size(imgbtn_modification, modification.header.w, modification.header.h);
    lv_obj_align(imgbtn_modification, LV_ALIGN_TOP_LEFT, 235, 207);
    lv_obj_add_event_cb(imgbtn_modification, button_modification_event_cb, LV_EVENT_CLICKED, NULL);


    LV_FONT_DECLARE(heiFont16_1);
    lv_obj_t* label_modification = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_modification, &heiFont16_1, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_modification, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_label_set_text(label_modification, "修改");
    lv_obj_align(label_modification, LV_ALIGN_TOP_LEFT, 238, 251);

    //设置
    LV_IMG_DECLARE(set);
    lv_obj_t* imgbtn_set = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_set, LV_IMGBTN_STATE_RELEASED, NULL, &set, NULL);
    lv_obj_set_size(imgbtn_set, set.header.w, set.header.h);
    lv_obj_align(imgbtn_set, LV_ALIGN_TOP_LEFT, 290, 207);
    lv_obj_add_event_cb(imgbtn_set, button_set_event_cb, LV_EVENT_CLICKED, NULL);


    LV_FONT_DECLARE(heiFont16_1);
    lv_obj_t* label_set = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_set, &heiFont16_1, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_set, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_label_set_text(label_set, "设置");
    lv_obj_align(label_set, LV_ALIGN_TOP_LEFT, 293, 251);


    LV_IMG_DECLARE(left);
    lv_obj_t* imgbtn_turn_left = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_turn_left, LV_IMGBTN_STATE_RELEASED, NULL, &left, NULL);
    printf("zhangchuan-%u %u\n\r",left.header.w,left.header.h);
    lv_obj_set_size(imgbtn_turn_left, left.header.w, left.header.h);
    lv_obj_align(imgbtn_turn_left, LV_ALIGN_TOP_LEFT, 347, 216);
    lv_obj_add_event_cb(imgbtn_turn_left, button_left_event_cb, LV_EVENT_CLICKED, NULL);


    LV_IMG_DECLARE(up);
    lv_obj_t* imgbtn_turn_up = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_turn_up, LV_IMGBTN_STATE_RELEASED, NULL, &up, NULL);
    lv_obj_set_size(imgbtn_turn_up, up.header.w, up.header.h);
    lv_obj_align(imgbtn_turn_up, LV_ALIGN_TOP_LEFT, 357, 207);
    lv_obj_add_event_cb(imgbtn_turn_up, button_up_event_cb, LV_EVENT_CLICKED, NULL);


    LV_IMG_DECLARE(right);
    lv_obj_t* imgbtn_turn_right = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_turn_right, LV_IMGBTN_STATE_RELEASED, NULL, &right, NULL);
    lv_obj_set_size(imgbtn_turn_right, right.header.w, right.header.h);
    lv_obj_align(imgbtn_turn_right, LV_ALIGN_TOP_LEFT, 389, 216);
    lv_obj_add_event_cb(imgbtn_turn_right, button_right_event_cb, LV_EVENT_CLICKED, NULL);


    LV_IMG_DECLARE(down);
    lv_obj_t* imgbtn_turn_down = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_turn_down, LV_IMGBTN_STATE_RELEASED, NULL, &down, NULL);
    lv_obj_set_size(imgbtn_turn_down, down.header.w, down.header.h);
    lv_obj_align(imgbtn_turn_down, LV_ALIGN_TOP_LEFT, 357, 249);
    lv_obj_add_event_cb(imgbtn_turn_down, button_down_event_cb, LV_EVENT_CLICKED, NULL);

    //增大
    LV_IMG_DECLARE(add);
    lv_obj_t* imgbtn_add = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_add, LV_IMGBTN_STATE_RELEASED, NULL, &add, NULL);
    lv_obj_set_size(imgbtn_add, add.header.w, add.header.h);
    lv_obj_align(imgbtn_add, LV_ALIGN_TOP_LEFT, 362, 221);
    lv_obj_add_event_cb(imgbtn_add, button_add_event_cb, LV_EVENT_CLICKED, NULL);

    //减小
    LV_IMG_DECLARE(reduce);
    lv_obj_t* imgbtn_reduce = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_reduce, LV_IMGBTN_STATE_RELEASED, NULL, &reduce, NULL);
    lv_obj_set_size(imgbtn_reduce, reduce.header.w, reduce.header.h);
    lv_obj_align(imgbtn_reduce, LV_ALIGN_TOP_LEFT, 362,237);
    lv_obj_add_event_cb(imgbtn_reduce, button_reduce_event_cb, LV_EVENT_CLICKED, NULL);


    //打印
    LV_IMG_DECLARE(print);
    imgbtn_print = lv_imgbtn_create(lv_scr_act());
    lv_imgbtn_set_src(imgbtn_print, LV_IMGBTN_STATE_RELEASED, NULL, &print, NULL);
    lv_obj_set_size(imgbtn_print, print.header.w, print.header.h);
    lv_obj_align(imgbtn_print, LV_ALIGN_TOP_LEFT, 425, 207);
    lv_obj_add_event_cb(imgbtn_print, button_print_event_cb, LV_EVENT_CLICKED, NULL);


    LV_FONT_DECLARE(heiFont16_1);
    label_print = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_print, &heiFont16_1, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label_print, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_label_set_text(label_print, "打印");
    lv_obj_align(label_print, LV_ALIGN_TOP_LEFT, 428, 251);


#if 0
    LV_IMG_DECLARE(bottom_img);
    Bottom_background_image = lv_img_create(lv_scr_act()); /* 创建图片部件 */
    lv_img_set_src(Bottom_background_image, &bottom_img); /* 设置图片源 */
    /* 设置图片位置 */
    lv_obj_align(Bottom_background_image, LV_ALIGN_TOP_LEFT, 0, 200);
    lv_obj_update_layout(Head_background_image); /* 更新图片参数 */


    LV_IMG_DECLARE(print_img);
    /* 定义并创建图片按钮 */
    print_imgbtn = lv_imgbtn_create(lv_scr_act());
    /* 设置图片释放时的图片 */
    lv_imgbtn_set_src(print_imgbtn, LV_IMGBTN_STATE_RELEASED, NULL, &print_img, NULL);
    /* 设置图片按钮大小 */
    lv_obj_set_size(print_imgbtn, 48, 48);
    /* 设置图片按钮位置 */
    lv_obj_align(print_imgbtn, LV_ALIGN_TOP_LEFT, 430, 201);
    lv_obj_add_event_cb(print_imgbtn, print_event_cb, LV_EVENT_PRESSED, NULL);
    

    LV_IMG_DECLARE(songFont10);
    Print_label = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(Print_label, (const lv_font_t *)&songFont10, 0);
    lv_label_set_text(Print_label, "喷印");
    lv_obj_align(Print_label, LV_ALIGN_TOP_LEFT, 432, 248);
    lv_obj_set_style_text_color(Print_label, lv_color_hex(0xffffff),LV_STATE_DEFAULT);
    
    LV_IMG_DECLARE(songFont28);

    //LV_IMG_DECLARE(lv_font_montserrat_48);
    /* 定义绘制标签 */
    lv_draw_label_dsc_t label_dsc;
    /* 初始化绘制标签 */
    lv_draw_label_dsc_init(&label_dsc);
    /* 设置标签颜色 */
    label_dsc.color = lv_color_black();
    label_dsc.font = (const lv_font_t *)&songFont28;
    //label_dsc.font = font_normal;
    label_dsc.align = LV_TEXT_ALIGN_LEFT;
    /* 在画布上绘制标签 */
    lv_canvas_draw_text(canvas, 10, 10, 1024,&label_dsc,"欢迎使用");

#endif

    	/* 打开beep驱动 */
	fd_buzzer = open("/dev/buzzer", O_RDWR);
	if(fd_buzzer < 0)
    {
		printf("file /dev/buzzer open failed!\r\n");
	}
    else
    {
        printf("file /dev/buzzer open ok!\r\n");
    }

    fd_mp1484_en = open("/dev/mp1484_en", O_RDWR);
	if(fd_mp1484_en < 0)
    {
		printf("file /dev/mp1484_en open failed!\r\n");
	}
    else
    {
        printf("file /dev/mp1484_en open ok!\r\n");
        set_mp1484_en(1);
    }


    fd_bm8563 = open("/dev/bm8563", O_RDWR);
	if(fd_bm8563 < 0)
    {
		printf("file /dev/bm8563 open failed!\r\n");
	}
    else
    {
        printf("file /dev/bm8563 open ok!\r\n");
    }


    fd_adc = open("/dev/adc", O_RDWR);
	if(fd_adc < 0)
    {
		printf("file /dev/adc open failed!\r\n");
	}
    else
    {
        printf("file /dev/adc open ok!\r\n");
    }

    timer_100ms = lv_timer_create(timer_100ms_cb, 100, NULL);
}


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/


#if 0

void lv_demo_widgets(void)
{
    if(LV_HOR_RES <= 320) disp_size = DISP_SMALL;
    else if(LV_HOR_RES < 720) disp_size = DISP_MEDIUM;
    else disp_size = DISP_LARGE;

    font_large = LV_FONT_DEFAULT;
    font_normal = LV_FONT_DEFAULT;

    lv_coord_t tab_h;
    if(disp_size == DISP_LARGE) {
        tab_h = 70;
#if LV_FONT_MONTSERRAT_24
        font_large     = &lv_font_montserrat_24;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_24 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_16
        font_normal    = &lv_font_montserrat_16;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_16 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }
    else if(disp_size == DISP_MEDIUM) {
        tab_h = 45;
#if LV_FONT_MONTSERRAT_20
        font_large     = &lv_font_montserrat_20;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_20 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_14
        font_normal    = &lv_font_montserrat_14;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_14 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }
    else {   /* disp_size == DISP_SMALL */
        tab_h = 45;
#if LV_FONT_MONTSERRAT_18
        font_large     = &lv_font_montserrat_18;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_18 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
#if LV_FONT_MONTSERRAT_12
        font_normal    = &lv_font_montserrat_12;
#else
        LV_LOG_WARN("LV_FONT_MONTSERRAT_12 is not enabled for the widgets demo. Using LV_FONT_DEFAULT instead.");
#endif
    }

#if LV_USE_THEME_DEFAULT
    lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), LV_THEME_DEFAULT_DARK,
                          font_normal);
#endif

    lv_style_init(&style_text_muted);
    lv_style_set_text_opa(&style_text_muted, LV_OPA_50);

    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, font_large);

    lv_style_init(&style_icon);
    lv_style_set_text_color(&style_icon, lv_theme_get_color_primary(NULL));
    lv_style_set_text_font(&style_icon, font_large);

    lv_style_init(&style_bullet);
    lv_style_set_border_width(&style_bullet, 0);
    lv_style_set_radius(&style_bullet, LV_RADIUS_CIRCLE);

    tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, tab_h);

    lv_obj_set_style_text_font(lv_scr_act(), font_normal, 0);

    if(disp_size == DISP_LARGE) {
        lv_obj_t * tab_btns = lv_tabview_get_tab_btns(tv);
        lv_obj_set_style_pad_left(tab_btns, LV_HOR_RES / 2, 0);
        lv_obj_t * logo = lv_img_create(tab_btns);
        LV_IMG_DECLARE(img_lvgl_logo);
        lv_img_set_src(logo, &img_lvgl_logo);
        lv_obj_align(logo, LV_ALIGN_LEFT_MID, -LV_HOR_RES / 2 + 25, 0);

        lv_obj_t * label = lv_label_create(tab_btns);
        lv_obj_add_style(label, &style_title, 0);
        lv_label_set_text(label, "LVGL v8");
        lv_obj_align_to(label, logo, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);

        label = lv_label_create(tab_btns);
        lv_label_set_text(label, "Widgets demo");
        lv_obj_add_style(label, &style_text_muted, 0);
        lv_obj_align_to(label, logo, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);
    }

    lv_obj_t * t1 = lv_tabview_add_tab(tv, "Profile");
    lv_obj_t * t2 = lv_tabview_add_tab(tv, "Analytics");
    lv_obj_t * t3 = lv_tabview_add_tab(tv, "Shop");
    profile_create(t1);
    analytics_create(t2);
    shop_create(t3);

    color_changer_create(tv);
}
#endif

void lv_demo_widgets_close(void)
{
    /*Delete all animation*/
    lv_anim_del(NULL, NULL);

    lv_timer_del(meter2_timer);
    meter2_timer = NULL;

    lv_obj_clean(lv_scr_act());

    lv_style_reset(&style_text_muted);
    lv_style_reset(&style_title);
    lv_style_reset(&style_icon);
    lv_style_reset(&style_bullet);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void profile_create(lv_obj_t * parent)
{
    lv_obj_t * panel1 = lv_obj_create(parent);
    lv_obj_set_height(panel1, LV_SIZE_CONTENT);


    LV_IMG_DECLARE(img_demo_widgets_avatar);
    lv_obj_t * avatar = lv_img_create(panel1);
    lv_img_set_src(avatar, &img_demo_widgets_avatar);

    lv_obj_t * name = lv_label_create(panel1);
    lv_label_set_text(name, "Elena Smith");
    lv_obj_add_style(name, &style_title, 0);

    lv_obj_t * dsc = lv_label_create(panel1);
    lv_obj_add_style(dsc, &style_text_muted, 0);
    lv_label_set_text(dsc, "This is a short description of me. Take a look at my profile!");
    lv_label_set_long_mode(dsc, LV_LABEL_LONG_WRAP);

    lv_obj_t * email_icn = lv_label_create(panel1);
    lv_obj_add_style(email_icn, &style_icon, 0);
    lv_label_set_text(email_icn, LV_SYMBOL_ENVELOPE);

    lv_obj_t * email_label = lv_label_create(panel1);
    lv_label_set_text(email_label, "elena@smith.com");

    lv_obj_t * call_icn = lv_label_create(panel1);
    lv_obj_add_style(call_icn, &style_icon, 0);
    lv_label_set_text(call_icn, LV_SYMBOL_CALL);

    lv_obj_t * call_label = lv_label_create(panel1);
    lv_label_set_text(call_label, "+79 246 123 4567");

    lv_obj_t * log_out_btn = lv_btn_create(panel1);
    lv_obj_set_height(log_out_btn, LV_SIZE_CONTENT);

    lv_obj_t * label = lv_label_create(log_out_btn);
    lv_label_set_text(label, "Log out");
    lv_obj_center(label);

    lv_obj_t * invite_btn = lv_btn_create(panel1);
    lv_obj_add_state(invite_btn, LV_STATE_DISABLED);
    lv_obj_set_height(invite_btn, LV_SIZE_CONTENT);

    label = lv_label_create(invite_btn);
    lv_label_set_text(label, "Invite");
    lv_obj_center(label);

    /*Create a keyboard*/
    lv_obj_t * kb = lv_keyboard_create(lv_scr_act());
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    /*Create the second panel*/
    lv_obj_t * panel2 = lv_obj_create(parent);
    lv_obj_set_height(panel2, LV_SIZE_CONTENT);

    lv_obj_t * panel2_title = lv_label_create(panel2);
    lv_label_set_text(panel2_title, "Your profile");
    lv_obj_add_style(panel2_title, &style_title, 0);

    lv_obj_t * user_name_label = lv_label_create(panel2);
    lv_label_set_text(user_name_label, "User name");
    lv_obj_add_style(user_name_label, &style_text_muted, 0);

    lv_obj_t * user_name = lv_textarea_create(panel2);
    lv_textarea_set_one_line(user_name, true);
    lv_textarea_set_placeholder_text(user_name, "Your name");
    lv_obj_add_event_cb(user_name, ta_event_cb, LV_EVENT_ALL, kb);

    lv_obj_t * password_label = lv_label_create(panel2);
    lv_label_set_text(password_label, "Password");
    lv_obj_add_style(password_label, &style_text_muted, 0);

    lv_obj_t * password = lv_textarea_create(panel2);
    lv_textarea_set_one_line(password, true);
    lv_textarea_set_password_mode(password, true);
    lv_textarea_set_placeholder_text(password, "Min. 8 chars.");
    lv_obj_add_event_cb(password, ta_event_cb, LV_EVENT_ALL, kb);

    lv_obj_t * gender_label = lv_label_create(panel2);
    lv_label_set_text(gender_label, "Gender");
    lv_obj_add_style(gender_label, &style_text_muted, 0);

    lv_obj_t * gender = lv_dropdown_create(panel2);
    lv_dropdown_set_options_static(gender, "Male\nFemale\nOther");

    lv_obj_t * birthday_label = lv_label_create(panel2);
    lv_label_set_text(birthday_label, "Birthday");
    lv_obj_add_style(birthday_label, &style_text_muted, 0);

    lv_obj_t * birthdate = lv_textarea_create(panel2);
    lv_textarea_set_one_line(birthdate, true);
    lv_obj_add_event_cb(birthdate, birthday_event_cb, LV_EVENT_ALL, NULL);

    /*Create the third panel*/
    lv_obj_t * panel3 = lv_obj_create(parent);
    lv_obj_t * panel3_title = lv_label_create(panel3);
    lv_label_set_text(panel3_title, "Your skills");
    lv_obj_add_style(panel3_title, &style_title, 0);

    lv_obj_t * experience_label = lv_label_create(panel3);
    lv_label_set_text(experience_label, "Experience");
    lv_obj_add_style(experience_label, &style_text_muted, 0);

    lv_obj_t * slider1 = lv_slider_create(panel3);
    lv_obj_set_width(slider1, LV_PCT(95));
    lv_obj_add_event_cb(slider1, slider_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_refresh_ext_draw_size(slider1);

    lv_obj_t * team_player_label = lv_label_create(panel3);
    lv_label_set_text(team_player_label, "Team player");
    lv_obj_add_style(team_player_label, &style_text_muted, 0);

    lv_obj_t * sw1 = lv_switch_create(panel3);

    lv_obj_t * hard_working_label = lv_label_create(panel3);
    lv_label_set_text(hard_working_label, "Hard-working");
    lv_obj_add_style(hard_working_label, &style_text_muted, 0);

    lv_obj_t * sw2 = lv_switch_create(panel3);

    if(disp_size == DISP_LARGE) {
        static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

        /*Create the top panel*/
        static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, 5, LV_GRID_CONTENT, LV_GRID_FR(2), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_1_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, 10, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

        static lv_coord_t grid_2_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_2_row_dsc[] = {
            LV_GRID_CONTENT,  /*Title*/
            5,                /*Separator*/
            LV_GRID_CONTENT,  /*Box title*/
            30,               /*Boxes*/
            5,                /*Separator*/
            LV_GRID_CONTENT,  /*Box title*/
            30,               /*Boxes*/
            LV_GRID_TEMPLATE_LAST
        };


        lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);

        lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);

        lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);
        lv_obj_set_grid_cell(avatar, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 5);
        lv_obj_set_grid_cell(name, LV_GRID_ALIGN_START, 2, 2, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(dsc, LV_GRID_ALIGN_STRETCH, 2, 4, LV_GRID_ALIGN_START, 1, 1);
        lv_obj_set_grid_cell(email_icn, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(email_label, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(call_icn, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 4, 1);
        lv_obj_set_grid_cell(call_label, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_CENTER, 4, 1);
        lv_obj_set_grid_cell(log_out_btn, LV_GRID_ALIGN_STRETCH, 4, 1, LV_GRID_ALIGN_CENTER, 3, 2);
        lv_obj_set_grid_cell(invite_btn, LV_GRID_ALIGN_STRETCH, 5, 1, LV_GRID_ALIGN_CENTER, 3, 2);

        lv_obj_set_grid_cell(panel2, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 1, 1);
        lv_obj_set_grid_dsc_array(panel2, grid_2_col_dsc, grid_2_row_dsc);
        lv_obj_set_grid_cell(panel2_title, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(user_name, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(user_name_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(password, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(password_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(birthdate, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 6, 1);
        lv_obj_set_grid_cell(birthday_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 5, 1);
        lv_obj_set_grid_cell(gender, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 6, 1);
        lv_obj_set_grid_cell(gender_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 5, 1);


        lv_obj_set_grid_cell(panel3, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
        lv_obj_set_grid_dsc_array(panel3, grid_2_col_dsc, grid_2_row_dsc);
        lv_obj_set_grid_cell(panel3_title, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(slider1, LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(experience_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(sw2, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 6, 1);
        lv_obj_set_grid_cell(hard_working_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 5, 1);
        lv_obj_set_grid_cell(sw1, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 6, 1);
        lv_obj_set_grid_cell(team_player_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 5, 1);
    }
    else if(disp_size == DISP_MEDIUM) {
        static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};


        /*Create the top panel*/
        static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, 1, LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_1_row_dsc[] = {
            LV_GRID_CONTENT, /*Name*/
            LV_GRID_CONTENT, /*Description*/
            LV_GRID_CONTENT, /*Email*/
            -20,
            LV_GRID_CONTENT, /*Phone*/
            LV_GRID_CONTENT, /*Buttons*/
            LV_GRID_TEMPLATE_LAST
        };

        static lv_coord_t grid_2_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_2_row_dsc[] = {
            LV_GRID_CONTENT,  /*Title*/
            5,                /*Separator*/
            LV_GRID_CONTENT,  /*Box title*/
            40,               /*Box*/
            LV_GRID_CONTENT,  /*Box title*/
            40,               /*Box*/
            LV_GRID_CONTENT,  /*Box title*/
            40,               /*Box*/
            LV_GRID_CONTENT,  /*Box title*/
            40,               /*Box*/
            LV_GRID_TEMPLATE_LAST
        };


        lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);
        lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);

        lv_obj_set_width(log_out_btn, 120);
        lv_obj_set_width(invite_btn, 120);

        lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);
        lv_obj_set_grid_cell(avatar, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_START, 0, 4);
        lv_obj_set_grid_cell(name, LV_GRID_ALIGN_START, 2, 2, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(dsc, LV_GRID_ALIGN_STRETCH, 2, 2, LV_GRID_ALIGN_START, 1, 1);
        lv_obj_set_grid_cell(email_label, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_CENTER, 2, 1);
        lv_obj_set_grid_cell(email_icn, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 2, 1);
        lv_obj_set_grid_cell(call_icn, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 4, 1);
        lv_obj_set_grid_cell(call_label, LV_GRID_ALIGN_START, 3, 1, LV_GRID_ALIGN_CENTER, 4, 1);
        lv_obj_set_grid_cell(log_out_btn, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 5, 1);
        lv_obj_set_grid_cell(invite_btn, LV_GRID_ALIGN_END, 3, 1, LV_GRID_ALIGN_CENTER, 5, 1);

        lv_obj_set_grid_cell(panel2, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 1, 1);
        lv_obj_set_grid_dsc_array(panel2, grid_2_col_dsc, grid_2_row_dsc);
        lv_obj_set_grid_cell(panel2_title, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(user_name_label, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(user_name, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 3, 1);
        lv_obj_set_grid_cell(password_label, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 4, 1);
        lv_obj_set_grid_cell(password, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 5, 1);
        lv_obj_set_grid_cell(birthday_label, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 6, 1);
        lv_obj_set_grid_cell(birthdate, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 7, 1);
        lv_obj_set_grid_cell(gender_label, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 8, 1);
        lv_obj_set_grid_cell(gender, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 9, 1);

        lv_obj_set_grid_cell(panel3, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
        lv_obj_set_grid_dsc_array(panel3, grid_2_col_dsc, grid_2_row_dsc);
        lv_obj_set_grid_cell(panel3_title, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(slider1, LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(experience_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(hard_working_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 4, 1);
        lv_obj_set_grid_cell(sw2, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 5, 1);
        lv_obj_set_grid_cell(team_player_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 6, 1);
        lv_obj_set_grid_cell(sw1, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 7, 1);
    }
    else if(disp_size == DISP_SMALL) {
        static lv_coord_t grid_main_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_main_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
        lv_obj_set_grid_dsc_array(parent, grid_main_col_dsc, grid_main_row_dsc);


        /*Create the top panel*/
        static lv_coord_t grid_1_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_1_row_dsc[] = {LV_GRID_CONTENT, /*Avatar*/
                                              LV_GRID_CONTENT, /*Name*/
                                              LV_GRID_CONTENT, /*Description*/
                                              LV_GRID_CONTENT, /*Email*/
                                              LV_GRID_CONTENT, /*Phone number*/
                                              LV_GRID_CONTENT, /*Button1*/
                                              LV_GRID_CONTENT, /*Button2*/
                                              LV_GRID_TEMPLATE_LAST
                                             };

        lv_obj_set_grid_dsc_array(panel1, grid_1_col_dsc, grid_1_row_dsc);


        static lv_coord_t grid_2_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_2_row_dsc[] = {
            LV_GRID_CONTENT,  /*Title*/
            5,                /*Separator*/
            LV_GRID_CONTENT,  /*Box title*/
            40,               /*Box*/
            LV_GRID_CONTENT,  /*Box title*/
            40,               /*Box*/
            LV_GRID_CONTENT,  /*Box title*/
            40,               /*Box*/
            LV_GRID_CONTENT,  /*Box title*/
            40, LV_GRID_TEMPLATE_LAST               /*Box*/
        };

        lv_obj_set_grid_dsc_array(panel2, grid_2_col_dsc, grid_2_row_dsc);
        lv_obj_set_grid_dsc_array(panel3, grid_2_col_dsc, grid_2_row_dsc);

        lv_obj_set_grid_cell(panel1, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);

        lv_obj_set_style_text_align(dsc, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_set_grid_cell(avatar, LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(name, LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 1, 1);
        lv_obj_set_grid_cell(dsc, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(email_icn, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(email_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(call_icn, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
        lv_obj_set_grid_cell(call_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 4, 1);
        lv_obj_set_grid_cell(log_out_btn, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 5, 1);
        lv_obj_set_grid_cell(invite_btn, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 6, 1);

        lv_obj_set_grid_cell(panel2, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 1, 1);
        lv_obj_set_grid_cell(panel2_title, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(user_name_label, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(user_name, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 3, 1);
        lv_obj_set_grid_cell(password_label, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 4, 1);
        lv_obj_set_grid_cell(password, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 5, 1);
        lv_obj_set_grid_cell(birthday_label, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 6, 1);
        lv_obj_set_grid_cell(birthdate, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 7, 1);
        lv_obj_set_grid_cell(gender_label, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 8, 1);
        lv_obj_set_grid_cell(gender, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_START, 9, 1);

        lv_obj_set_height(panel3, LV_SIZE_CONTENT);
        lv_obj_set_grid_cell(panel3, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(panel3_title, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(experience_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(slider1, LV_GRID_ALIGN_CENTER, 0, 2, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(hard_working_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 4, 1);
        lv_obj_set_grid_cell(sw1, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 5, 1);
        lv_obj_set_grid_cell(team_player_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 4, 1);
        lv_obj_set_grid_cell(sw2, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_START, 5, 1);
    }
}


static void analytics_create(lv_obj_t * parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);

    static lv_coord_t grid_chart_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), 10, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_chart_col_dsc[] = {20, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_t * chart1_cont = lv_obj_create(parent);
    lv_obj_set_flex_grow(chart1_cont, 1);
    lv_obj_set_grid_dsc_array(chart1_cont, grid_chart_col_dsc, grid_chart_row_dsc);

    lv_obj_set_height(chart1_cont, LV_PCT(100));
    lv_obj_set_style_max_height(chart1_cont, 300, 0);

    lv_obj_t * title = lv_label_create(chart1_cont);
    lv_label_set_text(title, "Unique visitors");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 0, 1);

    chart1 = lv_chart_create(chart1_cont);
    lv_group_add_obj(lv_group_get_default(), chart1);
    lv_obj_add_flag(chart1, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_grid_cell(chart1, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_chart_set_axis_tick(chart1, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 5, 1, true, 80);
    lv_chart_set_axis_tick(chart1, LV_CHART_AXIS_PRIMARY_X, 0, 0, 12, 1, true, 50);
    lv_chart_set_div_line_count(chart1, 0, 12);
    lv_chart_set_point_count(chart1, 12);
    lv_obj_add_event_cb(chart1, chart_event_cb, LV_EVENT_ALL, NULL);
    if(disp_size == DISP_SMALL) lv_chart_set_zoom_x(chart1, 256 * 3);
    else if(disp_size == DISP_MEDIUM) lv_chart_set_zoom_x(chart1, 256 * 2);

    lv_obj_set_style_border_side(chart1, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(chart1, 0, 0);

    ser1 = lv_chart_add_series(chart1, lv_theme_get_color_primary(chart1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));
    lv_chart_set_next_value(chart1, ser1, lv_rand(10, 80));

    lv_obj_t * chart2_cont = lv_obj_create(parent);
    lv_obj_add_flag(chart2_cont, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_set_flex_grow(chart2_cont, 1);

    lv_obj_set_height(chart2_cont, LV_PCT(100));
    lv_obj_set_style_max_height(chart2_cont, 300, 0);

    lv_obj_set_grid_dsc_array(chart2_cont, grid_chart_col_dsc, grid_chart_row_dsc);

    title = lv_label_create(chart2_cont);
    lv_label_set_text(title, "Monthly revenue");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 0, 1);

    chart2 = lv_chart_create(chart2_cont);
    lv_group_add_obj(lv_group_get_default(), chart2);
    lv_obj_add_flag(chart2, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    lv_obj_set_grid_cell(chart2, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_chart_set_axis_tick(chart2, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 5, 1, true, 80);
    lv_chart_set_axis_tick(chart2, LV_CHART_AXIS_PRIMARY_X, 0, 0, 12, 1, true, 50);
    lv_obj_set_size(chart2, LV_PCT(100), LV_PCT(100));
    lv_chart_set_type(chart2, LV_CHART_TYPE_BAR);
    lv_chart_set_div_line_count(chart2, 6, 0);
    lv_chart_set_point_count(chart2, 12);
    lv_obj_add_event_cb(chart2, chart_event_cb, LV_EVENT_ALL, NULL);
    lv_chart_set_zoom_x(chart2, 256 * 2);
    lv_obj_set_style_border_side(chart2, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(chart2, 0, 0);

    if(disp_size == DISP_SMALL) {
        lv_obj_set_style_pad_gap(chart2, 0, LV_PART_ITEMS);
        lv_obj_set_style_pad_gap(chart2, 2, LV_PART_MAIN);
    }
    else if(disp_size == DISP_LARGE) {
        lv_obj_set_style_pad_gap(chart2, 16, 0);
    }

    ser2 = lv_chart_add_series(chart2, lv_palette_lighten(LV_PALETTE_GREY, 1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser2, lv_rand(10, 80));

    ser3 = lv_chart_add_series(chart2, lv_theme_get_color_primary(chart1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));
    lv_chart_set_next_value(chart2, ser3, lv_rand(10, 80));

    lv_meter_scale_t * scale;
    lv_meter_indicator_t * indic;
    meter1 = create_meter_box(parent, "Monthly Target", "Revenue: 63%", "Sales: 44%", "Costs: 58%");
    lv_obj_add_flag(lv_obj_get_parent(meter1), LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    scale = lv_meter_add_scale(meter1);
    lv_meter_set_scale_range(meter1, scale, 0, 100, 270, 90);
    lv_meter_set_scale_ticks(meter1, scale, 0, 0, 0, lv_color_black());

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_values(&a, 20, 100);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);

    indic = lv_meter_add_arc(meter1, scale, 15, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_anim_set_exec_cb(&a, meter1_indic1_anim_cb);
    lv_anim_set_var(&a, indic);
    lv_anim_set_time(&a, 4100);
    lv_anim_set_playback_time(&a, 2700);
    lv_anim_start(&a);

    indic = lv_meter_add_arc(meter1, scale, 15, lv_palette_main(LV_PALETTE_RED), -20);
    lv_anim_set_exec_cb(&a, meter1_indic2_anim_cb);
    lv_anim_set_var(&a, indic);
    lv_anim_set_time(&a, 2600);
    lv_anim_set_playback_time(&a, 3200);
    a.user_data = indic;
    lv_anim_start(&a);

    indic = lv_meter_add_arc(meter1, scale, 15, lv_palette_main(LV_PALETTE_GREEN), -40);
    lv_anim_set_exec_cb(&a, meter1_indic3_anim_cb);
    lv_anim_set_var(&a, indic);
    lv_anim_set_time(&a, 2800);
    lv_anim_set_playback_time(&a, 1800);
    lv_anim_start(&a);

    meter2 = create_meter_box(parent, "Sessions", "Desktop: ", "Tablet: ", "Mobile: ");
    if(disp_size < DISP_LARGE) lv_obj_add_flag(lv_obj_get_parent(meter2), LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    scale = lv_meter_add_scale(meter2);
    lv_meter_set_scale_range(meter2, scale, 0, 100, 360, 90);
    lv_meter_set_scale_ticks(meter2, scale, 0, 0, 0, lv_color_black());

    static lv_meter_indicator_t * meter2_indic[3];
    meter2_indic[0] = lv_meter_add_arc(meter2, scale, 20, lv_palette_main(LV_PALETTE_RED), -10);
    lv_meter_set_indicator_start_value(meter2, meter2_indic[0], 0);
    lv_meter_set_indicator_end_value(meter2, meter2_indic[0], 39);

    meter2_indic[1] = lv_meter_add_arc(meter2, scale, 30, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(meter2, meter2_indic[1], 40);
    lv_meter_set_indicator_end_value(meter2, meter2_indic[1], 69);

    meter2_indic[2] = lv_meter_add_arc(meter2, scale, 10, lv_palette_main(LV_PALETTE_GREEN), -20);
    lv_meter_set_indicator_start_value(meter2, meter2_indic[2], 70);
    lv_meter_set_indicator_end_value(meter2, meter2_indic[2], 99);

    meter2_timer = lv_timer_create(meter2_timer_cb, 100, meter2_indic);

    meter3 = create_meter_box(parent, "Network Speed", "Low speed", "Normal Speed", "High Speed");
    if(disp_size < DISP_LARGE) lv_obj_add_flag(lv_obj_get_parent(meter3), LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    /*Add a special circle to the needle's pivot*/
    lv_obj_set_style_pad_hor(meter3, 10, 0);
    lv_obj_set_style_size(meter3, 10, LV_PART_INDICATOR);
    lv_obj_set_style_radius(meter3, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(meter3, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(meter3, lv_palette_darken(LV_PALETTE_GREY, 4), LV_PART_INDICATOR);
    lv_obj_set_style_outline_color(meter3, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(meter3, 3, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(meter3, lv_palette_darken(LV_PALETTE_GREY, 1), LV_PART_TICKS);

    scale = lv_meter_add_scale(meter3);
    lv_meter_set_scale_range(meter3, scale, 10, 60, 220, 360 - 220);
    lv_meter_set_scale_ticks(meter3, scale, 21, 3, 17, lv_color_white());
    lv_meter_set_scale_major_ticks(meter3, scale, 4, 4, 22, lv_color_white(), 15);

    indic = lv_meter_add_arc(meter3, scale, 10, lv_palette_main(LV_PALETTE_RED), 0);
    lv_meter_set_indicator_start_value(meter3, indic, 0);
    lv_meter_set_indicator_end_value(meter3, indic, 20);

    indic = lv_meter_add_scale_lines(meter3, scale, lv_palette_darken(LV_PALETTE_RED, 3), lv_palette_darken(LV_PALETTE_RED,
                                                                                                            3), true, 0);
    lv_meter_set_indicator_start_value(meter3, indic, 0);
    lv_meter_set_indicator_end_value(meter3, indic, 20);

    indic = lv_meter_add_arc(meter3, scale, 12, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(meter3, indic, 20);
    lv_meter_set_indicator_end_value(meter3, indic, 40);

    indic = lv_meter_add_scale_lines(meter3, scale, lv_palette_darken(LV_PALETTE_BLUE, 3),
                                     lv_palette_darken(LV_PALETTE_BLUE, 3), true, 0);
    lv_meter_set_indicator_start_value(meter3, indic, 20);
    lv_meter_set_indicator_end_value(meter3, indic, 40);

    indic = lv_meter_add_arc(meter3, scale, 10, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_meter_set_indicator_start_value(meter3, indic, 40);
    lv_meter_set_indicator_end_value(meter3, indic, 60);

    indic = lv_meter_add_scale_lines(meter3, scale, lv_palette_darken(LV_PALETTE_GREEN, 3),
                                     lv_palette_darken(LV_PALETTE_GREEN, 3), true, 0);
    lv_meter_set_indicator_start_value(meter3, indic, 40);
    lv_meter_set_indicator_end_value(meter3, indic, 60);

    indic = lv_meter_add_needle_line(meter3, scale, 4, lv_palette_darken(LV_PALETTE_GREY, 4), -25);

    lv_obj_t * mbps_label = lv_label_create(meter3);
    lv_label_set_text(mbps_label, "-");
    lv_obj_add_style(mbps_label, &style_title, 0);

    lv_obj_t * mbps_unit_label = lv_label_create(meter3);
    lv_label_set_text(mbps_unit_label, "Mbps");

    lv_anim_init(&a);
    lv_anim_set_values(&a, 10, 60);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, meter3_anim_cb);
    lv_anim_set_var(&a, indic);
    lv_anim_set_time(&a, 4100);
    lv_anim_set_playback_time(&a, 800);
    lv_anim_start(&a);

    lv_obj_update_layout(parent);
    if(disp_size == DISP_MEDIUM) {
        lv_obj_set_size(meter1, 200, 200);
        lv_obj_set_size(meter2, 200, 200);
        lv_obj_set_size(meter3, 200, 200);
    }
    else {
        lv_coord_t meter_w = lv_obj_get_width(meter1);
        lv_obj_set_height(meter1, meter_w);
        lv_obj_set_height(meter2, meter_w);
        lv_obj_set_height(meter3, meter_w);
    }

    lv_obj_align(mbps_label, LV_ALIGN_TOP_MID, 10, lv_pct(55));
    lv_obj_align_to(mbps_unit_label, mbps_label, LV_ALIGN_OUT_RIGHT_BOTTOM, 10, 0);
}

void shop_create(lv_obj_t * parent)
{
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);

    lv_obj_t * panel1 = lv_obj_create(parent);
    lv_obj_set_size(panel1, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_bottom(panel1, 30, 0);

    lv_obj_t * title = lv_label_create(panel1);
    lv_label_set_text(title, "Monthly Summary");
    lv_obj_add_style(title, &style_title, 0);

    lv_obj_t * date = lv_label_create(panel1);
    lv_label_set_text(date, "8-15 July, 2021");
    lv_obj_add_style(date, &style_text_muted, 0);

    lv_obj_t * amount = lv_label_create(panel1);
    lv_label_set_text(amount, "$27,123.25");
    lv_obj_add_style(amount, &style_title, 0);

    lv_obj_t * hint = lv_label_create(panel1);
    lv_label_set_text(hint, LV_SYMBOL_UP" 17% growth this week");
    lv_obj_set_style_text_color(hint, lv_palette_main(LV_PALETTE_GREEN), 0);

    chart3 = lv_chart_create(panel1);
    lv_chart_set_axis_tick(chart3, LV_CHART_AXIS_PRIMARY_Y, 0, 0, 6, 1, true, 80);
    lv_chart_set_axis_tick(chart3, LV_CHART_AXIS_PRIMARY_X, 0, 0, 7, 1, true, 50);
    lv_chart_set_type(chart3, LV_CHART_TYPE_BAR);
    lv_chart_set_div_line_count(chart3, 6, 0);
    lv_chart_set_point_count(chart3, 7);
    lv_obj_add_event_cb(chart3, shop_chart_event_cb, LV_EVENT_ALL, NULL);

    ser4 = lv_chart_add_series(chart3, lv_theme_get_color_primary(chart3), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));
    lv_chart_set_next_value(chart3, ser4, lv_rand(60, 90));

    if(disp_size == DISP_LARGE) {
        static lv_coord_t grid1_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid1_row_dsc[] = {
            LV_GRID_CONTENT,  /*Title*/
            LV_GRID_CONTENT,  /*Sub title*/
            20,               /*Spacer*/
            LV_GRID_CONTENT,  /*Amount*/
            LV_GRID_CONTENT,  /*Hint*/
            LV_GRID_TEMPLATE_LAST
        };

        lv_obj_set_size(chart3, lv_pct(100), lv_pct(100));
        lv_obj_set_style_pad_column(chart3, LV_DPX(30), 0);


        lv_obj_set_grid_dsc_array(panel1, grid1_col_dsc, grid1_row_dsc);
        lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);
        lv_obj_set_grid_cell(date, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 1, 1);
        lv_obj_set_grid_cell(amount, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 3, 1);
        lv_obj_set_grid_cell(hint, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 4, 1);
        lv_obj_set_grid_cell(chart3, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 5);
    }
    else if(disp_size == DISP_MEDIUM) {
        static lv_coord_t grid1_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid1_row_dsc[] = {
            LV_GRID_CONTENT,  /*Title + Date*/
            LV_GRID_CONTENT,  /*Amount + Hint*/
            200,              /*Chart*/
            LV_GRID_TEMPLATE_LAST
        };

        lv_obj_update_layout(panel1);
        lv_obj_set_width(chart3, lv_obj_get_content_width(panel1) - 20);
        lv_obj_set_style_pad_column(chart3, LV_DPX(30), 0);

        lv_obj_set_grid_dsc_array(panel1, grid1_col_dsc, grid1_row_dsc);
        lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(date, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
        lv_obj_set_grid_cell(amount, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
        lv_obj_set_grid_cell(hint, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
        lv_obj_set_grid_cell(chart3, LV_GRID_ALIGN_END, 0, 2, LV_GRID_ALIGN_STRETCH, 2, 1);
    }
    else if(disp_size == DISP_SMALL) {
        static lv_coord_t grid1_col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid1_row_dsc[] = {
            LV_GRID_CONTENT,  /*Title*/
            LV_GRID_CONTENT,  /*Date*/
            LV_GRID_CONTENT,  /*Amount*/
            LV_GRID_CONTENT,  /*Hint*/
            LV_GRID_CONTENT,  /*Chart*/
            LV_GRID_TEMPLATE_LAST
        };

        lv_obj_set_width(chart3, LV_PCT(95));
        lv_obj_set_height(chart3, LV_VER_RES - 70);
        lv_obj_set_style_max_height(chart3, 300, 0);
        lv_chart_set_zoom_x(chart3, 512);

        lv_obj_set_grid_dsc_array(panel1, grid1_col_dsc, grid1_row_dsc);
        lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 1);
        lv_obj_set_grid_cell(date, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 1, 1);
        lv_obj_set_grid_cell(amount, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(hint, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 3, 1);
        lv_obj_set_grid_cell(chart3, LV_GRID_ALIGN_END, 0, 1, LV_GRID_ALIGN_START, 4, 1);
    }

    lv_obj_t * list = lv_obj_create(parent);
    if(disp_size == DISP_SMALL) {
        lv_obj_add_flag(list, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_set_height(list, LV_PCT(100));
    }
    else {
        lv_obj_set_height(list, LV_PCT(100));
        lv_obj_set_style_max_height(list, 300, 0);
    }

    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(list, 1);
    lv_obj_add_flag(list, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);

    title = lv_label_create(list);
    lv_label_set_text(title, "Top products");
    lv_obj_add_style(title, &style_title, 0);

    LV_IMG_DECLARE(img_clothes);
    create_shop_item(list, &img_clothes, "Blue jeans", "Clothes", "$722");
    create_shop_item(list, &img_clothes, "Blue jeans", "Clothes", "$411");
    create_shop_item(list, &img_clothes, "Blue jeans", "Clothes", "$917");
    create_shop_item(list, &img_clothes, "Blue jeans", "Clothes", "$64");
    create_shop_item(list, &img_clothes, "Blue jeans", "Clothes", "$805");

    lv_obj_t * notifications = lv_obj_create(parent);
    if(disp_size == DISP_SMALL) {
        lv_obj_add_flag(notifications, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
        lv_obj_set_height(notifications, LV_PCT(100));
    }
    else  {
        lv_obj_set_height(notifications, LV_PCT(100));
        lv_obj_set_style_max_height(notifications, 300, 0);
    }

    lv_obj_set_flex_flow(notifications, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(notifications, 1);

    title = lv_label_create(notifications);
    lv_label_set_text(title, "Notification");
    lv_obj_add_style(title, &style_title, 0);

    lv_obj_t * cb;
    cb = lv_checkbox_create(notifications);
    lv_checkbox_set_text(cb, "Item purchased");

    cb = lv_checkbox_create(notifications);
    lv_checkbox_set_text(cb, "New connection");

    cb = lv_checkbox_create(notifications);
    lv_checkbox_set_text(cb, "New subscriber");
    lv_obj_add_state(cb, LV_STATE_CHECKED);

    cb = lv_checkbox_create(notifications);
    lv_checkbox_set_text(cb, "New message");
    lv_obj_add_state(cb, LV_STATE_DISABLED);

    cb = lv_checkbox_create(notifications);
    lv_checkbox_set_text(cb, "Milestone reached");
    lv_obj_add_state(cb, LV_STATE_CHECKED | LV_STATE_DISABLED);

    cb = lv_checkbox_create(notifications);
    lv_checkbox_set_text(cb, "Out of stock");


}

static void color_changer_create(lv_obj_t * parent)
{
    static lv_palette_t palette[] = {
        LV_PALETTE_BLUE, LV_PALETTE_GREEN, LV_PALETTE_BLUE_GREY,  LV_PALETTE_ORANGE,
        LV_PALETTE_RED, LV_PALETTE_PURPLE, LV_PALETTE_TEAL, _LV_PALETTE_LAST
    };

    lv_obj_t * color_cont = lv_obj_create(parent);
    lv_obj_remove_style_all(color_cont);
    lv_obj_set_flex_flow(color_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(color_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(color_cont, LV_OBJ_FLAG_FLOATING);

    lv_obj_set_style_bg_color(color_cont, lv_color_white(), 0);
    lv_obj_set_style_pad_right(color_cont, disp_size == DISP_SMALL ? LV_DPX(47) : LV_DPX(55), 0);
    lv_obj_set_style_bg_opa(color_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(color_cont, LV_RADIUS_CIRCLE, 0);

    if(disp_size == DISP_SMALL) lv_obj_set_size(color_cont, LV_DPX(52), LV_DPX(52));
    else lv_obj_set_size(color_cont, LV_DPX(60), LV_DPX(60));

    lv_obj_align(color_cont, LV_ALIGN_BOTTOM_RIGHT, - LV_DPX(10),  - LV_DPX(10));

    uint32_t i;
    for(i = 0; palette[i] != _LV_PALETTE_LAST; i++) {
        lv_obj_t * c = lv_btn_create(color_cont);
        lv_obj_set_style_bg_color(c, lv_palette_main(palette[i]), 0);
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_opa(c, LV_OPA_TRANSP, 0);
        lv_obj_set_size(c, 20, 20);
        lv_obj_add_event_cb(c, color_event_cb, LV_EVENT_ALL, &palette[i]);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    }

    lv_obj_t * btn = lv_btn_create(parent);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(btn, lv_color_white(), LV_STATE_CHECKED);
    lv_obj_set_style_pad_all(btn, 10, 0);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_add_event_cb(btn, color_changer_event_cb, LV_EVENT_ALL, color_cont);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_TINT, 0);

    if(disp_size == DISP_SMALL) {
        lv_obj_set_size(btn, LV_DPX(42), LV_DPX(42));
        lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -LV_DPX(15), -LV_DPX(15));
    }
    else {
        lv_obj_set_size(btn, LV_DPX(50), LV_DPX(50));
        lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -LV_DPX(15), -LV_DPX(15));
    }
}

static void color_changer_anim_cb(void * var, int32_t v)
{
    lv_obj_t * obj = var;
    lv_coord_t max_w = lv_obj_get_width(lv_obj_get_parent(obj)) - LV_DPX(20);
    lv_coord_t w;

    if(disp_size == DISP_SMALL) {
        w = lv_map(v, 0, 256, LV_DPX(52), max_w);
        lv_obj_set_width(obj, w);
        lv_obj_align(obj, LV_ALIGN_BOTTOM_RIGHT, - LV_DPX(10),  - LV_DPX(10));
    }
    else {
        w = lv_map(v, 0, 256, LV_DPX(60), max_w);
        lv_obj_set_width(obj, w);
        lv_obj_align(obj, LV_ALIGN_BOTTOM_RIGHT, - LV_DPX(10),  - LV_DPX(10));
    }

    if(v > LV_OPA_COVER) v = LV_OPA_COVER;

    uint32_t i;
    for(i = 0; i < lv_obj_get_child_cnt(obj); i++) {
        lv_obj_set_style_opa(lv_obj_get_child(obj, i), v, 0);
    }

}

static void color_changer_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t * color_cont = lv_event_get_user_data(e);
        if(lv_obj_get_width(color_cont) < LV_HOR_RES / 2) {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, color_cont);
            lv_anim_set_exec_cb(&a, color_changer_anim_cb);
            lv_anim_set_values(&a, 0, 256);
            lv_anim_set_time(&a, 200);
            lv_anim_start(&a);
        }
        else {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, color_cont);
            lv_anim_set_exec_cb(&a, color_changer_anim_cb);
            lv_anim_set_values(&a, 256, 0);
            lv_anim_set_time(&a, 200);
            lv_anim_start(&a);
        }
    }
}
static void color_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_FOCUSED) {
        lv_obj_t * color_cont = lv_obj_get_parent(obj);
        if(lv_obj_get_width(color_cont) < LV_HOR_RES / 2) {
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, color_cont);
            lv_anim_set_exec_cb(&a, color_changer_anim_cb);
            lv_anim_set_values(&a, 0, 256);
            lv_anim_set_time(&a, 200);
            lv_anim_start(&a);
        }
    }
    else if(code == LV_EVENT_CLICKED) {
        lv_palette_t * palette_primary = lv_event_get_user_data(e);
        lv_palette_t palette_secondary = (*palette_primary) + 3; /*Use another palette as secondary*/
        if(palette_secondary >= _LV_PALETTE_LAST) palette_secondary = 0;
#if LV_USE_THEME_DEFAULT
        lv_theme_default_init(NULL, lv_palette_main(*palette_primary), lv_palette_main(palette_secondary),
                              LV_THEME_DEFAULT_DARK, font_normal);
#endif
        lv_color_t color = lv_palette_main(*palette_primary);
        lv_style_set_text_color(&style_icon, color);
        lv_chart_set_series_color(chart1, ser1, color);
        lv_chart_set_series_color(chart2, ser3, color);
    }
}

static lv_obj_t * create_meter_box(lv_obj_t * parent, const char * title, const char * text1, const char * text2,
                                   const char * text3)
{
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(cont, 1);

    lv_obj_t * title_label = lv_label_create(cont);
    lv_label_set_text(title_label, title);
    lv_obj_add_style(title_label, &style_title, 0);

    lv_obj_t * meter = lv_meter_create(cont);
    lv_obj_remove_style(meter, NULL, LV_PART_MAIN);
    lv_obj_remove_style(meter, NULL, LV_PART_INDICATOR);
    lv_obj_set_width(meter, LV_PCT(100));

    lv_obj_t * bullet1 = lv_obj_create(cont);
    lv_obj_set_size(bullet1, 13, 13);
    lv_obj_remove_style(bullet1, NULL, LV_PART_SCROLLBAR);
    lv_obj_add_style(bullet1, &style_bullet, 0);
    lv_obj_set_style_bg_color(bullet1, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_t * label1 = lv_label_create(cont);
    lv_label_set_text(label1, text1);

    lv_obj_t * bullet2 = lv_obj_create(cont);
    lv_obj_set_size(bullet2, 13, 13);
    lv_obj_remove_style(bullet2, NULL, LV_PART_SCROLLBAR);
    lv_obj_add_style(bullet2, &style_bullet, 0);
    lv_obj_set_style_bg_color(bullet2, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_t * label2 = lv_label_create(cont);
    lv_label_set_text(label2, text2);

    lv_obj_t * bullet3 = lv_obj_create(cont);
    lv_obj_set_size(bullet3, 13, 13);
    lv_obj_remove_style(bullet3,  NULL, LV_PART_SCROLLBAR);
    lv_obj_add_style(bullet3, &style_bullet, 0);
    lv_obj_set_style_bg_color(bullet3, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_t * label3 = lv_label_create(cont);
    lv_label_set_text(label3, text3);

    if(disp_size == DISP_MEDIUM) {
        static lv_coord_t grid_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_FR(8), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

        lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);
        lv_obj_set_grid_cell(title_label, LV_GRID_ALIGN_START, 0, 4, LV_GRID_ALIGN_START, 0, 1);
        lv_obj_set_grid_cell(meter, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 1, 3);
        lv_obj_set_grid_cell(bullet1, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_CENTER, 2, 1);
        lv_obj_set_grid_cell(bullet2, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(bullet3, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_CENTER, 4, 1);
        lv_obj_set_grid_cell(label1, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_CENTER, 2, 1);
        lv_obj_set_grid_cell(label2, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_CENTER, 3, 1);
        lv_obj_set_grid_cell(label3, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    }
    else {
        static lv_coord_t grid_col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static lv_coord_t grid_row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
        lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);
        lv_obj_set_grid_cell(title_label, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 0, 1);
        lv_obj_set_grid_cell(meter, LV_GRID_ALIGN_START, 0, 2, LV_GRID_ALIGN_START, 1, 1);
        lv_obj_set_grid_cell(bullet1, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(bullet2, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 3, 1);
        lv_obj_set_grid_cell(bullet3, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 4, 1);
        lv_obj_set_grid_cell(label1, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 2, 1);
        lv_obj_set_grid_cell(label2, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 3, 1);
        lv_obj_set_grid_cell(label3, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 4, 1);
    }


    return meter;

}

static lv_obj_t * create_shop_item(lv_obj_t * parent, const void * img_src, const char * name, const char * category,
                                   const char * price)
{
    static lv_coord_t grid_col_dsc[] = {LV_GRID_CONTENT, 5, LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_grid_dsc_array(cont, grid_col_dsc, grid_row_dsc);

    lv_obj_t * img = lv_img_create(cont);
    lv_img_set_src(img, img_src);
    lv_obj_set_grid_cell(img, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_START, 0, 2);

    lv_obj_t * label;
    label = lv_label_create(cont);
    lv_label_set_text(label, name);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_END, 0, 1);

    label = lv_label_create(cont);
    lv_label_set_text(label, category);
    lv_obj_add_style(label, &style_text_muted, 0);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_START, 2, 1, LV_GRID_ALIGN_START, 1, 1);

    label = lv_label_create(cont);
    lv_label_set_text(label, price);
    lv_obj_set_grid_cell(label, LV_GRID_ALIGN_END, 3, 1, LV_GRID_ALIGN_END, 0, 1);

    return cont;
}

static void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);
    if(code == LV_EVENT_FOCUSED) {
        if(lv_indev_get_type(lv_indev_get_act()) != LV_INDEV_TYPE_KEYPAD) {
            lv_keyboard_set_textarea(kb, ta);
            lv_obj_set_style_max_height(kb, LV_HOR_RES * 2 / 3, 0);
            lv_obj_update_layout(tv);   /*Be sure the sizes are recalculated*/
            lv_obj_set_height(tv, LV_VER_RES - lv_obj_get_height(kb));
            lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_scroll_to_view_recursive(ta, LV_ANIM_OFF);
        }
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_set_height(tv, LV_VER_RES);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_indev_reset(NULL, ta);

    }
    else if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_set_height(tv, LV_VER_RES);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(ta, LV_STATE_FOCUSED);
        lv_indev_reset(NULL, ta);   /*To forget the last clicked object to make it focusable again*/
    }
}

static void birthday_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);

    if(code == LV_EVENT_FOCUSED) {
        if(lv_indev_get_type(lv_indev_get_act()) == LV_INDEV_TYPE_POINTER) {
            if(calendar == NULL) {
                lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
                calendar = lv_calendar_create(lv_layer_top());
                lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_50, 0);
                lv_obj_set_style_bg_color(lv_layer_top(), lv_palette_main(LV_PALETTE_GREY), 0);
                if(disp_size == DISP_SMALL) lv_obj_set_size(calendar, 180, 200);
                else if(disp_size == DISP_MEDIUM) lv_obj_set_size(calendar, 200, 220);
                else  lv_obj_set_size(calendar, 300, 330);
                lv_calendar_set_showed_date(calendar, 1990, 01);
                lv_obj_align(calendar, LV_ALIGN_CENTER, 0, 30);
                lv_obj_add_event_cb(calendar, calendar_event_cb, LV_EVENT_ALL, ta);

                lv_calendar_header_dropdown_create(calendar);
            }
        }
    }
}

static void calendar_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_user_data(e);
    lv_obj_t * obj = lv_event_get_current_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        lv_calendar_date_t d;
        lv_calendar_get_pressed_date(obj, &d);
        char buf[32];
        lv_snprintf(buf, sizeof(buf), "%02d.%02d.%d", d.day, d.month, d.year);
        lv_textarea_set_text(ta, buf);

        lv_obj_del(calendar);
        calendar = NULL;
        lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_opa(lv_layer_top(), LV_OPA_TRANSP, 0);
    }
}

static void slider_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
        lv_coord_t * s = lv_event_get_param(e);
        *s = LV_MAX(*s, 60);
    }
    else if(code == LV_EVENT_DRAW_PART_END) {
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
        if(dsc->part == LV_PART_KNOB && lv_obj_has_state(obj, LV_STATE_PRESSED)) {
            char buf[8];
            lv_snprintf(buf, sizeof(buf), "%"LV_PRId32, lv_slider_get_value(obj));

            lv_point_t text_size;
            lv_txt_get_size(&text_size, buf, font_normal, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

            lv_area_t txt_area;
            txt_area.x1 = dsc->draw_area->x1 + lv_area_get_width(dsc->draw_area) / 2 - text_size.x / 2;
            txt_area.x2 = txt_area.x1 + text_size.x;
            txt_area.y2 = dsc->draw_area->y1 - 10;
            txt_area.y1 = txt_area.y2 - text_size.y;

            lv_area_t bg_area;
            bg_area.x1 = txt_area.x1 - LV_DPX(8);
            bg_area.x2 = txt_area.x2 + LV_DPX(8);
            bg_area.y1 = txt_area.y1 - LV_DPX(8);
            bg_area.y2 = txt_area.y2 + LV_DPX(8);

            lv_draw_rect_dsc_t rect_dsc;
            lv_draw_rect_dsc_init(&rect_dsc);
            rect_dsc.bg_color = lv_palette_darken(LV_PALETTE_GREY, 3);
            rect_dsc.radius = LV_DPX(5);
            lv_draw_rect(dsc->draw_ctx, &rect_dsc, &bg_area);

            lv_draw_label_dsc_t label_dsc;
            lv_draw_label_dsc_init(&label_dsc);
            label_dsc.color = lv_color_white();
            label_dsc.font = font_normal;
            lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
        }
    }
}

static void chart_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED) {
        lv_obj_invalidate(obj); /*To make the value boxes visible*/
    }
    else if(code == LV_EVENT_DRAW_PART_BEGIN) {
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
        /*Set the markers' text*/
        if(dsc->part == LV_PART_TICKS && dsc->id == LV_CHART_AXIS_PRIMARY_X) {
            if(lv_chart_get_type(obj) == LV_CHART_TYPE_BAR) {
                const char * month[] = {"I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII"};
                lv_snprintf(dsc->text, dsc->text_length, "%s", month[dsc->value]);
            }
            else {
                const char * month[] = {"Jan", "Febr", "March", "Apr", "May", "Jun", "July", "Aug", "Sept", "Oct", "Nov", "Dec"};
                lv_snprintf(dsc->text, dsc->text_length, "%s", month[dsc->value]);
            }
        }

        /*Add the faded area before the lines are drawn */
        else if(dsc->part == LV_PART_ITEMS) {
#if LV_DRAW_COMPLEX
            /*Add  a line mask that keeps the area below the line*/
            if(dsc->p1 && dsc->p2) {
                lv_draw_mask_line_param_t line_mask_param;
                lv_draw_mask_line_points_init(&line_mask_param, dsc->p1->x, dsc->p1->y, dsc->p2->x, dsc->p2->y,
                                              LV_DRAW_MASK_LINE_SIDE_BOTTOM);
                int16_t line_mask_id = lv_draw_mask_add(&line_mask_param, NULL);

                /*Add a fade effect: transparent bottom covering top*/
                lv_coord_t h = lv_obj_get_height(obj);
                lv_draw_mask_fade_param_t fade_mask_param;
                lv_draw_mask_fade_init(&fade_mask_param, &obj->coords, LV_OPA_COVER, obj->coords.y1 + h / 8, LV_OPA_TRANSP,
                                       obj->coords.y2);
                int16_t fade_mask_id = lv_draw_mask_add(&fade_mask_param, NULL);

                /*Draw a rectangle that will be affected by the mask*/
                lv_draw_rect_dsc_t draw_rect_dsc;
                lv_draw_rect_dsc_init(&draw_rect_dsc);
                draw_rect_dsc.bg_opa = LV_OPA_50;
                draw_rect_dsc.bg_color = dsc->line_dsc->color;

                lv_area_t obj_clip_area;
                _lv_area_intersect(&obj_clip_area, dsc->draw_ctx->clip_area, &obj->coords);
                const lv_area_t * clip_area_ori = dsc->draw_ctx->clip_area;
                dsc->draw_ctx->clip_area = &obj_clip_area;
                lv_area_t a;
                a.x1 = dsc->p1->x;
                a.x2 = dsc->p2->x - 1;
                a.y1 = LV_MIN(dsc->p1->y, dsc->p2->y);
                a.y2 = obj->coords.y2;
                lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);
                dsc->draw_ctx->clip_area = clip_area_ori;
                /*Remove the masks*/
                lv_draw_mask_remove_id(line_mask_id);
                lv_draw_mask_remove_id(fade_mask_id);
            }
#endif


            const lv_chart_series_t * ser = dsc->sub_part_ptr;

            if(lv_chart_get_pressed_point(obj) == dsc->id) {
                if(lv_chart_get_type(obj) == LV_CHART_TYPE_LINE) {
                    dsc->rect_dsc->outline_color = lv_color_white();
                    dsc->rect_dsc->outline_width = 2;
                }
                else {
                    dsc->rect_dsc->shadow_color = ser->color;
                    dsc->rect_dsc->shadow_width = 15;
                    dsc->rect_dsc->shadow_spread = 0;
                }

                char buf[8];
                lv_snprintf(buf, sizeof(buf), "%"LV_PRIu32, dsc->value);

                lv_point_t text_size;
                lv_txt_get_size(&text_size, buf, font_normal, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

                lv_area_t txt_area;
                if(lv_chart_get_type(obj) == LV_CHART_TYPE_BAR) {
                    txt_area.y2 = dsc->draw_area->y1 - LV_DPX(15);
                    txt_area.y1 = txt_area.y2 - text_size.y;
                    if(ser == lv_chart_get_series_next(obj, NULL)) {
                        txt_area.x1 = dsc->draw_area->x1 + lv_area_get_width(dsc->draw_area) / 2;
                        txt_area.x2 = txt_area.x1 + text_size.x;
                    }
                    else {
                        txt_area.x2 = dsc->draw_area->x1 + lv_area_get_width(dsc->draw_area) / 2;
                        txt_area.x1 = txt_area.x2 - text_size.x;
                    }
                }
                else {
                    txt_area.x1 = dsc->draw_area->x1 + lv_area_get_width(dsc->draw_area) / 2 - text_size.x / 2;
                    txt_area.x2 = txt_area.x1 + text_size.x;
                    txt_area.y2 = dsc->draw_area->y1 - LV_DPX(15);
                    txt_area.y1 = txt_area.y2 - text_size.y;
                }

                lv_area_t bg_area;
                bg_area.x1 = txt_area.x1 - LV_DPX(8);
                bg_area.x2 = txt_area.x2 + LV_DPX(8);
                bg_area.y1 = txt_area.y1 - LV_DPX(8);
                bg_area.y2 = txt_area.y2 + LV_DPX(8);

                lv_draw_rect_dsc_t rect_dsc;
                lv_draw_rect_dsc_init(&rect_dsc);
                rect_dsc.bg_color = ser->color;
                rect_dsc.radius = LV_DPX(5);
                lv_draw_rect(dsc->draw_ctx, &rect_dsc, &bg_area);

                lv_draw_label_dsc_t label_dsc;
                lv_draw_label_dsc_init(&label_dsc);
                label_dsc.color = lv_color_white();
                label_dsc.font = font_normal;
                lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area,  buf, NULL);
            }
            else {
                dsc->rect_dsc->outline_width = 0;
                dsc->rect_dsc->shadow_width = 0;
            }
        }
    }
}


static void shop_chart_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_DRAW_PART_BEGIN) {
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
        /*Set the markers' text*/
        if(dsc->part == LV_PART_TICKS && dsc->id == LV_CHART_AXIS_PRIMARY_X) {
            const char * month[] = {"Jan", "Febr", "March", "Apr", "May", "Jun", "July", "Aug", "Sept", "Oct", "Nov", "Dec"};
            lv_snprintf(dsc->text, dsc->text_length, "%s", month[dsc->value]);
        }
        if(dsc->part == LV_PART_ITEMS) {
            dsc->rect_dsc->bg_opa = LV_OPA_TRANSP; /*We will draw it later*/
        }
    }
    if(code == LV_EVENT_DRAW_PART_END) {
        lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
        /*Add the faded area before the lines are drawn */
        if(dsc->part == LV_PART_ITEMS) {
            static const uint32_t devices[10] = {32, 43, 21, 56, 29, 36, 19, 25, 62, 35};
            static const uint32_t clothes[10] = {12, 19, 23, 31, 27, 32, 32, 11, 21, 32};
            static const uint32_t services[10] = {56, 38, 56, 13, 44, 32, 49, 64, 17, 33};

            lv_draw_rect_dsc_t draw_rect_dsc;
            lv_draw_rect_dsc_init(&draw_rect_dsc);

            lv_coord_t h = lv_area_get_height(dsc->draw_area);

            lv_area_t a;
            a.x1 = dsc->draw_area->x1;
            a.x2 = dsc->draw_area->x2;

            a.y1 = dsc->draw_area->y1;
            a.y2 = a.y1 + 4 + (devices[dsc->id] * h) / 100; /*+4 to overlap the radius*/
            draw_rect_dsc.bg_color = lv_palette_main(LV_PALETTE_RED);
            draw_rect_dsc.radius = 4;
            lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);

            a.y1 = a.y2 - 4;                                    /*-4 to overlap the radius*/
            a.y2 = a.y1 + (clothes[dsc->id] * h) / 100;
            draw_rect_dsc.bg_color = lv_palette_main(LV_PALETTE_BLUE);
            draw_rect_dsc.radius = 0;
            lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);

            a.y1 = a.y2;
            a.y2 = a.y1 + (services[dsc->id] * h) / 100;
            draw_rect_dsc.bg_color = lv_palette_main(LV_PALETTE_GREEN);
            lv_draw_rect(dsc->draw_ctx, &draw_rect_dsc, &a);
        }
    }
}


static void meter1_indic1_anim_cb(void * var, int32_t v)
{
    lv_meter_set_indicator_end_value(meter1, var, v);

    lv_obj_t * card = lv_obj_get_parent(meter1);
    lv_obj_t * label = lv_obj_get_child(card, -5);
    lv_label_set_text_fmt(label, "Revenue: %"LV_PRId32" %%", v);
}

static void meter1_indic2_anim_cb(void * var, int32_t v)
{
    lv_meter_set_indicator_end_value(meter1, var, v);

    lv_obj_t * card = lv_obj_get_parent(meter1);
    lv_obj_t * label = lv_obj_get_child(card, -3);
    lv_label_set_text_fmt(label, "Sales: %"LV_PRId32" %%", v);

}

static void meter1_indic3_anim_cb(void * var, int32_t v)
{
    lv_meter_set_indicator_end_value(meter1, var, v);

    lv_obj_t * card = lv_obj_get_parent(meter1);
    lv_obj_t * label = lv_obj_get_child(card, -1);
    lv_label_set_text_fmt(label, "Costs: %"LV_PRId32" %%", v);
}

static void meter2_timer_cb(lv_timer_t * timer)
{
    lv_meter_indicator_t ** indics = timer->user_data;

    static bool down1 = false;
    static bool down2 = false;
    static bool down3 = false;


    if(down1) {
        session_desktop -= 137;
        if(session_desktop < 1400) down1 = false;
    }
    else {
        session_desktop += 116;
        if(session_desktop > 4500) down1 = true;
    }

    if(down2) {
        session_tablet -= 3;
        if(session_tablet < 1400) down2 = false;
    }
    else {
        session_tablet += 9;
        if(session_tablet > 4500) down2 = true;
    }

    if(down3) {
        session_mobile -= 57;
        if(session_mobile < 1400) down3 = false;
    }
    else {
        session_mobile += 76;
        if(session_mobile > 4500) down3 = true;
    }

    uint32_t all = session_desktop + session_tablet + session_mobile;
    uint32_t pct1 = (session_desktop * 97) / all;
    uint32_t pct2 = (session_tablet * 97) / all;

    lv_meter_set_indicator_start_value(meter2, indics[0], 0);
    lv_meter_set_indicator_end_value(meter2, indics[0], pct1);

    lv_meter_set_indicator_start_value(meter2, indics[1], pct1 + 1);
    lv_meter_set_indicator_end_value(meter2, indics[1], pct1 + 1 + pct2);

    lv_meter_set_indicator_start_value(meter2, indics[2], pct1 + 1 + pct2 + 1);
    lv_meter_set_indicator_end_value(meter2, indics[2], 99);

    lv_obj_t * card = lv_obj_get_parent(meter2);
    lv_obj_t * label;

    label = lv_obj_get_child(card, -5);
    lv_label_set_text_fmt(label, "Desktop: %"LV_PRIu32, session_desktop);

    label = lv_obj_get_child(card, -3);
    lv_label_set_text_fmt(label, "Tablet: %"LV_PRIu32, session_tablet);

    label = lv_obj_get_child(card, -1);
    lv_label_set_text_fmt(label, "Mobile: %"LV_PRIu32, session_mobile);
}

static void meter3_anim_cb(void * var, int32_t v)
{
    lv_meter_set_indicator_value(meter3, var, v);

    lv_obj_t * label = lv_obj_get_child(meter3, 0);
    lv_label_set_text_fmt(label, "%"LV_PRId32, v);
}

#endif
