#ifndef __USB_UPGRADE_H__
#define __USB_UPGRADE_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define PERIPH_BASE             ((uint32_t)0xB8800000)
#define SCTRL_BASE              PERIPH_BASE
#define USB0_BASE               (PERIPH_BASE + 0x44000)
#define USB1_BASE               (PERIPH_BASE + 0x50000)

#define USB_EP_DESC_OUT         0x00
#define USB_EP_DESC_IN          0x80
#define USB_EP_DESC_NUM_M       0x0f

#define USB_MSC_SUBCLASS_SCSI   0x6
#define USB_MSC_PROTO_BULKONLY  0x50
#define USB_CUSTOM_SUBCLASS_SCSI   0x0
#define USB_CUSTOM_PROTO_BULKONLY  0x0

#define DATA_IN_EP_MAX_SIZE     512
#define DATA_OUT_EP_MAX_SIZE    512

#define EP0_MAX_PACKET_SIZE     64
#define DEV_ADDR_PENDING        0x80000000

#define DATA_IN_ENDPOINT        1
#define DATA_OUT_ENDPOINT       2

/******************************************************************************
 *
 * Assorted language IDs from the document "USB_LANGIDs.pdf" provided by the
 * USB Implementers' Forum (Version 1.0).
 *
 ******************************************************************************/
#define USB_LANG_CHINESE_PRC    (0x0804)       /* Chinese (PRC) */
#define USB_LANG_CHINESE_TAIWAN (0x0404)       /* Chinese (Taiwan) */
#define USB_LANG_EN_US          (0x0409)       /* English (United States) */
#define USB_LANG_EN_UK          (0x0809)       /* English (United Kingdom) */
#define USB_LANG_EN_AUS         (0x0C09)       /* English (Australia) */
#define USB_LANG_EN_CA          (0x1009)       /* English (Canada) */
#define USB_LANG_EN_NZ          (0x1409)       /* English (New Zealand) */
#define USB_LANG_FRENCH         (0x040C)       /* French (Standard) */
#define USB_LANG_GERMAN         (0x0407)       /* German (Standard) */
#define USB_LANG_HINDI          (0x0439)       /* Hindi */
#define USB_LANG_ITALIAN        (0x0410)       /* Italian (Standard) */
#define USB_LANG_JAPANESE       (0x0411)       /* Japanese */
#define USB_LANG_KOREAN         (0x0412)       /* Korean */
#define USB_LANG_ES_TRAD        (0x040A)       /* Spanish (Traditional) */
#define USB_LANG_ES_MODERN      (0x0C0A)       /* Spanish (Modern) */
#define USB_LANG_SWAHILI        (0x0441)       /* Swahili (Kenya) */
#define USB_LANG_URDU_IN        (0x0820)       /* Urdu (India) */
#define USB_LANG_URDU_PK        (0x0420)       /* Urdu (Pakistan) */

#define USBShort(usValue)       ((usValue) & 0xff), ((usValue) >> 8)

#define USB_EP_ATTR_BULK        0x02
#define USB_EP_ATTR_INTR        0x03
#define USB_EP_ATTR_TYPE_MASK   0x03

#define USB_CONF_ATTR_PWR_M     0xC0

#define USB_CONF_ATTR_SELF_PWR  0xC0
#define USB_CONF_ATTR_BUS_PWR   0x80
#define USB_CONF_ATTR_RWAKE     0xA0

#define USB_DTYPE_DEVICE        1
#define USB_DTYPE_CONFIGURATION 2
#define USB_DTYPE_STRING        3
#define USB_DTYPE_INTERFACE     4
#define USB_DTYPE_ENDPOINT      5
#define USB_DTYPE_DEVICE_QUAL   6
#define USB_DTYPE_OSPEED_CONF   7
#define USB_DTYPE_INTERFACE_PWR 8
#define USB_DTYPE_OTG           9
#define USB_DTYPE_INTERFACE_ASC 11
#define USB_DTYPE_CS_INTERFACE  36

#define USB_CLASS_DEVICE        0x00
#define USB_CLASS_AUDIO         0x01
#define USB_CLASS_CDC           0x02
#define USB_CLASS_HID           0x03
#define USB_CLASS_PHYSICAL      0x05
#define USB_CLASS_IMAGE         0x06
#define USB_CLASS_PRINTER       0x07
#define USB_CLASS_MASS_STORAGE  0x08
#define USB_CLASS_HUB           0x09
#define USB_CLASS_CDC_DATA      0x0a
#define USB_CLASS_SMART_CARD    0x0b
#define USB_CLASS_SECURITY      0x0d
#define USB_CLASS_VIDEO         0x0e
#define USB_CLASS_HEALTHCARE    0x0f
#define USB_CLASS_DIAG_DEVICE   0xdc
#define USB_CLASS_WIRELESS      0xe0
#define USB_CLASS_MISC          0xef
#define USB_CLASS_APP_SPECIFIC  0xfe
#define USB_CLASS_VEND_SPECIFIC 0xff
#define USB_CLASS_EVENTS        (0xffffffffU)

