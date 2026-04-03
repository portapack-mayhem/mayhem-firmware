/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012,2013 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

/**
 * @file    usb.h
 * @brief   USB Driver macros and structures.
 *
 * @addtogroup USB
 * @{
 */

#ifndef _USB_H_
#define _USB_H_

#if HAL_USE_USB || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define USB_RTYPE_DIR_MASK                  0x80
#define USB_RTYPE_DIR_HOST2DEV              0x00
#define USB_RTYPE_DIR_DEV2HOST              0x80
#define USB_RTYPE_TYPE_MASK                 0x60
#define USB_RTYPE_TYPE_STD                  0x00
#define USB_RTYPE_TYPE_CLASS                0x20
#define USB_RTYPE_TYPE_VENDOR               0x40
#define USB_RTYPE_TYPE_RESERVED             0x60
#define USB_RTYPE_RECIPIENT_MASK            0x1F
#define USB_RTYPE_RECIPIENT_DEVICE          0x00
#define USB_RTYPE_RECIPIENT_INTERFACE       0x01
#define USB_RTYPE_RECIPIENT_ENDPOINT        0x02
#define USB_RTYPE_RECIPIENT_OTHER           0x03

#define USB_REQ_GET_STATUS                  0
#define USB_REQ_CLEAR_FEATURE               1
#define USB_REQ_SET_FEATURE                 3
#define USB_REQ_SET_ADDRESS                 5
#define USB_REQ_GET_DESCRIPTOR              6
#define USB_REQ_SET_DESCRIPTOR              7
#define USB_REQ_GET_CONFIGURATION           8
#define USB_REQ_SET_CONFIGURATION           9
#define USB_REQ_GET_INTERFACE               10
#define USB_REQ_SET_INTERFACE               11
#define USB_REQ_SYNCH_FRAME                 12

#define USB_DESCRIPTOR_DEVICE               1
#define USB_DESCRIPTOR_CONFIGURATION        2
#define USB_DESCRIPTOR_STRING               3
#define USB_DESCRIPTOR_INTERFACE            4
#define USB_DESCRIPTOR_ENDPOINT             5
#define USB_DESCRIPTOR_DEVICE_QUALIFIER     6
#define USB_DESCRIPTOR_OTHER_SPEED_CFG      7
#define USB_DESCRIPTOR_INTERFACE_POWER      8
#define USB_DESCRIPTOR_INTERFACE_ASSOCIATION 11

#define USB_FEATURE_ENDPOINT_HALT           0
#define USB_FEATURE_DEVICE_REMOTE_WAKEUP    1
#define USB_FEATURE_TEST_MODE               2

#define USB_EARLY_SET_ADDRESS               0
#define USB_LATE_SET_ADDRESS                1

#define USB_EP0_STATUS_STAGE_SW             0
#define USB_EP0_STATUS_STAGE_HW             1

#define USB_SET_ADDRESS_ACK_SW              0
#define USB_SET_ADDRESS_ACK_HW              1

/**
 * @name    Helper macros for USB descriptors
 * @{
 */
/**
 * @brief   Helper macro for index values into descriptor strings.
 */
#define USB_DESC_INDEX(i) ((uint8_t)(i))

/**
 * @brief   Helper macro for byte values into descriptor strings.
 */
#define USB_DESC_BYTE(b) ((uint8_t)(b))

/**
 * @brief   Helper macro for word values into descriptor strings.
 */
#define USB_DESC_WORD(w)                                                    \
  (uint8_t)((w) & 255),                                                     \
  (uint8_t)(((w) >> 8) & 255)

/**
 * @brief   Helper macro for BCD values into descriptor strings.
 */
#define USB_DESC_BCD(bcd)                                                   \
  (uint8_t)((bcd) & 255),                                                   \
  (uint8_t)(((bcd) >> 8) & 255)

/**
 * @brief   Device Descriptor helper macro.
 */
