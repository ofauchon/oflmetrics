#ifndef I2C_BME280_H
#define I2C_BME280_H

//#warning  HeLLo  

#include <board.h>
#include <i2c.h>

#define TRACEME 1


#ifdef TRACEME 
#include <stdio.h>
#   define TRACE(...) printf(__VA_ARGS__)
#   define TRACEDUMP(A,B,C) uint8_t z; printf("DUMP %s: ",A);for (z=0;z<C;z++){printf("%02X ",B[z]); }printf("\r\n");;
#else
#   define TRACE(...)
#   define TRACEDUMP(...)
#endif

typedef enum {
    BMX280_MODE_SLEEP = 0,
    BMX280_MODE_FORCED = 1,
    BMX280_MODE_FORCED2 = 2,    // Forced
    BMX280_MODE_NORMAL = 3
} bmx280_mode_t;


typedef struct {
    uint16_t dig_T1;    /**< T1 coefficient */
    int16_t dig_T2;     /**< T2 coefficient */
    int16_t dig_T3;     /**< T3 coefficient */

    uint16_t dig_P1;    /**< P1 coefficient */
    int16_t dig_P2;     /**< P2 coefficient */
    int16_t dig_P3;     /**< P3 coefficient */
    int16_t dig_P4;     /**< P4 coefficient */
    int16_t dig_P5;     /**< P5 coefficient */
    int16_t dig_P6;     /**< P6 coefficient */
    int16_t dig_P7;     /**< P7 coefficient */
    int16_t dig_P8;     /**< P8 coefficient */
    int16_t dig_P9;     /**< P9 coefficient */

    uint8_t dig_H1;     /**< H1 coefficient */
    int16_t dig_H2;     /**< H2 coefficient */
    uint8_t dig_H3;     /**< H3 coefficient */
    int16_t dig_H4;     /**< H4 coefficient */
    int16_t dig_H5;     /**< H5 coefficient */
    int8_t dig_H6;      /**< H6 coefficient */
} bmx280_calibration_t;

#define BME280_I2C_ADDR                         0x76

#define BME280_CHIP_ID                          0x60    /* The identifier of the BME280 */
#define BMP280_CHIP_ID                          0x58    /* The identifier of the BMP280 */
#define BMX280_CHIP_ID_REG                      0xD0
#define BMEX80_RST_REG                          0xE0 /* Softreset Reg */

#define BMX280_DIG_T1_LSB_REG                   0x88
#define BMX280_DIG_T1_MSB_REG                   0x89
#define BMX280_DIG_T2_LSB_REG                   0x8A
#define BMX280_DIG_T2_MSB_REG                   0x8B
#define BMX280_DIG_T3_LSB_REG                   0x8C
#define BMX280_DIG_T3_MSB_REG                   0x8D
#define BMX280_DIG_P1_LSB_REG                   0x8E
#define BMX280_DIG_P1_MSB_REG                   0x8F
#define BMX280_DIG_P2_LSB_REG                   0x90
#define BMX280_DIG_P2_MSB_REG                   0x91
#define BMX280_DIG_P3_LSB_REG                   0x92
#define BMX280_DIG_P3_MSB_REG                   0x93
#define BMX280_DIG_P4_LSB_REG                   0x94
#define BMX280_DIG_P4_MSB_REG                   0x95
#define BMX280_DIG_P5_LSB_REG                   0x96
#define BMX280_DIG_P5_MSB_REG                   0x97
#define BMX280_DIG_P6_LSB_REG                   0x98
#define BMX280_DIG_P6_MSB_REG                   0x99
#define BMX280_DIG_P7_LSB_REG                   0x9A
#define BMX280_DIG_P7_MSB_REG                   0x9B
#define BMX280_DIG_P8_LSB_REG                   0x9C
#define BMX280_DIG_P8_MSB_REG                   0x9D
#define BMX280_DIG_P9_LSB_REG                   0x9E
#define BMX280_DIG_P9_MSB_REG                   0x9F

#define BME280_DIG_H1_REG                       0xA1
#define BME280_DIG_H2_LSB_REG                   0xE1
#define BME280_DIG_H2_MSB_REG                   0xE2
#define BME280_DIG_H3_REG                       0xE3
#define BME280_DIG_H4_MSB_REG                   0xE4 /* H4[11:4] */
#define BME280_DIG_H4_H5_REG                    0xE5 /* H5[3:0]  H4[3:0] */
#define BME280_DIG_H5_MSB_REG                   0xE6 /* H5[11:4] */
#define BME280_DIG_H6_REG                       0xE7

#define BME280_CTRL_HUMIDITY_REG                0xF2
#define BMX280_STAT_REG                         0xF3 /* Status Reg */
#define BMX280_CTRL_MEAS_REG                    0xF4 /* Ctrl Measure Reg */
#define BMX280_CONFIG_REG                       0xF5 /* Configuration Reg */
#define BMX280_PRESSURE_MSB_REG                 0xF7 /* Pressure MSB */
#define BMX280_PRESSURE_LSB_REG                 0xF8 /* Pressure LSB */
#define BMX280_PRESSURE_XLSB_REG                0xF9 /* Pressure XLSB */
#define BMX280_TEMPERATURE_MSB_REG              0xFA /* Temperature MSB */
#define BMX280_TEMPERATURE_LSB_REG              0xFB /* Temperature LSB */

#define BME280_DIG_H6_REG                       0xE7
#define BMX280_DIG_T1_LSB_REG                   0x88





int read_calibration_data(void);
int16_t bmx280_read_temperature(void);
uint16_t bme280_read_humidity(void);
uint16_t bmx280_read_pressure(void);
uint8_t detect_bme(void);
uint8_t get_ctrl_meas(void);
uint8_t get_status(void);
void sleepms(uint16_t tm);









#endif
