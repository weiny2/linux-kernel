#!/bin/bash

if test -z "$1"; then
    echo "usage: $0 [list of symbols]"
fi

cat <<EOF
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kgraft.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/ptrace.h>

EOF

for i in $@; do
    echo "extern void new_$i (void);"
    echo "KGR_PATCHED_FUNCTION(patch, $i, new_$i);"
done

echo "static const struct kgr_patch patch = {"
echo "	.patches = {"
for i in $@; do
    echo "		KGR_PATCH($i),"
done
echo "		KGR_PATCH_END"
echo "	}"
echo "};"

cat <<EOF
static int __init kgr_patcher_init(void)
{
        /* removing not supported (yet?) */
        __module_get(THIS_MODULE);
        /* +4 to skip push rbb / mov rsp,rbp prologue */
        kgr_start_patching(&patch);
        return 0;
}

static void __exit kgr_patcher_cleanup(void)
{
        printk(KERN_ERR "removing now buggy!\n");
}

module_init(kgr_patcher_init);
module_exit(kgr_patcher_cleanup);

MODULE_LICENSE("GPL");
EOF