#define USB_O_FADDR             0x00000000  // USB Device Functional Address
#define USB_O_POWER             0x00000001  // USB Power
#define USB_O_TXIS              0x00000002  // USB Transmit Interrupt Status
#define USB_O_RXIS              0x00000004  // USB Receive Interrupt Status
#define USB_O_TXIE              0x00000006  // USB Transmit Interrupt Enable
#define USB_O_RXIE              0x00000008  // USB Receive Interrupt Enable
#define USB_O_IS                0x0000000A  // USB General Interrupt Status
#define USB_O_IE                0x0000000B  // USB Interrupt Enable
#define USB_O_FRAME             0x0000000C  // USB Frame Value
#define USB_O_EPIDX             0x0000000E  // USB Endpoint Index
#define USB_O_TEST              0x0000000F  // USB Test Mode
#define USB_O_FIFO0             0x00000020  // USB FIFO Endpoint 0
#define USB_O_FIFO1             0x00000024  // USB FIFO Endpoint 1
#define USB_O_FIFO2             0x00000028  // USB FIFO Endpoint 2
#define USB_O_FIFO3             0x0000002C  // USB FIFO Endpoint 3
#define USB_O_FIFO4             0x00000030  // USB FIFO Endpoint 4
#define USB_O_FIFO5             0x00000034  // USB FIFO Endpoint 5
#define USB_O_FIFO6             0x00000038  // USB FIFO Endpoint 6
#define USB_O_FIFO7             0x0000003C  // USB FIFO Endpoint 7
#define USB_O_FIFO8             0x00000040  // USB FIFO Endpoint 8
#define USB_O_FIFO9             0x00000044  // USB FIFO Endpoint 9
#define USB_O_FIFO10            0x00000048  // USB FIFO Endpoint 10
#define USB_O_FIFO11            0x0000004C  // USB FIFO Endpoint 11
#define USB_O_FIFO12            0x00000050  // USB FIFO Endpoint 12
#define USB_O_FIFO13            0x00000054  // USB FIFO Endpoint 13
#define USB_O_FIFO14            0x00000058  // USB FIFO Endpoint 14
#define USB_O_FIFO15            0x0000005C  // USB FIFO Endpoint 15
#define USB_O_DEVCTL            0x00000060  // USB Device Control
#define USB_O_TXFIFOSZ          0x00000062  // USB Transmit Dynamic FIFO Sizing
#define USB_O_RXFIFOSZ          0x00000063  // USB Receive Dynamic FIFO Sizing
#define USB_O_TXFIFOADD         0x00000064  // USB Transmit FIFO Start Address
#define USB_O_RXFIFOADD         0x00000066  // USB Receive FIFO Start Address
#define USB_O_CONTIM            0x0000007A  // USB Connect Timing
#define USB_O_VPLEN             0x0000007B  // USB OTG VBUS Pulse Timing
#define USB_O_FSEOF             0x0000007D  // USB Full-Speed Last Transaction
                                            // to End of Frame Timing
#define USB_O_LSEOF             0x0000007E  // USB Low-Speed Last Transaction
                                            // to End of Frame Timing
#define USB_O_TXFUNCADDR0       0x00000080  // USB Transmit Functional Address
                                            // Endpoint 0
#define USB_O_TXHUBADDR0        0x00000082  // USB Transmit Hub Address
                                            // Endpoint 0
#define USB_O_TXHUBPORT0        0x00000083  // USB Transmit Hub Port Endpoint 0
#define USB_O_TXFUNCADDR1       0x00000088  // USB Transmit Functional Address
                                            // Endpoint 1
#define USB_O_TXHUBADDR1        0x0000008A  // USB Transmit Hub Address
                                            // Endpoint 1
#define USB_O_TXHUBPORT1        0x0000008B  // USB Transmit Hub Port Endpoint 1
#define USB_O_RXFUNCADDR1       0x0000008C  // USB Receive Functional Address
                                            // Endpoint 1
#define USB_O_RXHUBADDR1        0x0000008E  // USB Receive Hub Address Endpoint
                                            // 1
#define USB_O_RXHUBPORT1        0x0000008F  // USB Receive Hub Port Endpoint 1

#define USB_O_CSRL0             0x00000102  // USB Control and Status Endpoint
                                            // 0 Low
#define USB_O_CSRH0             0x00000103  // USB Control and Status Endpoint
                                            // 0 High
#define USB_O_COUNT0            0x00000108  // USB Receive Byte Count Endpoint
                                            // 0
#define USB_O_TYPE0             0x0000010A  // USB Type Endpoint 0
#define USB_O_NAKLMT            0x0000010B  // USB NAK Limit
#define USB_O_TXMAXP1           0x00000110  // USB Maximum Transmit Data
                                            // Endpoint 1
