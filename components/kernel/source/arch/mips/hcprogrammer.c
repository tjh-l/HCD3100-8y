#include <string.h>
#include <unistd.h>
#include <kernel/io.h>
#include <kernel/bootinfo.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mips/cpu.h>
#include <kernel/io.h>
#include <kernel/irqflags.h>
#include <kernel/lib/console.h>
#include <kernel/ld.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "hcprogrammer.h"

static void __usleep(unsigned long us);
static void USBEp0reqComplete(usbMusbDcdDevice_t *musb, struct usbDevRequest *pReq);
// clang-format off
static unsigned char usbMSCInterface[32] = {
	9,
	USB_DTYPE_CONFIGURATION,
	USBShort(32),
	1,

	1,
	0,

	USB_CONF_ATTR_BUS_PWR,
	250,

	9,
	USB_DTYPE_INTERFACE,
	0,
	0,
	2,
	USB_CLASS_VEND_SPECIFIC,
	USB_MSC_SUBCLASS_SCSI,
	USB_MSC_PROTO_BULKONLY,
	0,

	7,
	USB_DTYPE_ENDPOINT,
	USB_EP_DESC_IN | DATA_IN_ENDPOINT,
	USB_EP_ATTR_BULK,
	USBShort(DATA_IN_EP_MAX_SIZE),
	0,

	7,
	USB_DTYPE_ENDPOINT,
	USB_EP_DESC_OUT | DATA_OUT_ENDPOINT,
	USB_EP_ATTR_BULK,
	USBShort(DATA_OUT_EP_MAX_SIZE),
	0
};

static unsigned char usbMSCDeviceDescriptor[32] = {

	18,
	USB_DTYPE_DEVICE,
	USBShort(0x200),
	0,
	0,
	0,
	64,
	USBShort(0x1CBE),
	USBShort(0x0005),
	USBShort(0x100),
	0,
	0,
	0,
	1

};
static unsigned char usbQualifierDescriptor[10] = {
	10,
	USB_DTYPE_DEVICE_QUAL,
	USBShort(0x200),
	0xff,
	0,
	0,
	64,
	1,
	0,
};

static unsigned char usbBBStringDescritpor[48] =
{
	11,
	USB_DTYPE_STRING,
	'2', '0', '2', '3', '0', '6', '2', '5', 0,

	4,
	USB_DTYPE_STRING,
	USBShort(USB_LANG_EN_US),

	19,
	USB_DTYPE_STRING,
	'B', 'i', 'l', 'l', 'b', 'o', 'a', 'r', 'd', ' ', 'd', 'e', 'v', 'i', 'c', 'e', 0,

	14,
	USB_DTYPE_STRING,
	'H', 'i', 'C', 'h', 'i', 'p', ' ', 'T', 'e', 'c', 'h', 0,
};

static unsigned char usbEEStringDescritpor[18]=
{
	0x12,                                         /* bLength */
	0x03,                   /* bDescriptorType */
	'M', 0x00,                                    /* wcChar0 */
	'S', 0x00,                                    /* wcChar1 */
	'F', 0x00,                                    /* wcChar2 */
	'T', 0x00,                                    /* wcChar3 */
	'1', 0x00,                                    /* wcChar4 */
	'0', 0x00,                                    /* wcChar5 */
	'0', 0x00,                                    /* wcChar6 */
	0x17,                                         /* bVendorCode */
	0x00,                                         /* bReserved */
};

static unsigned char usbIndex4Descritpor[40]=
{
	0x28, 0x00, 0x00, 0x00,                       /* dwLength */
	0x00, 0x01,                                   /* bcdVersion */
	0x04, 0x00,                                   /* wIndex */
	0x01,                                         /* bCount */
	0,0,0,0,0,0,0,                                /* Reserved */
	/* WCID Function  */
	0x00,                                         /* bFirstInterfaceNumber */
	0x01,                                         /* bReserved */
	/* CID */
	'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
	/* sub CID */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0,0,0,0,0,0,                                  /* Reserved */
};

static unsigned char usbIndex5Descritpor[142]=
{
	///////////////////////////////////////
	/// WCID property descriptor
	///////////////////////////////////////
	0x8e, 0x00, 0x00, 0x00,                           /* dwLength */
	0x00, 0x01,                                       /* bcdVersion */
	0x05, 0x00,                                       /* wIndex */
	0x01, 0x00,                                       /* wCount */
	///////////////////////////////////////
	/// registry propter descriptor
	///////////////////////////////////////
	0x84, 0x00, 0x00, 0x00,                           /* dwSize */
	0x01, 0x00, 0x00, 0x00,                           /* dwPropertyDataType */
	0x28, 0x00,                                       /* wPropertyNameLength */
	/* DeviceInterfaceGUID */
	'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00,       /* wcName_20 */
	'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00,       /* wcName_20 */
	't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00,       /* wcName_20 */
	'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00,       /* wcName_20 */
	'U', 0x00, 'I', 0x00, 'D', 0x00, 0x00, 0x00,      /* wcName_20 */
	0x4e, 0x00, 0x00, 0x00,                           /* dwPropertyDataLength */
	/* {1D4B2365-4749-48EA-B38A-7C6FDDDD7E26} */
	'{', 0x00, '1', 0x00, 'D', 0x00, '4', 0x00,       /* wcData_39 */
	'B', 0x00, '2', 0x00, '3', 0x00, '6', 0x00,       /* wcData_39 */
	'5', 0x00, '-', 0x00, '4', 0x00, '7', 0x00,       /* wcData_39 */
	'4', 0x00, '9', 0x00, '-', 0x00, '4', 0x00,       /* wcData_39 */
	'8', 0x00, 'E', 0x00, 'A', 0x00, '-', 0x00,       /* wcData_39 */
	'B', 0x00, '3', 0x00, '8', 0x00, 'A', 0x00,       /* wcData_39 */
	'-', 0x00, '7', 0x00, 'C', 0x00, '6', 0x00,       /* wcData_39 */
	'F', 0x00, 'D', 0x00, 'D', 0x00, 'D', 0x00,       /* wcData_39 */
	'D', 0x00, '7', 0x00, 'E', 0x00, '2', 0x00,       /* wcData_39 */
	'6', 0x00, '}', 0x00, 0x00, 0x00,                 /* wcData_39 */
};
// clang-format on

