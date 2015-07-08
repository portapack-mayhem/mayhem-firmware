/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    STM32/OTGv1/usb_lld.h
 * @brief   STM32 USB subsystem low level driver header.
 *
 * @addtogroup USB
 * @{
 */

#ifndef _USB_LLD_H_
#define _USB_LLD_H_

#if HAL_USE_USB || defined(__DOXYGEN__)

#include "stm32_otg.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @brief   Maximum endpoint address.
 */
#if !STM32_USB_USE_OTG2 || defined(__DOXYGEN__)
#define USB_MAX_ENDPOINTS                   3
#else
#define USB_MAX_ENDPOINTS                   5
#endif

/**
 * @brief   Status stage handling method.
 */
#define USB_EP0_STATUS_STAGE                USB_EP0_STATUS_STAGE_SW

/**
 * @brief   The address can be changed immediately upon packet reception.
 */
#define USB_SET_ADDRESS_MODE                USB_EARLY_SET_ADDRESS

/**
 * @brief   Method for set address acknowledge.
 */
#define USB_SET_ADDRESS_ACK_HANDLING        USB_SET_ADDRESS_ACK_SW

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   OTG1 driver enable switch.
 * @details If set to @p TRUE the support for OTG_FS is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_USB_USE_OTG1) || defined(__DOXYGEN__)
#define STM32_USB_USE_OTG1                  FALSE
#endif

/**
 * @brief   OTG2 driver enable switch.
 * @details If set to @p TRUE the support for OTG_HS is included.
 * @note    The default is @p TRUE.
 */
#if !defined(STM32_USB_USE_OTG2) || defined(__DOXYGEN__)
#define STM32_USB_USE_OTG2                  FALSE
#endif

/**
 * @brief   OTG1 interrupt priority level setting.
 */
#if !defined(STM32_USB_OTG1_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_USB_OTG1_IRQ_PRIORITY         14
#endif

/**
 * @brief   OTG2 interrupt priority level setting.
 */
#if !defined(STM32_USB_OTG2_IRQ_PRIORITY) || defined(__DOXYGEN__)
#define STM32_USB_OTG2_IRQ_PRIORITY         14
#endif

/**
 * @brief   OTG1 RX shared FIFO size.
 * @note    Must be a multiple of 4.
 */
#if !defined(STM32_USB_OTG1_RX_FIFO_SIZE) || defined(__DOXYGEN__)
#define STM32_USB_OTG1_RX_FIFO_SIZE         512
#endif

/**
 * @brief   OTG2 RX shared FIFO size.
 * @note    Must be a multiple of 4.
 */
#if !defined(STM32_USB_OTG2_RX_FIFO_SIZE) || defined(__DOXYGEN__)
#define STM32_USB_OTG2_RX_FIFO_SIZE         1024
#endif

/**
 * @brief   Dedicated data pump threads priority.
 */
#if !defined(STM32_USB_OTG_THREAD_PRIO) || defined(__DOXYGEN__)
#define STM32_USB_OTG_THREAD_PRIO           LOWPRIO
#endif

/**
 * @brief   Dedicated data pump threads stack size.
 */
#if !defined(STM32_USB_OTG_THREAD_STACK_SIZE) || defined(__DOXYGEN__)
#define STM32_USB_OTG_THREAD_STACK_SIZE     128
#endif

/**
 * @brief   Exception priority level during TXFIFOs operations.
 * @note    Because an undocumented silicon behavior the operation of
 *          copying a packet into a TXFIFO must not be interrupted by
 *          any other operation on the OTG peripheral.
 *          This parameter represents the priority mask during copy
 *          operations. The default value only allows to call USB
 *          functions from callbacks invoked from USB ISR handlers.
 *          If you need to invoke USB functions from other handlers
 *          then raise this priority mast to the same level of the
 *          handler you need to use.
 * @note    The value zero means disabled, when disabled calling USB
 *          functions is only safe from thread level or from USB
 *          callbacks.
 */
