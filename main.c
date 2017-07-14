/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/ioctl.h>
#include <getopt.h>

#define ALLOC_SIZE 64

static void print_received_data(int i2c_address,const struct i2c_msg *rd_msg)
{
    printf("Received data from slave 0x%X:\n", i2c_address);
    int i = 0;
    while (1) {
        printf ("0x%X", (unsigned int)rd_msg->buf[i]);
        i++;
        if (i < rd_msg->len) {
            printf(",");
        }
        else {
            break;
        }
    }
    putchar('\n');
}

static void add_i2c_read_data(int i2c_address, struct i2c_msg *rd_msg, size_t number_of_data_to_read)
{
    rd_msg->addr = i2c_address;
    rd_msg->flags = I2C_M_RD;
    rd_msg->buf = malloc(number_of_data_to_read);
    memset(rd_msg->buf, 0, number_of_data_to_read);
    rd_msg->len = number_of_data_to_read;
}

static int add_i2c_write_data(int i2c_address, struct i2c_msg *wr_msg, const char *data_to_write)
{
    wr_msg->addr = i2c_address;
    wr_msg->flags = 0;
    int i = 0;

    wr_msg->buf = malloc(sizeof(char) * ALLOC_SIZE);

    const char *parse_head = data_to_write;
    char *parse_tail;

    while (1) {
        if (i >= ALLOC_SIZE) {
            wr_msg->buf = realloc(wr_msg->buf, (i/ALLOC_SIZE + 1) * ALLOC_SIZE * sizeof(char));
        }

        if (wr_msg->buf == 0) {
            printf("Run out of memory\n");
            return -1;
        }

        wr_msg->buf[i] = (unsigned char)strtol(parse_head, &parse_tail, 0); // Parses till end of string or no number if parse_tail is 0

        i++;
        // if end of string reached, or next address is invalid
        if (parse_tail == 0 || *parse_tail == 0) {
            break;
        }

        parse_head = parse_tail + 1;
    }

    wr_msg->len = i;
    return 0;
}

static int open_i2c_device(const char *i2c_character_device)
{
    int fd = open(i2c_character_device, O_RDWR);

    return fd;
}

static int i2c_detect(const char *i2c_character_device)
{
    int fd = open_i2c_device(i2c_character_device);
    int i;
    unsigned long timeout = 50; /* 50 * 10ms */
    struct i2c_msg rdwr_msg;
    char devices[512] = "";

    if (fd < 0) {
        printf("Can not open i2c device %s\n", i2c_character_device);
        return 20;
    }

    memset (&rdwr_msg, 0, sizeof(rdwr_msg));
    if (ioctl (fd, I2C_TIMEOUT, timeout) != 0) {
        printf("Can not set timeout\n");
        return 30;
    }

    printf("Check address:\n");
    for (i = 1;i < 0x7F; i++) {
        add_i2c_read_data(i, &rdwr_msg, 1);

        struct i2c_rdwr_ioctl_data rdwr_set;

        rdwr_set.msgs = &rdwr_msg;
        rdwr_set.nmsgs = 1;
        printf("\r0x%02X ", i);
        fflush(stdout);
        if (ioctl (fd, I2C_RDWR, &rdwr_set) >= 0) {
            snprintf(devices, sizeof(devices), "%s 0x%02X ", devices, i);
        }
    }
    printf("\n");
    printf("Found devices at addresses:\n");
    printf("%s\n", devices);
    return 0;
}

static void usage(void)
{
    printf ("\ni2ctool [OPTIONS...]\n\n"
            "Generate some traffic over i2c, this program should only used for debugging purposes\n\n"
            "  -d i2c Linux device (/dev/i2c-0)\n"
            "  -a i2c device address, the address of the i2c device on the bus\n"
            "  -w data to write, comma seperated bytes 0,1,2,3,...\n"
            "  -r count of data to read, write will be executed before read\n"
            "  -c detect devices on the bus\n"
            "  -h show this message\n\n");
}

int main(int argc, char** argv)
{
    int i2c_address = -1;
    int number_of_data_to_read = 0;
    char *data_to_write = 0;
    char *i2c_character_device = 0;
    int detect = 0;
    int c;

    opterr = 0;
    while ((c = getopt (argc, argv, "a:d:w:r:ch")) != -1) {
        switch (c)
        {
        case 'a':
            i2c_address = strtol(optarg, 0, 0);
            break;
        case 'd':
            i2c_character_device = optarg;
            break;
        case 'w':
            data_to_write = optarg;
            break;
        case 'r':
            number_of_data_to_read = strtol(optarg, 0, 0);
            break;
        case 'c':
            detect = 1;
            break;
        case 'h':
            usage();
            return 1;
        default:
            break;
        }
    }

    if (i2c_character_device == 0) {
        printf("Please specify a device\n");
        usage();
        return 3;
    }

    if (detect) {
        return i2c_detect(i2c_character_device);
    }

    if (i2c_address == -1) {
        printf("Please set a device address\n");
        usage();
        return 4;
    }

    if (number_of_data_to_read == 0 && data_to_write == 0) {
        printf("Please specify at least one transmission\n");
        usage();
        return 5;
    }

    int i2c_character_device_fd = open_i2c_device(i2c_character_device);
    if (i2c_character_device_fd <= 0) {
        printf("Can not open %s", i2c_character_device);
        return 6;
    }

    struct i2c_msg rdwr_msg[2];
    int msg_count = 0;

    memset (rdwr_msg, 0, sizeof(rdwr_msg));

    if (data_to_write != 0) {
        if (add_i2c_write_data(i2c_address, &rdwr_msg[msg_count], data_to_write) != 0) {
            goto cleanup;
        }
        msg_count ++;
    }

    if (number_of_data_to_read != 0) {
        add_i2c_read_data(i2c_address, &rdwr_msg[msg_count], number_of_data_to_read);
        msg_count ++;
    }

    struct i2c_rdwr_ioctl_data rdwr_set;

    rdwr_set.msgs = rdwr_msg;
    rdwr_set.nmsgs = msg_count;

    if(ioctl (i2c_character_device_fd, I2C_RDWR, &rdwr_set) == -1) {
        printf ("Read write transaction failed (bus %s, addr 0x%X)\n", i2c_character_device, i2c_address);
        goto cleanup;
    }

    if (number_of_data_to_read != 0) {
        print_received_data(i2c_address, &rdwr_msg[msg_count - 1]);
    }

cleanup:
    for (int i = 0; i < msg_count; i ++) {
        free (rdwr_msg[i].buf);
        rdwr_msg[i].buf = 0;
    }

    close(i2c_character_device_fd);
    i2c_character_device_fd = -1;

    return 0;
}

