/*
 * 2024 Guangshi (Shanghai) CO LTD
 *
 * Wing Hu
 */

#include "imx501.h"
#include "esp_log.h"
#include "fw_dnn.h"
#include "string.h"
#include "ai_i2c.h"
#define DNN_INPUT (0)

static const char *TAG = "imx501";

static Imx500Param s_imx500Param;

/**
 * @brief Read a sequence of bytes from a IMX501 sensor registers
 */
esp_err_t imx501_register_read(uint16_t reg_addr, size_t *data, size_t size) {
    return ai_i2c_register_read(reg_addr, data, size);
}

/**
 * @brief Read a sequence of bytes from a IMX501 sensor registers
 */
esp_err_t imx501_register_read_id(uint16_t reg_addr, uint8_t *data, size_t size) {
    return ai_i2c_register_read_id(reg_addr, data, size);
}

/**
 * @brief Write to IMX501 sensor register
 */
esp_err_t imx501_register_write(uint16_t reg_addr, size_t data, size_t size) {
    return ai_i2c_register_write(reg_addr, data, size);
}

/**
 * @brief i2c master initialization
 */

/**
 * @brief i2c imx501_register_test
 */
void imx501_register_start(void) {
    size_t data = 0; // 用于存储读取的寄存器数据
    uint8_t id[16];  // 用于存储传感器ID

    /* 读取IMX501的ID寄存器 */
    memset(id, 0, sizeof(id));                                             // 初始化ID缓冲区
    ESP_ERROR_CHECK(imx501_register_read_id(IMX501_ID_REG_ADDR,  id, 16)); // 读取16字节的ID
    printf("ID = ");
    for (int i = 0; i < 16; i++) {
        printf(" 0x%X ", id[i]); // 打印ID的每个字节
    }
    printf("\n");

    memset(&data, 0, sizeof(data)); // 初始化数据缓冲区

    /* 读取IMX501的版本寄存器 */
    ESP_ERROR_CHECK(imx501_register_read(IMX501_VERSION_REG_ADDR, &data, 1)); // 读取1字节的版本信息
    ESP_LOGI(TAG, "Version = 0x%X", data);                                    // 打印版本信息

    /* 写入IMX501的STREAM寄存器 */
    ESP_ERROR_CHECK(imx501_register_write(0x0103, 0x01, 1)); // 写入1字节的STREAM控制值

    /* 读取IMX501的STREAM寄存器（已注释） */
    // ESP_ERROR_CHECK(imx501_register_read(0x0103, &data, 1));
    // ESP_LOGI(TAG, "stream = 0x%X", data);
}

/**
 * @brief i2c initialization
 */
esp_err_t imx501_init(void) {


    imx501_register_start();

    return ESP_OK;
}



/**
 * @brief imx501_set_inck
 */
size_t imx501_set_inck(size_t inckFreq, size_t mipiDataRate, uint8_t imgOnlyMode, uint8_t flashType) {
    size_t inck;
    size_t inck_decimal;
    uint8_t prepllckDiv;
    uint16_t pllMpy;

    /* Setting IMAGING_ONLY_MODE_FW */
    if (imgOnlyMode == 1) {
        I2C_ACCESS_WRITE(REG_ADDR_IMAGING_ONLY_MODE_FW, DEF_VAL_IMAGING_ONLY_MODE_FW, REG_SIZE_IMAGING_ONLY_MODE_FW);
    } else {
        I2C_ACCESS_WRITE(REG_ADDR_IMAGING_ONLY_MODE_FW, DEF_VAL_NOTIMAGING_ONLY_MODE_FW, REG_SIZE_IMAGING_ONLY_MODE_FW);
    }

    /* calc INCK */
    inck = ((inckFreq >> 8) & 0xFF) * FREQ_1MHZ;
    inck_decimal = inckFreq & 0xFF;
    for (uint8_t i = 0; i < 8; i++) {
        inck += ((inck_decimal >> (7 - i)) & 1) * (FREQ_1MHZ >> (i + 1));
    }

    /* Setting IVT_PREPLLCK_DIV, IOP_PREPLLCK_DIV */
    if ((inck < INCK_12MHZ) || (inck > INCK_27MHZ)) {
        ESP_LOGI(TAG, "[IMX500LIB][SetInck] Failed INCK value : val=%d\n", inck);
        return ESP_FAIL;
    } else if (inck <= INCK_23MHZ) {
        prepllckDiv = 1;
    } else {
        prepllckDiv = 2;
    }

    s_imx500Param.m_inckFreq = inck;

    I2C_ACCESS_WRITE(REG_ADDR_IVT_PREPLLCK_DIV_C03, prepllckDiv, REG_SIZE_IVT_PREPLLCK_DIV_C03);
    I2C_ACCESS_WRITE(REG_ADDR_IOP_PREPLLCK_DIV_C03, prepllckDiv, REG_SIZE_IOP_PREPLLCK_DIV_C03);

    /* Setting IVT_PLL_MPY, IOP_PLL_MPY */
    pllMpy = TARGET_PLL_FREQ / (inck / prepllckDiv);
    I2C_ACCESS_WRITE(REG_ADDR_IVT_PLL_MPY_C03, pllMpy, REG_SIZE_IVT_PLL_MPY_C03);
    pllMpy = (mipiDataRate * FREQ_1MHZ) / (inck / prepllckDiv);
    I2C_ACCESS_WRITE(REG_ADDR_IOP_PLL_MPY_C03, pllMpy, REG_SIZE_IOP_PLL_MPY_C03);

    /* Setting EXCK_FREQ */
    I2C_ACCESS_WRITE(REG_ADDR_EXCK_FREQ_C01, inckFreq, REG_SIZE_EXCK_FREQ_C01);

    /* Setting FLASH_TYPE */
    I2C_ACCESS_WRITE(REG_ADDR_FLASH_TYPE, flashType, REG_SIZE_FLASH_TYPE);

    return ESP_OK;
}

