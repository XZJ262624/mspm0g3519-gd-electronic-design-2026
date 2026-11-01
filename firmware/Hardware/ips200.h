#ifndef IPS200_H_
#define IPS200_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPS200_PANEL_WIDTH                (240U)
#define IPS200_PANEL_HEIGHT               (320U)
#define IPS200_LANDSCAPE_WIDTH            (320U)
#define IPS200_LANDSCAPE_HEIGHT           (240U)

#define IPS200_RGB565(r, g, b) \
    ((uint16_t)((((uint16_t)(r) & 0xF8U) << 8U) | \
                (((uint16_t)(g) & 0xFCU) << 3U) | \
                (((uint16_t)(b) & 0xF8U) >> 3U)))

#define IPS200_COLOR_BLACK                ((uint16_t)0x0000U)
#define IPS200_COLOR_WHITE                ((uint16_t)0xFFFFU)
#define IPS200_COLOR_RED                  ((uint16_t)0xF800U)
#define IPS200_COLOR_GREEN                ((uint16_t)0x07E0U)
#define IPS200_COLOR_BLUE                 ((uint16_t)0x001FU)
#define IPS200_COLOR_YELLOW               ((uint16_t)0xFFE0U)
#define IPS200_COLOR_CYAN                 ((uint16_t)0x07FFU)
#define IPS200_COLOR_MAGENTA              ((uint16_t)0xF81FU)
#define IPS200_COLOR_GRAY                 ((uint16_t)0x8410U)
#define IPS200_COLOR_ORANGE               ((uint16_t)0xFD20U)

typedef enum
{
    IPS200_ROTATION_0 = 0,      /* 240 x 320 */
    IPS200_ROTATION_90,         /* 320 x 240, default */
    IPS200_ROTATION_180,        /* 240 x 320 */
    IPS200_ROTATION_270         /* 320 x 240 */
} IPS200_Rotation;

/*
 * Call SYSCFG_DL_init() before IPS200_Init().
 * SysConfig must initialize SPI0 before this call.
 * IPS200_Init() selects 320 x 240 landscape mode and turns on the backlight.
 */
void IPS200_Init(void);
void IPS200_SetRotation(IPS200_Rotation rotation);
void IPS200_SetBacklight(bool on);
void IPS200_DisplayOn(bool on);

uint16_t IPS200_GetWidth(void);
uint16_t IPS200_GetHeight(void);

void IPS200_Fill(uint16_t color);
void IPS200_FillRect(uint16_t x, uint16_t y,
                     uint16_t width, uint16_t height, uint16_t color);
void IPS200_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void IPS200_DrawLine(int16_t x0, int16_t y0,
                     int16_t x1, int16_t y1, uint16_t color);
void IPS200_DrawRect(uint16_t x, uint16_t y,
                     uint16_t width, uint16_t height, uint16_t color);

/*
 * Draw an RGB565 image in row-major order.
 * Pixels extending past the right or bottom edge are clipped.
 */
void IPS200_DrawRGB565(uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height,
                       const uint16_t *pixels);

/*
 * Built-in 5 x 7 font in a 6 x 8 cell.
 * Supported directly: digits, A-Z, common punctuation.
 * Lowercase letters are displayed as uppercase.
 */
void IPS200_DrawChar(uint16_t x, uint16_t y, char character,
                     uint16_t foreground, uint16_t background,
                     uint8_t scale);
void IPS200_DrawString(uint16_t x, uint16_t y, const char *text,
                       uint16_t foreground, uint16_t background,
                       uint8_t scale);
void IPS200_DrawInt(uint16_t x, uint16_t y, int32_t value,
                    uint16_t foreground, uint16_t background,
                    uint8_t scale);

/*
 * Draw elapsed time as HH:MM:SS.mmm.
 * total_ms is expressed in milliseconds.
 * The displayed hour field wraps to 00 after 99:59:59.999.
 * Only changed characters are redrawn after the first complete draw.
 */
void IPS200_DrawRuntime(uint16_t x, uint16_t y, uint32_t total_ms,
                        uint16_t foreground, uint16_t background,
                        uint8_t scale);

/* Color-bar test used by main_example.c. */
void IPS200_TestPattern(void);

#ifdef __cplusplus
}
#endif

#endif /* IPS200_H_ */
