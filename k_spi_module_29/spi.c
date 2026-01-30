#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include "spi.h" 

#define CMD 0
#define DATA 1

struct spi_data {
        struct spi_device *spi;	
	struct device dev;
	struct gpio_desc *dc;
	struct gpio_desc *reset;
	struct gpio_desc *cs_gpiod;
	struct spi_controller  *ctrl;
	int irq;
};

int find_char(char * c )
{
	char aplhabet = *c;
	int index = *c - 32;

	return 0;
}

void toggel_dc( struct gpio_desc *dc)
{
	int value = gpiod_get_value(dc);
	gpiod_set_value(dc, !value);
}

void set_dc(struct gpio_desc *dc , int value) 
{
	gpiod_set_value(dc, value);
}

int spi_oled_write(struct spi_data *sd, const void *buff, size_t len)
{
	struct spi_transfer t = {};
	struct spi_message m;
	
	t.tx_buf = buff;
	t.len = len;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	int ret = spi_sync(sd->spi, &m);

	return ret;
}

void  send_data( struct spi_data *sd , uint8_t data)
{
	set_dc(sd->dc, DATA);

	uint8_t cmd = data;
	size_t len = sizeof(cmd);

	int  ret = spi_oled_write( sd, &cmd,len);
	
	if (ret < 0) {
		dev_err(&sd->dev, "spi_oled_write() error\n");
		return ;
	}

	return;
}

void send_command( struct spi_data *sd, uint8_t cmd)
{
	set_dc(sd->dc, CMD);

	uint8_t command =  cmd;
	size_t len = sizeof(command);

	int ret = spi_oled_write(sd, &command, len);
	
	if (ret < 0) {
		dev_err(&sd->dev, "spi_oled_write() error\n");
		return ;
	}
	return;
}

void send_buff(struct spi_data *sd, const void *buf, size_t len)
{
	set_dc(sd->dc, DATA);

	int ret = spi_oled_write(sd, buf, len);

	if (ret < 0) {
		dev_err(&sd->dev, "spi_oled_write() error\n");
		return;
	}

	return;
}


void set_address_mode(struct spi_data *sd,int mode)
{
	send_command(sd, 0x20);
	send_command(sd,mode);
	return ;
}

void set_col_page(struct spi_data *sd, int start_col, int end_col, int start_page, int end_page)
{
	set_address_mode(sd, DEF_ADDR_MODE);
	send_command(sd, 0x21);
	send_command(sd, start_col);
	send_command(sd, end_col);

	send_command(sd, 0x22);
	send_command(sd, start_page);
	send_command(sd, end_page);

	return;
}

static void  draw_glyph(struct spi_data *sd, char *string)
{

	int total_page = 8;
	int total_col = 127;

	int start_col = 0;
	static 	int current_col = 0;
	static int end_col ;

	int start_page = 0;
	static int end_page = 0;
	static int current_page = 0;
	
	while(*string) {

		if (end_col  > total_col) {
			current_col = 0;
			current_page++;
			end_page = current_page;
		}
		if ((end_col > total_col) && (end_page > total_page)) {
			current_col = 0;
			current_page = 0;
			end_col = 0;
			end_page = 0;
		}
		char c = *string;
		int index = *string - 32;

		/* set len based on SDD1306 font */
		int len = sizeof(SSD1306_font[index]);
		end_col = current_col +len -1;

		/* send font to display */
		send_command(sd, 0x21);
		send_command(sd, current_col);
		send_command(sd, end_col);

		send_command(sd, 0x22);
		send_command(sd, current_page);
		send_command(sd, end_page);
		send_buff(sd, SSD1306_font[index], 5);

		current_col = end_col + 1;
		string++;
	}
	
	return;
}

void animate( struct spi_data *sd)
{

	uint8_t  v_line[8] = {0};
	uint8_t v_off[8] = {0};
	memset(v_line, 0XFF, sizeof(v_line));
	memset(v_off, 0x00, sizeof(v_off)); 

	uint8_t one_c [] = { 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00 };
	
	uint8_t pami[] = {
                0x7F, 0x09, 0x09, 0x09, 0x06, 0x00, //P
                0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00, //A
                0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00, //M
                0x00, 0x41, 0x7F, 0x41, 0x00, 0x00, //I

                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // space 

                0x7F, 0x40, 0x40, 0x40, 0x40, 0x00, //L
                0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, //O
                0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00, //V
                0x7F, 0x49, 0x49, 0x49, 0x41, 0x00, //E

                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // space

                0x07, 0x08, 0x70, 0x08, 0x07, 0x00, //Y
                0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, //O
                0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, //U
        };

	uint8_t start = 0;
	uint8_t end = start + sizeof(pami) - 1; 

	/* set column and page address */
	for (int i = 0; i < 127; i++) {
		if (i > 0) {
		       	send_command(sd, 0x21);
       			send_command(sd, i-1);  // Clear the previous column
		        send_command(sd, end);
		       	send_command(sd, 0x22);
		        send_command(sd, 0x00);
		       	send_command(sd, 0x07);
       			send_buff(sd, v_off, 8);
		}	
		
		send_command(sd, 0x21);
	   	send_command(sd, i);
	 	send_command(sd, end++);
		send_command(sd, 0x22);
		send_command(sd, 0x00);
		send_command(sd, 0x07);
		send_buff(sd, pami, sizeof(pami)); 
		msleep(100);
	}
	pr_info(" animation complete\n");
	return ;
}

