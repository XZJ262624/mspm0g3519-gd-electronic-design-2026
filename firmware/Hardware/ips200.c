#include "ips200.h"
#include "ips200_config.h"

#include <stddef.h>

/* ST7789V2 commands used by this driver. */
#define ST7789_CMD_SLEEP_OUT              (0x11U)
#define ST7789_CMD_DISPLAY_OFF            (0x28U)
#define ST7789_CMD_DISPLAY_ON             (0x29U)
#define ST7789_CMD_COLUMN_ADDRESS_SET     (0x2AU)
#define ST7789_CMD_ROW_ADDRESS_SET        (0x2BU)
#define ST7789_CMD_MEMORY_WRITE           (0x2CU)
#define ST7789_CMD_MEMORY_ACCESS_CONTROL  (0x36U)
#define ST7789_CMD_PIXEL_FORMAT_SET       (0x3AU)
#define ST7789_CMD_INVERSION_ON           (0x21U)

#define IPS200_DEFAULT_ROTATION           (IPS200_ROTATION_90)
#define IPS200_CHAR_WIDTH                 (6U)
#define IPS200_CHAR_HEIGHT                (8U)
#define IPS200_GLYPH_WIDTH                (5U)
#define IPS200_GLYPH_HEIGHT               (7U)

static uint16_t s_width = IPS200_LANDSCAPE_WIDTH;
static uint16_t s_height = IPS200_LANDSCAPE_HEIGHT;
static IPS200_Rotation s_rotation = IPS200_DEFAULT_ROTATION;

/*
 * Compact 5 x 7 font authored as five-bit rows.
 * Each bit maps left-to-right from bit 4 to bit 0.
 */
static const uint8_t s_digit_font[10][IPS200_GLYPH_HEIGHT] =
{
    {0x0EU, 0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x0EU}, /* 0 */
    {0x04U, 0x0CU, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU}, /* 1 */
    {0x0EU, 0x11U, 0x01U, 0x02U, 0x04U, 0x08U, 0x1FU}, /* 2 */
    {0x1EU, 0x01U, 0x01U, 0x0EU, 0x01U, 0x01U, 0x1EU}, /* 3 */
    {0x02U, 0x06U, 0x0AU, 0x12U, 0x1FU, 0x02U, 0x02U}, /* 4 */
    {0x1FU, 0x10U, 0x10U, 0x1EU, 0x01U, 0x01U, 0x1EU}, /* 5 */
    {0x0EU, 0x10U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x0EU}, /* 6 */
    {0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x08U, 0x08U}, /* 7 */
    {0x0EU, 0x11U, 0x11U, 0x0EU, 0x11U, 0x11U, 0x0EU}, /* 8 */
    {0x0EU, 0x11U, 0x11U, 0x0FU, 0x01U, 0x01U, 0x0EU}  /* 9 */
};

static const uint8_t s_upper_font[26][IPS200_GLYPH_HEIGHT] =
{
    {0x0EU, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U}, /* A */
    {0x1EU, 0x11U, 0x11U, 0x1EU, 0x11U, 0x11U, 0x1EU}, /* B */
    {0x0EU, 0x11U, 0x10U, 0x10U, 0x10U, 0x11U, 0x0EU}, /* C */
    {0x1EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x1EU}, /* D */
    {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x1FU}, /* E */
    {0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x10U}, /* F */
    {0x0EU, 0x11U, 0x10U, 0x17U, 0x11U, 0x11U, 0x0FU}, /* G */
    {0x11U, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U}, /* H */
    {0x0EU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU}, /* I */
    {0x07U, 0x02U, 0x02U, 0x02U, 0x12U, 0x12U, 0x0CU}, /* J */
    {0x11U, 0x12U, 0x14U, 0x18U, 0x14U, 0x12U, 0x11U}, /* K */
    {0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x1FU}, /* L */
    {0x11U, 0x1BU, 0x15U, 0x15U, 0x11U, 0x11U, 0x11U}, /* M */
    {0x11U, 0x19U, 0x15U, 0x13U, 0x11U, 0x11U, 0x11U}, /* N */
    {0x0EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU}, /* O */
    {0x1EU, 0x11U, 0x11U, 0x1EU, 0x10U, 0x10U, 0x10U}, /* P */
    {0x0EU, 0x11U, 0x11U, 0x11U, 0x15U, 0x12U, 0x0DU}, /* Q */
    {0x1EU, 0x11U, 0x11U, 0x1EU, 0x14U, 0x12U, 0x11U}, /* R */
    {0x0FU, 0x10U, 0x10U, 0x0EU, 0x01U, 0x01U, 0x1EU}, /* S */
    {0x1FU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U}, /* T */
    {0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU}, /* U */
    {0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0AU, 0x04U}, /* V */
    {0x11U, 0x11U, 0x11U, 0x15U, 0x15U, 0x15U, 0x0AU}, /* W */
    {0x11U, 0x11U, 0x0AU, 0x04U, 0x0AU, 0x11U, 0x11U}, /* X */
    {0x11U, 0x11U, 0x0AU, 0x04U, 0x04U, 0x04U, 0x04U}, /* Y */
    {0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x10U, 0x1FU}  /* Z */
};

