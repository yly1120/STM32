#include <stdint.h>
#include <stdio.h>
//这些代码片段是为了支持在使用ARM编译器时，
// 能够使用printf函数而不需要选择使用MicroLIB库。
// 它定义了一个__FILE结构体和一个__stdout对象，
// 并实现了_sys_exit和fputc函数，
// 以便将输出重定向到UART1接口。
// 最后，还定义了一个__io_putchar函数，
// 用于将字符发送到UART1接口。
// 配合之前的ringbuff使用
//
// 加入以下代码,支持printf函数,而不需要选择use MicroLIB
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
/* 告知连接器不从C库链接使用半主机的函数 */
#if defined(__CC_ARM) && !defined(__ARMCC_VERSION) || (__ARMCC_VERSION < 6000000)
#pragma import(__use_no_semihosting)
#else
__asm(".global __use_no_semihosting\n\t");
#endif

struct __FILE
{
    int handle;
};
FILE __stdout;

void _sys_exit(int x)
{
    x = x;
}

int fputc(int ch, FILE *f)
{
    uart1_send_byte((uint8_t)ch);
    return ch;
}
#endif

int __io_putchar(int ch)
{
    uart1_send_byte((uint8_t)ch);
    return ch;
}
