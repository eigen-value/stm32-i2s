#pragma once

#define STM32_I2S_WITH_OBJECT
#define USE_FULL_ASSERT

#ifndef I2S_FULLDUPLEXMODE_DISABLE
#  define I2S_FULLDUPLEXMODE_DISABLE   (0x00000000U)
#endif
#ifndef I2S_FULLDUPLEXMODE_ENABLE
#  define 	I2S_FULLDUPLEXMODE_ENABLE   (0x00000001U)
#endif

#ifdef ARDUINO_BLACKPILL_F411CE
  #define SPI_INSTANCE_FOR_I2S SPI3
  #define STM_I2S_PINS \
  {\
    {mclk, PB_10, GPIO_AF6_SPI3},\
    {bck, PB_3, GPIO_AF6_SPI3},\
    {ws, PA_4, GPIO_AF6_SPI3}, \
    {data_out, PB_4, GPIO_AF7_I2S3ext},\
    {data_in, PB_5, GPIO_AF6_SPI3},\
  }
// 8 MHz / M * N / R  => I2S Freq
  #define PLLM  16
  #define PLLN 192
  #define PLLR   2
  #define IS_F4

#endif

#if defined(ARDUINO_GENERIC_F411VETX) || defined(ARDUINO_DISCO_F411VE)
  #define SPI_INSTANCE_FOR_I2S SPI3
  #define STM_I2S_PINS \
    { \
      {mclk, PC_7, GPIO_AF6_SPI3},\
      {bck, PC_10, GPIO_AF6_SPI3},\
      {ws, PA_4, GPIO_AF6_SPI3},\
      {data_out, PC_12, GPIO_AF6_SPI3},\
      {data_in, PC_3, GPIO_AF6_SPI3}\
    };
// This board never enables HSE (confirmed via RCC_CR: HSEON=0/HSERDY=0) - the
// main PLL runs off the internal 16MHz HSI (RCC_PLLCFGR.PLLSRC=0), and
// PLLI2S shares that same source. HSI/PLLM*PLLN/PLLR => I2SxCLK, further
// divided by the I2SDIV/ODD prescaler (computed by HAL to best match the
// requested sample rate) to get Fs.
// These are the fallback/default values used for the 44.1kHz family
// (44100/22050/11025) - see Stm32I2sClass::getPLLI2S() in stm32-i2s.h, which
// picks a different, per-rate-tuned PLLM/N/R for the 48kHz family
// (8000/16000/32000/48000/96000/192000) since a single fixed setting can
// only ever be exact for one specific rate. PLLM=16 (not 8) is required
// here specifically because the reference is 16MHz, not 8MHz: with PLLM=8
// the PLLI2S VCO output hits 858MHz, nearly double the STM32F411's 432MHz
// maximum - an out-of-spec, jittery PLL that produces noise instead of a
// clean tone even though the average divided-down frequency looks correct
// on paper.
  #define PLLM   16
  #define PLLN  429
  #define PLLR    2
  #define IS_F4

// Empirically measured on THIS board only (via a monotonic frame counter
// sampled over a precisely-timed multi-second window): the real I2S output
// rate is consistently ~4x the requested Init.AudioFreq, even though
// i2sclk, Init.AudioFreq and the resulting I2SDIV/ODD were all
// independently confirmed correct via live register/variable inspection
// inside HAL_I2S_Init() itself - the discrepancy could not be traced to
// any specific HAL computation. Confirmed at two different target rates
// (11025 and 44100), each landing within ~1% of the true intended
// frequency once divided by 4. Not validated on any other board/chip -
// keep this scoped to ARDUINO_GENERIC_F411VETX specifically rather than
// applying it to every STM_I2S_PINS board.
  #define I2S_AUDIOFREQ_CORRECTION_DIV 4

#endif

#if defined(ARDUINO_GENERIC_F411VCTX)
  #define SPI_INSTANCE_FOR_I2S SPI2
  #define STM_I2S_PINS \
    { \
      {mclk, PC_6, GPIO_AF5_SPI2},\
      {bck, PB_10, GPIO_AF5_SPI2},\
      {ws, PB_12, GPIO_AF5_SPI2},\
      {data_out, PC_3, GPIO_AF5_SPI2},\
      {data_in, PC_2, GPIO_AF6_I2S2ext}\
    };

  #define PLLM   16
  #define PLLN  429
  #define PLLR    2
  #define IS_F4

// Same PLLM/N/R derivation as the ARDUINO_GENERIC_F411VETX block above (16MHz
// HSI reference, PLLM=16 to keep the PLLI2S VCO within spec). Confirmed
// working on real ARDUINO_GENERIC_F411VCTX hardware via PR #10, using SPI2 /
// DMA1 Stream4 instead of SPI3 / DMA1 Stream5. The correction factor was
// carried over from the VETX board rather than independently re-measured via
// register inspection on this board - if audio comes out at the wrong pitch,
// re-derive it for this specific board rather than assuming it still holds.
  #define I2S_AUDIOFREQ_CORRECTION_DIV 4

#endif

#ifdef STM32H750xx
  #define SPI_INSTANCE_FOR_I2S SPI3
  #define STM_I2S_PINS \
    { \
      {mclk, PC_7, GPIO_AF6_SPI3},\
      {bck, PC_10, GPIO_AF6_SPI3},\
      {ws, PA_4, GPIO_AF6_SPI3},\
      {data_out, PB_2, GPIO_AF6_SPI3},\
      {data_in, PC_11, GPIO_AF6_SPI3}\
    };
  #define IS_H7
#endif

#ifdef STM32H743xx
  #define SPI_INSTANCE_FOR_I2S SPI3
  #define STM_I2S_PINS \
    { \
      {mclk, PC_7, GPIO_AF6_SPI3},\
      {bck, PC_10, GPIO_AF6_SPI3},\
      {ws, PA_4, GPIO_AF6_SPI3},\
      {data_out, PB_5, GPIO_AF6_SPI3},\
      {data_in, PB_4, GPIO_AF6_SPI3}\
    };
  #define IS_H7
#endif

#ifdef STM32WB55xx
// STM32WB55xx do not provide SPI based I2S, so we cannot use I2S on this platform.
// They require the SAI/I2S API!
#endif

#ifdef STM32F723xx
  #define SPI_INSTANCE_FOR_I2S SPI3
  #define STM_I2S_PINS \
    { \
      {mclk, PC_7, GPIO_AF6_SPI3},\
      {bck, PC_10, GPIO_AF6_SPI3},\
      {ws, PA_4, GPIO_AF6_SPI3},\
      {data_out, PC_12, GPIO_AF6_SPI3},\
      {data_in, PC_11, GPIO_AF6_SPI3}\
    };
  #define IS_F7
// Unlike F4, F7's RCC_PLLI2SInitTypeDef has no PLLI2SM field - PLLI2S shares
// the main PLL's M divider (set to 8 by DISCO_F723IE's SystemClock_Config(),
// off the 16MHz HSI), giving a 2MHz PLLI2S VCO input. PLLN=192/PLLR=5 is the
// common CubeMX default for that input (76.8MHz I2S kernel clock);
// HAL_I2S_Init() derives the I2SDIV/ODD prescaler from Init.AudioFreq
// against that clock, so unlike the F411 boards above a per-sample-rate-
// tuned value isn't needed here.
  #define PLLN 192
  #define PLLR 5
#endif
