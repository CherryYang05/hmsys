// /**
//  * @Author      : CherryYang
//  * @Date        : 2025-01-12 18:17:07
//  * @FilePath    : /hmsys/src/tape.c
//  * @Description : 磁带设备交互
//  */

//  #include <arpa/inet.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <scsi/scsi.h>
// #include <scsi/sg.h>
// #include <stddef.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/ioctl.h>
// #include <sys/time.h>
// #include <sys/types.h>
// #include <time.h>
// #include <unistd.h>

// #define RETURN_OK (0)
// #define RETURN_ERROR (-1)

// #define PRINT_WRITEFILE

// typedef int in32_t;
// typedef unsigned char uint8_t;
// typedef unsigned short uint16_t;
// typedef unsigned int uint32_t;

// #define ltfs_u64tobe(dest, src)                    \
//     do {                                           \
//         uint32_t *tmp = (uint32_t *)dest;          \
//         uint64_t stmp = (src);                     \
//         tmp[0] = htonl((stmp >> 32) & 0xffffffff); \
//         tmp[1] = htonl(stmp & 0xffffffff);         \
//     } while (0)

// #define ltfs_betou64(buf) \
//     (uint64_t) ntohl(*((uint32_t *)(buf)) << 32) + (uint64_t)ntohl(*((uint32_t *)(buf) + 1))

// #define ltfs_betou32(buf) ntohl(*((uint32_t *)(buf)))

// #define READ 0x08
// #define WRITE 0x0A
// #define LOCATE10 0x2B
// #define LOCATE16 0x92
// #define READ_POSITION 0x34
// #define READ_END_OF_WRAP_POSITION 0xA3

// #define CDB6_LEN 6
// #define CDB10_LEN 10
// #define CDB16_LEN 16
// #define MAXSENSE 255
// #define REDPOS_LONG_LEN 32

// typedef struct tagSgDev {
//     int32_t fd;
// } SgDev;

// typedef struct tagTcPosition {
//     uint64_t block;
//     uint64_t filemarks;
//     uint32_t partition;
// } TcPosition;

// uint16_t GetBigEndian16(const void *p) {
//     return ((const uint8_t *)p)[0] << 8 | ((const uint8_t *)p)[1];
// }

// uint32_t GetBigEndian32(const void *p) {
//     return ((const uint8_t *)p)[0] << 24 |
//            ((const uint8_t *)p)[1] << 16 |
//            ((const uint8_t *)p)[2] << 8 |
//            ((const uint8_t *)p)[3];
// }

// void PutBigEndian16(uint16_t val, void *p) {
//     ((uint8_t *)p)[0] = (uint8_t)(val >> 8);
//     ((uint8_t *)p)[1] = (uint8_t)val;
// }

// void PrintBuf2(char *buf, size_t size) {
//     printf("Print buf value:\n");
//     for (uint64_t i = 0; i < size; i++) {
//         if (i != 0 && i % 256 == 0) {
//             printf("\n");
//         }
//         printf("%d ", buf[i]);
//     }
//     printf("\n");
// }

// int32_t sg_readEndOfWrap(SgDev *device) {
//     int32_t ret = RETURN_ERROR;
//     sg_io_hdr_t req = {0};
//     uint8_t cdb[CDB16_LEN] = {0};
//     uint8_t sense[MAXSENSE] = {0};
//     uint8_t buf[2560] = {0};

//     /* Build CDB */
//     cdb[0] = 0x12;
//     cdb[1] = 0x01;
//     cdb[2] = 0xB1;
//     cdb[3] = 0;
//     cdb[4] = 0x40;

//     /* Build Request */
//     req.dxfer_direction = SG_DXFER_FROM_DEV;
//     req.cmd_len = sizeof(cdb);
//     req.mx_sb_len = sizeof(sense);
//     req.dxfer_len = sizeof(buf);
//     req.dxferp = buf;
//     req.cmdp = cdb;
//     req.sbp = sense;
//     req.timeout = 60 * 1000;
//     req.interface_id = 'S';

