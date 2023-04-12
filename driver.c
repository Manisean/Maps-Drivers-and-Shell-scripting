/*#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>*/

#define MAP_SIZE 4096

static char *map_buffer; // buffer to store the map data
static size_t map_len;   // current length of the map

static int mapdriver_open(struct inode *inode, struct file *filp)
{
    // allocate memory for the map buffer
    map_buffer = kmalloc(MAP_SIZE, GFP_KERNEL);
    if (!map_buffer)
        return -ENOMEM;

    // initialize the buffer to all zeros
    memset(map_buffer, 0, MAP_SIZE);

    // initialize the current length of the map
    map_len = 0;

    // set the file pointer's private data to the map buffer
    filp->private_data = map_buffer;

    return 0;
}

static int mapdriver_release(struct inode *inode, struct file *filp)
{
    // free the map buffer
    kfree(map_buffer);

    return 0;
}

static ssize_t mapdriver_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;

    // check if the current position is beyond the end of the map
    if (*f_pos >= map_len)
        goto out;

    // limit the count to the number of bytes remaining in the map
    if (*f_pos + count > map_len)
        count = map_len - *f_pos;

    // copy the map data to the user buffer
    if (copy_to_user(buf, map_buffer + *f_pos, count)) {
        retval = -EFAULT;
        goto out;
    }

    // update the file pointer's position
    *f_pos += count;
    retval = count;

out:
    return retval;
}

static ssize_t mapdriver_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;

    // check if the write would exceed the map buffer size
    if (*f_pos + count > MAP_SIZE)
        count = MAP_SIZE - *f_pos;

    // copy the user data to the map buffer
    if (copy_from_user(map_buffer + *f_pos, buf, count)) {
        retval = -EFAULT;
        goto out;
    }

    // update the current length of the map if necessary
    if (*f_pos + count > map_len)
        map_len = *f_pos + count;

    // update the file pointer's position
    *f_pos += count;
    retval = count;

out:
    return retval;
}

static loff_t mapdriver_llseek(struct file *filp, loff_t off, int whence)
{
    loff_t newpos;

    switch (whence) {
    case SEEK_SET:
        newpos = off;
        break;
    case SEEK_CUR:
        newpos = filp->f_pos + off;
        break;
    case SEEK_END:
        newpos = map_len + off;
        break;
    default:
        return -EINVAL;
   
