/*
 *  ELF file handling for TCC
 * 
 *  Copyright (c) 2001-2004 Fabrice Bellard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tcc.h"

static int new_undef_sym = 0; /* Is there a new undefined sym since last new_undef_sym() */

ST_FUNC int put_elf_str(Section *s, const char *sym)
{
    int offset, len;
    char *ptr;

    len = strlen(sym) + 1;
    offset = s->data_offset;
    ptr = section_ptr_add(s, len);
    memcpy(ptr, sym, len);
    return offset;
}

/* elf symbol hashing function */
static unsigned long elf_hash(const unsigned char *name)
{
    unsigned long h = 0, g;
    
    while (*name) {
        h = (h << 4) + *name++;
        g = h & 0xf0000000;
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

/* rebuild hash table of section s */
/* NOTE: we do factorize the hash table code to go faster */
static void rebuild_hash(Section *s, unsigned int nb_buckets)
{
    ElfW(Sym) *sym;
    int *ptr, *hash, nb_syms, sym_index, h;
    char *strtab;

    strtab = s->link->data;
    nb_syms = s->data_offset / sizeof(ElfW(Sym));

    s->hash->data_offset = 0;
    ptr = section_ptr_add(s->hash, (2 + nb_buckets + nb_syms) * sizeof(int));
    ptr[0] = nb_buckets;
    ptr[1] = nb_syms;
    ptr += 2;
    hash = ptr;
    memset(hash, 0, (nb_buckets + 1) * sizeof(int));
    ptr += nb_buckets + 1;

    sym = (ElfW(Sym) *)s->data + 1;
    for(sym_index = 1; sym_index < nb_syms; sym_index++) {
        if (ELFW(ST_BIND)(sym->st_info) != STB_LOCAL) {
            h = elf_hash(strtab + sym->st_name) % nb_buckets;
            *ptr = hash[h];
            hash[h] = sym_index;
        } else {
            *ptr = 0;
        }
        ptr++;
        sym++;
    }
}

/* return the symbol number */
ST_FUNC int put_elf_sym(Section *s, addr_t value, unsigned long size,
    int info, int other, int shndx, const char *name)
{
    int name_offset, sym_index;
    int nbuckets, h;
    ElfW(Sym) *sym;
    Section *hs;
    
    sym = section_ptr_add(s, sizeof(ElfW(Sym)));
    if (name)
        name_offset = put_elf_str(s->link, name);
    else
        name_offset = 0;
    /* XXX: endianness */
    sym->st_name = name_offset;
    sym->st_value = value;
    sym->st_size = size;
    sym->st_info = info;
    sym->st_other = other;
    sym->st_shndx = shndx;
    sym_index = sym - (ElfW(Sym) *)s->data;
    hs = s->hash;
    if (hs) {
        int *ptr, *base;
        ptr = section_ptr_add(hs, sizeof(int));
        base = (int *)hs->data;
        /* only add global or weak symbols */
        if (ELFW(ST_BIND)(info) != STB_LOCAL) {
            /* add another hashing entry */
            nbuckets = base[0];
            h = elf_hash(name) % nbuckets;
            *ptr = base[2 + h];
            base[2 + h] = sym_index;
            base[1]++;
            /* we resize the hash table */
            hs->nb_hashed_syms++;
            if (hs->nb_hashed_syms > 2 * nbuckets) {
                rebuild_hash(s, 2 * nbuckets);
            }
        } else {
            *ptr = 0;
            base[1]++;
        }
    }
    return sym_index;
}

/* find global ELF symbol 'name' and return its index. Return 0 if not
   found. */
ST_FUNC int find_elf_sym(Section *s, const char *name)
{
    ElfW(Sym) *sym;
    Section *hs;
    int nbuckets, sym_index, h;
    const char *name1;
    
    hs = s->hash;
    if (!hs)
        return 0;
    nbuckets = ((int *)hs->data)[0];
    h = elf_hash(name) % nbuckets;
    sym_index = ((int *)hs->data)[2 + h];
    while (sym_index != 0) {
        sym = &((ElfW(Sym) *)s->data)[sym_index];
        name1 = s->link->data + sym->st_name;
        if (!strcmp(name, name1))
            return sym_index;
        sym_index = ((int *)hs->data)[2 + nbuckets + sym_index];
    }
    return 0;
}

/* return elf symbol value, signal error if 'err' is nonzero */
ST_FUNC addr_t get_elf_sym_addr(TCCState *s, const char *name, int err)
{
    int sym_index;
    ElfW(Sym) *sym;

    sym_index = find_elf_sym(s->symtab, name);
    sym = &((ElfW(Sym) *)s->symtab->data)[sym_index];
    if (!sym_index || sym->st_shndx == SHN_UNDEF) {
        if (err)
            tcc_error("%s not defined", name);
        return 0;
    }
    return sym->st_value;
}

/* return elf symbol value */
LIBTCCAPI void *tcc_get_symbol(TCCState *s, const char *name)
{
    return (void*)(uintptr_t)get_elf_sym_addr(s, name, 0);
}

#ifdef TCC_IS_NATIVE
/* return elf symbol value or error */
ST_FUNC void* tcc_get_symbol_err(TCCState *s, const char *name)
{
    return (void*)get_elf_sym_addr(s, name, 1);
}
#endif

/* add an elf symbol : check if it is already defined and patch
   it. Return symbol index. NOTE that sh_num can be SHN_UNDEF. */
ST_FUNC int add_elf_sym(Section *s, addr_t value, unsigned long size,
                       int info, int other, int sh_num, const char *name)
{
    ElfW(Sym) *esym;
    int sym_bind, sym_index, sym_type, esym_bind;
    unsigned char sym_vis, esym_vis, new_vis;

    sym_bind = ELFW(ST_BIND)(info);
    sym_type = ELFW(ST_TYPE)(info);
    sym_vis = ELFW(ST_VISIBILITY)(other);
        
    if (sym_bind != STB_LOCAL) {
        /* we search global or weak symbols */
        sym_index = find_elf_sym(s, name);
        if (!sym_index)
            goto do_def;
        esym = &((ElfW(Sym) *)s->data)[sym_index];
        if (esym->st_shndx != SHN_UNDEF) {
            esym_bind = ELFW(ST_BIND)(esym->st_info);
            /* propagate the most constraining visibility */
            /* STV_DEFAULT(0)<STV_PROTECTED(3)<STV_HIDDEN(2)<STV_INTERNAL(1) */
            esym_vis = ELFW(ST_VISIBILITY)(esym->st_other);
            if (esym_vis == STV_DEFAULT) {
                new_vis = sym_vis;
            } else if (sym_vis == STV_DEFAULT) {
                new_vis = esym_vis;
            } else {
                new_vis = (esym_vis < sym_vis) ? esym_vis : sym_vis;
            }
            esym->st_other = (esym->st_other & ~ELFW(ST_VISIBILITY)(-1))
                             | new_vis;
            other = esym->st_other; /* in case we have to patch esym */
            if (sh_num == SHN_UNDEF) {
                /* ignore adding of undefined symbol if the
                   corresponding symbol is already defined */
            } else if (sym_bind == STB_GLOBAL && esym_bind == STB_WEAK) {
                /* global overrides weak, so patch */
                goto do_patch;
            } else if (sym_bind == STB_WEAK && esym_bind == STB_GLOBAL) {
                /* weak is ignored if already global */
            } else if (sym_bind == STB_WEAK && esym_bind == STB_WEAK) {
                /* keep first-found weak definition, ignore subsequents */
            } else if (sym_vis == STV_HIDDEN || sym_vis == STV_INTERNAL) {
                /* ignore hidden symbols after */
            } else if (esym->st_shndx == SHN_COMMON
                    && (sh_num < SHN_LORESERVE || sh_num == SHN_COMMON)) {
                /* gr: Happens with 'tcc ... -static tcctest.c' on e.g. Ubuntu 6.01
                   No idea if this is the correct solution ... */
                goto do_patch;
            } else if (s == tcc_state->dynsymtab_section) {
                /* we accept that two DLL define the same symbol */
            } else {
#if 0
                printf("new_bind=%x new_shndx=%x new_vis=%x old_bind=%x old_shndx=%x old_vis=%x\n",
                       sym_bind, sh_num, new_vis, esym_bind, esym->st_shndx, esym_vis);
#endif
                tcc_error_noabort("'%s' defined twice", name);
            }
        } else {
        do_patch:
            esym->st_info = ELFW(ST_INFO)(sym_bind, sym_type);
            esym->st_shndx = sh_num;
            new_undef_sym = 1;
            esym->st_value = value;
            esym->st_size = size;
            esym->st_other = other;
        }
    } else {
    do_def:
        sym_index = put_elf_sym(s, value, size, 
                                ELFW(ST_INFO)(sym_bind, sym_type), other, 
                                sh_num, name);
    }
    return sym_index;
}

/* put relocation */
ST_FUNC void put_elf_reloc(Section *symtab, Section *s, unsigned long offset,
                          int type, int symbol)
{
    char buf[256];
    Section *sr;
    ElfW_Rel *rel;

    sr = s->reloc;
    if (!sr) {
        /* if no relocation section, create it */
        snprintf(buf, sizeof(buf), REL_SECTION_FMT, s->name);
        /* if the symtab is allocated, then we consider the relocation
           are also */
        sr = new_section(tcc_state, buf, SHT_RELX, symtab->sh_flags);
        sr->sh_entsize = sizeof(ElfW_Rel);
        sr->link = symtab;
        sr->sh_info = s->sh_num;
        s->reloc = sr;
    }
    rel = section_ptr_add(sr, sizeof(ElfW_Rel));
    rel->r_offset = offset;
    rel->r_info = ELFW(R_INFO)(symbol, type);
#ifdef TCC_TARGET_X86_64
    rel->r_addend = 0;
#endif
}

/* put stab debug information */

ST_FUNC void put_stabs(const char *str, int type, int other, int desc,
                      unsigned long value)
{
    Stab_Sym *sym;

    sym = section_ptr_add(stab_section, sizeof(Stab_Sym));
    if (str) {
        sym->n_strx = put_elf_str(stabstr_section, str);
    } else {
        sym->n_strx = 0;
    }
    sym->n_type = type;
    sym->n_other = other;
    sym->n_desc = desc;
    sym->n_value = value;
}

ST_FUNC void put_stabs_r(const char *str, int type, int other, int desc, 
                        unsigned long value, Section *sec, int sym_index)
{
    put_stabs(str, type, other, desc, value);
    put_elf_reloc(symtab_section, stab_section, 
                  stab_section->data_offset - sizeof(unsigned int),
                  R_DATA_32, sym_index);
}

ST_FUNC void put_stabn(int type, int other, int desc, int value)
{
    put_stabs(NULL, type, other, desc, value);
}

ST_FUNC void put_stabd(int type, int other, int desc)
{
    put_stabs(NULL, type, other, desc, 0);
}

/* In an ELF file symbol table, the local symbols must appear below
   the global and weak ones. Since TCC cannot sort it while generating
   the code, we must do it after. All the relocation tables are also
   modified to take into account the symbol table sorting */
static void sort_syms(TCCState *s1, Section *s)
{
    int *old_to_new_syms;
    ElfW(Sym) *new_syms;
    int nb_syms, i;
    ElfW(Sym) *p, *q;
    ElfW_Rel *rel, *rel_end;
    Section *sr;
    int type, sym_index;

    nb_syms = s->data_offset / sizeof(ElfW(Sym));
    new_syms = tcc_malloc(nb_syms * sizeof(ElfW(Sym)));
    old_to_new_syms = tcc_malloc(nb_syms * sizeof(int));

    /* first pass for local symbols */
    p = (ElfW(Sym) *)s->data;
    q = new_syms;
    for(i = 0; i < nb_syms; i++) {
        if (ELFW(ST_BIND)(p->st_info) == STB_LOCAL) {
            old_to_new_syms[i] = q - new_syms;
            *q++ = *p;
        }
        p++;
    }
    /* save the number of local symbols in section header */
    s->sh_info = q - new_syms;

    /* then second pass for non local symbols */
    p = (ElfW(Sym) *)s->data;
    for(i = 0; i < nb_syms; i++) {
        if (ELFW(ST_BIND)(p->st_info) != STB_LOCAL) {
            old_to_new_syms[i] = q - new_syms;
            *q++ = *p;
        }
        p++;
    }
    
    /* we copy the new symbols to the old */
    memcpy(s->data, new_syms, nb_syms * sizeof(ElfW(Sym)));
    tcc_free(new_syms);

    /* now we modify all the relocations */
    for(i = 1; i < s1->nb_sections; i++) {
        sr = s1->sections[i];
        if (sr->sh_type == SHT_RELX && sr->link == s) {
            rel_end = (ElfW_Rel *)(sr->data + sr->data_offset);
            for(rel = (ElfW_Rel *)sr->data;
                rel < rel_end;
                rel++) {
                sym_index = ELFW(R_SYM)(rel->r_info);
                type = ELFW(R_TYPE)(rel->r_info);
                sym_index = old_to_new_syms[sym_index];
                rel->r_info = ELFW(R_INFO)(sym_index, type);
            }
        }
    }
    
    tcc_free(old_to_new_syms);
}

/* relocate common symbols in the .bss section */
ST_FUNC void relocate_common_syms(void)
{
    ElfW(Sym) *sym, *sym_end;
    unsigned long offset, align;
    
    sym_end = (ElfW(Sym) *)(symtab_section->data + symtab_section->data_offset);
    for(sym = (ElfW(Sym) *)symtab_section->data + 1; 
        sym < sym_end;
        sym++) {
        if (sym->st_shndx == SHN_COMMON) {
            /* align symbol */
            align = sym->st_value;
            offset = bss_section->data_offset;
            offset = (offset + align - 1) & -align;
            sym->st_value = offset;
            sym->st_shndx = bss_section->sh_num;
            offset += sym->st_size;
            bss_section->data_offset = offset;
        }
    }
}

/* relocate symbol table, resolve undefined symbols if do_resolve is
   true and output error if undefined symbol. */
ST_FUNC void relocate_syms(TCCState *s1, int do_resolve)
{
    ElfW(Sym) *sym, *esym, *sym_end;
    int sym_bind, sh_num, sym_index;
    const char *name;

    sym_end = (ElfW(Sym) *)(symtab_section->data + symtab_section->data_offset);
    for(sym = (ElfW(Sym) *)symtab_section->data + 1; 
        sym < sym_end;
        sym++) {
        sh_num = sym->st_shndx;
        if (sh_num == SHN_UNDEF) {
            name = strtab_section->data + sym->st_name;
            if (do_resolve) {
#if defined TCC_IS_NATIVE && !defined _WIN32
                void *addr;
                name = symtab_section->link->data + sym->st_name;
                addr = resolve_sym(s1, name);
                if (addr) {
                    sym->st_value = (addr_t)addr;
                    goto found;
                }
#endif
            } else if (s1->dynsym) {
                /* if dynamic symbol exist, then use it */
                sym_index = find_elf_sym(s1->dynsym, name);
                if (sym_index) {
                    esym = &((ElfW(Sym) *)s1->dynsym->data)[sym_index];
                    sym->st_value = esym->st_value;
                    goto found;
                }
            }
            /* XXX: _fp_hw seems to be part of the ABI, so we ignore
               it */
            if (!strcmp(name, "_fp_hw"))
                goto found;
            /* only weak symbols are accepted to be undefined. Their
               value is zero */
            sym_bind = ELFW(ST_BIND)(sym->st_info);
            if (sym_bind == STB_WEAK) {
                sym->st_value = 0;
            } else {
                tcc_error_noabort("undefined symbol '%s'", name);
            }
        } else if (sh_num < SHN_LORESERVE) {
            /* add section base */
            sym->st_value += s1->sections[sym->st_shndx]->sh_addr;
        }
    found: ;
    }
}

#ifdef TCC_HAS_RUNTIME_PLTGOT
#ifdef TCC_TARGET_X86_64
#define JMP_TABLE_ENTRY_SIZE 14
static addr_t add_jmp_table(TCCState *s1, addr_t val)
{
    char *p = s1->runtime_plt_and_got + s1->runtime_plt_and_got_offset;
    s1->runtime_plt_and_got_offset += JMP_TABLE_ENTRY_SIZE;
    /* jmp *0x0(%rip) */
    p[0] = 0xff;
    p[1] = 0x25;
    *(int *)(p + 2) = 0;
    *(addr_t *)(p + 6) = val;
    return (addr_t)p;
}

static addr_t add_got_table(TCCState *s1, addr_t val)
{
    addr_t *p = (addr_t *)(s1->runtime_plt_and_got + s1->runtime_plt_and_got_offset);
    s1->runtime_plt_and_got_offset += sizeof(addr_t);
    *p = val;
    return (addr_t)p;
}
#elif defined TCC_TARGET_ARM
#define JMP_TABLE_ENTRY_SIZE 8
static addr_t add_jmp_table(TCCState *s1, int val)
{
    uint32_t *p = (uint32_t *)(s1->runtime_plt_and_got + s1->runtime_plt_and_got_offset);
    s1->runtime_plt_and_got_offset += JMP_TABLE_ENTRY_SIZE;
    /* ldr pc, [pc, #-4] */
    p[0] = 0xE51FF004;
    p[1] = val;
    return (addr_t)p;
}
#endif
#endif /* def TCC_HAS_RUNTIME_PLTGOT */

/* relocate a given section (CPU dependent) */
ST_FUNC void relocate_section(TCCState *s1, Section *s)
{
    Section *sr;
    ElfW_Rel *rel, *rel_end, *qrel;
    ElfW(Sym) *sym;
    int type, sym_index;
    unsigned char *ptr;
    addr_t val, addr;
#if defined TCC_TARGET_I386 || defined TCC_TARGET_X86_64
    int esym_index;
#endif

    sr = s->reloc;
    rel_end = (ElfW_Rel *)(sr->data + sr->data_offset);
    qrel = (ElfW_Rel *)sr->data;
    for(rel = qrel;
        rel < rel_end;
        rel++) {
        ptr = s->data + rel->r_offset;

        sym_index = ELFW(R_SYM)(rel->r_info);
        sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
        val = sym->st_value;
#ifdef TCC_TARGET_X86_64
        val += rel->r_addend;
#endif
        type = ELFW(R_TYPE)(rel->r_info);
        addr = s->sh_addr + rel->r_offset;

        /* CPU specific */
        switch(type) {
#if defined(TCC_TARGET_I386)
        case R_386_32:
            if (s1->output_type == TCC_OUTPUT_DLL) {
                esym_index = s1->symtab_to_dynsym[sym_index];
                qrel->r_offset = rel->r_offset;
                if (esym_index) {
                    qrel->r_info = ELFW(R_INFO)(esym_index, R_386_32);
                    qrel++;
                    break;
                } else {
                    qrel->r_info = ELFW(R_INFO)(0, R_386_RELATIVE);
                    qrel++;
                }
            }
            *(int *)ptr += val;
            break;
        case R_386_PC32:
            if (s1->output_type == TCC_OUTPUT_DLL) {
                /* DLL relocation */
                esym_index = s1->symtab_to_dynsym[sym_index];
                if (esym_index) {
                    qrel->r_offset = rel->r_offset;
                    qrel->r_info = ELFW(R_INFO)(esym_index, R_386_PC32);
                    qrel++;
                    break;
                }
            }
            *(int *)ptr += val - addr;
            break;
        case R_386_PLT32:
            *(int *)ptr += val - addr;
            break;
        case R_386_GLOB_DAT:
        case R_386_JMP_SLOT:
            *(int *)ptr = val;
            break;
        case R_386_GOTPC:
            *(int *)ptr += s1->got->sh_addr - addr;
            break;
        case R_386_GOTOFF:
            *(int *)ptr += val - s1->got->sh_addr;
            break;
        case R_386_GOT32:
            /* we load the got offset */
            *(int *)ptr += s1->sym_attrs[sym_index].got_offset;
            break;
        case R_386_16:
            if (s1->output_format != TCC_OUTPUT_FORMAT_BINARY) {
            output_file:
		tcc_error("can only produce 16-bit binary files");
            }
            *(short *)ptr += val;
            break;
        case R_386_PC16:
            if (s1->output_format != TCC_OUTPUT_FORMAT_BINARY)
		goto output_file;
            *(short *)ptr += val - addr;
            break;
#elif defined(TCC_TARGET_ARM)
        case R_ARM_PC24:
        case R_ARM_CALL:
        case R_ARM_JUMP24:
        case R_ARM_PLT32:
            {
                int x, is_thumb, is_call, h, blx_avail;
                x = (*(int *)ptr)&0xffffff;
                (*(int *)ptr) &= 0xff000000;
                if (x & 0x800000)
                    x -= 0x1000000;
                x <<= 2;
                blx_avail = (TCC_ARM_VERSION >= 5);
                is_thumb = val & 1;
                is_call = (type == R_ARM_CALL);
                x += val - addr;
                h = x & 2;
#ifdef TCC_HAS_RUNTIME_PLTGOT
                if (s1->output_type == TCC_OUTPUT_MEMORY) {
                    if ((x & 3) || x >= 0x2000000 || x < -0x2000000)
                        if (!(x & 3) || !blx_avail || !is_call) {
                            x += add_jmp_table(s1, val) - val; /* add veneer */
                            is_thumb = 0; /* Veneer uses ARM instructions */
                        }
                }
#endif
                if ((x & 3) || x >= 0x2000000 || x < -0x2000000)
                    if (!(x & 3) || !blx_avail || !is_call)
                        tcc_error("can't relocate value at %x",addr);
                x >>= 2;
                x &= 0xffffff;
                /* Only reached if blx is avail and it is a call */
                if (is_thumb) {
                    x |= h << 24;
                    (*(int *)ptr) = 0xfa << 24; /* bl -> blx */
                }
                (*(int *)ptr) |= x;
            }
            break;
        /* Since these relocations only concern Thumb-2 and blx instruction was
           introduced before Thumb-2, we can assume blx is available and not
           guard its use */
        case R_ARM_THM_CALL:
        case R_ARM_THM_JUMP24:
	    {
                int x, hi, lo, s, j1, j2, i1, i2, imm10, imm11;
                int to_thumb, is_call, to_plt, blx_bit = 1 << 12;
                Section *plt;

                /* weak reference */
                if (sym->st_shndx == SHN_UNDEF &&
                    ELFW(ST_BIND)(sym->st_info) == STB_WEAK)
                    break;

                /* Get initial offset */
                hi = (*(uint16_t *)ptr);
                lo = (*(uint16_t *)(ptr+2));
                s = (hi >> 10) & 1;
                j1 = (lo >> 13) & 1;
                j2 = (lo >> 11) & 1;
                i1 = (j1 ^ s) ^ 1;
                i2 = (j2 ^ s) ^ 1;
                imm10 = hi & 0x3ff;
                imm11 = lo & 0x7ff;
                x = (s << 24) | (i1 << 23) | (i2 << 22) |
                    (imm10 << 12) | (imm11 << 1);
                if (x & 0x01000000)
                    x -= 0x02000000;

                /* Relocation infos */
                to_thumb = val & 1;
                plt = s1->plt;
                to_plt = (val >= plt->sh_addr) &&
                         (val < plt->sh_addr + plt->data_offset);
                is_call = (type == R_ARM_THM_CALL);

                /* Compute final offset */
                if (to_plt && !is_call) /* Point to 1st instr of Thumb stub */
                    x -= 4;
                x += val - addr;
                if (!to_thumb && is_call) {
                    blx_bit = 0; /* bl -> blx */
                    x = (x + 3) & -4; /* Compute offset from aligned PC */
                }

                /* Check that relocation is possible
                   * offset must not be out of range
                   * if target is to be entered in arm mode:
                     - bit 1 must not set
                     - instruction must be a call (bl) or a jump to PLT */
                if (!to_thumb || x >= 0x1000000 || x < -0x1000000)
                    if (to_thumb || (val & 2) || (!is_call && !to_plt))
                        tcc_error("can't relocate value at %x",addr);

                /* Compute and store final offset */
                s = (x >> 24) & 1;
                i1 = (x >> 23) & 1;
                i2 = (x >> 22) & 1;
                j1 = s ^ (i1 ^ 1);
                j2 = s ^ (i2 ^ 1);
                imm10 = (x >> 12) & 0x3ff;
                imm11 = (x >> 1) & 0x7ff;
                (*(uint16_t *)ptr) = (uint16_t) ((hi & 0xf800) |
                                     (s << 10) | imm10);
                (*(uint16_t *)(ptr+2)) = (uint16_t) ((lo & 0xc000) |
                                (j1 << 13) | blx_bit | (j2 << 11) |
                                imm11);
            }
            break;
        case R_ARM_MOVT_ABS:
        case R_ARM_MOVW_ABS_NC:
            {
                int x, imm4, imm12;
                if (type == R_ARM_MOVT_ABS)
                    val >>= 16;
                imm12 = val & 0xfff;
                imm4 = (val >> 12) & 0xf;
                x = (imm4 << 16) | imm12;
                if (type == R_ARM_THM_MOVT_ABS)
                    *(int *)ptr |= x;
                else
                    *(int *)ptr += x;
	    }
            break;
        case R_ARM_THM_MOVT_ABS:
        case R_ARM_THM_MOVW_ABS_NC:
            {
                int x, i, imm4, imm3, imm8;
                if (type == R_ARM_THM_MOVT_ABS)
                    val >>= 16;
                imm8 = val & 0xff;
                imm3 = (val >> 8) & 0x7;
                i = (val >> 11) & 1;
                imm4 = (val >> 12) & 0xf;
                x = (imm3 << 28) | (imm8 << 16) | (i << 10) | imm4;
                if (type == R_ARM_THM_MOVT_ABS)
                    *(int *)ptr |= x;
                else
                    *(int *)ptr += x;
	    }
            break;
        case R_ARM_PREL31:
            {
                int x;
                x = (*(int *)ptr) & 0x7fffffff;
                (*(int *)ptr) &= 0x80000000;
                x = (x * 2) / 2;
                x += val - addr;
                if((x^(x>>1))&0x40000000)
                    tcc_error("can't relocate value at %x",addr);
                (*(int *)ptr) |= x & 0x7fffffff;
            }
        case R_ARM_ABS32:
            *(int *)ptr += val;
            break;
        case R_ARM_REL32:
            *(int *)ptr += val - addr;
            break;
        case R_ARM_BASE_PREL:
            *(int *)ptr += s1->got->sh_addr - addr;
            break;
        case R_ARM_GOTOFF32:
            *(int *)ptr += val - s1->got->sh_addr;
            break;
        case R_ARM_GOT_BREL:
            /* we load the got offset */
            *(int *)ptr += s1->sym_attrs[sym_index].got_offset;
            break;
        case R_ARM_COPY:
            break;
        case R_ARM_V4BX:
            /* trade Thumb support for ARMv4 support */
            if ((0x0ffffff0 & *(int*)ptr) == 0x012FFF10)
                *(int*)ptr ^= 0xE12FFF10 ^ 0xE1A0F000; /* BX Rm -> MOV PC, Rm */
            break;
        default:
            fprintf(stderr,"FIXME: handle reloc type %x at %x [%p] to %x\n",
                type, (unsigned)addr, ptr, (unsigned)val);
            break;
#elif defined(TCC_TARGET_C67)
        case R_C60_32:
            *(int *)ptr += val;
            break;
        case R_C60LO16:
            {
                uint32_t orig;
                
                /* put the low 16 bits of the absolute address */
                // add to what is already there
                
                orig  =   ((*(int *)(ptr  )) >> 7) & 0xffff;
                orig |=  (((*(int *)(ptr+4)) >> 7) & 0xffff) << 16;
                
                //patch both at once - assumes always in pairs Low - High
                
                *(int *) ptr    = (*(int *) ptr    & (~(0xffff << 7)) ) |  (((val+orig)      & 0xffff) << 7);
                *(int *)(ptr+4) = (*(int *)(ptr+4) & (~(0xffff << 7)) ) | ((((val+orig)>>16) & 0xffff) << 7);
            }
            break;
        case R_C60HI16:
            break;
        default:
            fprintf(stderr,"FIXME: handle reloc type %x at %x [%p] to %x\n",
                type, (unsigned)addr, ptr, (unsigned)val);
            break;
#elif defined(TCC_TARGET_X86_64)
        case R_X86_64_64:
            if (s1->output_type == TCC_OUTPUT_DLL) {
                qrel->r_info = ELFW(R_INFO)(0, R_X86_64_RELATIVE);
                qrel->r_addend = *(long long *)ptr + val;
                qrel++;
            }
            *(long long *)ptr += val;
            break;
        case R_X86_64_32:
        case R_X86_64_32S:
            if (s1->output_type == TCC_OUTPUT_DLL) {
                /* XXX: this logic may depend on TCC's codegen
                   now TCC uses R_X86_64_32 even for a 64bit pointer */
                qrel->r_info = ELFW(R_INFO)(0, R_X86_64_RELATIVE);
                qrel->r_addend = *(int *)ptr + val;
                qrel++;
            }
            *(int *)ptr += val;
            break;

        case R_X86_64_PC32:
            if (s1->output_type == TCC_OUTPUT_DLL) {
                /* DLL relocation */
                esym_index = s1->symtab_to_dynsym[sym_index];
                if (esym_index) {
                    qrel->r_offset = rel->r_offset;
                    qrel->r_info = ELFW(R_INFO)(esym_index, R_X86_64_PC32);
                    qrel->r_addend = *(int *)ptr;
                    qrel++;
                    break;
                }
            }
            /* fall through */
        case R_X86_64_PLT32: {
            long long diff;
            diff = (long long)val - addr;
            if (diff <= -2147483647 || diff > 2147483647) {
#ifdef TCC_HAS_RUNTIME_PLTGOT
                /* XXX: naive support for over 32bit jump */
                if (s1->output_type == TCC_OUTPUT_MEMORY) {
                    val = (add_jmp_table(s1, val - rel->r_addend) +
                           rel->r_addend);
                    diff = val - addr;
                }
#endif
                if (diff <= -2147483647 || diff > 2147483647) {
                    tcc_error("internal error: relocation failed");
                }
            }
            *(int *)ptr += diff;
        }
            break;
        case R_X86_64_GLOB_DAT:
        case R_X86_64_JUMP_SLOT:
            /* They don't need addend */
            *(int *)ptr = val - rel->r_addend;
            break;
        case R_X86_64_GOTPCREL:
#ifdef TCC_HAS_RUNTIME_PLTGOT
            if (s1->output_type == TCC_OUTPUT_MEMORY) {
                val = add_got_table(s1, val - rel->r_addend) + rel->r_addend;
                *(int *)ptr += val - addr;
                break;
            }
#endif
            *(int *)ptr += (s1->got->sh_addr - addr +
                            s1->sym_attrs[sym_index].got_offset - 4);
            break;
        case R_X86_64_GOTTPOFF:
            *(int *)ptr += val - s1->got->sh_addr;
            break;
        case R_X86_64_GOT32:
            /* we load the got offset */
            *(int *)ptr += s1->sym_attrs[sym_index].got_offset;
            break;
#else
#error unsupported processor
#endif
        }
    }
    /* if the relocation is allocated, we change its symbol table */
    if (sr->sh_flags & SHF_ALLOC)
        sr->link = s1->dynsym;
}

/* hacked and arm only */
/* it's a stripped_down relocate_section, without side effects, that counts how much extra RAM it needs for relocations */
ST_FUNC int get_plt_got_size_for_relocate_section(TCCState *s1, Section *s)
{
    Section *sr;
    ElfW_Rel *rel, *rel_end, *qrel;
    ElfW(Sym) *sym;
    int type, sym_index;
    unsigned char *ptr;
    addr_t val, addr;
    
    int extra_ram = 0;

    sr = s->reloc;
    rel_end = (ElfW_Rel *)(sr->data + sr->data_offset);
    qrel = (ElfW_Rel *)sr->data;
    for(rel = qrel;
        rel < rel_end;
        rel++) {
        ptr = s->data + rel->r_offset;

        sym_index = ELFW(R_SYM)(rel->r_info);
        sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
        val = sym->st_value;
        type = ELFW(R_TYPE)(rel->r_info);
        addr = s->sh_addr + rel->r_offset;

        /* CPU specific */
        switch(type) {
#if defined(TCC_TARGET_ARM)
        case R_ARM_PC24:
        case R_ARM_CALL:
        case R_ARM_JUMP24:
        case R_ARM_PLT32:
            {
                int x, is_thumb, is_call, blx_avail;
                x = (*(int *)ptr)&0xffffff;
                if (x & 0x800000)
                    x -= 0x1000000;
                x <<= 2;
                blx_avail = (TCC_ARM_VERSION >= 5);
                is_thumb = val & 1;
                is_call = (type == R_ARM_CALL);
                x += val - addr;
#ifdef TCC_HAS_RUNTIME_PLTGOT
                if (s1->output_type == TCC_OUTPUT_MEMORY) {
                    if ((x & 3) || x >= 0x2000000 || x < -0x2000000)
                        if (!(x & 3) || !blx_avail || !is_call) {
                            extra_ram += JMP_TABLE_ENTRY_SIZE;
                        }
                }
#endif
            }
            break;
        default:
            break;
#else
#error unsupported processor
#endif
        }
    }
    return extra_ram;
}

/* relocate relocation table in 'sr' */
static void relocate_rel(TCCState *s1, Section *sr)
{
    Section *s;
    ElfW_Rel *rel, *rel_end;
    
    s = s1->sections[sr->sh_info];
    rel_end = (ElfW_Rel *)(sr->data + sr->data_offset);
    for(rel = (ElfW_Rel *)sr->data;
        rel < rel_end;
        rel++) {
        rel->r_offset += s->sh_addr;
    }
}

/* count the number of dynamic relocations so that we can reserve
   their space */
static int prepare_dynamic_rel(TCCState *s1, Section *sr)
{
    ElfW_Rel *rel, *rel_end;
    int sym_index, esym_index, type, count;

    count = 0;
    rel_end = (ElfW_Rel *)(sr->data + sr->data_offset);
    for(rel = (ElfW_Rel *)sr->data; rel < rel_end; rel++) {
        sym_index = ELFW(R_SYM)(rel->r_info);
        type = ELFW(R_TYPE)(rel->r_info);
        switch(type) {
#if defined(TCC_TARGET_I386)
        case R_386_32:
#elif defined(TCC_TARGET_X86_64)
        case R_X86_64_32:
        case R_X86_64_32S:
        case R_X86_64_64:
#endif
            count++;
            break;
#if defined(TCC_TARGET_I386)
        case R_386_PC32:
#elif defined(TCC_TARGET_X86_64)
        case R_X86_64_PC32:
#endif
            esym_index = s1->symtab_to_dynsym[sym_index];
            if (esym_index)
                count++;
            break;
        default:
            break;
        }
    }
    if (count) {
        /* allocate the section */
        sr->sh_flags |= SHF_ALLOC;
        sr->sh_size = count * sizeof(ElfW_Rel);
    }
    return count;
}

static struct sym_attr *alloc_sym_attr(TCCState *s1, int index)
{
    int n;
    struct sym_attr *tab;

    if (index >= s1->nb_sym_attrs) {
        /* find immediately bigger power of 2 and reallocate array */
        n = 1;
        while (index >= n)
            n *= 2;
        tab = tcc_realloc(s1->sym_attrs, n * sizeof(*s1->sym_attrs));
        s1->sym_attrs = tab;
        memset(s1->sym_attrs + s1->nb_sym_attrs, 0,
               (n - s1->nb_sym_attrs) * sizeof(*s1->sym_attrs));
        s1->nb_sym_attrs = n;
    }
    return &s1->sym_attrs[index];
}

/* XXX: suppress that */
static void put32(unsigned char *p, uint32_t val)
{
    p[0] = val;
    p[1] = val >> 8;
    p[2] = val >> 16;
    p[3] = val >> 24;
}

#if defined(TCC_TARGET_I386) || defined(TCC_TARGET_ARM) || \
    defined(TCC_TARGET_X86_64)
static uint32_t get32(unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}
#endif

static void build_got(TCCState *s1)
{
    unsigned char *ptr;

    /* if no got, then create it */
    s1->got = new_section(s1, ".got", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE);
    s1->got->sh_entsize = 4;
    add_elf_sym(symtab_section, 0, 4, ELFW(ST_INFO)(STB_GLOBAL, STT_OBJECT), 
                0, s1->got->sh_num, "_GLOBAL_OFFSET_TABLE_");
    ptr = section_ptr_add(s1->got, 3 * PTR_SIZE);
#if PTR_SIZE == 4
    /* keep space for _DYNAMIC pointer, if present */
    put32(ptr, 0);
    /* two dummy got entries */
    put32(ptr + 4, 0);
    put32(ptr + 8, 0);
#else
    /* keep space for _DYNAMIC pointer, if present */
    put32(ptr, 0);
    put32(ptr + 4, 0);
    /* two dummy got entries */
    put32(ptr + 8, 0);
    put32(ptr + 12, 0);
    put32(ptr + 16, 0);
    put32(ptr + 20, 0);
#endif
}

/* put a got entry corresponding to a symbol in symtab_section. 'size'
   and 'info' can be modifed if more precise info comes from the DLL */
static void put_got_entry(TCCState *s1,
                          int reloc_type, unsigned long size, int info, 
                          int sym_index)
{
    int index;
    const char *name;
    ElfW(Sym) *sym;
    unsigned long offset;
    int *ptr;

    if (!s1->got)
        build_got(s1);

    /* if a got entry already exists for that symbol, no need to add one */
    if (sym_index < s1->nb_sym_attrs &&
        s1->sym_attrs[sym_index].got_offset)
        return;

    alloc_sym_attr(s1, sym_index)->got_offset = s1->got->data_offset;

    if (s1->dynsym) {
        sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
        name = symtab_section->link->data + sym->st_name;
        offset = sym->st_value;
#if defined(TCC_TARGET_I386) || defined(TCC_TARGET_X86_64)
        if (reloc_type ==
#ifdef TCC_TARGET_X86_64
            R_X86_64_JUMP_SLOT
#else
            R_386_JMP_SLOT
#endif
            ) {
            Section *plt;
            uint8_t *p;
            int modrm;

#if defined(TCC_OUTPUT_DLL_WITH_PLT)
            modrm = 0x25;
#else
            /* if we build a DLL, we add a %ebx offset */
            if (s1->output_type == TCC_OUTPUT_DLL)
                modrm = 0xa3;
            else
                modrm = 0x25;
#endif

            /* add a PLT entry */
            plt = s1->plt;
            if (plt->data_offset == 0) {
                /* first plt entry */
                p = section_ptr_add(plt, 16);
                p[0] = 0xff; /* pushl got + PTR_SIZE */
                p[1] = modrm + 0x10;
                put32(p + 2, PTR_SIZE);
                p[6] = 0xff; /* jmp *(got + PTR_SIZE * 2) */
                p[7] = modrm;
                put32(p + 8, PTR_SIZE * 2);
            }

            p = section_ptr_add(plt, 16);
            p[0] = 0xff; /* jmp *(got + x) */
            p[1] = modrm;
            put32(p + 2, s1->got->data_offset);
            p[6] = 0x68; /* push $xxx */
            put32(p + 7, (plt->data_offset - 32) >> 1);
            p[11] = 0xe9; /* jmp plt_start */
            put32(p + 12, -(plt->data_offset));

            /* the symbol is modified so that it will be relocated to
               the PLT */
#if !defined(TCC_OUTPUT_DLL_WITH_PLT)
            if (s1->output_type == TCC_OUTPUT_EXE)
#endif
                offset = plt->data_offset - 16;
        }
#elif defined(TCC_TARGET_ARM)
        if (reloc_type == R_ARM_JUMP_SLOT) {
            Section *plt;
            uint8_t *p;
            
            /* if we build a DLL, we add a %ebx offset */
            if (s1->output_type == TCC_OUTPUT_DLL)
                tcc_error("DLLs unimplemented!");

            /* add a PLT entry */
            plt = s1->plt;
            if (plt->data_offset == 0) {
                /* first plt entry */
                p = section_ptr_add(plt, 16);
                put32(p     , 0xe52de004);
                put32(p +  4, 0xe59fe010);
                put32(p +  8, 0xe08fe00e);
                put32(p + 12, 0xe5bef008);
            }

            if (s1->sym_attrs[sym_index].plt_thumb_stub) {
                p = section_ptr_add(plt, 20);
                put32(p  , 0x4778); // bx pc
                put32(p+2, 0x46c0); // nop
                p += 4;
            } else
                p = section_ptr_add(plt, 16);
            put32(p  , 0xe59fc004); // ldr ip, [pc, #4] // offset in GOT
            put32(p+4, 0xe08fc00c); // add ip, pc, ip // absolute address or offset
            put32(p+8, 0xe59cf000); // ldr pc, [ip] // load absolute address or load offset
            put32(p+12, s1->got->data_offset);

            /* the symbol is modified so that it will be relocated to
               the PLT */
            if (s1->output_type == TCC_OUTPUT_EXE)
                offset = plt->data_offset - 16;
        }
#elif defined(TCC_TARGET_C67)
        tcc_error("C67 got not implemented");
#else
#error unsupported CPU
#endif
        index = put_elf_sym(s1->dynsym, offset, 
                            size, info, 0, sym->st_shndx, name);
        /* put a got entry */
        put_elf_reloc(s1->dynsym, s1->got, 
                      s1->got->data_offset, 
                      reloc_type, index);
    }
    ptr = section_ptr_add(s1->got, PTR_SIZE);
    *ptr = 0;
}

/* build GOT and PLT entries */
ST_FUNC void build_got_entries(TCCState *s1)
{
    Section *s;
    ElfW_Rel *rel, *rel_end;
    ElfW(Sym) *sym;
    int i, type, reloc_type, sym_index;

    for(i = 1; i < s1->nb_sections; i++) {
        s = s1->sections[i];
        if (s->sh_type != SHT_RELX)
            continue;
        /* no need to handle got relocations */
        if (s->link != symtab_section)
            continue;
        rel_end = (ElfW_Rel *)(s->data + s->data_offset);
        for(rel = (ElfW_Rel *)s->data;
            rel < rel_end;
            rel++) {
            type = ELFW(R_TYPE)(rel->r_info);
            switch(type) {
#if defined(TCC_TARGET_I386)
            case R_386_GOT32:
            case R_386_GOTOFF:
            case R_386_GOTPC:
            case R_386_PLT32:
                if (!s1->got)
                    build_got(s1);
                if (type == R_386_GOT32 || type == R_386_PLT32) {
                    sym_index = ELFW(R_SYM)(rel->r_info);
                    sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
                    /* look at the symbol got offset. If none, then add one */
                    if (type == R_386_GOT32)
                        reloc_type = R_386_GLOB_DAT;
                    else
                        reloc_type = R_386_JMP_SLOT;
                    put_got_entry(s1, reloc_type, sym->st_size, sym->st_info, 
                                  sym_index);
                }
                break;
#elif defined(TCC_TARGET_ARM)
            case R_ARM_GOT_BREL:
            case R_ARM_GOTOFF32:
            case R_ARM_BASE_PREL:
            case R_ARM_PLT32:
                if (!s1->got)
                    build_got(s1);
                if (type == R_ARM_GOT_BREL || type == R_ARM_PLT32) {
                    sym_index = ELFW(R_SYM)(rel->r_info);
                    sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
                    /* look at the symbol got offset. If none, then add one */
                    if (type == R_ARM_GOT_BREL)
                        reloc_type = R_ARM_GLOB_DAT;
                    else
                        reloc_type = R_ARM_JUMP_SLOT;
                    put_got_entry(s1, reloc_type, sym->st_size, sym->st_info, 
                                  sym_index);
                }
                break;
#elif defined(TCC_TARGET_C67)
            case R_C60_GOT32:
            case R_C60_GOTOFF:
            case R_C60_GOTPC:
            case R_C60_PLT32:
                if (!s1->got)
                    build_got(s1);
                if (type == R_C60_GOT32 || type == R_C60_PLT32) {
                    sym_index = ELFW(R_SYM)(rel->r_info);
                    sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
                    /* look at the symbol got offset. If none, then add one */
                    if (type == R_C60_GOT32)
                        reloc_type = R_C60_GLOB_DAT;
                    else
                        reloc_type = R_C60_JMP_SLOT;
                    put_got_entry(s1, reloc_type, sym->st_size, sym->st_info, 
                                  sym_index);
                }
                break;
#elif defined(TCC_TARGET_X86_64)
            case R_X86_64_GOT32:
            case R_X86_64_GOTTPOFF:
            case R_X86_64_GOTPCREL:
            case R_X86_64_PLT32:
                if (!s1->got)
                    build_got(s1);
                if (type == R_X86_64_GOT32 || type == R_X86_64_GOTPCREL ||
                    type == R_X86_64_PLT32) {
                    sym_index = ELFW(R_SYM)(rel->r_info);
                    sym = &((ElfW(Sym) *)symtab_section->data)[sym_index];
                    /* look at the symbol got offset. If none, then add one */
                    if (type == R_X86_64_GOT32 || type == R_X86_64_GOTPCREL)
                        reloc_type = R_X86_64_GLOB_DAT;
                    else
                        reloc_type = R_X86_64_JUMP_SLOT;
                    put_got_entry(s1, reloc_type, sym->st_size, sym->st_info, 
                                  sym_index);
                }
                break;
#else
#error unsupported CPU
#endif
            default:
                break;
            }
        }
    }
}

ST_FUNC Section *new_symtab(TCCState *s1,
                           const char *symtab_name, int sh_type, int sh_flags,
                           const char *strtab_name, 
                           const char *hash_name, int hash_sh_flags)
{
    Section *symtab, *strtab, *hash;
    int *ptr, nb_buckets;

    symtab = new_section(s1, symtab_name, sh_type, sh_flags);
    symtab->sh_entsize = sizeof(ElfW(Sym));
    strtab = new_section(s1, strtab_name, SHT_STRTAB, sh_flags);
    put_elf_str(strtab, "");
    symtab->link = strtab;
    put_elf_sym(symtab, 0, 0, 0, 0, 0, NULL);
    
    nb_buckets = 1;

    hash = new_section(s1, hash_name, SHT_HASH, hash_sh_flags);
    hash->sh_entsize = sizeof(int);
    symtab->hash = hash;
    hash->link = symtab;

    ptr = section_ptr_add(hash, (2 + nb_buckets + 1) * sizeof(int));
    ptr[0] = nb_buckets;
    ptr[1] = 1;
    memset(ptr + 2, 0, (nb_buckets + 1) * sizeof(int));
    return symtab;
}

/* put dynamic tag */
static void put_dt(Section *dynamic, int dt, addr_t val)
{
    ElfW(Dyn) *dyn;
    dyn = section_ptr_add(dynamic, sizeof(ElfW(Dyn)));
    dyn->d_tag = dt;
    dyn->d_un.d_val = val;
}

static void add_init_array_defines(TCCState *s1, const char *section_name)
{
    Section *s;
    long end_offset;
    char sym_start[1024];
    char sym_end[1024];
    
    snprintf(sym_start, sizeof(sym_start), "__%s_start", section_name + 1);
    snprintf(sym_end, sizeof(sym_end), "__%s_end", section_name + 1);

    s = find_section(s1, section_name);
    if (!s) {
        end_offset = 0;
        s = data_section;
    } else {
        end_offset = s->data_offset;
    }

    add_elf_sym(symtab_section, 
                0, 0,
                ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE), 0,
                s->sh_num, sym_start);
    add_elf_sym(symtab_section, 
                end_offset, 0,
                ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE), 0,
                s->sh_num, sym_end);
}