//     ret = ioctl(device->fd, SG_IO, &req);
//     if (ret >= 0) {
//         uint16_t wrapCount = 0;
//         uint16_t respSize = GetBigEndian16(buf);
//         respSize += sizeof(uint16_t);
//         printf("respSize: %u\n", respSize);
//         PrintBuf2(buf, 512);
//         uint16_t i = 0;
//         i += 4;
//     } else {
//         printf("ioctl error, dev(%d) errno(%d)\n", device->fd, errno);
//     }
//     return ret;
// }

// int32_t sg_readpos(SgDev *device, TcPosition *pos) {
//     int32_t ret = RETURN_ERROR;
//     sg_io_hdr_t req = {0};
//     uint8_t cdb[CDB10_LEN] = {READ_POSITION, 0x06, 0, 0, 0, 0, 0, 0, 0, 0};
//     uint8_t sense[MAXSENSE] = {0};
//     uint8_t buf[REDPOS_LONG_LEN] = {0};

//     /* Build CDB */
//     // cdb[0] = READ_POSITION;
//     // cdb[1] = 0x06;  // long format
//     // cdb[2] = 0xB1;
//     // cdb[3] = 0;
//     // cdb[4] = 0x40;

//     /* Build Request */
//     memset(&req, 0, sizeof(sg_io_hdr_t));
//     req.dxfer_direction = SG_DXFER_FROM_DEV;
//     req.cmd_len = sizeof(cdb);
//     req.mx_sb_len = sizeof(req);
//     req.dxfer_len = 32;
//     req.dxferp = buf;

//     req.cmdp = cdb;
//     req.sbp = &req;
//     req.timeout = 20000;
//     req.interface_id = 'S';

//     ret = ioctl(device->fd, SG_IO, &req);
//     if (ret >= 0) {
//         // uint16_t wrapCount = 0;
//         // uint16_t respSize = GetBigEndian16(buf);
//         // respSize += sizeof(uint16_t);
//         // printf("respSize: %u\n", respSize);
//         // PrintBuf(buf, 32);
//         // uint16_t i = 0;
//         // i += 4;
//         pos->partition = ltfs_betou32(buf + 4);
//         pos->block = ltfs_betou64(buf + 8);
//         pos->filemarks = ltfs_betou64(buf + 16);
//     } else {
//         printf("ioctl error, dev(%d) errno(%d)\n", device->fd, errno);
//     }
//     return ret;
// }

// int32_t sg_locate10(SgDev *device, TcPosition dest, TcPosition *pos) {
//     int32_t ret = RETURN_ERROR;
//     sg_io_hdr_t req = {0};
//     uint8_t cdb[CDB16_LEN] = {0};
//     uint8_t sense[MAXSENSE] = {0};

//     // printf("locate: dst part(%llu) block(%llu)\n", (uint64_t)dest.partition, (uint64_t)dest.block);

//     cdb[0] = LOCATE10;
//     cdb[1] = (0 << 2) | (0 << 1) | 0;
//     cdb[3] = (dest.block >> 24) & 0xFF;  // 31:24
//     cdb[4] = (dest.block >> 16) & 0xFF;  // 23:16
//     cdb[5] = (dest.block >> 8) & 0xFF;   // 15:8
//     cdb[6] = dest.block & 0xFF;          // 7:0
//     cdb[8] = dest.partition;

//     req.dxfer_direction = SG_DXFER_NONE;
//     req.cmd_len = sizeof(cdb);
//     req.mx_sb_len = sizeof(sense);
//     req.cmdp = cdb;
//     req.sbp = sense;
//     req.timeout = 60 * 1000;
//     req.interface_id = 'S';

//     ret = ioctl(device->fd, SG_IO, &req);
//     if (ret < 0) {
//         printf("locate ioctl error(%d)\n", errno);
//         return ret;
//     }
//     return sg_readpos(device, pos);
// }

// int32_t sg_locate(SgDev *device, TcPosition dest, TcPosition *pos) {
//     int32_t ret = RETURN_ERROR;
//     sg_io_hdr_t req = {0};
//     uint8_t cdb[CDB16_LEN] = {0};
//     uint8_t sense[MAXSENSE] = {0};

