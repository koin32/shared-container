// Simplified working skeleton. Requires building against correct kernel headers.
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/list.h>
#include <linux/kref.h>
#include <linux/string.h>

#include "varser_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varser Example");
MODULE_DESCRIPTION("Varser container kernel module (skeleton)");

/* per-variable */
struct varser_var {
    char name[VARSER_MAX_VAR_NAME];
    uint8_t type;
    uint32_t size; /* allocated size */
    void *data;    /* kernel buffer */
    struct rw_semaphore rw; /* per-variable rw lock */
    struct list_head list;
};

/* container */
struct varser_container {
    char name[VARSER_MAX_CONTAINER_NAME];
    struct list_head vars;
    struct kref refcount;
    struct mutex container_lock; /* protects vars list */
    struct list_head list; /* global containers list linkage */
    int lock_policy;
};

static LIST_HEAD(container_list);
static DEFINE_MUTEX(global_list_lock);

/* helper: find by name (assume caller holds global_list_lock) */
static struct varser_container *find_container_locked(const char *name)
{
    struct varser_container *c;
    list_for_each_entry(c, &container_list, list) {
        if (strncmp(c->name, name, VARSER_MAX_CONTAINER_NAME) == 0)
            return c;
    }
    return NULL;
}

/* release function for kref */
static void varser_container_release(struct kref *kref)
{
    struct varser_container *c = container_of(kref, struct varser_container, refcount);
    struct varser_var *v, *tmp;

    mutex_lock(&c->container_lock);
    list_for_each_entry_safe(v, tmp, &c->vars, list) {
        list_del(&v->list);
        if (v->data) kfree(v->data);
        kfree(v);
    }
    mutex_unlock(&c->container_lock);

    mutex_lock(&global_list_lock);
    list_del(&c->list);
    mutex_unlock(&global_list_lock);

    kfree(c);
    pr_info("varser: container '%s' freed\n", c->name);
}

/* create/init container (called while holding global_list_lock or not) */
static struct varser_container *varser_create_container(const struct varser_register *reg)
{
    struct varser_container *c;
    unsigned i;

    c = kzalloc(sizeof(*c), GFP_KERNEL);
    if (!c) return NULL;
    INIT_LIST_HEAD(&c->vars);
    mutex_init(&c->container_lock);
    kref_init(&c->refcount);
    strncpy(c->name, reg->container_name, VARSER_MAX_CONTAINER_NAME-1);
    c->lock_policy = 0;

    for (i = 0; i < reg->var_count && i < VARSER_MAX_VARS; ++i) {
        struct varser_var *v = kzalloc(sizeof(*v), GFP_KERNEL);
        if (!v) { /* free allocated so far */ goto err_vars; }
        INIT_LIST_HEAD(&v->list);
        strncpy(v->name, reg->vars[i].name, VARSER_MAX_VAR_NAME-1);
        v->type = reg->vars[i].type;
        v->size = reg->vars[i].size ? reg->vars[i].size : 8; /* default small size */
        v->data = kzalloc(v->size, GFP_KERNEL);
        if (!v->data) { kfree(v); goto err_vars; }
        init_rwsem(&v->rw);
        list_add_tail(&v->list, &c->vars);
    }

    list_add_tail(&c->list, &container_list);
    pr_info("varser: created container '%s' vars=%u\n", c->name, reg->var_count);
    return c;

err_vars:
    {
        struct varser_var *vv, *tmp;
        list_for_each_entry_safe(vv, tmp, &c->vars, list) {
            list_del(&vv->list);
            if (vv->data) kfree(vv->data);
            kfree(vv);
        }
        kfree(c);
        return NULL;
    }
}