static void USBSetupVar(usbMusbDcdDevice_t *musb, int idx)
{
	struct bootinfo *pbootinfo = (struct bootinfo *)0xA0000000;
	memset(musb, 0, sizeof(musb));
	if (idx == 0)
		musb->baseAddr = USB0_BASE;
	else
		musb->baseAddr = USB1_BASE;
	musb->uiInstanceNo = idx;
	if ((REG32_GET_FIELD2((uint32_t)0xb8800000, 16, 16) == 0x1700) || pbootinfo->hcprogrammer_winusb_en) {
		REG8_WRITE(usbMSCInterface + 14, 0);
		REG8_WRITE(usbMSCInterface + 15, USB_CUSTOM_SUBCLASS_SCSI);
		REG8_WRITE(usbMSCInterface + 16, USB_CUSTOM_PROTO_BULKONLY);
		REG16_WRITE(usbMSCDeviceDescriptor + 8, 0x6871);
		REG16_WRITE(usbMSCDeviceDescriptor + 10, 0x1700);
		REG16_WRITE(usbMSCDeviceDescriptor + 12, 0x200);
		REG8_WRITE(usbMSCDeviceDescriptor + 14, 1);
		REG8_WRITE(usbMSCDeviceDescriptor + 15, 2);
		REG8_WRITE(usbMSCDeviceDescriptor + 16, 3);
		REG8_WRITE(usbMSCDeviceDescriptor + 16, 1);
	}
}

static void USBPhyOn(uint32_t uiPHYConfigRegAddr, uint32_t usbType)
{
	if (usbType & 0x10)
		*((uint16_t *)(uiPHYConfigRegAddr + 0x1020)) |= 0x10C0;
	*((uint32_t *)(uiPHYConfigRegAddr + USB_O_UTMI)) &= 0xFFFFFF6F;
	*((uint32_t *)(uiPHYConfigRegAddr + USB_O_UTMI)) |= (0x01 | usbType);
	__usleep(900);
	*((uint32_t *)(uiPHYConfigRegAddr + USB_O_UTMI)) &= ~(0x01);
}

static void USBClockCfg(uint32_t ulIndex)
{
	if (ulIndex == 0x00) {
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVCLK1)) &= ~SCTRL_DEVCLKRST1_USB0;
	} else {
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVCLK1)) &= ~SCTRL_DEVCLKRST1_USB1;
	}
}

static void USBClockOff(uint32_t ulIndex)
{
	if (ulIndex == 0x00) {
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVCLK1)) |= SCTRL_DEVCLKRST1_USB0;
	} else {
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVCLK1)) |= SCTRL_DEVCLKRST1_USB1;
	}
}

static void USBReset(uint32_t ulBase)
{
	if (ulBase == USB0_BASE) {
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVRST0)) |= SCTRL_DEVRST0_USB0; // USB_PORT_0
		__usleep(900);
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVRST0)) &= ~SCTRL_DEVRST0_USB0; // USB_PORT_0
	} else if (ulBase == USB1_BASE) {
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVRST1)) |= SCTRL_DEVRST1_USB1; // USB_PORT_1
		__usleep(900);
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVRST1)) &= ~SCTRL_DEVRST1_USB1; // USB_PORT_1
	}
}

static void USBDevDisconnect(uint32_t ulBase)
{
	/* Disable connection to the USB bus. */
	HWREGB(ulBase + USB_O_POWER) &= (~USB_POWER_SOFTCONN);
}

static void USBDevConnect(uint32_t ulBase)
{
	/* Enable connection to the USB bus. */
	HWREGB(ulBase + USB_O_POWER) |= USB_POWER_SOFTCONN;
}

static void USBDeviceInit(usbMusbDcdDevice_t *musb)
{
	/* Initializing device request structure. */
	musb->Ep0Req.deviceAddress = 0U;
	musb->Ep0Req.pbuf = NULL;
	musb->Ep0Req.length = 0U;
	musb->Ep0Req.zeroLengthPacket = 0U;
	musb->Ep0Req.status = 0;
	musb->Ep0Req.actualLength = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_IN;
	musb->Ep0Req.deviceEndptInfo.endpointNumber = 0U;
}

#ifdef CONFIG_SOC_HC16XX
static void USBSetMode(usbMusbDcdDevice_t *musb, int mode)
{
	uint32_t musb_base = musb->baseAddr;
	unsigned char reg = 0;

	if (mode == 0) {
		HWREGB(musb_base + USB_O_UTMI) = (0x1 << 7);
		__usleep(900);
		reg = HWREGB(musb_base + USB_O_UTMI);
		reg |= ((0x1 << 7) | (0x3 << 2)); // disable OTG function
		reg &= ~(0x1 << 4); // set as host mode
		HWREGB(musb_base + USB_O_UTMI) = reg;
	} else if (mode == 1) {
		HWREGB(musb_base + USB_O_UTMI) = ((0x1 << 7) | (0x1 << 4));
		__usleep(900);

		reg = HWREGB(musb_base + USB_O_UTMI);
		reg |= ((0x1 << 7) | (0x3 << 2)); // disable OTG function
		reg |= (0x1 << 4); // set as peripheral
		HWREGB(musb_base + USB_O_UTMI) = reg;
	}

	reg = HWREGB(musb_base + USB_O_DEVCTL);
	reg &= ~0x01;
	HWREGB(musb_base + USB_O_DEVCTL) = reg;
	__usleep(900);

	reg = HWREGB(musb_base + USB_O_DEVCTL);
	reg |= 0x01;
	HWREGB(musb_base + USB_O_DEVCTL) = reg;

	reg = HWREGB(musb_base + USB_O_POWER);
	reg |= 0x40;
	HWREGB(musb_base + USB_O_POWER) = reg;
	__usleep(900);
}
#elif defined(CONFIG_SOC_HC15XX)
static void USBSetMode(usbMusbDcdDevice_t *musb, int mode)
{
	uint32_t musb_base = musb->baseAddr;
	unsigned char reg = 0;
	if (mode == 0) {
		if (musb->uiInstanceNo == 0) {
			reg = HWREGB(musb_base + USB_O_UTMI);
			reg |= (0x1 << 7); // bit[7]=1  OTG_DIS
			HWREGB(musb_base + USB_O_UTMI) = reg;
		} else {
			reg = HWREGB(musb_base + USB_O_UTMI);
			reg &= ~(0x1 << 6); // bit[6]=0 id=0
			HWREGB(musb_base + USB_O_UTMI) = reg;
		}
	} else if (mode == 1) {
		if (musb->uiInstanceNo == 0) {
			HWREGB(musb_base + USB_O_UTMI) = ((0x1 << 7) | (0x1 << 4));
			reg = HWREGB(musb_base + USB_O_UTMI);
			reg &= ~(0x1 << 7); // bit[7]=0  OTG_DIS
			HWREGB(musb_base + USB_O_UTMI) = reg;
			HWREGH(musb_base + 0x1020) = 0x10c0; //configure phy vbus/avalid/bvalid =1
		} else {
			HWREGB(musb_base + USB_O_UTMI) = ((0x1 << 7) | (0x1 << 4));
			reg = HWREGB(musb_base + USB_O_UTMI);
			reg |= (0x1 << 6); // bit[6]=1 id=1
			HWREGB(musb_base + USB_O_UTMI) = reg;
			HWREGH(musb_base + 0x1020) = 0x10c0; //configure phy vbus/avalid/bvalid =1
		}
	}

	reg = HWREGB(musb_base + USB_O_DEVCTL);
	reg &= ~0x01;
	HWREGB(musb_base + USB_O_DEVCTL) = reg;
	__usleep(900);

	reg = HWREGB(musb_base + USB_O_DEVCTL);
	reg |= 0x01;
	HWREGB(musb_base + USB_O_DEVCTL) = reg;

	reg = HWREGB(musb_base + USB_O_POWER);
	reg |= 0x40;
	HWREGB(musb_base + USB_O_POWER) = reg;
	__usleep(900);
}
#endif