//     // printf("locate: dst part(%llu) block(%llu)\n", (uint64_t)dest.partition, (uint64_t)dest.block);

//     cdb[0] = LOCATE16;
//     if (pos->partition != dest.partition) {
//         cdb[1] = 0x02;  // set change partition(CP) flag
//     }
//     cdb[3] = (uint8_t)(dest.partition & 0xff);
//     ltfs_u64tobe(cdb + 4, dest.block);

//     req.dxfer_direction = SG_DXFER_NONE;
//     req.cmd_len = sizeof(cdb);
//     req.mx_sb_len = sizeof(sense);
//     req.cmdp = cdb;
//     req.sbp = sense;
//     req.timeout = 60 * 1000;
//     req.interface_id = 'S';

//     ret = ioctl(device->fd, SG_IO, &req);
//     if (ret < 0) {
//         printf("locate ioctl error(%d)\n", errno);
//         return ret;
//     }
//     return sg_readpos(device, pos);
// }

// void PrintBuf(char *buf, size_t size) {
//     printf("Print buf value:\n");
//     for (uint64_t i = 0; i < size; i++) {
//         if (i != 0 && i % 256 == 0) {
//             printf("\n");
//         }
//         printf("%d ", buf[i]);
//     }
//     printf("\n");
// }

// int32_t sg_read(SgDev *device, char *buf, size_t io_size, TcPosition *pos) {
//     int32_t ret = RETURN_ERROR;
//     sg_io_hdr_t req = {0};
//     uint8_t cdb[CDB16_LEN] = {0};
//     uint8_t sense[MAXSENSE] = {0};

//     // printf("read: part(%llu) block(%llu) size(%llu)\n", (uint64_t)pos->partition, (uint64_t)pos->block, size);

//     cdb[0] = READ;
//     cdb[1] = 0x02;
//     cdb[2] = (io_size >> 16) & 0xFF;
//     cdb[3] = (io_size >> 8) & 0xFF;
//     cdb[4] = (io_size)&0xFF;

//     req.dxfer_direction = SG_DXFER_FROM_DEV;
//     req.cmd_len = sizeof(cdb);
//     req.mx_sb_len = sizeof(sense);
//     req.dxfer_len = io_size;
//     req.dxferp = (uint8_t *)buf;
//     req.cmdp = cdb;
//     req.sbp = sense;
//     req.timeout = 60 * 1000;
//     req.flags = SG_FLAG_DIRECT_IO;
//     req.interface_id = 'S';

//     ret = ioctl(device->fd, SG_IO, &req);
//     if (ret < 0) {
//         printf("locate ioctl error(%d)\n", errno);
//     }
//     return ret;
// }

// int32_t ScsiStreamWrite(SgDev *device, uint8_t *data, uint32_t dataLen, TcPosition *pos) {
//     int32_t ret = RETURN_ERROR;
//     uint8_t cdb[CDB6_LEN] = {0};
//     cdb[0] = WRITE;
//     cdb[2] = (dataLen >> 16) & 0xFF;
//     cdb[3] = (dataLen >> 8) & 0xFF;
//     cdb[4] = dataLen & 0xFF;

//     printf("write: part(%llu) block(%llu) size(%llu)\n", (uint64_t)pos->partition, (uint64_t)pos->block, dataLen);

//     uint8_t sense[MAXSENSE] = {0};
//     sg_io_hdr_t sgData;
//     (void)memset(&sgData, 0, sizeof(sg_io_hdr_t));
//     sgData.dxfer_direction = SG_DXFER_TO_DEV;
//     sgData.dxfer_len = dataLen;
//     sgData.dxferp = data;
//     sgData.cmdp = cdb;
//     sgData.cmd_len = sizeof(cdb);
//     sgData.sbp = sense;
//     sgData.mx_sb_len = sizeof(sense);
//     sgData.timeout = 60 * 1000;
//     sgData.interface_id = 'S';

//     ret = ioctl(device->fd, SG_IO, &sgData);
//     if (ret < 0) {
//         printf("locate ioctl error(%d)\n", errno);
//     }
//     return ret;
// }

