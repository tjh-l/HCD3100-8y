#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <nuttx/i2c/i2c_master.h>
#include <kernel/module.h>
#include <sys/unistd.h>
#include <kernel/lib/fdt_api.h>
#include <linux/slab.h>
#include <nuttx/fs/fs.h>
#include <linux/delay.h>
#include <kernel/elog.h>
#include <hcuapi/ms9601.h>

struct ms9601a_priv{
	uint8_t sel_rx_port_last;
	uint8_t sel_rx_port;
	uint8_t rx_5v_state;
	uint8_t input_clk;
	bool input_clk_4k_flag;
	bool eq_need_config_flag;
};

struct ms9601a_data{
	unsigned int i2c_addr;
	const char *i2c_devpath;
	int i2c_fd;
	void *priv;
};

static uint8_t ms9601a_read(struct ms9601a_data *ms9601a,uint8_t index);

static int ms9601a_i2c_init(struct ms9601a_data *ms9601a)
{
	if(ms9601a != NULL && ms9601a->i2c_devpath !=NULL){
		ms9601a->i2c_fd = open(ms9601a->i2c_devpath,O_RDWR);
		if(ms9601a->i2c_fd < 0){
			log_e("%s  i2c open fail\n\n",__func__);
			return -1;
		}
	}
	return 0;
}
static void ms9601a_write(struct ms9601a_data *ms9601a,uint8_t index, uint8_t value)
{
	uint8_t wdata[2] = {0,0};
	struct i2c_transfer_s xfer = {0};
	struct i2c_msg_s i2c_msg = {0};

	wdata[0] = index;
	wdata[1] = value;

	i2c_msg.addr = ms9601a->i2c_addr;
	i2c_msg.flags = 0x0;
	i2c_msg.buffer = wdata;
	i2c_msg.length = 2;

	xfer.msgc = 1;
	xfer.msgv = &i2c_msg;

	if (ioctl(ms9601a->i2c_fd, I2CIOC_TRANSFER, &xfer) < 0) {
		log_e("i2c write failed, 0x0f is 0x%x, 0x20 is 0x%x.\n",
				ms9601a_read(ms9601a,0x0f), ms9601a_read(ms9601a,0x20));
	}
}

static uint8_t ms9601a_read(struct ms9601a_data *ms9601a,uint8_t index)
{
	struct i2c_transfer_s xfer;
	struct i2c_msg_s i2c_msg[2] = {0};
	uint8_t wdata[2] = {0,0};

	wdata[0] = index;
	i2c_msg[0].addr = ms9601a->i2c_addr;
	i2c_msg[0].buffer = &wdata[0];
	i2c_msg[0].flags = 0;
	i2c_msg[0].length = 1;

	i2c_msg[1].addr = ms9601a->i2c_addr;
	i2c_msg[1].buffer = &wdata[1];
	i2c_msg[1].flags = 1;
	i2c_msg[1].length = 1;

	xfer.msgv = i2c_msg;
	xfer.msgc = 2;

	if (ioctl(ms9601a->i2c_fd, I2CIOC_TRANSFER, &xfer) < 0 || wdata[1]==0xff) {
		log_e ("i2c_read fd:%d-->addr %d 0x%x failed\n", ms9601a->i2c_fd,
				i2c_msg[0].addr, wdata[1]);
	}
	return wdata[1];
}

static void hdmi_tx_output_en(struct ms9601a_data *ms9601a, bool b_value)
{
	if(b_value)	{
		ms9601a_write(ms9601a,0x20,0x0f);
	}else{
		ms9601a_write(ms9601a,0x20,0x03);
	}
}

static void hdmi_rx_port_switch(struct ms9601a_data *ms9601a, uint8_t port)
{
	ms9601a_write(ms9601a, 0x0f, port);
}

static void hdmi_ldo_off(struct ms9601a_data *ms9601a)
{
	ms9601a_write(ms9601a, 0x36, 0x81);		
}

static int ms9601a_init(struct ms9601a_data *ms9601a)
{
	uint8_t chip_id = 0;

	ms9601a->i2c_fd = -1;

	if(ms9601a_i2c_init(ms9601a) < 0){
		return -1;
	}
	hdmi_ldo_off(ms9601a);

	chip_id = ms9601a_read(ms9601a,0x01);
	if(chip_id == 0x60){
		log_d("ms9601A chip valid\n");
	}else{
		log_e("ms9601A chip 0x%x not valid\n",chip_id);
	}
	hdmi_tx_output_en(ms9601a,0);
	return 0;
}