ST_FUNC void tcc_add_bcheck(TCCState *s1)
{
#ifdef CONFIG_TCC_BCHECK
    unsigned long *ptr;
    Section *init_section;
    unsigned char *pinit;
    int sym_index;

    if (0 == s1->do_bounds_check)
        return;

    /* XXX: add an object file to do that */
    ptr = section_ptr_add(bounds_section, sizeof(unsigned long));
    *ptr = 0;
    add_elf_sym(symtab_section, 0, 0,
                ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE), 0,
                bounds_section->sh_num, "__bounds_start");
#ifdef TCC_TARGET_I386
    if (s1->output_type != TCC_OUTPUT_MEMORY) {
        /* add 'call __bound_init()' in .init section */
        init_section = find_section(s1, ".init");
        pinit = section_ptr_add(init_section, 5);
        pinit[0] = 0xe8;
        put32(pinit + 1, -4);
        sym_index = find_elf_sym(symtab_section, "__bound_init");
        put_elf_reloc(symtab_section, init_section,
                      init_section->data_offset - 4, R_386_PC32, sym_index);
    }
#endif
#endif
}

static inline int tcc_add_support(TCCState *s1, const char *filename)
{
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s/%s", s1->tcc_lib_path, filename);
    return tcc_add_file(s1, buf);
}

