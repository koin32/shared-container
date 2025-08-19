#ifndef KCONTAINER_UAPI_H
#define KCONTAINER_UAPI_H


#include <linux/ioctl.h>
#include <linux/types.h>


#define KCONTAINER_DEV_NAME "kcontainer"
#define KCONTAINER_IOC_MAGIC 'K'


/* Флаги контейнера (зарезервированы на будущее) */
#define KC_FLAG_NONE 0u


struct kc_create_req {
__u64 id; /* идентификатор контейнера */
__u64 size; /* требуемый размер, байт (округляется до страниц) */
__u32 flags; /* KC_FLAG_* */
__u32 mode; /* права доступа (битовая маска, напр. 0660) */
};


struct kc_info {
__u64 id;
__u64 size;
__u32 refcnt; /* число открытых файлов (приблизительно) */
__u32 pad;
};


/* Возвращает fd на файл контейнера (shmem). */
#define KC_IOCTL_GET_FD _IOWR(KCONTAINER_IOC_MAGIC, 1, __u64)
/* Создаёт контейнер, если его нет. */
#define KC_IOCTL_CREATE _IOW (KCONTAINER_IOC_MAGIC, 2, struct kc_create_req)
/* Уничтожает контейнер (если нет активных ссылок). */
#define KC_IOCTL_DESTROY _IOW (KCONTAINER_IOC_MAGIC, 3, __u64)
/* Получить информацию. */
#define KC_IOCTL_INFO _IOWR(KCONTAINER_IOC_MAGIC, 4, struct kc_info)


#endif /* KCONTAINER_UAPI_H */