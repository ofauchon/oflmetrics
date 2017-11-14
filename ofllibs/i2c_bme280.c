
#include <stdio.h>
#include <string.h>
#include <i2c.h>

#include "i2c_bme280.h"

//#include <uinstd.h>
/* 
 * Debug tools
 */

static uint8_t measurement_regs[8];
static bmx280_calibration_t cal; 
static int32_t t_fine ; // Fine resolution temperature value, also needed for pressure and humidity


/*
void delayMicroseconds(uint16_t d){
        uint16_t i;
        for (i=0; i< ((d - CORRECTION) * CYCLES_PER_US) ; i++) {
                asm("mov r0,r0"); 
        }

}
*/
void sleepms(uint16_t tm){
	uint16_t t; 
	for (t=0; t<tm; t++)
	{
			delayMicroseconds(1000);
	}

}

uint8_t write_u8_reg(uint8_t slave, uint8_t reg, uint8_t *data, uint8_t datalen){

	uint8_t ret=0; 

	uint8_t buf[1+datalen]; 
	buf[0]=reg;
	uint8_t ctr;  
	for (ctr=0; ctr<datalen; ctr++) buf[1+ctr]=data[ctr]; 
	i2c_transmitinit(slave, datalen + 1 , buf);
	sleepms(10);

	ret=datalen;
	return ret; 
}


uint8_t read_u8_reg(uint8_t slave, uint8_t reg, uint8_t *data, uint8_t datalen){

	uint8_t ret=0; 

//	printf("## Transmit slave addr\r\n");
	i2c_transmitinit(slave, 1, &reg);
	sleepms(10);
	//while (!i2c_transferred());
	
//	printf("## Receiveinit\r\n");
	i2c_receiveinit(slave, datalen, data);
	while (!i2c_transferred()) {
		sleepms(10);
	}

	ret=datalen;
	return ret; 
}

uint16_t get_uint16_le(const uint8_t *buffer, size_t offset)
{
    return (((uint16_t)buffer[offset + 1]) << 8) + buffer[offset];
}

int16_t get_int16_le(const uint8_t *buffer, size_t offset)
{
    return (((int16_t)buffer[offset + 1]) << 8) + buffer[offset];
}





/**
 * Read compensation data, 0x88..0x9F, 0xA1, 0xE1..0xE7
 *
 * This function reads all calibration bytes at once. These are
 * the registers DIG_T1_LSB (0x88) upto and including DIG_H6 (0xE7).
 */
