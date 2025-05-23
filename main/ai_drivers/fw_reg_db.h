/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Wing Hu
 */

#ifndef __RG_DB_H__
#define __RG_DB_H__

/* sensor register file */
static const char *regdb_common = "/spiffs/reg_db/reg_com.txt";           // regisettting_common.txt
static const char *regdb_moduletype = "/spiffs/reg_db/reg_type2_es2.txt"; // regsetting_type2_AC12435MM_es2.txt
static const char *regdb_esver = "/spiffs/reg_db/reg_es2.txt";            // regsetting_es2.txt
static const char *regdb_rawsize = "/spiffs/reg_db/reg_v2h2.txt";         // regsetting_raw_v2h2_mipi1000.txt
static const char *regdb_binningopt = "/spiffs/reg_db/reg_bin_bayer.txt"; // regsetting_binning_bayer.txt

typedef struct {
    unsigned short m_addr;
    unsigned char m_size;
    unsigned char m_rsvd;
    unsigned int m_value;
} SettingInfo;

#ifdef __cplusplus
extern "C" {
#endif

int regsetting_load(void);
void regsetting_unload(void);

void regsetting_get_freq_type(size_t *exe_freq, size_t *flash_type);
void regsetting_get_gain(size_t *gain);
void regsetting_set_reg_val(void);

#ifdef __cplusplus
}

#endif

#endif