// // int32_t sg_readpos(SgDev *device, TcPosition *pos) {
// //     int32_t ret = RETURN_ERROR;
// //     sg_io_hdr_t req = {0};
// //     uint8_t cdb[CDB10_LEN] = {0};
// //     uint8_t sense[MAXSENSE] = {0};
// //     uint8_t buf[REDPOS_LONG_LEN] = {0};
// //     /* Build CDB */
// //     cdb[0] = READ_POSITION;
// //     cdb[1] = 0x06; /* Long Format */

// //     req.dxfer_direction = SG_DXFER_FROM_DEV;
// //     req.cmd_len = sizeof(cdb);
// //     req.mx_sb_len = sizeof(sense);
// //     req.dxfer_len = sizeof(buf);
// //     req.dxferp = (uint8_t *)buf;
// //     req.cmdp = cdb;
// //     req.sbp = sense;
// //     req.timeout = 60 * 1000;
// //     req.flags = SG_FLAG_DIRECT_IO;
// //     req.interface_id = 'S';

// //     ret = ioctl(device->fd, SG_IO, &req);
// //     if (ret < 0) {
// //         printf("locate ioctl error(%d)\n", errno);
// //     }
// //     return ret;
// // }

// int32_t sg_write(SgDev *device, uint8_t *buf, size_t size, TcPosition *pos) {
//     int32_t ret = RETURN_ERROR;
//     sg_io_hdr_t req = {0};
//     uint8_t cdb[CDB6_LEN] = {0};
//     uint8_t sense[MAXSENSE] = {0};

//     // printf("write: part(%llu) block(%llu) size(%llu)\n", (uint64_t)pos->partition, (uint64_t)pos->block, size);

//     cdb[0] = WRITE;
//     cdb[1] = 0x00;
//     cdb[2] = (size >> 16) & 0xFF;
//     cdb[3] = (size >> 8) & 0xFF;
//     cdb[4] = (size)&0xFF;

//     req.dxfer_direction = SG_DXFER_TO_DEV;
//     req.cmd_len = sizeof(cdb);
//     req.mx_sb_len = sizeof(sense);
//     req.dxfer_len = size;
//     req.dxferp = (unsigned char *)buf;
//     req.cmdp = cdb;
//     req.sbp = sense;
//     req.timeout = 60 * 1000;
//     req.flags = SG_FLAG_DIRECT_IO;
//     req.interface_id = 'S';

//     ret = ioctl(device->fd, SG_IO, &req);
//     if (ret < 0) {
//         printf("locate ioctl error(%d)\n", errno);
//     }
//     return ret;
// }

// int32_t sg_modesense(SgDev *device, uint8_t *buf, size_t size, TcPosition *pos) {
//     int32_t ret = RETURN_OK;
//     sg_io_hdr_t req = {0};
//     uint8_t cdb[CDB10_LEN] = {0};
//     uint8_t sense[MAXSENSE] = {0};

//     // printf("write: part(%llu) block(%llu) size(%llu)\n", (uint64_t)pos->partition, (uint64_t)pos->block, size);

//     cdb[0] = 0x5A;
//     cdb[1] = 0x00;
//     cdb[2] = 0x10 | (0x0F & 0x3F);  // 0x0F 表示不启用压缩
//     cdb[3] = 0x00;

//     req.dxfer_direction = SG_DXFER_TO_DEV;
//     req.cmd_len = sizeof(cdb);
//     req.mx_sb_len = sizeof(req);
//     req.dxfer_len = size;
//     req.dxferp = (unsigned char *)buf;
//     req.cmdp = cdb;
//     req.sbp = &req;
//     req.timeout = 60 * 1000;
//     req.flags = SG_FLAG_DIRECT_IO;
//     req.interface_id = 'S';

//     ret = ioctl(device->fd, SG_IO, &req);
//     if (ret < 0) {
//         printf("locate ioctl error(%d)\n", errno);
//     }
//     PrintBuf2(buf, size);
//     return ret;
// }

