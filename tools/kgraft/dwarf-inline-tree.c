#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>

#include <libelf.h>

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>

#define HAVE_ELF64_GETEHDR

#define string char*
#include "dwarf_names.h"
#undef string

int ellipsis = 0;
int verbose = 0;
static char *file_name;
static char *program_name;
Dwarf_Error err;

void
print_error(Dwarf_Debug dbg, char *msg, int dwarf_code,
	    Dwarf_Error err)
{
    if (dwarf_code == DW_DLV_ERROR) {
	char *errmsg = dwarf_errmsg(err);
	long long myerr = dwarf_errno(err);

	fprintf(stderr, "%s ERROR:  %s:  %s (%lld)\n",
		program_name, msg, errmsg, myerr);
    } else if (dwarf_code == DW_DLV_NO_ENTRY) {
	fprintf(stderr, "%s NO ENTRY:  %s: \n", program_name, msg);
    } else if (dwarf_code == DW_DLV_OK) {
	fprintf(stderr, "%s:  %s \n", program_name, msg);
    } else {
	fprintf(stderr, "%s InternalError:  %s:  code %d\n",
		program_name, msg, dwarf_code);
    }
    exit(1);
}

static int indent_level;

static void
print_attribute(Dwarf_Debug dbg, Dwarf_Die die,
		Dwarf_Half attr,
		Dwarf_Attribute attr_in,
		char **srcfiles, Dwarf_Signed cnt, Dwarf_Half tag)
{
    Dwarf_Attribute attrib = 0;
    char *atname = 0;
    int tres = 0;
    Dwarf_Half form = 0;

    attrib = attr_in;
    atname = get_AT_name(dbg, attr);

    tres = dwarf_whatform (attrib, &form, &err);
    if (tres != DW_DLV_OK)
	print_error (dbg, "dwarf_whatform", tres, err);
    printf("\t\t%-28s%s\t", atname, get_FORM_name (dbg, form));
    /* Don't move over the attributes for the top-level compile_unit
     * DIEs.  */
    if (tag == DW_TAG_compile_unit)
      {
	printf ("\n");
	return;
      }
    switch (form) {
	case DW_FORM_addr:
	    {
		Dwarf_Addr a;
		tres = dwarf_formaddr (attrib, &a, &err);
		if (tres != DW_DLV_OK)
		    print_error (dbg, "dwarf_formaddr", tres, err);
		printf ("0x%llx", (unsigned long long)a);
	    }
	    break;
	case DW_FORM_data4:
	case DW_FORM_data8:
	case DW_FORM_data1:
	case DW_FORM_data2:
	case DW_FORM_udata:
	    {
		/* Bah.  From just looking at FORM_data[1248] we don't
		 * really know if it's signed or unsigned.  We have to
		 * look at the context.  Luckily only two ATs can be signed. */
		switch (attr) {
		    case DW_AT_upper_bound:
		    case DW_AT_lower_bound:
			  {
			    Dwarf_Signed s;
			    tres = dwarf_formsdata (attrib, &s, &err);
			    if (tres != DW_DLV_OK)
			      print_error (dbg, "dwarf_formudata", tres, err);
			    printf ("%lld", s);
			  }
			break;
		    default:
			  {
			    Dwarf_Unsigned u;
			    tres = dwarf_formudata (attrib, &u, &err);
			    if (tres != DW_DLV_OK)
			      print_error (dbg, "dwarf_formudata", tres, err);
			    printf ("%llu", u);
			  }
			break;
		}
	    }
	    break;
	case DW_FORM_sdata:
	    {
		Dwarf_Signed s;
		tres = dwarf_formsdata (attrib, &s, &err);
		if (tres != DW_DLV_OK)
		    print_error (dbg, "dwarf_formsdata", tres, err);
		printf ("%lld", s);
	    }
	    break;
	case DW_FORM_string:
	case DW_FORM_strp:
	    {
		char *s;
		tres = dwarf_formstring (attrib, &s, &err);
		if (tres != DW_DLV_OK)
		    print_error (dbg, "dwarf_formstring", tres, err);
		printf ("%s\n", s);
	    }
	    break;
	case DW_FORM_block:
	case DW_FORM_block1:
	case DW_FORM_block2:
	case DW_FORM_block4:
	    {
		Dwarf_Block *b;
		tres = dwarf_formblock (attrib, &b, &err);
		if (tres != DW_DLV_OK)
		    print_error (dbg, "dwarf_formblock", tres, err);
		printf ("[block data]");
	    }
	    break;
	case DW_FORM_flag:
	    {
		Dwarf_Bool b;
		tres = dwarf_formflag (attrib, &b, &err);
		if (tres != DW_DLV_OK)
		    print_error (dbg, "dwarf_formflag", tres, err);
		printf ("%s", b ? "true" : "false");
	    }
	    break;
	case DW_FORM_ref_addr:
	case DW_FORM_ref1:
	case DW_FORM_ref2:
	case DW_FORM_ref4:
	case DW_FORM_ref8:
	case DW_FORM_ref_udata:
	    {
		Dwarf_Off o;
		tres = dwarf_global_formref (attrib, &o, &err);
		if (tres != DW_DLV_OK)
		    print_error (dbg, "dwarf_global_formref", tres, err);
		printf ("ref <0x%x>\n", o);
	    }
	    break;
	case DW_FORM_indirect:
	default:
	    print_error (dbg, "broken DW_FORM", 0, 0);
	    break;
    }
    printf ("\n");
}

