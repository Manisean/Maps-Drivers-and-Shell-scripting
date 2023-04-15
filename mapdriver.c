/****
 * ascii.c
 *
 * The ASCII character device driver implementation
 *
 * CREDITS:
 *   o Many parts of the driver code has to be credited to
 *     Ori Pomerantz, in his chardev.c (Copyright (C) 1998-1999)
 *
 *     Source:  The Linux Kernel Module Programming Guide (specifically,
 *              http://www.tldp.org/LDP/lkmpg/2.6/html/index.html)
 */

#include "mapdriver.h"
#include "team_gen.h"

#define BSIZE 1024
#define RESET_MAP _IO('k', 0)
#define ZERO_OUT_BUFFER _IO('k', 1)
#define CHECK_MAP _IO('k', 2)

/* Driver's Status is kept here */
static driver_status_t status =
{
	'a',   /* Starting ASCII char is '0' */
	false, /* Not busy at the beginning */
	{0},   /* buffer */
	NULL,  /* buffer's ptr */
	-1,    /* major */
	-1     /* minor */
};

char map[BSIZE*BSIZE + WIDTH] = {0};
void init_map(void) {
    size_t i, j, k;

    for (i = 0, k = 0; i < WIDTH; i++) {
        for (j = 0; j < HEIGHT; j++, k++) {
            map[k] = member_map[i][j];
        }
        map[k++] = '\n';
    }
    map[k] = '\0';
}


/* This function is called whenever a process
 * attempts to open the device file
 */
static int device_open(inode, file)
	struct inode* inode;
	struct file*  file;
{
	static int counter = 0;

#ifdef _DEBUG
	printk("device_open(%p,%p)\n", inode, file);
#endif

	/* This is how you get the minor device number in
	 * case you have more than one physical device using
	 * the driver.
	 */
	status.minor = inode->i_rdev >> 8;
	status.minor = inode->i_rdev & 0xFF;

	printk
	(
		"Device: %d.%d, busy: %d\n",
		status.major,
		status.minor,
		status.busy
	);

	/* We don't want to talk to two processes at the
	 * same time
	 */
	if(status.busy)
		return -EBUSY;

	/* If this was a process, we would have had to be
	 * more careful here.
	 *
	 * In the case of processes, the danger would be
	 * that one process might have check busy
	 * and then be replaced by the schedualer by another
	 * process which runs this function. Then, when the
	 * first process was back on the CPU, it would assume
	 * the device is still not open.
	 *
	 * However, Linux guarantees that a process won't be
	 * replaced while it is running in kernel context.
	 *
	 * In the case of SMP, one CPU might increment
	 * busy while another CPU is here, right after
	 * the check. However, in version 2.0 of the
	 * kernel this is not a problem because there's a lock
	 * to guarantee only one CPU will be kernel module at
	 * the same time. This is bad in  terms of
	 * performance, so version 2.2 changed it.
	 */

	status.busy = true;

	/* Initialize the message. */
	sprintf
	(
		status.buf,
		"If I told you once, I told you %d times - %s",
		counter++,
		"Hello, world\n"
	);

	/* The only reason we're allowed to do this sprintf
	 * is because the maximum length of the message
	 * (assuming 32 bit integers - up to 10 digits
	 * with the minus sign) is less than DRV_BUF_SIZE, which
	 * is 80. BE CAREFUL NOT TO OVERFLOW BUFFERS,
	 * ESPECIALLY IN THE KERNEL!!!
	 */

	status.buf_ptr = status.buf;

	return SUCCESS;
}


/* This function is called when a process closes the
 * device file.
 */
static int device_release(inode, file)
	struct inode* inode;
	struct file*  file;
{
#ifdef _DEBUG
	printk ("device_release(%p,%p)\n", inode, file);
#endif

	/* We're now ready for our next caller */
	status.busy = false;

	return SUCCESS;
}


/* This function is called whenever a process which
 * have already opened the device file attempts to
 * read from it.
 */
static ssize_t device_read(file, buffer, length, offset)
	struct file* file;
    char*        buffer;  /* The buffer to fill with data */
    size_t       length;  /* The length of the buffer */
    loff_t*      offset;  /* Our offset in the file */
{
	int bytes_read = 0;
    int map_size = strlen(map);
    int bytes_available = map_size - *offset;
    if (bytes_available <= 0) { 
        return 0;
    }
    if (bytes_available > length) {
        bytes_available = length;
    }

    // copy data from the map buffer to user buffer
    if (copy_to_user(buffer, map + *offset, bytes_available)) {
        return -EFAULT;
    }

    *offset += bytes_available; // update offset to reflect number of bytes read
    bytes_read = bytes_available;

    // if all bytes have been read, reset the buffer pointer to the beginning
    if (*offset >= map_size) {
        *offset = 0;
    }

    return bytes_read;
}