// int32_t sg_modeselect(SgDev *device, uint8_t *buf, size_t size, TcPosition *pos) {
//     int32_t ret = RETURN_OK;
//     sg_io_hdr_t req = {0};
//     uint8_t cdb[CDB10_LEN] = {0};
//     uint8_t sense[MAXSENSE] = {0};

//     // printf("write: part(%llu) block(%llu) size(%llu)\n", (uint64_t)pos->partition, (uint64_t)pos->block, size);

//     cdb[0] = 0x55;
//     cdb[1] = 0x11;
//     cdb[2] = 0x00;
//     cdb[3] = 0x00;
//     cdb[7] = (size >> 8) & 0xFF;
//     cdb[8] = size & 0xFF;
//     // cdb[9] = 

//     req.dxfer_direction = SG_DXFER_TO_DEV;
//     req.cmd_len = sizeof(cdb);
//     req.mx_sb_len = sizeof(req);
//     req.dxfer_len = size;
//     req.dxferp = (unsigned char *)buf;
//     req.cmdp = cdb;
//     req.sbp = &req;
//     req.timeout = 60 * 1000;
//     req.flags = SG_FLAG_DIRECT_IO;
//     req.interface_id = 'S';

//     ret = ioctl(device->fd, SG_IO, &req);
//     if (ret < 0) {
//         printf("locate ioctl error(%d)\n", errno);
//         return ret;
//     }
//     PrintBuf2(buf, size);
//     return ret;
// }

// int32_t sg_rewind(SgDev *device) {
//     int32_t ret = RETURN_OK;
//     sg_io_hdr_t req = {0};
//     uint8_t cdb[CDB10_LEN] = {0};
//     uint8_t sense[MAXSENSE] = {0};

//     // printf("write: part(%llu) block(%llu) size(%llu)\n", (uint64_t)pos->partition, (uint64_t)pos->block, size);

//     cdb[0] = 0x01;
//     cdb[1] = 0x00;
//     cdb[2] = 0x00;
//     cdb[3] = 0x00;

//     req.dxfer_direction = SG_DXFER_TO_DEV;
//     req.cmd_len = sizeof(cdb);
//     req.mx_sb_len = sizeof(req);
//     // req.dxfer_len = size;
//     // req.dxferp = (unsigned char *)buf;
//     req.cmdp = cdb;
//     req.sbp = &req;
//     req.timeout = 60 * 1000;
//     req.flags = SG_FLAG_DIRECT_IO;
//     req.interface_id = 'S';

//     ret = ioctl(device->fd, SG_IO, &req);
//     if (ret < 0) {
//         printf("locate ioctl error(%d)\n", errno);
//     }
//     return ret;
// }




// void PrintCurPos(SgDev *sgDev, TcPosition *pos) {
//     int32_t ret = sg_readpos(sgDev, pos);
//     // printf("*readpos ret: %d\n", ret);
//     printf("readpos: filemarks(%lu) block(%lu) partition(%llu)\n",  (uint64_t)pos->filemarks, (uint64_t)pos->block, (uint64_t)pos->partition);
// }

// void ftdsStart(struct timeval *startTime) {
//     gettimeofday(startTime, NULL);
// }

// /**
//  * @brief
//  *
//  * @param startTime
//  * @param costTimeS
//  */
// void ftdsEnd(struct timeval *startTime, double *costTimeS) {
//     struct timeval endTime;
//     gettimeofday(&endTime, NULL);
//     uint64_t costTime = (endTime.tv_sec - startTime->tv_sec) * 1000 * 1000 + endTime.tv_usec - startTime->tv_usec;
//     *costTimeS = (double)costTime / 1000 / 1000;
// }

// /**
//  * @brief
//  *
//  * @param startTime
//  * @param costTimeU
//  */
// void ftdsEndUs(struct timeval *startTime, uint64_t *costTimeU) {
//     struct timeval endTime;
//     gettimeofday(&endTime, NULL);
//     *costTimeU = (endTime.tv_sec - startTime->tv_sec) * 1000 * 1000 + endTime.tv_usec - startTime->tv_usec;
// }

