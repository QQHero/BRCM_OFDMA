// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017 Free Electrons
 *
 * Authors:
 *    Boris Brezillon <boris.brezillon@free-electrons.com>
 *    Peter Pan <peterpandong@micron.com>
 */

#define pr_fmt(fmt)    "nand-bbt: " fmt

#include <linux/mtd/nand.h>
#include <linux/slab.h>

#if defined(CONFIG_TENDA_PRIVATE_BSP) && defined(CONFIG_MTD_NAND)
#include <linux/mtd/bbm.h>
#include <linux/vmalloc.h>
#define BBT_BLOCK_GOOD        0x00
#define BBT_BLOCK_WORN        0x01
#define BBT_BLOCK_RESERVED    0x02
#define BBT_BLOCK_FACTORY_BAD    0x03

#define BBT_ENTRY_MASK        0x03
#define BBT_ENTRY_SHIFT        2

/* Buswidth is 16 bit */
#define NAND_BUSWIDTH_16    0x00000002

static inline uint8_t bbt_get_entry(struct nand_device *nand, int block)
{
    uint8_t entry = nand->bbt.bbt[block >> BBT_ENTRY_SHIFT];
    entry >>= (block & BBT_ENTRY_MASK) * 2;
    return entry & BBT_ENTRY_MASK;
}

/*
 * Define some generic bad / good block scan pattern which are used
 * while scanning a device for factory marked good / bad blocks.
 */
static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

