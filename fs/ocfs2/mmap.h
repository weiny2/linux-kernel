#ifndef OCFS2_MMAP_H
#define OCFS2_MMAP_H

int ocfs2_mmap(struct file *file, struct vm_area_struct *vma,
	       unsigned long map_flags);

#endif  /* OCFS2_MMAP_H */