/* add tcc runtime libraries */
ST_FUNC void tcc_add_runtime(TCCState *s1)
{
    tcc_add_bcheck(s1);

    /* add libc */
    if (!s1->nostdlib) {
        tcc_add_library(s1, "c");
#ifdef CONFIG_USE_LIBGCC
        tcc_add_file(s1, TCC_LIBGCC);
#elif !defined WITHOUT_LIBTCC
        tcc_add_support(s1, "libtcc1.a");
#endif
        /* add crt end if not memory output */
        if (s1->output_type != TCC_OUTPUT_MEMORY)
            tcc_add_crt(s1, "crtn.o");
    }
}

/* add various standard linker symbols (must be done after the
   sections are filled (for example after allocating common
   symbols)) */
ST_FUNC void tcc_add_linker_symbols(TCCState *s1)
{
    char buf[1024];
    int i;
    Section *s;

    add_elf_sym(symtab_section, 
                text_section->data_offset, 0,
                ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE), 0,
                text_section->sh_num, "_etext");
    add_elf_sym(symtab_section, 
                data_section->data_offset, 0,
                ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE), 0,
                data_section->sh_num, "_edata");
    add_elf_sym(symtab_section, 
                bss_section->data_offset, 0,
                ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE), 0,
                bss_section->sh_num, "_end");
    /* horrible new standard ldscript defines */
    add_init_array_defines(s1, ".preinit_array");
    add_init_array_defines(s1, ".init_array");
    add_init_array_defines(s1, ".fini_array");
    
    /* add start and stop symbols for sections whose name can be
       expressed in C */
    for(i = 1; i < s1->nb_sections; i++) {
        s = s1->sections[i];
        if (s->sh_type == SHT_PROGBITS &&
            (s->sh_flags & SHF_ALLOC)) {
            const char *p;
            int ch;

            /* check if section name can be expressed in C */
            p = s->name;
            for(;;) {
                ch = *p;
                if (!ch)
                    break;
                if (!isid(ch) && !isnum(ch))
                    goto next_sec;
                p++;
            }
            snprintf(buf, sizeof(buf), "__start_%s", s->name);
            add_elf_sym(symtab_section, 
                        0, 0,
                        ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE), 0,
                        s->sh_num, buf);
            snprintf(buf, sizeof(buf), "__stop_%s", s->name);
            add_elf_sym(symtab_section,
                        s->data_offset, 0,
                        ELFW(ST_INFO)(STB_GLOBAL, STT_NOTYPE), 0,
                        s->sh_num, buf);
        }
    next_sec: ;
    }
}