static const uint8_t s_question_glyph[IPS200_GLYPH_HEIGHT] =
{
    0x0EU, 0x11U, 0x01U, 0x02U, 0x04U, 0x00U, 0x04U
};

static inline void IPS200_PinHigh(uint32_t pin)
{
    DL_GPIO_setPins(IPS200_GPIO_PORT, pin);
}

static inline void IPS200_PinLow(uint32_t pin)
{
    DL_GPIO_clearPins(IPS200_GPIO_PORT, pin);
}

static void IPS200_DelayMs(uint32_t milliseconds)
{
    while (milliseconds > 0U)
    {
        delay_cycles(CPUCLK_FREQ / 1000U);
        milliseconds--;
    }
}

static inline void IPS200_SPIWrite8(uint8_t data)
{
    DL_SPI_transmitDataBlocking8(IPS200_SPI_INST, data);
}

static inline void IPS200_SPIWrite16(uint16_t data)
{
    IPS200_SPIWrite8((uint8_t)(data >> 8U));
    IPS200_SPIWrite8((uint8_t)data);
}

/*
 * Blocking writes only guarantee space in the TX FIFO. Wait for the final
 * clock edge before changing DC or CS.
 */
static inline void IPS200_SPIWaitIdle(void)
{
    while (!DL_SPI_isTXFIFOEmpty(IPS200_SPI_INST))
    {
    }

    while (DL_SPI_isBusy(IPS200_SPI_INST))
    {
    }
}

static void IPS200_WriteCommandData(uint8_t command,
                                    const uint8_t *data,
                                    uint32_t length)
{
    IPS200_PinLow(IPS200_PIN_CS);
    IPS200_PinLow(IPS200_PIN_DC);
    IPS200_SPIWrite8(command);
    IPS200_SPIWaitIdle();
    IPS200_PinHigh(IPS200_PIN_DC);

    while (length > 0U)
    {
        IPS200_SPIWrite8(*data);
        data++;
        length--;
    }

    IPS200_SPIWaitIdle();
    IPS200_PinHigh(IPS200_PIN_CS);
}

static void IPS200_WriteCommand(uint8_t command)
{
    IPS200_WriteCommandData(command, NULL, 0U);
}

static void IPS200_SetAddressWindow(uint16_t x0, uint16_t y0,
                                    uint16_t x1, uint16_t y1)
{
    uint8_t address[4];

    address[0] = (uint8_t)(x0 >> 8U);
    address[1] = (uint8_t)x0;
    address[2] = (uint8_t)(x1 >> 8U);
    address[3] = (uint8_t)x1;
    IPS200_WriteCommandData(ST7789_CMD_COLUMN_ADDRESS_SET, address, 4U);

    address[0] = (uint8_t)(y0 >> 8U);
    address[1] = (uint8_t)y0;
    address[2] = (uint8_t)(y1 >> 8U);
    address[3] = (uint8_t)y1;
    IPS200_WriteCommandData(ST7789_CMD_ROW_ADDRESS_SET, address, 4U);
}