/* file->private_data will store pointer to container when opened with OPEN_CONTAINER */
static long varser_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *uarg = (void __user *)arg;

    if (_IOC_TYPE(cmd) != VARSER_IOCTL_MAGIC) return -ENOTTY;

    switch (cmd) {
    case VARSER_IOCTL_REGISTER:
    {
        struct varser_register reg;
        if (copy_from_user(&reg, uarg, sizeof(reg))) return -EFAULT;

        mutex_lock(&global_list_lock);
        if (find_container_locked(reg.container_name)) {
            mutex_unlock(&global_list_lock);
            return -EEXIST;
        }
        if (!varser_create_container(&reg)) {
            mutex_unlock(&global_list_lock);
            return -ENOMEM;
        }
        mutex_unlock(&global_list_lock);
        return 0;
    }
    case VARSER_IOC_OPEN_CONTAINER:
    {
        char name[VARSER_MAX_CONTAINER_NAME];
        struct varser_container *c;
        if (copy_from_user(name, uarg, VARSER_MAX_CONTAINER_NAME)) return -EFAULT;

        mutex_lock(&global_list_lock);
        c = find_container_locked(name);
        if (!c) {
            mutex_unlock(&global_list_lock);
            return -ENOENT;
        }
        kref_get(&c->refcount);
        mutex_unlock(&global_list_lock);
        file->private_data = c;
        return 0;
    }
    case VARSER_IOC_CLOSE_CONTAINER:
    {
        struct varser_container *c = file->private_data;
        if (!c) return -EINVAL;
        file->private_data = NULL;
        kref_put(&c->refcount, varser_container_release);
        return 0;
    }
    case VARSER_IOCTL_GET:
    case VARSER_IOCTL_SET:
    {
        struct varser_var_access access;
        struct varser_container *c = file->private_data;
        struct varser_var *v;
        int found = 0;

        if (copy_from_user(&access, uarg, sizeof(access))) return -EFAULT;
        if (!c) return -EINVAL;
        /* find variable under container lock */
        mutex_lock(&c->container_lock);
        list_for_each_entry(v, &c->vars, list) {
            if (strncmp(v->name, access.var_name, VARSER_MAX_VAR_NAME) == 0) { found = 1; break; }
        }
        mutex_unlock(&c->container_lock);
        if (!found) return -ENOENT;

        if (cmd == VARSER_IOCTL_GET) {
            /* read lock */
            down_read(&v->rw);
            if (access.buf_size < v->size) { up_read(&v->rw); return -EINVAL; }
            if (access.user_buf == 0) { up_read(&v->rw); return -EINVAL; }
            if (copy_to_user((void __user *)((uintptr_t)access.user_buf), v->data, v->size)) {
                up_read(&v->rw);
                return -EFAULT;
            }
            up_read(&v->rw);
            return 0;
        } else {
            /* set */
            down_write(&v->rw);
            if (access.buf_size < v->size) { up_write(&v->rw); return -EINVAL; }
            if (access.user_buf == 0) { up_write(&v->rw); return -EINVAL; }
            if (copy_from_user(v->data, (void __user *)((uintptr_t)access.user_buf), v->size)) {
                up_write(&v->rw);
                return -EFAULT;
            }
            up_write(&v->rw);
            return 0;
        }
    }
    case VARSER_IOC_LIST_CONTAINERS:
    {
        char buf[4096];
        size_t pos = 0;
        struct varser_container *c;
        mutex_lock(&global_list_lock);
        list_for_each_entry(c, &container_list, list) {
            int len = snprintf(buf + pos, sizeof(buf) - pos, "%s\n", c->name);
            if (len < 0 || pos + len >= sizeof(buf)) break;
            pos += len;
        }
        mutex_unlock(&global_list_lock);
        if (copy_to_user(uarg, buf, pos)) return -EFAULT;
        return pos;
    }
    default:
        return -ENOTTY;
    }
}

static int varser_open(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}

static int varser_release(struct inode *inode, struct file *file)
{
    struct varser_container *c = file->private_data;
    if (c) {
        file->private_data = NULL;
        kref_put(&c->refcount, varser_container_release);
    }
    return 0;
}

static const struct file_operations varser_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = varser_ioctl,
    .open = varser_open,
    .release = varser_release,
};

static struct miscdevice varser_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "varser",
    .fops = &varser_fops,
};

static int __init varser_init(void)
{
    int ret = misc_register(&varser_misc);
    if (ret) {
        pr_err("varser: misc_register failed %d\n", ret);
        return ret;
    }
    pr_info("varser: module loaded\n");
    return 0;
}

static void __exit varser_exit(void)
{
    misc_deregister(&varser_misc);

    /* try to free all containers (if any remain) */
    mutex_lock(&global_list_lock);
    while (!list_empty(&container_list)) {
        struct varser_container *c = list_first_entry(&container_list, struct varser_container, list);
        list_del(&c->list);
        /* force release */
        varser_container_release(&c->refcount);
    }
    mutex_unlock(&global_list_lock);

    pr_info("varser: module unloaded\n");
}

module_init(varser_init);
module_exit(varser_exit);
