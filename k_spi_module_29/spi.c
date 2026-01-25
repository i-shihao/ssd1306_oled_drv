#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>

struct spi_data {
	struct spi_transfer *st;
        struct spi_device *spi;	
	struct device dev;
	struct gpio_desc *dc;
	struct gpio_desc *reset;
	struct gpio_desc *cs_gpiod;
	struct spi_controller  *ctrl;
	int irq;
};

int  display_init( struct  spi_data *sd)
{ 
	if (!sd) 
		return -ENODEV; 

	/* Initialize the display set o to res */
	gpiod_set_value(sd->reset, 1);

	/* Set the dc value to value to low  initially */
	gpiod_set_value(sd->dc, 0);

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

	ret = display_init(sd);

	if (ret < 0) {
		 dev_err(dev, "display_init() error\n");
		 return -ENODEV;
	}

	pr_info(" init complete\n");

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