#define USB_DESC_DEVICE(bcdUSB, bDeviceClass, bDeviceSubClass,              \
                        bDeviceProtocol, bMaxPacketSize, idVendor,          \
                        idProduct, bcdDevice, iManufacturer,                \
                        iProduct, iSerialNumber, bNumConfigurations)        \
  USB_DESC_BYTE(18),                                                        \
  USB_DESC_BYTE(USB_DESCRIPTOR_DEVICE),                                     \
  USB_DESC_BCD(bcdUSB),                                                     \
  USB_DESC_BYTE(bDeviceClass),                                              \
  USB_DESC_BYTE(bDeviceSubClass),                                           \
  USB_DESC_BYTE(bDeviceProtocol),                                           \
  USB_DESC_BYTE(bMaxPacketSize),                                            \
  USB_DESC_WORD(idVendor),                                                  \
  USB_DESC_WORD(idProduct),                                                 \
  USB_DESC_BCD(bcdDevice),                                                  \
  USB_DESC_INDEX(iManufacturer),                                            \
  USB_DESC_INDEX(iProduct),                                                 \
  USB_DESC_INDEX(iSerialNumber),                                            \
  USB_DESC_BYTE(bNumConfigurations)

/**
 * @brief   Configuration Descriptor helper macro.
 */
#define USB_DESC_CONFIGURATION(wTotalLength, bNumInterfaces,                \
                               bConfigurationValue, iConfiguration,         \
                               bmAttributes, bMaxPower)                     \
  USB_DESC_BYTE(9),                                                         \
  USB_DESC_BYTE(USB_DESCRIPTOR_CONFIGURATION),                              \
  USB_DESC_WORD(wTotalLength),                                              \
  USB_DESC_BYTE(bNumInterfaces),                                            \
  USB_DESC_BYTE(bConfigurationValue),                                       \
  USB_DESC_INDEX(iConfiguration),                                           \
  USB_DESC_BYTE(bmAttributes),                                              \
  USB_DESC_BYTE(bMaxPower)

/**
 * @brief   Interface Descriptor helper macro.
 */
#define USB_DESC_INTERFACE(bInterfaceNumber, bAlternateSetting,             \
                           bNumEndpoints, bInterfaceClass,                  \
                           bInterfaceSubClass, bInterfaceProtocol,          \
                           iInterface)                                      \
  USB_DESC_BYTE(9),                                                         \
  USB_DESC_BYTE(USB_DESCRIPTOR_INTERFACE),                                  \
  USB_DESC_BYTE(bInterfaceNumber),                                          \
  USB_DESC_BYTE(bAlternateSetting),                                         \
  USB_DESC_BYTE(bNumEndpoints),                                             \
  USB_DESC_BYTE(bInterfaceClass),                                           \
  USB_DESC_BYTE(bInterfaceSubClass),                                        \
  USB_DESC_BYTE(bInterfaceProtocol),                                        \
  USB_DESC_INDEX(iInterface)

/**
 * @brief   Interface Association Descriptor helper macro.
 */
#define USB_DESC_INTERFACE_ASSOCIATION(bFirstInterface,                     \
                           bInterfaceCount, bFunctionClass,                 \
                           bFunctionSubClass, bFunctionProcotol,            \
                           iInterface)                                      \
  USB_DESC_BYTE(8),                                                         \
  USB_DESC_BYTE(USB_DESCRIPTOR_INTERFACE_ASSOCIATION),                      \
  USB_DESC_BYTE(bFirstInterface),                                           \
  USB_DESC_BYTE(bInterfaceCount),                                           \
  USB_DESC_BYTE(bFunctionClass),                                            \
  USB_DESC_BYTE(bFunctionSubClass),                                         \
  USB_DESC_BYTE(bFunctionProcotol),                                         \
  USB_DESC_INDEX(iInterface)

/**
 * @brief   Endpoint Descriptor helper macro.
 */
