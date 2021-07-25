/*
 *   pps-gpio-dtless -- PPS client driver without devicetree
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#define pr_fmt(fmt) "pps-gpio-dtless" ": " fmt

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pps_kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>


#define DEV_NAME            "PPS GPIO Device"
#define GPIO_IRQ_NAME       "PPS GPIO IRQ"
#define IRQF_TRIGGER_BOTH   (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)

static int gpio = 364;                  // default to GPIO 11 on the Odyssey x86
static int assert_falling_edge = 1;     // assert the rising edge by default

module_param(gpio, int, S_IRUSR);
MODULE_PARM_DESC(gpio, "PPS GPIO");
module_param(assert_falling_edge, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(assert_falling_edge, "PPS GPIO edge to assert: 0 = falling, 1 = rising");

static struct pps_device *pps;
static int irq = 0;


static irqreturn_t pps_gpio_irq_handler(int irq, void *data)
{
    struct pps_event_time ts;
    int rising_edge;
 
    pps_get_ts(&ts);
 
    rising_edge = gpio_get_value(gpio);
    if ((rising_edge && !assert_falling_edge) || (!rising_edge && assert_falling_edge))
        pps_event(pps, &ts, PPS_CAPTUREASSERT, NULL);
    else if ((rising_edge && assert_falling_edge) || (!rising_edge && !assert_falling_edge))
        pps_event(pps, &ts, PPS_CAPTURECLEAR, NULL);
 
    return IRQ_HANDLED;
}

static int pps_gpio_register(void)
{
    int ret, pps_default_params;
    struct pps_source_info info = {
        .name   = KBUILD_MODNAME,
        .path   = "",
        .mode   = PPS_CAPTUREASSERT | PPS_OFFSETASSERT | \
                  PPS_CAPTURECLEAR | PPS_OFFSETCLEAR | \
                  PPS_CANWAIT | PPS_TSFMT_TSPEC,
        .owner  = THIS_MODULE,
        .dev    = NULL
    };

    /* GPIO setup */
    ret = gpio_request(gpio, "PPS");
    if (ret) {
        pr_warn("Failed to request PPS GPIO %u\n", gpio);
        return -EINVAL;
    }   
    ret = gpio_direction_input(gpio);
    if (ret) {
        pr_warn("Failed to set PPS GPIO direction\n");
        gpio_free(gpio);
        return -EINVAL;
    }

    /* map GPIO to IRQ */
    irq = gpio_to_irq(gpio);
    if(irq  < 0) {
        pr_warn("Failed to get IRQ for GPIO\n");
        gpio_free(gpio);
        return -EINVAL;
    }

    /* register as PPS source */
    pps_default_params = PPS_CAPTUREASSERT | PPS_OFFSETASSERT;
    pps = pps_register_source(&info, pps_default_params);
    if (pps == NULL) {
        pr_err("Failed to register GPIO PPS source\n");
        gpio_free(gpio);
        return -EINVAL;
    }

    /* register IRQ */
    ret = request_irq(irq, pps_gpio_irq_handler, IRQF_TRIGGER_BOTH, GPIO_IRQ_NAME, DEV_NAME);
    if (ret) {
        pr_warn("Failed to register IRQ\n");
        pps_unregister_source(pps);
        gpio_free(gpio);
        return -EINVAL;
    }

    pr_info("Registered GPIO %d as PPS source\n", gpio);
    return 0;
}

static int pps_gpio_remove(void)
{
    free_irq(irq, DEV_NAME);
    gpio_free(gpio);
    pps_unregister_source(pps);
    pr_info("Removed GPIO %d as PPS source\n", gpio);
    return 0;
}

static int __init pps_gpio_init(void)
{
    int ret = pps_gpio_register();
    if (ret < 0)
        return ret;
    return 0;
}

static void __exit pps_gpio_exit(void)
{
    pps_gpio_remove();
}

module_init(pps_gpio_init);
module_exit(pps_gpio_exit);

MODULE_AUTHOR("Joschka Kieser");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");