static void BUSB_Init(usbMusbDcdDevice_t *musb)
{
	if (!(REG32_READ(0xb8818a00) & BIT30) && musb->baseAddr == USB0_BASE)
		return;
	if (!(REG32_READ(0xb8818a00) & BIT31) && musb->baseAddr == USB1_BASE)
		return;
	USBDeviceInit(musb);

	/* ip reset */
	USBReset(musb->baseAddr);
	/* set host */
	USBSetMode(musb, 0);
	/* set device */
	USBSetMode(musb, 1);
}

static uint32_t USBIntStatusEndpoint(uint32_t ulBase)
{
	uint32_t ulStatus;

	/* Get the transmit interrupt status. */
	ulStatus = HWREGH(ulBase + USB_O_TXIS);
	ulStatus |= (HWREGH(ulBase + USB_O_RXIS) << USB_INTEP_RX_SHIFT);

	/* Return the combined interrupt status. */
	return (ulStatus);
}

static uint32_t USBIntStatusControl(uint32_t ulBase)
{
	return (uint32_t)HWREGB(ulBase + USB_O_IS);
}

static uint32_t USBEndpointStatus(uint32_t ulBase, uint32_t ulEndpoint)
{
	uint32_t ulStatus;

	/* Get the TX portion of the endpoint status. */
	ulStatus = HWREGH(ulBase + EP_OFFSET(ulEndpoint) + USB_O_TXCSRL1);

	/* Get the RX portion of the endpoint status. */
	ulStatus |= ((HWREGH(ulBase + EP_OFFSET(ulEndpoint) + USB_O_RXCSRL1)) << USB_RX_EPSTATUS_SHIFT);

	/* Return the endpoint status. */
	return (ulStatus);
}

static int USBEndpointDataGet(uint32_t ulBase, uint32_t ulEndpoint, unsigned char *pucData, uint32_t *pulSize)
{
	uint32_t ulRegister, ulByteCount, ulFIFO;

	/* Get the address of the receive status register to use, based on the endpoint. */
	if (ulEndpoint == USB_EP_0)
		ulRegister = USB_O_CSRL0;
	else
		ulRegister = USB_O_RXCSRL1 + EP_OFFSET(ulEndpoint);

	/* Don't allow reading of data if the RxPktRdy bit is not set. */
	if ((HWREGH(ulBase + ulRegister) & USB_CSRL0_RXRDY) == 0) {
		/* Can't read the data because none is available. */
		*pulSize = 0;

		/* Return a failure since there is no data to read. */
		return (-1);
	}

	/* Get the byte count in the FIFO. */
	ulByteCount = HWREGH(ulBase + USB_O_COUNT0 + ulEndpoint);

	/* Determine how many bytes we will actually copy. */
	ulByteCount = (ulByteCount < *pulSize) ? ulByteCount : *pulSize;

	/* Return the number of bytes we are going to read. */
	*pulSize = ulByteCount;

	/* Calculate the FIFO address. */
	ulFIFO = ulBase + USB_O_FIFO0 + (ulEndpoint >> 2);

	while ((((uint32_t)pucData % 4) != 0x00) && (ulByteCount != 0x0)) {
		*pucData++ = HWREGB(ulFIFO);
		ulByteCount--;
	}

	while (ulByteCount >= 4) {
		*((uint32_t *)pucData) = HWREG(ulFIFO);
		pucData += 4;
		ulByteCount -= 4;
	}

	/* Read the data out of the FIFO. */
	for (; ulByteCount > 0; ulByteCount--) {
		/* Read a byte at a time from the FIFO. */
		*pucData++ = HWREGB(ulFIFO);
	}

	return (0);
}

static int USBEndpointDataPut(uint32_t ulBase, uint32_t ulEndpoint, unsigned char *pucData, uint32_t ulSize)
{
	uint32_t ulFIFO;
	unsigned char ucTxPktRdy;

	/* Get the bit position of TxPktRdy based on the endpoint. */
	if (ulEndpoint == USB_EP_0)
		ucTxPktRdy = USB_CSRL0_TXRDY;
	else
		ucTxPktRdy = USB_TXCSRL1_TXRDY;

	/* Don't allow transmit of data if the TxPktRdy bit is already set. */
	if (HWREGB(ulBase + USB_O_CSRL0 + ulEndpoint) & ucTxPktRdy)
		return (-1);

	/* Calculate the FIFO address. */
	ulFIFO = ulBase + USB_O_FIFO0 + (ulEndpoint >> 2);

	while ((((uint32_t)pucData % 4) != 0x00) && (ulSize != 0x0)) {
		HWREGB(ulFIFO) = *pucData++;
		ulSize--;
	}

	while (ulSize >= 4) {
		HWREG(ulFIFO) = *((uint32_t *)pucData);
		pucData += 4;
		ulSize -= 4;
	}

	/* Write the data to the FIFO. */
	for (; ulSize > 0; ulSize--) {
		HWREGB(ulFIFO) = *pucData++;
	}

	return (0);
}