#define USB_O_TXCSRL1           0x00000112  // USB Transmit Control and Status
                                            // Endpoint 1 Low
#define USB_O_TXCSRH1           0x00000113  // USB Transmit Control and Status
                                            // Endpoint 1 High
#define USB_O_RXMAXP1           0x00000114  // USB Maximum Receive Data
                                            // Endpoint 1
#define USB_O_RXCSRL1           0x00000116  // USB Receive Control and Status
                                            // Endpoint 1 Low
#define USB_O_RXCSRH1           0x00000117  // USB Receive Control and Status
                                            // Endpoint 1 High
#define USB_O_RXCOUNT1          0x00000118  // USB Receive Byte Count Endpoint
                                            // 1
#define USB_O_TXTYPE1           0x0000011A  // USB Host Transmit Configure Type
                                            // Endpoint 1
#define USB_O_TXINTERVAL1       0x0000011B  // USB Host Transmit Interval
                                            // Endpoint 1
#define USB_O_RXTYPE1           0x0000011C  // USB Host Configure Receive Type
                                            // Endpoint 1
#define USB_O_RXINTERVAL1       0x0000011D  // USB Host Receive Polling
                                            // Interval Endpoint 1
#define USB_O_RQPKTCOUNT1       0x00000304  // USB Request Packet Count in
                                            // Block Transfer Endpoint 1
#define USB_O_RXDPKTBUFDIS      0x00000340  // USB Receive Double Packet Buffer
                                            // Disable
#define USB_O_TXDPKTBUFDIS      0x00000342  // USB Transmit Double Packet
                                            // Buffer Disable

#define USB_O_DMA_BASE          0x00000200
#define USB_O_DMA_CTRL          0x04
#define USB_O_UTMI              0x00000380

#define SCTRL_O_CHIP            0x00000000
#define SCTRL_O_SCRATCH         0x00000004
#define SCTRL_O_NBDBG           0x00000024
#define SCTRL_O_INTPOL0         0x00000028
#define SCTRL_O_INTPOL1         0x0000002C
#define SCTRL_O_CPUIS0          0x00000030
#define SCTRL_O_CPUIS1          0x00000034
#define SCTRL_O_CPUIE0          0x00000038
#define SCTRL_O_CPUIE1          0x0000003C

#define SCTRL_O_DEVCLK0         0x00000060
#define SCTRL_O_DEVCLK1         0x00000064

#define SCTRL_DEVCLKRST1_USB0   ((uint32_t)0x01000000)  /* bit 24*/
#define SCTRL_DEVCLKRST1_USB1   ((uint32_t)0x02000000)  /* bit 25*/

#define SCTRL_DEVRST0_USB0      ((uint32_t)0x10000000)  /* bit 28*/
#define SCTRL_DEVRST1_USB1      ((uint32_t)0x20000000)  /* bit 29*/

#define SCTRL_O_DEVRST0         0x00000080
#define SCTRL_O_DEVRST1         0x00000084

#define USB_POWER_ISOUP         0x00000080  // Isochronous Update
#define USB_POWER_HS_MODE       0x00000010  // High speed mode
#define USB_POWER_SOFTCONN      0x00000040  // Soft Connect/Disconnect
#define USB_POWER_RESET         0x00000008  // RESET Signaling
#define USB_POWER_RESUME        0x00000004  // RESUME Signaling
#define USB_POWER_SUSPEND       0x00000002  // SUSPEND Mode
#define USB_POWER_PWRDNPHY      0x00000001  // Power Down PHY

#define USB_INTCTRL_ALL         0x000003FF  // All control interrupt sources
#define USB_INTCTRL_STATUS      0x000000FF  // Status Interrupts
#define USB_INTCTRL_VBUS_ERR    0x00000080  // VBUS Error
#define USB_INTCTRL_SESSION     0x00000040  // Session Start Detected
#define USB_INTCTRL_SESSION_END 0x00000040  // Session End Detected
#define USB_INTCTRL_DISCONNECT  0x00000020  // Disconnect Detected
#define USB_INTCTRL_CONNECT     0x00000010  // Device Connect Detected
#define USB_INTCTRL_SOF         0x00000008  // Start of Frame Detected
#define USB_INTCTRL_BABBLE      0x00000004  // Babble signaled
#define USB_INTCTRL_RESET       0x00000004  // Reset signaled
#define USB_INTCTRL_RESUME      0x00000002  // Resume detected
#define USB_INTCTRL_SUSPEND     0x00000001  // Suspend detected
#define USB_INTCTRL_MODE_DETECT 0x00000200  // Mode value valid
#define USB_INTCTRL_POWER_FAULT 0x00000100  // Power Fault detected

#define USB_INTEP_0             0x00000001  // Endpoint 0 Interrupt

