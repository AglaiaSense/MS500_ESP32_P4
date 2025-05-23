/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Wing Hu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "fw_reg_db.h"
extern "C" {
    #include "../imx501.h"
}

SettingInfo*        mp_common = NULL;
SettingInfo*        mp_moduleType = NULL;
SettingInfo*        mp_esVer = NULL;
SettingInfo*        mp_rawSize = NULL;
SettingInfo*        mp_binningOpt = NULL;

/**
 * @brief regsetting_string2decimal
 */
static size_t regsetting_string2decimal(std::string str)
{
    size_t dec = 0;
    uint8_t state = 0;
    size_t i = 0;

    while (i < str.size()) {
        switch (state) {
        case 0:
            if (str[i] == '0') {
                state = 1;
            }
            break;

        case 1:
            if (str[i] == 'x') {
                state = 2;
            }
            break;

        case 2:
            if ((str[i] >= '0') && (str[i] <= '9')) {
                dec <<= 4;
                dec += str[i] - 0x30;
            }
            else if ((str[i] >= 'A') && (str[i] <= 'F')) {
                dec <<= 4;
                dec += str[i] - 0x37;
            }
            else if ((str[i] >= 'a') && (str[i] <= 'f')) {
                dec <<= 4;
                dec += str[i] - 0x57;
            }
            else if ((str[i] == '\r') || (str[i] <= '\n')) {
                /* nop */
            }
            else {
                printf("[REGISTER_DB] ERROR: value format, value=%s\n", str.c_str());
                return 0;
            }
            break;

        default:
            break;

        }
        i++;
    }
    return dec;
}

/**
 * @brief regsetting_read_file
 */
static int regsetting_read_file(const char* file_path, SettingInfo** pp_reg_setting)
{
    SettingInfo* p_reg;
    std::string line;
    size_t line_num = 0;
    size_t count = 0;
    printf("[REGISTER_DB] open: %s \n", file_path);
    /* get line number */
    {
        std::ifstream ifs(file_path);
        if (ifs) {
            while (1) {
                getline(ifs, line);
                line_num++;

                if (ifs.eof()) {
                    break;
                }
            }
        }
        else {
            printf("[REGISTER_DB] ERROR: can't open %s file\n", file_path);
            return -1;
        }
    }

    /*CodeSonar Fix*/
    if(line_num <= 0 || line_num > UINT32_MAX) {
            printf("[REGISTER_DB] ERROR: invalid line_num=%d\n", line_num);
	    return -1;
    }

    /* [Note]
       The number of line_num includes EOF lines */
    if (sizeof(SettingInfo) >= (UINT32_MAX / line_num)) {
        printf("[REGISTER_DB] ERROR: over size %s file, line_num=%d\n", file_path, line_num);
        return -1;
    }

    p_reg = (SettingInfo*)malloc(sizeof(SettingInfo) * line_num);
    if (p_reg == NULL) {
        printf("[REGISTER_DB] ERROR: can't malloc %s file, line_num=%d\n", file_path, line_num);
        return -1;
    }

    /* get reg info */
    std::ifstream ifs(file_path);
    while (getline(ifs, line)) {

        std::istringstream stream(line);
        std::string field;
        std::vector<std::string> result;
        while (getline(stream, field, ',')) {
            result.push_back(field);
        }

        p_reg[count].m_addr = regsetting_string2decimal(result.at(1));
        p_reg[count].m_size = stoi(result.at(2));
        p_reg[count].m_value = regsetting_string2decimal(result.at(3));
        count++;
	
        /*CodeSonar Fix*/
        if(count >= line_num) {
            break;
        }   
    }

    /* set anchor data */
    p_reg[count].m_size = 0;

    *pp_reg_setting = p_reg;

    return 0;
}

/**
 * @brief regsetting_load
 */
extern "C" int regsetting_load(void)
{
    int ret = 0;

    /* common register */
    ret = regsetting_read_file(regdb_common, &mp_common);
    if (ret != 0)
        printf("[REGISTER_DB] reading common register error...\n");
 
    /* module type register */
    ret = regsetting_read_file(regdb_moduletype, &mp_moduleType);
    if (ret != 0)
        printf("[REGISTER_DB] reading module type register error...\n");
 
    /* ES version register */
    ret = regsetting_read_file(regdb_esver, &mp_esVer);
    if (ret != 0)
        printf("[REGISTER_DB] reading ES version register error...\n");

    /* raw size register */
    ret = regsetting_read_file(regdb_rawsize, &mp_rawSize);
    if (ret != 0)
        printf("[REGISTER_DB] reading raw size register error...\n");

    /* binning option register */
    ret = regsetting_read_file(regdb_binningopt, &mp_binningOpt);
    if (ret != 0)
        printf("[REGISTER_DB] reading binning option register error...\n");

    return 0;
}

