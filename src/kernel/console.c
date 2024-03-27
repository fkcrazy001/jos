#include <jp/console.h>
#include <jp/io.h>
#include <jp/string.h>

#define CRT_ADDR_REG 0x3D4 // CRT(6845)索引寄存器
#define CRT_DATA_REG 0x3D5 // CRT(6845)数据寄存器

#define CRT_START_ADDR_H 0xC // 显示内存起始位置 - 高位
#define CRT_START_ADDR_L 0xD // 显示内存起始位置 - 低位
#define CRT_CURSOR_H 0xE     // 光标位置 - 高位
#define CRT_CURSOR_L 0xF     // 光标位置 - 低位

#define MEM_BASE 0xB8000              // 显卡内存起始位置
#define MEM_SIZE 0x4000               // 显卡内存大小, 16k
#define MEM_END (MEM_BASE + MEM_SIZE) // 显卡内存结束位置
#define WIDTH 80                      // 屏幕文本列数
#define HEIGHT 25                     // 屏幕文本行数
#define ROW_SIZE (WIDTH * 2)          // 每行字节数
#define SCR_SIZE (ROW_SIZE * HEIGHT)  // 屏幕字节数

#define NUL 0x00
#define ENQ 0x05
#define ESC 0x1B // ESC
#define BEL 0x07 // \a
#define BS 0x08  // \b
#define HT 0x09  // \t
#define LF 0x0A  // \n
#define VT 0x0B  // \v
#define FF 0x0C  // \f
#define CR 0x0D  // \r
#define DEL 0x7F

// 当前显示器开始的内存位置， 由于显卡内存区域大于屏幕能够显示的范围，
// 所以需要选择一部分显卡的内存用于展示
static u32 screen;

// 当前光标的内存位置
static u32 pos;

// 当前光标的坐标
static u32 x,y;

// 字符的样式
static u8 attr = 7;

//空格
static u16 erase = 0x0720; // 07=attr, 0x20 = 空格的asci码

static void get_screen(void)
{
    outb(CRT_ADDR_REG, CRT_START_ADDR_H);
    screen = inb(CRT_DATA_REG) << 8;
    outb(CRT_ADDR_REG, CRT_START_ADDR_L);
    screen |= inb(CRT_DATA_REG);
    screen <<= 1;
    screen += MEM_BASE;
}

static void set_screen(void)
{
    outb(CRT_ADDR_REG, CRT_START_ADDR_H);
    outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 9) & 0xff);
    outb(CRT_ADDR_REG, CRT_START_ADDR_L);
    outb(CRT_DATA_REG, ((screen - MEM_BASE) >> 1) & 0xff);
}

static void get_cursor(void)
{
    outb(CRT_ADDR_REG, CRT_CURSOR_H);
    pos = inb(CRT_DATA_REG) << 8;
    outb(CRT_ADDR_REG, CRT_CURSOR_L);
    pos |= inb(CRT_DATA_REG);
    get_screen();
    pos <<= 1;
    pos+=MEM_BASE;
    u32 delta = (pos - screen) >> 1;
    x = delta / WIDTH;
    y = delta / WIDTH;
}

static void set_cursor(void)
{
    outb(CRT_ADDR_REG, CRT_CURSOR_H);
    outb(CRT_DATA_REG, ((pos - MEM_BASE) >> 9) & 0xff);
    outb(CRT_ADDR_REG, CRT_CURSOR_L);
    outb(CRT_DATA_REG, ((pos - MEM_BASE) >> 1) & 0xff);
}

void console_clear(void)
{
    screen = MEM_BASE;
    pos = MEM_BASE;
    x=y=0;
    set_cursor();
    set_screen();
    u16 *ptr = (u16*)screen;
    while(ptr < (u16*)MEM_END) {
        *ptr++ = erase;
    }
    return;
}

static void cmd_cr(void) 
{  // \r 
    pos -= (x << 1);
    x = 0;
    //
}

static void scroll_up(void)
{
    if (screen + SCR_SIZE + ROW_SIZE >= MEM_END) {
        memcpy((void*)MEM_BASE, (void*)screen, SCR_SIZE);
        pos -= (screen - MEM_BASE);
        screen = MEM_BASE;
    }
    {
        u16 *ptr = (uint16*)(screen + SCR_SIZE);
        for (size_t i = 0; i < WIDTH; ++i) {
            *ptr++ = erase;
        }
        screen += ROW_SIZE;
        pos += ROW_SIZE;
    }
    set_screen();
    return;
}

static void cmd_lf(void) 
{  // \n
    if (y + 1 < HEIGHT){
        y++;
        pos += ROW_SIZE;
        return;
    }
    //
    scroll_up();
}


static void cmd_bs(void) 
{  // backspace
    if (x) {
        x--;
        pos -= 2;
        *(u16*)pos = erase;
    }
    //
}

static void cmd_del(void) 
{  // delete
    *(u16*)pos = erase;
    //
}

void console_write(char* buf, u32 count)
{
    char ch;
    while (count--)
    {
        ch = *buf++;
        switch (ch)
        {
        case NUL:
            /* code */
            break;
        case ENQ:
            break;
        case ESC: // ESC
            break;
        case BEL: // \a
            /* @todo */
            break;
        case BS: // \b
            cmd_bs();
            break;
        case HT: // 
            break;
        case LF: // 
            cmd_lf();
            cmd_cr();
            break;
        case VT: // 
            break;
        case FF:
            cmd_lf();
            break;
        case CR:
            cmd_cr();
            break;
        case DEL:
            cmd_del();
            break;
        default:
            if (x >= WIDTH) {
                x -= WIDTH;
                pos -= ROW_SIZE;
                cmd_lf();
            }
            *((char*)pos) = ch;
            pos++;
            *((char*)pos) = attr;
            pos++;
            x++;
            break;
        }
        set_cursor();
    }
    
    return;
}

void console_init(void)
{
    console_clear();
    //screen = 80 *2 + MEM_BASE;
}