/* This function is called when somebody tries to write
 * into our device file.
 */
static ssize_t device_write(file, buffer, length, offset)
	struct file* file;
	const char*  buffer;  /* The buffer with the data to write */
    size_t       length;  /* The length of the buffer */
    loff_t*      offset;  /* Our offset in the file */
{
	int bytes_written = 0;

	/* Determine the current position in the buffer */
	char *curr_pos = status.buf + *offset;
	char *end_pos = status.buf + BSIZE*BSIZE;

	/* Check if writing beyond the end of the buffer */
	if (curr_pos + length > end_pos) {
		printk("ascii::device_write() - Write exceeds buffer size\n");
		return -EINVAL;
	}

	/* Write data to the buffer */
	while (length > 0) {
		get_user(*curr_pos++, buffer++);
		length--;
		bytes_written++;
	}

#ifdef _DEBUG
	printk("ascii::device_write() - Wrote %d bytes, %d left\n", bytes_written, length);
#endif

	/* Update the offset */
	*offset += bytes_written;

	/* Update the buffer length if necessary */
	if (*offset > length) {
		length = *offset;
	}

	/* Write functions are supposed to return the number
	 * of bytes actually written to the device
	 */
	return bytes_written;
}

long my_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        case RESET_MAP:
            // reset the map to its initial state
            memset(map, 0, sizeof(map));
            init_map();
            break;
        case ZERO_OUT_BUFFER:
            // zero out the entire buffer
            memset(status.buf, 0, sizeof(status.buf));
            status.buf_ptr = status.buf;
            break;
        case CHECK_MAP:
            {
                int i, len;
                bool error = false;

                // check byte length over width of the first line
                len = strcspn(map, "\n");
                if ((len % WIDTH) != 0) {
                    printk(KERN_INFO "Map is inconsistent: byte length over width of the first line does not yield an integer.\n");
                    error = true;
                }

                // check for non-printable ASCII characters
                for (i = 0; i < BSIZE; i++) {
                    if (map[i] == '\0') {
                        break;
                    }
                    if ((map[i] < 32) && (map[i] != '\n')) {
                        printk(KERN_INFO "Map is inconsistent: contains non-printable ASCII characters at position %d.\n", i);
                        error = true;
                    }
                }

                if (!error) {
                    printk(KERN_INFO "Map is consistent.\n");
                }

                break;
            }
        default:
            return -ENOTTY;  // ioctl command not recognized
    }

    return 0;
}

static loff_t device_lseek(struct file *file, loff_t offset, int whence)
{
    loff_t newpos = 0;

    switch (whence) {
        case 0: // SEEK_SET
            newpos = offset;
            break;

        case 1: // SEEK_CUR
            newpos = file->f_pos + offset;
            break;

        case 2: // SEEK_END
            newpos = strlen(map) + offset;
            break;

        default: // invalid whence
            return -EINVAL;
    }

    if (newpos < 0 || newpos > strlen(map))
        return -EINVAL;

    file->f_pos = newpos;
    return newpos;
}



/* Initialize the module - Register the character device */
int
init_module(void)
{
	/* Register the character device (atleast try) */
	status.major = register_chrdev
	(
		0,
		DEVICE_NAME,
		&Fops
	);

	/* Negative values signify an error */
	if(status.major < 0)
	{
		printk
		(
			"Sorry, registering the ASCII device failed with %d\n",
			status.major
		);

		return status.major;
	}

	printk
	(
		"Registeration is a success. The major device number is %d.\n",
		status.major
	);

	printk
	(
		"If you want to talk to the device driver,\n" \
		"you'll have to create a device file. \n" \
		"We suggest you use:\n\n" \
		"mknod %s c %d <minor>\n\n" \
		"You can try different minor numbers and see what happens.\n",
		DEVICE_NAME,
		status.major
	);

	return SUCCESS;
}


/* Cleanup - unregister the appropriate file from /proc */
void
cleanup_module(void)
{
	unregister_chrdev(status.major, DEVICE_NAME);
}

MODULE_LICENSE("GPL");

/* EOF */