// void PrintfDelayPic(uint64_t *totalIoCost, uint64_t *totalIoCostCnt) {
//     if (*totalIoCostCnt >= 10) {
//         // printf("ftds read avg %lfn", (double)totalloCost / totalloCostCnt);
//         double avg_delay = (double)*totalIoCost / *totalIoCostCnt;
//         if (avg_delay < 1000000) { // less than 1s 
//             for (int j = 0; j < avg_delay; j += 100) {
//                 printf("-");
//             }
//             //printf("\n");
//             printf("%.2lf us\n", avg_delay);
//         } else {
//             for (int j = 0; j < avg_delay; j += 100000) {
//                 printf ("*");
//             }
//             //printf("\n");
//             printf("%.2lf s\n", avg_delay / 1000000);
//             *totalIoCost = 0;
//             *totalIoCostCnt = 0;
//         }
//     }
// }

// void PrintfAvg(uint64_t *totalIoCost, uint64_t *totalIoCostCnt, char *str) {
//     if (*totalIoCostCnt >= 10) {
//         double avg_delay = (double)*totalIoCost / *totalIoCostCnt;
//         printf("%s %.2lf us\n", str, avg_delay);
//         *totalIoCost = 0;
//         *totalIoCostCnt = 0;
//     }
// }

// /**
//  * @brief 
//  * 
//  * @param argc 
//  * @param argv
//  *        argv[1]: 设备名
//  *        argv[2]: 指令名
//  * readpos:
//  *          获取磁带机当前位置，包括磁头位置信息，文件标记
//  * 
//  * readwrap:
//  * 
//  * locate:
//  * 
//  * read:
//  *          从磁带机读取指定的字节数
//  *          argv[3]: 要读取的字节数
//  * 
//  * readfile:
//  *          从磁带机读取文件
//  *          argv[3]: bufSize(io size), 每次读取的数据大小(单位: B)
//  *          argv[4]: dataCap, 要读取的数据总量(单位: KB)
//  * write:
//  * 
//  * writefile:
//  *          将文件写入磁带机。注意：代码只支持读取一个 bufSize 的文件，
//  *          若指定的写入大小超过 bufSize，则重复写入该 bufSize 大小的文件
//  *          argv[3]: bufSize, 每次写入的数据大小(单位是 B)
//  *          argv[4]: filename, 要写入的文件名
//  *          argv[5]: dataCap, 要写入的数据总大小(单位是 KB)
//  * 
//  * modesense:
//  * 
//  * 
//  * @return int 
//  */
// int main(int32_t argc, char **argv) {
//     int32_t ret = RETURN_ERROR;
//     SgDev sgDev = {0};
//     TcPosition pos = {0};

//     if (argc < 3) {
//         printf("Input: ./main /dev/IBMtape0n op [num]\n");
//         return RETURN_ERROR;
//     }
//     sgDev.fd = open(argv[1], O_RDWR);
//     // printf("dev(%d)\n", sgDev.fd);

//     if (sgDev.fd < 0) {
//         printf("Open dev(%s) failed.\n", argv[1]);
//         return RETURN_ERROR;
//     }
//     if (strcmp(argv[2], "readpos") == 0) {
//         ret = sg_readpos(&sgDev, &pos);
//         printf("readpos ret: %d\n", ret);
//         printf("readpos filemarks(%llu) block(%llu) partition(%llu)\n", (uint64_t)pos.filemarks, (uint64_t)pos.block, (uint64_t)pos.partition);
//         if (ret != RETURN_OK) {
//             close(sgDev.fd);
//             return ret;
//         }
//     }

//     if (strcmp(argv[2], "readwrap") == 0) {
//         ret = sg_readEndOfWrap(&sgDev);
//         printf("readwrap ret:%d\n", ret);
//         if (ret != RETURN_OK) {
//             close(sgDev.fd);
//             return ret;
//         }