int read_calibration_data(void)
{
    //TRACE("read_calibration_data START\r\n");
    uint8_t buffer[128];        /* 128 should be enough to read all calibration bytes */
    int nr_bytes;
    int nr_bytes_to_read = (BME280_DIG_H6_REG - BMX280_DIG_T1_LSB_REG) + 1;
    
    uint8_t offset = 0x88;
    TRACE("read_calibration_data: Reading %d calibration bytes \r\n", nr_bytes_to_read);

    memset(buffer, 0, sizeof(buffer));
    nr_bytes = read_u8_reg(BME280_I2C_ADDR, offset, buffer, nr_bytes_to_read);
    if (nr_bytes != nr_bytes_to_read) {
        printf("Unable to read calibration data\r\n");
        return -1;
    }

    cal.dig_T1 = get_uint16_le(buffer, BMX280_DIG_T1_LSB_REG - offset);
    cal.dig_T2 = get_int16_le(buffer, BMX280_DIG_T2_LSB_REG - offset);
    cal.dig_T3 = get_int16_le(buffer, BMX280_DIG_T3_LSB_REG - offset);
    cal.dig_P1 = get_uint16_le(buffer, BMX280_DIG_P1_LSB_REG - offset);
    cal.dig_P2 = get_int16_le(buffer, BMX280_DIG_P2_LSB_REG - offset);
    cal.dig_P3 = get_int16_le(buffer, BMX280_DIG_P3_LSB_REG - offset);
    cal.dig_P4 = get_int16_le(buffer, BMX280_DIG_P4_LSB_REG - offset);
    cal.dig_P5 = get_int16_le(buffer, BMX280_DIG_P5_LSB_REG - offset);
    cal.dig_P6 = get_int16_le(buffer, BMX280_DIG_P6_LSB_REG - offset);
    cal.dig_P7 = get_int16_le(buffer, BMX280_DIG_P7_LSB_REG - offset);
    cal.dig_P8 = get_int16_le(buffer, BMX280_DIG_P8_LSB_REG - offset);
    cal.dig_P9 = get_int16_le(buffer, BMX280_DIG_P9_LSB_REG - offset);
    cal.dig_H1 = buffer[BME280_DIG_H1_REG - offset];
    cal.dig_H2 = get_int16_le(buffer, BME280_DIG_H2_LSB_REG - offset);
    cal.dig_H3 = buffer[BME280_DIG_H3_REG - offset];
    cal.dig_H4 = (((int16_t)buffer[BME280_DIG_H4_MSB_REG - offset]) << 4) +
        (buffer[BME280_DIG_H4_H5_REG - offset] & 0x0F);
    cal.dig_H5 = (((int16_t)buffer[BME280_DIG_H5_MSB_REG - offset]) << 4) +
        ((buffer[BME280_DIG_H4_H5_REG - offset] & 0xF0) >> 4);
    cal.dig_H6 = buffer[BME280_DIG_H6_REG - offset];

    TRACE("Calibration temp data: %d %d %d\r\n", cal.dig_T1,cal.dig_T2,cal.dig_T3);
    TRACE("Calibration pres data: %d %d %d %d %d %d %d %d %d\r\n", cal.dig_P1,cal.dig_P2,cal.dig_P3,
    	cal.dig_P4,cal.dig_P5,cal.dig_P6,
    	cal.dig_P7,cal.dig_P8,cal.dig_P9);
    TRACE("Calibration humi data: %d %d %d %d %d %d \r\n", cal.dig_H1,cal.dig_H2,cal.dig_H3,
    	cal.dig_H4,cal.dig_H5,cal.dig_H6);


    TRACE("Configuration of the device\r\n");
    uint8_t b;
    // Configuration can be changed only in standby mode
    b=0;
    write_u8_reg(BME280_I2C_ADDR, BMX280_CTRL_MEAS_REG, &b, 1);  // Se 
    b=0x01;  // 001 => humidity oversampling x1
    write_u8_reg(BME280_I2C_ADDR, BME280_CTRL_HUMIDITY_REG, &b, 1);  // Se 
    // Change configuration 001 001 00 (0x25)=> pressure oversampling x1, temp oversampling x1 , Sleep mode
    b=0x24;  
    write_u8_reg(BME280_I2C_ADDR, BMX280_CTRL_MEAS_REG, &b, 1);  // Se 


    return 1; 
}


uint8_t get_ctrl_meas(void)
{
	uint8_t r; 
    read_u8_reg(BME280_I2C_ADDR, BMX280_CTRL_MEAS_REG, &r, 1);
    //TRACE("get_ctrl_meas: %02X\r\n", r);
    return r;
}

uint8_t get_status(void)
{
	uint8_t r; 
    read_u8_reg(BME280_I2C_ADDR, BMX280_STAT_REG, &r, 1);
    //TRACE("get_status: %02X\r\n", r);
    return r;
}


/**
 * @brief Start a measurement and read the registers
 */
int do_measurement(uint8_t pMode)
{
	TRACE("do_measurement START\r\n");
    uint8_t ctrl_meas = get_ctrl_meas();
    uint8_t run_mode = ctrl_meas & 3;
    if (run_mode != pMode) {
    	TRACE("do_measurement: crtl_mes value: %02X , will set Force mode %02X\r\n", run_mode, pMode);
        ctrl_meas &= ~3;
        ctrl_meas |= pMode;
        write_u8_reg(BME280_I2C_ADDR, BMX280_CTRL_MEAS_REG, &ctrl_meas, 1 );



        size_t count = 0;
        while (count < 10 && (get_status() & 0x08) != 0) {
            ++count;
        }
    }
    int nr_bytes;
    int nr_bytes_to_read = sizeof(measurement_regs);
    uint8_t offset = BMX280_PRESSURE_MSB_REG;

    nr_bytes = read_u8_reg(BME280_I2C_ADDR, offset, measurement_regs, nr_bytes_to_read);
    if (nr_bytes != nr_bytes_to_read) {
        TRACE("Unable to read temperature data\n");
        return -1;
    }

    TRACEDUMP("Raw Sensor Data", measurement_regs, nr_bytes);


    return 0;
}

/*
 * Returns temperature in DegC, resolution is 0.01 DegC.
 * t_fine carries fine temperature as global value
 */
