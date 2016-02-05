KERNEL_VERSION=4.3.0+

# Am I invoked by Jenkins?
export ByJenkins=no
[ `id -un` == root ] && pwd | grep --quiet jenkins && ByJenkins=yes