/**
 * @brief imx501_set_mipi_bit_rate
 */
int imx501_set_mipi_bit_rate(size_t mipiLane) {
    size_t prepllckDiv;
    size_t pllMpy;
    size_t outBitRate;
    size_t outBitRateInt;
    size_t outBitRateDec;
    size_t outBitRateTmp;

    /* calc Output Bit Rate */
    I2C_ACCESS_READ(REG_ADDR_IOP_PREPLLCK_DIV_C03, &prepllckDiv, REG_SIZE_IVT_PREPLLCK_DIV_C03);
    I2C_ACCESS_READ(REG_ADDR_IOP_PLL_MPY_C03, &pllMpy, REG_SIZE_IOP_PLL_MPY_C03);
    outBitRate = (s_imx500Param.m_inckFreq / prepllckDiv) * pllMpy * mipiLane;
    outBitRateInt = outBitRate / FREQ_1MHZ;
    outBitRateTmp = outBitRate % FREQ_1MHZ;
    outBitRateDec = 0;
    for (size_t i = 0; i < 16; i++) {
        outBitRateDec <<= 1;
        outBitRateTmp *= 2;
        if (outBitRateTmp >= FREQ_1MHZ) {
            outBitRateDec |= 1;
            outBitRateTmp -= FREQ_1MHZ;
        }
    }
    I2C_ACCESS_WRITE(REG_ADDR_REQ_LINK_BIT_RATE_MBPS_C08, (outBitRateInt << 16) | outBitRateDec, REG_SIZE_REQ_LINK_BIT_RATE_MBPS_C08);

    return 0;
}

/**
 * @brief imx501_set_default_param
 */
int imx501_set_default_param(void) {
    I2C_ACCESS_WRITE(REG_ADDR_SCALE_MODE, DEF_VAL_SCALE_MODE, REG_SIZE_SCALE_MODE);
    I2C_ACCESS_WRITE(REG_ADDR_SCALE_MODE_EXT, DEF_VAL_SCALE_MODE_EXT, REG_SIZE_SCALE_MODE_EXT);
    return 0;
}

/**
 * @brief SetOutSizeDnnCH.
 */