#define USB_EP_0                0x00000000  // Endpoint 0
#define USB_EP_1                0x00000010  // Endpoint 1
#define USB_EP_2                0x00000020  // Endpoint 2
#define USB_EP_3                0x00000030  // Endpoint 3
#define USB_EP_4                0x00000040  // Endpoint 4
#define USB_EP_5                0x00000050  // Endpoint 5
#define USB_EP_6                0x00000060  // Endpoint 6
#define USB_EP_7                0x00000070  // Endpoint 7
#define USB_EP_8                0x00000080  // Endpoint 8
#define USB_EP_9                0x00000090  // Endpoint 9
#define USB_EP_10               0x000000A0  // Endpoint 10
#define USB_EP_11               0x000000B0  // Endpoint 11
#define USB_EP_12               0x000000C0  // Endpoint 12
#define USB_EP_13               0x000000D0  // Endpoint 13
#define USB_EP_14               0x000000E0  // Endpoint 14
#define USB_EP_15               0x000000F0  // Endpoint 15
#define NUM_USB_EP              16          // Number of supported endpoints Biju

//*****************************************************************************
//
// These macros allow conversion between 0-based endpoint indices and the
// USB_EP_x values required when calling various USB APIs.
//
//*****************************************************************************
#define INDEX_TO_USB_EP(x)      ((x) << 4)
#define USB_EP_TO_INDEX(x)      ((x) >> 4)

//*****************************************************************************
//
// The following are values that can be passed to USBFIFOConfigSet() as the
// ulFIFOSize parameter.
//
//*****************************************************************************
#define USB_FIFO_SZ_8           0x00000000  // 8 byte FIFO
#define USB_FIFO_SZ_16          0x00000001  // 16 byte FIFO
#define USB_FIFO_SZ_32          0x00000002  // 32 byte FIFO
#define USB_FIFO_SZ_64          0x00000003  // 64 byte FIFO
#define USB_FIFO_SZ_128         0x00000004  // 128 byte FIFO
#define USB_FIFO_SZ_256         0x00000005  // 256 byte FIFO
#define USB_FIFO_SZ_512         0x00000006  // 512 byte FIFO
#define USB_FIFO_SZ_1024        0x00000007  // 1024 byte FIFO
#define USB_FIFO_SZ_2048        0x00000008  // 2048 byte FIFO
#define USB_FIFO_SZ_4096        0x00000009  // 4096 byte FIFO
#define USB_FIFO_SZ_8192        0x00000010  // Biju

#define USB_FIFO_SZ_8_DB        0x00000011  // 8 byte double buffered FIFO
                                            // (occupying 16 bytes)
#define USB_FIFO_SZ_16_DB       0x00000012  // 16 byte double buffered FIFO
                                            // (occupying 32 bytes)
#define USB_FIFO_SZ_32_DB       0x00000013  // 32 byte double buffered FIFO
                                            // (occupying 64 bytes)
#define USB_FIFO_SZ_64_DB       0x00000014  // 64 byte double buffered FIFO
                                            // (occupying 128 bytes)
#define USB_FIFO_SZ_128_DB      0x00000015  // 128 byte double buffered FIFO
                                            // (occupying 256 bytes)
#define USB_FIFO_SZ_256_DB      0x00000016  // 256 byte double buffered FIFO
                                            // (occupying 512 bytes)
#define USB_FIFO_SZ_512_DB      0x00000017  // 512 byte double buffered FIFO
                                            // (occupying 1024 bytes)
#define USB_FIFO_SZ_1024_DB     0x00000018  // 1024 byte double buffered FIFO
                                            // (occupying 2048 bytes)
#define USB_FIFO_SZ_2048_DB     0x00000019  // 2048 byte double buffered FIFO
                                            // (occupying 4096 bytes)
#define USB_FIFO_SZ_4096_DB     0x00000020  //Biju


#define USB_INTEP_RX_SHIFT      16

#define HWREGB(x) (*((volatile unsigned char *)(x)))
#define HWREGH(x) (*((volatile unsigned short *)(x)))
#define HWREG(x) (*((volatile unsigned long *)(x)))

//*****************************************************************************
//
// The following are values that are returned from USBEndpointStatus().  The
// USB_HOST_* values are used when the USB controller is in host mode and the
// USB_DEV_* values are used when the USB controller is in device mode.
//
//*****************************************************************************
#define USB_HOST_IN_PID_ERROR   0x01000000  // Stall on this endpoint received
#define USB_HOST_IN_NOT_COMP    0x00100000  // Device failed to respond
#define USB_HOST_IN_STALL       0x00400000  // Stall on this endpoint received
#define USB_HOST_IN_DATA_ERROR  0x00080000  // CRC or bit-stuff error
                                            // (ISOC Mode)
#define USB_HOST_IN_NAK_TO      0x00080000  // NAK received for more than the
                                            // specified timeout period
