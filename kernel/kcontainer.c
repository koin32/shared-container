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


struct kcont {
u64 id;
u64 size;
struct file *shmem_file; /* файл shmem, разделяемый */
refcount_t refs; /* логические ссылки контейнера */
struct hlist_node node;
};


static DEFINE_HASHTABLE(g_table, 10); /* 1024 бакета */
static DEFINE_MUTEX(g_lock);


static struct kcont *kcont_lookup(u64 id)
{
struct kcont *c;
hash_for_each_possible(g_table, c, node, id) {
if (c->id == id) return c;
}
return NULL;
}


static struct kcont *kcont_create(u64 id, u64 size)
{
struct kcont *c;
struct file *filp;
/* Выравнивание до страницы произойдёт внутри shmem */
if (size == 0) return ERR_PTR(-EINVAL);


filp = shmem_file_setup("kcontainer", size, 0);
if (IS_ERR(filp)) return (void*)filp;


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
if (!c) return -ENOENT;
/* Разрешаем удалять только если нет внешних ссылок на файл */
if (!refcount_dec_and_test(&c->refs)) {
/* Есть ещё логические ссылки (например, fd у процессов) */
refcount_inc(&c->refs); /* вернуть обратно */
return -EBUSY;
}
hash_del(&c->node);
fput(c->shmem_file);
kfree(c);
return 0;
}


module_exit(kcontainer_exit);