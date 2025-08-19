// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/shmem_fs.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/anon_inodes.h>

#include "kcontainer_uapi.h"

MODULE_DESCRIPTION("Kernel shared container with shmem-backed mmap by ID");
MODULE_AUTHOR("you");
MODULE_LICENSE("GPL");

// Define kcont structure first
struct kcont {
    u64 id;
    u64 size;
    struct file *shmem_file; /* shmem file, shared */
    refcount_t refs; /* logical references to the container */
    struct hlist_node node;
};

// Define hash table and mutex after struct kcont
#define KCONT_HASH_BITS 10
static DEFINE_HASHTABLE(g_table, KCONT_HASH_BITS); /* 1024 buckets */
static DEFINE_MUTEX(g_lock);

static struct kcont *kcont_lookup(u64 id)
{
    struct kcont *c;
    hash_for_each_possible(g_table, c, node, id) {
        if (c->id == id)
            return c;
    }
    return NULL;
}

static struct kcont *kcont_create(u64 id, u64 size)
{
    struct kcont *c;
    struct file *filp;
    /* Size will be page-aligned inside shmem */
    if (size == 0)
        return ERR_PTR(-EINVAL);

    filp = shmem_file_setup("kcontainer", size, 0);
    if (IS_ERR(filp))
        return (void *)filp;

    c = kzalloc(sizeof(*c), GFP_KERNEL);
    if (!c) {
        fput(filp);
        return ERR_PTR(-ENOMEM);
    }
    c->id = id;
    c->size = size;
    c->shmem_file = filp;
    refcount_set(&c->refs, 1);

    hash_add(g_table, &c->node, c->id);
    return c;
}

static int kcont_destroy_locked(u64 id)
{
    struct kcont *c = kcont_lookup(id);
    if (!c)
        return -ENOENT;
    /* Only allow deletion if no external references exist */
    if (!refcount_dec_and_test(&c->refs)) {
        /* There are still logical references (e.g., fds in processes) */
        refcount_inc(&c->refs); /* Restore the reference */
        return -EBUSY;
    }
    hash_del(&c->node);
    fput(c->shmem_file);
    kfree(c);
    return 0;
}

static long kcontainer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case KC_IOCTL_CREATE: {
        struct kc_create_req req;
        struct kcont *c;
        if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
            return -EFAULT;
        mutex_lock(&g_lock);
        c = kcont_lookup(req.id);
        if (c) {
            mutex_unlock(&g_lock);
            return -EEXIST;
        }
        c = kcont_create(req.id, req.size);
        mutex_unlock(&g_lock);
        if (IS_ERR(c))
            return PTR_ERR(c);
        return 0;
    }
    case KC_IOCTL_GET_FD: {
        u64 id;
        struct kcont *c;
        int fd;
        if (copy_from_user(&id, (void __user *)arg, sizeof(id)))
            return -EFAULT;
        mutex_lock(&g_lock);
        c = kcont_lookup(id);
        if (!c) {
            mutex_unlock(&g_lock);
            return -ENOENT;
        }
        refcount_inc(&c->refs);
        fd = get_unused_fd_flags(O_RDWR);
        if (fd < 0) {
            refcount_dec(&c->refs);
            mutex_unlock(&g_lock);
            return fd;
        }
        fd_install(fd, get_file(c->shmem_file));
        mutex_unlock(&g_lock);
        return fd;
    }
    case KC_IOCTL_DESTROY: {
        u64 id;
        int ret;
        if (copy_from_user(&id, (void __user *)arg, sizeof(id)))
            return -EFAULT;
        mutex_lock(&g_lock);
        ret = kcont_destroy_locked(id);
        mutex_unlock(&g_lock);
        return ret;
    }
    case KC_IOCTL_INFO: {
        struct kc_info info;
        struct kcont *c;
        if (copy_from_user(&info, (void __user *)arg, sizeof(info)))
            return -EFAULT;
        mutex_lock(&g_lock);
        c = kcont_lookup(info.id);
        if (!c) {
            mutex_unlock(&g_lock);
            return -ENOENT;
        }
        info.size = c->size;
        info.refcnt = refcount_read(&c->refs);
        mutex_unlock(&g_lock);
        if (copy_to_user((void __user *)arg, &info, sizeof(info)))
            return -EFAULT;
        return 0;
    }
    default:
        return -ENOTTY;
    }
}

static const struct file_operations kcontainer_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = kcontainer_ioctl,
    .compat_ioctl = kcontainer_ioctl,
};

static struct miscdevice kcontainer_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = KCONTAINER_DEV_NAME,
    .fops = &kcontainer_fops,
};

static int __init kcontainer_init(void)
{
    int ret;
    hash_init(g_table);
    ret = misc_register(&kcontainer_dev);
    if (ret) {
        pr_err("kcontainer: failed to register device\n");
        return ret;
    }
    pr_info("kcontainer: loaded\n");
    return 0;
}

static void __exit kcontainer_exit(void)
{
    int bkt;
    struct kcont *c;
    struct hlist_node *tmp;

    pr_info("kcontainer: unloading\n");

    mutex_lock(&g_lock);
    hash_for_each_safe(g_table, bkt, tmp, c, node) {
        hash_del(&c->node);
        fput(c->shmem_file);
        kfree(c);
    }
    mutex_unlock(&g_lock);
    misc_deregister(&kcontainer_dev);
}

module_init(kcontainer_init);
module_exit(kcontainer_exit);