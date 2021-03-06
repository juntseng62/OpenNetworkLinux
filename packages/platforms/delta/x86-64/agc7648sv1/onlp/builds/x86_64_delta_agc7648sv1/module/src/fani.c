/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright (C) 2017 Delta Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include "platform_lib.h"
#include <onlplib/i2c.h>
#include <onlp/platformi/fani.h>

typedef struct fan_path_S
{
    char *status;
    char *speed;
    char *ctrl_speed;
}fan_path_T;

#define MAKE_FAN_INFO_NODE_ON_FAN_BOARD(id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##id##_ON_FAN_BOARD), "Chassis Fan "#id, 0 }, \
        0x0, \
        (ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE), \
        0, \
        0, \
        ONLP_FAN_MODE_INVALID, \
    }

#define MAKE_FAN_INFO_NODE_ON_PSU(psu_id, fan_id) \
    { \
        { ONLP_FAN_ID_CREATE(FAN_##fan_id##_ON_PSU##psu_id), "Chassis PSU-"#psu_id " Fan "#fan_id, 0 }, \
        0x0, \
        (ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE), \
        0, \
        0, \
        ONLP_FAN_MODE_INVALID, \
    }

/* Static fan information */
onlp_fan_info_t linfo[] = {
    { }, // Not used 
    MAKE_FAN_INFO_NODE_ON_FAN_BOARD(1),
    MAKE_FAN_INFO_NODE_ON_FAN_BOARD(2),
    MAKE_FAN_INFO_NODE_ON_FAN_BOARD(3),
    MAKE_FAN_INFO_NODE_ON_FAN_BOARD(4),
    MAKE_FAN_INFO_NODE_ON_FAN_BOARD(5),
    MAKE_FAN_INFO_NODE_ON_FAN_BOARD(6),
    MAKE_FAN_INFO_NODE_ON_FAN_BOARD(7),
    MAKE_FAN_INFO_NODE_ON_FAN_BOARD(8),
    MAKE_FAN_INFO_NODE_ON_PSU(1,1),
    MAKE_FAN_INFO_NODE_ON_PSU(2,1),
};

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

static int dni_fani_info_get_fan(int local_id, onlp_fan_info_t* info, char *dev_name)
{
    int rv = ONLP_STATUS_OK;
    uint8_t bit_data = 0x00;
    UINT4 u4Data = 0;
    UINT4 multiplier = 1;
    uint8_t present_bit = 0x00;

    if(dni_bmc_sensor_read(dev_name, &u4Data, multiplier, FAN_SENSOR) == ONLP_STATUS_OK)
    {
        info->rpm = u4Data;
        info->percentage = (info->rpm * 100) / MAX_FRONT_FAN_SPEED;
    }
   
    rv = dni_bmc_fanpresent_info_get(&bit_data); 
    if(rv == ONLP_STATUS_OK)
        present_bit = bit_data;
    else
        rv = ONLP_STATUS_E_INVALID;

    switch(local_id) {
        case FAN_4_ON_FAN_BOARD:
        case FAN_8_ON_FAN_BOARD:
            if((present_bit & 1) == 0)
                info->status |= ONLP_FAN_STATUS_PRESENT;
            else
                info->status |= ONLP_FAN_STATUS_FAILED;
            break;
        case FAN_3_ON_FAN_BOARD:
        case FAN_7_ON_FAN_BOARD:
            if((present_bit & (1 << 1)) == 0)
                info->status |= ONLP_FAN_STATUS_PRESENT;
            else
                info->status |= ONLP_FAN_STATUS_FAILED;
            break;
        case FAN_2_ON_FAN_BOARD:
        case FAN_6_ON_FAN_BOARD:
            if((present_bit & (1 << 2)) == 0)
                info->status |= ONLP_FAN_STATUS_PRESENT;
            else
                info->status |= ONLP_FAN_STATUS_FAILED;
            break;
        case FAN_1_ON_FAN_BOARD:
        case FAN_5_ON_FAN_BOARD:
            if((present_bit & (1 << 3)) == 0)
                info->status |= ONLP_FAN_STATUS_PRESENT;
            else
                info->status |= ONLP_FAN_STATUS_FAILED;
            break;
    }
  
    return rv;
}

static int dni_fani_info_get_fan_on_psu(int local_id, onlp_fan_info_t* info, char *dev_name)
{
    int rv = ONLP_STATUS_OK;
    UINT4 multiplier = 1;
    UINT4 u4Data = 0;
    uint8_t psu_present_bit = 0x00;
    int rpm_data = 0;
    int bit_data = 0;

    dni_bmc_data_get(BMC_SWPLD_BUS, SWPLD_1_ADDR, PSU_REGISTER, &bit_data);
    psu_present_bit = bit_data;
    rv = dni_bmc_sensor_read(dev_name, &u4Data, multiplier, FAN_SENSOR);
    rpm_data = (int)u4Data;

    switch(local_id) {
        case FAN_1_ON_PSU1:
            psu_present_bit = bit_data;
            if((psu_present_bit & 0x01) != 0x01)
            {
                info->rpm = rpm_data;
                info->percentage = (info->rpm * 100) / MAX_FRONT_FAN_SPEED;
                info->status |= ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_B2F;
            }
            else
            {
                info->status |= ONLP_FAN_STATUS_FAILED;
                rv = ONLP_STATUS_E_INVALID;
            }
            break;
        case FAN_1_ON_PSU2:
            if((psu_present_bit & 0x02) != 0x02)
            {
                info->rpm = rpm_data;
                info->percentage = (info->rpm * 100) / MAX_FRONT_FAN_SPEED;
                info->status |= ONLP_FAN_STATUS_PRESENT | ONLP_FAN_STATUS_B2F;
            }
            else
            {
                info->status |= ONLP_FAN_STATUS_FAILED;
                rv = ONLP_STATUS_E_INVALID;
            }
            break;
    }

    return rv;
}

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int onlp_fani_init(void)
{
    lockinit();
    return ONLP_STATUS_OK;
}

int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    int local_id;
    int rv = ONLP_STATUS_OK;

    VALIDATE(id);
    local_id = ONLP_OID_ID_GET(id);
    *info = linfo[ONLP_OID_ID_GET(id)];

    switch(local_id) {
        case FAN_1_ON_FAN_BOARD:
            rv = dni_fani_info_get_fan(local_id, info, "Fantray_1_1");
            break;
        case FAN_2_ON_FAN_BOARD:
            rv = dni_fani_info_get_fan(local_id, info, "Fantray_1_2");
            break;
        case FAN_3_ON_FAN_BOARD:
            rv = dni_fani_info_get_fan(local_id, info, "Fantray_1_3");
            break;
        case FAN_4_ON_FAN_BOARD:
            rv = dni_fani_info_get_fan(local_id, info, "Fantray_1_4");
            break;
        case FAN_5_ON_FAN_BOARD:
            rv = dni_fani_info_get_fan(local_id, info, "Fantray_2_1");
            break;
        case FAN_6_ON_FAN_BOARD:
            rv = dni_fani_info_get_fan(local_id, info, "Fantray_2_2");
            break;
        case FAN_7_ON_FAN_BOARD:
            rv = dni_fani_info_get_fan(local_id, info, "Fantray_2_3");
            break;
        case FAN_8_ON_FAN_BOARD:
            rv = dni_fani_info_get_fan(local_id, info, "Fantray_2_4");
            break;
        case FAN_1_ON_PSU1:
            rv = dni_fani_info_get_fan_on_psu(local_id, info, "PSU1_Fan");
            break;
        case FAN_1_ON_PSU2: 
            rv = dni_fani_info_get_fan_on_psu(local_id, info, "PSU2_Fan");
            break;
        default:
            rv = ONLP_STATUS_E_INVALID;
            break;
    }

    return rv;
}

/*
 * This function sets the speed of the given fan in RPM.
 *
 * This function will only be called if the fan supprots the RPM_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    return ONLP_STATUS_OK;
}

/*
 * This function sets the fan speed of the given OID as a percentage.
 *
 * This will only be called if the OID has the PERCENTAGE_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    return ONLP_STATUS_OK;
}

/*
 * This function sets the fan speed of the given OID as per
 * the predefined ONLP fan speed modes: off, slow, normal, fast, max.
 *
 * Interpretation of these modes is up to the platform.
 *
 */
int onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan direction of the given OID.
 *
 * This function is only relevant if the fan OID supports both direction
 * capabilities.
 *
 * This function is optional unless the functionality is available.
 */
int onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Generic fan ioctl. Optional.
 */
int onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

