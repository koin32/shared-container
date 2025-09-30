#ifndef VARSER_IOCTL_H
#define VARSER_IOCTL_H

/* Single header usable in kernel and user-space.
 * In kernel build __KERNEL__ is defined -> use linux/types.h and kernel-style types (u8, u32, u64).
 * In user-space -> include <stdint.h> and provide typedef aliases for u8/u32/u64.
 */

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#include <stddef.h>
/* provide kernel-style short names used below */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#endif

#include <linux/ioctl.h> /* safe in both worlds; in user-space it's usually available */

/* Sizes / limits */
#define VARSER_MAX_VAR_NAME        64
#define VARSER_MAX_CONTAINER_NAME  256
#define VARSER_MAX_VARS            128

/* Variable types */
#define VARSER_TYPE_INT32   1
#define VARSER_TYPE_INT64   2
#define VARSER_TYPE_UINT8   3
#define VARSER_TYPE_UINT64  4
#define VARSER_TYPE_FLOAT   5
#define VARSER_TYPE_DOUBLE  6
#define VARSER_TYPE_STRING  7
#define VARSER_TYPE_BLOB    8

/* Data structures passed via ioctl (packed layout assumptions) */
struct varser_var_desc {
    char name[VARSER_MAX_VAR_NAME];
    u8   type;    /* VARSER_TYPE_* */
    u32  size;    /* for string/blob */
    u8   reserved[3];
};

struct varser_register {
    char container_name[VARSER_MAX_CONTAINER_NAME];
    u32  var_count;
    u8   reserved[4];
    struct varser_var_desc vars[VARSER_MAX_VARS];
};

struct varser_var_access {
    char container_name[VARSER_MAX_CONTAINER_NAME];
    char var_name[VARSER_MAX_VAR_NAME];
    u32  buf_size;      /* size of user buffer in bytes */
    u8   reserved[4];
    unsigned long user_buf; /* uintptr_t: pointer to user-space buffer */
};

/* IOCTL numbers (both descriptive and compatibility aliases)
 *
 * We define VARSER_IOCTL_* names and also alias old VARSER_IOC_* names so existing code compiles.
 */

#define VARSER_IOCTL_MAGIC 'V'

#define VARSER_IOCTL_REGISTER  _IOW(VARSER_IOCTL_MAGIC, 1, struct varser_register)
#define VARSER_IOCTL_SET       _IOW(VARSER_IOCTL_MAGIC, 2, struct varser_var_access)
#define VARSER_IOCTL_GET       _IOWR(VARSER_IOCTL_MAGIC,3, struct varser_var_access)

#define VARSER_IOCTL_OPEN_CONTAINER   _IOW(VARSER_IOCTL_MAGIC, 4, char[VARSER_MAX_CONTAINER_NAME])
#define VARSER_IOCTL_CLOSE_CONTAINER  _IO(VARSER_IOCTL_MAGIC, 5)
#define VARSER_IOCTL_LIST_CONTAINERS  _IOR(VARSER_IOCTL_MAGIC, 6, char[4096])

/* Алиасы для старого кода */
#define VARSER_IOC_MAGIC           VARSER_IOCTL_MAGIC
#define VARSER_IOC_REGISTER        VARSER_IOCTL_REGISTER
#define VARSER_IOC_SET_VAR         VARSER_IOCTL_SET
#define VARSER_IOC_GET_VAR         VARSER_IOCTL_GET
#define VARSER_IOC_OPEN_CONTAINER  VARSER_IOCTL_OPEN_CONTAINER
#define VARSER_IOC_CLOSE_CONTAINER VARSER_IOCTL_CLOSE_CONTAINER
#define VARSER_IOC_LIST_CONTAINERS VARSER_IOCTL_LIST_CONTAINERS
/* OPEN/CLOSE already defined as VARSER_IOC_OPEN_CONTAINER / VARSER_IOC_CLOSE_CONTAINER */

#endif /* VARSER_IOCTL_H */