static int USBEndpointDataSend(uint32_t ulBase, uint32_t ulEndpoint, uint32_t ulTransType)
{
	uint32_t ulTxPktRdy;
	uint32_t ucTxPktRdy;

	/* Get the bit position of TxPktRdy based on the endpoint. */
	if (ulEndpoint == USB_EP_0) {
		ulTxPktRdy = ulTransType & 0xff;
		ucTxPktRdy = USB_CSRL0_TXRDY;
	} else {
		ulTxPktRdy = (ulTransType >> 8) & 0xff;
		ucTxPktRdy = USB_TXCSRL1_TXRDY;
	}

	/* Don't allow transmit of data if the TxPktRdy bit is already set. */
	if (HWREGB(ulBase + USB_O_CSRL0 + ulEndpoint) & ucTxPktRdy)
		return (-1);

	/* Set TxPktRdy in order to send the data. */
	HWREGB(ulBase + USB_O_CSRL0 + ulEndpoint) = ulTxPktRdy;

	return (0);
}

static void USBDEP0StateTx(usbMusbDcdDevice_t *musb)
{
	uint32_t ulNumBytes;
	unsigned char *pData;

	/* In the TX state on endpoint zero. */
	musb->Ep0Req.status = USB_MUSB_STATE_TX;

	/* Set the number of bytes to send this iteration. */
	ulNumBytes = musb->ulEP0DataRemain;

	/* Limit individual transfers to 64 bytes. */
	if (ulNumBytes > EP0_MAX_PACKET_SIZE)
		ulNumBytes = EP0_MAX_PACKET_SIZE;

	/* Save the pointer so that it can be passed to the USBEndpointDataPut() function. */
	pData = (unsigned char *)musb->pEP0Data;

	/* Advance the data pointer and counter to the next data to be sent. */
	musb->ulEP0DataRemain -= ulNumBytes;
	musb->pEP0Data += ulNumBytes;

	/* Put the data in the correct FIFO. */
	USBEndpointDataPut(musb->baseAddr, USB_EP_0, pData, ulNumBytes);

	/* If this is exactly 64 then don't set the last packet yet. */
	if (ulNumBytes == EP0_MAX_PACKET_SIZE) {
		/*
		 * There is more data to send or exactly 64 bytes were sent, this
		 * means that there is either more data coming or a null packet needs
		 * to be sent to complete the transaction.
		 */
		USBEndpointDataSend(musb->baseAddr, USB_EP_0, USB_TRANS_IN);
	} else {
		/* Now go to the status state and wait for the transmit to complete. */
		musb->Ep0Req.status = USB_MUSB_STATE_STATUS;

		/* Send the last bit of data. */
		USBEndpointDataSend(musb->baseAddr, USB_EP_0, USB_TRANS_IN_LAST);
	}
}

static void USBMusbDcdEp0Req(usbMusbDcdDevice_t *musb)
{
	/* Retrieve private data from core object */

	/* Check which kind of transfer has been requested */
	switch (musb->Ep0Req.status) {
	case USB_MUSB_STATE_IDLE:
		if (1U == musb->Ep0Req.zeroLengthPacket) {
			/*
			 * It is a 2 state request, no data phase
			 * EP0 remains in IDLE state when there is no data phase
			 */
			musb->Ep0Req.status = USB_MUSB_STATE_IDLE;
		} else {
			/* This is a 2 state request */
			if (USB_TOKEN_TYPE_IN == musb->Ep0Req.deviceEndptInfo.endpointDirection) {
				/* This is a in data request */
				musb->Ep0Req.status = USB_MUSB_STATE_TX;

				musb->pEP0Data = musb->Ep0Req.pbuf;
				musb->ulEP0DataRemain = musb->Ep0Req.length;

				USBDEP0StateTx(musb);
			} else if (USB_TOKEN_TYPE_OUT == musb->Ep0Req.deviceEndptInfo.endpointDirection) {
				/* This is a out data request */
				musb->Ep0Req.status = USB_MUSB_STATE_RX;

				musb->pEP0Data = musb->Ep0Req.pbuf;
				musb->ulEP0DataRemain = musb->Ep0Req.length;
			}
		}
		break;
	default:
		/* This condition is not taken care of, error */
		break;
	}
}

static void USBEp0reqComplete(usbMusbDcdDevice_t *musb, struct usbDevRequest *pReq)
{
	/* Endpoint 0 request has been completed, so necessary action need to be taken */
	if (USB_TOKEN_TYPE_IN == pReq->deviceEndptInfo.endpointDirection) {
		if (1u != musb->Ep0Req.zeroLengthPacket) {
			/* it was the data stage of the device descriptor. Return the ZLP */
			musb->Ep0Req.deviceAddress = musb->deviceAddress;
			musb->Ep0Req.pbuf = NULL;
			musb->Ep0Req.length = 0U;
			musb->Ep0Req.zeroLengthPacket = 1U;
			musb->Ep0Req.actualLength = 0U;
			musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_OUT;
			USBMusbDcdEp0Req(musb);
		}
	}
}

static void USBDGetStatus(usbMusbDcdDevice_t *musb)
{
	const char data[2] = { 0 };
	/* Setup the request and send the USB status */
	musb->Ep0Req.deviceAddress = musb->deviceAddress;
	musb->Ep0Req.pbuf = (void *)data;
	musb->Ep0Req.length = 2U;
	musb->Ep0Req.zeroLengthPacket = 0U;
	musb->Ep0Req.actualLength = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_IN;

	USBMusbDcdEp0Req(musb);
}

static void USBDevEndpointDataAck(uint32_t ulBase, uint32_t ulEndpoint, uint32_t bIsLastPacket)
{
	/* Determine which endpoint is being acked. */
	if (ulEndpoint == USB_EP_0) {
		/* Clear RxPktRdy, and optionally DataEnd, on endpoint zero. */
		HWREGB(ulBase + USB_O_CSRL0) = USB_CSRL0_RXRDYC | (bIsLastPacket ? USB_CSRL0_DATAEND : 0);
	} else {
		/* Clear RxPktRdy on all other endpoints. */
		HWREGB(ulBase + USB_O_RXCSRL1 + EP_OFFSET(ulEndpoint)) &= ~(USB_RXCSRL1_RXRDY);
	}
}

