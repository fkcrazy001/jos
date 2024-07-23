#include <jp/interrupt.h>
#include <jp/io.h>
#include <jp/types.h>
#include <jp/debug.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_CTRL_PORT 0x64

// 不可见的 scancode
#define INV 0

// print screen
#define CODE_PRINT_SCREEN_DOWN 0xb7

#define KEYBOARD_CMD_LED 0xed
#define KEYBOARD_CMD_ACK 0xfa
#define KEYBOARD_CMD_REPEAT 0xfe

typedef enum
{
    KEY_NONE,
    KEY_ESC,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_MINUS,
    KEY_EQUAL,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_Q,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_BRACKET_L,
    KEY_BRACKET_R,
    KEY_ENTER,
    KEY_CTRL_L,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_SEMICOLON,
    KEY_QUOTE,
    KEY_BACKQUOTE,
    KEY_SHIFT_L,
    KEY_BACKSLASH,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_COMMA,
    KEY_POINT,
    KEY_SLASH,
    KEY_SHIFT_R,
    KEY_STAR,
    KEY_ALT_L,
    KEY_SPACE,
    KEY_CAPSLOCK,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_NUMLOCK,
    KEY_SCRLOCK,
    KEY_PAD_7,
    KEY_PAD_8,
    KEY_PAD_9,
    KEY_PAD_MINUS,
    KEY_PAD_4,
    KEY_PAD_5,
    KEY_PAD_6,
    KEY_PAD_PLUS,
    KEY_PAD_1,
    KEY_PAD_2,
    KEY_PAD_3,
    KEY_PAD_0,
    KEY_PAD_POINT,
    KEY_54,
    KEY_55,
    KEY_56,
    KEY_F11,
    KEY_F12,
    KEY_59,
    KEY_WIN_L,
    KEY_WIN_R,
    KEY_CLIPBOARD,
    KEY_5D,
    KEY_5E,

    // 以下为自定义按键，为和 keymap 索引匹配
    KEY_PRINT_SCREEN,
} KEY;