#define USB_HOST_IN_ERROR       0x00040000  // Failed to communicate with a
                                            // device
#define USB_HOST_IN_FIFO_FULL   0x00020000  // RX FIFO full
#define USB_HOST_IN_PKTRDY      0x00010000  // Data packet ready
#define USB_HOST_OUT_NAK_TO     0x00000080  // NAK received for more than the
                                            // specified timeout period
#define USB_HOST_OUT_NOT_COMP   0x00000080  // No response from device
                                            // (ISOC mode)
#define USB_HOST_OUT_STALL      0x00000020  // Stall on this endpoint received
#define USB_HOST_OUT_ERROR      0x00000004  // Failed to communicate with a
                                            // device
#define USB_HOST_OUT_FIFO_NE    0x00000002  // TX FIFO is not empty
#define USB_HOST_OUT_PKTPEND    0x00000001  // Transmit still being transmitted
#define USB_HOST_EP0_NAK_TO     0x00000080  // NAK received for more than the
                                            // specified timeout period
#define USB_HOST_EP0_STATUS     0x00000040  // This was a status packet
#define USB_HOST_EP0_ERROR      0x00000010  // Failed to communicate with a
                                            // device
#define USB_HOST_EP0_RX_STALL   0x00000004  // Stall on this endpoint received
#define USB_HOST_EP0_RXPKTRDY   0x00000001  // Receive data packet ready
#define USB_DEV_RX_SENT_STALL   0x00400000  // Stall was sent on this endpoint
#define USB_DEV_RX_DATA_ERROR   0x00080000  // CRC error on the data
#define USB_DEV_RX_OVERRUN      0x00040000  // OUT packet was not loaded due to
                                            // a full FIFO
#define USB_DEV_RX_FIFO_FULL    0x00020000  // RX FIFO full
#define USB_DEV_RX_PKT_RDY      0x00010000  // Data packet ready
#define USB_DEV_TX_NOT_COMP     0x00000080  // Large packet split up, more data
                                            // to come
#define USB_DEV_TX_SENT_STALL   0x00000020  // Stall was sent on this endpoint
#define USB_DEV_TX_UNDERRUN     0x00000004  // IN received with no data ready
#define USB_DEV_TX_FIFO_NE      0x00000002  // The TX FIFO is not empty
#define USB_DEV_TX_TXPKTRDY     0x00000001  // Transmit still being transmitted
#define USB_DEV_EP0_SETUP_END   0x00000010  // Control transaction ended before
                                            // Data End seen
#define USB_DEV_EP0_SENT_STALL  0x00000004  // Stall was sent on this endpoint
#define USB_DEV_EP0_IN_PKTPEND  0x00000002  // Transmit data packet pending
#define USB_DEV_EP0_OUT_PKTRDY  0x00000001  // Receive data packet ready

#define EP_OFFSET(Endpoint)     (Endpoint - 0x10)
#define USB_RX_EPSTATUS_SHIFT   16

//*****************************************************************************
//
// The following are values that can be passed to USBHostEndpointConfig() and
// USBDevEndpointConfigSet() as the ulFlags parameter.
//
//*****************************************************************************
#define USB_EP_AUTO_SET         0x00000001  // Auto set feature enabled
#define USB_EP_AUTO_REQUEST     0x00000002  // Auto request feature enabled
#define USB_EP_AUTO_CLEAR       0x00000004  // Auto clear feature enabled
#define USB_EP_DMA_MODE_0       0x00000008  // Enable DMA access using mode 0
#define USB_EP_DMA_MODE_1       0x00000010  // Enable DMA access using mode 1
#define USB_EP_MODE_ISOC        0x00000000  // Isochronous endpoint
#define USB_EP_MODE_BULK        0x00000100  // Bulk endpoint
#define USB_EP_MODE_INT         0x00000200  // Interrupt endpoint
#define USB_EP_MODE_CTRL        0x00000300  // Control endpoint
#define USB_EP_MODE_MASK        0x00000300  // Mode Mask
#define USB_EP_SPEED_LOW        0x00000000  // Low Speed
#define USB_EP_SPEED_FULL       0x00001000  // Full Speed
#define USB_EP_SPEED_HIGH       0x00004000  //High Speed
#define USB_EP_HOST_IN          0x00000000  // Host IN endpoint
#define USB_EP_HOST_OUT         0x00002000  // Host OUT endpoint
#define USB_EP_DEV_IN           0x00002000  // Device IN endpoint
#define USB_EP_DEV_OUT          0x00000000  // Device OUT endpoint

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_O_CSRL0 register.
//
//*****************************************************************************
#define USB_CSRL0_NAKTO         0x00000080  // NAK Timeout
#define USB_CSRL0_SETENDC       0x00000080  // Setup End Clear
#define USB_CSRL0_STATUS        0x00000040  // STATUS Packet
#define USB_CSRL0_RXRDYC        0x00000040  // RXRDY Clear
#define USB_CSRL0_REQPKT        0x00000020  // Request Packet
#define USB_CSRL0_STALL         0x00000020  // Send Stall
#define USB_CSRL0_SETEND        0x00000010  // Setup End
#define USB_CSRL0_ERROR         0x00000010  // Error
#define USB_CSRL0_DATAEND       0x00000008  // Data End
#define USB_CSRL0_SETUP         0x00000008  // Setup Packet
#define USB_CSRL0_STALLED       0x00000004  // Endpoint Stalled
#define USB_CSRL0_TXRDY         0x00000002  // Transmit Packet Ready
#define USB_CSRL0_RXRDY         0x00000001  // Receive Packet Ready

