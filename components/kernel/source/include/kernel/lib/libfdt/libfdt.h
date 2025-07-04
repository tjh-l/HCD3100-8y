#ifndef _LIBFDT_H
#define _LIBFDT_H
/*
 * libfdt - Flat Device Tree manipulation
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 *
 * libfdt is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 *
 *  a) This library is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation; either version 2 of the
 *     License, or (at your option) any later version.
 *
 *     This library is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public
 *     License along with this library; if not, write to the Free
 *     Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *     MA 02110-1301 USA
 *
 * Alternatively,
 *
 *  b) Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *     1. Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *     2. Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *     NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *     OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *     EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "libfdt_env.h"
#include "fdt.h"

#define FDT_FIRST_SUPPORTED_VERSION	0x10
#define FDT_LAST_SUPPORTED_VERSION	0x11

/* Error codes: informative error codes */
#define FDT_ERR_NOTFOUND	1
	/* FDT_ERR_NOTFOUND: The requested node or property does not exist */
#define FDT_ERR_EXISTS		2
	/* FDT_ERR_EXISTS: Attemped to create a node or property which
	 * already exists */
#define FDT_ERR_NOSPACE		3
	/* FDT_ERR_NOSPACE: Operation needed to expand the device
	 * tree, but its buffer did not have sufficient space to
	 * contain the expanded tree. Use fdt_open_into() to move the
	 * device tree to a buffer with more space. */

/* Error codes: codes for bad parameters */
#define FDT_ERR_BADOFFSET	4
	/* FDT_ERR_BADOFFSET: Function was passed a structure block
	 * offset which is out-of-bounds, or which points to an
	 * unsuitable part of the structure for the operation. */
#define FDT_ERR_BADPATH		5
	/* FDT_ERR_BADPATH: Function was passed a badly formatted path
	 * (e.g. missing a leading / for a function which requires an
	 * absolute path) */
#define FDT_ERR_BADPHANDLE	6
	/* FDT_ERR_BADPHANDLE: Function was passed an invalid phandle
	 * value.  phandle values of 0 and -1 are not permitted. */
#define FDT_ERR_BADSTATE	7
	/* FDT_ERR_BADSTATE: Function was passed an incomplete device
	 * tree created by the sequential-write functions, which is
	 * not sufficiently complete for the requested operation. */

/* Error codes: codes for bad device tree blobs */
#define FDT_ERR_TRUNCATED	8
	/* FDT_ERR_TRUNCATED: Structure block of the given device tree
	 * ends without an FDT_END tag. */
#define FDT_ERR_BADMAGIC	9
	/* FDT_ERR_BADMAGIC: Given "device tree" appears not to be a
	 * device tree at all - it is missing the flattened device
	 * tree magic number. */
#define FDT_ERR_BADVERSION	10
	/* FDT_ERR_BADVERSION: Given device tree has a version which
	 * can't be handled by the requested operation.  For
	 * read-write functions, this may mean that fdt_open_into() is
	 * required to convert the tree to the expected version. */
#define FDT_ERR_BADSTRUCTURE	11
	/* FDT_ERR_BADSTRUCTURE: Given device tree has a corrupt
	 * structure block or other serious error (e.g. misnested
	 * nodes, or subnodes preceding properties). */
#define FDT_ERR_BADLAYOUT	12
	/* FDT_ERR_BADLAYOUT: For read-write functions, the given
	 * device tree has it's sub-blocks in an order that the
	 * function can't handle (memory reserve map, then structure,
	 * then strings).  Use fdt_open_into() to reorganize the tree
	 * into a form suitable for the read-write operations. */

/* "Can't happen" error indicating a bug in libfdt */
#define FDT_ERR_INTERNAL	13
	/* FDT_ERR_INTERNAL: libfdt has failed an internal assertion.
	 * Should never be returned, if it is, it indicates a bug in
	 * libfdt itself. */

#define FDT_ERR_MAX		13

/**********************************************************************/
/* Low-level functions (you probably don't need these)                */
/**********************************************************************/

const void *fdt_offset_ptr(const void *fdt, int offset, unsigned int checklen);
static inline void *fdt_offset_ptr_w(void *fdt, int offset, int checklen)
{
	return (void *)(uintptr_t)fdt_offset_ptr(fdt, offset, checklen);
}

uint32_t fdt_next_tag(const void *fdt, int offset, int *nextoffset);