int SetOutSizeDnnCH(uint16_t xOutSize,
                    sc_dnn_nw_info_t *nw_info,
                    uint8_t num_of_networks) {
    uint16_t correct_size;
    size_t header_line, in_body_line, out_body_line, tmp_out_body_line;

    if (nw_info == NULL) {
        printf("[SetOutSizeDnnCH] : nw_info is NULL\n");
        return -1;
    }

    size_t header_size = nw_info->dnnHeaderSize;
    size_t input_tensor_size = nw_info->inputTensorSize;

    sc_dnn_nw_info_t *tmp_nw_info = nw_info;
    for (size_t i = 0; i < num_of_networks; i++) {

        /*CodeSonar Fix*/
        if (tmp_nw_info == NULL) {
            break;
        }

        if (tmp_nw_info->inputTensorSize > input_tensor_size) {
            input_tensor_size = tmp_nw_info->inputTensorSize;
        }
        tmp_nw_info++;
    }

    /* calc correct size */
    correct_size = (xOutSize / IMX500_HW_ALIGN) * IMX500_HW_ALIGN;
    if (correct_size > IMX500_HW_LIMIT_LINE_PIX) {
        correct_size = IMX500_HW_LIMIT_LINE_PIX;
    }

    /* calc line */
    header_line = (header_size + (correct_size - 1)) / correct_size;
    in_body_line = header_line + ((input_tensor_size + (correct_size - 1)) / correct_size);
    out_body_line = header_line;

    tmp_nw_info = nw_info;
    for (size_t i = 0; i < num_of_networks; i++) {
        /*CodeSonar Fix*/
        if (tmp_nw_info == NULL) {
            break;
        }

        tmp_out_body_line = header_line;
        for (size_t j = 0; j < tmp_nw_info->outputTensorNum; j++) {
            tmp_out_body_line += ((tmp_nw_info->p_outputTensorSizeInfo[j].tensorSize + (correct_size - 1)) / correct_size);
        }

        if (tmp_out_body_line > out_body_line) {
            out_body_line = tmp_out_body_line;
        }
        tmp_nw_info++;
    }

    /* set register */
    // I2C_ACCESS_WRITE(REG_ADDR_DD_CH06_X_OUT_SIZE, xOutSize, REG_SIZE_DD_CH06_X_OUT_SIZE);
    // I2C_ACCESS_WRITE(REG_ADDR_DD_CH07_X_OUT_SIZE, xOutSize, REG_SIZE_DD_CH07_X_OUT_SIZE);
    // I2C_ACCESS_WRITE(REG_ADDR_DD_CH08_X_OUT_SIZE, xOutSize, REG_SIZE_DD_CH08_X_OUT_SIZE);
    // I2C_ACCESS_WRITE(REG_ADDR_DD_CH09_X_OUT_SIZE, xOutSize, REG_SIZE_DD_CH09_X_OUT_SIZE);

    // I2C_ACCESS_WRITE(REG_ADDR_DD_CH07_Y_OUT_SIZE, in_body_line, REG_SIZE_DD_CH07_Y_OUT_SIZE);
    // I2C_ACCESS_WRITE(REG_ADDR_DD_CH08_Y_OUT_SIZE, out_body_line, REG_SIZE_DD_CH08_Y_OUT_SIZE);

    return 0;
}

/**
 * @brief imx501_set_scaling_raw_img.
 */
int imx501_set_scaling_raw_img(uint16_t width,
                               uint16_t height,
                               sc_dnn_nw_info_t *nw_info,
                               uint8_t num_of_networks) {
    size_t crop_width;
    size_t crop_height;
    size_t scale_ratio;
    size_t image_size;
    int return_code;

    /* store requeset value */
    s_imx500Param.m_rawImgWidth = width;
    s_imx500Param.m_rawImgHeight = height;

    /* get crop size */
    I2C_ACCESS_READ(REG_ADDR_DIG_CROP_IMAGE_WIDTH, &crop_width, REG_SIZE_DIG_CROP_IMAGE_WIDTH);
    I2C_ACCESS_READ(REG_ADDR_DIG_CROP_IMAGE_HEIGHT, &crop_height, REG_SIZE_DIG_CROP_IMAGE_HEIGHT);

    /* --- Horizontal scaling ---------------------------------------------------------------- */
    if (width > s_imx500Param.m_rawImgWidthMax) {
        printf("[IMX500LIB][SetSclRawImg] correct width(over max)  : %d -> %d\n", width, s_imx500Param.m_rawImgWidthMax);
        width = s_imx500Param.m_rawImgWidthMax;
    }
    if (width >= crop_width) {
        if (width > crop_width) {
            printf("[IMX500LIB][SetSclRawImg] correct width(over crop) : %d -> %d\n", width, crop_width);
        }
        scale_ratio = DEF_VAL_SCALE_M;
        width = crop_width;
    } else {
        /* calc ratio */
        scale_ratio = (crop_width * DEF_VAL_SCALE_M) / width;
        image_size = ((crop_width * DEF_VAL_SCALE_M) / scale_ratio) & 0xFFFC; /* multiple of 4 */
        if (image_size < RAW_IMAGE_WIDTH_MIN) {
            image_size = RAW_IMAGE_WIDTH_MIN;
        }
        if (image_size != width) { /* for debug */
            printf("[IMX500LIB][SetSclRawImg] (crop_width x ratio) : width = %d : %d\n", image_size, width);
        }
    }
    /* set register */
    I2C_ACCESS_WRITE(REG_ADDR_SCALE_M, scale_ratio, REG_SIZE_SCALE_M);
    I2C_ACCESS_WRITE(REG_ADDR_X_OUT_SIZE, width, REG_SIZE_X_OUT_SIZE);

    if (nw_info != NULL) {
        return_code = SetOutSizeDnnCH(width, nw_info, num_of_networks);
        if (return_code != 0) {
            printf("[IMX500LIB][SetSclRawImg] called error SetOutSizeDnnCH()\n");
            return return_code;
        }
    }

    /* --- Vertical scaling ------------------------------------------------------------------ */
    if (height > s_imx500Param.m_rawImgHeightMax) {
        printf("[IMX500LIB][SetSclRawImg] correct height(over max) : %d -> %d\n", height, s_imx500Param.m_rawImgHeightMax);
        height = s_imx500Param.m_rawImgHeightMax;
    }
    if (height >= crop_height) {
        if (height > crop_height) {
            printf("[IMX500LIB][SetSclRawImg] correct height(over crop): %d -> %d\n", height, crop_height);
        }
        scale_ratio = DEF_VAL_SCALE_M_EXT;
        height = crop_height;
    } else {
        /* calc ratio */
        scale_ratio = (crop_height * DEF_VAL_SCALE_M_EXT) / height;
        image_size = ((crop_height * DEF_VAL_SCALE_M_EXT) / scale_ratio) & 0xFFFE; /* multiple of 2 */
        if (image_size < RAW_IMAGE_HEIGHT_MIN) {
            image_size = RAW_IMAGE_HEIGHT_MIN;
        }
        if (image_size != height) { /* for debug */
            printf("[IMX500LIB][SetSclRawImg] (crop_height x ratio) : height = %d : %d\n", image_size, height);
        }
    }
    /* set register */
    I2C_ACCESS_WRITE(REG_ADDR_SCALE_M_EXT, scale_ratio, REG_SIZE_SCALE_M_EXT);
    I2C_ACCESS_WRITE(REG_ADDR_Y_OUT_SIZE, height, REG_SIZE_Y_OUT_SIZE);

    return 0;
}

