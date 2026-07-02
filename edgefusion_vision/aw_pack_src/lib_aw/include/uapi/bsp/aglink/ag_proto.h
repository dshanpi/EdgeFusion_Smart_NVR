/*
 * ag_proto.h
 *
 * Copyright (c) 2025
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinnertech.com>
 *
 * aglink transmit protocol define for drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __AG_LINK_PROTO_H__
#define __AG_LINK_PROTO_H__

#ifdef AGLINK_CHECKSUM
#define AGLINK_HDR_CKSUM
#endif
#ifdef AGLINK_HDR_CKSUM
#include "checksum.h"
#endif

#define TAG     "AW"
#define TAG_LEN sizeof(uint16_t)
#define CKSUM_LEN sizeof(uint16_t)

#define AGLINK_VERSION "1.0.1.1.4.2511220951"

#define MIN_FREE_SPACE (50*1024*1024) //50MB

struct aglink_hdr {
	uint16_t tag;
#ifdef AGLINK_HDR_CKSUM
	uint16_t checksum;
#endif
	uint8_t  type;
	uint8_t  ch;
	uint16_t seq;
	uint16_t len; /* payload len */
	uint8_t  payload[];
} __attribute__((packed));

#define XR_HDR_SZ sizeof(struct aglink_hdr)

enum AG_MODE {
	AG_MODE_PHOTO = 0,
	AG_MODE_VIDEO,
	AG_MODE_DOWNLOAD,
	AG_MODE_OTA,
	AG_MODE_AI,
	AG_MODE_NORMAL,
	AG_MODE_RECORD_AUDIO,
	AG_MODE_IDLE,
	AG_MODE_LIVESTREAM,
	AG_MODE_AOV,
	AG_MODE_VIDEO_VERTICAL,
	AG_MODE_VIDEO_PREVIEW,
};

/* form TWS */
enum AG_AD_ID {
	AG_AD_TEST = 0,
	AG_AD_TIME,
	AG_AD_AP,
	AG_AD_P2P,
	AG_AD_STA,
	AG_AD_AUDIO,  //5
	AG_AD_OTA_START,
	AG_AD_VIDEO_STOP,
	AG_AD_RECORD_AUDIO_STOP,
	AG_AD_DELETE_MEDIA,
	AG_AD_TAKE_SG_PHOTO,  //10
	AG_AD_GET_SYS_VERSION,
	AG_AD_GET_CHIP_TEMP,
	AG_AD_GET_STROGE_CAPACITY,
	AG_AD_DOWNLOAD_STOP,
	AG_AD_GET_MEDIA_NUM,  //15
	AG_AD_SAVE_SYS_LOG,
	AG_AD_RT_MEDIA_STOP,
	AG_AD_P2P_RESTART,
	AG_AD_AUDIO_NAME,
	AG_AD_FACTORY_RESET,  //20
	AG_AD_AOV_STOP,
	AG_AD_GET_THUMB,
	AG_AD_AOV_VIDEO_SWAP,
	AG_AD_DIS_LINE = 64,
	AG_AD_PAUSE,
	AG_AD_RESUME,
	AG_AD_RCVD_COMP,
};

/* form AW */
enum AG_VD_ID {
	AG_VD_TEST = 0,
	AG_VD_SYNC_1,
	AG_VD_SYNC_2,
	AG_VD_THUMB,
	AG_VD_THUMB_END,
	AG_VD_POWER_OFF,
	AG_VD_AP_SUCCESS,
	AG_VD_P2P_SUCCESS,
	AG_VD_VIDEO_START,
	AG_VD_VIDEO_END,
	AG_VD_STA_SUCCESS,
	AG_VD_STA_FAILED,
	AG_VD_OTA_SUCCESS,
	AG_VD_OTA_PG_BAR,
	AG_VD_OTA_FAILED,
	AG_VD_SDP,
	AG_VD_RECORD_AUDIO_START,
	AG_VD_SG_PHOTO_END,
	AG_VD_SYS_VERSION,
	AG_VD_CHIP_TEMP,
	AG_VD_STROGE_CAPACITY,
	AG_VD_GENERIC_RESPONSE,
	AG_VD_MEDIA_NUM,
	AG_VD_AOV_START,
	AG_VD_MAX,
};

enum AG_GNR_CODE {
	AG_SUCCESS,                //Processed successfully
	AG_REJECTED,               //rejected this command
	AG_FAILED_NOMEMORY,        //no enough memory
	AG_FAILED_WRITE_FILE,      //write file failed
	AG_FAILED_READ_FILE,       //read file failed
	AG_FAILED_PROCESS,         //process failed
	AG_FAILED_NETWORK,         //Network access exception
	AG_FAILED_PARAMETER,       //Parameter error
	AG_DOWNLOAD_KEEPALIVE,     //keepalive
	AG_CONNECT_TIMEOUT,       //timeout
	AG_NETWORK_DISCONNECT,    //disconnect
	AG_LIVESTREAM_KEEPALIVE,   //livestream keepalive
	AG_LIVESTREAM_TIMEOUT,     //livestream timeout
	AG_LIVESTREAM_NOSTREAM,    //livestream no stream
	AG_FAILED_CAMERA_STATUS,   //VIN init fail
	AG_FAILED_DISK_FULL,      //Disk Full
	AG_FAILED_MOUNT_DISK,      //Mount UDISK
	AG_FAILED_UNKNOWN,         //unkown error
};