/******************************************************************************
 *
 * The following defines are used with the bmRequestType member of tUSBRequest.
 *
 * Request types have 3 bit fields:
 * 4:0 - Is the recipient type.
 * 6:5 - Is the request type.
 * 7 - Is the direction of the request.
 *
 ******************************************************************************/
#define USB_RTYPE_DIR_IN        0x80
#define USB_RTYPE_DIR_OUT       0x00

#define USB_RTYPE_TYPE_M        0x60
#define USB_RTYPE_VENDOR        0x40
#define USB_RTYPE_CLASS         0x20
#define USB_RTYPE_STANDARD      0x00

#define USB_RTYPE_RECIPIENT_M   0x1f
#define USB_RTYPE_OTHER         0x03
#define USB_RTYPE_ENDPOINT      0x02
#define USB_RTYPE_INTERFACE     0x01
#define USB_RTYPE_DEVICE        0x00

//*****************************************************************************
//
// The following are values that can be passed to USBEndpointDataSend() as the
// ulTransType parameter.
//
//*****************************************************************************
#define USB_TRANS_OUT           0x00000102  // Normal OUT transaction
#define USB_TRANS_IN            0x00000102  // Normal IN transaction
#define USB_TRANS_IN_LAST       0x0000010a  // Final IN transaction (for
                                            // endpoint 0 in device mode)
#define USB_TRANS_SETUP         0x0000110a  // Setup transaction (for endpoint
                                            // 0)
#define USB_TRANS_STATUS        0x00000142  // Status transaction (for endpoint
                                            // 0)

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_O_TXCSRL1 register.
//
//*****************************************************************************
#define USB_TXCSRL1_NAKTO       0x00000080  // NAK Timeout
#define USB_TXCSRL1_CLRDT       0x00000040  // Clear Data Toggle
#define USB_TXCSRL1_STALLED     0x00000020  // Endpoint Stalled
#define USB_TXCSRL1_STALL       0x00000010  // Send STALL
#define USB_TXCSRL1_SETUP       0x00000010  // Setup Packet
#define USB_TXCSRL1_FLUSH       0x00000008  // Flush FIFO
#define USB_TXCSRL1_ERROR       0x00000004  // Error
#define USB_TXCSRL1_UNDRN       0x00000004  // Underrun
#define USB_TXCSRL1_FIFONE      0x00000002  // FIFO Not Empty
#define USB_TXCSRL1_TXRDY       0x00000001  // Transmit Packet Ready

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_O_RXCSRL1 register.
//
//*****************************************************************************
#define USB_RXCSRL1_CLRDT       0x00000080  // Clear Data Toggle
#define USB_RXCSRL1_STALLED     0x00000040  // Endpoint Stalled
#define USB_RXCSRL1_STALL       0x00000020  // Send STALL
#define USB_RXCSRL1_REQPKT      0x00000020  // Request Packet
#define USB_RXCSRL1_FLUSH       0x00000010  // Flush FIFO
#define USB_RXCSRL1_DATAERR     0x00000008  // Data Error
#define USB_RXCSRL1_NAKTO       0x00000008  // NAK Timeout
#define USB_RXCSRL1_OVER        0x00000004  // Overrun
#define USB_RXCSRL1_ERROR       0x00000004  // Error
#define USB_RXCSRL1_FULL        0x00000002  // FIFO Full
#define USB_RXCSRL1_RXRDY       0x00000001  // Receive Packet Ready

#define USBLIB_NUM_EP           16    /* Number of supported endpoints. */

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_O_TXCSRH1 register.
//
//*****************************************************************************
#define USB_TXCSRH1_AUTOSET     0x00000080  // Auto Set
#define USB_TXCSRH1_ISO         0x00000040  // Isochronous Transfers
#define USB_TXCSRH1_MODE        0x00000020  // Mode
#define USB_TXCSRH1_DMAEN       0x00000010  // DMA Request Enable
#define USB_TXCSRH1_FDT         0x00000008  // Force Data Toggle
#define USB_TXCSRH1_DMAMOD      0x00000004  // DMA Request Mode
#define USB_TXCSRH1_DTWE        0x00000002  // Data Toggle Write Enable
#define USB_TXCSRH1_DT          0x00000001  // Data Toggle

