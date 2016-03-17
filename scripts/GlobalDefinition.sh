KERNEL_VERSION=4.5.0-rc5+

# Am I invoked by Jenkins?
export ByJenkins=no
[ `id -un` == root ] && pwd | grep --quiet jenkins && ByJenkins=yes
