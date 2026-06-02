#include "colorlut.h"
#include "lcd_io.h"
#include "platform.h"
#include "lcdfont.h"
#include <stdio.h>

#if (PLATFORM_DEVICE == DEVICE_AT32) && (PLATFORM_DRIVER == DRIVER_SPL)
#include "at32f421_wk_config.h"
#include "at32f421_gpio.h"
#include "wk_system.h"
#include "at32f421_spi.h"
#elif (PLATFORM_DEVICE == DEVICE_STM32) && (PLATFORM_DRIVER == DRIVER_HAL)
// 预留位置
#endif

// SPI DMA缓冲区（每像素2字节）
#if COLORLUT_VERTICAL_PRIORITY == 0
// 横条优先：缓冲区大小 = 宽度
static uint16_t spi_buf[COLORLUT_AREA_WIDTH];
#else
// 竖条优先：缓冲区大小 = 高度
static uint16_t spi_buf[COLORLUT_AREA_HEIGHT];
#endif

// 调色板存储数组
static uint16_t colorlut_palette[COLORLUT_MAX_COLORS];
static size_t colorlut_count = 0;

/**
 * @brief 初始化调色板
 *
 * 将用户提供的 RGB565 颜色数组复制到内部调色板。
 * 如果数量超过最大支持值，则自动截断。
 *
 * @param colors RGB565颜色数组
 * @param count  调色板颜色数量 (最大16或256)
 */
void colorlut_init(const uint16_t *colors, size_t count)
{
    if (count > COLORLUT_MAX_COLORS)
        count = COLORLUT_MAX_COLORS;

    for (size_t i = 0; i < count; i++)
    {
        colorlut_palette[i] = colors[i];
    }

    colorlut_count = count;
}

/**
 * @brief 获取调色板中的颜色
 *
 * 根据索引返回对应的 RGB565 颜色。
 * 如果索引越界，则返回黑色 (0x0000)。
 *
 * @param index 调色板索引
 * @return uint16_t RGB565颜色
 */
uint16_t colorlut_get_color(uint8_t index)
{
    if (index >= colorlut_count)
    {
        return 0x0000; // 默认返回黑色
    }
    return colorlut_palette[index];
}

/**
 * @brief 设置调色板中的某个颜色
 *
 * 修改指定索引的颜色值。如果索引超过当前调色板大小，
 * 会自动扩展调色板大小。
 *
 * @param index 调色板索引
 * @param color RGB565颜色
 */
void colorlut_set_color(uint8_t index, uint16_t color)
{
    if (index < COLORLUT_MAX_COLORS)
    {
        colorlut_palette[index] = color;
        if (index >= colorlut_count)
        {
            colorlut_count = index + 1;
        }
    }
}

/**
 * @brief 清空索引缓冲区
 *
 * 将整个缓冲区填充为指定的颜色索引。
 * 在 4bit 模式下，每字节包含两个像素索引；
 * 在 8bit 模式下，每字节包含一个像素索引。
 *
 * @param buf   索引缓冲区指针
 * @param width 图像宽度
 * @param height图像高度
 * @param color_index 填充颜色索引
 */
void colorlut_clear_buffer(uint8_t *buf, size_t width, size_t height, uint8_t color_index)
{
#if COLORLUT_MAX_COLORS <= 16
    // 4bit模式：每字节两个像素
    uint8_t packed = (color_index & 0x0F) | ((color_index & 0x0F) << 4);
    size_t bytes = (width * height + 1) / 2; // 向上取整
    for (size_t i = 0; i < bytes; i++)
    {
        buf[i] = packed;
    }
#elif COLORLUT_MAX_COLORS <= 256
    // 8bit模式：每字节一个像素
    size_t pixels = width * height;
    for (size_t i = 0; i < pixels; i++)
    {
        buf[i] = color_index;
    }
#else
    #error "Unsupported COLORLUT_MAX_COLORS value"
#endif
}

/**
 * @brief 设置某个像素的索引值
 *
 * 根据坐标计算缓冲区位置，并写入调色板索引。
 *
 * @param buf 索引缓冲区指针
 * @param x   像素X坐标
 * @param y   像素Y坐标
 * @param color_index 调色板索引
 */
void colorlut_set_pixel(uint8_t *buf, size_t x, size_t y, uint8_t color_index)
{
#if COLORLUT_MAX_COLORS <= 16
    size_t pixel_index = y * COLORLUT_AREA_WIDTH + x; // 假设屏幕宽度宏已定义
    size_t byte_index = pixel_index / 2;
    if ((pixel_index & 1) == 0)
    {
        // 低4bit
        buf[byte_index] = (buf[byte_index] & 0xF0) | (color_index & 0x0F);
    }
    else
    {
        // 高4bit
        buf[byte_index] = (buf[byte_index] & 0x0F) | ((color_index & 0x0F) << 4);
    }
#elif COLORLUT_MAX_COLORS <= 256
    size_t pixel_index = y * COLORLUT_AREA_WIDTH + x;
    buf[pixel_index] = color_index;
#endif
}