static void hdmi_port_next(struct ms9601a_data *ms9601a)					
{
	uint8_t i = 0;
	uint8_t rx_input = 0;
	struct ms9601a_priv *priv = NULL;
	uint8_t hot_code[] = {
		0x01, 0x02, 0x04, 0x08, 0x10
	};
	
	priv = ms9601a->priv;
	priv->sel_rx_port = priv->sel_rx_port_last;
	for (i = 0; i < MS9601_PORT_MAX; i++)
	{
		priv->sel_rx_port++;
		if (priv->sel_rx_port >= MS9601_PORT_MAX)
		{
			priv->sel_rx_port = 0;
		}   
		rx_input = hot_code[priv->sel_rx_port];
		if (rx_input & priv->rx_5v_state)
		{
			return;
		}
	}
	//All input ports are removed.
	priv->sel_rx_port_last = 3;	
	hdmi_rx_port_switch(ms9601a,0x03);
}

static  void hdmi_rx_det(struct ms9601a_data *ms9601a)					
{
	uint8_t u8_rx5v_state_temp = 0;
	uint8_t u8_rx5v_state = 0;
	struct ms9601a_priv *priv = NULL;
	int i = 0;

	priv = ms9601a->priv;
	u8_rx5v_state = ms9601a_read(ms9601a,0x51);

	if (u8_rx5v_state & 0x01)
	{
		u8_rx5v_state_temp |= (0x01 << 0);
	}
	if (u8_rx5v_state & 0x02)
	{
		u8_rx5v_state_temp |= (0x01 << 1);
	}
	if (u8_rx5v_state & 0x04)
	{
		u8_rx5v_state_temp |= (0x01 << 2);
	}
	/* Check the 5V change of the input port.*/
	if(priv->rx_5v_state != u8_rx5v_state_temp)
	{
		/* If a new input port id identfied.*/
		if ((~(priv->rx_5v_state)) & u8_rx5v_state_temp)		
		{   
			/* Obtain the newly added port and its corresponding number.*/
			u8_rx5v_state = (~(priv->rx_5v_state)) & u8_rx5v_state_temp;	
			for (i = 0; i < MS9601_PORT_MAX; i++)							
			{
				if (u8_rx5v_state & (0x01<<i))
					break;
			}
			log_d("hdmi rx port input.\n");
			priv->rx_5v_state = u8_rx5v_state_temp;			
		}
		/* If the selected input port is removed.*/
		else
		{
			log_d("hdmi rx port removed\n");
			/* Obtain the removed port and its corresponding number.*/
			u8_rx5v_state = priv->rx_5v_state & (~u8_rx5v_state_temp);	
			priv->rx_5v_state = u8_rx5v_state_temp;	
			if (u8_rx5v_state & (0x01 << priv->sel_rx_port_last))
			{
				hdmi_port_next(ms9601a);
			}
		}
	}
}

static void hdmi_input_switch(struct ms9601a_data *ms9601a)
{
	uint8_t i=0,u8_chn_num=0;
	struct ms9601a_priv *priv = NULL;
	uint8_t PARA_3_SEL[] = {
		0x00, //Port 0
		0x01, //Port 1
		0x02, //Port 2
	};
	
	priv = ms9601a->priv;
	if (priv->sel_rx_port_last != priv->sel_rx_port)
	{
		if(priv->sel_rx_port >= MS9601_PORT_MAX)
			return;
		priv->sel_rx_port_last = priv->sel_rx_port;
		i = priv->sel_rx_port;
		u8_chn_num = PARA_3_SEL[i];
		log_d("Switch input port.\n");
		hdmi_tx_output_en(ms9601a,0);
		/* Port switch must select null,then the delay is 120ms.*/
		hdmi_rx_port_switch(ms9601a,0x03);
		msleep(120);

		hdmi_rx_port_switch(ms9601a,u8_chn_num);
		hdmi_tx_output_en(ms9601a,1);
		priv->input_clk = 0;
	}
}

static void hdmi_clk_det(struct ms9601a_data *ms9601a)				
{
	uint8_t freqm_0,freqm_1,freqm_0_comp,freqm_1_comp;
	struct ms9601a_priv *priv = NULL;
	
	priv = ms9601a->priv;

	freqm_0 = ms9601a_read(ms9601a,0x07);
	freqm_1 = ms9601a_read(ms9601a,0x08);
	// Change clk detection mode,clk was detected to be 1/2 of normal.
	ms9601a_write(ms9601a,0x05,0x03);  
	msleep(5);
	freqm_0_comp = ms9601a_read(ms9601a,0x07);
	freqm_1_comp = ms9601a_read(ms9601a,0x08);
	msleep(1);
	// Restored normal clk dedetection mode.
	ms9601a_write(ms9601a,0x05,0x07);  
	//Clk not updated 
	if(freqm_1 == freqm_1_comp)
	{
		freqm_0 = 0;
		freqm_1 = 0;
	}
	if (abs(freqm_1 - priv->input_clk) >= 2)
	{
		priv->input_clk = freqm_1;

		if (freqm_1 == 0)
		{
			log_d("input_error_clk = %d\n",freqm_1);
			hdmi_tx_output_en(ms9601a,0);
			return;
		}
		log_d("input_clk = %d\n",freqm_1);

		if(freqm_1 > 72){
			priv->input_clk_4k_flag = 1;
			log_d("input timing is 4K\n");
		}
		else {
			priv->input_clk_4k_flag = 0;
			log_d("input timing is not 4K\n");
		}
		priv->eq_need_config_flag = 1;
	}
}