static void tcc_output_binary(TCCState *s1, FILE *f,
                              const int *section_order)
{
    Section *s;
    int i, offset, size;

    offset = 0;
    for(i=1;i<s1->nb_sections;i++) {
        s = s1->sections[section_order[i]];
        if (s->sh_type != SHT_NOBITS &&
            (s->sh_flags & SHF_ALLOC)) {
            while (offset < s->sh_offset) {
                fputc(0, f);
                offset++;
            }
            size = s->sh_size;
            fwrite(s->data, 1, size, f);
            offset += size;
        }
    }
}

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#define	HAVE_PHDR	1
#define	EXTRA_RELITEMS	14

/* move the relocation value from .dynsym to .got */
void patch_dynsym_undef(TCCState *s1, Section *s)
{
    uint32_t *gotd = (void *)s1->got->data;
    ElfW(Sym) *sym, *sym_end;

    gotd += 3;	// dummy entries in .got
    /* relocate symbols in .dynsym */
    sym_end = (ElfW(Sym) *)(s->data + s->data_offset);
    for (sym = (ElfW(Sym) *)s->data + 1; sym < sym_end; sym++) {
	if (sym->st_shndx == SHN_UNDEF) {
	    *gotd++ = sym->st_value + 6; // XXX 6 is magic ?
	    sym->st_value = 0;
	}
    }
}
#else
#define	HAVE_PHDR	0
#define	EXTRA_RELITEMS	9