int16_t bmx280_read_temperature(void)
{   
    if (do_measurement(BMX280_MODE_FORCED) < 0) {
        return -10000;
    }

    /* Read the uncompensated temperature */
    int32_t adc_T = (((uint32_t)measurement_regs[3 + 0]) << 12) |
        (((uint32_t)measurement_regs[3 + 1]) << 4) |
        ((((uint32_t)measurement_regs[3 + 2]) >> 4) & 0x0F);

    /*
     * Compensate the temperature value.
     * The following is code from Bosch's BME280_driver bme280_compensate_temperature_int32()
     * The variable names and the many defines have been modified to make the code
     * more readable.
     */
    int32_t var1;
    int32_t var2;

    var1 = ((((adc_T >> 3) - ((int32_t)cal.dig_T1 << 1))) * ((int32_t)cal.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)cal.dig_T1)) * ((adc_T >> 4) - ((int32_t)cal.dig_T1))) >> 12) *
            ((int32_t)cal.dig_T3)) >> 14;

    /* calculate t_fine (used for pressure and humidity too) */
    t_fine = var1 + var2;

    return (t_fine * 5 + 128) >> 8;
}

/*
 * Returns pressure in Pa
 */
uint16_t bmx280_read_pressure(void)
{
    /* Read the uncompensated pressure */
    int32_t adc_P = (((uint32_t)measurement_regs[0 + 0]) << 12) |
        (((uint32_t)measurement_regs[0 + 1]) << 4) |
        ((((uint32_t)measurement_regs[0 + 2]) >> 4) & 0x0F);

    int64_t var1;
    int64_t var2;
    int64_t p_acc;

    /*
     * Compensate the pressure value.
     * The following is code from Bosch's BME280_driver bme280_compensate_pressure_int64()
     * The variable names and the many defines have been modified to make the code
     * more readable.
     */
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)cal.dig_P6;
    var2 = var2 + ((var1 * (int64_t)cal.dig_P5) << 17);
    var2 = var2 + (((int64_t)cal.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)cal.dig_P3) >> 8) + ((var1 * (int64_t)cal.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)cal.dig_P1) >> 33;
    /* Avoid division by zero */
    if (var1 == 0) {
        return 0xFFFF;
    }

    p_acc = 1048576 - adc_P;
    p_acc = (((p_acc << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)cal.dig_P9) * (p_acc >> 13) * (p_acc >> 13)) >> 25;
    var2 = (((int64_t)cal.dig_P8) * p_acc) >> 19;
    p_acc = ((p_acc + var1 + var2) >> 8) + (((int64_t)cal.dig_P7) << 4);

    return (uint16_t)((p_acc >> 8)/100);
}

uint16_t bme280_read_humidity(void)
{

    /* Read the uncompensated pressure */
    int32_t adc_H = (((uint32_t)measurement_regs[6 + 0]) << 8) |
        (((uint32_t)measurement_regs[6 + 1]));

    /*
     * Compensate the humidity value.
     * The following is code from Bosch's BME280_driver bme280_compensate_humidity_int32()
     * The variable names and the many defines have been modified to make the code
     * more readable.
     * The value is first computed as a value in %rH as unsigned 32bit integer
     * in Q22.10 format(22 integer 10 fractional bits).
     */
    int32_t var1;

    /* calculate x1*/
    var1 = (t_fine - ((int32_t)76800));
    /* calculate x1*/
    var1 = (((((adc_H << 14) - (((int32_t)cal.dig_H4) << 20) - (((int32_t)cal.dig_H5) * var1)) +
              ((int32_t)16384)) >> 15) *
            (((((((var1 * ((int32_t)cal.dig_H6)) >> 10) *
                 (((var1 * ((int32_t)cal.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
               ((int32_t)2097152)) * ((int32_t)cal.dig_H2) + 8192) >> 14));
    var1 = (var1 - (((((var1 >> 15) * (var1 >> 15)) >> 7) * ((int32_t)cal.dig_H1)) >> 4));
    var1 = (var1 < 0 ? 0 : var1);
    var1 = (var1 > 419430400 ? 419430400 : var1);
    /* First multiply to avoid losing the accuracy after the shift by ten */
    return (100 * ((uint32_t)var1 >> 12)) >> 10;
}

uint8_t detect_bme(void)
{
    // Read chipId
    uint8_t buf;
    read_u8_reg(BME280_I2C_ADDR, BMX280_CHIP_ID_REG, &buf, 1);
    TRACE("ChipID : 0x%02X\r\n", buf);
    if (buf!=BME280_CHIP_ID){
        TRACE("Wrong chip id (%02X != %02X)\n", buf, BME280_CHIP_ID);
        return 0;
    }
    return 1; 
}