#define USB_DESC_ENDPOINT(bEndpointAddress, bmAttributes, wMaxPacketSize,   \
                          bInterval)                                        \
  USB_DESC_BYTE(7),                                                         \
  USB_DESC_BYTE(USB_DESCRIPTOR_ENDPOINT),                                   \
  USB_DESC_BYTE(bEndpointAddress),                                          \
  USB_DESC_BYTE(bmAttributes),                                              \
  USB_DESC_WORD(wMaxPacketSize),                                            \
  USB_DESC_BYTE(bInterval)
/** @} */

/**
 * @name    Endpoint types and settings
 * @{
 */
#define USB_EP_MODE_TYPE                0x0003  /**< Endpoint type mask.    */
#define USB_EP_MODE_TYPE_CTRL           0x0000  /**< Control endpoint.      */
#define USB_EP_MODE_TYPE_ISOC           0x0001  /**< Isochronous endpoint.  */
#define USB_EP_MODE_TYPE_BULK           0x0002  /**< Bulk endpoint.         */
#define USB_EP_MODE_TYPE_INTR           0x0003  /**< Interrupt endpoint.    */
#define USB_EP_MODE_LINEAR_BUFFER       0x0000  /**< Linear buffer mode.    */
#define USB_EP_MODE_QUEUE_BUFFER        0x0010  /**< Queue buffer mode.     */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an USB driver.
 */
typedef struct USBDriver USBDriver;

/**
 * @brief   Type of an endpoint identifier.
 */
typedef uint8_t usbep_t;

/**
 * @brief   Type of a driver state machine possible states.
 */
typedef enum {
  USB_UNINIT   = 0,                     /**< Not initialized.               */
  USB_STOP     = 1,                     /**< Stopped.                       */
  USB_READY    = 2,                     /**< Ready, after bus reset.        */
  USB_SELECTED = 3,                     /**< Address assigned.              */
  USB_ACTIVE   = 4                      /**< Active, configuration selected.*/
} usbstate_t;

/**
 * @brief   Type of an endpoint status.
 */
typedef enum {
  EP_STATUS_DISABLED = 0,               /**< Endpoint not active.           */
  EP_STATUS_STALLED = 1,                /**< Endpoint opened but stalled.   */
  EP_STATUS_ACTIVE = 2                  /**< Active endpoint.               */
} usbepstatus_t;

/**
 * @brief   Type of an endpoint zero state machine states.
 */
typedef enum {
  USB_EP0_WAITING_SETUP,                /**< Waiting for SETUP data.        */
  USB_EP0_TX,                           /**< Transmitting.                  */
  USB_EP0_WAITING_TX0,                  /**< Waiting transmit 0.            */
  USB_EP0_WAITING_STS,                  /**< Waiting status.                */
  USB_EP0_RX,                           /**< Receiving.                     */
  USB_EP0_SENDING_STS,                  /**< Sending status.                */
  USB_EP0_ERROR                         /**< Error, EP0 stalled.            */
} usbep0state_t;

/**
 * @brief   Type of an enumeration of the possible USB events.
 */
typedef enum {
  USB_EVENT_RESET = 0,                  /**< Driver has been reset by host. */
  USB_EVENT_ADDRESS = 1,                /**< Address assigned.              */
  USB_EVENT_CONFIGURED = 2,             /**< Configuration selected.        */
  USB_EVENT_SUSPEND = 3,                /**< Entering suspend mode.         */
  USB_EVENT_WAKEUP = 4,                 /**< Leaving suspend mode.          */
  USB_EVENT_STALLED = 5                 /**< Endpoint 0 error, stalled.     */
} usbevent_t;

/**
 * @brief   Type of an USB descriptor.
 */
typedef struct {
  /**
   * @brief   Descriptor size in unicode characters.
   */
  size_t                        ud_size;
  /**
   * @brief   Pointer to the descriptor.
   */
  const uint8_t                 *ud_string;
} USBDescriptor;

/**
 * @brief   Type of an USB generic notification callback.
 *
 * @param[in] usbp      pointer to the @p USBDriver object triggering the
 *                      callback
 */