static inline void IPS200_BeginMemoryWrite(void)
{
    IPS200_PinLow(IPS200_PIN_CS);
    IPS200_PinLow(IPS200_PIN_DC);
    IPS200_SPIWrite8(ST7789_CMD_MEMORY_WRITE);
    IPS200_SPIWaitIdle();
    IPS200_PinHigh(IPS200_PIN_DC);
}

static inline void IPS200_EndMemoryWrite(void)
{
    IPS200_SPIWaitIdle();
    IPS200_PinHigh(IPS200_PIN_CS);
}

static void IPS200_WriteColorRepeat(uint16_t color, uint32_t count)
{
    while (count > 0U)
    {
        IPS200_SPIWrite16(color);
        count--;
    }
}

static uint8_t IPS200_GetGlyphRow(char character, uint8_t row)
{
    if ((character >= 'a') && (character <= 'z'))
    {
        character = (char)(character - ('a' - 'A'));
    }

    if ((character >= '0') && (character <= '9'))
    {
        return s_digit_font[(uint8_t)(character - '0')][row];
    }

    if ((character >= 'A') && (character <= 'Z'))
    {
        return s_upper_font[(uint8_t)(character - 'A')][row];
    }

    switch (character)
    {
        case ' ': return 0x00U;
        case '-': return (row == 3U) ? 0x1FU : 0x00U;
        case '_': return (row == 6U) ? 0x1FU : 0x00U;
        case '.': return (row == 6U) ? 0x04U : 0x00U;
        case ':': return ((row == 2U) || (row == 5U)) ? 0x04U : 0x00U;
        case '+':
            if (row == 3U)
            {
                return 0x1FU;
            }
            return ((row == 1U) || (row == 2U) ||
                    (row == 4U) || (row == 5U)) ? 0x04U : 0x00U;
        case '/':
            switch (row)
            {
                case 0U: return 0x01U;
                case 1U: return 0x02U;
                case 2U: return 0x02U;
                case 3U: return 0x04U;
                case 4U: return 0x08U;
                case 5U: return 0x08U;
                default: return 0x10U;
            }
        case '=': return ((row == 2U) || (row == 4U)) ? 0x1FU : 0x00U;
        case '%':
            switch (row)
            {
                case 0U: return 0x19U;
                case 1U: return 0x1AU;
                case 2U: return 0x04U;
                case 3U: return 0x04U;
                case 4U: return 0x0BU;
                case 5U: return 0x13U;
                default: return 0x00U;
            }
        default: return s_question_glyph[row];
    }
}

void IPS200_SetBacklight(bool on)
{
#if IPS200_BLK_ACTIVE_HIGH
    if (on)
    {
        IPS200_PinHigh(IPS200_PIN_BLK);
    }
    else
    {
        IPS200_PinLow(IPS200_PIN_BLK);
    }
#else
    if (on)
    {
        IPS200_PinLow(IPS200_PIN_BLK);
    }
    else
    {
        IPS200_PinHigh(IPS200_PIN_BLK);
    }
#endif
}

void IPS200_DisplayOn(bool on)
{
    IPS200_WriteCommand(on ? ST7789_CMD_DISPLAY_ON :
                            ST7789_CMD_DISPLAY_OFF);
}

void IPS200_SetRotation(IPS200_Rotation rotation)
{
    uint8_t madctl;

    switch (rotation)
    {
        case IPS200_ROTATION_0:
            madctl = 0x00U;
            s_width = IPS200_PANEL_WIDTH;
            s_height = IPS200_PANEL_HEIGHT;
            break;

        case IPS200_ROTATION_180:
            madctl = 0xC0U;
            s_width = IPS200_PANEL_WIDTH;
            s_height = IPS200_PANEL_HEIGHT;
            break;

        case IPS200_ROTATION_270:
            madctl = 0xA0U;
            s_width = IPS200_LANDSCAPE_WIDTH;
            s_height = IPS200_LANDSCAPE_HEIGHT;
            break;

        case IPS200_ROTATION_90:
        default:
            rotation = IPS200_ROTATION_90;
            madctl = 0x70U;
            s_width = IPS200_LANDSCAPE_WIDTH;
            s_height = IPS200_LANDSCAPE_HEIGHT;
            break;
    }

    s_rotation = rotation;
    IPS200_WriteCommandData(ST7789_CMD_MEMORY_ACCESS_CONTROL, &madctl, 1U);
}