static char keymap[][4] = {
    /* 扫描码 未与 shift 组合  与 shift 组合 以及相关状态 */
    /* ---------------------------------- */
    /* 字符  shift按下的字符  字符(L)是否被按下 字符(R)是否被按下*/
    /* 同一个字符会有多个扫描码，比如ctrl左ctrl和右边ctrl，区别是右ctrl有ext码，但扫描码一致*/
    /* 0x00 */ {INV, INV, false, false},   // NULL
    /* 0x01 */ {0x1b, 0x1b, false, false}, // ESC
    /* 0x02 */ {'1', '!', false, false},
    /* 0x03 */ {'2', '@', false, false},
    /* 0x04 */ {'3', '#', false, false},
    /* 0x05 */ {'4', '$', false, false},
    /* 0x06 */ {'5', '%', false, false},
    /* 0x07 */ {'6', '^', false, false},
    /* 0x08 */ {'7', '&', false, false},
    /* 0x09 */ {'8', '*', false, false},
    /* 0x0A */ {'9', '(', false, false},
    /* 0x0B */ {'0', ')', false, false},
    /* 0x0C */ {'-', '_', false, false},
    /* 0x0D */ {'=', '+', false, false},
    /* 0x0E */ {'\b', '\b', false, false}, // BACKSPACE
    /* 0x0F */ {'\t', '\t', false, false}, // TAB
    /* 0x10 */ {'q', 'Q', false, false},
    /* 0x11 */ {'w', 'W', false, false},
    /* 0x12 */ {'e', 'E', false, false},
    /* 0x13 */ {'r', 'R', false, false},
    /* 0x14 */ {'t', 'T', false, false},
    /* 0x15 */ {'y', 'Y', false, false},
    /* 0x16 */ {'u', 'U', false, false},
    /* 0x17 */ {'i', 'I', false, false},
    /* 0x18 */ {'o', 'O', false, false},
    /* 0x19 */ {'p', 'P', false, false},
    /* 0x1A */ {'[', '{', false, false},
    /* 0x1B */ {']', '}', false, false},
    /* 0x1C */ {'\n', '\n', false, false}, // ENTER
    /* 0x1D */ {INV, INV, false, false},   // CTRL_L
    /* 0x1E */ {'a', 'A', false, false},
    /* 0x1F */ {'s', 'S', false, false},
    /* 0x20 */ {'d', 'D', false, false},
    /* 0x21 */ {'f', 'F', false, false},
    /* 0x22 */ {'g', 'G', false, false},
    /* 0x23 */ {'h', 'H', false, false},
    /* 0x24 */ {'j', 'J', false, false},
    /* 0x25 */ {'k', 'K', false, false},
    /* 0x26 */ {'l', 'L', false, false},
    /* 0x27 */ {';', ':', false, false},
    /* 0x28 */ {'\'', '"', false, false},
    /* 0x29 */ {'`', '~', false, false},
    /* 0x2A */ {INV, INV, false, false}, // SHIFT_L
    /* 0x2B */ {'\\', '|', false, false},
    /* 0x2C */ {'z', 'Z', false, false},
    /* 0x2D */ {'x', 'X', false, false},
    /* 0x2E */ {'c', 'C', false, false},
    /* 0x2F */ {'v', 'V', false, false},
    /* 0x30 */ {'b', 'B', false, false},
    /* 0x31 */ {'n', 'N', false, false},
    /* 0x32 */ {'m', 'M', false, false},
    /* 0x33 */ {',', '<', false, false},
    /* 0x34 */ {'.', '>', false, false},
    /* 0x35 */ {'/', '?', false, false},
    /* 0x36 */ {INV, INV, false, false},  // SHIFT_R
    /* 0x37 */ {'*', '*', false, false},  // PAD *
    /* 0x38 */ {INV, INV, false, false},  // ALT_L
    /* 0x39 */ {' ', ' ', false, false},  // SPACE
    /* 0x3A */ {INV, INV, false, false},  // CAPSLOCK
    /* 0x3B */ {INV, INV, false, false},  // F1
    /* 0x3C */ {INV, INV, false, false},  // F2
    /* 0x3D */ {INV, INV, false, false},  // F3
    /* 0x3E */ {INV, INV, false, false},  // F4
    /* 0x3F */ {INV, INV, false, false},  // F5
    /* 0x40 */ {INV, INV, false, false},  // F6
    /* 0x41 */ {INV, INV, false, false},  // F7
    /* 0x42 */ {INV, INV, false, false},  // F8
    /* 0x43 */ {INV, INV, false, false},  // F9
    /* 0x44 */ {INV, INV, false, false},  // F10
    /* 0x45 */ {INV, INV, false, false},  // NUMLOCK
    /* 0x46 */ {INV, INV, false, false},  // SCRLOCK
    /* 0x47 */ {'7', INV, false, false},  // pad 7 - Home
    /* 0x48 */ {'8', INV, false, false},  // pad 8 - Up
    /* 0x49 */ {'9', INV, false, false},  // pad 9 - PageUp
    /* 0x4A */ {'-', '-', false, false},  // pad -
    /* 0x4B */ {'4', INV, false, false},  // pad 4 - Left
    /* 0x4C */ {'5', INV, false, false},  // pad 5
    /* 0x4D */ {'6', INV, false, false},  // pad 6 - Right
    /* 0x4E */ {'+', '+', false, false},  // pad +
    /* 0x4F */ {'1', INV, false, false},  // pad 1 - End
    /* 0x50 */ {'2', INV, false, false},  // pad 2 - Down
    /* 0x51 */ {'3', INV, false, false},  // pad 3 - PageDown
    /* 0x52 */ {'0', INV, false, false},  // pad 0 - Insert
    /* 0x53 */ {'.', 0x7F, false, false}, // pad . - Delete
    /* 0x54 */ {INV, INV, false, false},  //
    /* 0x55 */ {INV, INV, false, false},  //
    /* 0x56 */ {INV, INV, false, false},  //
    /* 0x57 */ {INV, INV, false, false},  // F11
    /* 0x58 */ {INV, INV, false, false},  // F12
    /* 0x59 */ {INV, INV, false, false},  //
    /* 0x5A */ {INV, INV, false, false},  //
    /* 0x5B */ {INV, INV, false, false},  // Left Windows
    /* 0x5C */ {INV, INV, false, false},  // Right Windows
    /* 0x5D */ {INV, INV, false, false},  // Clipboard
    /* 0x5E */ {INV, INV, false, false},  //

    // Print Screen 是强制定义 本身是 0xB7
    /* 0x5F */ {INV, INV, false, false}, // PrintScreen
};

static bool capslock_state; // 大写锁定
static bool scrlock_state;  // 滚动锁定
static bool numlock_state;  // 数字锁定
static bool extcode_state;  // 扩展码状态