/**
 * @brief imx501_set_frame_rate.
 */
int imx501_set_frame_rate(uint8_t frmRate) {
    size_t lineLenPck;
    size_t lineLenInck;
    size_t prepllckDiv;
    size_t pllMpy;
    size_t pixelRate;
    size_t frmLineLen;
    size_t fllLshift = 0;
    size_t dnnMaxRunTime;

    /* get LINE_LENGTH_PCK */
    I2C_ACCESS_READ(REG_ADDR_LINE_LENGTH_PCK_C03, &lineLenPck, REG_SIZE_LINE_LENGTH_PCK_C03);
    /* calc LINE_LENGTH_INCK */
    I2C_ACCESS_READ(REG_ADDR_IVT_PREPLLCK_DIV_C03, &prepllckDiv, REG_SIZE_IVT_PREPLLCK_DIV_C03);
    I2C_ACCESS_READ(REG_ADDR_IVT_PLL_MPY_C03, &pllMpy, REG_SIZE_IVT_PLL_MPY_C03);

    lineLenInck = ((lineLenPck + 4) / TOTAL_NUM_OF_IVTPX_CH) * IVTPXCK_CLOCK_DIVISION_RATIO * prepllckDiv;
    lineLenInck = (lineLenInck + (pllMpy - 1)) / pllMpy;
    I2C_ACCESS_WRITE(REG_ADDR_LINE_LENGTH_INCK_M0F, lineLenInck, REG_SIZE_LINE_LENGTH_INCK_M0F);

    /* calc Pixel_rate */
    pixelRate = PIXEL_RATE_FOR_CALC_FRM_RATE; /* Use fixed value for frame rate calculation */

    /* calc FRM_LENGTH_LINES & FLL_LSHIFT */
    frmLineLen = pixelRate / (frmRate * lineLenPck);
    frmLineLen = (frmLineLen / 4) * 4;
    while (frmLineLen > FRM_LENGTH_LINES_SIZE_MAX) {
        fllLshift++;
        frmLineLen >>= 1;
    }

    /* calc DNN_MAX_RUN_TIME */
    dnnMaxRunTime = (1000 / frmRate) - DEF_VAL_DNN_MAX_RUN_TIME;

    /* set register */
    I2C_ACCESS_WRITE(REG_ADDR_FRM_LENGTH_LINES_C03, frmLineLen, REG_SIZE_FRM_LENGTH_LINES_C03);
    I2C_ACCESS_WRITE(REG_ADDR_FLL_LSHIFT_M02, fllLshift, REG_SIZE_FLL_LSHIFT_M02);
    I2C_ACCESS_WRITE(REG_ADDR_DNN_MAX_RUN_TIME, dnnMaxRunTime, REG_SIZE_DNN_MAX_RUN_TIME);

    return 0;
}

/**
 * @brief imx501_set_variable_param
 */