uint16_t IPS200_GetWidth(void)
{
    return s_width;
}

uint16_t IPS200_GetHeight(void)
{
    return s_height;
}

void IPS200_Init(void)
{
    static const uint8_t porch_control[] =
    {
        0x0CU, 0x0CU, 0x00U, 0x33U, 0x33U
    };
    static const uint8_t power_control[] = {0xA4U, 0xA1U};
    static const uint8_t positive_gamma[] =
    {
        0xD0U, 0x08U, 0x0EU, 0x09U, 0x09U, 0x05U, 0x31U,
        0x33U, 0x48U, 0x17U, 0x14U, 0x15U, 0x31U, 0x34U
    };
    static const uint8_t negative_gamma[] =
    {
        0xD0U, 0x08U, 0x0EU, 0x09U, 0x09U, 0x15U, 0x31U,
        0x33U, 0x48U, 0x17U, 0x14U, 0x15U, 0x31U, 0x34U
    };
    uint8_t parameter;

    /*
     * SysConfig owns the PA12/PA9 SPI pin mux. Only configure the four
     * software-controlled pins here.
     */
    DL_GPIO_clearPins(IPS200_GPIO_PORT, IPS200_PIN_BLK);
    DL_GPIO_setPins(IPS200_GPIO_PORT,
                    IPS200_PIN_CS | IPS200_PIN_DC | IPS200_PIN_RST);
    DL_GPIO_enableOutput(IPS200_GPIO_PORT, IPS200_CONTROL_PINS);

    IPS200_SetBacklight(false);

    IPS200_PinLow(IPS200_PIN_RST);
    IPS200_DelayMs(5U);
    IPS200_PinHigh(IPS200_PIN_RST);
    IPS200_DelayMs(120U);

    IPS200_WriteCommand(ST7789_CMD_SLEEP_OUT);
    IPS200_DelayMs(120U);

    s_rotation = IPS200_DEFAULT_ROTATION;
    IPS200_SetRotation(s_rotation);

    parameter = 0x05U; /* 16-bit RGB565 */
    IPS200_WriteCommandData(ST7789_CMD_PIXEL_FORMAT_SET, &parameter, 1U);
    IPS200_WriteCommandData(0xB2U, porch_control,
                            (uint32_t)sizeof(porch_control));

    parameter = 0x35U;
    IPS200_WriteCommandData(0xB7U, &parameter, 1U);
    parameter = 0x29U;
    IPS200_WriteCommandData(0xBBU, &parameter, 1U);
    parameter = 0x01U;
    IPS200_WriteCommandData(0xC2U, &parameter, 1U);
    parameter = 0x19U;
    IPS200_WriteCommandData(0xC3U, &parameter, 1U);
    parameter = 0x20U;
    IPS200_WriteCommandData(0xC4U, &parameter, 1U);
    parameter = 0x1AU;
    IPS200_WriteCommandData(0xC5U, &parameter, 1U);
    parameter = 0x1FU;
    IPS200_WriteCommandData(0xC6U, &parameter, 1U);

    IPS200_WriteCommandData(0xD0U, power_control,
                            (uint32_t)sizeof(power_control));
    IPS200_WriteCommandData(0xE0U, positive_gamma,
                            (uint32_t)sizeof(positive_gamma));
    IPS200_WriteCommandData(0xE1U, negative_gamma,
                            (uint32_t)sizeof(negative_gamma));

    IPS200_WriteCommand(ST7789_CMD_INVERSION_ON);
    IPS200_DisplayOn(true);
    IPS200_DelayMs(20U);

    IPS200_Fill(IPS200_COLOR_BLACK);
    IPS200_SetBacklight(true);
}