static void USBDevEndpointConfigSet(uint32_t ulBase, uint32_t ulEndpoint, uint32_t ulMaxPacketSize, uint32_t ulFlags)
{
	uint32_t ulRegister;

	/* Determine if a transmit or receive endpoint is being configured. */
	if (ulFlags & USB_EP_DEV_IN) {
		/* Set the maximum packet size. */
		HWREGH((ulBase + EP_OFFSET(ulEndpoint) + USB_O_TXMAXP1)) = ulMaxPacketSize;

		/* The transmit control value is zero unless options are enabled. */
		ulRegister = 0;

		/* Allow auto setting of TxPktRdy when max packet size has been loaded into the FIFO. */
		if (ulFlags & USB_EP_AUTO_SET)
			ulRegister |= USB_TXCSRH1_AUTOSET;

		/* Configure the DMA mode. */
		if (ulFlags & USB_EP_DMA_MODE_1)
			ulRegister |= USB_TXCSRH1_DMAEN | USB_TXCSRH1_DMAMOD;
		else if (ulFlags & USB_EP_DMA_MODE_0)
			ulRegister |= USB_TXCSRH1_DMAEN;

		/* Enable isochronous mode if requested. */
		if ((ulFlags & USB_EP_MODE_MASK) == USB_EP_MODE_ISOC) {
			ulRegister |= USB_TXCSRH1_ISO;
		}

		/* Write the transmit control value. */
		HWREGB(ulBase + EP_OFFSET(ulEndpoint) + USB_O_TXCSRH1) = (unsigned char)ulRegister;

		/* Reset the Data toggle to zero. */
		HWREGB(ulBase + EP_OFFSET(ulEndpoint) + USB_O_TXCSRL1) = USB_TXCSRL1_CLRDT;
	} else {
		/* Set the MaxPacketSize. */
		HWREGH((ulBase + EP_OFFSET(ulEndpoint) + USB_O_RXMAXP1)) = ulMaxPacketSize;

		/* The receive control value is zero unless options are enabled. */
		ulRegister = 0;

		/* Allow auto clearing of RxPktRdy when packet of size max packet has been unloaded from the FIFO. */
		if (ulFlags & USB_EP_AUTO_CLEAR)
			ulRegister = USB_RXCSRH1_AUTOCL;

		/* Configure the DMA mode. */
		if (ulFlags & USB_EP_DMA_MODE_1)
			ulRegister |= USB_RXCSRH1_DMAEN | USB_RXCSRH1_DMAMOD;
		else if (ulFlags & USB_EP_DMA_MODE_0)
			ulRegister |= USB_RXCSRH1_DMAEN;

		/* Enable isochronous mode if requested. */
		if ((ulFlags & USB_EP_MODE_MASK) == USB_EP_MODE_ISOC)
			ulRegister |= USB_RXCSRH1_ISO;

		/* Write the receive control value. */
		HWREGB(ulBase + EP_OFFSET(ulEndpoint) + USB_O_RXCSRH1) = (unsigned char)ulRegister;

		/* Reset the Data toggle to zero. */
		HWREGB(ulBase + EP_OFFSET(ulEndpoint) + USB_O_RXCSRL1) = USB_RXCSRL1_CLRDT;
	}
}

static void USBIndexWrite(uint32_t ulBase, uint32_t ulEndpoint, uint32_t ulIndexedReg, uint32_t ulValue, uint32_t ulSize)
{
	uint32_t ulIndex;

	/* Save the old index in case it was in use. */
	ulIndex = HWREGB(ulBase + USB_O_EPIDX);

	/* Set the index. */
	HWREGB(ulBase + USB_O_EPIDX) = ulEndpoint;

	/* Determine the size of the register value. */
	if (ulSize == 1)
		HWREGB(ulBase + ulIndexedReg) = ulValue;
	else
		HWREGH(ulBase + ulIndexedReg) = ulValue;

	/* Restore the old index in case it was in use. */
	HWREGB(ulBase + USB_O_EPIDX) = ulIndex;
}

static void USBFIFOConfigSet(uint32_t ulBase, uint32_t ulEndpoint, uint32_t ulFIFOAddress, uint32_t ulFIFOSize, uint32_t ulFlags)
{
	/* See if the transmit or receive FIFO is being configured. */
	if (ulFlags & (USB_EP_HOST_OUT | USB_EP_DEV_IN)) {
		/* Set the transmit FIFO location and size for this endpoint. */
		USBIndexWrite(ulBase, ulEndpoint >> 4, USB_O_TXFIFOSZ, ulFIFOSize, 1);
		USBIndexWrite(ulBase, ulEndpoint >> 4, USB_O_TXFIFOADD, ulFIFOAddress >> 3, 2);
	} else {
		/* Set the receive FIFO location and size for this endpoint. */
		USBIndexWrite(ulBase, ulEndpoint >> 4, USB_O_RXFIFOSZ, ulFIFOSize, 1);
		USBIndexWrite(ulBase, ulEndpoint >> 4, USB_O_RXFIFOADD, ulFIFOAddress >> 3, 2);
	}
}

static void USBMusbDcdDeviceConfig(usbMusbDcdDevice_t *musb)
{
	/* Set the endpoint configuration. */
	USBDevEndpointConfigSet(musb->baseAddr, INDEX_TO_USB_EP(0x1), 0x200, 0x2100);
	/* Set the endpoint configuration. */
	USBDevEndpointConfigSet(musb->baseAddr, INDEX_TO_USB_EP(0x2), 0x200, 0x0100);

	USBFIFOConfigSet(musb->baseAddr, INDEX_TO_USB_EP(0x1), 0x40, USB_FIFO_SZ_512, USB_EP_DEV_IN);
	USBFIFOConfigSet(musb->baseAddr, INDEX_TO_USB_EP(0x2), 0x240, USB_FIFO_SZ_512, USB_EP_DEV_OUT);

	/* If we get to the end, all is well. */
}

static void USBMusbDcdSetConfiguration(usbMusbDcdDevice_t *musb)
{
	/* Need to ACK the data on end point 0 with last data set as this has no data phase.  */
	USBDevEndpointDataAck(musb->baseAddr, USB_EP_0, true);
	USBMusbDcdDeviceConfig(musb);
}

static void USBMusbDcdConfigDevChara(usbMusbDcdDevice_t *musb, usbDeviceDcdChara_t cfgType, unsigned int cfgVal)
{
	/* Retrieve private data from core object */

	switch (cfgType) {
	case USB_DEVICE_DCD_CHARA_ADDRESS:
		/* Set the address of the device */
		musb->devAddr = cfgVal;

		/* Save the device address as we cannot change address until the status phase is complete. */
		musb->devAddr = musb->devAddr | DEV_ADDR_PENDING;

		/* Need to ACK the data on end point 0 with last data set as this has no data phase. */
		USBDevEndpointDataAck(musb->baseAddr, USB_EP_0, true);

		/* Transition directly to the status state since there is no data phase for this request. */
		musb->Ep0Req.status = USB_MUSB_STATE_STATUS;

		break;

	case USB_DEVICE_DCD_EP_CONFIG:
		/* Set endpoint configurations */
		USBMusbDcdSetConfiguration(musb);
		break;

	default:
		/* error condition on default */
		break;
	}
}

