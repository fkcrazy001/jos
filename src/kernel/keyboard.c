#include <jp/interrupt.h>
#include <jp/io.h>
#include <jp/types.h>
#include <jp/debug.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_CTRL_PORT 0x64


static void keyboard_handler(void)
{
    u16 scancode = inb(KEYBOARD_DATA_PORT);
    DEBUGK("keyboard input 0x%x\n", scancode);
}

void keyboard_init(void)
{
    pic_set_interrupt_handler(IRQ_KEYBOARD, keyboard_handler);
    pic_set_interrupt(IRQ_KEYBOARD, true);
}