typedef void (*usbcallback_t)(USBDriver *usbp);

/**
 * @brief   Type of an USB endpoint callback.
 *
 * @param[in] usbp      pointer to the @p USBDriver object triggering the
 *                      callback
 * @param[in] ep        endpoint number
 */
typedef void (*usbepcallback_t)(USBDriver *usbp, usbep_t ep);

/**
 * @brief   Type of an USB event notification callback.
 *
 * @param[in] usbp      pointer to the @p USBDriver object triggering the
 *                      callback
 * @param[in] event     event type
 */
typedef void (*usbeventcb_t)(USBDriver *usbp, usbevent_t event);

/**
 * @brief   Type of a requests handler callback.
 * @details The request is encoded in the @p usb_setup buffer.
 *
 * @param[in] usbp      pointer to the @p USBDriver object triggering the
 *                      callback
 * @return              The request handling exit code.
 * @retval FALSE        Request not recognized by the handler.
 * @retval TRUE         Request handled.
 */
typedef bool_t (*usbreqhandler_t)(USBDriver *usbp);

/**
 * @brief   Type of an USB descriptor-retrieving callback.
 */
typedef const USBDescriptor * (*usbgetdescriptor_t)(USBDriver *usbp,
                                                    uint8_t dtype,
                                                    uint8_t dindex,
                                                    uint16_t lang);

#include "usb_lld.h"

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/**
 * @name    Macro Functions
 * @{
 */
/**
 * @brief   Returns the driver state.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @return              The driver state.
 *
 * @iclass
 */
#define usbGetDriverStateI(usbp) ((usbp)->state)

/**
 * @brief   Fetches a 16 bits word value from an USB message.
 *
 * @param[in] p         pointer to the 16 bits word
 *
 * @notapi
 */
#define usbFetchWord(p) ((uint16_t)*(p) | ((uint16_t)*((p) + 1) << 8))

/**
 * @brief   Connects the USB device.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @api
 */
#define usbConnectBus(usbp) usb_lld_connect_bus(usbp)

/**
 * @brief   Disconnect the USB device.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @api
 */
#define usbDisconnectBus(usbp) usb_lld_disconnect_bus(usbp)

/**
 * @brief   Returns the current frame number.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @return              The current frame number.
 *
 * @api
 */
#define usbGetFrameNumber(usbp) usb_lld_get_frame_number(usbp)

/**
 * @brief   Returns the status of an IN endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @return              The operation status.
 * @retval FALSE        Endpoint ready.
 * @retval TRUE         Endpoint transmitting.
 *
 * @iclass
 */
#define usbGetTransmitStatusI(usbp, ep) ((usbp)->transmitting & (1 << (ep)))

/**
 * @brief   Returns the status of an OUT endpoint.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @return              The operation status.
 * @retval FALSE        Endpoint ready.
 * @retval TRUE         Endpoint receiving.
 *
 * @iclass
 */
#define usbGetReceiveStatusI(usbp, ep) ((usbp)->receiving & (1 << (ep)))

/**
 * @brief   Returns the exact size of a receive transaction.
 * @details The received size can be different from the size specified in
 *          @p usbStartReceiveI() because the last packet could have a size
 *          different from the expected one.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @return              Received data size.
 *
 * @iclass
 */
#define usbGetReceiveTransactionSizeI(usbp, ep)                             \
  usb_lld_get_transaction_size(usbp, ep)

/**
 * @brief   Request transfer setup.
 * @details This macro is used by the request handling callbacks in order to
 *          prepare a transaction over the endpoint zero.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] buf       pointer to a buffer for the transaction data
 * @param[in] n         number of bytes to be transferred
 * @param[in] endcb     callback to be invoked after the transfer or @p NULL
 *
 * @api
 */
#define usbSetupTransfer(usbp, buf, n, endcb) {                             \
  (usbp)->ep0next  = (buf);                                                 \
  (usbp)->ep0n     = (n);                                                   \
  (usbp)->ep0endcb = (endcb);                                               \
}

