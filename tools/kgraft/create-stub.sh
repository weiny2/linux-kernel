#!/bin/bash

if test -z "$1"; then
    echo "usage: $0 [list of symbols]"
fi

cat <<EOF
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kgraft.h>

EOF

for i in $@; do
    echo "extern void new_$i (void);"
    echo "KGR_PATCHED_FUNCTION($i, new_$i);"
done

echo "static struct kgr_patch patch = {"
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
        __module_get(THIS_MODULE);
        return kgr_start_patching(&patch);
}
module_init(kgr_patcher_init);

MODULE_LICENSE("GPL");
EOF