static void USBDSetAddress(usbMusbDcdDevice_t *musb, usbSetupPkt_t *pSetup)
{
	/*Return the ZLP .*/
	musb->Ep0Req.deviceAddress = musb->deviceAddress;
	musb->Ep0Req.pbuf = NULL;
	musb->Ep0Req.length = 0U;
	musb->Ep0Req.zeroLengthPacket = 1U;
	musb->Ep0Req.actualLength = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_IN;

	musb->Ep0Req.deviceAddress = pSetup->wValue;
	musb->deviceAddress = pSetup->wValue;

	/* Transition directly to the status state since there is no data phase for this request.*/
	USBMusbDcdEp0Req(musb);

	/* We need to wait till the set address ACK phase is complete to change device address, but for now lets go ahead.*/

	USBMusbDcdConfigDevChara(musb, USB_DEVICE_DCD_CHARA_ADDRESS, musb->deviceAddress);
}

static void USBDGetDescriptor(usbMusbDcdDevice_t *musb, usbSetupPkt_t *pSetup)
{
	/* Which descriptor are we being asked for? */
	uint16_t config_len;

	musb->Ep0Req.deviceAddress = musb->deviceAddress;
	musb->Ep0Req.zeroLengthPacket = 0U;
	musb->Ep0Req.actualLength = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_IN;

	switch (pSetup->wValue >> 8) {
	case USB_DESC_TYPE_DEVICE: {
		musb->Ep0Req.pbuf = usbMSCDeviceDescriptor;
		musb->Ep0Req.length = 18;
		USBMusbDcdEp0Req(musb);
		break;
	}

	case USB_DESC_TYPE_CONFIGURATION: {
		config_len = pSetup->wLength;
		musb->Ep0Req.pbuf = usbMSCInterface;
		musb->Ep0Req.length = sizeof(usbMSCInterface);
		if (config_len == 0x09)
			musb->Ep0Req.length = 0x09;
		USBMusbDcdEp0Req(musb);
		break;
	}

	case USB_DESC_TYPE_STRING: {
		if ((pSetup->wValue & 0x00FF) == 0xEE) {
			musb->Ep0Req.pbuf = usbEEStringDescritpor;
			musb->Ep0Req.length = sizeof(usbEEStringDescritpor);
		} else {
			musb->Ep0Req.pbuf = usbBBStringDescritpor;
			musb->Ep0Req.length = sizeof(usbBBStringDescritpor);
		}
		USBMusbDcdEp0Req(musb);
		break;
	}

	case USB_DESC_TYPE_DEVICE_QUALIFIER: {
		musb->Ep0Req.pbuf = usbQualifierDescriptor;
		musb->Ep0Req.length = sizeof(usbQualifierDescriptor);
		USBMusbDcdEp0Req(musb);
		break;
	}

	default:
		break;
	}
}

static void USBEnumCompleteProc(usbMusbDcdDevice_t *musb)
{
	musb->outEpReq.status = USBD_UPG_OUT_00_IDLE_FLAG;
	musb->outEpReq.deviceEndptInfo.endpointNumber = 0x2;
	musb->inEpReq.deviceEndptInfo.endpointNumber = 0x1;
	return;
}

static void USBDSetConfiguration(usbMusbDcdDevice_t *musb, usbSetupPkt_t *pSetup)
{
	/* Return the ZLP(Zero length packet). */
	musb->Ep0Req.deviceAddress = musb->deviceAddress;
	musb->Ep0Req.pbuf = NULL;
	musb->Ep0Req.length = 0U;
	musb->Ep0Req.zeroLengthPacket = 1U;
	musb->Ep0Req.actualLength = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_OUT;

	/*
	 * Transition directly to the status state since there is no data phase
	 * for this request i.e. it is a 2 stage transfer
	 */

	USBMusbDcdConfigDevChara(musb, USB_DEVICE_DCD_EP_CONFIG, pSetup->wValue);

	USBMusbDcdEp0Req(musb);

	/* Call the enumeration complete so that it will setup the USB for first OUT request */
	/* If the enum complete handler has been installed, call it */
	USBEnumCompleteProc(musb);
}

static void WinUsbDescriptor(usbMusbDcdDevice_t *musb, usbSetupPkt_t *pSetup)
{
	volatile uint16_t Index = 0;

	Index = pSetup->wIndex;
	musb->Ep0Req.deviceAddress = musb->deviceAddress;
	if (Index == 4) {
		musb->Ep0Req.pbuf = usbIndex4Descritpor;
		musb->Ep0Req.length = pSetup->wLength;
	} else {
		/* Index == 5 */
		musb->Ep0Req.pbuf = usbIndex5Descritpor;
		musb->Ep0Req.length = pSetup->wLength;
	}

	musb->Ep0Req.zeroLengthPacket = 0U;
	musb->Ep0Req.actualLength = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_IN;

	USBMusbDcdEp0Req(musb);
}

static void USBDevSetupDispatcher(usbMusbDcdDevice_t *musb, usbSetupPkt_t *pSetup)
{
	switch (pSetup->bRequest) {
	case 0:
		USBDGetStatus(musb);
		break;
	case 5:
		USBDSetAddress(musb, pSetup);
		break;
	case 6:
		USBDGetDescriptor(musb, pSetup);
		break;
	case 9:
		USBDSetConfiguration(musb, pSetup);
		break;
	case 23: //0x17
		WinUsbDescriptor(musb, pSetup);
		break;
	default:
		break;
	}
}

static void USBDevAddrSet(uint32_t ulBase, uint32_t ulAddress)
{
	/* Set the function address in the correct location. */
	HWREGB(ulBase + USB_O_FADDR) = (unsigned char)ulAddress;
}