int
get_file_and_name (Dwarf_Debug dbg, Dwarf_Die die, int *file, char **name)
{
  Dwarf_Attribute attr;
  Dwarf_Half form = 0;
  int tres = 0;
  int ret = DW_DLV_OK;

  if (dwarf_attr (die, DW_AT_abstract_origin, &attr, &err) == DW_DLV_OK
      && dwarf_whatform (attr, &form, &err) == DW_DLV_OK)
    {
      Dwarf_Off o;
      Dwarf_Die ref;
      tres = dwarf_global_formref (attr, &o, &err);
      if (tres != DW_DLV_OK)
	print_error (dbg, "dwarf_global_formref", tres, err);
      else
	{
	  if (dwarf_offdie (dbg, o, &ref, &err) == DW_DLV_OK)
	    get_file_and_name (dbg, ref, file, name);
	}
    }

  if (dwarf_attr (die, DW_AT_decl_file, &attr, &err) == DW_DLV_OK
      && dwarf_whatform (attr, &form, &err) == DW_DLV_OK)
    {
      if (form == DW_FORM_sdata)
	{
	  Dwarf_Signed s;
	  if ((tres = dwarf_formsdata (attr, &s, &err)) == DW_DLV_OK)
	    *file = s;
	  else
	    ret = DW_DLV_ERROR, print_error (dbg, "dwarf_formsdata", tres, err);
	}
      else
	{
	  Dwarf_Unsigned u;
	  if ((tres = dwarf_formudata (attr, &u, &err)) == DW_DLV_OK)
	    *file = u;
	  else
	    ret = DW_DLV_ERROR, print_error (dbg, "dwarf_formudata", tres, err);
	}
    }

  if ((dwarf_attr (die, DW_AT_MIPS_linkage_name, &attr, &err) == DW_DLV_OK
       && dwarf_whatform (attr, &form, &err) == DW_DLV_OK)
      || (dwarf_attr (die, DW_AT_name, &attr, &err) == DW_DLV_OK
	  && dwarf_whatform (attr, &form, &err) == DW_DLV_OK))
    {
      char *s;
      tres = dwarf_formstring (attr, &s, &err);
      if (tres != DW_DLV_OK)
	ret = DW_DLV_ERROR, print_error (dbg, "dwarf_formstring", tres, err);
      *name = s;
    }
  return ret;
}