int imx501_set_variable_param(uint8_t frame_rate, sc_dnn_nw_info_t *nw_info, uint8_t num_of_networks) {
    int ret_code;
    size_t mipiLane;
    size_t xOutSize;
    size_t yOutSize;

    /* get register value */
    I2C_ACCESS_READ(REG_ADDR_CSI_LANE_MODE, &mipiLane, REG_SIZE_CSI_LANE_MODE);
    I2C_ACCESS_READ(REG_ADDR_X_OUT_SIZE, &xOutSize, REG_SIZE_X_OUT_SIZE);
    I2C_ACCESS_READ(REG_ADDR_Y_OUT_SIZE, &yOutSize, REG_SIZE_Y_OUT_SIZE);

    /* --- MIPI Global timing ----------------------------- */
    ret_code = imx501_set_mipi_bit_rate(mipiLane + 1);
    if (ret_code != 0) {
        printf("[IMX500LIB][SetValPrm] called error imx501_set_mipi_bit_rate()\n");
        return -1;
    }

    // printf("mipi lane number = %d\n", mipiLane);
    /* ---  image size ------------------------------------ */
    if (mipiLane == 1) {
        s_imx500Param.m_rawImgWidthMax = DEF_VAL_DD_X_OUT_SIZE_V2H2;
        s_imx500Param.m_rawImgHeightMax = DEF_VAL_DD_Y_OUT_SIZE_V2H2;

        /* set binning register */
        I2C_ACCESS_WRITE(REG_ADDR_BINNING_MODE, DEF_VAL_BINNING_MODE, REG_SIZE_BINNING_MODE);
        I2C_ACCESS_WRITE(REG_ADDR_BINNING_TYPE, DEF_VAL_BINNING_TYPE_V2H2, REG_SIZE_BINNING_TYPE);
        /* ---  binning option -------------------------------- */

        /* set binning weighting */
        I2C_ACCESS_WRITE(REG_ADDR_BINNING_WEIGHTING, DEF_VAL_BINNING_WEIGHTING_BAYER, REG_SIZE_BINNING_WEIGHTING);
        I2C_ACCESS_WRITE(REG_ADDR_SUB_WEIGHTING, DEF_VAL_SUB_WEIGHTING_BAYER, REG_SIZE_SUB_WEIGHTING);
    } else { /* == 4lane */
        s_imx500Param.m_rawImgWidthMax = DEF_VAL_DD_X_OUT_SIZE_FULLSIZE;
        s_imx500Param.m_rawImgHeightMax = DEF_VAL_DD_Y_OUT_SIZE_FULLSIZE;
    }
    s_imx500Param.m_rawImgWidth = xOutSize;
    s_imx500Param.m_rawImgHeight = yOutSize;

    ret_code = imx501_set_scaling_raw_img(xOutSize, yOutSize, nw_info, num_of_networks);
    if (ret_code != 0) {
        printf("[IMX500LIB][SetValPrm] called error SetScalingRawImg()\n");
        return ret_code;
    }

    /* --- framerate -------------------------------------- */
    ret_code = imx501_set_frame_rate(frame_rate);
    if (ret_code != 0) {
        printf("[IMX500LIB][SetValPrm] called error SetFrameRate()\n");
        return ret_code;
    }

    /* --- Input Tensor Infomation ------------------------ */
    if (nw_info != NULL) {
        const uint16_t lev_pl_norm_offset_add[3][MAX_NUM_OF_NETWORKS] = {{REG_OFST0_LEV_PL_NORM_YM_YADD, REG_OFST1_LEV_PL_NORM_YM_YADD, REG_OFST2_LEV_PL_NORM_YM_YADD},
                                                                         {REG_OFST0_LEV_PL_NORM_CB_YADD, REG_OFST1_LEV_PL_NORM_CB_YADD, REG_OFST2_LEV_PL_NORM_CB_YADD},
                                                                         {REG_OFST0_LEV_PL_NORM_CR_YADD, REG_OFST1_LEV_PL_NORM_CR_YADD, REG_OFST2_LEV_PL_NORM_CR_YADD}};
        const uint16_t lev_pl_norm_offset_sht[3][MAX_NUM_OF_NETWORKS] = {{REG_OFST0_LEV_PL_NORM_YM_YSFT, REG_OFST1_LEV_PL_NORM_YM_YSFT, REG_OFST2_LEV_PL_NORM_YM_YSFT},
                                                                         {REG_OFST0_LEV_PL_NORM_CB_YSFT, REG_OFST1_LEV_PL_NORM_CB_YSFT, REG_OFST2_LEV_PL_NORM_CB_YSFT},
                                                                         {REG_OFST0_LEV_PL_NORM_CR_YSFT, REG_OFST1_LEV_PL_NORM_CR_YSFT, REG_OFST2_LEV_PL_NORM_CR_YSFT}};

        for (int i = 0; i < num_of_networks; i++) {

            /* set input tensor format */
            I2C_ACCESS_WRITE(REG_ADDR_DNN_INPUT_FORMAT_BASE + i, DNN_INPUT_FORMAT_YUV444, REG_SIZE_DNN_INPUT_FORMAT);

            /* set normalization / clip */
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K00 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK00, REG_SIZE_DNN_YCMTRX_K00);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K01 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK01, REG_SIZE_DNN_YCMTRX_K01);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K02 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK02, REG_SIZE_DNN_YCMTRX_K02);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K03 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK03, REG_SIZE_DNN_YCMTRX_K03);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K10 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK10, REG_SIZE_DNN_YCMTRX_K10);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K11 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK11, REG_SIZE_DNN_YCMTRX_K11);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K12 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK12, REG_SIZE_DNN_YCMTRX_K12);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K13 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK13, REG_SIZE_DNN_YCMTRX_K13);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K20 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK20, REG_SIZE_DNN_YCMTRX_K20);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K21 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK21, REG_SIZE_DNN_YCMTRX_K21);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K22 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK22, REG_SIZE_DNN_YCMTRX_K22);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_K23 + (i * REG_OFST_DNN_YCMTRX), nw_info->NormK23, REG_SIZE_DNN_YCMTRX_K23);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_Y_CLIP + (i * REG_OFST_DNN_YCMTRX), nw_info->yClip, REG_SIZE_DNN_YCMTRX_Y_CLIP);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_CB_CLIP + (i * REG_OFST_DNN_YCMTRX), nw_info->cbClip, REG_SIZE_DNN_YCMTRX_CB_CLIP);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_YCMTRX_CR_CLIP + (i * REG_OFST_DNN_YCMTRX), nw_info->crClip, REG_SIZE_DNN_YCMTRX_CR_CLIP);
            for (int j = 0; j < 3; j++) {
                I2C_ACCESS_WRITE(REG_ADDR_DNN_INPUT_NORM + (j * REG_OFST_DNN_INPUT_NORM_CH) + (i * REG_OFST_DNN_INPUT_NORM), nw_info->rgbNorm[j].add, REG_SIZE_DNN_INPUT_NORM);
                I2C_ACCESS_WRITE(REG_ADDR_DNN_INPUT_NORM_SHIFT + (j * REG_OFST_DNN_INPUT_NORM_CH) + (i * REG_OFST_DNN_INPUT_NORM), nw_info->rgbNorm[j].shift, REG_SIZE_DNN_INPUT_NORM_SHIFT);
                I2C_ACCESS_WRITE(REG_ADDR_DNN_INPUT_NORM_CLIP_MAX + (j * REG_OFST_DNN_INPUT_NORM_CH) + (i * REG_OFST_DNN_INPUT_NORM), nw_info->rgbNorm[j].clipMax, REG_SIZE_DNN_INPUT_NORM_CLIP_MAX);
                I2C_ACCESS_WRITE(REG_ADDR_DNN_INPUT_NORM_CLIP_MIN + (j * REG_OFST_DNN_INPUT_NORM_CH) + (i * REG_OFST_DNN_INPUT_NORM), nw_info->rgbNorm[j].clipMin, REG_SIZE_DNN_INPUT_NORM_CLIP_MIN);
            }
            I2C_ACCESS_WRITE(REG_ADDR_DNN_NORM_YM_CLIP + (i * REG_OFST_DNN_NORM_CLIP), nw_info->yClip, REG_SIZE_DNN_NORM_YM_CLIP);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_NORM_CB_CLIP + (i * REG_OFST_DNN_NORM_CLIP), nw_info->cbClip, REG_SIZE_DNN_NORM_CB_CLIP);
            I2C_ACCESS_WRITE(REG_ADDR_DNN_NORM_CR_CLIP + (i * REG_OFST_DNN_NORM_CLIP), nw_info->crClip, REG_SIZE_DNN_NORM_CR_CLIP);
            I2C_ACCESS_WRITE(REG_ADDR_LEV_PL_NORM_YM_YADD + lev_pl_norm_offset_add[0][i], 0, REG_SIZE_LEV_PL_NORM_YM_YADD);
            I2C_ACCESS_WRITE(REG_ADDR_LEV_PL_NORM_YM_YSFT + lev_pl_norm_offset_sht[0][i], 0, REG_SIZE_LEV_PL_NORM_YM_YSFT);
            I2C_ACCESS_WRITE(REG_ADDR_LEV_PL_NORM_CB_YADD + lev_pl_norm_offset_add[1][i], 0, REG_SIZE_LEV_PL_NORM_CB_YADD);
            I2C_ACCESS_WRITE(REG_ADDR_LEV_PL_NORM_CB_YSFT + lev_pl_norm_offset_sht[1][i], 0, REG_SIZE_LEV_PL_NORM_CB_YSFT);
            I2C_ACCESS_WRITE(REG_ADDR_LEV_PL_NORM_CR_YADD + lev_pl_norm_offset_add[2][i], 0, REG_SIZE_LEV_PL_NORM_CR_YADD);
            I2C_ACCESS_WRITE(REG_ADDR_LEV_PL_NORM_CR_YSFT + lev_pl_norm_offset_sht[2][i], 0, REG_SIZE_LEV_PL_NORM_CR_YSFT);
            I2C_ACCESS_WRITE(REG_ADDR_LEV_PL_GAIN_VALUE + (i * REG_OFST_LEV_PL_GAIN), nw_info->yGgain, REG_SIZE_LEV_PL_GAIN_VALUE);
            for (int j = 0; j < BAYER_CH_MAX; j++) {
                // I2C_ACCESS_WRITE(REG_ADDR_ROT_DNN_NORM          + (j * REG_OFST_ROT_DNN_NORM_CH) + (i * REG_OFST_ROT_DNN_NORM_DNN), 0, REG_SIZE_ROT_DNN_NORM);
                // I2C_ACCESS_WRITE(REG_ADDR_ROT_DNN_NORM_SHIFT    + (j * REG_OFST_ROT_DNN_NORM_CH) + (i * REG_OFST_ROT_DNN_NORM_DNN), 0, REG_SIZE_ROT_DNN_NORM_SHIFT);
                // I2C_ACCESS_WRITE(REG_ADDR_ROT_DNN_NORM_CLIP_MAX + (j * REG_OFST_ROT_DNN_NORM_CH) + (i * REG_OFST_ROT_DNN_NORM_DNN), 0, REG_SIZE_ROT_DNN_NORM_CLIP_MAX);
                // I2C_ACCESS_WRITE(REG_ADDR_ROT_DNN_NORM_CLIP_MIN + (j * REG_OFST_ROT_DNN_NORM_CH) + (i * REG_OFST_ROT_DNN_NORM_DNN), 0, REG_SIZE_ROT_DNN_NORM_CLIP_MIN);
            }

            nw_info++;
        }
    }

    return 0;
}