/* zero plt offsets of weak symbols in .dynsym */
void patch_dynsym_undef(TCCState *s1, Section *s)
{
    ElfW(Sym) *sym, *sym_end;

    sym_end = (ElfW(Sym) *)(s->data + s->data_offset);
    for (sym = (ElfW(Sym) *)s->data + 1; sym < sym_end; sym++)
        if (sym->st_shndx == SHN_UNDEF && ELFW(ST_BIND)(sym->st_info) == STB_WEAK)
            sym->st_value = 0;
}
#endif

ST_FUNC void fill_got_entry(TCCState *s1, ElfW_Rel *rel)
{
	int sym_index = ELFW(R_SYM) (rel->r_info);
	ElfW(Sym) *sym = &((ElfW(Sym) *) symtab_section->data)[sym_index];
	unsigned long offset;

	if (sym_index >= s1->nb_sym_attrs)
		return;
	offset = s1->sym_attrs[sym_index].got_offset;
	section_reserve(s1->got, offset + PTR_SIZE);
#ifdef TCC_TARGET_X86_64
	/* only works for x86-64 */
	put32(s1->got->data + offset + 4, sym->st_value >> 32);
#endif
	put32(s1->got->data + offset, sym->st_value & 0xffffffff);
}

ST_FUNC void fill_got(TCCState *s1)
{
	Section *s;
	ElfW_Rel *rel, *rel_end;
	int i;

	for(i = 1; i < s1->nb_sections; i++) {
		s = s1->sections[i];
		if (s->sh_type != SHT_RELX)
			continue;
		/* no need to handle got relocations */
		if (s->link != symtab_section)
			continue;
		rel_end = (ElfW_Rel *) (s->data + s->data_offset);
		for(rel = (ElfW_Rel *) s->data; rel < rel_end; rel++) {
			switch (ELFW(R_TYPE) (rel->r_info)) {
			case R_X86_64_GOT32:
			case R_X86_64_GOTPCREL:
			case R_X86_64_PLT32:
				fill_got_entry(s1, rel);
				break;
			}
		}
	}
}


