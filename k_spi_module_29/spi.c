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

int spi_transfer(struct spi_data *sd, const void *buff, ssize_t len)
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

int  send_data( struct spi_data *sd , uint8_t value)
{
	uint8_t cmd = value;
	ssize_t len = sizeof(value);
	int  ret = spi_transfer( sd, &cmd,len);
	
	if (ret < 0) {
		pr_info("spi_tranfer() error\n");
		return EINVAL;
	}
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


void  init_check( struct  spi_data * sd)
{
	uint8_t h_line[128] = {0};
	uint8_t v_line[8] = {0};
	
	memset(v_line, 0XFF, sizeof(v_line));
	memset(h_line , 0xFF, sizeof(h_line));
	
	set_dc(sd->dc ,CMD);

	/* vertical  display check */
	send_data(sd, 0x21);
	send_data(sd, 0x08);
	send_data(sd, 0x08);

	send_data(sd, 0x22);
	send_data(sd, 0x00);
	send_data(sd, 0x07);

	toggel_dc(sd->dc);

	send_data(sd,*v_line);

	/* Horizontal display check */
	set_dc(sd->dc, CMD);

	send_data(sd,0x21);
	send_data(sd, 0x00);
	send_data(sd, 0x7F);

	send_data(sd, 0x22);
	send_data(sd, 0x03);
	send_data(sd, 0x03);

	toggel_dc(sd->dc);

	send_data(sd, *h_line);

	/* One pixel display check */
	set_dc(sd->dc, CMD);

	send_data(sd, 0x21);
	send_data(sd, 0x08);
	send_data(sd, 0x08);

	send_data(sd, 0x22);
	send_data(sd, 0x03);
	send_data(sd, 0x03);

	toggel_dc(sd->dc);

	send_data(sd, 0x01);
}

void  clear_display(struct spi_data *sd)
{
	uint8_t total_size[1024] = {0};

	set_dc(sd->dc, CMD);

	/* Set Horizontal addressing mode */
	send_data(sd, 0x20);
	send_data(sd, 0x00);

	/* Set page and column addresses */
	send_data(sd, 0x21);
	send_data(sd, 0x00);
	send_data(sd, 0x7F);

	send_data(sd, 0x22);
	send_data(sd, 0x00);
	send_data(sd, 0x07);

	toggel_dc(sd->dc);

	send_data(sd ,*total_size);
}

int  display_init( struct  spi_data *sd)
{ 
	/* Init protocol for SSD1360 */
	int dcv = gpiod_get_value(sd->dc);
	
	if (dcv > 0 ) 
		return EINVAL;

	send_data(sd,  0xAE);
	send_data(sd,  0xA8);
	send_data(sd,  0x3F); 
	send_data(sd,  0xD3);
	send_data(sd,  0x00);
	send_data(sd,  0x40);
	send_data(sd,  0xA0);
	send_data(sd,  0xC0);
	send_data(sd,  0xDA);
	send_data(sd,  0x12);
	send_data(sd,  0x80);
	send_data(sd,  0x81);
	send_data(sd,  0xA4);
	send_data(sd,  0xA6);
	send_data(sd,  0xD5);
	send_data(sd,  0x80);
	send_data(sd,  0x8D);
	send_data(sd,  0x14);
	send_data(sd,  0xAF);
	
	return 0;
} 	

void oled_shutdown(struct spi_device *spi)
{
	return;
}

void oled_remove(struct spi_device *spi)
{
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
	
	udelay(3);

	gpiod_set_value(sd->reset, 0);

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