static void USBMusbDcdEp0EvntHandler(usbMusbDcdDevice_t *musb)
{
	uint32_t ulEPStatus;
	uint32_t ulSize;
	uint32_t ulDataSize;

	/* Get the end point 0 status. */
	ulEPStatus = USBEndpointStatus(musb->baseAddr, USB_EP_0);

	switch (musb->Ep0Req.status) {
	/* In the IDLE state the code is waiting to receive data from the host. */
	case USB_MUSB_STATE_IDLE: {
		/* Is there a packet waiting for us? */
		if (ulEPStatus & USB_DEV_EP0_OUT_PKTRDY) {
			ulSize = EP0_MAX_PACKET_SIZE;

			/* Get the data from the USB controller end point 0. */
			USBEndpointDataGet(musb->baseAddr, USB_EP_0, musb->ucDataBufferIn, &ulSize);

			/* If there was a null setup packet then just return. */
			if (!ulSize)
				return;

			/* Parsing the setup command bytes */
			/* Call the callback function */
			USBDevSetupDispatcher(musb, (usbSetupPkt_t *)musb->ucDataBufferIn);
		}
		break;
	}

	/* Handle the status state, this is a transitory state from */
	/* USB_MUSB_STATE_TX or USB_MUSB_STATE_RX back to USB_MUSB_STATE_IDLE. */
	case USB_MUSB_STATE_STATUS: {
		/* completion of status phase of transfer */
		/* EP0 is now idle */
		USBEp0reqComplete(musb, &musb->Ep0Req);

		musb->Ep0Req.status = USB_MUSB_STATE_IDLE;

		/* If there is a pending address change then set the address. */
		if (musb->devAddr & DEV_ADDR_PENDING) {
			musb->devAddr &= ~DEV_ADDR_PENDING;
			USBDevAddrSet(musb->baseAddr, musb->devAddr);
		}

		/* If a new packet is already pending, we need to read it */
		/* and handle whatever request it contains. */
		if (ulEPStatus & USB_DEV_EP0_OUT_PKTRDY) {
			/* Process the newly arrived packet. */
			ulSize = EP0_MAX_PACKET_SIZE;

			/* Get the data from the USB controller end point 0. */
			USBEndpointDataGet(musb->baseAddr, USB_EP_0, musb->ucDataBufferIn, &ulSize);

			/* If there was a null setup packet then just return. */
			if (!ulSize) {
				break;
			}

			/* Parsing the setup command bytes */
			/* Call the callback function */
			USBDevSetupDispatcher(musb, (usbSetupPkt_t *)musb->ucDataBufferIn);
		}

		break;
	}

	/* Data is still being sent to the host so handle this in the EP0StateTx() function. */
	case USB_MUSB_STATE_TX: {
		USBDEP0StateTx(musb);
		break;
	}

	/* Handle the receive state for commands that are receiving data on endpoint zero. */
	case USB_MUSB_STATE_RX: {
		/* Set the number of bytes to get out of this next packet. */
		if (musb->ulEP0DataRemain > EP0_MAX_PACKET_SIZE)
			ulDataSize = EP0_MAX_PACKET_SIZE;
		else
			ulDataSize = musb->ulEP0DataRemain;

		/* Get the data from the USB controller end point 0. */
		USBEndpointDataGet(musb->baseAddr, USB_EP_0, musb->pEP0Data, &ulDataSize);

		/* If there we not more that EP0_MAX_PACKET_SIZE or more bytes */
		/* remaining then this transfer is complete.  If there were less than */
		/* EP0_MAX_PACKET_SIZE remaining then there still needs to be */
		/* null packet sent before this is complete. */
		if (musb->ulEP0DataRemain <= EP0_MAX_PACKET_SIZE) {
			USBEp0reqComplete(musb, &musb->Ep0Req);
			musb->Ep0Req.status = USB_MUSB_STATE_IDLE;
			USBDevEndpointDataAck(musb->baseAddr, USB_EP_0, true);
		} else {
			USBEp0reqComplete(musb, &musb->Ep0Req);
		}

		/* Advance the pointer. */
		musb->pEP0Data += ulDataSize;

		/* Decrement the number of bytes that are being waited on. */
		musb->ulEP0DataRemain -= ulDataSize;

		break;
	}
	/* The device stalled endpoint zero so check if the stall needs to be */
	/* cleared once it has been successfully sent. */
	case USB_MUSB_STATE_STALL: {
		/* If we sent a stall then acknowledge this interrupt. */
		if (ulEPStatus & USB_DEV_EP0_SENT_STALL) {
			/* Reset the global end point 0 state to IDLE. */
			musb->Ep0Req.status = USB_MUSB_STATE_IDLE;
		}
		break;
	}
	/* Halt on an unknown state, but only in DEBUG mode builds. */
	default:
		break;
	}
}

static void USBReqIn(usbMusbDcdDevice_t *musb, uint32_t epNum, uint32_t *pUsbData, usbTokenType_t tokenType,
			uint32_t length, usbTransferType_t transferType)
{
	musb->inEpReq.deviceAddress = musb->deviceAddress;
	musb->inEpReq.pbuf = pUsbData;
	musb->inEpReq.length = length;
	musb->inEpReq.zeroLengthPacket = 0U;
	musb->inEpReq.actualLength = 0U;
	musb->inEpReq.deviceEndptInfo.endpointDirection = tokenType;
	musb->inEpReq.deviceEndptInfo.endpointNumber = epNum;

	USBEndpointDataPut(musb->baseAddr, INDEX_TO_USB_EP(epNum), musb->inEpReq.pbuf, musb->inEpReq.length);
	USBEndpointDataSend(musb->baseAddr, INDEX_TO_USB_EP(epNum), USB_TRANS_IN);
}

static unsigned int USBGenerateCode(unsigned char *pDataBuf, unsigned int pDataLen)
{
	unsigned int i = 0;
	unsigned int codeData = 0x00;
	unsigned int *pDataBuff = (unsigned int *)pDataBuf;
	pDataLen = pDataLen >> 2;
	for (i = 0; i < pDataLen; i++) {
		codeData ^= pDataBuff[i];
	}
	return codeData;
}