/**********************************************************************/
/* Traversal functions                                                */
/**********************************************************************/

int fdt_next_node(const void *fdt, int offset, int *depth);

/**********************************************************************/
/* General functions                                                  */
/**********************************************************************/

#define fdt_get_header(fdt, field) \
	(fdt32_to_cpu(((const struct fdt_header *)(fdt))->field))
#define fdt_magic(fdt) 			(fdt_get_header(fdt, magic))
#define fdt_totalsize(fdt)		(fdt_get_header(fdt, totalsize))
#define fdt_off_dt_struct(fdt)		(fdt_get_header(fdt, off_dt_struct))
#define fdt_off_dt_strings(fdt)		(fdt_get_header(fdt, off_dt_strings))
#define fdt_off_mem_rsvmap(fdt)		(fdt_get_header(fdt, off_mem_rsvmap))
#define fdt_version(fdt)		(fdt_get_header(fdt, version))
#define fdt_last_comp_version(fdt) 	(fdt_get_header(fdt, last_comp_version))
#define fdt_boot_cpuid_phys(fdt) 	(fdt_get_header(fdt, boot_cpuid_phys))
#define fdt_size_dt_strings(fdt) 	(fdt_get_header(fdt, size_dt_strings))
#define fdt_size_dt_struct(fdt)		(fdt_get_header(fdt, size_dt_struct))

#define __fdt_set_hdr(name) \
	static inline void fdt_set_##name(void *fdt, uint32_t val) \
	{ \
		struct fdt_header *fdth = (struct fdt_header*)fdt; \
		fdth->name = cpu_to_fdt32(val); \
	}
__fdt_set_hdr(magic);
__fdt_set_hdr(totalsize);
__fdt_set_hdr(off_dt_struct);
__fdt_set_hdr(off_dt_strings);
__fdt_set_hdr(off_mem_rsvmap);
__fdt_set_hdr(version);
__fdt_set_hdr(last_comp_version);
__fdt_set_hdr(boot_cpuid_phys);
__fdt_set_hdr(size_dt_strings);
__fdt_set_hdr(size_dt_struct);
#undef __fdt_set_hdr

/**
 * fdt_check_header - sanity check a device tree or possible device tree
 * @fdt: pointer to data which might be a flattened device tree
 *
 * fdt_check_header() checks that the given buffer contains what
 * appears to be a flattened device tree with sane information in its
 * header.
 *
 * returns:
 *     0, if the buffer appears to contain a valid device tree
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE, standard meanings, as above
 */
int fdt_check_header(const void *fdt);

/**********************************************************************/
/* Read-only functions                                                */
/**********************************************************************/

/**
 * fdt_string - retrieve a string from the strings block of a device tree
 * @fdt: pointer to the device tree blob
 * @stroffset: offset of the string within the strings block (native endian)
 *
 * fdt_string() retrieves a pointer to a single string from the
 * strings block of the device tree blob at fdt.
 *
 * returns:
 *     a pointer to the string, on success
 *     NULL, if stroffset is out of bounds
 */
const char *fdt_string(const void *fdt, int stroffset);

/**
 * fdt_num_mem_rsv - retrieve the number of memory reserve map entries
 * @fdt: pointer to the device tree blob
 *
 * Returns the number of entries in the device tree blob's memory
 * reservation map.  This does not include the terminating 0,0 entry
 * or any other (0,0) entries reserved for expansion.
 *
 * returns:
 *     the number of entries
 */
int fdt_num_mem_rsv(const void *fdt);

/**
 * fdt_get_mem_rsv - retrieve one memory reserve map entry
 * @fdt: pointer to the device tree blob
 * @address, @size: pointers to 64-bit variables
 *
 * On success, *address and *size will contain the address and size of
 * the n-th reserve map entry from the device tree blob, in
 * native-endian format.
 *
 * returns:
 *     0, on success
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE, standard meanings
 */
int fdt_get_mem_rsv(const void *fdt, int n, uint64_t *address, uint64_t *size);

/**
 * fdt_subnode_offset_namelen - find a subnode based on substring
 * @fdt: pointer to the device tree blob
 * @parentoffset: structure block offset of a node
 * @name: name of the subnode to locate
 * @namelen: number of characters of name to consider
 *
 * Identical to fdt_subnode_offset(), but only examine the first
 * namelen characters of name for matching the subnode name.  This is
 * useful for finding subnodes based on a portion of a larger string,
 * such as a full path.
 */
