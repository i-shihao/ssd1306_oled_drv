#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
struct i2c_data { 
        struct i2c_client *client; 
        struct device dev; 
	u8 *framebuffer;
}; 

static int ssd_displayinit(struct i2c_data *id)
{
	dev_info(&id->dev, "Init sequence complete\n");
	return 0;
}

static int ssd1306_probe(struct i2c_client *client)
{
	struct i2c_data *id;
	struct device *dev = &client->dev;
	
	id = devm_kzalloc(dev, sizeof(*id), GFP_KERNEL);

	if (!id)
		return -ENOMEM;


	/* set i2c_data */
	id->client = client;

	i2c_set_clientdata(client, id);

	/* allocate framebuffer */
	id->framebuffer = devm_kzalloc(dev, 1024, GFP_KERNEL);
	
	if (!id->framebuffer)
				return -ENOMEM;
	
	int ret = ssd_displayinit(id);

	if (ret < 0)
		return -EINVAL;
	
	dev_info(dev, "Device ready\n");
	return 0;
}

static void ssd1306_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	
	dev_info(dev, "Driver removed\n");
	return;
}

static void ssd1306_shutdown(struct i2c_client *client)
{
	pr_info("Shutting down!\n");
	return;
}

static const struct of_device_id ssd1306_of_match[] = {
    { .compatible = "solomon,ssd1306-i2c" },
    { }
};
MODULE_DEVICE_TABLE(of, ssd1306_of_match);

static const struct i2c_device_id ssd1306_idtable[] = {
	{ "solomon,ssd1306-i2c"},
	{ }
};

MODULE_DEVICE_TABLE(i2c, ssd1306_idtable);

static struct i2c_driver ssd1306_driver = {
	.driver = {
		.name = "ssd1306",
		.pm = NULL,
		.of_match_table = ssd1306_of_match,
	},
	.id_table = ssd1306_idtable ,
	.probe = ssd1306_probe ,
	.remove = ssd1306_remove,
	.shutdown = ssd1306_shutdown
};

static int __init ssd1306_init(void)
{
	int ret;
	
	ret = i2c_add_driver(&ssd1306_driver);

	return ret;
}

static void __exit ssd1306_exit(void)
{
	return i2c_del_driver(&ssd1306_driver);
}

module_init(ssd1306_init);
module_exit(ssd1306_exit);
MODULE_AUTHOR("Shi Hao <i.shihao.999@gmail.com>");
MODULE_DESCRIPTION("A simple ssd1306 OLED (128*64) display driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ssd1306");