// CTRL 键状态
#define ctrl_state (keymap[KEY_CTRL_L][2] || keymap[KEY_CTRL_L][3])

// ALT 键状态
#define alt_state (keymap[KEY_ALT_L][2] || keymap[KEY_ALT_L][3])

// SHIFT 键状态
#define shift_state (keymap[KEY_SHIFT_L][2] || keymap[KEY_SHIFT_R][2])

#define EXT_CODE 0xe0


// 0. 输出缓冲区状态：1 表示输出缓冲区满
// 1. 输入缓冲区状态：1 表示输入缓冲区满
// 2. 系统标志位：加电时置为 0，自检通过时置为 1
// 3. 命令/数据位：1 表示输入缓冲区的内容是命令，0 表示输入缓冲区的内容是数据
// 4. 1 表示键盘启用，0 表示键盘禁用
// 5. 1 表示发送超时 （存疑）
// 6. 1 表示接受超时 （存疑）
// 7. 奇偶校验出错
static void keyboard_wait_tx(void)
{
    u8 state;
    do {
        state = inb(KEYBOARD_CTRL_PORT);
    } while(state & 0x02);
}

static bool keyboard_wait_ack_ok(void)
{
    u8 state;
    do {
        state = inb(KEYBOARD_DATA_PORT);
    } while(state != KEYBOARD_CMD_ACK);
    return state == KEYBOARD_CMD_ACK;
}

static void set_leds(void)
{
    u8 leds = ((capslock_state<<2)|(numlock_state<<1)|scrlock_state);
    u8 retry = 3;
again:
    if (retry-- <= 0) {
        DEBUGK("set led state failed after 3 retrys, give up...\n");
        return;
    }
    keyboard_wait_tx();
    outb(KEYBOARD_DATA_PORT, KEYBOARD_CMD_LED);
    if(!keyboard_wait_ack_ok())
        goto again;

    keyboard_wait_tx();
    outb(KEYBOARD_DATA_PORT, leds);
    // while (keyboard_wait_ack_ok())
    // {
    //     DEBUGK("recv ack ok when init\n");
    // }
    if(!keyboard_wait_ack_ok())
        goto again;
}



static void keyboard_handler(void)
{
    u16 scancode = inb(KEYBOARD_DATA_PORT);
    u8 ext = 2;
    // DEBUGK("keyboard input 0x%x\n", scancode);
    if (scancode == EXT_CODE) {
        extcode_state = true;
        return;
    }
    if (extcode_state) {
        ext = 3;
        scancode |= ((u16)(EXT_CODE << 8) & 0xff00);
        extcode_state = false;
    }

    // 通码
    u16 makecode = (scancode) & 0x7f;
    if (makecode == CODE_PRINT_SCREEN_DOWN) {
        makecode = KEY_PRINT_SCREEN;
    }
    if (makecode > KEY_PRINT_SCREEN) {
        DEBUGK("invalid make code 0x%x, origin 0x%x\n", makecode, scancode);
        return;
    }
    bool breakcode = ((scancode & 0x80)!=0);
    if (breakcode) {
        keymap[makecode][ext] = false;
        return;
    }
    keymap[makecode][ext] = true;

    bool led = false;
    if (makecode == KEY_NUMLOCK) {
        numlock_state = !numlock_state;
        led = true;
    } else if (makecode == KEY_CAPSLOCK) {
        capslock_state = !capslock_state;
        led = true;
    } else if (makecode == KEY_SCRLOCK) {
        scrlock_state = !scrlock_state;
        led = true;
    }
    
    if (led)
        set_leds();
    bool shift = shift_state;
    char ch = keymap[makecode][0];
    if(ch >= 'a' && ch <= 'z')
        shift ^= capslock_state;
    if (ext==3 && (makecode != KEY_SLASH)) {
        // DEBUGK("extern code, scancode = 0x%x\n", scancode);
        // ext mode, we always use 1 rather than 0, though they are the same
        // in most cases.
        ch = keymap[makecode][1];
    } else {
        ch = keymap[makecode][shift];
    }
    if (ch == INV)
        return;
    DEBUGK("%c pressed\n", ch);
}

void keyboard_init(void)
{
    capslock_state = false;
    scrlock_state = false;
    numlock_state = false;
    extcode_state = false;
    set_leds();
    pic_set_interrupt_handler(IRQ_KEYBOARD, keyboard_handler);
    pic_set_interrupt(IRQ_KEYBOARD, true);
}