//*****************************************************************************
//
// The following are defines for the bit fields in the USB_O_RXCSRH1 register.
//
//*****************************************************************************
#define USB_RXCSRH1_AUTOCL      0x00000080  // Auto Clear
#define USB_RXCSRH1_AUTORQ      0x00000040  // Auto Request
#define USB_RXCSRH1_ISO         0x00000040  // Isochronous Transfers
#define USB_RXCSRH1_DMAEN       0x00000020  // DMA Request Enable
#define USB_RXCSRH1_DISNYET     0x00000010  // Disable NYET
#define USB_RXCSRH1_PIDERR      0x00000010  // PID Error
#define USB_RXCSRH1_DMAMOD      0x00000008  // DMA Request Mode
#define USB_RXCSRH1_DTWE        0x00000004  // Data Toggle Write Enable
#define USB_RXCSRH1_DT          0x00000002  // Data Toggle

#define USBD_UPG_OUT_06_WAIT                    0x20000040

#define USBD_UPG_IN_04_WAIT                     0x10000010


#define USBD_UPG_OUT_00_IDLE_FLAG               0x30000001
#define USBD_UPG_OUT_01_REC_DATA                0x30000002

#define USBD_UPG_IN_00_REC_DATA                 0x40000001

typedef enum usbDescType {
	/**< not a descriptor.*/
	USB_DESC_TYPE_NOT_DESC = 0U,
	/**< device descriptor.*/
	USB_DESC_TYPE_DEVICE = 1U,
	/**< Configuration descriptor.*/
	USB_DESC_TYPE_CONFIGURATION = 2U,
	/**< String descriptor.*/
	USB_DESC_TYPE_STRING = 3U,
	/**< Interface descriptor.*/
	USB_DESC_TYPE_INTERFACE = 4U,
	/**< Endpoint descriptor.*/
	USB_DESC_TYPE_ENDPOINT = 5U,
	/**< device qualifier descriptor.*/
	USB_DESC_TYPE_DEVICE_QUALIFIER = 6U,
	/**< Other speed config descriptor.*/
	USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION = 7U,
	/**< Interface power descriptor.*/
	USB_DESC_TYPE_INTERFACE_POWER = 8U,
	/**< any other descriptor.*/
	USB_DESC_TYPE_OTHER = 9U
} usbDescType_t;

typedef enum usbDeviceDcdChara {
	/**< min value of this enum .*/
	USB_DEVICE_DCD_CHARA_MIN = 1U,
	/**< this param when selected sets dcd speed.*/
	USB_DEVICE_DCD_CHARA_SPEED = USB_DEVICE_DCD_CHARA_MIN,
	/**< this param when selected sets dcd address.*/
	USB_DEVICE_DCD_CHARA_ADDRESS = 2U,
	/**< this param when selected EP config.*/
	USB_DEVICE_DCD_EP_CONFIG = 3U,
	/**< this param when selected EP config.*/
	USB_DEVICE_DCD_EP_INTERFACE = 4U,
	/**< max value of this enum .*/
	USB_DEVICE_DCD_CHARA_MAX = 5U,
	/**< this param when selected enables or disables remote wakeup.*/
	USB_DEVICE_DCD_CHARA_REMOTE_WAKE = USB_DEVICE_DCD_CHARA_MAX
} usbDeviceDcdChara_t;

typedef enum usbMusbDcdEp0State {
	/* The USB device is waiting on a request from the host controller on */
	/* endpoint zero. */
	USB_MUSB_STATE_IDLE = 0,

	/* The USB device is sending data back to the host due to an IN request. */
	USB_MUSB_STATE_TX,

	/* The USB device is receiving data from the host due to an OUT */
	/* request from the host. */
	USB_MUSB_STATE_RX,

	/* The USB device has completed the IN or OUT request and is now waiting */
	/* for the host to acknowledge the end of the IN/OUT transaction.  This */
	/* is the status phase for a USB control transaction. */
	USB_MUSB_STATE_STATUS,

	/* This endpoint has signaled a stall condition and is waiting for the */
	/* stall to be acknowledged by the host controller. */
	USB_MUSB_STATE_STALL
} tEP0State;

typedef enum usbTransferType {
	/**< Control transfer.*/
	USB_TRANSFER_TYPE_CONTROL = 0U,
	/**< Isochronous transfer.*/
	USB_TRANSFER_TYPE_ISOCH = 1U,
	/**< bulk transfer.*/
	USB_TRANSFER_TYPE_BULK = 2U,
	/**< interrupt transfer.*/
	USB_TRANSFER_TYPE_INTERRUPT = 3U
} usbTransferType_t;

