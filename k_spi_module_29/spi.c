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

void toggel_dc( struct gpio_desc *dc)
{
	int value = gpiod_get_value(dc);
	gpiod_set_value(dc, !value);
}

void set_dc(struct gpio_desc *dc , int value) 
{
	gpiod_set_value(dc, value);
}


void  init_check( struct gpio_desc *dc)
{
	uint8_t h_line[128] = {0};
	uint8_t v_line[8] = {0};
	
	memset(v_line, 0XFF, sizeof(v_line));
	memset(h_line , 0xFF, sizeof(h_line));
	
	set_dc(dc ,CMD);

	/* vertical  display check */
	send_command(0x21);
	send_command(0x08);
	send_command(0x08);

	send_command(0x22);
	send_command(0x00);
	send_command(0x07);

	toggel_dc(dc);

	send_data(v_line);

	/* Horizontal display check */
	set_dc(dc, CMD);

	send_command(0x21);
	send_command(0x00);
	send_command(0x7F);

	send_command(0x22);
	send_command(0x03);
	send_command(0x03);

	toggel_dc(dc);

	send_data(h_line);

	/* One pixel display check */
	set_dc(dc, CMD);

	send_command(0x21);
	send_command(0x08);
	send_command(0x08);

	send_command(0x22);
	send_command(0x03);
	send_command(0x03);

	toggel_dc(dc);

	send_data(0x01);
}

void  clear_display(struct gpio_desc *dc)
{
	uint8_t total_size[1024] = {0};

	set_dc(dc, CMD);

	/* Set Horizontal addressing mode */
	send_command(0x20);
	send_command(0x00);

	/* Set page and column addresses */
	send_command(0x21);
	send_command(0x00);
	send_command(0x7F);

	send_command(0x22);
	send_command(0x00);
	send_command(0x07);

	toggel_dc(dc);

	send_data(total_size);
}

int  display_init( struct  spi_data *sd)
{ 
	/* Init protocol for SSD1360 */
	int dcv = gpiod_get_value(sd->dc);
	
	if (dcv > 0 ) 
		return EINVAL;

	send_command(0xAE);
	send_command(0xA8);
	send_command(0x3F); 
	send_command(0xD3);
	send_command(0x00);
	send_command(0x40);
	send_command(0xA0);
	send_command(0xC0);
	send_command(0xDA);
	send_command(0x12);
	send_command(0x80);
	send_command(0x81);
	send_command(0xA4);
	send_command(0xA6);
	send_command(0xD5);
	send_command(0x80);
	send_command(0x8D);
	send_command(0x14);
	send_command(0xAF);
	
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

	clear_display(sd->dc);
	dev_info(dev, "Display is ready\n");

	init_check(sd->dc);
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
