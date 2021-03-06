/*
 * generic SPI functions
 *
 * Copyright 2013, 2014 Zefir Kurtisi <zefir.kurtisi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#include "spi-context.h"

#include "logging.h"
#include "miner.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>


int fp_uart;
	



	
	
	
	
int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;
    if  ( tcgetattr( fd,&oldtio)  !=  0) 
    { 
        //perror("SetupSerial 1");
        return -1;
    }
    bzero( &newtio, sizeof( newtio ) );
    newtio.c_cflag  |=  CLOCAL | CREAD; 
    newtio.c_cflag &= ~CSIZE; 

    switch( nBits )
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    }

    switch( nEvent )
    {
    case 'O':                     //奇校验
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'E':                     //偶校验
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'N':                    //无校验
        newtio.c_cflag &= ~PARENB;
        break;
    }

switch( nSpeed )
    {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    default:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }
    if( nStop == 1 )
    {
        newtio.c_cflag &=  ~CSTOPB;
    }
    else if ( nStop == 2 )
    {
        newtio.c_cflag |=  CSTOPB;
    }
    newtio.c_cc[VTIME]  = 0;
    newtio.c_cc[VMIN] = 0;
    tcflush(fd,TCIFLUSH);
    if((tcsetattr(fd,TCSANOW,&newtio))!=0)
    {
        return -1;
    }
    return 0;
}
	
	
	
	
	
	
	
	

struct spi_ctx *spi_init(struct spi_config *config)
{
	char dev_fname[PATH_MAX];
	
	unsigned char cmdrst_tx[2];
	
	struct spi_ctx *ctx;
	char tt = 0x5a;
	
	
	if (config == NULL)
		return NULL;
	config->speed = 500000;
		
		
	sprintf(dev_fname, SPI_DEVICE_TEMPLATE, config->bus, config->cs_line);
	fp_uart = open("/dev/ttyAMA0",O_RDWR);
	if (fp_uart < 0) {
		applog(LOG_ERR, "uart: Can not open uart device /dev/ttyAMA0");
		return NULL;
	} else
		applog(LOG_ERR, "uart: open uart device /dev/ttyAMA0");
	//test uart read and write
	set_opt(fp_uart,115200, 8, 'N', 1);
	int i;
	//for(i=0;i<0xff;i++)
	//write(fp_uart , &tt , 1);
	
	
	int fd = open(dev_fname, O_RDWR);
	if (fd < 0) {
		applog(LOG_ERR, "SPI: Can not open SPI device %s", dev_fname);
		return NULL;
	}
	int mode = 0;
	if ((ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) ||
	    (ioctl(fd, SPI_IOC_RD_MODE, &mode) < 0) ||
	    (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &config->bits) < 0) ||
	    (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &config->bits) < 0) ||
	    (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &config->speed) < 0) ||
	    (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &config->speed) < 0)) {
		applog(LOG_ERR, "SPI: ioctl error on SPI device %d", config->mode);
		applog(LOG_ERR, "SPI: ioctl error on SPI device %s", dev_fname);
		close(fd);
		return NULL;
	}
	struct spi_ioc_transfer spi_send_data;
	
	unsigned char send_data_tx[2];
	unsigned char send_data_rx[2];
	//reset chip here
	send_data_tx[0] = 0xff;		// 'b10111111&address  
	send_data_tx[1] = 0xff;
	spi_send_data.tx_buf = (unsigned long)send_data_tx;
    spi_send_data.rx_buf = (unsigned long)send_data_rx;
    spi_send_data.len = ARRAY_SIZE(send_data_tx);
    spi_send_data.delay_usecs = 10;
    spi_send_data.speed_hz = config->speed;
    spi_send_data.bits_per_word = config->bits;
	ioctl(fd, SPI_IOC_MESSAGE(1), &spi_send_data);
	
	//delay(100);
	
	//write PD
	//printf("config pll\n");
	send_data_tx[0] = 0xad;		// 'b10111111&address  
	cmdrst_tx[1] = 0x9d;
//#endif
	spi_send_data.tx_buf = (unsigned long)send_data_tx;
    spi_send_data.rx_buf = (unsigned long)send_data_rx;
    spi_send_data.len = ARRAY_SIZE(send_data_tx);
    spi_send_data.delay_usecs = 10;
    spi_send_data.speed_hz = config->speed;
    spi_send_data.bits_per_word = config->bits;
	ioctl(fd, SPI_IOC_MESSAGE(1), &spi_send_data);


	applog(LOG_ERR, "SPI send %x", send_data_tx[0]);
	applog(LOG_ERR, "SPI send %x", send_data_tx[1]);
	
	send_data_tx[0] = 0xad;		// 'b10111111&address  
	cmdrst_tx[1] = 0x1d;
	


	spi_send_data.tx_buf = (unsigned long)send_data_tx;
    spi_send_data.rx_buf = (unsigned long)send_data_rx;
    spi_send_data.len = ARRAY_SIZE(send_data_tx);
    spi_send_data.delay_usecs = 10;
    spi_send_data.speed_hz = config->speed;
    spi_send_data.bits_per_word = config->bits;
	ioctl(fd, SPI_IOC_MESSAGE(1), &spi_send_data);
	
	applog(LOG_ERR, "SPI send %x", send_data_tx[0]);
	applog(LOG_ERR, "SPI send %x", send_data_tx[1]);
	
	//read reg 63
	send_data_tx[0] = 0x7f;		// 'b10111111&address  
	send_data_tx[1] = 0x01;	
	
	spi_send_data.tx_buf = (unsigned long)send_data_tx;
    spi_send_data.rx_buf = (unsigned long)send_data_rx;
    spi_send_data.len = 2;
    spi_send_data.delay_usecs = 10;
    spi_send_data.speed_hz = config->speed;
    spi_send_data.bits_per_word = config->bits;
	ioctl(fd, SPI_IOC_MESSAGE(1), &spi_send_data);

	
	applog(LOG_ERR, "SPI send %x", send_data_tx[0]);
	applog(LOG_ERR, "SPI send %x", send_data_tx[1]);
	
	ctx = malloc(sizeof(*ctx));
	assert(ctx != NULL);

	ctx->fd = fd;
	ctx->config = *config;
	ctx->config.mode = 0;
	applog(LOG_WARNING, "SPI '%s': mode=%hhu, bits=%hhu, speed=%u",
	       dev_fname, ctx->config.mode, ctx->config.bits,
	       ctx->config.speed);
	applog(LOG_ERR, "SPI fd is  %x", ctx->fd);
	
	return ctx;
}

extern void spi_exit(struct spi_ctx *ctx)
{
	if (NULL == ctx)
		return;

	close(ctx->fd);
	free(ctx);
}


extern bool spi_encode(){ 		//encode package


}

extern bool spi_decode(){		//decode package
}

extern bool spi_transfer(struct spi_ctx *ctx, uint8_t *txbuf,
			 uint8_t *rxbuf, int len)
{
	struct spi_ioc_transfer xfr;
	int ret;

	if (rxbuf != NULL)
		memset(rxbuf, 0xff, len);

	ret = len;
	
	
	xfr.tx_buf = (unsigned long)txbuf;
	xfr.rx_buf = (unsigned long)rxbuf;
	xfr.len = len;
	xfr.speed_hz = ctx->config.speed;
	xfr.delay_usecs = ctx->config.delay;
	xfr.bits_per_word = ctx->config.bits;
	xfr.cs_change = 0;
	xfr.pad = 0;

	ret = ioctl(ctx->fd, SPI_IOC_MESSAGE(1), &xfr);
	int i;
	/*for(i=0;i<len;i++){
			applog(LOG_ERR, "SPI send %x", *(txbuf+i));
	}
	*/
	
	if (ret < 1)
		applog(LOG_ERR, "SPI: ioctl error on SPI device: %d", ret);

	return ret > 0;
}