static void hdmi_eq_update(struct ms9601a_data *ms9601a)			
{
	uint8_t idx,val,offset;
	struct ms9601a_priv *priv = NULL;
	uint8_t EQ_Address[] = {
		0x11, 0x12, 0x13, 0x14, 0x16, 0x18, 0x19, 0x1E, 0x21, 0x22, 0x25, 0x26	//eq egister address
	};
	uint8_t PARA_3_EQ_4K[] = {
		0x2F, 0x00, 0x40, 0x00, 0x40, 0x00, 0x66, 0x0B, 0x7F, 0x79, 0x30, 0x51	//4K30_EQ
	};

	priv = ms9601a->priv;
	if (!priv->eq_need_config_flag){
		return;
	}
	priv->eq_need_config_flag = 0;
	/* Must close output before update eq&drive.*/
	hdmi_tx_output_en(ms9601a,0);
	for (idx = 0; idx < 12; idx ++){
		offset = EQ_Address[idx];
		val = PARA_3_EQ_4K[idx];
		ms9601a_write(ms9601a,offset, val);
	}
	hdmi_tx_output_en(ms9601a,1);
}

static void ms9601a_media_service(struct ms9601a_data *ms9601a)
{
	hdmi_rx_det(ms9601a);
	hdmi_input_switch(ms9601a);
	hdmi_clk_det(ms9601a);
	hdmi_eq_update(ms9601a);
}

static int ms9601a_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	struct inode *inode = filep->f_inode;
	struct ms9601a_data *ms9601a_data = inode->i_private;
	struct ms9601a_priv *priv = ms9601a_data->priv;
	
	switch (cmd) {
		case MS9601A_SET_CH_SEL:
			priv->sel_rx_port = arg;
			log_d("%s set 0x%p ch sel %ud\n", __func__,ms9601a_data, (unsigned int)arg);
			ms9601a_media_service(ms9601a_data);
			break;
		case MS9601A_SET_CH_AUTO:
			break;
		default:break;
	}

	return 0;
}

static int ms9601a_open(struct file *filep)
{
	struct inode *inode = filep->f_inode;
	struct ms9601a_data *ms9601a_data = inode->i_private;
	struct ms9601a_priv *priv = NULL;

	priv =(struct ms9601a_priv*)malloc(sizeof(struct ms9601a_priv));
	if(priv == NULL)
		return -ENOMEM;

	priv->eq_need_config_flag = 0;
	priv->input_clk_4k_flag = 0;
	priv->input_clk = 0;
	priv->rx_5v_state = 0;
	priv->sel_rx_port = 3;
	priv->sel_rx_port_last = 3;

	ms9601a_data->priv = priv;
	return 0;
}

static int ms9601a_close(struct file *filep)
{
	struct inode *inode = filep->f_inode;
	struct ms9601a_data *ms9601a_data = inode->i_private;
	struct ms9601a_priv *priv = ms9601a_data->priv;
	
	if(priv)
		free(priv);

	return 0;
}

static const struct file_operations ms9601a_fops = {
	.open 	= 	ms9601a_open,
	.close 	= 	ms9601a_close,
	.read 	= 	dummy_read,
	.write 	= 	dummy_write,
	.poll 	= 	NULL,
	.ioctl 	= 	ms9601a_ioctl
};

static int ms9601a_probe(const char *node)
{	
	int 		np;
	const char 	*status;
	struct ms9601a_data *ms9601a = NULL;

	np = fdt_get_node_offset_by_path(node);
	if (np < 0) {
		log_e("Can't find node ms9601a in dts\n");
		return -ENOENT;
	}

	ms9601a = kzalloc(sizeof(struct ms9601a_data), GFP_KERNEL);
	if (ms9601a == NULL)
		return -ENOMEM;

	if (!fdt_get_property_string_index(np, "status", 0, &status) &&
			!strcmp(status, "disabled"))
		return -ENOENT;
	if (fdt_get_property_u_32_index(np, "i2c-addr", 0, (u32 *)&ms9601a->i2c_addr))
		return -ENOENT;
	if (fdt_get_property_string_index(np, "i2c-devpath", 0, &ms9601a->i2c_devpath))
		return -ENOENT;

	if(ms9601a_init(ms9601a) < 0){
		return -ENOENT;
	}
	register_driver("/dev/ms9601a", &ms9601a_fops, 0666, ms9601a);	
	return 0;
}

static int hdmi_switch_ms9601a_init(void)
{
	ms9601a_probe("/hcrtos/ms9601a");
	return 0;
}
module_driver(ms9601, hdmi_switch_ms9601a_init, NULL, 2)