#if !defined(STM32_USB_OTGFIFO_FILL_BASEPRI) || defined(__DOXYGEN__)
#define STM32_USB_OTGFIFO_FILL_BASEPRI      0
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if STM32_USB_USE_OTG1 && !STM32_HAS_OTG1
#error "OTG1 not present in the selected device"
#endif

#if STM32_USB_USE_OTG2 && !STM32_HAS_OTG2
#error "OTG2 not present in the selected device"
#endif

#if !STM32_USB_USE_OTG1 && !STM32_USB_USE_OTG2
#error "USB driver activated but no USB peripheral assigned"
#endif

#if STM32_USB_USE_OTG1 &&                                                \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_USB_OTG1_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to OTG1"
#endif

#if STM32_USB_USE_OTG2 &&                                                \
    !CORTEX_IS_VALID_KERNEL_PRIORITY(STM32_USB_OTG2_IRQ_PRIORITY)
#error "Invalid IRQ priority assigned to OTG2"
#endif

#if (STM32_USB_OTG1_RX_FIFO_SIZE & 3) != 0
#error "OTG1 RX FIFO size must be a multiple of 4"
#endif

#if (STM32_USB_OTG2_RX_FIFO_SIZE & 3) != 0
#error "OTG2 RX FIFO size must be a multiple of 4"
#endif

#if defined(STM32F4XX) || defined(STM32F2XX)
#define STM32_USBCLK                        STM32_PLL48CLK
#elif defined(STM32F10X_CL)
#define STM32_USBCLK                        STM32_OTGFSCLK
#else
#error "unsupported STM32 platform for OTG functionality"
#endif

#if STM32_USBCLK != 48000000
#error "the USB OTG driver requires a 48MHz clock"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Peripheral-specific parameters block.
 */
typedef struct {
  uint32_t                      rx_fifo_size;
  uint32_t                      otg_ram_size;
  uint32_t                      num_endpoints;
} stm32_otg_params_t;

/**
 * @brief   Type of an IN endpoint state structure.
 */
typedef struct {
  /**
   * @brief   Buffer mode, queue or linear.
   */
  bool_t                        txqueued;
  /**
   * @brief   Requested transmit transfer size.
   */
  size_t                        txsize;
  /**
   * @brief   Transmitted bytes so far.
   */
  size_t                        txcnt;
  union {
    struct {
      /**
       * @brief   Pointer to the transmission linear buffer.
       */
      const uint8_t             *txbuf;
    } linear;
    struct {
      /**
       * @brief   Pointer to the output queue.
       */
      OutputQueue               *txqueue;
    } queue;
  } mode;
  /**
   * @brief   Total transmit transfer size.
   */
  size_t                        totsize;
} USBInEndpointState;

/**
 * @brief   Type of an OUT endpoint state structure.
 */
typedef struct {
  /**
   * @brief   Buffer mode, queue or linear.
   */
  bool_t                        rxqueued;
  /**
   * @brief   Requested receive transfer size.
   */
  size_t                        rxsize;
  /**
   * @brief   Received bytes so far.
   */
  size_t                        rxcnt;
  union {
    struct {
      /**
       * @brief   Pointer to the receive linear buffer.
       */
      uint8_t                   *rxbuf;
    } linear;
    struct {
      /**
       * @brief   Pointer to the input queue.
       */
      InputQueue               *rxqueue;
    } queue;
  } mode;
  /**
   * @brief   Total transmit transfer size.
   */
  size_t                        totsize;
} USBOutEndpointState;

/**
 * @brief   Type of an USB endpoint configuration structure.
 * @note    Platform specific restrictions may apply to endpoints.
 */