void IPS200_FillRect(uint16_t x, uint16_t y,
                     uint16_t width, uint16_t height, uint16_t color)
{
    uint32_t pixel_count;

    if ((width == 0U) || (height == 0U) ||
        (x >= s_width) || (y >= s_height))
    {
        return;
    }

    if (width > (uint16_t)(s_width - x))
    {
        width = (uint16_t)(s_width - x);
    }

    if (height > (uint16_t)(s_height - y))
    {
        height = (uint16_t)(s_height - y);
    }

    IPS200_SetAddressWindow(x, y,
                            (uint16_t)(x + width - 1U),
                            (uint16_t)(y + height - 1U));
    IPS200_BeginMemoryWrite();

    pixel_count = (uint32_t)width * (uint32_t)height;
    IPS200_WriteColorRepeat(color, pixel_count);

    IPS200_EndMemoryWrite();
}

void IPS200_Fill(uint16_t color)
{
    IPS200_FillRect(0U, 0U, s_width, s_height, color);
}

void IPS200_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    IPS200_FillRect(x, y, 1U, 1U, color);
}

void IPS200_DrawLine(int16_t x0, int16_t y0,
                     int16_t x1, int16_t y1, uint16_t color)
{
    int32_t dx = (x1 >= x0) ? (int32_t)x1 - x0 : (int32_t)x0 - x1;
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy_abs = (y1 >= y0) ? (int32_t)y1 - y0 : (int32_t)y0 - y1;
    int32_t dy = -dy_abs;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t error = dx + dy;

    for (;;)
    {
        if ((x0 >= 0) && (y0 >= 0) &&
            ((uint16_t)x0 < s_width) && ((uint16_t)y0 < s_height))
        {
            IPS200_DrawPixel((uint16_t)x0, (uint16_t)y0, color);
        }

        if ((x0 == x1) && (y0 == y1))
        {
            break;
        }

        {
            int32_t twice_error = 2 * error;

            if (twice_error >= dy)
            {
                error += dy;
                x0 = (int16_t)(x0 + sx);
            }

            if (twice_error <= dx)
            {
                error += dx;
                y0 = (int16_t)(y0 + sy);
            }
        }
    }
}

void IPS200_DrawRect(uint16_t x, uint16_t y,
                     uint16_t width, uint16_t height, uint16_t color)
{
    uint16_t x1;
    uint16_t y1;

    if ((width == 0U) || (height == 0U) ||
        (x >= s_width) || (y >= s_height))
    {
        return;
    }

    if (width > (uint16_t)(s_width - x))
    {
        width = (uint16_t)(s_width - x);
    }

    if (height > (uint16_t)(s_height - y))
    {
        height = (uint16_t)(s_height - y);
    }

    x1 = (uint16_t)(x + width - 1U);
    y1 = (uint16_t)(y + height - 1U);

    IPS200_DrawLine((int16_t)x, (int16_t)y,
                    (int16_t)x1, (int16_t)y, color);
    IPS200_DrawLine((int16_t)x, (int16_t)y1,
                    (int16_t)x1, (int16_t)y1, color);
    IPS200_DrawLine((int16_t)x, (int16_t)y,
                    (int16_t)x, (int16_t)y1, color);
    IPS200_DrawLine((int16_t)x1, (int16_t)y,
                    (int16_t)x1, (int16_t)y1, color);
}

void IPS200_DrawRGB565(uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height,
                       const uint16_t *pixels)
{
    uint16_t draw_width;
    uint16_t draw_height;
    uint16_t row;
    uint16_t column;

    if ((pixels == NULL) || (width == 0U) || (height == 0U) ||
        (x >= s_width) || (y >= s_height))
    {
        return;
    }

    draw_width = width;
    draw_height = height;

    if (draw_width > (uint16_t)(s_width - x))
    {
        draw_width = (uint16_t)(s_width - x);
    }

    if (draw_height > (uint16_t)(s_height - y))
    {
        draw_height = (uint16_t)(s_height - y);
    }

    IPS200_SetAddressWindow(x, y,
                            (uint16_t)(x + draw_width - 1U),
                            (uint16_t)(y + draw_height - 1U));
    IPS200_BeginMemoryWrite();

    for (row = 0U; row < draw_height; row++)
    {
        const uint16_t *source_row = pixels + ((uint32_t)row * width);

        for (column = 0U; column < draw_width; column++)
        {
            IPS200_SPIWrite16(source_row[column]);
        }
    }

    IPS200_EndMemoryWrite();
}

