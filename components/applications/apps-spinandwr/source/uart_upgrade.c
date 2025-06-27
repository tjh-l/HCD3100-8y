#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <kernel/io.h>
#include <kernel/ld.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

typedef void (*kernel_entry_t)(void);

void run_code_rom(unsigned int tmpVal)
{
	unsigned int pbootentry;
	pbootentry = tmpVal;
	((kernel_entry_t)pbootentry)();
}

unsigned char BUart_Get_Data(void)
{
	unsigned char syncFlag = 0x00;
	unsigned char data;

	while (1) {
		syncFlag = *((volatile unsigned char *)0xB8818305);
		if (syncFlag & 0x01) {
			data = *((volatile unsigned char *)0xB8818300);
			return data;
		}
	}
}

int BUart_Peek_Data(unsigned char *data)
{
	unsigned char syncFlag = 0x00;

	syncFlag = *((volatile unsigned char *)0xB8818305);
	if (syncFlag & 0x01) {
		*data = *((volatile unsigned char *)0xB8818300);
		return 0;
	}

	return -1;
}

void BUart_Send_Data(unsigned char syncData)
{
	int i = 0;
	unsigned char syncFlag = 0x00;

	while (1) {
		syncFlag = *((volatile unsigned char *)0xB8818305);
		if (syncFlag & 0x20) {
			*((volatile unsigned char *)0xB8818300) = syncData;
			return;
		}
	}
}

void BUart_Exec_Code(unsigned int flagData)
{
	unsigned int tmpVal = 0x00;

	tmpVal |= BUart_Get_Data();
	tmpVal |= BUart_Get_Data() << 8;
	tmpVal |= BUart_Get_Data() << 16;
	tmpVal |= BUart_Get_Data() << 24;
	run_code_rom(tmpVal);
}

void BUart_RW_Data(unsigned int flagData, unsigned int wrFlag)
{
	unsigned int tmpVal = 0x00;
	volatile unsigned char *dAddr = 0x00;
	unsigned int i = 0x00;
	unsigned int iCnt = 0x00;

	tmpVal = BUart_Get_Data();
	tmpVal |= BUart_Get_Data() << 8;
	tmpVal |= BUart_Get_Data() << 16;
	tmpVal |= BUart_Get_Data() << 24;

	iCnt = BUart_Get_Data();
	iCnt |= BUart_Get_Data() << 8;
	iCnt |= BUart_Get_Data() << 16;
	iCnt |= BUart_Get_Data() << 24;

	dAddr = (volatile unsigned char *)tmpVal;
	if (wrFlag) {
		for (i = 0; i < iCnt; i++) {
			*dAddr++ = BUart_Get_Data();
		}
	} else {
		for (i = 0; i < iCnt; i++) {
			BUart_Send_Data(*dAddr++);
		}
	}
}

int BUart_Upgrade_Process(void)
{
	unsigned char tmpVal = 0x00;
	while (1) {
		if (BUart_Peek_Data(&tmpVal) < 0) {
			portYIELD();
			continue;
		}

		taskENTER_CRITICAL();
		switch (tmpVal & 0xF0) {
		case 0x60:
			BUart_Send_Data(0x61);
			break;
		case 0x50:
			BUart_Exec_Code(tmpVal);
			break;
		case 0x40:
			BUart_Send_Data(0x41);
			BUart_RW_Data(tmpVal, 0x1);
			break;
		case 0x30:
			BUart_Send_Data(0x31);
			BUart_RW_Data(tmpVal, 0x0);
			break;
		case 0xE0:
			taskEXIT_CRITICAL();
			return 0;
			break;
		default:
			break;
		}
		taskEXIT_CRITICAL();
	}
	return 0;
}

unsigned long baud_flag = 0x00;
void set_baudrate(void)
{
	baud_flag = *((volatile unsigned long *)0x80000200);
	switch (baud_flag) {
	case 843750: /* 843750 */
		*((unsigned int *)0xB8818300) = 0x1b010700;
		*((unsigned int *)0xB8818304) = 0x00306003;
		*((unsigned int *)0xB8818308) = 0x001f6308;
		*((unsigned int *)0xB881830c) = 0x00010000;
		*((unsigned int *)0xB8818314) = 0x0000002a;
		*((volatile unsigned char *)0xB8818303) = 0x83;
		*((volatile unsigned char *)0xB8818300) = 0x04;
		*((volatile unsigned char *)0xB8818303) = 0x03;
		*((volatile unsigned char *)0xB8818308) = 0x08;
		break;
	default: /* 115200 */
		*((unsigned int *)0xB8818300) = 0x1b010700;
		*((unsigned int *)0xB8818304) = 0x00306003;
		*((unsigned int *)0xB8818308) = 0x001f6300;
		*((unsigned int *)0xB881830c) = 0x00010000;
		*((unsigned int *)0xB8818314) = 0x0000002a;
		break;
	}
}

static void enable_uart_strap_pin(void)
{
	void *strap_pin_addr = (void *)&STRAP_PIN_CTRL;
	REG32_CLR_BIT(strap_pin_addr, BIT18); //disabled uart0 strap pin
	REG32_CLR_BIT(strap_pin_addr, BIT19); //disabled sflash csj strap pin
}

void uart_upgrade_process(void)
{
	enable_uart_strap_pin();
	set_baudrate();
	BUart_Upgrade_Process();

	return ;
}
