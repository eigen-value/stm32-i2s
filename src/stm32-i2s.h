/**
 * @file stm32-i2s.h
 * @author phil Schatzmann
 * @brief Main entry point header for this library
 * @version 0.1
 * @date 2022-09-22
 *
 * @copyright Copyright (c) 2022
 */
#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Arduino.h"
#include "PinConfigured.h"
#include "stm32-config-i2s.h"
#ifdef STM32H7xx
#  include "stm32h7xx_hal.h"
#  include "stm32h7xx_ll_rcc.h"
#endif
#ifdef STM32F4xx
#  include "stm32f4xx_hal.h"
#endif

// Full-duplex (simultaneous tx+rx on one I2S instance) needs the I2Sxext
// peripheral, which F4 and H7 have and F7 (e.g. F723) does not - HAL_I2SEx_
// TransmitReceive_DMA isn't even declared without it.
#if defined(IS_F4) || defined(IS_H7)
#define I2S_HAS_FULLDUPLEX_EXT
#endif

#ifdef STM_I2S_PINS


extern uint32_t g_anOutputPinConfigured[MAX_NB_PORT];

namespace stm32_i2s {

using byte = uint8_t;

extern "C" void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s);
extern "C" void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
extern "C" void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s);
extern "C" void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
extern "C" void DMA1_Stream0_IRQHandler(void);
extern "C" void DMA1_Stream5_IRQHandler(void);
extern "C" void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s);
extern "C" void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s);
extern "C" void Report_Error(int no);
extern "C" void STM32_LOG(const char *fmt, ...);
extern bool stm32_i2s_is_error;

/// @brief i2s pin function used as documentation
enum I2SPinFunction { mclk, bck, ws, data_out, data_in };

/**
 * @brief Define individual Pin. This is used to set up processor specific
 * arrays with all I2S pins and to setup and end the pin definition.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */

struct I2SPin {
  I2SPin(I2SPinFunction f, PinName n, int alt) {
    function = f;
    pin = n;
    altFunction = alt;
  }
  I2SPinFunction function;
  PinName pin;
  int altFunction;

  void begin() {
    end();
    // define the pin function
    pin_function(pin, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_NOPULL, altFunction));
  }

  /// Undo the current pin function
  void end() {
    PinName p = pin;
    if (p != NC) {
      // If the pin that support PWM or DAC output, we need to turn it off
#if (defined(HAL_DAC_MODULE_ENABLED) && !defined(HAL_DAC_MODULE_ONLY)) || \
    (defined(HAL_TIM_MODULE_ENABLED) && !defined(HAL_TIM_MODULE_ONLY))
      if (is_pin_configured(p, g_anOutputPinConfigured)) {
#if defined(HAL_DAC_MODULE_ENABLED) && !defined(HAL_DAC_MODULE_ONLY)
        if (pin_in_pinmap(p, PinMap_DAC)) {
          dac_stop(p);
        } else
#endif  // HAL_DAC_MODULE_ENABLED && !HAL_DAC_MODULE_ONLY
#if defined(HAL_TIM_MODULE_ENABLED) && !defined(HAL_TIM_MODULE_ONLY)
            if (pin_in_pinmap(p, PinMap_TIM)) {
          pwm_stop(p);
        }
#endif  // HAL_TIM_MODULE_ENABLED && !HAL_TIM_MODULE_ONLY
        {
          reset_pin_configured(p, g_anOutputPinConfigured);
        }
      }
#endif
    }
  }
};