void  init_check( struct  spi_data * sd)
{
	uint8_t h_line[128] = {0};
	uint8_t v_line[8] = {0};
	
	memset(v_line, 0XFF, sizeof(v_line));
	memset(h_line , 0xFF, sizeof(h_line));
	

	/* vertical  display check */
	send_command(sd, 0x21);
	send_command(sd, 0x00);
	send_command(sd, 0x00);

	send_command(sd, 0x22);
	send_command(sd, 0x00);
	send_command(sd, 0x07);

	send_buff(sd, v_line, 8);

	/* Horizontal display check */
	send_command(sd, 0x21);
	send_command(sd, 0x00);
	send_command(sd, 0x7F);

	send_command(sd, 0x22);
	send_command(sd, 0x06);
	send_command(sd, 0x06);
	
	send_buff(sd, h_line, 128);

	/* One pixel display check */
	
	send_command(sd, 0x21);
	send_command(sd, 0x08);
	send_command(sd, 0x08);

	send_command(sd, 0x22);
	send_command(sd, 0x03);
	send_command(sd, 0x03);

	send_data(sd, 0x01);
}

void  clear_display(struct spi_data *sd)
{
	uint8_t total_size[1024] = {0};

	/* Set Horizontal addressing mode */
	send_command(sd, 0x20);
	send_command(sd, 0x00);

	/* Set page and column addresses */
	send_command(sd, 0x21);
	send_command(sd, 0x00);
	send_command(sd, 0x7F);

	send_command(sd, 0x22);
	send_command(sd, 0x00);
	send_command(sd, 0x07);

	send_buff(sd, total_size, 1024);
}

int  display_init( struct  spi_data *sd)
{ 
	/* Init protocol for SSD1360 */
	int dcv = gpiod_get_value(sd->dc);
	
	if (dcv > 0 ) 
		return EINVAL;

	send_command(sd,  0xAE);
	send_command(sd,  0xA8);
	send_command(sd,  0x3F); 
	send_command(sd,  0xD3);
	send_command(sd,  0x00);
	send_command(sd,  0x40);
	send_command(sd,  0xA0);
	send_command(sd,  0xC0);
	send_command(sd,  0xDA);
	send_command(sd,  0x12);
	send_command(sd,  0x80);
	send_command(sd,  0x81);
	send_command(sd,  0xA4);
	send_command(sd,  0xA6);
	send_command(sd,  0xD5);
	send_command(sd,  0x80);
	send_command(sd,  0x8D);
	send_command(sd,  0x14);
	send_command(sd,  0xAF);
	
	return 0;
} 	

void oled_shutdown(struct spi_device *spi)
{
	struct spi_data *sd = spi_get_drvdata(spi);
	
	/* send comman to power off display */
	send_command(sd, 0xAE);

	msleep(50);
	dev_info(&sd->dev, "oled display powered off\n");

	return;
}

void oled_remove(struct spi_device *spi)
{
	struct spi_data *sd = spi_get_drvdata(spi);

	if (!sd) 
		return;
	dev_info(&sd->dev, "oled driver removed \n");

	return ;
}

int oled_probe(struct spi_device *spi)
{
	int ret;
	struct device *dev = &spi->dev;
	struct spi_data *sd = NULL; 

	sd = devm_kzalloc(dev, sizeof(*sd), GFP_KERNEL);
	
	if (!sd) {
		dev_info(dev, "kzalloc() error\n");
		return  -ENOMEM;
	}

	/* Set data to spi_device */
	sd->spi = spi;
	sd->dev = spi->dev;
	sd->ctrl = spi->controller;


	/* set slave device data */ 
	spi->max_speed_hz = 400000;
	spi->bits_per_word = 8;
	spi->mode = 0; 
	spi_setup(spi);

	/* Get the gpios from device tree */
	sd->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	
	if (!sd->reset) {
		dev_err(dev, "reset gpiod_get() error\n");
		return -ENODEV;
	}

	sd->dc = devm_gpiod_get(dev, "dc", GPIOD_OUT_LOW);

	if(!sd->dc) {
		dev_err(dev, " dc gpiod_get() error\n");
		return -ENODEV;
	}

	spi_set_drvdata(spi,sd);

	/* Power on the display */
	gpiod_set_value(sd->reset, 1);
	
	msleep(100);

	gpiod_set_value(sd->reset, 0);

	msleep(100);

	gpiod_set_value(sd->dc, 0);

	ret = display_init(sd);

	if (ret < 0) {
		 dev_err(dev, "display_init() error\n");
		 return -ENODEV;
	}

	dev_info(dev, "init complete\n");

	clear_display(sd);
	dev_info(dev, "Display is ready\n");

	init_check(sd);
	dev_info(dev, "Init check successful\n");
	
	mdelay(100);
	clear_display(sd);

	dev_info(dev, "Draw_glyph()  \n");

	return 0;
}

static const struct of_device_id oled_match_table [] ={
	{.compatible ="mycompany,oled-1360"},
	{ }
};

MODULE_DEVICE_TABLE(of,oled_match_table);

static struct  spi_driver oled_drv = { 
	.driver = {
		.name = "spi_oled",
		.of_match_table = oled_match_table,
	},
	.probe = oled_probe,
	.remove= oled_remove,
	.shutdown = oled_shutdown,
	.id_table = NULL,

};

static int __init spi_init(void)
{
	int ret ;
	
	ret = spi_register_driver(&oled_drv);

	if (ret < 0) {
		pr_info("spi_register_driver() error\n");
		goto r_spi;
	}
	pr_info("oled drv loaded\n");

	return 0;
r_spi:
	spi_unregister_driver(&oled_drv);
	return -1;

} 

static void __exit spi_exit(void)
{
	spi_unregister_driver(&oled_drv);

	return ;
}

module_init(spi_init);
module_exit(spi_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("<Shi Hao>");
MODULE_DESCRIPTION("Spi driver for 0.9inch OLED display <128*64>");