int fdt_subnode_offset_namelen(const void *fdt, int parentoffset,
			       const char *name, int namelen);
/**
 * fdt_subnode_offset - find a subnode of a given node
 * @fdt: pointer to the device tree blob
 * @parentoffset: structure block offset of a node
 * @name: name of the subnode to locate
 *
 * fdt_subnode_offset() finds a subnode of the node at structure block
 * offset parentoffset with the given name.  name may include a unit
 * address, in which case fdt_subnode_offset() will find the subnode
 * with that unit address, or the unit address may be omitted, in
 * which case fdt_subnode_offset() will find an arbitrary subnode
 * whose name excluding unit address matches the given name.
 *
 * returns:
 *	structure block offset of the requested subnode (>=0), on success
 *	-FDT_ERR_NOTFOUND, if the requested subnode does not exist
 *	-FDT_ERR_BADOFFSET, if parentoffset did not point to an FDT_BEGIN_NODE tag
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_subnode_offset(const void *fdt, int parentoffset, const char *name);

/**
 * fdt_path_offset - find a tree node by its full path
 * @fdt: pointer to the device tree blob
 * @path: full path of the node to locate
 *
 * fdt_path_offset() finds a node of a given path in the device tree.
 * Each path component may omit the unit address portion, but the
 * results of this are undefined if any such path component is
 * ambiguous (that is if there are multiple nodes at the relevant
 * level matching the given component, differentiated only by unit
 * address).
 *
 * returns:
 *	structure block offset of the node with the requested path (>=0), on success
 *	-FDT_ERR_BADPATH, given path does not begin with '/' or is invalid
 *	-FDT_ERR_NOTFOUND, if the requested node does not exist
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_path_offset(const void *fdt, const char *path);

/**
 * fdt_get_name - retrieve the name of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: structure block offset of the starting node
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_get_name() retrieves the name (including unit address) of the
 * device tree node at structure block offset nodeoffset.  If lenp is
 * non-NULL, the length of this name is also returned, in the integer
 * pointed to by lenp.
 *
 * returns:
 *	pointer to the node's name, on success
 *		If lenp is non-NULL, *lenp contains the length of that name (>=0)
 *	NULL, on error
 *		if lenp is non-NULL *lenp contains an error code (<0):
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE, standard meanings
 */
const char *fdt_get_name(const void *fdt, int nodeoffset, int *lenp);

/**
 * fdt_first_property_offset - find the offset of a node's first property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: structure block offset of a node
 *
 * fdt_first_property_offset() finds the first property of the node at
 * the given structure block offset.
 *
 * returns:
 *	structure block offset of the property (>=0), on success
 *	-FDT_ERR_NOTFOUND, if the requested node has no properties
 *	-FDT_ERR_BADOFFSET, if nodeoffset did not point to an FDT_BEGIN_NODE tag
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_first_property_offset(const void *fdt, int nodeoffset);

/**
 * fdt_next_property_offset - step through a node's properties
 * @fdt: pointer to the device tree blob
 * @offset: structure block offset of a property
 *
 * fdt_next_property_offset() finds the property immediately after the
 * one at the given structure block offset.  This will be a property
 * of the same node as the given property.
 *
 * returns:
 *	structure block offset of the next property (>=0), on success
 *	-FDT_ERR_NOTFOUND, if the given property is the last in its node
 *	-FDT_ERR_BADOFFSET, if nodeoffset did not point to an FDT_PROP tag
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_next_property_offset(const void *fdt, int offset);

/**
 * fdt_get_property_by_offset - retrieve the property at a given offset
 * @fdt: pointer to the device tree blob
 * @offset: offset of the property to retrieve
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_get_property_by_offset() retrieves a pointer to the
 * fdt_property structure within the device tree blob at the given
 * offset.  If lenp is non-NULL, the length of the property value is
 * also returned, in the integer pointed to by lenp.
 *
 * returns:
 *	pointer to the structure representing the property
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_PROP tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const struct fdt_property *fdt_get_property_by_offset(const void *fdt,
						      int offset,
						      int *lenp);

/**
 * fdt_get_property_namelen - find a property based on substring
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @namelen: number of characters of name to consider
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * Identical to fdt_get_property_namelen(), but only examine the first
 * namelen characters of name for matching the property name.
 */
const struct fdt_property *fdt_get_property_namelen(const void *fdt,
						    int nodeoffset,
						    const char *name,
						    int namelen, int *lenp);