/* handle one die */
void
print_one_die(Dwarf_Debug dbg, Dwarf_Die die,
	      char **srcfiles, Dwarf_Signed cnt)
{
    Dwarf_Signed i;
    Dwarf_Off offset, overall_offset;
    char *tagname;
    Dwarf_Half tag;
    Dwarf_Signed atcnt;
    Dwarf_Attribute *atlist;
    int tres;
    int ores;
    int atres;

    tres = dwarf_tag(die, &tag, &err);
    if (tres != DW_DLV_OK) {
	print_error(dbg, "accessing tag of die!", tres, err);
    }
    tagname = get_TAG_name(dbg, tag);
    ores = dwarf_dieoffset(die, &overall_offset, &err);
    if (ores != DW_DLV_OK) {
	print_error(dbg, "dwarf_dieoffset", ores, err);
    }
    ores = dwarf_die_CU_offset(die, &offset, &err);
    if (ores != DW_DLV_OK) {
	print_error(dbg, "dwarf_die_CU_offset", ores, err);
    }

    if (verbose)
      {
	if (indent_level == 0) {
		printf
		    ("\nCOMPILE_UNIT<header overall offset = %llu>:\n",
		     overall_offset - offset);
	}
	printf("<%d><%5llu>\t%s\n", indent_level, offset, tagname);
      }

    if (tag == DW_TAG_subprogram || tag == DW_TAG_inlined_subroutine)
      {
	char *name = 0;
	int filenum = -1;
	char *prefix;
	Dwarf_Attribute attr;
	if (tag == DW_TAG_inlined_subroutine)
	  prefix = "I";
	else if (dwarf_attr (die, DW_AT_low_pc, &attr, &err) == DW_DLV_OK)
	  prefix = "D";
	else
	  prefix = "U";
	if (get_file_and_name (dbg, die, &filenum, &name) == DW_DLV_OK)
	  {
	    char *filename;
	    if (filenum > 0 && filenum <= cnt)
	      filename = srcfiles[filenum - 1];
	    else
	      filename = "";
	    printf ("%s %s:%s\n", prefix, filename, name);
	  }
	else
	  printf ("%s couldn't decode name or file\n", prefix);
      }

    if (!verbose)
      return;

    atres = dwarf_attrlist(die, &atlist, &atcnt, &err);
    if (atres == DW_DLV_ERROR) {
	print_error(dbg, "dwarf_attrlist", atres, err);
    } else if (atres == DW_DLV_NO_ENTRY) {
	/* indicates there are no attrs.  It is not an error. */
	atcnt = 0;
    }

    for (i = 0; i < atcnt; i++) {
	Dwarf_Half attr;
	int ares;

	ares = dwarf_whatattr(atlist[i], &attr, &err);
	if (ares == DW_DLV_OK) {
	    print_attribute(dbg, die, attr,
			    atlist[i], srcfiles, cnt, tag);
	} else {
	    print_error(dbg, "dwarf_whatattr entry missing", ares, err);
	}
    }

    for (i = 0; i < atcnt; i++) {
	dwarf_dealloc(dbg, atlist[i], DW_DLA_ATTR);
    }
    if (atres == DW_DLV_OK) {
	dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
    }
}

/* recursively follow the die tree */
void
print_die_and_children(Dwarf_Debug dbg, Dwarf_Die in_die_in,
		       char **srcfiles, Dwarf_Signed cnt)
{
    Dwarf_Die child;
    Dwarf_Die sibling;
    Dwarf_Error err;
    int tres;
    int cdres;
    Dwarf_Die in_die = in_die_in;

    for (;;) {
	/* here to pre-descent processing of the die */
	print_one_die(dbg, in_die, srcfiles, cnt);

	cdres = dwarf_child(in_die, &child, &err);
	/* child first: we are doing depth-first walk */
	if (cdres == DW_DLV_OK) {
	    indent_level++;
	    print_die_and_children(dbg, child, srcfiles, cnt);
	    indent_level--;
	    dwarf_dealloc(dbg, child, DW_DLA_DIE);
	} else if (cdres == DW_DLV_ERROR) {
	    print_error(dbg, "dwarf_child", cdres, err);
	}

	cdres = dwarf_siblingof(dbg, in_die, &sibling, &err);
	if (cdres == DW_DLV_OK) {
	    /* print_die_and_children(dbg, sibling, srcfiles, cnt); We 
	       loop around to actually print this, rather than
	       recursing. Recursing is horribly wasteful of stack
	       space. */
	} else if (cdres == DW_DLV_ERROR) {
	    print_error(dbg, "dwarf_siblingof", cdres, err);
	}

	/* Here do any post-descent (ie post-dwarf_child) processing
	   of the in_die. */

	if (in_die != in_die_in) {
	    /* Dealloc our in_die, but not the argument die, it belongs 
	       to our caller. Whether the siblingof call worked or not. 
	     */
	    dwarf_dealloc(dbg, in_die, DW_DLA_DIE);
	}
	if (cdres == DW_DLV_OK) {
	    /* Set to process the sibling, loop again. */
	    in_die = sibling;
	} else {
	    /* We are done, no more siblings at this level. */

	    break;
	}
    }				/* end for loop on siblings */
}

