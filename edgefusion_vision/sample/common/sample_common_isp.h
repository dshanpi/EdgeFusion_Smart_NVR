/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef __SAMPLE_COMMON_ISP_H__
#define __SAMPLE_COMMON_ISP_H__

#include "media/mpi_vi.h"
#include "mm_common.h"
#include "mm_comm_rc.h"
#include "media/mpi_isp.h"

#ifdef __cplusplus
       extern "C" {
#endif

#define BUF_SIZE 1024
#define SUXI_DUMP_DUMP_PATH "/sys/class/sunxi_dump/dump"
#define SUXI_DUMP_WRITE_PATH "/sys/class/sunxi_dump/write"
#define MIPIA_PAYLOAD_REGS 0x45811118
#define MIPIB_PAYLOAD_REGS 0x45811518
#define MIPI_PAYLOAD_ERRPD_MASK 0x1f80
#define MIPIA_PHY_DESKEW_REGS 0x45810118
#define MIPIB_PHY_DESKEW_REGS 0x45810218
#define MIPI_PHY_DESKEW_CLK_DLY_MASK 0x01f
#define MIPI_PHY_DESKEW_CLK_DLY_SET 20
#define MIPI_PHY_DESKEW_CLK_DLY_MAX 0x01f
#define REGS_FLUSH 0xffffffff

typedef enum {
    ISP_AE_MDOE = 1 << 0,
    ISP_AE_EXP_BAIS = 1 << 1,
    ISP_AE_ISO_SENSITIVE = 1 << 2,
    ISP_AE_METERING = 1 << 3,
    ISP_AE_EV_IDX = 1 << 4,
    ISP_AE_LOCK = 1 << 5,
    ISP_AE_TABLE = 1 << 6,
    ISP_AE_SCENE = 1 << 7,
    ISP_AE_FLICKER = 1 << 8,
    ISP_AE_FACE_AE = 1 << 9,
    ISP_AWB_MODE = 1 << 10,
    ISP_SPEC_BRIGHTNESS = 1 << 11,
    ISP_SPEC_CONTRAST = 1 << 12,
    ISP_SPEC_SATURATION = 1 << 13,
    ISP_SPEC_SHARPNESS = 1 << 14,
    ISP_SPEC_HUE = 1 << 15,
    ISP_SPEC_PLTM = 1 << 16,
    ISP_SPEC_2DNR = 1 << 17,
    ISP_SPEC_3DNR = 1 << 18,
    ISP_SPEC_COLOR_EFFECT = 1 << 19,
    ISP_SPEC_MIRROR = 1 << 20,
    ISP_SPEC_FLIP = 1 << 21,
    ISP_SPEC_IR_STATUS = 1 << 22,
    ISP_SETTINGS_FREQ = 1 << 23,
    TEST_ISP_ALL = (1 << 24) - 1,
    TEST_TOTAL = 24
} TestType;

typedef enum {
    TEST_ISP_AE = ISP_AE_MDOE | ISP_AE_EXP_BAIS | ISP_AE_ISO_SENSITIVE | ISP_AE_METERING |
                    ISP_AE_METERING | ISP_AE_EV_IDX | ISP_AE_LOCK | ISP_AE_TABLE |
                    ISP_AE_SCENE | ISP_AE_FLICKER |ISP_AE_FACE_AE,
    TEST_ISP_AWB = ISP_AWB_MODE,
    TEST_ISP_SPEC = ISP_SPEC_BRIGHTNESS | ISP_SPEC_CONTRAST | ISP_SPEC_SATURATION |
                    ISP_SPEC_SHARPNESS | ISP_SPEC_HUE | ISP_SPEC_PLTM | ISP_SPEC_COLOR_EFFECT | ISP_SPEC_IR_STATUS,
    TEST_ISP_NR = ISP_SPEC_2DNR | ISP_SPEC_3DNR,
    TEST_ISP_FLIP = ISP_SPEC_MIRROR | ISP_SPEC_FLIP,
} TestGroupType;

typedef struct IspAeTestCfg {
    unsigned int mAeMode;
    unsigned int mExpBais;
    unsigned int mIsoSensitive;
    int mAeMetering;
    int mAeEvIdx;
    int mAeMaxEvIdx;
    unsigned int mAeLock;
    struct ae_table_info mAeTable;
    enum ae_table_mode mAeScene;
    int mAeFlicker;
    struct isp_face_ae_attr_info mFaceAeInfo;
    RECT_S mFaceRoiRgn[AE_FACE_MAX_NUM];
    SIZE_S mRes;
    unsigned int mTiggerFaceAeForTest;
} IspAeTestCfg;

typedef struct IspAwbTestCfg {
    unsigned int mAwbMode;
    unsigned int mAwbColorTemp;
    struct isp_wb_gain mAwbGain;
} IspAwbTestCfg;

typedef struct IspSpecEffectTestCfg {
    int mBrightess;
    int mContrast;
    int mSaturation;
    int mSharpness;
    int mHue;
    int mPltmStren;
    int m2dnrStren;
    int m3dnrStren;
    enum colorfx mColorStyle;
    int mMirror;
    int mFlip;
    ISP_CFG_MODE mIrStatus;
} IspSpecEffectTestCfg;

typedef struct IspSettingsTestCfg {
    unsigned short mIspAlgoFreq;
} IspSettingsTestCfg;

typedef struct IspApiTaskConfig {
    void (*tasks[TEST_TOTAL])();
    int completed[TEST_TOTAL];
    int mTasksNum;
    int mCurrentTask;
    int mtaskSchedule;
    unsigned int mTaskList[TEST_TOTAL];
    unsigned int mActuralTaskNum;
} IspApiTaskConfig;

typedef struct IspApiTestCtrlConfig {
    unsigned int mTestIntervalMs;
    unsigned int mTetstIspChannelId;
    pthread_t ispTestThreadId;
    IspApiTaskConfig mIspApiTask;
    IspAeTestCfg mTestAeCfg;
    IspAwbTestCfg mTetstAwbCfg;
    IspSpecEffectTestCfg mTestSpecEffectCfg;
    IspSettingsTestCfg mTetstSettingsCfg;
} IspApiTestCtrlConfig;

int ispApiTestInit(IspApiTestCtrlConfig *pIspTestContext);
int ispApiTestExit(IspApiTestCtrlConfig *pIspTestContext);


typedef struct MipiDeskConfig {
    unsigned int mPayLoadStatus;
    unsigned int mPayLoadAddr;
    unsigned int mPhyDeskRegsAddr;
    unsigned int mPhyDeskValue;
    unsigned int mPhyDeskValueMax;
    unsigned int mCalcDeskStart;
    unsigned int mCalcDeskPass;
    unsigned int mCalcDeskEnd;
    unsigned int mCalcDeskBest;
} MipiDeskConfig;

typedef struct DetMipiDeskCtrlConfig{
    pthread_t mdetMipiDeskThreadId;
    bool mdetMipiDeskEnable;
    unsigned int mDetectIntervalMs;
    MipiDeskConfig mMipiDeskCfg;
    bool mdetMipiDeskDone;
} DetMipiDeskCtrlConfig;

int detectMipiDeskTestInit(DetMipiDeskCtrlConfig *pDetMipiDeskContext);
int detectMipiDeskTestExit(DetMipiDeskCtrlConfig *pDetMipiDeskContext);

#ifdef __cplusplus
       }
#endif

#endif