/**
 * @brief 获取某个像素的索引值
 *
 * 根据坐标计算缓冲区位置，并返回调色板索引。
 *
 * @param buf 索引缓冲区指针
 * @param x   像素X坐标
 * @param y   像素Y坐标
 * @return uint8_t 调色板索引
 */
uint8_t colorlut_get_pixel(const uint8_t *buf, size_t x, size_t y)
{
#if COLORLUT_MAX_COLORS <= 16
    size_t pixel_index = y * COLORLUT_AREA_WIDTH + x;
    size_t byte_index = pixel_index / 2;
    if ((pixel_index & 1) == 0)
    {
        return buf[byte_index] & 0x0F;
    }
    else
    {
        return (buf[byte_index] >> 4) & 0x0F;
    }
#elif COLORLUT_MAX_COLORS <= 256
    size_t pixel_index = y * COLORLUT_AREA_WIDTH + x;
    return buf[pixel_index];
#endif
}

/**
 * @brief 绘制直线
 *
 * 使用 Bresenham 算法在索引缓冲区中绘制一条直线。
 * 算法通过逐点设置像素索引实现，支持任意斜率。
 *
 * @param buf 索引缓冲区
 * @param x0  起点X
 * @param y0  起点Y
 * @param x1  终点X
 * @param y1  终点Y
 * @param color_index 调色板索引
 */