/**
 * @brief imx501_start
 */
int imx501_start(void) {
    size_t reg_val;
    size_t input_tensor_line;
    size_t output_tensor_line;
    size_t kpi_line;
    size_t pq_setting_line;

    // printf("[IMX500LIB][CTRL] start\n");
    I2C_ACCESS_WRITE(0xD800, 0, 1);

    /* spi */
#if DNN_INPUT
    I2C_ACCESS_WRITE(0xD764, 12416, 4);
#else
    I2C_ACCESS_WRITE(0xD764, 0x00007800, 4);
#endif

    I2C_ACCESS_WRITE(0xD768, 0x00000000, 1);

    I2C_ACCESS_WRITE(0xD753, 0x00000000, 1);
    I2C_ACCESS_WRITE(0xD754, 0x00000000, 1);
    I2C_ACCESS_WRITE(0xD755, 0x00000001, 1); // DNN output
#if DNN_INPUT
    I2C_ACCESS_WRITE(0xD756, 0x00000001, 1); // DNN input
#else
    I2C_ACCESS_WRITE(0xD756, 0x00000000, 1); // DNN input
#endif

    I2C_ACCESS_WRITE(0xD761, 0x00000000, 1);
    I2C_ACCESS_WRITE(0xD757, 0x00000003, 1);
    I2C_ACCESS_WRITE(0xD758, 0x00000000, 1);
    I2C_ACCESS_WRITE(0xD759, 0x00000000, 1);
    I2C_ACCESS_WRITE(0xD75C, 0x00ADF340, 4);
    // I2C_ACCESS_WRITE(0xD75C, 22800000, 4);    // SPI clock

    /* power save mode */
    I2C_ACCESS_WRITE(0x3F50, 0x00000000, 1);
    I2C_ACCESS_WRITE(0xD038, 0x00000064, 2);

    I2C_ACCESS_WRITE(0xD100, 0x00000004, 1);
    I2C_ACCESS_WRITE(0x0100, 0x00000001, 1);

    I2C_ACCESS_WRITE(0xD753, 0x00000001, 1);

    return 0;
}