//     } else if (strcmp(argv[2], "locate") == 0) {
//         TcPosition dest = {0};
//         dest.block = atoi(argv[3]);
//         struct timeval startTime;
//         double costTimeS = 0;
//         ftdsStart(&startTime);
//         ret = sg_locate10(&sgDev, dest, &pos);
//         ftdsEnd(&startTime, &costTimeS);
//         // printf("locate cost time(%lf), ret: %d\n", costTimeS, ret);
//         // printf("locate partition(%llu), block(%llu) filemarks(%llu)\n", (uint64_t)pos.partition, (uint64_t)pos.block, (uint64_t)pos.filemarks);
//         if (ret != RETURN_OK) {
//             close(sgDev.fd);
//             return ret;
//         }



//     } else if (strcmp(argv[2], "read") == 0) {
//         uint32_t bufSize = atoi(argv[3]);
//         char *buf = (char *)malloc(bufSize);
//         if (buf == NULL) {
//             printf("Malloc buf size(%llu) failed\n", bufSize);
//             close(sgDev.fd);
//             return RETURN_ERROR;
//         }
//         PrintCurPos(&sgDev, &pos);
//         ret = sg_read(&sgDev, buf, bufSize, &pos);
//         // printf("read ret:%d\n", ret);
//         if (ret != RETURN_OK) {
//             printf("read dev(%d) part(%u) block(%llu) size(%llu) failed, ret(%d).\n", sgDev.fd, pos.partition, pos.block, bufSize, ret);
//         } else {
//             PrintBuf(buf, bufSize);
//         }
//         PrintCurPos(&sgDev, &pos);



//     } else if (strcmp(argv[2], "readfile") == 0) {
//         uint32_t bufSize = atoi(argv[3]);
//         uint64_t block1;
//         uint64_t block2;
//         char *buf = (char *)malloc(bufSize);
//         if (buf == NULL) {
//             printf("Malloc buf size(%llu) failed\n", bufSize);
//             close(sgDev.fd);
//             return RETURN_ERROR;
//         }
//         PrintCurPos(&sgDev, &pos);
//         TcPosition dest = {0};
//         uint64_t dataCap = atoi(argv[4]);   // KB
//         uint64_t loop = dataCap / (bufSize / 1024);
//         struct timeval startTime, ioTime;
//         double costTimeS = 0;
//         uint64_t ioCost = 0;
//         uint64_t totalIoCost = 0;
//         uint64_t totalIoCostCnt = 0;
//         printf("Read IO start, cap(%llu KB), io size(%llu B)\n", dataCap, bufSize);
//         printf("io idx | delay(us)\n");
//         ftdsStart(&startTime);
//         for (int i = 0; i < loop; i++) {
//             ftdsStart(&ioTime);
//             ret = sg_read(&sgDev, buf, bufSize, &pos);
//             if (i == 0) {
//                 block1 = pos.block;
//                 printf("read: part(%llu) block(%llu) size(%llu)\n", (uint64_t)pos.partition, (uint64_t)pos.block, bufSize);
//             }
//             ftdsEndUs(&ioTime, &ioCost);
//             totalIoCost += ioCost;
//             totalIoCostCnt++;
//             // printf("ftds_readfile %d %llu %d\n", i, ioCost, ret);
//             // PrintfAvg(&totalIoCost, &totalIoCostCnt, "ftds_readfile_avg");
//             // PrintfDelayPic(&totalIoCost, &totalIoCostCnt);
//             if (ret != RETURN_OK) {
//                 printf("read dev(%d) part(%u) block(%llu) size(%llu) failed, ret(%d).\n", sgDev.fd, pos.partition, pos.block, bufSize, ret);
//             }
//         }
//         ftdsEnd(&startTime, &costTimeS);
//         printf("read %luMB cost time(%lfs), bw(%lf MB/s), avg_delay(%lfus)\n", dataCap / 1024, costTimeS, (dataCap / 1024) / costTimeS, costTimeS * 1000 * 1000 / loop);
        
//         free(buf);
//         PrintCurPos(&sgDev, &pos);
//         block2 = pos.block;
//         printf("read block: %ld\n", block2 - block1);



