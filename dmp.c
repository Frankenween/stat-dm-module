#include <linux/device-mapper.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>

// For DMERR
#define DM_MSG_PREFIX "stat proxy"

struct dmp_data {
    struct dm_dev *dev;
    sector_t start;

    // Attribute for file in dmp/stat
    struct kobj_attribute attr;

    unsigned long long reads;
    unsigned long long writes;
    unsigned long long read_blocks_size;
    unsigned long long write_blocks_size;
};

// dmp/stat directory
static struct kobject *stats_kobj;

static ssize_t show_dmp_stat(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    const struct dmp_data *data = container_of(attr, struct dmp_data, attr);
    const unsigned long long avg_read = data->reads == 0 ? 0 : (data->read_blocks_size / data->reads);
    const unsigned long long avg_write = data->writes == 0 ? 0 : (data->write_blocks_size / data->writes);
    const unsigned long long total_reqs = data->reads + data->writes;
    const unsigned long long avg_total =
        total_reqs == 0 ? 0 : (data->read_blocks_size + data->write_blocks_size) / total_reqs;
    // Result string is definitely less than PAGE_SIZE, so no need to check buffer overflow
    return sysfs_emit(buf,
        "read:\n"
        "  reqs: %llu\n"
        "  avg size: %llu\n"
        "write:\n"
        "  reqs: %llu\n"
        "  avg size: %llu\n"
        "total:\n"
        "  reqs: %llu\n"
        "  avg size: %llu\n",
        data->reads, avg_read,
        data->writes, avg_write,
        total_reqs, avg_total
        );
}

static int dmp_map(struct dm_target *ti, struct bio *bio) {
    struct dmp_data *my_data = ti->private;

    switch (bio_op(bio)) {
    case REQ_OP_READ:
        my_data->reads++;
        my_data->read_blocks_size += bio->bi_iter.bi_size;
        break;
    case REQ_OP_WRITE:
        my_data->writes++;
        my_data->write_blocks_size += bio->bi_iter.bi_size;
        break;
    default:
        break;
    }

    bio_set_dev(bio, my_data->dev->bdev);
    submit_bio(bio);

    return DM_MAPIO_SUBMITTED;
}

int init_dmp_attribute(struct dmp_data *data) {
    // Filled with zeroes so \0 will be
    char *attr_name = kzalloc(4 + sizeof(data->dev->name) + 1, GFP_KERNEL);
    if (attr_name == 0) {
        return -ENOMEM;
    }
    sprintf(attr_name, "dmp_%s", data->dev->name);
    data->attr.attr.name = attr_name;
    data->attr.attr.mode = S_IRUGO;
    data->attr.show = show_dmp_stat;
    data->attr.store = NULL;
    return 0;
}

static int dmp_target_ctr(struct dm_target *ti, unsigned int argc, char **argv) {
    printk(KERN_INFO "dmp: construct");

    if (argc != 1) {
        printk(KERN_CRIT "dmp: required one argument, got %d\n", argc);
        ti->error = "Invalid argument count";
        return -EINVAL;
    }

    struct dmp_data *data = kmalloc(sizeof(struct dmp_data), GFP_KERNEL);
    if (data == NULL) {
        printk(KERN_ERR "dmp: kmalloc failed\n");
        ti->error = "kmalloc returned NULL";
        return -ENOMEM;
    }
    data->start = ti->begin;
    data->reads = 0;
    data->writes = 0;
    data->read_blocks_size = 0;
    data->write_blocks_size = 0;

    int ret = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &data->dev);
    if (ret) {
        printk(KERN_ERR "dmp: lookup failed\n");
        ti->error = "Device lookup failed";
        goto bad;
    }
    ret = init_dmp_attribute(data);
    if (ret) {
        printk(KERN_ERR "dmp: attribute alloc failed\n");
        ti->error = "Attribute allocation failed";
        goto bad;
    }
    ret = sysfs_create_file(stats_kobj, &data->attr.attr);
    if (ret) {
        printk(KERN_ERR "dmp: attribute creation failed\n");
        ti->error = "Attribute creation failed";
        kfree(data->attr.attr.name); // attribute name is already allocated
        goto bad;
    }

    ti->private = data;
    return 0;

    bad:
    kfree(data);
    return ret;
}

static void dmp_target_dtr(struct dm_target *ti) {
    struct dmp_data *data = ti->private;

    printk(KERN_INFO "dmp: destruct\n");
    // First delete attribute
    sysfs_remove_file(stats_kobj, &data->attr.attr);
    kfree(data->attr.attr.name); // we allocated this field, now free it

    // Put device
    dm_put_device(ti, data->dev);

    // Now device is destructed and we need to free data
    kfree(data);
}

static struct target_type dmp_target = {
    .name = "dmp",
    .version = {0, 1, 0},
    .module = THIS_MODULE,
    .map = dmp_map,
    .ctr = dmp_target_ctr,
    .dtr = dmp_target_dtr
};


static int init_dmp(void) {
    const int result = dm_register_target(&dmp_target);
    if (result < 0) {
        printk(KERN_ERR "dmp module registration failed: can't register(%d)\n", result);
        return result;
    }
    stats_kobj = kobject_create_and_add("stat", &(THIS_MODULE)->mkobj.kobj);
    if (stats_kobj == 0) {
        dm_unregister_target(&dmp_target);
        printk(KERN_ERR "dmp module registration failed: no mem for kobj\n");
        return -ENOMEM;
    }
    printk(KERN_INFO "dmp module registration result: %d\n", result);
    return 0;
}

static void unload_dmp(void) {
    kobject_put(stats_kobj);
    dm_unregister_target(&dmp_target);
    printk(KERN_INFO "Unregistered dmp\n");
}

module_init(init_dmp);
module_exit(unload_dmp);

MODULE_AUTHOR("Vladimir Riabchun");
MODULE_LICENSE("GPL");