typedef struct {
  /**
   * @brief   Type and mode of the endpoint.
   */
  uint32_t                      ep_mode;
  /**
   * @brief   Setup packet notification callback.
   * @details This callback is invoked when a setup packet has been
   *          received.
   * @post    The application must immediately call @p usbReadPacket() in
   *          order to access the received packet.
   * @note    This field is only valid for @p USB_EP_MODE_TYPE_CTRL
   *          endpoints, it should be set to @p NULL for other endpoint
   *          types.
   */
  usbepcallback_t               setup_cb;
  /**
   * @brief   IN endpoint notification callback.
   * @details This field must be set to @p NULL if the IN endpoint is not
   *          used.
   */
  usbepcallback_t               in_cb;
  /**
   * @brief   OUT endpoint notification callback.
   * @details This field must be set to @p NULL if the OUT endpoint is not
   *          used.
   */
  usbepcallback_t               out_cb;
  /**
   * @brief   IN endpoint maximum packet size.
   * @details This field must be set to zero if the IN endpoint is not
   *          used.
   */
  uint16_t                      in_maxsize;
  /**
   * @brief   OUT endpoint maximum packet size.
   * @details This field must be set to zero if the OUT endpoint is not
   *          used.
   */
  uint16_t                      out_maxsize;
  /**
   * @brief   @p USBEndpointState associated to the IN endpoint.
   * @details This structure maintains the state of the IN endpoint.
   */
  USBInEndpointState            *in_state;
  /**
   * @brief   @p USBEndpointState associated to the OUT endpoint.
   * @details This structure maintains the state of the OUT endpoint.
   */
  USBOutEndpointState           *out_state;
  /* End of the mandatory fields.*/
  /**
   * @brief   Determines the space allocated for the TXFIFO as multiples of
   *          the packet size (@p in_maxsize). Note that zero is interpreted
   *          as one for simplicity and robustness.
   */
  uint16_t                      in_multiplier;
  /**
   * @brief   Pointer to a buffer for setup packets.
   * @details Setup packets require a dedicated 8-bytes buffer, set this
   *          field to @p NULL for non-control endpoints.
   */
  uint8_t                       *setup_buf;
} USBEndpointConfig;

/**
 * @brief   Type of an USB driver configuration structure.
 */
typedef struct {
  /**
   * @brief   USB events callback.
   * @details This callback is invoked when an USB driver event is registered.
   */
  usbeventcb_t                  event_cb;
  /**
   * @brief   Device GET_DESCRIPTOR request callback.
   * @note    This callback is mandatory and cannot be set to @p NULL.
   */
  usbgetdescriptor_t            get_descriptor_cb;
  /**
   * @brief   Requests hook callback.
   * @details This hook allows to be notified of standard requests or to
   *          handle non standard requests.
   */
  usbreqhandler_t               requests_hook_cb;
  /**
   * @brief   Start Of Frame callback.
   */
  usbcallback_t                 sof_cb;
  /* End of the mandatory fields.*/
} USBConfig;

/**
 * @brief   Structure representing an USB driver.
 */