/**
 * @brief regsetting_unload
 */
extern "C" void regsetting_unload(void)
{
   /* free malloc memory */
    if (mp_common != NULL)
        free(mp_common);
    if (mp_moduleType != NULL)
        free(mp_moduleType);
    if (mp_esVer != NULL)
        free(mp_esVer);
    if (mp_rawSize != NULL)
        free(mp_rawSize);
    if (mp_binningOpt != NULL)
        free(mp_binningOpt);
}

/**
 * @brief regsetting_search_data
 */
static int regsetting_search_data(SettingInfo* p_search, uint16_t search_addr, size_t* p_get_data)
{
    /*CodeSonar Fix*/
    if(p_get_data == NULL) {
        printf("[REGISTER_DB][ERROR] p_get_data is NULL ");
        return -1;
    }

    while (p_search != NULL && p_search->m_size != 0) {

        if (p_search->m_addr == search_addr) {
            //printf("[REGISTER_DB] found reqest data, address=0x%04X, value=0x%08X\n", search_addr, p_search->m_value);
            *p_get_data = p_search->m_value;
            return 0;
        }
        p_search++;
	
    }
    return -1;
}

/**
 * @brief regsetting_get_data
 */
static int regsetting_get_data(uint16_t req_addr, size_t* p_get_data)
{
    int ret;

    /* check param */
    if (p_get_data == NULL) {
        printf("[REGISTER_DB] ERROR: argument is NULL\n");
        return -1;
    }

    /* search data in common file */
    ret = regsetting_search_data(mp_common, req_addr, p_get_data);
    if (ret == 0) {
        return 0;
    }

    /* search data in module type file */
    ret = regsetting_search_data(mp_moduleType, req_addr, p_get_data);
    if (ret == 0) {
        return 0;
    }

    /* search data in es version file */
    ret = regsetting_search_data(mp_esVer, req_addr, p_get_data);
    if (ret == 0) {
        return 0;
    }

    /* search data in raw size file */
    ret = regsetting_search_data(mp_rawSize, req_addr, p_get_data);
    if (ret == 0) {
        return 0;
    }

    /* search data in binning option file */
    ret = regsetting_search_data(mp_binningOpt, req_addr, p_get_data);
    if (ret == 0) {
        return 0;
    }

    printf("[REGISTER_DB] ERROR: not found reqest data, address=0x%04X\n", req_addr);
    return -1;
}

/**
 * @brief regsetting_get_freq_type
 */
void regsetting_get_freq_type(size_t *exe_freq, size_t *flash_type)
{
    int ret;

    ret = regsetting_get_data(REG_ADDR_EXCK_FREQ_C01, exe_freq);
    if (ret != 0) {
        printf("[REGISTER_DB] ERROR: could not get INCK frequency\n");
        return;
    }

    ret = regsetting_get_data(REG_ADDR_FLASH_TYPE, flash_type);
    if (ret != 0) {
        printf("[REGISTER_DB] ERROR: could not get flash type\n");
        return;
    }
}

/**
 * @brief regsetting_get_gain
 */
void regsetting_get_gain(size_t *gain)
{
    int ret;

    ret = regsetting_get_data(REG_ADDR_LEV_PL_GAIN_VALUE, gain);
    if (ret != 0) {
        printf("[REGISTER_DB] ERROR: could not get INCK frequency\n");
        return;
    }
}

/**
 * @brief set_reg_val
 */
static int set_reg_val(SettingInfo *param)
{
    if (param == NULL) {
        printf("[REGISTER_DB] ERROR: set_reg_val param is NULL\n");
        return -1;
    }

    while (param->m_size != 0) {
        I2C_ACCESS_WRITE(param->m_addr, param->m_value, param->m_size);
        //printf("[REGISTER_DB] add = 0x%X, val = 0x%X\n", param->m_addr, param->m_value);
        param++;
    }

    return 0;
}

/**
 * @brief regsetting_set_reg_val
 */
void regsetting_set_reg_val(void)
{
    set_reg_val(mp_common);
    set_reg_val(mp_moduleType);
    set_reg_val(mp_esVer);
    set_reg_val(mp_rawSize);
    set_reg_val(mp_binningOpt);

    printf("[REGISTER_DB] register writing successfully\n");
}