/**
 * fdt_get_property - find a given property in a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_get_property() retrieves a pointer to the fdt_property
 * structure within the device tree blob corresponding to the property
 * named 'name' of the node at offset nodeoffset.  If lenp is
 * non-NULL, the length of the property value is also returned, in the
 * integer pointed to by lenp.
 *
 * returns:
 *	pointer to the structure representing the property
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_NOTFOUND, node does not have named property
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const struct fdt_property *fdt_get_property(const void *fdt, int nodeoffset,
					    const char *name, int *lenp);
static inline struct fdt_property *fdt_get_property_w(void *fdt, int nodeoffset,
						      const char *name,
						      int *lenp)
{
	return (struct fdt_property *)(uintptr_t)
		fdt_get_property(fdt, nodeoffset, name, lenp);
}

/**
 * fdt_getprop_by_offset - retrieve the value of a property at a given offset
 * @fdt: pointer to the device tree blob
 * @ffset: offset of the property to read
 * @namep: pointer to a string variable (will be overwritten) or NULL
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_getprop_by_offset() retrieves a pointer to the value of the
 * property at structure block offset 'offset' (this will be a pointer
 * to within the device blob itself, not a copy of the value).  If
 * lenp is non-NULL, the length of the property value is also
 * returned, in the integer pointed to by lenp.  If namep is non-NULL,
 * the property's namne will also be returned in the char * pointed to
 * by namep (this will be a pointer to within the device tree's string
 * block, not a new copy of the name).
 *
 * returns:
 *	pointer to the property's value
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *		if namep is non-NULL *namep contiains a pointer to the property
 *		name.
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_PROP tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const void *fdt_getprop_by_offset(const void *fdt, int offset,
				  const char **namep, int *lenp);

/**
 * fdt_getprop_namelen - get property value based on substring
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @namelen: number of characters of name to consider
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * Identical to fdt_getprop(), but only examine the first namelen
 * characters of name for matching the property name.
 */
const void *fdt_getprop_namelen(const void *fdt, int nodeoffset,
				const char *name, int namelen, int *lenp);

/**
 * fdt_getprop - retrieve the value of a given property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_getprop() retrieves a pointer to the value of the property
 * named 'name' of the node at offset nodeoffset (this will be a
 * pointer to within the device blob itself, not a copy of the value).
 * If lenp is non-NULL, the length of the property value is also
 * returned, in the integer pointed to by lenp.
 *
 * returns:
 *	pointer to the property's value
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_NOTFOUND, node does not have named property
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const void *fdt_getprop(const void *fdt, int nodeoffset,
			const char *name, int *lenp);
static inline void *fdt_getprop_w(void *fdt, int nodeoffset,
				  const char *name, int *lenp)
{
	return (void *)(uintptr_t)fdt_getprop(fdt, nodeoffset, name, lenp);
}

/**
 * fdt_get_phandle - retrieve the phandle of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: structure block offset of the node
 *
 * fdt_get_phandle() retrieves the phandle of the device tree node at
 * structure block offset nodeoffset.
 *
 * returns:
 *	the phandle of the node at nodeoffset, on success (!= 0, != -1)
 *	0, if the node has no phandle, or another error occurs
 */
uint32_t fdt_get_phandle(const void *fdt, int nodeoffset);

/**
 * fdt_get_alias_namelen - get alias based on substring
 * @fdt: pointer to the device tree blob
 * @name: name of the alias th look up
 * @namelen: number of characters of name to consider
 *
 * Identical to fdt_get_alias(), but only examine the first namelen
 * characters of name for matching the alias name.
 */
const char *fdt_get_alias_namelen(const void *fdt,
				  const char *name, int namelen);

/**
 * fdt_get_alias - retreive the path referenced by a given alias
 * @fdt: pointer to the device tree blob
 * @name: name of the alias th look up
 *
 * fdt_get_alias() retrieves the value of a given alias.  That is, the
 * value of the property named 'name' in the node /aliases.
 *
 * returns:
 *	a pointer to the expansion of the alias named 'name', of it exists
 *	NULL, if the given alias or the /aliases node does not exist
 */
const char *fdt_get_alias(const void *fdt, const char *name);
int fdt_get_alias_id(const void *fdt, const char *name);