void colorlut_draw_line(uint8_t *buf, size_t x0, size_t y0, size_t x1, size_t y1, uint8_t color_index)
{
    int dx = (int)x1 - (int)x0;
    int dy = (int)y1 - (int)y0;
    int sx = (dx >= 0) ? 1 : -1;
    int sy = (dy >= 0) ? 1 : -1;
    dx = (dx >= 0) ? dx : -dx;
    dy = (dy >= 0) ? dy : -dy;

    int err = dx - dy;

    while (1)
    {
        colorlut_set_pixel(buf, x0, y0, color_index);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

/**
 * @brief 绘制空心矩形
 *
 * 在索引缓冲区中绘制一个矩形的边框。
 * 通过调用直线绘制函数绘制四条边。
 *
 * @param buf 索引缓冲区
 * @param x   左上角X
 * @param y   左上角Y
 * @param w   宽度
 * @param h   高度
 * @param color_index 调色板索引
 */
void colorlut_draw_rect(uint8_t *buf, size_t x, size_t y, size_t w, size_t h, uint8_t color_index)
{
    size_t x1 = x + w - 1;
    size_t y1 = y + h - 1;

    // 上边
    colorlut_draw_line(buf, x, y, x1, y, color_index);
    // 下边
    colorlut_draw_line(buf, x, y1, x1, y1, color_index);
    // 左边
    colorlut_draw_line(buf, x, y, x, y1, color_index);
    // 右边
    colorlut_draw_line(buf, x1, y, x1, y1, color_index);
}

/**
 * @brief 绘制填充矩形
 *
 * @param buf 索引缓冲区
 * @param x   左上角X
 * @param y   左上角Y
 * @param w   宽度
 * @param h   高度
 * @param color_index 调色板索引
 */
void colorlut_fill_rect(uint8_t *buf, size_t x, size_t y, size_t w, size_t h, uint8_t color_index)
{
    for (size_t row = y; row < y + h; row++)
    {
        for (size_t col = x; col < x + w; col++)
        {
            colorlut_set_pixel(buf, col, row, color_index);
        }
    }
}

/**
 * @brief 在索引缓冲区中显示单个字符（叠加模式）
 *
 * @param buf 索引缓冲区指针
 * @param x 显示字符的屏幕X坐标
 * @param y 显示字符的屏幕Y坐标
 * @param character 要显示的字符
 * @param color_index 调色板索引（前景色）
 * @param font_size 字号
 */
void colorlut_show_char(uint8_t *buf, uint16_t x, uint16_t y, uint8_t character, uint8_t color_index, uint8_t font_size)
{
    uint8_t temp, char_width, t;
    uint16_t i, typeface_size;
    uint16_t x0 = x;

    char_width = font_size / 2;
    typeface_size = (char_width / 8 + ((char_width % 8) ? 1 : 0)) * font_size;
    character = character - ' '; // 得到偏移后的值

    for (i = 0; i < typeface_size; i++)
    {
        if (font_size == FONTSIZE_1206) temp = ascii_1206[character][i];
        else if (font_size == FONTSIZE_1608) temp = ascii_1608[character][i];
        else if (font_size == FONTSIZE_2412) temp = ascii_2412[character][i];
        else if (font_size == FONTSIZE_3216) temp = ascii_3216[character][i];
        else return;

        for (t = 0; t < 8; t++)
        {
            if (temp & (0x01 << t))
            {
                // 在缓冲区中绘制一个点（叠加模式，只设置前景色）
                colorlut_set_pixel(buf, x, y, color_index);
            }
            x++;
            if ((x - x0) == char_width)
            {
                x = x0;
                y++;
                break;
            }
        }
    }
}

/**
 * @brief 在索引缓冲区中显示字符串（叠加模式）
 *
 * @param buf 索引缓冲区指针
 * @param x 显示字符串的屏幕X坐标
 * @param y 显示字符串的屏幕Y坐标
 * @param str 要显示的字符串
 * @param color_index 调色板索引（前景色）
 * @param font_size 字号
 */
void colorlut_show_string(uint8_t *buf, uint16_t x, uint16_t y, const uint8_t *str, uint8_t color_index, uint8_t font_size)
{
    while (*str != '\0')
    {
        colorlut_show_char(buf, x, y, *str, color_index, font_size);
        x += font_size / 2; // 字符宽度
        str++;
    }
}


/**
 * @brief 将索引缓冲区内容刷新到LCD显示器
 *
 * 本函数支持局部区域刷新。它会将指定区域的索引缓冲区内容展开为RGB565数据，
 * 并通过SPI DMA传输到LCD。为了避免一次性占用过多内存，数据按行或列逐次传输。
 *
 * @param src     索引缓冲区指针（对应刷新区域的索引数据，原点为0,0）
 * @param x_start 刷新区域在屏幕的起始X坐标
 * @param y_start 刷新区域在屏幕的起始Y坐标
 * @param width   刷新区域的宽度（像素数）
 * @param height  刷新区域的高度（像素数）
 *
 * @note 使用前需确保LCD已初始化，并且SPI DMA可用。
 *       如果需要全屏刷新，可传入 (0,0,LCD_W,LCD_H)。
 */
void colorlut_flush_to_lcd(const uint8_t *src,
                           size_t x_start,
                           size_t y_start,
                           size_t width,
                           size_t height)
{
    // 设置显示区域
    lcdio_address_set(x_start, y_start, x_start + width - 1, y_start + height - 1);

    // 发送数据模式
    lcdio_write_mode(WRITE_DATA);

    bool is_first_line = true;

#if COLORLUT_VERTICAL_PRIORITY == 0
    // 横条优先：逐行扫描
    for (size_t y = 0; y < height; y += 1)
    {
        // 等待上一次 DMA 完成，确保 spi_buf 可安全写入
        if (is_first_line)
        {
            is_first_line = false;
        }
        else
        {
            while(lcdio_interface_is_free() == false);
        }

        // 展开一行数据到缓冲区，保证逐行逐像素顺序
        uint16_t buf_index = 0;
        for (size_t x = 0; x < width; x++)
        {
            uint8_t color_index = colorlut_get_pixel(src, x, y);
            uint16_t color = colorlut_get_color(color_index);
            spi_buf[buf_index] = (color >> 8) | (color << 8); // 高低字节交换
            buf_index++;
        }

        // 启动 DMA 传输（lines 行 × width 像素 × 2字节）
        lcdio_transmit_buffer((uint8_t*)spi_buf, width * 2);

    }
#else
    // 竖条优先：逐列扫描
    for (size_t x = 0; x < width; x += 1)
    {
        // 等待上一次 DMA 完成，确保 spi_buf 可安全写入
        if (is_first_line)
        {
            is_first_line = false;
        }
        else
        {
            while(lcdio_interface_is_free() == false);
        }

        // 展开一列数据到缓冲区，保证逐列逐像素顺序
        uint16_t buf_index = 0;
        for (size_t y = 0; y < height; y++)
        {
            uint8_t color_index = colorlut_get_pixel(src, x_start + x, y_start + y);
            spi_buf[buf_index] = colorlut_get_color(color_index);
            buf_index++;
        }

        // 启动 DMA 传输（lines 行 × width 像素 × 2字节）
        lcdio_transmit_buffer((uint8_t*)spi_buf, height * 2);
    }
#endif
    // 等待最后一次 DMA 完成 + SPI 空闲
    lcdio_wait_interface_free();
}