static void
print_infos(Dwarf_Debug dbg)
{
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Die cu_die = 0;
    Dwarf_Unsigned next_cu_offset = 0;
    int nres = DW_DLV_OK;

    /* Loop until it fails.  */
    while ((nres =
	    dwarf_next_cu_header(dbg, &cu_header_length, &version_stamp,
				 &abbrev_offset, &address_size,
				 &next_cu_offset, &err))
	   == DW_DLV_OK) {
	int sres;

	if (verbose)
	{
		printf("\nCU_HEADER:\n");
		printf("\t\t%-28s%llu\n", "cu_header_length",
		       cu_header_length);
		printf("\t\t%-28s%d\n", "version_stamp", version_stamp);
		printf("\t\t%-28s%llu\n", "abbrev_offset",
		       abbrev_offset);
		printf("\t\t%-28s%d", "address_size", address_size);
	}

	/* process a single compilation unit in .debug_info. */
	sres = dwarf_siblingof(dbg, NULL, &cu_die, &err);
	if (sres == DW_DLV_OK) {
	    {
		Dwarf_Signed cnt = 0;
		char **srcfiles = 0;
		int srcf = dwarf_srcfiles(cu_die,
					  &srcfiles, &cnt, &err);

		if (srcf != DW_DLV_OK) {
		    srcfiles = 0;
		    cnt = 0;
		}

		print_die_and_children(dbg, cu_die, srcfiles, cnt);
		if (srcf == DW_DLV_OK) {
		    int si;

		    for (si = 0; si < cnt; ++si) {
			dwarf_dealloc(dbg, srcfiles[si], DW_DLA_STRING);
		    }
		    dwarf_dealloc(dbg, srcfiles, DW_DLA_LIST);
		}
	    }
	    dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
	} else if (sres == DW_DLV_NO_ENTRY) {
	    /* do nothing I guess. */
	} else {
	    print_error(dbg, "Regetting cu_die", sres, err);
	}
    }
    if (nres == DW_DLV_ERROR) {
	char *errmsg = dwarf_errmsg(err);
	long long myerr = dwarf_errno(err);

	fprintf(stderr, "%s ERROR:  %s:  %s (%lld)\n",
		program_name, "attempting to print .debug_info",
		errmsg, myerr);
	fprintf(stderr, "attempting to continue.\n");
    }
}

static void
process_one_file (Elf *elf, char *file_name)
{
    Dwarf_Debug dbg;
    int dres;

    if (verbose)
      printf ("processing %s\n", file_name);
    dres = dwarf_elf_init(elf, DW_DLC_READ, NULL, NULL, &dbg, &err);
    if (dres == DW_DLV_NO_ENTRY) {
	printf("No DWARF information present in %s\n", file_name);
	return;
    }
    if (dres != DW_DLV_OK) {
	print_error(dbg, "dwarf_elf_init", dres, err);
    }

    print_infos(dbg);

    dres = dwarf_finish(dbg, &err);
    if (dres != DW_DLV_OK) {
	print_error(dbg, "dwarf_finish", dres, err);
    }
    return;
}
	
int
main(int argc, char *argv[])
{
    int f;
    Elf_Cmd cmd;
    Elf *arf, *elf;

    program_name = argv[0];
    
    (void) elf_version(EV_NONE);
    if (elf_version(EV_CURRENT) == EV_NONE) {
	(void) fprintf(stderr, "dwarf-inline-tree: libelf.a out of date.\n");
	exit(1);
    }

    if (argc < 2)
    {
	fprintf (stderr, "dwarf-inline-tree <input-elf>\n");
	exit (2);
    }
    file_name = argv[1];
    f = open(file_name, O_RDONLY);
    if (f == -1) {
	fprintf(stderr, "%s ERROR:  can't open %s\n", program_name,
		file_name);
	return 1;
    }

    cmd = ELF_C_READ;
    arf = elf_begin(f, cmd, (Elf *) 0);
    while ((elf = elf_begin(f, cmd, arf)) != 0) {
	Elf32_Ehdr *eh32;

#ifdef HAVE_ELF64_GETEHDR
	Elf64_Ehdr *eh64;
#endif /* HAVE_ELF64_GETEHDR */
	eh32 = elf32_getehdr(elf);
	if (!eh32) {
#ifdef HAVE_ELF64_GETEHDR
	    /* not a 32-bit obj */
	    eh64 = elf64_getehdr(elf);
	    if (!eh64) {
		/* not a 64-bit obj either! */
		/* dwarfdump is quiet when not an object */
	    } else {
		process_one_file(elf, file_name);
	    }
#endif /* HAVE_ELF64_GETEHDR */
	} else {
	    process_one_file(elf, file_name);
	}
	cmd = elf_next(elf);
	elf_end(elf);
    }
    elf_end(arf);
    return 0;
}