/**
 * @brief   Reads a setup packet from the dedicated packet buffer.
 * @details This function must be invoked in the context of the @p setup_cb
 *          callback in order to read the received setup packet.
 * @pre     In order to use this function the endpoint must have been
 *          initialized as a control endpoint.
 * @note    This function can be invoked both in thread and IRQ context.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 * @param[out] buf      buffer where to copy the packet data
 *
 * @special
 */
#define usbReadSetup(usbp, ep, buf) usb_lld_read_setup(usbp, ep, buf)
/** @} */

/**
 * @name    Low Level driver helper macros
 * @{
 */
/**
 * @brief   Common ISR code, usb event callback.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] evt       USB event code
 *
 * @notapi
 */
#define _usb_isr_invoke_event_cb(usbp, evt) {                               \
  if (((usbp)->config->event_cb) != NULL)                                   \
    (usbp)->config->event_cb(usbp, evt);                                    \
}

/**
 * @brief   Common ISR code, SOF callback.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 *
 * @notapi
 */
#define _usb_isr_invoke_sof_cb(usbp) {                                      \
  if (((usbp)->config->sof_cb) != NULL)                                     \
    (usbp)->config->sof_cb(usbp);                                           \
}

/**
 * @brief   Common ISR code, setup packet callback.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
#define _usb_isr_invoke_setup_cb(usbp, ep) {                                \
  (usbp)->epc[ep]->setup_cb(usbp, ep);                                      \
}

/**
 * @brief   Common ISR code, IN endpoint callback.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
#define _usb_isr_invoke_in_cb(usbp, ep) {                                   \
  (usbp)->transmitting &= ~(1 << (ep));                                     \
  (usbp)->epc[ep]->in_cb(usbp, ep);                                         \
}

/**
 * @brief   Common ISR code, OUT endpoint event.
 *
 * @param[in] usbp      pointer to the @p USBDriver object
 * @param[in] ep        endpoint number
 *
 * @notapi
 */
#define _usb_isr_invoke_out_cb(usbp, ep) {                                  \
  (usbp)->receiving &= ~(1 << (ep));                                        \
  (usbp)->epc[ep]->out_cb(usbp, ep);                                        \
}
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void usbInit(void);
  void usbObjectInit(USBDriver *usbp);
  void usbStart(USBDriver *usbp, const USBConfig *config);
  void usbStop(USBDriver *usbp);
  void usbInitEndpointI(USBDriver *usbp, usbep_t ep,
                        const USBEndpointConfig *epcp);
  void usbDisableEndpointsI(USBDriver *usbp);
  void usbReadSetupI(USBDriver *usbp, usbep_t ep, uint8_t *buf);
  void usbPrepareReceive(USBDriver *usbp, usbep_t ep,
                         uint8_t *buf, size_t n);
  void usbPrepareTransmit(USBDriver *usbp, usbep_t ep,
                          const uint8_t *buf, size_t n);
  void usbPrepareQueuedReceive(USBDriver *usbp, usbep_t ep,
                               InputQueue *iqp, size_t n);
  void usbPrepareQueuedTransmit(USBDriver *usbp, usbep_t ep,
                                OutputQueue *oqp, size_t n);
  bool_t usbStartReceiveI(USBDriver *usbp, usbep_t ep);
  bool_t usbStartTransmitI(USBDriver *usbp, usbep_t ep);
  bool_t usbStallReceiveI(USBDriver *usbp, usbep_t ep);
  bool_t usbStallTransmitI(USBDriver *usbp, usbep_t ep);
  void _usb_reset(USBDriver *usbp);
  void _usb_ep0setup(USBDriver *usbp, usbep_t ep);
  void _usb_ep0in(USBDriver *usbp, usbep_t ep);
  void _usb_ep0out(USBDriver *usbp, usbep_t ep);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_USB */

#endif /* _USB_H_ */

/** @} */
