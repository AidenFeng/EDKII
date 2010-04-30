/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TPS65950_H__
#define __TPS65950_H__

#define EXTERNAL_DEVICE_REGISTER_TO_SLAVE_ADDRESS(x)     (((x) >> 8) & 0xFF)
#define EXTERNAL_DEVICE_REGISTER_TO_REGISTER(x)          ((x) & 0xFF)
#define EXTERNAL_DEVICE_REGISTER(SlaveAddress, Register) (((SlaveAddress) & 0xFF) << 8 | ((Register) & 0xFF))

//I2C Address group
#define I2C_ADDR_GRP_ID1      0x48
#define I2C_ADDR_GRP_ID2      0x49
#define I2C_ADDR_GRP_ID3      0x4A
#define I2C_ADDR_GRP_ID4      0x4B
#define I2C_ADDR_GRP_ID5      0x12

//MMC definitions.
#define VMMC1_DEV_GRP         0x82
#define DEV_GRP_P1            BIT5

#define VMMC1_DEDICATED_REG   0x85 
#define VSEL_1_85V            0x0
#define VSEL_2_85V            0x1
#define VSEL_3_00V            0x2
#define VSEL_3_15V            0x3

#define TPS65950_GPIO_CTRL    0xaa  //I2C_ADDR_GRP_ID2
#define CARD_DETECT_ENABLE    (BIT2 | BIT0) // GPIO ON + GPIO CD1 enabled


#define GPIODATAIN1           0x98  //I2C_ADDR_GRP_ID2
#define CARD_DETECT_BIT       BIT0

//LEDEN register
#define LEDEN                 0xEE
#define LEDAON                BIT0
#define LEDBON                BIT1
#define LEDAPWM               BIT4
#define LEDBPWM               BIT5

#endif //__TPS65950_H__
