#include <string.h>
#include <unistd.h>
#include <kernel/io.h>
#include <kernel/ld.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "usb_upgrade.h"

static unsigned char usbDataBufferIn[64];
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

static void UsbPhyOn(uint32_t uiPHYConfigRegAddr, uint32_t usbType)
{
	if (usbType & 0x10) {
		*((uint16_t *)(uiPHYConfigRegAddr + 0x1020)) |= 0x10C0;
	}
	*((uint32_t *)(uiPHYConfigRegAddr + USB_O_UTMI)) &= 0xFFFFFF6F;
	*((uint32_t *)(uiPHYConfigRegAddr + USB_O_UTMI)) |= (0x01 | usbType);
	usleep(900);
	*((uint32_t *)(uiPHYConfigRegAddr + USB_O_UTMI)) &= ~(0x01);
}

static void usbClockCfg(uint32_t ulIndex)
{
	if (ulIndex == 0x00) {
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVCLK1)) &= ~SCTRL_DEVCLKRST1_USB0;
	} else {
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVCLK1)) &= ~SCTRL_DEVCLKRST1_USB1;
	}
}

static void USBReset(uint32_t ulBase)
{
	if (ulBase == USB0_BASE) {
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVRST0)) |= SCTRL_DEVRST0_USB0; // USB_PORT_0
		usleep(900);
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVRST0)) &= ~SCTRL_DEVRST0_USB0; // USB_PORT_0
	}
	if (ulBase == USB1_BASE) {
		*((uint32_t *)(SCTRL_BASE + SCTRL_O_DEVRST1)) |= SCTRL_DEVRST1_USB1; // USB_PORT_1
		usleep(900);
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

static void USBDeviceInit(usbMusbDcdDevice_t *musb, char *pName)
{
	/* Initializing device request structure. */
	musb->Ep0Req.deviceAddress = 0U;
	musb->Ep0Req.pbuf = NULL;
	musb->Ep0Req.length = 0U;
	musb->Ep0Req.zeroLengthPacket = 0U;
	musb->Ep0Req.status = 0;
	musb->Ep0Req.actualLength = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointType = USB_TRANSFER_TYPE_CONTROL;
	musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_IN;
	musb->Ep0Req.deviceEndptInfo.MaxEndpointSize = 64U;
	musb->Ep0Req.deviceEndptInfo.endpointNumber = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointInterval = 0xFF;
}

#ifdef CONFIG_SOC_HC16XX
static void USBSetMode(usbMusbDcdDevice_t *musb, int mode)
{
	uint32_t musb_base = musb->baseAddr;
	unsigned char reg = 0;

	if (mode == 0) {
		HWREGB(musb_base + USB_O_UTMI) = (0x1 << 7);
		usleep(900);
		reg = HWREGB(musb_base + USB_O_UTMI);
		reg |= ((0x1 << 7) | (0x3 << 2)); // disable OTG function
		reg &= ~(0x1 << 4); // set as host mode
		HWREGB(musb_base + USB_O_UTMI) = reg;
	} else if (mode == 1) {
		HWREGB(musb_base + USB_O_UTMI) = ((0x1 << 7) | (0x1 << 4));
		usleep(900);

		reg = HWREGB(musb_base + USB_O_UTMI);
		reg |= ((0x1 << 7) | (0x3 << 2)); // disable OTG function
		reg |= (0x1 << 4); // set as peripheral
		HWREGB(musb_base + USB_O_UTMI) = reg;
	}

	reg = HWREGB(musb_base + USB_O_DEVCTL);
	reg &= ~0x01;
	HWREGB(musb_base + USB_O_DEVCTL) = reg;
	usleep(900);

	reg = HWREGB(musb_base + USB_O_DEVCTL);
	reg |= 0x01;
	HWREGB(musb_base + USB_O_DEVCTL) = reg;

	reg = HWREGB(musb_base + USB_O_POWER);
	reg |= 0x40;
	HWREGB(musb_base + USB_O_POWER) = reg;
	usleep(900);
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
	usleep(900);

	reg = HWREGB(musb_base + USB_O_DEVCTL);
	reg |= 0x01;
	HWREGB(musb_base + USB_O_DEVCTL) = reg;

	reg = HWREGB(musb_base + USB_O_POWER);
	reg |= 0x40;
	HWREGB(musb_base + USB_O_POWER) = reg;
	usleep(900);
}
#endif

static void BUSB_Init(usbMusbDcdDevice_t *musb)
{
	USBDeviceInit(musb, 0);

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
	uint32_t ulStatus = 0;

	/* Get the general interrupt status, these bits go into the upper 8 bits
	 * of the returned value. */

	/*ulStatus = HWREGB(USB_0_OTGBASE + USB_0_GENR_INTR); */

	/* Get the general interrupt status, these bits go into the upper 8 bits
	 * of the returned value. */
	ulStatus |= (HWREGB(ulBase + USB_O_IS));

	/* Return the combined interrupt status. */
	return (ulStatus);
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

static int USBEndpointDataGet(uint32_t ulBase, uint32_t ulEndpoint,
		unsigned char *pucData, uint32_t *pulSize)
{
	uint32_t ulRegister, ulByteCount, ulFIFO;

	/* Get the address of the receive status register to use, based on the
	 * endpoint. */
	if (ulEndpoint == USB_EP_0) {
		ulRegister = USB_O_CSRL0;
	} else {
		ulRegister = USB_O_RXCSRL1 + EP_OFFSET(ulEndpoint);
	}

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

	/* Success. */
	return (0);
}

static int USBEndpointDataPut(uint32_t ulBase, uint32_t ulEndpoint, unsigned char *pucData, uint32_t ulSize)
{
	uint32_t ulFIFO;
	unsigned char ucTxPktRdy;

	/* Get the bit position of TxPktRdy based on the endpoint. */
	if (ulEndpoint == USB_EP_0) {
		ucTxPktRdy = USB_CSRL0_TXRDY;
	} else {
		ucTxPktRdy = USB_TXCSRL1_TXRDY;
	}

	/* Don't allow transmit of data if the TxPktRdy bit is already set. */
	if (HWREGB(ulBase + USB_O_CSRL0 + ulEndpoint) & ucTxPktRdy) {
		return (-1);
	}

	/* Calculate the FIFO address. */
	ulFIFO = ulBase + USB_O_FIFO0 + (ulEndpoint >> 2);

	while ((((uint32_t)pucData % 4) != 0x00) && (ulSize != 0x0)) {
		//*pucData++ = HWREGB(ulFIFO);
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

	/* Success. */
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
	if (HWREGB(ulBase + USB_O_CSRL0 + ulEndpoint) & ucTxPktRdy) {
		return (-1);
	}

	/* Set TxPktRdy in order to send the data. */
	HWREGB(ulBase + USB_O_CSRL0 + ulEndpoint) = ulTxPktRdy;

	/* Success. */
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
	if (ulNumBytes > EP0_MAX_PACKET_SIZE) {
		ulNumBytes = EP0_MAX_PACKET_SIZE;
	}

	/* Save the pointer so that it can be passed to the USBEndpointDataPut() */
	/* function. */
	pData = (unsigned char *)musb->pEP0Data;

	/* Advance the data pointer and counter to the next data to be sent. */
	musb->ulEP0DataRemain -= ulNumBytes;
	musb->pEP0Data += ulNumBytes;

	/* Put the data in the correct FIFO. */
	USBEndpointDataPut(musb->baseAddr, USB_EP_0, pData, ulNumBytes);

	/* If this is exactly 64 then don't set the last packet yet. */
	if (ulNumBytes == EP0_MAX_PACKET_SIZE) {
		/* There is more data to send or exactly 64 bytes were sent, this */
		/* means that there is either more data coming or a null packet needs */
		/* to be sent to complete the transaction. */
		USBEndpointDataSend(musb->baseAddr, USB_EP_0, USB_TRANS_IN);
	} else {
		/* Now go to the status state and wait for the transmit to complete. */
		musb->Ep0Req.status = USB_MUSB_STATE_STATUS;

		/* Send the last bit of data. */
		USBEndpointDataSend(musb->baseAddr, USB_EP_0, USB_TRANS_IN_LAST);

		/* If there is a sent callback then call it. */
	}
}

static void USBMusbDcdEp0Req(usbMusbDcdDevice_t *musb)
{
	/* Retrieve private data from core object */

	/* Check which kind of transfer has been requested */
	switch (musb->Ep0Req.status) {
	case USB_MUSB_STATE_IDLE:
		if (1U == musb->Ep0Req.zeroLengthPacket) {
			/* It is a 2 state request, no data phase */
			/* EP0 remains in IDLE state when there is no data phase */
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

static void usbEp0reqComplete(usbMusbDcdDevice_t *musb, struct usbDevRequest *pReq)
{
	//usbMusbDcdDevice_t *musb = musb;

	/* Endpoint 0 request has been completed,
	 * so necessary action need to be taken
	 */
	if (USB_TOKEN_TYPE_IN == pReq->deviceEndptInfo.endpointDirection) {
		if (1u != musb->Ep0Req.zeroLengthPacket) {
			/* it was the data stage of the device descriptor. */
			/*Return the ZLP .*/
			musb->Ep0Req.deviceAddress = musb->deviceAddress;
			musb->Ep0Req.pbuf = NULL;
			musb->Ep0Req.length = 0U;
			musb->Ep0Req.zeroLengthPacket = 1U;
			//musb->Ep0Req.status = 1;
			musb->Ep0Req.actualLength = 0U;
			musb->Ep0Req.deviceEndptInfo.endpointType =
				USB_TRANSFER_TYPE_CONTROL;
			musb->Ep0Req.deviceEndptInfo.endpointDirection =
				USB_TOKEN_TYPE_OUT;
			musb->Ep0Req.reqComplete = (void (*)(void *, struct usbDevRequest *))usbEp0reqComplete;
			//musb->pDcdCore->dcdActions.pFnEndpt0Req(musb->pDcdCore, req);
			USBMusbDcdEp0Req(musb);
		}
	}
}

static void USBDGetStatus(usbMusbDcdDevice_t *musb, usbSetupPkt_t *pSetup)
{
	uint16_t data = 0;

	/* Determine which type of status was requested */
	switch (pSetup->bmRequestType & USB_RTYPE_RECIPIENT_M) {
	case USB_RTYPE_DEVICE:
		/* Return the current status for the device */
		/* TODO Check how this status is to be stored and where */
		data = 0;
		break;
	case USB_RTYPE_INTERFACE:
		data = 0;
		break;
	case USB_RTYPE_ENDPOINT:
		/* TODO Check logic on how to return this status for endpoints */
		data = 0;
		break;
	default:
		/* Set endpoint stall */
		break;
	}
	musb->ucTmpData[0] = data;
	musb->ucTmpData[1] = 0x00;

	/* Setup the request and send the USB status */
	musb->Ep0Req.deviceAddress = musb->deviceAddress;
	musb->Ep0Req.pbuf = musb->ucTmpData;
	musb->Ep0Req.length = 2U;
	musb->Ep0Req.zeroLengthPacket = 0U;
	//musb->Ep0Req.status = 1;
	musb->Ep0Req.actualLength = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointType = USB_TRANSFER_TYPE_CONTROL;
	musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_IN;
	musb->Ep0Req.reqComplete = (void (*)(void *, struct usbDevRequest *))usbEp0reqComplete;

	//musb->pDcdCore->dcdActions.pFnEndpt0Req (musb->pDcdCore, req);
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

static void USBDevEndpointConfigSet(uint32_t ulBase, uint32_t ulEndpoint,
				    uint32_t ulMaxPacketSize, uint32_t ulFlags)
{
	uint32_t ulRegister;

	/* Determine if a transmit or receive endpoint is being configured. */
	if (ulFlags & USB_EP_DEV_IN) {
		/* Set the maximum packet size. */
		HWREGH((ulBase + EP_OFFSET(ulEndpoint) + USB_O_TXMAXP1)) = ulMaxPacketSize;

		/* The transmit control value is zero unless options are enabled. */
		ulRegister = 0;

		/* Allow auto setting of TxPktRdy when max packet size has been loaded */
		/* into the FIFO. */
		if (ulFlags & USB_EP_AUTO_SET) {
			ulRegister |= USB_TXCSRH1_AUTOSET;
		}

		/* Configure the DMA mode. */
		if (ulFlags & USB_EP_DMA_MODE_1) {
			ulRegister |= USB_TXCSRH1_DMAEN | USB_TXCSRH1_DMAMOD;
		} else if (ulFlags & USB_EP_DMA_MODE_0) {
			ulRegister |= USB_TXCSRH1_DMAEN;
		}

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

		/* Allow auto clearing of RxPktRdy when packet of size max packet
		 * has been unloaded from the FIFO. */
		if (ulFlags & USB_EP_AUTO_CLEAR) {
			ulRegister = USB_RXCSRH1_AUTOCL;
		}

		/* Configure the DMA mode. */
		if (ulFlags & USB_EP_DMA_MODE_1) {
			ulRegister |= USB_RXCSRH1_DMAEN | USB_RXCSRH1_DMAMOD;
		} else if (ulFlags & USB_EP_DMA_MODE_0) {
			ulRegister |= USB_RXCSRH1_DMAEN;
		}

		/* Enable isochronous mode if requested. */
		if ((ulFlags & USB_EP_MODE_MASK) == USB_EP_MODE_ISOC) {
			ulRegister |= USB_RXCSRH1_ISO;
		}

		/* Write the receive control value. */
		HWREGB(ulBase + EP_OFFSET(ulEndpoint) + USB_O_RXCSRH1) = (unsigned char)ulRegister;

		/* Reset the Data toggle to zero. */
		HWREGB(ulBase + EP_OFFSET(ulEndpoint) + USB_O_RXCSRL1) = USB_RXCSRL1_CLRDT;
	}
}

static void USBIndexWrite(uint32_t ulBase, uint32_t ulEndpoint,
		uint32_t ulIndexedReg, uint32_t ulValue,
		uint32_t ulSize)
{
	uint32_t ulIndex;

	/* Save the old index in case it was in use. */
	ulIndex = HWREGB(ulBase + USB_O_EPIDX);

	/* Set the index. */
	HWREGB(ulBase + USB_O_EPIDX) = ulEndpoint;

	/* Determine the size of the register value. */
	if (ulSize == 1) {
		/* Set the value. */
		HWREGB(ulBase + ulIndexedReg) = ulValue;
	} else {
		/* Set the value. */
		HWREGH(ulBase + ulIndexedReg) = ulValue;
	}

	/* Restore the old index in case it was in use. */
	HWREGB(ulBase + USB_O_EPIDX) = ulIndex;
}

static void USBFIFOConfigSet(uint32_t ulBase, uint32_t ulEndpoint,
			     uint32_t ulFIFOAddress, uint32_t ulFIFOSize,
			     uint32_t ulFlags)
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

static void usbMusbDcdDeviceConfig(usbMusbDcdDevice_t *musb, const usbConfigHeader_t *psConfig, const tFIFOConfig *psFIFOConfig)
{
	/* Set the endpoint configuration. */
	USBDevEndpointConfigSet(musb->baseAddr, INDEX_TO_USB_EP(0x1), 0x200, 0x2100);
	/* Set the endpoint configuration. */
	USBDevEndpointConfigSet(musb->baseAddr, INDEX_TO_USB_EP(0x2), 0x200, 0x0100);

	USBFIFOConfigSet(musb->baseAddr, INDEX_TO_USB_EP(0x1), 0x40, USB_FIFO_SZ_512, USB_EP_DEV_IN);
	USBFIFOConfigSet(musb->baseAddr, INDEX_TO_USB_EP(0x2), 0x240, USB_FIFO_SZ_512, USB_EP_DEV_OUT);

	/* If we get to the end, all is well. */
}

static void usbMusbDcdSetConfiguration(usbMusbDcdDevice_t *musb)
{
	/* pointer to the descriptors */

	/* Need to ACK the data on end point 0 with last data set as this has no */
	/* data phase. */
	USBDevEndpointDataAck(musb->baseAddr, USB_EP_0, true);

	usbMusbDcdDeviceConfig(musb, NULL, NULL);
}

static void USBMusbDcdConfigDevChara(usbMusbDcdDevice_t *musb,usbDeviceDcdChara_t cfgType, unsigned int cfgVal)
{
	/* Retrieve private data from core object */

	switch (cfgType) {
	case USB_DEVICE_DCD_CHARA_ADDRESS:

		/* Set the address of the device */
		musb->devAddr = cfgVal;

		/* Save the device address as we cannot change address until the status */
		/* phase is complete. */
		musb->devAddr = musb->devAddr | DEV_ADDR_PENDING;

		/* Need to ACK the data on end point 0 with last data set as this has no */
		/* data phase. */
		USBDevEndpointDataAck(musb->baseAddr, USB_EP_0, true);

		/* Transition directly to the status state since there is no data phase */
		/* for this request. */
		musb->Ep0Req.status = USB_MUSB_STATE_STATUS;

		break;

	case USB_DEVICE_DCD_EP_CONFIG:
		/* Set endpoint configurations */
		//musb->deviceInstance.ulConfiguration = (uint8_t)pDevCharecteristic->epConfigNo;
		usbMusbDcdSetConfiguration(musb);
		break;

	default:
		/* error condition on default */
		break;
	}
}

static void USBDSetAddress(usbMusbDcdDevice_t *musb, usbSetupPkt_t * pSetup )
{

	/*Return the ZLP .*/
	musb->Ep0Req.deviceAddress = musb->deviceAddress;
	musb->Ep0Req.pbuf = NULL;
	musb->Ep0Req.length = 0U;
	musb->Ep0Req.zeroLengthPacket = 1U;
	//musb->Ep0Req.status = 1;
	musb->Ep0Req.actualLength = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointType = USB_TRANSFER_TYPE_CONTROL;
	musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_IN;
	musb->Ep0Req.reqComplete = (void (*)(void *, struct usbDevRequest *))usbEp0reqComplete;

	musb->Ep0Req.deviceAddress = pSetup->wValue;
	musb->deviceAddress = pSetup->wValue;

	/* Transition directly to the status state since there is no data phase
	   for this request.*/
	///musb->pDcdCore->dcdActions.pFnEndpt0Req(musb->pDcdCore, req);
	USBMusbDcdEp0Req(musb);

	/*We need to wait till the set address ACK phase is complete to change
	  device address, but for now lets go ahead.*/

	//usbSetDeviceCharaParam(musb,USB_DEVICE_DCD_CHARA_ADDRESS,musb->deviceAddress);

	//musb->pDcdCore->dcdActions.pFnConfigDevChara (musb->pDcdCore, &(musb->pDcdCore->dcdCharecteristics));
	USBMusbDcdConfigDevChara(musb, USB_DEVICE_DCD_CHARA_ADDRESS, musb->deviceAddress);
}

static void USBDGetDescriptor(usbMusbDcdDevice_t *musb, usbSetupPkt_t *pSetup)
{
	/* Which descriptor are we being asked for?*/

	switch (pSetup->wValue >> 8) {
	/*This request was for a device descriptor.*/
	case USB_DESC_TYPE_DEVICE: {
		/*Return the externally provided device descriptor.*/
		musb->Ep0Req.deviceAddress = musb->deviceAddress;
		musb->Ep0Req.pbuf = musb->pMSCDeviceDescriptor_RK;
		musb->Ep0Req.length = 0x12; //pSetup->wLength;//(uint32_t ) musb->pDcdCore->pDesc.pDeviceDesc->bLength;
		musb->Ep0Req.zeroLengthPacket = 0U;
		//musb->Ep0Req.status = 1;
		musb->Ep0Req.actualLength = 0U;
		musb->Ep0Req.deviceEndptInfo.endpointType = USB_TRANSFER_TYPE_CONTROL;
		musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_IN;
		musb->Ep0Req.reqComplete = (void (*)(void *, struct usbDevRequest *))usbEp0reqComplete;

		//musb->pDcdCore->dcdActions.pFnEndpt0Req(musb->pDcdCore, req);
		USBMusbDcdEp0Req(musb);
		break;
	}

		/*This request was for a configuration descriptor.*/
	case USB_DESC_TYPE_CONFIGURATION: {
		//uIndex = (uint8_t)(pSetup->wValue & 0xFF);
		/*Return the externally provided device descriptor.*/
		musb->Ep0Req.deviceAddress = musb->deviceAddress;
		musb->Ep0Req.pbuf = musb->pMSCInterface_RK; //g_sMSCDeviceInfo.ppConfigDescriptors[uIndex];
		musb->Ep0Req.length = 0x20;

		musb->Ep0Req.zeroLengthPacket = 0U;
		//musb->Ep0Req.status = 1;
		musb->Ep0Req.actualLength = 0U;
		musb->Ep0Req.deviceEndptInfo.endpointType = USB_TRANSFER_TYPE_CONTROL;
		musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_IN;

		//musb->pDcdCore->dcdActions.pFnEndpt0Req(musb->pDcdCore, req);
		USBMusbDcdEp0Req(musb);
		break;
	}

	case USB_DESC_TYPE_STRING: {
		/*This request was for a string descriptor.*/
		break;
	}

	default: {
		/*Any other request is not handled by the default enumeration handler
		so see if it needs to be passed on to another handler.*/
		break;
	}
	}
}

static void USBEnumCompleteProc(usbMusbDcdDevice_t *musb)
{
	musb->outEpReq.status = USBD_UPG_OUT_00_IDLE_FLAG;
	musb->outEpReq.deviceEndptInfo.endpointNumber = 0x2;
	musb->inEpReq.deviceEndptInfo.endpointNumber = 0x1;
	return;
}

static void USBDSetConfiguration(usbMusbDcdDevice_t *musb,
		usbSetupPkt_t *pSetup)
{
	/*Return the ZLP(Zero length packet).*/
	musb->Ep0Req.deviceAddress = musb->deviceAddress;
	musb->Ep0Req.pbuf = NULL;
	musb->Ep0Req.length = 0U;
	musb->Ep0Req.zeroLengthPacket = 1U;
	//musb->Ep0Req.status = 1;
	musb->Ep0Req.actualLength = 0U;
	musb->Ep0Req.deviceEndptInfo.endpointType = USB_TRANSFER_TYPE_CONTROL;
	musb->Ep0Req.deviceEndptInfo.endpointDirection = USB_TOKEN_TYPE_OUT;

	/* Transition directly to the status state since there is no data phase
	   for this request i.e. it is a 2 stage transfer*/

	/* Set the asked configuration */
	//usbSetDeviceCharaParam (musb, USB_DEVICE_DCD_EP_CONFIG,pSetup->wValue);

	//musb->pDcdCore->dcdActions.pFnConfigDevChara(musb->pDcdCore,&(musb->pDcdCore->dcdCharecteristics));
	USBMusbDcdConfigDevChara(musb, USB_DEVICE_DCD_EP_CONFIG,
			pSetup->wValue);

	//musb->pDcdCore->dcdActions.pFnEndpt0Req(musb->pDcdCore, req);
	USBMusbDcdEp0Req(musb);

	/* Call the enumeration complete so that it will setup the USB for
	 * first OUT request
	 */

	/* If the enum complete handler has been installed, call it */
	USBEnumCompleteProc(musb);
}

static void USBDevSetupDispatcher(usbMusbDcdDevice_t *musb,
				  usbSetupPkt_t *pSetup)
{
	/* See if this is a standard request or not.*/
	if ((pSetup->bmRequestType & USB_RTYPE_TYPE_M) != USB_RTYPE_STANDARD) {
		return;

	}
	/* it is a standard request. */
	else {
		switch (pSetup->bRequest) {
		case 0:
			USBDGetStatus(musb, pSetup);
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
		default:
			/* If there is no handler then stall this request.*/
			break;
		}
	}
}

static void USBDevAddrSet(uint32_t ulBase, uint32_t ulAddress)
{
	/* Check the arguments. */
	/* Set the function address in the correct location. */
	HWREGB(ulBase + USB_O_FADDR) = (unsigned char)ulAddress;
}

static void usbMusbDcdEp0EvntHandler(usbMusbDcdDevice_t *musb)
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
			USBEndpointDataGet(musb->baseAddr, USB_EP_0, usbDataBufferIn, &ulSize);

			/* If there was a null setup packet then just return. */
			if (!ulSize) {
				return;
			}

			/* Parsing the setup command bytes */
			/* Call the callback function */
			//USBDevEndpt0Handler(musb,USB_ENDPT0_EVENT_SETUP_PACKET_RECEIVED,(usbSetupPkt_t *) usbDataBufferIn);
			USBDevSetupDispatcher(musb, (usbSetupPkt_t *)usbDataBufferIn);
		}
		break;
	}

	/* Handle the status state, this is a transitory state from */
	/* USB_MUSB_STATE_TX or USB_MUSB_STATE_RX back to USB_MUSB_STATE_IDLE. */
	case USB_MUSB_STATE_STATUS: {
		/* completion of status phase of transfer */
		/* EP0 is now idle */
		musb->Ep0Req.reqComplete(musb, &musb->Ep0Req);

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
			USBEndpointDataGet(musb->baseAddr, USB_EP_0, usbDataBufferIn, &ulSize);

			/* If there was a null setup packet then just return. */
			if (!ulSize) {
				break;
			}

			/* Parsing the setup command bytes */
			/* Call the callback function */
			//USBDevEndpt0Handler(musb,USB_ENDPT0_EVENT_SETUP_PACKET_RECEIVED,(usbSetupPkt_t *) usbDataBufferIn);
			USBDevSetupDispatcher(musb, (usbSetupPkt_t *)usbDataBufferIn);
		}

		break;
	}

	/* Data is still being sent to the host so handle this in the */
	/* EP0StateTx() function. */
	case USB_MUSB_STATE_TX: {
		USBDEP0StateTx(musb);
		break;
	}

	/* We are still in the middle of sending the configuration descriptor */
	/* so handle this in the EP0StateTxConfig() function. */
	/* Not valid with DCD driver. May need to enable later. */
	/*case USB_MUSB_STATE_TX_CONFIG: */
	/*{ */
	/*    USBDEP0StateTxConfig(ulIndex); */
	/*    break; */
	/*} */

	/* Handle the receive state for commands that are receiving data on */
	/* endpoint zero. */
	case USB_MUSB_STATE_RX: {
		/* Set the number of bytes to get out of this next packet. */
		if (musb->ulEP0DataRemain > EP0_MAX_PACKET_SIZE) {
			/* Don't send more than EP0_MAX_PACKET_SIZE bytes. */
			ulDataSize = EP0_MAX_PACKET_SIZE;
		} else {
			/* There was space so send the remaining bytes. */
			ulDataSize = musb->ulEP0DataRemain;
		}

		/* Get the data from the USB controller end point 0. */
		USBEndpointDataGet(musb->baseAddr, USB_EP_0, musb->pEP0Data, &ulDataSize);

		/* If there we not more that EP0_MAX_PACKET_SIZE or more bytes */
		/* remaining then this transfer is complete.  If there were less than */
		/* EP0_MAX_PACKET_SIZE remaining then there still needs to be */
		/* null packet sent before this is complete. */
		if (musb->ulEP0DataRemain <= EP0_MAX_PACKET_SIZE) {
			/* Need to ACK the data on end point 0 in this case and set the */
			/* data end as this is the last of the data. */
			/*USBDevEndpointDataAck(musb->uiBaseAddr, USB_EP_0, true); */

			musb->Ep0Req.reqComplete(musb, &musb->Ep0Req);

			/* Return to the idle state. */
			musb->Ep0Req.status = USB_MUSB_STATE_IDLE;

			USBDevEndpointDataAck(musb->baseAddr, USB_EP_0, true);

			/* If there is a receive callback then call it. */

		} else {
			/* Need to ACK the data on end point 0 in this case */
			/* without setting data end because more data is coming. */
			/*USBDevEndpointDataAck(musb->uiBaseAddr, USB_EP_0, false); */

			musb->Ep0Req.reqComplete(musb, &musb->Ep0Req);
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
			/* Clear the Setup End condition. */
			//USBDevEndpointStatusClear(musb->baseAddr, USB_EP_0,
			//                          USB_DEV_EP0_SENT_STALL);

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

static void usbMusbDcdEpEvntHandler_IN(void *musbPtr)
{
	uint32_t ulByteCount;

	usbMusbDcdDevice_t *musb = (usbMusbDcdDevice_t *)musbPtr;

	uint32_t epNum;

	epNum = musb->inEpReq.deviceEndptInfo.endpointNumber;

	switch (musb->inEpReq.status) {
	case USBD_UPG_IN_00_REC_DATA:
		musb->outEpReq.status = USBD_UPG_OUT_00_IDLE_FLAG;
		break;

	case USBD_UPG_IN_00_DDR_DATA:
		musb->outEpReq.status = USBD_UPG_OUT_02_FRM_FLAG;
		break;
	case USBD_UPG_IN_01_FRM_DATA:
		musb->outEpReq.status = USBD_UPG_OUT_04_BRN_FLAG;
		break;
	case USBD_UPG_IN_02_BRN_STAT:
		musb->outEpReq.status = USBD_UPG_OUT_05_RST_FLAG;
		break;
	case USBD_UPG_IN_03_RST_STAT:
		musb->outEpReq.status = USBD_UPG_OUT_07_FINISH;
		break;
	case USBD_UPG_IN_04_WAIT:
		break;
	default:
		break;
	}
}

static void usbUpgReqOut(usbMusbDcdDevice_t *musb, uint32_t epNum,
			 uint32_t *pUsbData, usbTokenType_t tokenType,
			 uint32_t length, usbTransferType_t transferType)
{
	musb->outEpReq.deviceAddress = musb->deviceAddress;
	musb->outEpReq.pbuf = pUsbData;
	musb->outEpReq.length = length;
	musb->outEpReq.zeroLengthPacket = 0U;
	musb->outEpReq.status = 1;
	musb->outEpReq.actualLength = 0U;
	musb->outEpReq.deviceEndptInfo.endpointType = transferType;
	musb->outEpReq.deviceEndptInfo.endpointDirection = tokenType;
	musb->outEpReq.deviceEndptInfo.endpointNumber = epNum;

	//if(musb->inEpReq.length > 512)
	//	usbUpgDmaReq(musb);
}

static void usbUpgReqIn(usbMusbDcdDevice_t *musb, uint32_t epNum, uint32_t *pUsbData,
		usbTokenType_t tokenType, uint32_t length,
		usbTransferType_t transferType)
{
	musb->inEpReq.deviceAddress = musb->deviceAddress;
	musb->inEpReq.pbuf = pUsbData;
	musb->inEpReq.length = length;
	musb->inEpReq.zeroLengthPacket = 0U;
	musb->inEpReq.status = 1;
	musb->inEpReq.actualLength = 0U;
	musb->inEpReq.deviceEndptInfo.endpointType = transferType;
	musb->inEpReq.deviceEndptInfo.endpointDirection = tokenType;
	musb->inEpReq.deviceEndptInfo.endpointNumber = epNum;

	//USBMusbDcdEpReq(musb->pDcdCore, musb->outEpReq);

	//brad_doDmaDisTx(musb->baseAddr, INDEX_TO_USB_EP(epNum));
	/* FIFO mode */
	/* taking the endpoint number from the dcd data (req) and
	   convert it to the end point value that the hw uses. */
	USBEndpointDataPut(musb->baseAddr, INDEX_TO_USB_EP(epNum),
			musb->inEpReq.pbuf, musb->inEpReq.length);
	USBEndpointDataSend(musb->baseAddr, INDEX_TO_USB_EP(epNum),
			USB_TRANS_IN);
}

static unsigned int usbGenerateCode(unsigned char *pDataBuf,
				    unsigned int pDataLen)
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

static void usbMusbDcdEpEvntHandler_OUT(void *musbPtr)
{
	uint32_t ulByteCount;

	usbMusbDcdDevice_t *musb = (usbMusbDcdDevice_t *)musbPtr;

	uint32_t epNum;
	uint32_t tmpCrc = 0x00;

	epNum = musb->outEpReq.deviceEndptInfo.endpointNumber;
	epNum = INDEX_TO_USB_EP(epNum);

	switch (musb->outEpReq.status) {
	case USBD_UPG_OUT_00_IDLE_FLAG:
		musb->outEpReq.pbuf = &musb->gslIDLHeadFlag;
		ulByteCount = 0x200;
		USBEndpointDataGet(musb->baseAddr, epNum, musb->outEpReq.pbuf,
				   &ulByteCount);
		USBDevEndpointDataAck(musb->baseAddr, epNum, false);
		if (musb->gslIDLHeadFlag.uHeadFlag == 0x1991A0A1) { //write addr data
			usbUpgReqOut(musb, 0x2, (uint32_t *)musb->gslIDLHeadFlag.uFlagType,
				USB_TOKEN_TYPE_OUT,
				musb->gslIDLHeadFlag.uTypeLen,
				USB_TRANSFER_TYPE_BULK);
			musb->inEpReq.status = USBD_UPG_IN_04_WAIT;
			musb->outEpReq.status = USBD_UPG_OUT_01_REC_DATA;
		}
		if (musb->gslIDLHeadFlag.uHeadFlag == 0x1992B0B1) {
			usbUpgReqIn(
				musb,
				musb->inEpReq.deviceEndptInfo.endpointNumber,
				(uint32_t *)musb->gslIDLHeadFlag.uFlagType,
				USB_TOKEN_TYPE_IN,
				musb->gslIDLHeadFlag.uTypeLen,
				USB_TRANSFER_TYPE_BULK);
			musb->inEpReq.status = USBD_UPG_IN_00_REC_DATA;
			musb->outEpReq.status = USBD_UPG_OUT_06_WAIT;
		}
		if (musb->gslIDLHeadFlag.uHeadFlag == 0x1993C0C1) {
			void (*appl)(void);
			appl = (void (*)(void))musb->gslIDLHeadFlag.uFlagType;
			appl();
			usbUpgReqIn(
				musb,
				musb->inEpReq.deviceEndptInfo.endpointNumber,
				(uint32_t *)&musb->gslIDLHeadFlag.uFlagType,
				USB_TOKEN_TYPE_IN, 0x4, USB_TRANSFER_TYPE_BULK);
			musb->inEpReq.status = USBD_UPG_IN_00_REC_DATA;
			musb->outEpReq.status = USBD_UPG_OUT_06_WAIT;
		}
		break;
	case USBD_UPG_OUT_01_REC_DATA:
		if (musb->outEpReq.dmaMode != 0x1900) {
			ulByteCount = 0x200;
			USBEndpointDataGet(musb->baseAddr, epNum,
					   musb->outEpReq.pbuf, &ulByteCount);
			USBDevEndpointDataAck(musb->baseAddr, epNum, false);
			musb->outEpReq.actualLength += ulByteCount;
			musb->outEpReq.pbuf += ulByteCount;
			if (REG8_READ(0xb8818a02) == 0xA5) {
				REG8_WRITE(0xb8818a02, 0x5A);
			}
			if (musb->outEpReq.actualLength >= musb->outEpReq.length) {
				tmpCrc = usbGenerateCode(
					(unsigned char *)musb->gslIDLHeadFlag.uFlagType,
					musb->gslIDLHeadFlag.uTypeLen);
				if (tmpCrc != musb->gslIDLHeadFlag.uTypeCrc)
					musb->gslIDLHeadFlag.uTypeCrc = tmpCrc;
				usbUpgReqIn(musb,
					    musb->inEpReq.deviceEndptInfo.endpointNumber,
					    (uint32_t *)&musb->gslIDLHeadFlag.uTypeCrc,
					    USB_TOKEN_TYPE_IN, 0x4,
					    USB_TRANSFER_TYPE_BULK);
				musb->inEpReq.status = USBD_UPG_IN_00_REC_DATA;
				musb->outEpReq.status = USBD_UPG_OUT_06_WAIT;
			} else {
				musb->inEpReq.status = USBD_UPG_IN_04_WAIT;
			}
		}

		break;
	default:
		break;
	}
}

static uint32_t BUSBIrqProcess(usbMusbDcdDevice_t *musb)
{
	uint32_t ulStatus;
	uint32_t ulTmpEpInt = 0x00;

	//musb->ulIrqStatus |= USBIntStatusControl(musb->baseAddr);

	ulStatus = musb->ulIrqStatus;

	/* Received a reset from the host. */
	if (ulStatus & USB_INTCTRL_RESET) {
		return 0;
	}

	/* USB device was disconnected. */
	if (ulStatus & USB_INTCTRL_DISCONNECT) {
		musb->Ep0Req.status = USB_MUSB_STATE_IDLE;
		return 0;
	}

	/* Handle end point 0 interrupts. */
	if (musb->epIrqStatus & USB_INTEP_0) {
		usbMusbDcdEp0EvntHandler(musb);
	}

	ulTmpEpInt = musb->epIrqStatus;
	/* Other endpoint and device interrupts */
	if (ulTmpEpInt & 0xFFFEFFFE) {
		// msc
		musb->epIrqStatus = ulTmpEpInt & 0xFFFE;
		if (musb->epIrqStatus != 0x00)
			usbMusbDcdEpEvntHandler_IN(musb);
		musb->epIrqStatus = ulTmpEpInt & 0xFFFE0000;
		if (musb->epIrqStatus != 0x00)
			usbMusbDcdEpEvntHandler_OUT(musb);
	}

	return 0;
} /* USBMusbDcdIntrHandler */

static int BUSBIrqRead(usbMusbDcdDevice_t *musb)
{
	uint32_t ulBase = musb->baseAddr;
	musb->ulIrqStatus = 0x0;
	musb->epIrqStatus = USBIntStatusEndpoint(ulBase);
	musb->ulIrqStatus |= USBIntStatusControl(ulBase);

	return musb->epIrqStatus;
}

static void usb_setup_var(usbMusbDcdDevice_t *musb, int idx)
{
	memset(musb, 0, sizeof(musb));
	if (idx == 0)
		musb->baseAddr = USB0_BASE;
	else
		musb->baseAddr = USB1_BASE;
	musb->uiInstanceNo = idx;
	musb->pMSCDeviceDescriptor_RK = usbMSCDeviceDescriptor;
	musb->pMSCInterface_RK = usbMSCInterface;
}

static void usb_isr(uint32_t parameter)
{
	int irq;
	usbMusbDcdDevice_t *pmusbObj = (usbMusbDcdDevice_t *)parameter;

	irq = BUSBIrqRead(pmusbObj);
	BUSBIrqProcess(pmusbObj);
}

void usb_upgrade_process(void)
{
	usbMusbDcdDevice_t musbObj[2];

	usb_setup_var(&musbObj[0], 0);
	usb_setup_var(&musbObj[1], 1);

	BUSB_Init(&musbObj[0]);
	BUSB_Init(&musbObj[1]);

#if 1
	xPortInterruptInstallISR((int32_t)&USB0_MC_INTR, usb_isr, (uint32_t)&musbObj[0]);
	xPortInterruptInstallISR((int32_t)&USB1_MC_INTR, usb_isr, (uint32_t)&musbObj[1]);

	while (1) {
		usleep(100000);
	}
#else
	while (1) {
		int irq;
		irq = BUSBIrqRead(&musbObj[0]);
		irq |= BUSBIrqRead(&musbObj[1]);
		BUSBIrqProcess(&musbObj[0]);
		BUSBIrqProcess(&musbObj[1]);
		portYIELD();
	}
#endif
}
