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
        struct spi_device *spi;	
	struct device dev;
	struct gpio_desc *cs_gpiod;
	int irq;
};

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
	struct device *dev = &spi->dev;
	struct spi_data *sd = NULL; 
	sd = devm_kzalloc(dev, sizeof(*sd), GFP_KERNEL);
	
	if (!sd) {
		dev_info(dev, "kzalloc() error\n");
		return  -ENOMEM;
	}
	/* Attaching private data to spi_device */
	sd->spi = spi;
	sd->dev = spi->dev;
	spi_set_drvdata(spi,sd);


	return 1;
}

static const struct of_device_id oled_match_table [] ={
	{.compatible ="mycompany,spi-oled"},
	{.compatible ="mycompany,spi-oled"},
	{ }
};

MODULE_DEVICE_TABLE(of,oled_match_table);

static const  struct  spi_driver oled_drv = { 
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
	return 1 ;
} 

static void __exit spi_exit(void)
{
	return ;
}

module_init(spi_init);
module_exit(spi_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("<Shi Hao>");
MODULE_DESCRIPTION("Spi driver for 0.9inch OLED display <128*64>");