typedef enum usbTokenType {
	/**< out token.*/
	USB_TOKEN_TYPE_OUT = 0U,
	/**< in token.*/
	USB_TOKEN_TYPE_IN = 1U,
	/**< sof token - for future use.*/
	USB_TOKEN_TYPE_SOF = 2U,
	/**< setup token - in case a setup transfer needs to be configured by the
         protocol Core.*/
	USB_TOKEN_TYPE_SETUP = 3U
} usbTokenType_t;

typedef enum usbDeviceState {
	/**< Unknown state - State of data structures unknown.*/
	USB_DEVICE_STATE_UNKNOWN = 0U,
	/**< Device data structures initialized, controller inite'd.*/
	USB_DEVICE_STATE_INITIALIZED = 1U,
	/**< Connected to host.*/
	USB_DEVICE_STATE_CONNECTED = 2U,
	/**< Host has driven reset.*/
	USB_DEVICE_STATE_RESET = 3U,
	/**< address set.*/
	USB_DEVICE_STATE_ADDRESSED = 4U,
	/**<  configuration value sent by host accepted - internal setConfig ()
          has returned successfully.*/
	USB_DEVICE_STATE_CONFIGURED = 5U,
	/**< enumeration is complete. If some custom enumeration commands are sent
         by host the this state is different from configured state.*/
	USB_DEVICE_STATE_ENUM_COMPLETE = 6U,
	/**< One of the endpoints in the device is stalled.*/
	USB_DEVICE_STATE_STALLED_ENDPOINT = 7U,
	/**< Device is starting the suspend procedure.*/
	USB_DEVICE_STATE_INITIATE_SUSPENDED = 8U,
	/**< Device is in suspend state.*/
	USB_DEVICE_STATE_SUSPENDED = 9U,
	/**< Low power mode active.*/
	USB_DEVICE_STATE_LPM_ACTIVE = 10U,
	/**< Unknown error in device.*/
	USB_DEVICE_STATE_ERROR = 11U
} usbDeviceState_t;

typedef struct _UpgHeaderFlag {
	unsigned int uHeadFlag;
	unsigned int uFlagType;
	unsigned int uTypeLen;
	unsigned int uTypeCrc;
} UpgHeaderFlag;

typedef struct usbDevEndptInfo {
	/**< IN , OUT or Setup .*/
	usbTokenType_t endpointDirection;
	/**< USB2.0 spec specifies 4 bits this needs to be extended for USB 3.0. */
	uint32_t endpointNumber;
} usbDevEndptInfo_t;

typedef struct usbDevRequest {
	/**< Address assigned by the host. Note that this field would not be used.*/
	uint32_t deviceAddress;
	/**<Pointer to data buffer - for incoming or out going traffic.*/
	void *pbuf;
	/**<How much of data needs to be sent or received.*/
	/**<PIO mode or DMA mode of copy. If PIO mode(0)is set then , the CPU will
        copy data to controller FIFOs. If DMA mode (value 1)is set, then the
        CPU would not take any action to copy data to or from Controller
        FIFOs.*/
	uint32_t length;
	/**< Pointer to device endpoint info structure . */
	/**<USB 3.0 streamID  .*/
	usbDevEndptInfo_t deviceEndptInfo;
	/**<whether this request is for a zero length packet or not.*/
	uint32_t zeroLengthPacket;
	/**<Status of the request  (0)- Complete, (1)- not processed yet,
	  (2) being processed,(-1) Error */
	int32_t status;
	/**<number of bytes actually received or transmitted.*/
	uint32_t actualLength;
} usbDevRequest_t;

typedef struct usbDevRequest usbEndpt0Request_t;
typedef struct usbDevRequest usbEndptRequest_t;

typedef struct usbMusbDcdDevice {
	uint32_t uiInstanceNo;
	uint32_t baseAddr; /**< Base address of the USB core  */
	uint32_t devAddr; /**< Device address */
	usbEndpt0Request_t Ep0Req; /**< Current endpoint 0 request */
	usbEndptRequest_t inEpReq; /**< Current endpoint request for in transfer*/
	usbEndptRequest_t outEpReq; /**< Current endpoint request for out transfer */
	uint32_t deviceAddress;
	unsigned char *pEP0Data;
	volatile unsigned int ulEP0DataRemain;
	UpgHeaderFlag gslIDLHeadFlag;
	unsigned char ucDataBufferIn[EP0_MAX_PACKET_SIZE];
} usbMusbDcdDevice_t;

typedef struct usbSetupPkt {
	/**< USB Standard request type.*/
	uint8_t bmRequestType;
	/**< Request code.*/
	uint8_t bRequest;
	/**< USB 2.0 WValue.*/
	uint16_t wValue;
	/**< USB 2.0 WIndex.*/
	uint16_t wIndex;
	/**< USB 2.0 WLength.*/
	uint16_t wLength;
} usbSetupPkt_t;

#endif
