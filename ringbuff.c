#include <stdint.h>


// 环形缓冲区大小
#define RB_SIZE 128

// 环形缓冲区结构体
typedef struct 
{
    uint8_t buf[RB_SIZE];       // 数据
    volatile uint16_t head;     // 头指针
    volatile uint16_t tail;     // 尾指针
} ringbuf_t;

// 相关变量
static uint8_t rx_byte;

static ringbuf_t uart1_rx_rb;
static ringbuf_t uart1_tx_rb;

static uint8_t tx_byte;
static volatile uint8_t uart1_tx_busy = 0;

static volatile uint32_t uart1_rx_overflow_cnt = 0;
static volatile uint32_t uart1_tx_overflow_cnt = 0;
// static volatile uint32_t uart1_error_cnt = 0;

// 相关函数声明
static uint16_t rb_next(uint16_t index);
static uint8_t rb_push(ringbuf_t *rb, uint8_t data);
static uint8_t rb_pop(ringbuf_t *rb, uint8_t *data);

static void uart1_start_tx(void);
static uint8_t uart1_send_byte(uint8_t data);
static uint16_t uart1_send_buffer(const uint8_t *data, uint16_t len);

// 加入以下代码,支持printf函数,而不需要选择use MicroLIB
int __io_putchar(int ch)
{
    uart1_send_byte((uint8_t)ch);
    // HAL_UART_Transmit(&huart1, &(uint8_t)ch, 1, HAL_MAX_DELAY);
    return ch;
}

static uint16_t rb_next(uint16_t index)
{
    return (uint16_t)(index + 1) % RB_SIZE;
}

// 往缓冲区里写入一个字节
static uint8_t rb_push(ringbuf_t *rb, uint8_t data)
{
    uint16_t next = rb_next(rb->head);

    // 判断缓冲区是否满了
    if(next == rb->tail)
    {
        return 0;   // 缓冲区满
    }

    rb->buf[rb->head] = data;
    rb->head = next;
    return 1;   // 成功
}

// 从缓冲区里取出一个字节
static uint8_t rb_pop(ringbuf_t *rb, uint8_t *data)
{
    // 判断缓冲区是否空
    if(rb->head == rb->tail)
    {
        return 0;   // 缓冲区空
    }

    *data = rb->buf[rb->tail];
    rb->tail = rb_next(rb->tail);
    return 1;   // 成功
}

static void uart1_start_tx(void)
{
    if(uart1_tx_busy)
    {
        return;
    }
    // 从发送缓冲区取出一个字节,如果有数据就开始发送
    if(rb_pop(&uart1_tx_rb, &tx_byte))
    {
        uart1_tx_busy = 1;
        HAL_UART_Transmit_IT(&huartx, &tx_byte, 1);     // huartx改成自己用的
    }
}

// 发送单字节
static uint8_t uart1_send_byte(uint8_t data)
{
    uint8_t ok;

    __disable_irq();    // 关闭中断,防止与中断处理函数冲突
    ok = rb_push(&uart1_rx_rb, data);
    if(ok)
    {
        uart1_start_tx();
    }
    else
    {
        uart1_tx_overflow_cnt++;
    }
    __enable_irq();   // 开启中断

    return ok;
}

// 发送数组
static uint16_t uart1_send_buffer(const uint8_t *data, uint16_t len)
{
    uint16_t i;

    for(i = 0; i < len; i++)
    {
        if(!uart1_send_byte(data[i]))
        {
            break;
        }
    }

    return i;  // 返回实际发送的字节数
}

void main()
{
    ...
    // 开启中断回调
    HAL_UART_Receive_IT(&huartx, &rx_byte, 1);          // huartx改成自己用的

    // 测试缓冲区和printf函数
    printf("USART1 ready\r\n");
    uart1_send_buffer((const uint8_t *)"world\r\n", 7);

    while(1)
    {
        uint8_t ch;
        // 从接收缓冲区取出数据并回显
        while(rb_pop(&uart1_rx_rb, &ch))
        {
            uart1_send_byte(ch);
        }
    }
}

// 两个中断回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USARTx)                  // huartx改成自己用的
    {
      if (!rb_push(&uart1_rx_rb, rx_byte))
      {
        uart1_rx_overflow_cnt++;
      }

      HAL_UART_Receive_IT(&huartx, &rx_byte, 1); // 重新使能下一字节中断
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USARTx)                    // huartx改成自己用的
  {
    if (rb_pop(&uart1_tx_rb, &tx_byte))
    {
      HAL_UART_Transmit_IT(&huartx, &tx_byte, 1);
    }
    else
    {
      uart1_tx_busy = 0;
    }
  }
}