/* Generic flash bbt descriptors */
static uint8_t bbt_pattern[] = {'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = {'1', 't', 'b', 'B' };

static struct nand_bbt_descr bbt_main_descr = {
    .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
    .offs =    8,
    .len = 4,
    .veroffs = 12,
    .maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
    .pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_descr = {
    .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
    .offs =    8,
    .len = 4,
    .veroffs = 12,
    .maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
    .pattern = mirror_pattern
};

static struct nand_bbt_descr bbt_main_no_oob_descr = {
    .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP
        | NAND_BBT_NO_OOB,
    .len = 4,
    .veroffs = 4,
    .maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
    .pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_no_oob_descr = {
    .options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
        | NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP
        | NAND_BBT_NO_OOB,
    .len = 4,
    .veroffs = 4,
    .maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
    .pattern = mirror_pattern
};

#define BADBLOCK_SCAN_MASK (~NAND_BBT_NO_OOB)

/**
 * nand_create_badblock_pattern - [INTERN] Creates a BBT descriptor structure
 * @this: NAND chip to create descriptor for
 *
 * This function allocates and initializes a nand_bbt_descr for BBM detection
 * based on the properties of @this. The new descriptor is stored in
 * this->badblock_pattern. Thus, this->badblock_pattern should be NULL when
 * passed to this function.
 */
static int nand_create_badblock_pattern(struct nand_device *nand)
{
    struct nand_bbt_descr *bd;
    if (nand->bbt.badblock_pattern) {
        pr_warn("Bad block pattern already allocated; not replacing\n");
        return -EINVAL;
    }
    bd = kzalloc(sizeof(*bd), GFP_KERNEL);
    if (!bd)
        return -ENOMEM;
    bd->options = nand->bbt.bbt_options & BADBLOCK_SCAN_MASK;
    bd->offs = nand->bbt.badblockpos;
    bd->len = (nand->bbt.options & NAND_BUSWIDTH_16) ? 2 : 1;
    bd->pattern = scan_ff_pattern;
    bd->options |= NAND_BBT_DYNAMICSTRUCT;
    nand->bbt.badblock_pattern = bd;
    return 0;
}

static int check_pattern_no_oob(uint8_t *buf, struct nand_bbt_descr *td)
{
    if (memcmp(buf, td->pattern, td->len))
        return -1;
    return 0;
}

/**
 * check_pattern - [GENERIC] check if a pattern is in the buffer
 * @buf: the buffer to search
 * @len: the length of buffer to search
 * @paglen: the pagelength
 * @td: search pattern descriptor
 *
 * Check for a pattern at the given place. Used to search bad block tables and
 * good / bad block identifiers.
 */
static int check_pattern(uint8_t *buf, int len, int paglen, struct nand_bbt_descr *td)
{
    if (td->options & NAND_BBT_NO_OOB)
        return check_pattern_no_oob(buf, td);

    /* Compare the pattern */
    if (memcmp(buf + paglen + td->offs, td->pattern, td->len))
        return -1;

    return 0;
}

/**
 * check_short_pattern - [GENERIC] check if a pattern is in the buffer
 * @buf: the buffer to search
 * @td:    search pattern descriptor
 *
 * Check for a pattern at the given place. Used to search bad block tables and
 * good / bad block identifiers. Same as check_pattern, but no optional empty
 * check.
 */
static int check_short_pattern(uint8_t *buf, struct nand_bbt_descr *td)
{
    /* Compare the pattern */
    if (memcmp(buf + td->offs, td->pattern, td->len))
        return -1;
    return 0;
}

/* Scan a given block partially */
static int scan_block_fast(struct mtd_info *mtd, struct nand_bbt_descr *bd,
               loff_t offs, uint8_t *buf, int numpages)
{
    struct mtd_oob_ops ops;
    int j, ret;

    ops.ooblen = mtd->oobsize;
    ops.oobbuf = buf;
    ops.ooboffs = 0;
    ops.datbuf = NULL;

    if (unlikely(bd->options & NAND_BBT_ACCESS_BBM_RAW))
        ops.mode = MTD_OPS_RAW;
    else
        ops.mode = MTD_OPS_PLACE_OOB;

    for (j = 0; j < numpages; j++) {
        /*
         * Read the full oob until read_oob is fixed to handle single
         * byte reads for 16 bit buswidth.
         */
        ret = mtd_read_oob(mtd, offs, &ops);
        /* Ignore ECC errors when checking for BBM */
        if (ret && !mtd_is_bitflip_or_eccerr(ret)) {
            return ret;
        }

        if (check_short_pattern(buf, bd)) {
            return 1;
        }

        offs += mtd->writesize;
    }
    return 0;
}

static inline void bbt_mark_entry(struct nand_device *nand, int block,
        uint8_t mark)
{
    uint8_t msk = (mark & BBT_ENTRY_MASK) << ((block & BBT_ENTRY_MASK) * 2);
    nand->bbt.bbt[block >> BBT_ENTRY_SHIFT] |= msk;
}

/**
 * create_bbt - [GENERIC] Create a bad block table by scanning the device
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @bd: descriptor for the good/bad block search pattern
 * @chip: create the table for a specific chip, -1 read all chips; applies only
 *        if NAND_BBT_PERCHIP option is set
 *
 * Create a bad block table by scanning the device for the given good/bad block
 * identify pattern.
 */
static int create_bbt(struct nand_device *nand, uint8_t *buf,
    struct nand_bbt_descr *bd, int chip)
{
    struct mtd_info *mtd = nanddev_to_mtd(nand);
    int i, numblocks, numpages;
    int startblock;
    loff_t from;

    pr_info("Scanning device for bad blocks\n");

    if (bd->options & NAND_BBT_SCAN2NDPAGE)
        numpages = 2;
    else
        numpages = 1;

    if (chip == -1) {
        numblocks = mtd->size >> nand->bbt.bbt_erase_shift;
        startblock = 0;
        from = 0;
    } else {
        if (chip >= nand->bbt.numchips) {
            pr_warn("create_bbt(): chipnr (%d) > available chips (%d)\n",
                   chip + 1, nand->bbt.numchips);
            return -EINVAL;
        }
        numblocks = nand->bbt.chipsize >> nand->bbt.bbt_erase_shift;
        startblock = chip * numblocks;
        numblocks += startblock;
        from = (loff_t)startblock << nand->bbt.bbt_erase_shift;
    }

    if (nand->bbt.bbt_options & NAND_BBT_SCANLASTPAGE)
        from += mtd->erasesize - (mtd->writesize * numpages);

    for (i = startblock; i < numblocks; i++) {
        int ret;

        BUG_ON(bd->options & NAND_BBT_NO_OOB);

        ret = scan_block_fast(mtd, bd, from, buf, numpages);
        if (ret < 0)
            return ret;

        if (ret) {
            bbt_mark_entry(nand, i, BBT_BLOCK_FACTORY_BAD);
            pr_info("Bad eraseblock %d at 0x%012llx\n",
                i, (unsigned long long)from);
            mtd->ecc_stats.badblocks++;
        }
        from += (1 << nand->bbt.bbt_erase_shift);
    }
    return 0;
}

/**
 * nand_memory_bbt - [GENERIC] create a memory based bad block table
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function creates a memory based bbt by scanning the device for
 * manufacturer / software marked good / bad blocks.
 */
static inline int nand_memory_bbt(struct nand_device *nand, struct nand_bbt_descr *bd)
{
    return create_bbt(nand, nand->bbt.buffers->databuf, bd, -1);
}

/**
 * verify_bbt_descr - verify the bad block description
 * @mtd: MTD device structure
 * @bd: the table to verify
 *
 * This functions performs a few sanity checks on the bad block description
 * table.
 */
static void verify_bbt_descr(struct mtd_info *mtd, struct nand_bbt_descr *bd)
{
    struct nand_device *nand = mtd_to_nanddev(mtd);
    u32 pattern_len;
    u32 bits;
    u32 table_size;

    if (!bd)
        return;

    pattern_len = bd->len;
    bits = bd->options & NAND_BBT_NRBITS_MSK;

    BUG_ON((nand->bbt.bbt_options & NAND_BBT_NO_OOB) &&
            !(nand->bbt.bbt_options & NAND_BBT_USE_FLASH));
    BUG_ON(!bits);

    if (bd->options & NAND_BBT_VERSION)
        pattern_len++;

    if (bd->options & NAND_BBT_NO_OOB) {
        BUG_ON(!(nand->bbt.bbt_options & NAND_BBT_USE_FLASH));
        BUG_ON(!(nand->bbt.bbt_options & NAND_BBT_NO_OOB));
        BUG_ON(bd->offs);
        if (bd->options & NAND_BBT_VERSION)
            BUG_ON(bd->veroffs != bd->len);
        BUG_ON(bd->options & NAND_BBT_SAVECONTENT);
    }

    if (bd->options & NAND_BBT_PERCHIP)
        table_size = nand->bbt.chipsize >> nand->bbt.bbt_erase_shift;
    else
        table_size = mtd->size >> nand->bbt.bbt_erase_shift;
    table_size >>= 3;
    table_size *= bits;
    if (bd->options & NAND_BBT_NO_OOB)
        table_size += pattern_len;
    BUG_ON(table_size > (1 << nand->bbt.bbt_erase_shift));
}

/* BBT marker is in the first page, no OOB */
static int scan_read_data(struct mtd_info *mtd, uint8_t *buf, loff_t offs,
             struct nand_bbt_descr *td)
{
    size_t retlen;
    size_t len;

    len = td->len;
    if (td->options & NAND_BBT_VERSION)
        len++;

    return mtd_read(mtd, offs, len, &retlen, buf);
}

/**
 * scan_read_oob - [GENERIC] Scan data+OOB region to buffer
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @offs: offset at which to scan
 * @len: length of data region to read
 *
 * Scan read data from data+OOB. May traverse multiple pages, interleaving
 * page,OOB,page,OOB,... in buf. Completes transfer and returns the "strongest"
 * ECC condition (error or bitflip). May quit on the first (non-ECC) error.
 */
static int scan_read_oob(struct mtd_info *mtd, uint8_t *buf, loff_t offs,
             size_t len)
{
    struct mtd_oob_ops ops;
    int res, ret = 0;

    ops.mode = MTD_OPS_PLACE_OOB;
    ops.ooboffs = 0;
    ops.ooblen = mtd->oobsize;

    while (len > 0) {
        ops.datbuf = buf;
        ops.len = min(len, (size_t)mtd->writesize);
        ops.oobbuf = buf + ops.len;

        res = mtd_read_oob(mtd, offs, &ops);
        if (res) {
            if (!mtd_is_bitflip_or_eccerr(res))
                return res;
            else if (mtd_is_eccerr(res) || !ret)
                ret = res;
        }

        buf += mtd->oobsize + mtd->writesize;
        len -= mtd->writesize;
        offs += mtd->writesize;
    }
    return ret;
}

static int scan_read(struct mtd_info *mtd, uint8_t *buf, loff_t offs,
             size_t len, struct nand_bbt_descr *td)
{
    if (td->options & NAND_BBT_NO_OOB)
        return scan_read_data(mtd, buf, offs, td);
    else
        return scan_read_oob(mtd, buf, offs, len);
}

/* Scan write data with oob to flash */
static int scan_write_bbt(struct mtd_info *mtd, loff_t offs, size_t len,
              uint8_t *buf, uint8_t *oob)
{
    struct mtd_oob_ops ops;

    ops.mode = MTD_OPS_PLACE_OOB;
    ops.ooboffs = 0;
    ops.ooblen = mtd->oobsize;
    ops.datbuf = buf;
    ops.oobbuf = oob;
    ops.len = len;

    return mtd_write_oob(mtd, offs, &ops);
}

static u32 bbt_get_ver_offs(struct mtd_info *mtd, struct nand_bbt_descr *td)
{
    u32 ver_offs = td->veroffs;

    if (!(td->options & NAND_BBT_NO_OOB))
        ver_offs += mtd->writesize;
    return ver_offs;
}

/**
 * read_abs_bbts - [GENERIC] Read the bad block table(s) for all chips starting at a given page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md:    descriptor for the bad block table mirror
 *
 * Read the bad block table(s) for all chips starting at a given page. We
 * assume that the bbt bits are in consecutive order.
 */
static void read_abs_bbts(struct mtd_info *mtd, uint8_t *buf,
              struct nand_bbt_descr *td, struct nand_bbt_descr *md)
{
    struct nand_device *nand = mtd_to_nanddev(mtd);

    /* Read the primary version, if available */
    if (td->options & NAND_BBT_VERSION) {
        scan_read(mtd, buf, (loff_t)td->pages[0] << nand->bbt.page_shift,
                  mtd->writesize, td);
        td->version[0] = buf[bbt_get_ver_offs(mtd, td)];
        pr_info("Bad block table at page %d, version 0x%02X\n",
             td->pages[0], td->version[0]);
    }

    /* Read the mirror version, if available */
    if (md && (md->options & NAND_BBT_VERSION)) {
        scan_read(mtd, buf, (loff_t)md->pages[0] << nand->bbt.page_shift,
                  mtd->writesize, md);
        md->version[0] = buf[bbt_get_ver_offs(mtd, md)];
        pr_info("Bad block table at page %d, version 0x%02X\n",
             md->pages[0], md->version[0]);
    }
}

/**
 * add_marker_len - compute the length of the marker in data area
 * @td: BBT descriptor used for computation
 *
 * The length will be 0 if the marker is located in OOB area.
 */
static u32 add_marker_len(struct nand_bbt_descr *td)
{
    u32 len;

    if (!(td->options & NAND_BBT_NO_OOB))
        return 0;

    len = td->len;
    if (td->options & NAND_BBT_VERSION)
        len++;
    return len;
}

/**
 * read_bbt - [GENERIC] Read the bad block table starting from page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @page: the starting page
 * @num: the number of bbt descriptors to read
 * @td: the bbt describtion table
 * @offs: block number offset in the table
 *
 * Read the bad block table starting from page.
 */
static int read_bbt(struct mtd_info *mtd, uint8_t *buf, int page, int num,
        struct nand_bbt_descr *td, int offs)
{
    int res, ret = 0, i, j, act = 0;
    struct nand_device *nand = mtd_to_nanddev(mtd);
    size_t retlen, len, totlen;
    loff_t from;
    int bits = td->options & NAND_BBT_NRBITS_MSK;
    uint8_t msk = (uint8_t)((1 << bits) - 1);
    u32 marker_len;
    int reserved_block_code = td->reserved_block_code;

    totlen = (num * bits) >> 3;
    marker_len = add_marker_len(td);
    from = ((loff_t)page) << nand->bbt.page_shift;

    while (totlen) {
        len = min(totlen, (size_t)(1 << nand->bbt.bbt_erase_shift));
        if (marker_len) {
            /*
             * In case the BBT marker is not in the OOB area it
             * will be just in the first page.
             */
            len -= marker_len;
            from += marker_len;
            marker_len = 0;
        }
        res = mtd_read(mtd, from, len, &retlen, buf);
        if (res < 0) {
            if (mtd_is_eccerr(res)) {
                pr_info("nand_bbt: ECC error in BBT at 0x%012llx\n",
                    from & ~mtd->writesize);
                return res;
            } else if (mtd_is_bitflip(res)) {
                pr_info("nand_bbt: corrected error in BBT at 0x%012llx\n",
                    from & ~mtd->writesize);
                ret = res;
            } else {
                pr_info("nand_bbt: error reading BBT\n");
                return res;
            }
        }

        /* Analyse data */
        for (i = 0; i < len; i++) {
            uint8_t dat = buf[i];
            for (j = 0; j < 8; j += bits, act++) {
                uint8_t tmp = (dat >> j) & msk;
                if (tmp == msk)
                    continue;
                if (reserved_block_code && (tmp == reserved_block_code)) {
                    pr_info("nand_read_bbt: reserved block at 0x%012llx\n",
                         (loff_t)(offs + act) <<
                         nand->bbt.bbt_erase_shift);
                    bbt_mark_entry(nand, offs + act,
                            BBT_BLOCK_RESERVED);
                    mtd->ecc_stats.bbtblocks++;
                    continue;
                }
                /*
                 * Leave it for now, if it's matured we can
                 * move this message to pr_debug.
                 */
                pr_info("nand_read_bbt: bad block at 0x%012llx\n",
                     (loff_t)(offs + act) <<
                     nand->bbt.bbt_erase_shift);
                /* Factory marked bad or worn out? */
                if (tmp == 0)
                    bbt_mark_entry(nand, offs + act,
                            BBT_BLOCK_FACTORY_BAD);
                else
                    bbt_mark_entry(nand, offs + act,
                            BBT_BLOCK_WORN);
                mtd->ecc_stats.badblocks++;
            }
        }
        totlen -= len;
        from += len;
    }
    return ret;
}


/**
 * read_abs_bbt - [GENERIC] Read the bad block table starting at a given page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @chip: read the table for a specific chip, -1 read all chips; applies only if
 *        NAND_BBT_PERCHIP option is set
 *
 * Read the bad block table for all chips starting at a given page. We assume
 * that the bbt bits are in consecutive order.
 */
static int read_abs_bbt(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *td, int chip)
{
    struct nand_device *nand = mtd_to_nanddev(mtd);
    int res = 0, i;

    if (td->options & NAND_BBT_PERCHIP) {
        int offs = 0;
        for (i = 0; i < nand->bbt.numchips; i++) {
            if (chip == -1 || chip == i)
                res = read_bbt(mtd, buf, td->pages[i],
                    nand->bbt.chipsize >> nand->bbt.bbt_erase_shift,
                    td, offs);
            if (res)
                return res;
            offs += nand->bbt.chipsize >> nand->bbt.bbt_erase_shift;
        }
    } else {
        res = read_bbt(mtd, buf, td->pages[0],
                mtd->size >> nand->bbt.bbt_erase_shift, td, 0);
        if (res)
            return res;
    }
    return 0;
}

/**
 * search_bbt - [GENERIC] scan the device for a specific bad block table
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 *
 * Read the bad block table by searching for a given ident pattern. Search is
 * preformed either from the beginning up or from the end of the device
 * downwards. The search starts always at the start of a block. If the option
 * NAND_BBT_PERCHIP is given, each chip is searched for a bbt, which contains
 * the bad block information of this chip. This is necessary to provide support
 * for certain DOC devices.
 *
 * The bbt ident pattern resides in the oob area of the first page in a block.
 */
static int search_bbt(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *td)
{
    struct nand_device *nand = mtd_to_nanddev(mtd);
    int i, chips;
    int startblock, block, dir;
    int scanlen = mtd->writesize + mtd->oobsize;
    int bbtblocks;
    int blocktopage = nand->bbt.bbt_erase_shift - nand->bbt.page_shift;

    /* Search direction top -> down? */
    if (td->options & NAND_BBT_LASTBLOCK) {
        startblock = (mtd->size >> nand->bbt.bbt_erase_shift) - 1;
        dir = -1;
    } else {
        startblock = 0;
        dir = 1;
    }

    /* Do we have a bbt per chip? */
    if (td->options & NAND_BBT_PERCHIP) {
        chips = nand->bbt.numchips;
        bbtblocks = nand->bbt.chipsize >> nand->bbt.bbt_erase_shift;
        startblock &= bbtblocks - 1;
    } else {
        chips = 1;
        bbtblocks = mtd->size >> nand->bbt.bbt_erase_shift;
    }

    for (i = 0; i < chips; i++) {
        /* Reset version information */
        td->version[i] = 0;
        td->pages[i] = -1;
        /* Scan the maximum number of blocks */
        for (block = 0; block < td->maxblocks; block++) {

            int actblock = startblock + dir * block;
            loff_t offs = (loff_t)actblock << nand->bbt.bbt_erase_shift;

            /* Read first page */
            scan_read(mtd, buf, offs, mtd->writesize, td);
            if (!check_pattern(buf, scanlen, mtd->writesize, td)) {
                td->pages[i] = actblock << blocktopage;
                if (td->options & NAND_BBT_VERSION) {
                    offs = bbt_get_ver_offs(mtd, td);
                    td->version[i] = buf[offs];
                }
                break;
            }
        }
        startblock += nand->bbt.chipsize >> nand->bbt.bbt_erase_shift;
    }
    /* Check, if we found a bbt for each requested chip */
    for (i = 0; i < chips; i++) {
        if (td->pages[i] == -1)
            pr_warn("Bad block table not found for chip %d\n", i);
        else
            pr_info("Bad block table found at page %d, version 0x%02X\n",
                td->pages[i], td->version[i]);
    }
    return 0;
}

/**
 * search_read_bbts - [GENERIC] scan the device for bad block table(s)
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md: descriptor for the bad block table mirror
 *
 * Search and read the bad block table(s).
 */
static void search_read_bbts(struct mtd_info *mtd, uint8_t *buf,
                 struct nand_bbt_descr *td,
                 struct nand_bbt_descr *md)
{
    /* Search the primary table */
    search_bbt(mtd, buf, td);

    /* Search the mirror table */
    if (md)
        search_bbt(mtd, buf, md);
}

/**
 * write_bbt - [GENERIC] (Re)write the bad block table
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md: descriptor for the bad block table mirror
 * @chipsel: selector for a specific chip, -1 for all
 *
 * (Re)write the bad block table.
 */
static int write_bbt(struct mtd_info *mtd, uint8_t *buf,
             struct nand_bbt_descr *td, struct nand_bbt_descr *md,
             int chipsel)
{
    struct nand_device *nand = mtd_to_nanddev(mtd);
    struct erase_info einfo;
    int i, res, chip = 0;
    int bits, startblock, dir, page, offs, numblocks, sft, sftmsk;
    int nrchips, pageoffs, ooboffs;
    uint8_t msk[4];
    uint8_t rcode = td->reserved_block_code;
    size_t retlen, len = 0;
    loff_t to;
    struct mtd_oob_ops ops;

    ops.ooblen = mtd->oobsize;
    ops.ooboffs = 0;
    ops.datbuf = NULL;
    ops.mode = MTD_OPS_PLACE_OOB;

    if (!rcode)
        rcode = 0xff;
    /* Write bad block table per chip rather than per device? */
    if (td->options & NAND_BBT_PERCHIP) {
        numblocks = (int)(nand->bbt.chipsize >> nand->bbt.bbt_erase_shift);
        /* Full device write or specific chip? */
        if (chipsel == -1) {
            nrchips = nand->bbt.numchips;
        } else {
            nrchips = chipsel + 1;
            chip = chipsel;
        }
    } else {
        numblocks = (int)(mtd->size >> nand->bbt.bbt_erase_shift);
        nrchips = 1;
    }

    /* Loop through the chips */
    for (; chip < nrchips; chip++) {
        /*
         * There was already a version of the table, reuse the page
         * This applies for absolute placement too, as we have the
         * page nr. in td->pages.
         */
        if (td->pages[chip] != -1) {
            page = td->pages[chip];
            goto write;
        }

        /*
         * Automatic placement of the bad block table. Search direction
         * top -> down?
         */
        if (td->options & NAND_BBT_LASTBLOCK) {
            startblock = numblocks * (chip + 1) - 1;
            dir = -1;
        } else {
            startblock = chip * numblocks;
            dir = 1;
        }

        for (i = 0; i < td->maxblocks; i++) {
            int block = startblock + dir * i;
            /* Check, if the block is bad */
            switch (bbt_get_entry(nand, block)) {
            case BBT_BLOCK_WORN:
            case BBT_BLOCK_FACTORY_BAD:
                continue;
            }
            page = block <<
                (nand->bbt.bbt_erase_shift - nand->bbt.page_shift);
            /* Check, if the block is used by the mirror table */
            if (!md || md->pages[chip] != page)
                goto write;
        }
        pr_err("No space left to write bad block table\n");
        return -ENOSPC;
    write:

        /* Set up shift count and masks for the flash table */
        bits = td->options & NAND_BBT_NRBITS_MSK;
        msk[2] = ~rcode;
        switch (bits) {
        case 1: sft = 3; sftmsk = 0x07; msk[0] = 0x00; msk[1] = 0x01;
            msk[3] = 0x01;
            break;
        case 2: sft = 2; sftmsk = 0x06; msk[0] = 0x00; msk[1] = 0x01;
            msk[3] = 0x03;
            break;
        case 4: sft = 1; sftmsk = 0x04; msk[0] = 0x00; msk[1] = 0x0C;
            msk[3] = 0x0f;
            break;
        case 8: sft = 0; sftmsk = 0x00; msk[0] = 0x00; msk[1] = 0x0F;
            msk[3] = 0xff;
            break;
        default: return -EINVAL;
        }

        to = ((loff_t)page) << nand->bbt.page_shift;

        /* Must we save the block contents? */
        if (td->options & NAND_BBT_SAVECONTENT) {
            /* Make it block aligned */
            to &= ~(((loff_t)1 << nand->bbt.bbt_erase_shift) - 1);
            len = 1 << nand->bbt.bbt_erase_shift;
            res = mtd_read(mtd, to, len, &retlen, buf);
            if (res < 0) {
                if (retlen != len) {
                    pr_info("nand_bbt: error reading block for writing the bad block table\n");
                    return res;
                }
                pr_warn("nand_bbt: ECC error while reading block for writing bad block table\n");
            }
            /* Read oob data */
            ops.ooblen = (len >> nand->bbt.page_shift) * mtd->oobsize;
            ops.oobbuf = &buf[len];
            res = mtd_read_oob(mtd, to + mtd->writesize, &ops);
            if (res < 0 || ops.oobretlen != ops.ooblen)
                goto outerr;

            /* Calc the byte offset in the buffer */
            pageoffs = page - (int)(to >> nand->bbt.page_shift);
            offs = pageoffs << nand->bbt.page_shift;
            /* Preset the bbt area with 0xff */
            memset(&buf[offs], 0xff, (size_t)(numblocks >> sft));
            ooboffs = len + (pageoffs * mtd->oobsize);

        } else if (td->options & NAND_BBT_NO_OOB) {
            ooboffs = 0;
            offs = td->len;
            /* The version byte */
            if (td->options & NAND_BBT_VERSION)
                offs++;
            /* Calc length */
            len = (size_t)(numblocks >> sft);
            len += offs;
            /* Make it page aligned! */
            len = ALIGN(len, mtd->writesize);
            /* Preset the buffer with 0xff */
            memset(buf, 0xff, len);
            /* Pattern is located at the begin of first page */
            memcpy(buf, td->pattern, td->len);
        } else {
            /* Calc length */
            len = (size_t)(numblocks >> sft);
            /* Make it page aligned! */
            len = ALIGN(len, mtd->writesize);
            /* Preset the buffer with 0xff */
            memset(buf, 0xff, len +
                   (len >> nand->bbt.page_shift)* mtd->oobsize);
            offs = 0;
            ooboffs = len;
            /* Pattern is located in oob area of first page */
            memcpy(&buf[ooboffs + td->offs], td->pattern, td->len);
        }

        if (td->options & NAND_BBT_VERSION)
            buf[ooboffs + td->veroffs] = td->version[chip];

        /* Walk through the memory table */
        for (i = 0; i < numblocks; i++) {
            uint8_t dat;
            int sftcnt = (i << (3 - sft)) & sftmsk;
            dat = bbt_get_entry(nand, chip * numblocks + i);
            /* Do not store the reserved bbt blocks! */
            buf[offs + (i >> sft)] &= ~(msk[dat] << sftcnt);
        }

        memset(&einfo, 0, sizeof(einfo));
        einfo.mtd = mtd;
        einfo.addr = to;
        einfo.len = 1 << nand->bbt.bbt_erase_shift;
//        res = nand_erase_nand(mtd, &einfo, 1);
//        if (res < 0)
//            goto outerr;

        res = scan_write_bbt(mtd, to, len, buf,
                td->options & NAND_BBT_NO_OOB ? NULL :
                &buf[len]);
        if (res < 0)
            goto outerr;

        pr_info("Bad block table written to 0x%012llx, version 0x%02X\n",
             (unsigned long long)to, td->version[chip]);

        /* Mark it as used */
        td->pages[chip] = page;
    }
    return 0;

 outerr:
    pr_warn("nand_bbt: error while writing bad block table %d\n", res);
    return res;
}

/**
 * nand_update_bbt - update bad block table(s)
 * @mtd: MTD device structure
 * @offs: the offset of the newly marked block
 *
 * The function updates the bad block table(s).
 */
static int nand_update_bbt(struct mtd_info *mtd, loff_t offs)
{
    struct nand_device *nand = mtd_to_nanddev(mtd);
    int len, res = 0;
    int chip, chipsel;
    uint8_t *buf;
    struct nand_bbt_descr *td = nand->bbt.bbt_td;
    struct nand_bbt_descr *md = nand->bbt.bbt_md;

    if (!nand->bbt.bbt || !td)
        return -EINVAL;

    /* Allocate a temporary buffer for one eraseblock incl. oob */
    len = (1 << nand->bbt.bbt_erase_shift);
    len += (len >> nand->bbt.page_shift) * mtd->oobsize;
    buf = kmalloc(len, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    /* Do we have a bbt per chip? */
    if (td->options & NAND_BBT_PERCHIP) {
        chip = (int)(offs >> nand->bbt.chip_shift);
        chipsel = chip;
    } else {
        chip = 0;
        chipsel = -1;
    }

    td->version[chip]++;
    if (md)
        md->version[chip]++;

    /* Write the bad block table to the device? */
    if (td->options & NAND_BBT_WRITE) {
        res = write_bbt(mtd, buf, td, md, chipsel);
        if (res < 0)
            goto out;
    }
    /* Write the mirror bad block table to the device? */
    if (md && (md->options & NAND_BBT_WRITE)) {
        res = write_bbt(mtd, buf, md, td, chipsel);
    }

 out:
    kfree(buf);
    return res;
}

/**
 * mark_bbt_regions - [GENERIC] mark the bad block table regions
 * @mtd: MTD device structure
 * @td: bad block table descriptor
 *
 * The bad block table regions are marked as "bad" to prevent accidental
 * erasures / writes. The regions are identified by the mark 0x02.
 */
static void mark_bbt_region(struct mtd_info *mtd, struct nand_bbt_descr *td)
{
    struct nand_device *nand = mtd_to_nanddev(mtd);
    int i, j, chips, block, nrblocks, update;
    uint8_t oldval;

    /* Do we have a bbt per chip? */
    if (td->options & NAND_BBT_PERCHIP) {
        chips = nand->bbt.numchips;
        nrblocks = (int)(nand->bbt.chipsize >> nand->bbt.bbt_erase_shift);
    } else {
        chips = 1;
        nrblocks = (int)(mtd->size >> nand->bbt.bbt_erase_shift);
    }

    for (i = 0; i < chips; i++) {
        if ((td->options & NAND_BBT_ABSPAGE) ||
            !(td->options & NAND_BBT_WRITE)) {
            if (td->pages[i] == -1)
                continue;
            block = td->pages[i] >> (nand->bbt.bbt_erase_shift - nand->bbt.page_shift);
            oldval = bbt_get_entry(nand, block);
            bbt_mark_entry(nand, block, BBT_BLOCK_RESERVED);
            if ((oldval != BBT_BLOCK_RESERVED) &&
                    td->reserved_block_code)
                nand_update_bbt(mtd, (loff_t)block <<
                        nand->bbt.bbt_erase_shift);
            continue;
        }
        update = 0;
        if (td->options & NAND_BBT_LASTBLOCK)
            block = ((i + 1) * nrblocks) - td->maxblocks;
        else
            block = i * nrblocks;
        for (j = 0; j < td->maxblocks; j++) {
            oldval = bbt_get_entry(nand, block);
            bbt_mark_entry(nand, block, BBT_BLOCK_RESERVED);
            if (oldval != BBT_BLOCK_RESERVED)
                update = 1;
            block++;
        }
        /*
         * If we want reserved blocks to be recorded to flash, and some
         * new ones have been marked, then we need to update the stored
         * bbts.  This should only happen once.
         */
        if (update && td->reserved_block_code)
            nand_update_bbt(mtd, (loff_t)(block - 1) <<
                    nand->bbt.bbt_erase_shift);
    }
}

/**
 * check_create - [GENERIC] create and write bbt(s) if necessary 如有必要，创建和编写bbt
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function checks the results of the previous call to read_bbt and creates
 * / updates the bbt(s) if necessary. Creation is necessary if no bbt was found
 * for the chip/device. Update is necessary if one of the tables is missing or
 * the version nr. of one table is less than the other.
 该函数检查之前调用read_bbt的结果，并在必要时创建/更新bbt。如果没有找到芯片/设备的bbt，
 则必须创建。如果其中一个表缺失或其中一个表的版本nr小于另一个表，则需要进行更新。
 */
static int check_create(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *bd)
{
    int i, chips, writeops, create, chipsel, res, res2;
    struct nand_device *nand = mtd_to_nanddev(mtd);
    struct nand_bbt_descr *td = nand->bbt.bbt_td;
    struct nand_bbt_descr *md = nand->bbt.bbt_md;
    struct nand_bbt_descr *rd, *rd2;

    /* Do we have a bbt per chip? */
    if (td->options & NAND_BBT_PERCHIP)
        chips = nand->bbt.numchips;
    else
        chips = 1;

    for (i = 0; i < chips; i++) {
        writeops = 0;
        create = 0;
        rd = NULL;
        rd2 = NULL;
        res = res2 = 0;
        /* Per chip or per device? */
        chipsel = (td->options & NAND_BBT_PERCHIP) ? i : -1;
        /* Mirrored table available? */
        if (md) {
            if (td->pages[i] == -1 && md->pages[i] == -1) {
                create = 1;
                writeops = 0x03;
            } else if (td->pages[i] == -1) {
                rd = md;
                writeops = 0x01;
            } else if (md->pages[i] == -1) {
                rd = td;
                writeops = 0x02;
            } else if (td->version[i] == md->version[i]) {
                rd = td;
                if (!(td->options & NAND_BBT_VERSION))
                    rd2 = md;
            } else if (((int8_t)(td->version[i] - md->version[i])) > 0) {
                rd = td;
                writeops = 0x02;
            } else {
                rd = md;
                writeops = 0x01;
            }
        } else {
            if (td->pages[i] == -1) {
                create = 1;
                writeops = 0x01;
            } else {
                rd = td;
            }
        }

        if (create) {
            /* Create the bad block table by scanning the device? */
            if (!(td->options & NAND_BBT_CREATE))
                continue;

            /* Create the table in memory by scanning the chip(s) */
            if (!(nand->bbt.bbt_options & NAND_BBT_CREATE_EMPTY))
                create_bbt(nand, buf, bd, chipsel);

            td->version[i] = 1;
            if (md)
                md->version[i] = 1;
        }

        /* Read back first? */
        if (rd) {
            res = read_abs_bbt(mtd, buf, rd, chipsel);
            if (mtd_is_eccerr(res)) {
                /* Mark table as invalid */
                rd->pages[i] = -1;
                rd->version[i] = 0;
                i--;
                continue;
            }
        }
        /* If they weren't versioned, read both */
        if (rd2) {
            res2 = read_abs_bbt(mtd, buf, rd2, chipsel);
            if (mtd_is_eccerr(res2)) {
                /* Mark table as invalid */
                rd2->pages[i] = -1;
                rd2->version[i] = 0;
                i--;
                continue;
            }
        }

        /* Scrub the flash table(s)? */
        if (mtd_is_bitflip(res) || mtd_is_bitflip(res2))
            writeops = 0x03;

        /* Update version numbers before writing */
        if (md) {
            td->version[i] = max(td->version[i], md->version[i]);
            md->version[i] = td->version[i];
        }

        /* Write the bad block table to the device? */
        if ((writeops & 0x01) && (td->options & NAND_BBT_WRITE)) {
            res = write_bbt(mtd, buf, td, md, chipsel);
            if (res < 0)
                return res;
        }

        /* Write the mirror bad block table to the device? */
        if ((writeops & 0x02) && md && (md->options & NAND_BBT_WRITE)) {
            res = write_bbt(mtd, buf, md, td, chipsel);
            if (res < 0)
                return res;
        }
    }
    return 0;
}

/**
 * nand_isreserved_bbt - [NAND Interface] Check if a block is reserved
 * @mtd: MTD device structure
 * @offs: offset in the device
 这两个函数，在分区表划分时，扫描了整个mtd块的信息
 */
int nanddev_isreserved_bbt(struct nand_device *nand, loff_t offs)
{
    int block;

    block = (int)(offs >> nand->bbt.bbt_erase_shift);
    return bbt_get_entry(nand, block) == BBT_BLOCK_RESERVED;
}
EXPORT_SYMBOL_GPL(nanddev_isreserved_bbt);


/**
 * nand_scan_bbt - [NAND Interface] scan, find, read and maybe create bad block table(s)
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function checks, if a bad block table(s) is/are already available. If
 * not it scans the device for manufacturer marked good / bad blocks and writes
 * the bad block table(s) to the selected place.
 *
 * The bad block table memory is allocated here. It must be freed by calling
 * the nand_free_bbt function.
 */
static int nand_scan_bbt(struct nand_device *nand, struct nand_bbt_descr *bd)
{
    struct mtd_info *mtd = nanddev_to_mtd(nand);
    int len, res;
    uint8_t *buf;
    struct nand_bbt_descr *td = nand->bbt.bbt_td;
    struct nand_bbt_descr *md = nand->bbt.bbt_md;
    int i;

    len = (mtd->size >> (nand->bbt.bbt_erase_shift + 2)) ? : 1;
    /*
     * Allocate memory (2bit per block) and clear the memory bad block
     * table.
     * 分配内存(每个块2位)并清除内存坏块表 128M的flash共申请256字节大小空间
     */
    nand->bbt.bbt = kzalloc(len, GFP_KERNEL);
    if (!nand->bbt.bbt)
        return -ENOMEM;
    /*
     * If no primary table decriptor is given, scan the device to build a
     * memory based bad block table.
     如果没有给出主表的描述符，则扫描设备以建立一个基于内存的坏块表。
     */
    if (!td) {
        if ((res = nand_memory_bbt(nand, bd))) {
            pr_err("nand_bbt: can't scan flash and build the RAM-based BBT\n");
            goto err;
        }
        return 0;
    }
    verify_bbt_descr(mtd, td);
    verify_bbt_descr(mtd, md);

    /* Allocate a temporary buffer for one eraseblock incl. oob  为一个擦除块分配一个临时缓冲区，包括oob */
    len = (1 << nand->bbt.bbt_erase_shift);
    len += (len >> nand->bbt.page_shift) * mtd->oobsize;
    buf = vmalloc(len);
    if (!buf) {
        res = -ENOMEM;
        goto err;
    }

    /* Is the bbt at a given page? */
    if (td->options & NAND_BBT_ABSPAGE) {
        read_abs_bbts(mtd, buf, td, md);
    } else {
        /* Search the bad block table using a pattern in oob */
        search_read_bbts(mtd, buf, td, md);
    }

    res = check_create(mtd, buf, bd);
    if (res)
        goto err;

    /* Prevent the bbt regions from erasing / writing 防止bbt区域被擦除/写入*/
    mark_bbt_region(mtd, td);
    if (md)
        mark_bbt_region(mtd, md);

    vfree(buf);
    return 0;

err:
    kfree(nand->bbt.bbt);
    nand->bbt.bbt = NULL;
    return res;
}
#endif

/**
 * nanddev_bbt_init() - Initialize the BBT (Bad Block Table)
 * @nand: NAND device
 *
 * Initialize the in-memory BBT.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int nanddev_bbt_init(struct nand_device *nand)
{
    unsigned int bits_per_block = fls(NAND_BBT_BLOCK_NUM_STATUS);
    unsigned int nblocks = nanddev_neraseblocks(nand);
    unsigned int nwords = DIV_ROUND_UP(nblocks * bits_per_block,
                       BITS_PER_LONG);
#if defined(CONFIG_TENDA_PRIVATE_BSP) && defined(CONFIG_MTD_NAND)
    int ret;
    struct nand_buffers *nbuf;
    struct mtd_info *mtd = nanddev_to_mtd(nand);

    if (!(nand->bbt.options & NAND_OWN_BUFFERS)) { // 1
        nbuf = kzalloc(sizeof(*nbuf) + mtd->writesize
                + mtd->oobsize * 3, GFP_KERNEL);
        if (!nbuf)
            return -ENOMEM;
        nbuf->ecccalc = (uint8_t *)(nbuf + 1);
        nbuf->ecccode = nbuf->ecccalc + mtd->oobsize;
        nbuf->databuf = nbuf->ecccode + mtd->oobsize;

        nand->bbt.buffers = nbuf;
    } else {
        if (!nand->bbt.buffers)
            return -ENOMEM;
    }
#endif
    nand->bbt.cache = kcalloc(nwords, sizeof(*nand->bbt.cache),
                  GFP_KERNEL);
    if (!nand->bbt.cache)
        return -ENOMEM;

#if defined(CONFIG_TENDA_PRIVATE_BSP) && defined(CONFIG_MTD_NAND)
    if (nand->bbt.bbt_options & NAND_BBT_USE_FLASH) {
    /* Use the default pattern descriptors */
        if (!nand->bbt.bbt_td) {
            if (nand->bbt.bbt_options & NAND_BBT_NO_OOB) {
                nand->bbt.bbt_td = &bbt_main_no_oob_descr;
                nand->bbt.bbt_md = &bbt_mirror_no_oob_descr;
            } else {
                nand->bbt.bbt_td = &bbt_main_descr;
                nand->bbt.bbt_md = &bbt_mirror_descr;
            }
        }
    } else {
        nand->bbt.bbt_td = NULL;
        nand->bbt.bbt_md = NULL;
    }

    nand->bbt.bbt_erase_shift = ffs(mtd->erasesize) - 1;

    if (!nand->bbt.badblock_pattern) {
        ret = nand_create_badblock_pattern(nand);
        if (ret) {
            return ret;
        }
    }

    if (!nand->bbt.controller) {
        nand->bbt.controller = &nand->bbt.hwcontrol;
        spin_lock_init(&nand->bbt.controller->lock);
        init_waitqueue_head(&nand->bbt.controller->wq);
    }
    
    return nand_scan_bbt(nand, nand->bbt.badblock_pattern);
#else
    return 0;
#endif
}
EXPORT_SYMBOL_GPL(nanddev_bbt_init);

#if defined(CONFIG_TENDA_PRIVATE_BSP) && defined(CONFIG_MTD_NAND)
/**
 * nand_isbad_bbt - [NAND Interface] Check if a block is bad
 * @mtd: MTD device structure
 * @offs: offset in the device
 * @allowbbt: allow access to bad block table region
  这两个函数，在分区表初始化时，扫描了整个mtd块的信息，更新了对应的坏块信息
 */
int nanddev_isbad_bbt(struct mtd_info *mtd, loff_t offs, int allowbbt)
{
    struct nand_device *this = mtd_to_nanddev(mtd);
    int block, res;

    block = (int)(offs >> this->bbt.bbt_erase_shift);
    res = bbt_get_entry(this, block);

    pr_debug("nand_isbad_bbt(): bbt info for offs 0x%08x: (block %d) 0x%02x\n",
         (unsigned int)offs, block, res);

    switch (res) {
    case BBT_BLOCK_GOOD:
        return 0;
    case BBT_BLOCK_WORN:
        return 1;
    case BBT_BLOCK_RESERVED:
        return allowbbt ? 0 : 1;
    }
    return 1;
}
EXPORT_SYMBOL_GPL(nanddev_isbad_bbt);
#endif

/**
 * nanddev_bbt_cleanup() - Cleanup the BBT (Bad Block Table)
 * @nand: NAND device
 *
 * Undoes what has been done in nanddev_bbt_init()
 */
void nanddev_bbt_cleanup(struct nand_device *nand)
{
    kfree(nand->bbt.cache);
}
EXPORT_SYMBOL_GPL(nanddev_bbt_cleanup);

/**
 * nanddev_bbt_update() - Update a BBT
 * @nand: nand device
 *
 * Update the BBT. Currently a NOP function since on-flash bbt is not yet
 * supported.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int nanddev_bbt_update(struct nand_device *nand)
{
    return 0;
}
EXPORT_SYMBOL_GPL(nanddev_bbt_update);

/**
 * nanddev_bbt_get_block_status() - Return the status of an eraseblock
 * @nand: nand device
 * @entry: the BBT entry
 *
 * Return: a positive number nand_bbt_block_status status or -%ERANGE if @entry
 *       is bigger than the BBT size.
 */
int nanddev_bbt_get_block_status(const struct nand_device *nand,
                 unsigned int entry)
{
    unsigned int bits_per_block = fls(NAND_BBT_BLOCK_NUM_STATUS);
    unsigned long *pos = nand->bbt.cache +
                 ((entry * bits_per_block) / BITS_PER_LONG);
    unsigned int offs = (entry * bits_per_block) % BITS_PER_LONG;
    unsigned long status;

    if (entry >= nanddev_neraseblocks(nand))
        return -ERANGE;

    status = pos[0] >> offs;
    if (bits_per_block + offs > BITS_PER_LONG)
        status |= pos[1] << (BITS_PER_LONG - offs);

    return status & GENMASK(bits_per_block - 1, 0);
}
EXPORT_SYMBOL_GPL(nanddev_bbt_get_block_status);

/**
 * nanddev_bbt_set_block_status() - Update the status of an eraseblock in the
 *                    in-memory BBT
 * @nand: nand device
 * @entry: the BBT entry to update
 * @status: the new status
 *
 * Update an entry of the in-memory BBT. If you want to push the updated BBT
 * the NAND you should call nanddev_bbt_update().
 *
 * Return: 0 in case of success or -%ERANGE if @entry is bigger than the BBT
 *       size.
 */
int nanddev_bbt_set_block_status(struct nand_device *nand, unsigned int entry,
                 enum nand_bbt_block_status status)
{
    unsigned int bits_per_block = fls(NAND_BBT_BLOCK_NUM_STATUS);
    unsigned long *pos = nand->bbt.cache +
                 ((entry * bits_per_block) / BITS_PER_LONG);
    unsigned int offs = (entry * bits_per_block) % BITS_PER_LONG;
    unsigned long val = status & GENMASK(bits_per_block - 1, 0);

    if (entry >= nanddev_neraseblocks(nand))
        return -ERANGE;

    pos[0] &= ~GENMASK(offs + bits_per_block - 1, offs);
    pos[0] |= val << offs;

    if (bits_per_block + offs > BITS_PER_LONG) {
        unsigned int rbits = bits_per_block + offs - BITS_PER_LONG;

        pos[1] &= ~GENMASK(rbits - 1, 0);
        pos[1] |= val >> rbits;
    }

    return 0;
}
EXPORT_SYMBOL_GPL(nanddev_bbt_set_block_status);