static void USBMusbDcdEpEvntHandler_OUT(usbMusbDcdDevice_t *musb)
{
	uint32_t ulByteCount;
	uint32_t epNum;
	uint32_t tmpCrc = 0x00;

	epNum = musb->outEpReq.deviceEndptInfo.endpointNumber;
	epNum = INDEX_TO_USB_EP(epNum);

	switch (musb->outEpReq.status) {
	case USBD_UPG_OUT_00_IDLE_FLAG:
		musb->outEpReq.pbuf = &musb->gslIDLHeadFlag;
		ulByteCount = 0x200;
		USBEndpointDataGet(musb->baseAddr, epNum, musb->outEpReq.pbuf, &ulByteCount);
		USBDevEndpointDataAck(musb->baseAddr, epNum, false);
		if (musb->gslIDLHeadFlag.uHeadFlag == 0x1991A0A1) {
			/*
			 * Write Data:
			 * uHeadFlag = 0x1991A0A1
			 * uFlagType = addr
			 * uTypeLen = length
			 */
			musb->outEpReq.actualLength = 0U;
			musb->outEpReq.pbuf = (void *)musb->gslIDLHeadFlag.uFlagType;
			musb->outEpReq.length = musb->gslIDLHeadFlag.uTypeLen;
			musb->outEpReq.status = USBD_UPG_OUT_01_REC_DATA;
		} else if (musb->gslIDLHeadFlag.uHeadFlag == 0x1992B0B1) {
			/*
			 * Read Data:
			 * uHeadFlag = 0x1992B0B1
			 * uFlagType = addr
			 * uTypeLen = length
			 */
			USBReqIn(musb, musb->inEpReq.deviceEndptInfo.endpointNumber,
				    (uint32_t *)musb->gslIDLHeadFlag.uFlagType, USB_TOKEN_TYPE_IN,
				    musb->gslIDLHeadFlag.uTypeLen, USB_TRANSFER_TYPE_BULK);
			musb->outEpReq.status = USBD_UPG_OUT_00_IDLE_FLAG;
		} else if (musb->gslIDLHeadFlag.uHeadFlag == 0x1993C0C1) {
			void (*appl)(void);
			appl = (void (*)(void))musb->gslIDLHeadFlag.uFlagType;
			appl();
			musb->outEpReq.status = USBD_UPG_OUT_00_IDLE_FLAG;
		}
		break;

	case USBD_UPG_OUT_01_REC_DATA:
		ulByteCount = 0x200;
		USBEndpointDataGet(musb->baseAddr, epNum, musb->outEpReq.pbuf, &ulByteCount);
		USBDevEndpointDataAck(musb->baseAddr, epNum, false);
		musb->outEpReq.actualLength += ulByteCount;
		musb->outEpReq.pbuf += ulByteCount;
		if (musb->outEpReq.actualLength >= musb->outEpReq.length) {
			tmpCrc = USBGenerateCode((unsigned char *)musb->gslIDLHeadFlag.uFlagType,
						 musb->gslIDLHeadFlag.uTypeLen);
			if (tmpCrc != musb->gslIDLHeadFlag.uTypeCrc)
				musb->gslIDLHeadFlag.uTypeCrc = tmpCrc;
			USBReqIn(musb, musb->inEpReq.deviceEndptInfo.endpointNumber,
				 (uint32_t *)&musb->gslIDLHeadFlag.uTypeCrc, USB_TOKEN_TYPE_IN, 0x4,
				 USB_TRANSFER_TYPE_BULK);
			musb->outEpReq.status = USBD_UPG_OUT_00_IDLE_FLAG;
		} else {
		}
		break;

	default:
		break;
	}
}

static uint32_t BUSBIrqProcess(usbMusbDcdDevice_t *musb)
{
	uint32_t ulStatus;
	uint32_t ulTmpEpInt;
	uint32_t ulBase = musb->baseAddr;

	ulStatus = USBIntStatusControl(ulBase);
	ulTmpEpInt = USBIntStatusEndpoint(ulBase);

	/* Handle end point 0 interrupts. */
	if (ulTmpEpInt & USB_INTEP_0)
		USBMusbDcdEp0EvntHandler(musb);

	/* Other endpoint and device interrupts */
	if (ulTmpEpInt & 0xFFFEFFFE) {
		if ((ulTmpEpInt & 0xFFFE0000) != 0x00)
			USBMusbDcdEpEvntHandler_OUT(musb);
	}

	return 0;
}

static int usb_upgrade_process(void)
{
	uint32_t stime;
	uint32_t elapsed;
	usbMusbDcdDevice_t musbObj[2];
	struct bootinfo *pbootinfo = (struct bootinfo *)0xA0000000;

	USBSetupVar(&musbObj[0], 0);
	USBSetupVar(&musbObj[1], 1);

	stime = sys_time_from_boot();

	BUSB_Init(&musbObj[0]);
	BUSB_Init(&musbObj[1]);

	while (1) {
		BUSBIrqProcess(&musbObj[0]);
		BUSBIrqProcess(&musbObj[1]);

		elapsed = sys_time_from_boot() - stime;
		if (REG8_READ(0xb8818a02) == 0xA5) {
			return 1;
		}

		if (elapsed > (pbootinfo->hcprogrammer_sync_detect_timeout + 500UL)) {
			return 0;
		}
	}

	return 0;
}

unsigned long sys_time_from_boot(void)
{
	uint32_t sec = REG32_READ(0xb8818a0c);
	uint32_t msec = REG16_READ(0xb8818a0a);
	return sec * 1000 + msec;
}

static unsigned long usb_eqirq_detect(void)
{
	uint32_t ulStatus = 0;
	ulStatus = REG16_READ((uint32_t)&USB0 + 0x00000002);
	ulStatus |= REG16_READ((uint32_t)&USB0 + 0x00000004) << 16;
	ulStatus |= REG16_READ((uint32_t)&USB1 + 0x00000002);
	ulStatus |= REG16_READ((uint32_t)&USB1 + 0x00000004) << 16;
	return ulStatus;
}

static void __usleep(unsigned long us)
{
	uint32_t ctime = sys_time_from_boot();
	us = (us + 999) / 1000;
	while (sys_time_from_boot() < (ctime + us))
		;
}

static void usb_reset(void)
{
	REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x80, BIT28);
	__usleep(900);
	REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x80, BIT28);

	REG32_SET_BIT((uint32_t)&MSYSIO0 + 0x84, BIT29);
	__usleep(900);
	REG32_CLR_BIT((uint32_t)&MSYSIO0 + 0x84, BIT29);
	USBClockOff(0);
	USBClockOff(1);
}

int sys_hcprogrammer_check(void)
{
	struct bootinfo *pbootinfo = (struct bootinfo *)0xA0000000;
	int turn_off_hcprogrammer = 0;

	if (REG16_READ(0xb8818a00) != 0x1995)
		return 1;

	hw_watchdog_reset(3 * 1000 * 1000);
	if (usb_eqirq_detect()) {
		if (usb_upgrade_process()) {
			hw_watchdog_disable();
			__usleep(1000000);
			reset();
		} else {
			turn_off_hcprogrammer = 1;
		}
	}

	if (sys_time_from_boot() >= pbootinfo->hcprogrammer_irq_detect_timeout)
		turn_off_hcprogrammer = 1;

	if (turn_off_hcprogrammer) {
		REG16_WRITE(0xb8818a00, 0xe66a);
		hw_watchdog_disable();
		usb_reset();
		return 1;
	}

	return 0;
}

int sys_hcprogrammer_check_timeout(void)
{
	struct bootinfo *pbootinfo = (struct bootinfo *)0xA0000000;
	if (REG16_READ(0xb8818a00) != 0x1995)
		return 1;

	while (sys_time_from_boot() <= pbootinfo->hcprogrammer_irq_detect_timeout) {
		sys_hcprogrammer_check();
	}

	return 1;
}