/* output an ELF file */
/* XXX: suppress unneeded sections */
static int elf_output_file(TCCState *s1, const char *filename)
{
    return -1;
}

LIBTCCAPI int tcc_output_file(TCCState *s, const char *filename)
{
    return -1;
}

static void *load_data(int fd, unsigned long file_offset, unsigned long size)
{
    void *data;

    data = tcc_malloc(size);
    lseek(fd, file_offset, SEEK_SET);
    read(fd, data, size);
    return data;
}

typedef struct SectionMergeInfo {
    Section *s;            /* corresponding existing section */
    unsigned long offset;  /* offset of the new section in the existing section */
    uint8_t new_section;       /* true if section 's' was added */
    uint8_t link_once;         /* true if link once section */
} SectionMergeInfo;

/* load an object file and merge it with current files */
/* XXX: handle correctly stab (debug) info */
ST_FUNC int tcc_load_object_file(TCCState *s1, 
                                int fd, unsigned long file_offset)
{ 
    ElfW(Ehdr) ehdr;
    ElfW(Shdr) *shdr, *sh;
    int size, i, j, offset, offseti, nb_syms, sym_index, ret;
    unsigned char *strsec, *strtab;
    int *old_to_new_syms;
    char *sh_name, *name;
    SectionMergeInfo *sm_table, *sm;
    ElfW(Sym) *sym, *symtab;
    ElfW_Rel *rel, *rel_end;
    Section *s;

    int stab_index;
    int stabstr_index;

    stab_index = stabstr_index = 0;

    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
        goto fail1;
    if (ehdr.e_ident[0] != ELFMAG0 ||
        ehdr.e_ident[1] != ELFMAG1 ||
        ehdr.e_ident[2] != ELFMAG2 ||
        ehdr.e_ident[3] != ELFMAG3)
        goto fail1;
    /* test if object file */
    if (ehdr.e_type != ET_REL)
        goto fail1;
    /* test CPU specific stuff */
    if (ehdr.e_ident[5] != ELFDATA2LSB ||
        ehdr.e_machine != EM_TCC_TARGET) {
    fail1:
        tcc_error_noabort("invalid object file");
        return -1;
    }
    /* read sections */
    shdr = load_data(fd, file_offset + ehdr.e_shoff, 
                     sizeof(ElfW(Shdr)) * ehdr.e_shnum);
    sm_table = tcc_mallocz(sizeof(SectionMergeInfo) * ehdr.e_shnum);
    
    /* load section names */
    sh = &shdr[ehdr.e_shstrndx];
    strsec = load_data(fd, file_offset + sh->sh_offset, sh->sh_size);

    /* load symtab and strtab */
    old_to_new_syms = NULL;
    symtab = NULL;
    strtab = NULL;
    nb_syms = 0;
    for(i = 1; i < ehdr.e_shnum; i++) {
        sh = &shdr[i];
        if (sh->sh_type == SHT_SYMTAB) {
            if (symtab) {
                tcc_error_noabort("object must contain only one symtab");
            fail:
                ret = -1;
                goto the_end;
            }
            nb_syms = sh->sh_size / sizeof(ElfW(Sym));
            symtab = load_data(fd, file_offset + sh->sh_offset, sh->sh_size);
            sm_table[i].s = symtab_section;

            /* now load strtab */
            sh = &shdr[sh->sh_link];
            strtab = load_data(fd, file_offset + sh->sh_offset, sh->sh_size);
        }
    }
        
    /* now examine each section and try to merge its content with the
       ones in memory */
    for(i = 1; i < ehdr.e_shnum; i++) {
        /* no need to examine section name strtab */
        if (i == ehdr.e_shstrndx)
            continue;
        sh = &shdr[i];
        sh_name = strsec + sh->sh_name;
        /* ignore sections types we do not handle */
        if (sh->sh_type != SHT_PROGBITS &&
            sh->sh_type != SHT_RELX && 
#ifdef TCC_ARM_EABI
            sh->sh_type != SHT_ARM_EXIDX &&
#endif
            sh->sh_type != SHT_NOBITS && 
            sh->sh_type != SHT_PREINIT_ARRAY &&
            sh->sh_type != SHT_INIT_ARRAY &&
            sh->sh_type != SHT_FINI_ARRAY &&
            strcmp(sh_name, ".stabstr")
            )
            continue;
        if (sh->sh_addralign < 1)
            sh->sh_addralign = 1;
        /* find corresponding section, if any */
        for(j = 1; j < s1->nb_sections;j++) {
            s = s1->sections[j];
            if (!strcmp(s->name, sh_name)) {
                if (!strncmp(sh_name, ".gnu.linkonce", 
                             sizeof(".gnu.linkonce") - 1)) {
                    /* if a 'linkonce' section is already present, we
                       do not add it again. It is a little tricky as
                       symbols can still be defined in
                       it. */
                    sm_table[i].link_once = 1;
                    goto next;
                } else {
                    goto found;
                }
            }
        }
        /* not found: create new section */
        s = new_section(s1, sh_name, sh->sh_type, sh->sh_flags);
        /* take as much info as possible from the section. sh_link and
           sh_info will be updated later */
        s->sh_addralign = sh->sh_addralign;
        s->sh_entsize = sh->sh_entsize;
        sm_table[i].new_section = 1;
    found:
        if (sh->sh_type != s->sh_type) {
            tcc_error_noabort("invalid section type");
            goto fail;
        }

        /* align start of section */
        offset = s->data_offset;

        if (0 == strcmp(sh_name, ".stab")) {
            stab_index = i;
            goto no_align;
        }
        if (0 == strcmp(sh_name, ".stabstr")) {
            stabstr_index = i;
            goto no_align;
        }

        size = sh->sh_addralign - 1;
        offset = (offset + size) & ~size;
        if (sh->sh_addralign > s->sh_addralign)
            s->sh_addralign = sh->sh_addralign;
        s->data_offset = offset;
    no_align:
        sm_table[i].offset = offset;
        sm_table[i].s = s;
        /* concatenate sections */
        size = sh->sh_size;
        if (sh->sh_type != SHT_NOBITS) {
            unsigned char *ptr;
            lseek(fd, file_offset + sh->sh_offset, SEEK_SET);
            ptr = section_ptr_add(s, size);
            read(fd, ptr, size);
        } else {
            s->data_offset += size;
        }
    next: ;
    }

    /* //gr relocate stab strings */
    if (stab_index && stabstr_index) {
        Stab_Sym *a, *b;
        unsigned o;
        s = sm_table[stab_index].s;
        a = (Stab_Sym *)(s->data + sm_table[stab_index].offset);
        b = (Stab_Sym *)(s->data + s->data_offset);
        o = sm_table[stabstr_index].offset;
        while (a < b) 
            a->n_strx += o, a++;
    }

    /* second short pass to update sh_link and sh_info fields of new
       sections */
    for(i = 1; i < ehdr.e_shnum; i++) {
        s = sm_table[i].s;
        if (!s || !sm_table[i].new_section)
            continue;
        sh = &shdr[i];
        if (sh->sh_link > 0)
            s->link = sm_table[sh->sh_link].s;
        if (sh->sh_type == SHT_RELX) {
            s->sh_info = sm_table[sh->sh_info].s->sh_num;
            /* update backward link */
            s1->sections[s->sh_info]->reloc = s;
        }
    }
    sm = sm_table;

    /* resolve symbols */
    old_to_new_syms = tcc_mallocz(nb_syms * sizeof(int));

    sym = symtab + 1;
    for(i = 1; i < nb_syms; i++, sym++) {
        if (sym->st_shndx != SHN_UNDEF &&
            sym->st_shndx < SHN_LORESERVE) {
            sm = &sm_table[sym->st_shndx];
            if (sm->link_once) {
                /* if a symbol is in a link once section, we use the
                   already defined symbol. It is very important to get
                   correct relocations */
                if (ELFW(ST_BIND)(sym->st_info) != STB_LOCAL) {
                    name = strtab + sym->st_name;
                    sym_index = find_elf_sym(symtab_section, name);
                    if (sym_index)
                        old_to_new_syms[i] = sym_index;
                }
                continue;
            }
            /* if no corresponding section added, no need to add symbol */
            if (!sm->s)
                continue;
            /* convert section number */
            sym->st_shndx = sm->s->sh_num;
            /* offset value */
            sym->st_value += sm->offset;
        }
        /* add symbol */
        name = strtab + sym->st_name;
        sym_index = add_elf_sym(symtab_section, sym->st_value, sym->st_size, 
                                sym->st_info, sym->st_other, 
                                sym->st_shndx, name);
        old_to_new_syms[i] = sym_index;
    }

    /* third pass to patch relocation entries */
    for(i = 1; i < ehdr.e_shnum; i++) {
        s = sm_table[i].s;
        if (!s)
            continue;
        sh = &shdr[i];
        offset = sm_table[i].offset;
        switch(s->sh_type) {
        case SHT_RELX:
            /* take relocation offset information */
            offseti = sm_table[sh->sh_info].offset;
            rel_end = (ElfW_Rel *)(s->data + s->data_offset);
            for(rel = (ElfW_Rel *)(s->data + offset);
                rel < rel_end;
                rel++) {
                int type;
                unsigned sym_index;
                /* convert symbol index */
                type = ELFW(R_TYPE)(rel->r_info);
                sym_index = ELFW(R_SYM)(rel->r_info);
                /* NOTE: only one symtab assumed */
                if (sym_index >= nb_syms)
                    goto invalid_reloc;
                sym_index = old_to_new_syms[sym_index];
                /* ignore link_once in rel section. */
                if (!sym_index && !sm->link_once
#ifdef TCC_TARGET_ARM
                    && type != R_ARM_V4BX
#endif
                   ) {
                invalid_reloc:
                    tcc_error_noabort("Invalid relocation entry [%2d] '%s' @ %.8x",
                        i, strsec + sh->sh_name, rel->r_offset);
                    goto fail;
                }
                rel->r_info = ELFW(R_INFO)(sym_index, type);
                /* offset the relocation offset */
                rel->r_offset += offseti;
#ifdef TCC_TARGET_ARM
                /* Jumps and branches from a Thumb code to a PLT entry need
                   special handling since PLT entries are ARM code.
                   Unconditional bl instructions referencing PLT entries are
                   handled by converting these instructions into blx
                   instructions. Other case of instructions referencing a PLT
                   entry require to add a Thumb stub before the PLT entry to
                   switch to ARM mode. We set bit 0 of the got offset of a
                   symbol to indicate such a case. */
                if (type == R_ARM_THM_JUMP24)
                    alloc_sym_attr(s1, sym_index)->plt_thumb_stub = 1;
#endif
            }
            break;
        default:
            break;
        }
    }
    
    ret = 0;
 the_end:
    tcc_free(symtab);
    tcc_free(strtab);
    tcc_free(old_to_new_syms);
    tcc_free(sm_table);
    tcc_free(strsec);
    tcc_free(shdr);
    return ret;
}