enum AG_UDISK_SPACE_STATUS {
	AG_UDISK_ENOUGH = 0,
	AG_UDISK_NOT_ENOUGH,
	AG_UDISK_FAILED,
};

#define AG_ALIGNED 2

struct ag_generic_rsp {
	uint8_t mode;
	uint8_t code;
} __attribute__((packed, aligned(AG_ALIGNED)));

// TYPE: AG_AD_P2P
struct ag_wifi_p2p {
	uint8_t role;
	uint8_t name[32];
	uint8_t pwd[32];
} __attribute__((packed, aligned(AG_ALIGNED)));

// TYPE: AG_VD_P2P_SUCCESS
struct ag_wifi_p2p_success {
	uint8_t mac[6];
} __attribute__((packed, aligned(AG_ALIGNED)));

// TYPE: AG_AD_AP
struct ag_wifi_ap {
	uint8_t ssid[32];
	uint8_t pwd[32];
	uint8_t channel;
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE: AG_AD_STA
struct ag_wifi_sta {
	uint8_t ssid[32];
	uint8_t pwd[32];
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE: AG_AD_TIME
struct ag_time {
	uint8_t mode;    // AG_MODE
	uint64_t timestamp;
	uint8_t timeout;    // FLOW_CTL_TIMEOUT
	uint16_t width;    //thumb width
	uint16_t height;    //thumb height
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE: AG_AD_AUDIO
struct ag_audio {
	uint8_t data[1024];
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE: AG_AD_AUDIO_NAME
struct ag_audio_name {
	uint8_t state;
	uint8_t data[24];
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE: AG_AD_OTA_START
struct ag_ota_start {
	uint8_t url[128];
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE: AG_AD_DELETE_MEDIA
enum del_media {
	DM_SINGLE = 0,
	DM_ALL = 1
};

struct ag_del_media {
	uint8_t type;
	uint8_t name[24];
} __attribute__((packed, aligned(AG_ALIGNED)));
//TYPE: AG_VD_SYNC_1/2
struct ag_sync {
	uint8_t mode;
	uint8_t str[5];
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE: COMMON CODE,eg error code.
struct ag_common {
	uint8_t code;
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE: AG_PHOTO_COUNT
struct ag_photo_count {
	uint32_t jpg_count;
	uint32_t mp4_count;
	uint32_t audio_count;
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE: AG_VD_SYNC_1/2
struct ag_ota_bar {
	uint8_t download_schedule;
	uint8_t write_data_schedule;
} __attribute__((packed, aligned(AG_ALIGNED)));

//transport protocol
enum AG_NET_PROTOCOL {
	AG_PROTO_HTTP = 0,
	AG_PROTO_HTTPS,
	AG_PROTO_WEBSOCKET,
	AG_PROTO_WEBRTC,
	AG_PROTO_RTSP,
	AG_PROTO_TCP,
};
//TYPE: AG_VD_SDP
struct ag_sdp {
	uint8_t ip[4];
	uint32_t port;
	uint8_t path[32];
	uint8_t protocol; //AG_NET_PROTOCOL
} __attribute__((packed, aligned(AG_ALIGNED)));

enum ag_bootmedium {
	AG_BOOT_NOR = 0,
	AG_BOOT_SDNAND,
	AG_BOOT_UNKNOW,
};

enum ag_version_verify {
	AG_VERSION_IDENTICAL = 0,   //The sdnand version is the identical as the nor version.
	AG_VERSION_DIFFERENT,           //The sdnand version is the different as the nor version.
	AG_VERSION_NONEED,                      //Currently in sdnand media, no need to read.
	AG_VERSION_UNKNOW,                      //An error occurred while reading sdnand version information.
};

//TYPE:AG_VD_SYS_VERSION
struct ag_sys_version {
	uint8_t version[24];
	uint8_t bootmedium;
	uint8_t nor_partition_num;
	uint8_t sdnand_partition_num;
	uint8_t version_verify;
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE:AW_VD_CHIP_TEMP
struct ag_chip_temp {
	int temp;
} __attribute__((packed, aligned(AG_ALIGNED)));

//TYPE:AG_VD_STROGE_CAPACITY
struct ag_stroge_capacity {
	uint32_t used;
	uint32_t free;
} __attribute__((packed, aligned(AG_ALIGNED)));
#endif