void IPS200_DrawChar(uint16_t x, uint16_t y, char character,
                     uint16_t foreground, uint16_t background,
                     uint8_t scale)
{
    uint16_t cell_width;
    uint16_t cell_height;
    uint16_t output_row;
    uint16_t output_column;

    if (scale == 0U)
    {
        scale = 1U;
    }

    cell_width = (uint16_t)(IPS200_CHAR_WIDTH * scale);
    cell_height = (uint16_t)(IPS200_CHAR_HEIGHT * scale);

    if ((x >= s_width) || (y >= s_height) ||
        (cell_width > (uint16_t)(s_width - x)) ||
        (cell_height > (uint16_t)(s_height - y)))
    {
        return;
    }

    IPS200_SetAddressWindow(x, y,
                            (uint16_t)(x + cell_width - 1U),
                            (uint16_t)(y + cell_height - 1U));
    IPS200_BeginMemoryWrite();

    for (output_row = 0U; output_row < cell_height; output_row++)
    {
        uint8_t glyph_row_index = (uint8_t)(output_row / scale);
        uint8_t glyph_row = (glyph_row_index < IPS200_GLYPH_HEIGHT) ?
                            IPS200_GetGlyphRow(character, glyph_row_index) :
                            0U;

        for (output_column = 0U;
             output_column < cell_width;
             output_column++)
        {
            uint8_t glyph_column = (uint8_t)(output_column / scale);
            bool pixel_on = false;

            if (glyph_column < IPS200_GLYPH_WIDTH)
            {
                uint8_t mask =
                    (uint8_t)(1U << (IPS200_GLYPH_WIDTH - 1U - glyph_column));
                pixel_on = ((glyph_row & mask) != 0U);
            }

            IPS200_SPIWrite16(pixel_on ? foreground : background);
        }
    }

    IPS200_EndMemoryWrite();
}

void IPS200_DrawString(uint16_t x, uint16_t y, const char *text,
                       uint16_t foreground, uint16_t background,
                       uint8_t scale)
{
    uint16_t start_x = x;
    uint16_t cell_width;
    uint16_t cell_height;

    if (text == NULL)
    {
        return;
    }

    if (scale == 0U)
    {
        scale = 1U;
    }

    if ((start_x >= s_width) || (y >= s_height))
    {
        return;
    }

    cell_width = (uint16_t)(IPS200_CHAR_WIDTH * scale);
    cell_height = (uint16_t)(IPS200_CHAR_HEIGHT * scale);

    if ((cell_width > (uint16_t)(s_width - start_x)) ||
        (cell_height > (uint16_t)(s_height - y)))
    {
        return;
    }

    while (*text != '\0')
    {
        if (*text == '\n')
        {
            x = start_x;
            y = (uint16_t)(y + cell_height);
            text++;
            continue;
        }

        if (cell_width > (uint16_t)(s_width - x))
        {
            x = start_x;
            y = (uint16_t)(y + cell_height);
        }

        if ((y >= s_height) ||
            (cell_height > (uint16_t)(s_height - y)))
        {
            break;
        }

        IPS200_DrawChar(x, y, *text, foreground, background, scale);
        x = (uint16_t)(x + cell_width);
        text++;
    }
}

void IPS200_DrawInt(uint16_t x, uint16_t y, int32_t value,
                    uint16_t foreground, uint16_t background,
                    uint8_t scale)
{
    char buffer[12];
    uint8_t length = 0U;
    uint8_t index;
    uint32_t magnitude;

    if (value < 0)
    {
        magnitude = (uint32_t)(-(value + 1)) + 1U;
    }
    else
    {
        magnitude = (uint32_t)value;
    }

    do
    {
        buffer[length] = (char)('0' + (magnitude % 10U));
        length++;
        magnitude /= 10U;
    }
    while ((magnitude != 0U) && (length < (uint8_t)(sizeof(buffer) - 1U)));

    if ((value < 0) && (length < (uint8_t)(sizeof(buffer) - 1U)))
    {
        buffer[length] = '-';
        length++;
    }

    for (index = 0U; index < (uint8_t)(length / 2U); index++)
    {
        char temporary = buffer[index];
        buffer[index] = buffer[(uint8_t)(length - 1U - index)];
        buffer[(uint8_t)(length - 1U - index)] = temporary;
    }

    buffer[length] = '\0';
    IPS200_DrawString(x, y, buffer, foreground, background, scale);
}

