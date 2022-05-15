#include <linux/init.h>   // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h> // Core header for loading LKMs into the kernel
#include <linux/device.h> // Header to support the kernel Driver Model
#include <linux/kernel.h> // Contains types, macros, functions for the kernel
#include <linux/fs.h>     // Header for the Linux file system support
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/uaccess.h>

#define DEVICE_VID 0x0483
#define DEVICE_PID 0xff11
#define PACKET_LEN 0x40

const struct usb_device_id usbd_table[] = {
  {USB_DEVICE(DEVICE_VID, DEVICE_PID)}, 
  {}
};

MODULE_DEVICE_TABLE(usb, usbd_table);

struct usbd_device
{
  struct usb_device *dev;
  struct usb_interface *interface;
  __u8 *buffer;
  __u8 bulk_in_addr;
  __u8 bulk_out_addr;
};

struct usbd_device *usbd;

static int usbd_open(struct inode *i, struct file *filp)
{
  if(!usbd)
    return -ENODEV;
  return 0;
}

static int usbd_close(struct inode *i, struct file *filp)
{
  return 0;
}

static ssize_t usbd_read(struct file *filp, char *buff, size_t len, loff_t *off)
{
  int ret, cnt = 0;

  if(!usbd)
    return -ENODEV;

  memset(usbd->buffer,0,PACKET_LEN);

  ret = usb_bulk_msg(usbd->dev, usb_rcvbulkpipe(usbd->dev, usbd->bulk_in_addr), usbd->buffer, PACKET_LEN, &cnt, 0);

  if (ret)
  {
    //printk(KERN_ALERT "USBD BULK READ FAILED!, ERR: %d\n", ret);
    return ret;
  }

  if(len > PACKET_LEN)
    len = PACKET_LEN;

  if (copy_to_user(buff, usbd->buffer, len))
  {
    return -EFAULT;
  }
  //printk(KERN_ALERT "USBD READ %d bytes\n", cnt);
  return cnt;
}

static ssize_t usbd_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
  int ret, cnt;

  if(!usbd)
    return -ENODEV;

  memset(usbd->buffer,0,PACKET_LEN);

  if(len > PACKET_LEN)
    len = PACKET_LEN;

  if (copy_from_user(usbd->buffer, buff, len))
  {
    return -EFAULT;
  }

  ret = usb_bulk_msg(usbd->dev, usb_sndbulkpipe(usbd->dev, usbd->bulk_out_addr), usbd->buffer, PACKET_LEN, &cnt, 0);

  

  if (ret)
  {
    //printk(KERN_ALERT "USBD BULK WRITE FAILED!, ERR: %d\n", ret);
    return ret;
  }

  //printk(KERN_ALERT "USBD WROTE %d bytes\n",cnt);
  return cnt;
}

static struct file_operations usbd_fops = {
    .owner = THIS_MODULE,
    .open = usbd_open,
    .release = usbd_close,
    .read = usbd_read,
    .write = usbd_write,
};

static struct usb_class_driver usbd_class = {
  .name = "usbd",
  .fops = &usbd_fops,
};



static int usbd_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
  int i, ret;
  struct usb_host_interface *iface_desc;

  struct usb_endpoint_descriptor *ep;

  usbd = kzalloc(sizeof(struct usbd_device),GFP_KERNEL);

  if(!usbd)
  {
    return -ENOMEM;
  }

  usbd->dev = usb_get_dev(interface_to_usbdev(interface));
  usbd->interface = interface;
  usbd->buffer = kmalloc(PACKET_LEN,GFP_KERNEL);

  printk(KERN_ALERT "device address : %u\n",usbd->dev->devaddr);

  if(!usbd->buffer)
  {
    kfree(usbd);
    return -ENOMEM;
  }

  iface_desc = interface->cur_altsetting;

  for (i = 0; i < iface_desc->desc.bNumEndpoints; i++)
  {
    ep = &iface_desc->endpoint[i].desc;
    if (usb_endpoint_is_bulk_in(ep))
    {
      usbd->bulk_in_addr = ep->bEndpointAddress;
      printk(KERN_ALERT "found ep in at %d", usbd->bulk_in_addr);
    }
    if (usb_endpoint_is_bulk_out(ep))
    {
      usbd->bulk_out_addr = ep->bEndpointAddress;
      printk(KERN_ALERT "found ep out at %d", usbd->bulk_out_addr);
    }
  }

  if ((ret = usb_register_dev(interface, &usbd_class)) < 0)
  {
    kfree(usbd->buffer);
    kfree(usbd);
    printk(KERN_ALERT "USBD PROBE FAILED!, CANNOT REGISTER DEVICE\n");
  }
  else
  {
    printk(KERN_ALERT "USBD PROBE SUCCESS! minor: %d\n", interface->minor);
  }

  return ret;
}

static void usbd_disconnect(struct usb_interface *interface)
{
  kfree(usbd->buffer);
  kfree(usbd);
  usbd = NULL;
  usb_deregister_dev(interface, &usbd_class);
  printk(KERN_ALERT "USBD DISCONNECT SUCCESS!\n");
}

static struct usb_driver usbd_driver = {
    .name = "usbd",
    .probe = usbd_probe,
    .disconnect = usbd_disconnect,
    .id_table = usbd_table,
};

static int __init usbd_init(void)
{
  int result;
  result = usb_register(&usbd_driver);
  if (result < 0)
  {
    printk(KERN_ALERT "error registering : usbd");
    return -1;
  }
  printk(KERN_ALERT "usbd: HZ-> %d \n",HZ);
  return 0;
}

static void __exit usbd_exit(void)
{
  usb_deregister(&usbd_driver);
}

module_init(usbd_init);
module_exit(usbd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ankit Sharma");
MODULE_DESCRIPTION("A simple linux usb driver");
MODULE_VERSION("0.1");