/**
 * @brief Processor specific settings that are needed to set up I2S
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
struct HardwareConfig {
  IRQn_Type irq1 = DMA1_Stream0_IRQn;
  IRQn_Type irq2 = DMA1_Stream5_IRQn;

  DMA_Stream_TypeDef *rx_instance = DMA1_Stream0;
  #ifdef IS_F4
  uint32_t rx_channel = DMA_CHANNEL_3;
  #endif
  uint32_t rx_direction = DMA_PERIPH_TO_MEMORY;

  DMA_Stream_TypeDef *tx_instance = DMA1_Stream5;
  #ifdef IS_F4
  uint32_t tx_channel = DMA_CHANNEL_0;
  #endif
  uint32_t tx_direction = DMA_MEMORY_TO_PERIPH;

  I2SPin pins[5] = STM_I2S_PINS;

  int buffer_size = 1024;

  HardwareConfig() {
    // overwrite processor specific default settings if necessary
  }
};

/**
 * @brief Currently supported parameters
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
struct I2SSettingsSTM32 {
  uint32_t mode = I2S_MODE_MASTER_TX;
  uint32_t standard = I2S_STANDARD_PHILIPS;
  uint32_t fullduplexmode = I2S_FULLDUPLEXMODE_ENABLE;
  uint32_t sample_rate = I2S_AUDIOFREQ_44K;
  uint32_t data_format = I2S_DATAFORMAT_16B;
  HardwareConfig hardware_config;
  // optioinal reference that will be provided by the callbacks
  void *ref = nullptr;
};


class Stm32I2sClass;
extern Stm32I2sClass *self_I2S;

/**
 * @brief I2S API for STM32
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class Stm32I2sClass {
  friend void DMA1_Stream0_IRQHandler(void);
  friend void DMA1_Stream5_IRQHandler(void);
  friend void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s);
  friend void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s);
  friend void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s);
  friend void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
  friend void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s);
  friend void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s);

 public:
  Stm32I2sClass() {self_I2S = this; }
  /// start I2S w/o DMA: use write and readBytes
  bool begin(I2SSettingsSTM32 settings, bool transmit, bool receive) {
    this->use_dma = false;
    this->settings = settings;
    this->hw = settings.hardware_config;
    int buffer_size = hw.buffer_size;
    bool result = true;

    if (!i2s_begin()) {
      return false;
    }

    if (!transmit && !receive) {
      return false;
    }

    if (use_dma) {
      // HAL DMA calls expect a sample count, not a byte count
      int samples = buffer_size / getBytes();
      if (transmit && !receive) {
        if (HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t *)dma_buffer_tx,
                                 samples) != HAL_OK) {
          STM32_LOG("error HAL_I2S_Transmit_DMA");
          Report_Error(1);
          result = false;
        }
      }

      if (receive && !transmit) {
        if (HAL_I2S_Receive_DMA(&hi2s3, (uint16_t *)dma_buffer_rx,
                                samples) != HAL_OK) {
          STM32_LOG("error: HAL_I2S_Receive_DMA");
          Report_Error(2);
          result = false;
        }
      }

      if (receive && transmit) {
#ifdef I2S_HAS_FULLDUPLEX_EXT
        if (HAL_I2SEx_TransmitReceive_DMA(&hi2s3, (uint16_t *)dma_buffer_tx,
                                          (uint16_t *)dma_buffer_rx,
                                          samples) != HAL_OK) {
          STM32_LOG("error HAL_I2SEx_TransmitReceive_DMA");
          Report_Error(3);
          result = false;
        }
#else
        STM32_LOG("error: full-duplex I2S (simultaneous tx+rx) is not supported on this MCU");
        result = false;
#endif
      }
    }

    return result;
  }

  /// Start to transmit I2S data
  bool beginWriteDMA(I2SSettingsSTM32 settings,
                     void (*readToTransmit)(uint8_t *buffer, uint16_t byteCount,
                                            void *ref) = nullptr) {
    this->use_dma = true;
    this->settings = settings;
    this->hw = settings.hardware_config;
    int buffer_size = hw.buffer_size;
    readToTransmitCB = readToTransmit;
    if (dma_buffer_tx == nullptr) dma_buffer_tx = new byte[buffer_size];
    bool result = true;
    if (!i2s_begin()) {
      return false;
    }
    // start circular dma; HAL expects a sample count, not a byte count
    if (HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t *)dma_buffer_tx,
                             buffer_size / getBytes()) != HAL_OK) {
      STM32_LOG("error HAL_I2S_Transmit_DMA");
      Report_Error(4);
      result = false;
    }
    return result;
  }

  /// Start to receive I2S data
  bool beginReadDMA(I2SSettingsSTM32 settings,
                    void (*writeFromReceive)(uint8_t *buffer,
                                             uint16_t byteCount,
                                             void *ref) = nullptr) {
    this->use_dma = true;
    this->settings = settings;
    this->hw = settings.hardware_config;
    int buffer_size = hw.buffer_size;
    bool result = true;
    writeFromReceiveCB = writeFromReceive;
    if (dma_buffer_rx == nullptr) dma_buffer_rx = new byte[buffer_size];
    if (!i2s_begin()) {
      return false;
    }
    // start circular dma; HAL expects a sample count, not a byte count
    if (HAL_I2S_Receive_DMA(&hi2s3, (uint16_t *)dma_buffer_rx,
                            buffer_size / getBytes()) != HAL_OK) {
      STM32_LOG("error: HAL_I2S_Transmit_DMA");
      Report_Error(5);
      result = false;
    }
    return result;
  }


  /// Start to receive and transmit I2S data
  bool beginReadWriteDMA(I2SSettingsSTM32 settings,
                         void (*readToTransmit)(uint8_t *buffer,
                                                uint16_t byteCount,
                                                void *) = nullptr,
                         void (*writeFromReceive)(uint8_t *buffer,
                                                  uint16_t byteCount,
                                                  void *) = nullptr) {
    this->use_dma = true;
    this->settings = settings;
    this->hw = settings.hardware_config;
    int buffer_size = hw.buffer_size;
    bool result = true;
    readToTransmitCB = readToTransmit;
    writeFromReceiveCB = writeFromReceive;

    if (dma_buffer_tx == nullptr) dma_buffer_tx = new byte[buffer_size];
    if (dma_buffer_rx == nullptr) dma_buffer_rx = new byte[buffer_size];

    if (!i2s_begin()) {
      return false;
    }
    // HAL expects a sample count, not a byte count
#ifdef I2S_HAS_FULLDUPLEX_EXT
    if (HAL_I2SEx_TransmitReceive_DMA(&hi2s3, (uint16_t *)dma_buffer_tx,
                                      (uint16_t *)dma_buffer_rx,
                                      buffer_size / getBytes()) != HAL_OK) {
      STM32_LOG("error HAL_I2SEx_TransmitReceive_DMA");
      Report_Error(6);
      result = false;
    }
#else
    STM32_LOG("error: full-duplex I2S (simultaneous tx+rx) is not supported on this MCU");
    result = false;
#endif
    return result;
  }


  void end() {
    if (use_dma) HAL_I2S_DMAStop(&hi2s3);
    HAL_I2S_DeInit(&hi2s3);
    HAL_I2S_MspDeInit(&hi2s3);
    if (dma_buffer_tx != nullptr) {
      delete[] (dma_buffer_tx);
      dma_buffer_tx = nullptr;
    }
    if (dma_buffer_rx != nullptr) {
      delete[] (dma_buffer_rx);
      dma_buffer_rx = nullptr;
    }
  }

  /// @brief Write method which needs to be called when ansync mode is disabled
  /// @param data
  /// @param bytes
  /// @return
  size_t write(const uint8_t *data, size_t bytes) {
    HAL_StatusTypeDef rc = HAL_OK;
    if (!this->use_dma) {
      int samples = bytes / getBytes();
      HAL_StatusTypeDef rc =
          HAL_I2S_Transmit(&hi2s3, (uint16_t *)data, samples, HAL_MAX_DELAY);
    }
    return rc == HAL_OK ? bytes : 0;
  }

  /// @brief Read method which needs to be called when ansync mode is disabled
  /// @param data
  /// @param bytes
  /// @return
  size_t readBytes(uint8_t *data, size_t bytes) {
    HAL_StatusTypeDef rc = HAL_OK;
    if (!this->use_dma) {
      int samples = bytes / getBytes();
      HAL_StatusTypeDef rc =
          HAL_I2S_Receive(&hi2s3, (uint16_t *)data, samples, HAL_MAX_DELAY);
    }
    return rc == HAL_OK ? bytes : 0;
  }

void STM32_LOG(const char *msg) {
  Serial.println(msg);
  Serial.flush();
}

 protected:
  I2SSettingsSTM32 settings;
  I2S_HandleTypeDef hi2s3;
  byte *dma_buffer_tx = nullptr;
  byte *dma_buffer_rx = nullptr;
  void (*readToTransmitCB)(uint8_t *buffer, uint16_t byteCount, void *ref);
  void (*writeFromReceiveCB)(uint8_t *buffer, uint16_t byteCount, void *ref);
  DMA_HandleTypeDef hdma_i2s3_ext_rx;
  DMA_HandleTypeDef hdma_i2s3_ext_tx;
  HardwareConfig hw;
  bool use_dma = false;

  int getBytes() {
    if (settings.data_format == I2S_DATAFORMAT_16B) return 2;
    if (settings.data_format == I2S_DATAFORMAT_24B) return 4;
    if (settings.data_format == I2S_DATAFORMAT_32B) return 4;
    STM32_LOG("unsuppoted data_format");
    return 2;
  }

  /// @brief Callback for double buffer
  /// @param hi2s
  void cb_TxRxComplete(I2S_HandleTypeDef *hi2s) {
    // second half finished, filling it up again while first  half is playing
    uint8_t *tmp_buffer_tx = (uint8_t *)hi2s->pTxBuffPtr;
    uint8_t *tmp_buffer_rx = (uint8_t *)hi2s->pRxBuffPtr;
    uint16_t buffer_size_tx = hi2s->TxXferSize * 2;  // XferSize is in words
    uint16_t buffer_size_rx = hi2s->RxXferSize * 2;
    if (readToTransmitCB != nullptr)
      readToTransmitCB(&(tmp_buffer_tx[buffer_size_tx / 2]), buffer_size_tx / 2,
                       settings.ref);
    if (writeFromReceiveCB != nullptr)
      writeFromReceiveCB(&(tmp_buffer_rx[buffer_size_rx / 2]),
                         buffer_size_rx / 2, settings.ref);
  }

  /// @brief Callback for double buffer
  /// @param hi2s
  void cb_TxRxHalfComplete(I2S_HandleTypeDef *hi2s) {
    // second half finished, filling it up again while first  half is playing
    uint8_t *tmp_buffer_tx = (uint8_t *)hi2s->pTxBuffPtr;
    uint8_t *tmp_buffer_rx = (uint8_t *)hi2s->pRxBuffPtr;
    uint16_t buffer_size_tx = hi2s->TxXferSize * 2;  // XferSize is in words
    uint16_t buffer_size_rx = hi2s->RxXferSize * 2;
    if (readToTransmitCB != nullptr)
      readToTransmitCB(&(tmp_buffer_tx[0]), buffer_size_tx / 2, settings.ref);
    if (writeFromReceiveCB != nullptr)
      writeFromReceiveCB(&(tmp_buffer_rx[0]), buffer_size_rx / 2, settings.ref);
  }

  /// @brief Callback for DMA interrupt request
  inline void cb_dmaIrqRx() { HAL_DMA_IRQHandler(&hdma_i2s3_ext_rx); }

  /// @brief Callback for DMA interrupt request
  inline void cb_dmaIrqTx() { HAL_DMA_IRQHandler(&hdma_i2s3_ext_tx); }

  /// @brief Callback I2S intitialization
  inline void cb_HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s) {
    cb_i2s_MspInit(hi2s);
  }

  /// @brief Callback I2S de-intitialization
  inline void cb_HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s) {
    cb_i2s_MspDeInit(hi2s);
  }

#ifdef PLLM
  /// Provides the PLLI2S M/N/R divider values that best approximate the
  /// requested sample rate, assuming the 16MHz HSI reference used by boards
  /// that never enable HSE (confirmed via RCC_CR: HSEON=0/HSERDY=0, with
  /// RCC_PLLCFGR.PLLSRC=0 selecting HSI for the main PLL, which PLLI2S
  /// shares). A single fixed PLLN can only ever be exact for one specific
  /// rate: the 44.1kHz family (11025/22050/44100) needs a different PLLM/N
  /// than the 48kHz family (8000/16000/32000/48000/96000/192000) to hit the
  /// target frequency instead of landing tens of percent off (which sounds
  /// like noise, not a mistuned tone) - and using an 8MHz-HSE-derived PLLM
  /// against this board's real 16MHz HSI reference drives the PLLI2S VCO
  /// output to ~858MHz, nearly double the STM32F411's 432MHz maximum: an
  /// out-of-spec, jittery PLL that also sounds like noise even though the
  /// divided-down average frequency happens to look correct on paper.
  /// Values computed by brute-force search over the valid PLLM(2-63)/
  /// PLLN(50-432)/PLLR(2-7)/I2SDIV/ODD space (respecting the 1-2MHz VCO
  /// input and 100-432MHz VCO output limits) for lowest error against each
  /// target, using the real 16MHz HSI reference.
  void getPLLI2S(uint32_t sample_rate, uint32_t &m, uint32_t &n, uint32_t &r) {
    switch (sample_rate) {
      case 8000:   m =  8; n = 128; r = 5; return;
      case 16000:  m = 10; n =  64; r = 5; return;
      case 32000:  m = 10; n = 128; r = 5; return;
      case 48000:  m = 10; n = 192; r = 5; return;
      case 96000:  m = 14; n = 172; r = 2; return;
      case 192000: m = 14; n = 344; r = 2; return;
      default:
        // 11025 / 22050 / 44100 (and fallback for anything else)
        m = PLLM;
        n = PLLN;
        r = PLLR;
        return;
    }
  }
#endif

  /// Starts the i2s processing
  bool i2s_begin() {
    stm32_i2s_is_error = false;
    if (settings.sample_rate == 0) {
      STM32_LOG("sample_rate must not be 0");
      return false;
    }
    STM32_LOG(use_dma ? "use_dma: true" : "use_dma: false");
    stm32_i2s_is_error = false;
    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick.
     */
    // HAL_Init();// Not needed -> called by Arduino
    /* Configure the system clock */
    // SystemClock_Config(); // Not needed -> called by Arduino
    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    if (use_dma) MX_DMA_Init();
    MX_I2S3_Init();
    return !stm32_i2s_is_error;
  }

  /**
   * @brief GPIO Initialization Function for I2S pins
   *
   * @param None
   * @retval None
   */
   void MX_GPIO_Init(void) {

    // Define pins
    for (I2SPin &pin : hw.pins) {
      pin.begin();
    }
  }

  /**
   * Enable DMA controller clock
   */
   void MX_DMA_Init(void) {
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA1_Stream0_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(hw.irq1, 0, 0);
    HAL_NVIC_EnableIRQ(hw.irq1);
    /* DMA1_Stream5_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(hw.irq2, 0, 0);
    HAL_NVIC_EnableIRQ(hw.irq2);
   
  }

  /**
   * @brief I2S3 Initialization Function
   * @param None
   * @retval None
   */
   void MX_I2S3_Init(void) {
    hi2s3.Instance = SPI_INSTANCE_FOR_I2S;
    hi2s3.Init.Mode = settings.mode;
    hi2s3.Init.Standard = settings.standard;
    // I2S_AUDIOFREQ_CORRECTION_DIV is only defined for boards where this
    // exact correction was empirically measured (see stm32-config-i2s.h) -
    // must not silently apply to boards/chips it was never validated on.
#ifdef I2S_AUDIOFREQ_CORRECTION_DIV
    hi2s3.Init.AudioFreq = settings.sample_rate / I2S_AUDIOFREQ_CORRECTION_DIV;
#else
    hi2s3.Init.AudioFreq = settings.sample_rate;
#endif
    hi2s3.Init.DataFormat = settings.data_format;
    hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
    hi2s3.Init.CPOL = I2S_CPOL_LOW;
#ifdef IS_F4
    hi2s3.Init.FullDuplexMode = settings.fullduplexmode;
    hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
#endif
#ifdef IS_H7
    hi2s3.Init.FirstBit = I2S_FIRSTBIT_MSB;
    hi2s3.Init.WSInversion = I2S_WS_INVERSION_DISABLE;
    hi2s3.Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
    hi2s3.Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_DISABLE;
#endif

    if (HAL_I2S_Init(&hi2s3) != HAL_OK) {
      Report_Error(7);
    }
  }

  /**
   * @brief I2S MSP Initialization
   * This function configures the hardware resources used in this example
   * @param hi2s: I2S handle pointer
   * @retval None
   */
   void cb_i2s_MspInit(I2S_HandleTypeDef *hi2s) {
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    /**
     * Initializes the peripherals clock
     */
#ifdef IS_F4
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
#endif
#ifdef IS_F7
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
#endif
#ifdef IS_H7
    if (hi2s->Instance == SPI1)
      PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
    if (hi2s->Instance == SPI2)
      PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
    if (hi2s->Instance == SPI3)
#endif

#ifdef PLLM
    {
      uint32_t pllm, plln, pllr;
      getPLLI2S(settings.sample_rate, pllm, plln, pllr);
      PeriphClkInitStruct.PLLI2S.PLLI2SM = pllm;
      PeriphClkInitStruct.PLLI2S.PLLI2SN = plln;
      PeriphClkInitStruct.PLLI2S.PLLI2SR = pllr;
    }
#endif
#ifdef IS_F7
    // F7's PLLI2S has no M divider (shared with the main PLL) - only N/R
    // are ours to set, see PLLN/PLLR in stm32-config-i2s.h.
    PeriphClkInitStruct.PLLI2S.PLLI2SN = PLLN;
    PeriphClkInitStruct.PLLI2S.PLLI2SR = PLLR;
#endif

#ifdef SPI_CLOCK_SOURCE
    LL_RCC_SetSPIClockSource(SPI_CLOCK_SOURCE);
#else
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
      Report_Error(8);
    }
#endif

    /* Peripheral clock enable */
    if (hi2s->Instance == SPI1)
      __HAL_RCC_SPI1_CLK_ENABLE();
    else if (hi2s->Instance == SPI2)
      __HAL_RCC_SPI2_CLK_ENABLE();
    else if (hi2s->Instance == SPI3)
      __HAL_RCC_SPI3_CLK_ENABLE();
    else
      STM32_LOG("error: invalid hi2s->Instance");

    if (use_dma) {
      /* I2S3 DMA Init */
      if (dma_buffer_rx != nullptr) {
#ifdef IS_F4
        uint32_t ch = hw.rx_channel;
#else
        uint32_t ch = 0; // not used
#endif
        setupDMA(hdma_i2s3_ext_rx, hw.rx_instance, ch,
                 hw.rx_direction, hi2s->Instance);
        __HAL_LINKDMA(hi2s, hdmarx, hdma_i2s3_ext_rx);
      }

      if (dma_buffer_tx != nullptr) {
#ifdef IS_F4
        uint32_t ch = hw.tx_channel;
#else
        uint32_t ch = 0; // not used
#endif
        setupDMA(hdma_i2s3_ext_tx, hw.tx_instance, ch,
                 hw.tx_direction, hi2s->Instance);
        __HAL_LINKDMA(hi2s, hdmatx, hdma_i2s3_ext_tx);
      }
    }
  }

  /**
   * @brief DMA Initialization
   * This function configures and initializes the DMA
   */
  void setupDMA(DMA_HandleTypeDef &dma, DMA_Stream_TypeDef *instance,
                uint32_t channel, uint32_t direction, SPI_TypeDef* port) {
    dma.Instance = instance;
#ifdef IS_F4
    dma.Init.Channel = channel;
#endif
#ifdef IS_H7
    if (direction == DMA_PERIPH_TO_MEMORY) {
      if (port == SPI1) 
        dma.Init.Request = DMA_REQUEST_SPI1_RX;
      if (port == SPI2) 
        dma.Init.Request = DMA_REQUEST_SPI2_RX;
      if (port == SPI3) 
        dma.Init.Request = DMA_REQUEST_SPI3_RX;
    }
    if (direction == DMA_MEMORY_TO_PERIPH) {
      if (port == SPI1) 
        dma.Init.Request = DMA_REQUEST_SPI1_TX;
      if (port == SPI2) 
        dma.Init.Request = DMA_REQUEST_SPI2_TX;
      if (port == SPI3) 
        dma.Init.Request = DMA_REQUEST_SPI3_TX;
    }
#endif

    dma.Init.Direction = direction;
    dma.Init.PeriphInc = DMA_PINC_DISABLE;
    dma.Init.MemInc = DMA_MINC_ENABLE;
    dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dma.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    dma.Init.Mode = DMA_CIRCULAR;
    dma.Init.Priority = DMA_PRIORITY_HIGH;
    dma.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    dma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    dma.Init.MemBurst = DMA_PBURST_INC4;
    dma.Init.PeriphBurst = DMA_PBURST_INC4;
    if (HAL_DMA_Init(&dma) != HAL_OK) {
      Report_Error(9);
    }
  }

  /**
   * @brief I2S MSP De-Initialization
   * This function freeze the hardware resources used in this example
   * @param hi2s: I2S handle pointer
   * @retval None
   */
  virtual void cb_i2s_MspDeInit(I2S_HandleTypeDef *hi2s) {
    if (hi2s->Instance == SPI1)
      __HAL_RCC_SPI1_CLK_DISABLE();
    else if (hi2s->Instance == SPI2)
      __HAL_RCC_SPI2_CLK_DISABLE();
    else if (hi2s->Instance == SPI3)
      __HAL_RCC_SPI3_CLK_DISABLE();
    else
      STM32_LOG("error: invalid hi2s->Instance");

    /* Peripheral clock disable */

    /* I2S3 DMA DeInit */
    HAL_DMA_DeInit(hi2s->hdmarx);
    HAL_DMA_DeInit(hi2s->hdmatx);
  }
};

///// @brief Global I2S Object
//extern Stm32I2sClass I2S;

}  // namespace stm32_i2s

#else
#warning STM_I2S_PINS not defined
#endif