void IPS200_DrawRuntime(uint16_t x, uint16_t y, uint32_t total_ms,
                        uint16_t foreground, uint16_t background,
                        uint8_t scale)
{
    enum
    {
        RUNTIME_CHAR_COUNT = 12
    };

    static char previous_text[RUNTIME_CHAR_COUNT + 1U];
    static uint16_t previous_x;
    static uint16_t previous_y;
    static uint16_t previous_foreground;
    static uint16_t previous_background;
    static uint8_t previous_scale;
    static bool cache_valid = false;

    char time_text[RUNTIME_CHAR_COUNT + 1U];
    uint16_t cell_width;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
    uint32_t milliseconds;
    uint8_t index;
    bool redraw_all;

    hours = (total_ms / 3600000U) % 100U;
    minutes = (total_ms / 60000U) % 60U;
    seconds = (total_ms / 1000U) % 60U;
    milliseconds = total_ms % 1000U;

    if (scale == 0U)
    {
        scale = 1U;
    }

    time_text[0] = (char)('0' + (hours / 10U));
    time_text[1] = (char)('0' + (hours % 10U));
    time_text[2] = ':';
    time_text[3] = (char)('0' + (minutes / 10U));
    time_text[4] = (char)('0' + (minutes % 10U));
    time_text[5] = ':';
    time_text[6] = (char)('0' + (seconds / 10U));
    time_text[7] = (char)('0' + (seconds % 10U));
    time_text[8] = '.';
    time_text[9] = (char)('0' + (milliseconds / 100U));
    time_text[10] =
        (char)('0' + ((milliseconds / 10U) % 10U));
    time_text[11] = (char)('0' + (milliseconds % 10U));
    time_text[12] = '\0';

    cell_width = (uint16_t)(IPS200_CHAR_WIDTH * scale);

    redraw_all =
        (!cache_valid) ||
        (x != previous_x) ||
        (y != previous_y) ||
        (foreground != previous_foreground) ||
        (background != previous_background) ||
        (scale != previous_scale);

    /*
     * The main loop calls this function once per millisecond. Redrawing only
     * changed characters prevents a full 12-character SPI transfer each time.
     */
    for (index = 0U; index < RUNTIME_CHAR_COUNT; index++)
    {
        if (redraw_all ||
            (time_text[index] != previous_text[index]))
        {
            IPS200_DrawChar(
                (uint16_t)(x + ((uint16_t)index * cell_width)),
                y,
                time_text[index],
                foreground,
                background,
                scale);
        }

        previous_text[index] = time_text[index];
    }

    previous_text[RUNTIME_CHAR_COUNT] = '\0';
    previous_x = x;
    previous_y = y;
    previous_foreground = foreground;
    previous_background = background;
    previous_scale = scale;
    cache_valid = true;
}

void IPS200_TestPattern(void)
{
    static const uint16_t colors[8] =
    {
        IPS200_COLOR_WHITE,
        IPS200_COLOR_YELLOW,
        IPS200_COLOR_CYAN,
        IPS200_COLOR_GREEN,
        IPS200_COLOR_MAGENTA,
        IPS200_COLOR_RED,
        IPS200_COLOR_BLUE,
        IPS200_COLOR_BLACK
    };
    uint16_t bar_width = (uint16_t)(s_width / 8U);
    uint8_t index;

    for (index = 0U; index < 8U; index++)
    {
        uint16_t x = (uint16_t)((uint16_t)index * bar_width);
        uint16_t width = (index == 7U) ?
                         (uint16_t)(s_width - x) : bar_width;

        IPS200_FillRect(x, 0U, width, s_height, colors[index]);
    }
}