//     } else if (strcmp(argv[2], "write") == 0) {
//         uint32_t bufSize = atoi(argv[3]);
//         char *buf = (char *)malloc(bufSize);
//         if (buf == NULL) {
//             printf("Malloc buf size(%llu) failed\n", bufSize);
//             close(sgDev.fd);
//             return RETURN_ERROR;
//         }
//         for (int32_t i = 0; i < bufSize; i++) {
//             buf[i] = 3;
//         }

//         PrintCurPos(&sgDev, &pos);
//         ret = sg_write(&sgDev, buf, bufSize, &pos);
//         printf("write ret: %d\n", ret);
//         if (ret != RETURN_OK) {
//             printf("write: dev(%d) part(%u) block(%llu) size(%llu) failed, ret(%d).\n", sgDev.fd, pos.partition, pos.block, bufSize, ret);
//         }
//         free(buf);
//         PrintCurPos(&sgDev, &pos);



//     } else if (strcmp(argv[2], "writefile") == 0) {
//         uint32_t bufSize = atoi(argv[3]);       // 写入的块大小
//         char *buf = (char *)malloc(bufSize);
//         if (buf == NULL) {
//             printf("Malloc buf size(%llu) failed\n", bufSize);
//             close(sgDev.fd);
//             return RETURN_ERROR;
//         }
//         FILE *fp = fopen(argv[4], "r");
//         if (fp == NULL) {
//             printf("Open file(%s) failed.\n", argv[4]);
//             free(buf);
//             close(sgDev.fd);
//             return RETURN_ERROR;
//         } else {
//             fread(buf, sizeof(buf[0]), bufSize, fp);
//             fclose(fp);
//         }

//         PrintCurPos(&sgDev, &pos);

//         uint64_t dataCap = atol(argv[5]);       // KB，要写入的总数据量大小
//         uint64_t loop = dataCap / (bufSize / 1024);
//         struct timeval startTime, ioTime;
//         double costTimeS = 0;
//         uint64_t ioCost = 0;
//         uint64_t totalIoCost = 0;
//         uint64_t totalIoCostCnt = 0;
//         printf("Read IO start, cap(%llu KB), io size(%llu B)\n", dataCap, bufSize);
//         printf("io idx | delay(us)\n");
//         ftdsStart(&startTime);
//         for (int i = 0; i < loop; i++) {
//             ftdsStart(&ioTime);
//             ret = sg_write(&sgDev, buf, bufSize, &pos);
//             ftdsEndUs(&ioTime, &ioCost);
//             totalIoCost += ioCost;
//             totalIoCostCnt++;
// #ifdef PRINT_WRITEFILE
//     printf("ftds_readfile %d %llu %d\n", i, ioCost, ret);
//     PrintfAvg(&totalIoCost, &totalIoCostCnt, "ftds_writefile_avg");
//     PrintfDelayPic(&totalIoCost, &totalIoCostCnt);
// #endif
//             if (ret != RETURN_OK) {
//                 printf("write dev(%d) part(%u) block(%llu) size(%llu) failed, ret(%d).\n", sgDev.fd, pos.partition, pos.block, bufSize, ret);
//             }
//         }
//         ftdsEnd(&startTime, &costTimeS);
//         printf("write %luMB cost time(%lf), bw(%lf MB/s), avg_delay(%lfus)\n", dataCap / 1024, costTimeS, (dataCap / 1024) / costTimeS, costTimeS * 1000 * 1000 / loop);
        
//         free(buf);
//         PrintCurPos(&sgDev, &pos);



//     } else if (strcmp(argv[2], "modesense") == 0) {
//         uint32_t bufSize = 32;
//         char buf[32] = {0};
//         ret = sg_modesense(&sgDev, buf, bufSize, &pos);




//     } else if (strcmp(argv[2], "modeselect") == 0) {
//         uint32_t bufSize = 32;
//         char buf[32] = {0};
//         ret = sg_modeselect(&sgDev, buf, bufSize, &pos);




//     } else if (strcmp(argv[2], "rewind") == 0) {
//         ret = sg_rewind(&sgDev);


//     }
//     close(sgDev.fd);
//     return ret;
// }