/**
 * fdt_get_path - determine the full path of a node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose path to find
 * @buf: character buffer to contain the returned path (will be overwritten)
 * @buflen: size of the character buffer at buf
 *
 * fdt_get_path() computes the full path of the node at offset
 * nodeoffset, and records that path in the buffer at buf.
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset.
 *
 * returns:
 *	0, on success
 *		buf contains the absolute path of the node at
 *		nodeoffset, as a NUL-terminated string.
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_NOSPACE, the path of the given node is longer than (bufsize-1)
 *		characters and will not fit in the given buffer.
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_get_path(const void *fdt, int nodeoffset, char *buf, int buflen);

/**
 * fdt_supernode_atdepth_offset - find a specific ancestor of a node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose parent to find
 * @supernodedepth: depth of the ancestor to find
 * @nodedepth: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_supernode_atdepth_offset() finds an ancestor of the given node
 * at a specific depth from the root (where the root itself has depth
 * 0, its immediate subnodes depth 1 and so forth).  So
 *	fdt_supernode_atdepth_offset(fdt, nodeoffset, 0, NULL);
 * will always return 0, the offset of the root node.  If the node at
 * nodeoffset has depth D, then:
 *	fdt_supernode_atdepth_offset(fdt, nodeoffset, D, NULL);
 * will return nodeoffset itself.
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset.
 *
 * returns:

 *	structure block offset of the node at node offset's ancestor
 *		of depth supernodedepth (>=0), on success
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
*	-FDT_ERR_NOTFOUND, supernodedepth was greater than the depth of nodeoffset
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_supernode_atdepth_offset(const void *fdt, int nodeoffset,
				 int supernodedepth, int *nodedepth);

/**
 * fdt_node_depth - find the depth of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose parent to find
 *
 * fdt_node_depth() finds the depth of a given node.  The root node
 * has depth 0, its immediate subnodes depth 1 and so forth.
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset.
 *
 * returns:
 *	depth of the node at nodeoffset (>=0), on success
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_depth(const void *fdt, int nodeoffset);

/**
 * fdt_parent_offset - find the parent of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose parent to find
 *
 * fdt_parent_offset() locates the parent node of a given node (that
 * is, it finds the offset of the node which contains the node at
 * nodeoffset as a subnode).
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset, *twice*.
 *
 * returns:
 *	structure block offset of the parent of the node at nodeoffset
 *		(>=0), on success
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_parent_offset(const void *fdt, int nodeoffset);

/**
 * fdt_node_offset_by_prop_value - find nodes with a given property value
 * @fdt: pointer to the device tree blob
 * @startoffset: only find nodes after this offset
 * @propname: property name to check
 * @propval: property value to search for
 * @proplen: length of the value in propval
 *
 * fdt_node_offset_by_prop_value() returns the offset of the first
 * node after startoffset, which has a property named propname whose
 * value is of length proplen and has value equal to propval; or if
 * startoffset is -1, the very first such node in the tree.
 *
 * To iterate through all nodes matching the criterion, the following
 * idiom can be used:
 *	offset = fdt_node_offset_by_prop_value(fdt, -1, propname,
 *					       propval, proplen);
 *	while (offset != -FDT_ERR_NOTFOUND) {
 *		// other code here
 *		offset = fdt_node_offset_by_prop_value(fdt, offset, propname,
 *						       propval, proplen);
 *	}
 *
 * Note the -1 in the first call to the function, if 0 is used here
 * instead, the function will never locate the root node, even if it
 * matches the criterion.
 *
 * returns:
 *	structure block offset of the located node (>= 0, >startoffset),
 *		 on success
 *	-FDT_ERR_NOTFOUND, no node matching the criterion exists in the
 *		tree after startoffset
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_offset_by_prop_value(const void *fdt, int startoffset,
				  const char *propname,
				  const void *propval, int proplen);

/**
 * fdt_node_offset_by_phandle - find the node with a given phandle
 * @fdt: pointer to the device tree blob
 * @phandle: phandle value
 *
 * fdt_node_offset_by_phandle() returns the offset of the node
 * which has the given phandle value.  If there is more than one node
 * in the tree with the given phandle (an invalid tree), results are
 * undefined.
 *
 * returns:
 *	structure block offset of the located node (>= 0), on success
 *	-FDT_ERR_NOTFOUND, no node with that phandle exists
 *	-FDT_ERR_BADPHANDLE, given phandle value was invalid (0 or -1)
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_offset_by_phandle(const void *fdt, uint32_t phandle);

/**
 * fdt_node_check_compatible: check a node's compatible property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of a tree node
 * @compatible: string to match against
 *
 *
 * fdt_node_check_compatible() returns 0 if the given node contains a
 * 'compatible' property with the given string as one of its elements,
 * it returns non-zero otherwise, or on error.
 *
 * returns:
 *	0, if the node has a 'compatible' property listing the given string
 *	1, if the node has a 'compatible' property, but it does not list
 *		the given string
 *	-FDT_ERR_NOTFOUND, if the given node has no 'compatible' property
 * 	-FDT_ERR_BADOFFSET, if nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_check_compatible(const void *fdt, int nodeoffset,
			      const char *compatible);

/**
 * fdt_node_offset_by_compatible - find nodes with a given 'compatible' value
 * @fdt: pointer to the device tree blob
 * @startoffset: only find nodes after this offset
 * @compatible: 'compatible' string to match against
 *
 * fdt_node_offset_by_compatible() returns the offset of the first
 * node after startoffset, which has a 'compatible' property which
 * lists the given compatible string; or if startoffset is -1, the
 * very first such node in the tree.
 *
 * To iterate through all nodes matching the criterion, the following
 * idiom can be used:
 *	offset = fdt_node_offset_by_compatible(fdt, -1, compatible);
 *	while (offset != -FDT_ERR_NOTFOUND) {
 *		// other code here
 *		offset = fdt_node_offset_by_compatible(fdt, offset, compatible);
 *	}
 *
 * Note the -1 in the first call to the function, if 0 is used here
 * instead, the function will never locate the root node, even if it
 * matches the criterion.
 *
 * returns:
 *	structure block offset of the located node (>= 0, >startoffset),
 *		 on success
 *	-FDT_ERR_NOTFOUND, no node matching the criterion exists in the
 *		tree after startoffset
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_offset_by_compatible(const void *fdt, int startoffset,
				  const char *compatible);

/**
 * fdt_setprop - create or change a property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to change
 * @name: name of the property to change
 * @val: pointer to data to set the property value to
 * @len: length of the property value
 *
 * fdt_setprop() sets the value of the named property in the given
 * node to the given value and length, creating the property if it
 * does not already exist.
 *
 * This function may insert or delete data from the blob, and will
 * therefore change the offsets of some existing nodes.
 *
 * returns:
 *	0, on success
 *	-FDT_ERR_NOSPACE, there is insufficient free space in the blob to
 *		contain the new property value
 *	-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE tag
 *	-FDT_ERR_BADLAYOUT,
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_BADLAYOUT,
 *	-FDT_ERR_TRUNCATED, standard meanings
 */