typedef struct ArchiveHeader {
    char ar_name[16];           /* name of this member */
    char ar_date[12];           /* file mtime */
    char ar_uid[6];             /* owner uid; printed as decimal */
    char ar_gid[6];             /* owner gid; printed as decimal */
    char ar_mode[8];            /* file mode, printed as octal   */
    char ar_size[10];           /* file size, printed as decimal */
    char ar_fmag[2];            /* should contain ARFMAG */
} ArchiveHeader;

static int get_be32(const uint8_t *b)
{
    return b[3] | (b[2] << 8) | (b[1] << 16) | (b[0] << 24);
}

/* load only the objects which resolve undefined symbols */
static int tcc_load_alacarte(TCCState *s1, int fd, int size)
{
    return -1;
}

/* load a '.a' file */
ST_FUNC int tcc_load_archive(TCCState *s1, int fd)
{
    return -1;
}

#ifndef TCC_TARGET_PE
/* load a DLL and all referenced DLLs. 'level = 0' means that the DLL
   is referenced by the user (so it should be added as DT_NEEDED in
   the generated ELF file) */
ST_FUNC int tcc_load_dll(TCCState *s1, int fd, const char *filename, int level)
{ 
    return -1;
}

/* interpret a subset of GNU ldscripts to handle the dummy libc.so
   files */