struct USBDriver {
  /**
   * @brief   Driver state.
   */
  usbstate_t                    state;
  /**
   * @brief   Current configuration data.
   */
  const USBConfig               *config;
  /**
   * @brief   Bit map of the transmitting IN endpoints.
   */
  uint16_t                      transmitting;
  /**
   * @brief   Bit map of the receiving OUT endpoints.
   */
  uint16_t                      receiving;
  /**
   * @brief   Active endpoints configurations.
   */
  const USBEndpointConfig       *epc[USB_MAX_ENDPOINTS + 1];
  /**
   * @brief   Fields available to user, it can be used to associate an
   *          application-defined handler to an IN endpoint.
   * @note    The base index is one, the endpoint zero does not have a
   *          reserved element in this array.
   */
  void                          *in_params[USB_MAX_ENDPOINTS];
  /**
   * @brief   Fields available to user, it can be used to associate an
   *          application-defined handler to an OUT endpoint.
   * @note    The base index is one, the endpoint zero does not have a
   *          reserved element in this array.
   */
  void                          *out_params[USB_MAX_ENDPOINTS];
  /**
   * @brief   Endpoint 0 state.
   */
  usbep0state_t                 ep0state;
  /**
   * @brief   Next position in the buffer to be transferred through endpoint 0.
   */
  uint8_t                       *ep0next;
  /**
   * @brief   Number of bytes yet to be transferred through endpoint 0.
   */
  size_t                        ep0n;
  /**
   * @brief   Endpoint 0 end transaction callback.
   */
  usbcallback_t                 ep0endcb;
  /**
   * @brief   Setup packet buffer.
   */
  uint8_t                       setup[8];
  /**
   * @brief   Current USB device status.
   */
  uint16_t                      status;
  /**
   * @brief   Assigned USB address.
   */
  uint8_t                       address;
  /**
   * @brief   Current USB device configuration.
   */
  uint8_t                       configuration;
#if defined(USB_DRIVER_EXT_FIELDS)
  USB_DRIVER_EXT_FIELDS
#endif
  /* End of the mandatory fields.*/
  /**
   * @brief   Pointer to the OTG peripheral associated to this driver.
   */
  stm32_otg_t                   *otg;
  /**
   * @brief   Peripheral-specific parameters.
   */
  const stm32_otg_params_t      *otgparams;
  /**
   * @brief   Pointer to the next address in the packet memory.
   */
  uint32_t                      pmnext;
  /**
   * @brief   Mask of TXFIFOs to be filled by the pump thread.
   */
  uint32_t                      txpending;
  /**
   * @brief   Pointer to the thread.
   */
  Thread                        *thd_ptr;
  /**
   * @brief   Pointer to the thread when it is sleeping or @p NULL.
   */
  Thread                        *thd_wait;
  /**
   * @brief   Working area for the dedicated data pump thread;
   */
  WORKING_AREA(wa_pump, STM32_USB_OTG_THREAD_STACK_SIZE);
};

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @brief   Returns the exact size of a receive transaction.
 * @details The received size can be different from the size specified in
 *          @p usbStartReceiveI() because the last packet could have a size
 *          different from the expected one.
 * @pre     The OUT endpoint must have been configured in transaction mode
 *          in order to use this function.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @return              Received data size.
 *
 * @notapi
 */
#define usb_lld_get_transaction_size(usbp, ep)                              \
  ((usbp)->epc[ep]->out_state->rxcnt)

/**
 * @brief   Connects the USB device.
 *
 * @api
 */
#define usb_lld_connect_bus(usbp) ((usbp)->otg->GCCFG |= GCCFG_VBUSBSEN)

/**
 * @brief   Disconnect the USB device.
 *
 * @api
 */
#define usb_lld_disconnect_bus(usbp) ((usbp)->otg->GCCFG &= ~GCCFG_VBUSBSEN)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_USB_USE_OTG1 && !defined(__DOXYGEN__)
extern USBDriver USBD1;
#endif

#if STM32_USB_USE_OTG2 && !defined(__DOXYGEN__)
extern USBDriver USBD2;
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void usb_lld_init(void);
  void usb_lld_start(USBDriver *usbp);
  void usb_lld_stop(USBDriver *usbp);
  void usb_lld_reset(USBDriver *usbp);
  void usb_lld_set_address(USBDriver *usbp);
  void usb_lld_init_endpoint(USBDriver *usbp, usbep_t ep);
  void usb_lld_disable_endpoints(USBDriver *usbp);
  usbepstatus_t usb_lld_get_status_in(USBDriver *usbp, usbep_t ep);
  usbepstatus_t usb_lld_get_status_out(USBDriver *usbp, usbep_t ep);
  void usb_lld_read_setup(USBDriver *usbp, usbep_t ep, uint8_t *buf);
  void usb_lld_prepare_receive(USBDriver *usbp, usbep_t ep);
  void usb_lld_prepare_transmit(USBDriver *usbp, usbep_t ep);
  void usb_lld_start_out(USBDriver *usbp, usbep_t ep);
  void usb_lld_start_in(USBDriver *usbp, usbep_t ep);
  void usb_lld_stall_out(USBDriver *usbp, usbep_t ep);
  void usb_lld_stall_in(USBDriver *usbp, usbep_t ep);
  void usb_lld_clear_out(USBDriver *usbp, usbep_t ep);
  void usb_lld_clear_in(USBDriver *usbp, usbep_t ep);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_USB */

#endif /* _USB_LLD_H_ */

/** @} */