int fdt_setprop(void *fdt, int nodeoffset, const char *name,
		const void *val, int len);
int fdt_setprop_at_offset(void *fdt, int nodeoffset, const char *name,
		const void *val, int len, int offset);

/**
 * fdt_setprop_u32 - set a property to a 32-bit integer
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to change
 * @name: name of the property to change
 * @val: 32-bit integer value for the property (native endian)
 *
 * fdt_setprop_u32() sets the value of the named property in the given
 * node to the given 32-bit integer value (converting to big-endian if
 * necessary), or creates a new property with that value if it does
 * not already exist.
 *
 * This function may insert or delete data from the blob, and will
 * therefore change the offsets of some existing nodes.
 *
 * returns:
 *	0, on success
 *	-FDT_ERR_NOSPACE, there is insufficient free space in the blob to
 *		contain the new property value
 *	-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE tag
 *	-FDT_ERR_BADLAYOUT,
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_BADLAYOUT,
 *	-FDT_ERR_TRUNCATED, standard meanings
 */
static inline int fdt_setprop_u32(void *fdt, int nodeoffset, const char *name,
				  uint32_t val)
{
	fdt32_t tmp = cpu_to_fdt32(val);
	return fdt_setprop(fdt, nodeoffset, name, &tmp, sizeof(tmp));
}

static inline int fdt_setprop_u32_index(void *fdt, int nodeoffset, const char *name,
				  int index, uint32_t val)
{
	fdt32_t tmp = cpu_to_fdt32(val);
	return fdt_setprop_at_offset(fdt, nodeoffset, name, &tmp, sizeof(tmp), index * sizeof(tmp));
}

#ifdef __cplusplus
}
#endif

#endif /* _LIBFDT_H */