ST_FUNC int tcc_load_ldscript(TCCState *s1)
{
    return -1;
}

/* read a section from a .o file without having to create a TCC state and add the file there */
/* it just reads that file, allocates memory for that section and fill it */
/* buffer is malloc'ed from here (free it after you are done with it) */
PUB_FUNC void* tcc_load_offline_section(char* filename, char* section_name)
{ 
    int fd = open(filename, O_RDONLY | O_BINARY);
    ElfW(Ehdr) ehdr;
    ElfW(Shdr) *shdr, *sh;
    int i, j;
    unsigned char *strsec;
    char *sh_name;
    Section *s;
    void* ret = 0;

    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
        goto fail1;
    if (ehdr.e_ident[0] != ELFMAG0 ||
        ehdr.e_ident[1] != ELFMAG1 ||
        ehdr.e_ident[2] != ELFMAG2 ||
        ehdr.e_ident[3] != ELFMAG3)
        goto fail1;
    /* test if object file */
    if (ehdr.e_type != ET_REL)
        goto fail1;
    if (0) {
    fail1:
        tcc_error_noabort("invalid object file");
        close(fd);
        return 0;
    }
    /* read sections */
    shdr = load_data(fd, ehdr.e_shoff, 
                     sizeof(ElfW(Shdr)) * ehdr.e_shnum);
    
    /* load section names */
    sh = &shdr[ehdr.e_shstrndx];
    strsec = load_data(fd, sh->sh_offset, sh->sh_size);

    /* now examine each section */
    for(i = 1; i < ehdr.e_shnum; i++) {
        sh = &shdr[i];
        sh_name = strsec + sh->sh_name;
        if (!strcmp(sh_name, section_name))
        {
            /* found section, read it contents and return it (of course, after cleaning up) */
            lseek(fd, sh->sh_offset, SEEK_SET);
            int size = sh->sh_size;
            void* ptr = tcc_malloc(size);
            read(fd, ptr, size);
            ret = ptr;
            break;
        }
    }

 the_end:
    close(fd);
    tcc_free(strsec);
    tcc_free(shdr);
    return ret;
}

#endif /* ndef TCC_TARGET_PE */