/**
 * @brief stream_start()
 */
int stream_start(void) {
    int ret = 0;

    printf("[IMX500_LIB] stream_start\n");

    const sc_dnn_nw_info_t *p_dnn_config = get_dnn_nw_info();
    uint8_t num_of_networks = get_dnn_num_of_networks();

#if 0
    /*TEST LOGS*/
    printf("[TEST] num_of_networks = %d\n", num_of_networks);
    sc_dnn_nw_info_t *p_tmp_dnn_cfg = (sc_dnn_nw_info_t *) p_dnn_config;
    for(int n = 0; n < num_of_networks; n++) {
        printf("[TEST] n =%d\n", n);
        if(p_tmp_dnn_cfg != NULL) {
            printf("[TEST] dnnHeaderSize = %d\n", p_tmp_dnn_cfg->dnnHeaderSize);
            printf("[TEST] inputTensorWidth = %d\n", p_tmp_dnn_cfg->inputTensorWidth);
            printf("[TEST] inputTensorHeight = %d\n", p_tmp_dnn_cfg->inputTensorHeight);
            printf("[TEST] inputTensorWidthStride = %d\n", p_tmp_dnn_cfg->inputTensorWidthStride);
            printf("[TEST] inputTensorHeightStride = %d\n", p_tmp_dnn_cfg->inputTensorHeightStride);
            printf("[TEST] inputTensorSize = %d\n", p_tmp_dnn_cfg->inputTensorSize);
            printf("[TEST] inputTensorFormat = %d\n", p_tmp_dnn_cfg->inputTensorFormat);

            printf("[TEST] inputTensorNorm_K00=0x%04X \n", p_tmp_dnn_cfg->NormK00);
            printf("[TEST] inputTensorNorm_K03=0x%04X \n", p_tmp_dnn_cfg->NormK03);
            printf("[TEST] inputTensorNorm_K11=0x%04X \n", p_tmp_dnn_cfg->NormK11);
            printf("[TEST] inputTensorNorm_K13=0x%04X \n", p_tmp_dnn_cfg->NormK13);
            printf("[TEST] inputTensorNorm_K22=0x%04X \n", p_tmp_dnn_cfg->NormK22);
            printf("[TEST] inputTensorNorm_K23=0x%04X \n", p_tmp_dnn_cfg->NormK23);
            printf("[TEST] yClip=0x%08X \n", p_tmp_dnn_cfg->yClip);
            printf("[TEST] cbClip=0x%08X \n", p_tmp_dnn_cfg->cbClip);
            printf("[TEST] crClip=0x%08X \n", p_tmp_dnn_cfg->crClip);
            printf("[TEST] inputTensorNorm_YGain=0x%02X \n", p_tmp_dnn_cfg->yGgain);

            for(int i = 0; i < p_tmp_dnn_cfg->outputTensorNum; i++) {
                printf("[TEST] p_outputTensorSize[%d] = %d\n", i, p_tmp_dnn_cfg->p_outputTensorSizeInfo[i].tensorSize);
            }
            p_tmp_dnn_cfg++;
        }
    }
#endif

    /* drive mode setting */
    imx501_set_default_param();
    imx501_set_variable_param(30, p_dnn_config, num_of_networks);

    /* start stream */
    ret = imx501_start();
    if (ret != 0) {
        printf("[IMX500_MANAGER] failed to start imx501, ret=0x%08x", ret);
        return -1;
    }

    printf("[IMX500_MANAGER] IMX500Lib start done\n");
    return ret;
}

/**
 * @brief imx501_set_model
 */
int imx501_set_model(int model_id) {
    I2C_ACCESS_WRITE(0xD701, model_id, 1);

    return 0;
}

/**
 * @brief imx501_set_roi
 */
int imx501_set_roi(int x, int y, int w, int h) {
    I2C_ACCESS_WRITE(0xD500, y, 2);
    I2C_ACCESS_WRITE(0xD502, x, 2);
    I2C_ACCESS_WRITE(0xD504, h, 2);
    I2C_ACCESS_WRITE(0xD506, w, 2);

    return 0;
}

/**
 * @brief imx501_set_standby
 */
int imx501_set_standby(int enable) {
    I2C_ACCESS_WRITE(0x0100, enable, 1);

    return 0;
}

int imx500_dnn_input_enable(int dnn_input_enable) {
    if (dnn_input_enable == 1) {
        I2C_ACCESS_WRITE(0xD764, 12416, 4);
        I2C_ACCESS_WRITE(0xD756, 0x00000001, 1);
    } else if (dnn_input_enable == 0) {
        I2C_ACCESS_WRITE(0xD764, 0x00007800, 4);
        I2C_ACCESS_WRITE(0xD756, 0x00000000, 1);
    }

    return 0;
}
