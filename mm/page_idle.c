// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/pagemap.h>
#include <linux/rmap.h>
#include <linux/mmu_notifier.h>
#include <linux/page_ext.h>
#include <linux/page_idle.h>
#include <linux/node.h>
#include <linux/migrate.h>
#include <linux/random.h>
#include <linux/xarray.h>

#define BITMAP_CHUNK_SIZE	sizeof(u64)
#define BITMAP_CHUNK_BITS	(BITMAP_CHUNK_SIZE * BITS_PER_BYTE)

/*
 * Idle page tracking only considers user memory pages, for other types of
 * pages the idle flag is always unset and an attempt to set it is silently
 * ignored.
 *
 * We treat a page as a user memory page if it is on an LRU list, because it is
 * always safe to pass such a page to rmap_walk(), which is essential for idle
 * page tracking. With such an indicator of user pages we can skip isolated
 * pages, but since there are not usually many of them, it will hardly affect
 * the overall result.
 *
 * This function tries to get a user memory page by pfn as described above.
 */
static struct page *page_idle_get_page(unsigned long pfn)
{
	struct page *page;
	pg_data_t *pgdat;

	if (!pfn_valid(pfn))
		return NULL;

	page = pfn_to_page(pfn);
	if (!page || !PageLRU(page) ||
	    !get_page_unless_zero(page))
		return NULL;

	pgdat = page_pgdat(page);
	spin_lock_irq(&pgdat->lru_lock);
	if (unlikely(!PageLRU(page))) {
		put_page(page);
		page = NULL;
	}
	spin_unlock_irq(&pgdat->lru_lock);
	return page;
}

static bool page_idle_clear_pte_refs_one(struct page *page,
					struct vm_area_struct *vma,
					unsigned long addr, void *arg)
{
	struct page_vma_mapped_walk pvmw = {
		.page = page,
		.vma = vma,
		.address = addr,
	};
	bool referenced = false;

	while (page_vma_mapped_walk(&pvmw)) {
		addr = pvmw.address;
		if (pvmw.pte) {
			/*
			 * For PTE-mapped THP, one sub page is referenced,
			 * the whole THP is referenced.
			 */
			if (ptep_clear_young_notify(vma, addr, pvmw.pte))
				referenced = true;
		} else if (IS_ENABLED(CONFIG_TRANSPARENT_HUGEPAGE)) {
			if (pmdp_clear_young_notify(vma, addr, pvmw.pmd))
				referenced = true;
		} else {
			/* unexpected pmd-mapped page? */
			WARN_ON_ONCE(1);
		}
	}

	if (referenced) {
		clear_page_idle(page);
		/*
		 * We cleared the referenced bit in a mapping to this page. To
		 * avoid interference with page reclaim, mark it young so that
		 * page_referenced() will return > 0.
		 */
		set_page_young(page);
	}
	return true;
}

static void page_idle_clear_pte_refs(struct page *page)
{
	/*
	 * Since rwc.arg is unused, rwc is effectively immutable, so we
	 * can make it static const to save some cycles and stack.
	 */
	static const struct rmap_walk_control rwc = {
		.rmap_one = page_idle_clear_pte_refs_one,
		.anon_lock = page_lock_anon_vma_read,
	};
	bool need_lock;

	if (!page_mapped(page) ||
	    !page_rmapping(page))
		return;

	need_lock = !PageAnon(page) || PageKsm(page);
	if (need_lock && !trylock_page(page))
		return;

	rmap_walk(page, (struct rmap_walk_control *)&rwc);

	if (need_lock)
		unlock_page(page);
}

static ssize_t page_idle_bitmap_read(struct file *file, struct kobject *kobj,
				     struct bin_attribute *attr, char *buf,
				     loff_t pos, size_t count)
{
	u64 *out = (u64 *)buf;
	struct page *page;
	unsigned long pfn, end_pfn;
	int bit;

	if (pos % BITMAP_CHUNK_SIZE || count % BITMAP_CHUNK_SIZE)
		return -EINVAL;

	pfn = pos * BITS_PER_BYTE;
	if (pfn >= max_pfn)
		return 0;

	end_pfn = pfn + count * BITS_PER_BYTE;
	if (end_pfn > max_pfn)
		end_pfn = max_pfn;

	for (; pfn < end_pfn; pfn++) {
		bit = pfn % BITMAP_CHUNK_BITS;
		if (!bit)
			*out = 0ULL;
		page = page_idle_get_page(pfn);
		if (page) {
			if (page_is_idle(page)) {
				/*
				 * The page might have been referenced via a
				 * pte, in which case it is not idle. Clear
				 * refs and recheck.
				 */
				page_idle_clear_pte_refs(page);
				if (page_is_idle(page))
					*out |= 1ULL << bit;
			}
			put_page(page);
		}
		if (bit == BITMAP_CHUNK_BITS - 1)
			out++;
		cond_resched();
	}
	return (char *)out - buf;
}

static ssize_t page_idle_bitmap_write(struct file *file, struct kobject *kobj,
				      struct bin_attribute *attr, char *buf,
				      loff_t pos, size_t count)
{
	const u64 *in = (u64 *)buf;
	struct page *page;
	unsigned long pfn, end_pfn;
	int bit;

	if (pos % BITMAP_CHUNK_SIZE || count % BITMAP_CHUNK_SIZE)
		return -EINVAL;

	pfn = pos * BITS_PER_BYTE;
	if (pfn >= max_pfn)
		return -ENXIO;

	end_pfn = pfn + count * BITS_PER_BYTE;
	if (end_pfn > max_pfn)
		end_pfn = max_pfn;

	for (; pfn < end_pfn; pfn++) {
		bit = pfn % BITMAP_CHUNK_BITS;
		if ((*in >> bit) & 1) {
			page = page_idle_get_page(pfn);
			if (page) {
				page_idle_clear_pte_refs(page);
				set_page_idle(page);
				put_page(page);
			}
		}
		if (bit == BITMAP_CHUNK_BITS - 1)
			in++;
		cond_resched();
	}
	return (char *)in - buf;
}

static struct bin_attribute page_idle_bitmap_attr =
		__BIN_ATTR(bitmap, 0600,
			   page_idle_bitmap_read, page_idle_bitmap_write, 0);

static struct bin_attribute *page_idle_bin_attrs[] = {
	&page_idle_bitmap_attr,
	NULL,
};

static const struct attribute_group page_idle_attr_group = {
	.bin_attrs = page_idle_bin_attrs,
	.name = "page_idle",
};

#ifndef CONFIG_64BIT
static bool need_page_idle(void)
{
	return true;
}
struct page_ext_operations page_idle_ops = {
	.need = need_page_idle,
};
#endif

static int __init page_idle_init(void)
{
	int err;

	err = sysfs_create_group(mm_kobj, &page_idle_attr_group);
	if (err) {
		pr_err("page_idle: register sysfs failed\n");
		return err;
	}
	return 0;
}
subsys_initcall(page_idle_init);

#define NODE_RANDOM_CANDIDATE_MULTIPLE		8
#define NODE_RANDOM_PREFETCH_NUM		8

struct random_migrate_work_item {
	int nr;
	int threshold;
	unsigned long when;
	struct xarray pages;
};

static inline unsigned long node_random_gen_random_max(unsigned long max)
{
	unsigned long val;

	if (max <= U32_MAX)
		return prandom_u32_max(max);

	prandom_bytes(&val, sizeof(val));

	return val % max;
}

static void flush_tlb_cpu(void *data)
{
	__flush_tlb();
}

static void flush_tlb_node(int nid)
{
	int cpu;
	const struct cpumask *cpu_mask;

	cpu = get_cpu();
	cpu_mask = cpumask_of_node(nid);
	smp_call_function_many(cpu_mask, flush_tlb_cpu, NULL, 1);
	if (cpumask_test_cpu(cpu, cpu_mask))
		flush_tlb_cpu(NULL);
	put_cpu();
}

static void node_random_flush_tlb_node(int nid)
{
	int tnid;

	while (cpumask_empty(cpumask_of_node(nid))) {
		tnid = next_promotion_node(nid);
		if (tnid == -1)
			break;
		nid = tnid;
	}

	flush_tlb_node(nid);
}

static void node_random_mod_delayed_work(int nid, struct delayed_work *dwork,
					 int delay)
{
	int tnid;

	while (cpumask_empty(cpumask_of_node(nid))) {
		tnid = next_promotion_node(nid);
		if (tnid == -1)
			break;
		nid = tnid;
	}

	mod_delayed_work_node(nid, system_wq, dwork, delay);
}

static void node_random_xas_add(struct xa_state *xas, void *entry)
{
	do {
		xas_lock(xas);
		xas_store(xas, entry);
		if (!xas_error(xas))
			xas_next(xas);
		xas_unlock(xas);
	} while (xas_nomem(xas, GFP_KERNEL | __GFP_THISNODE));
}

static struct page *node_random_gen_random_page(struct pglist_data *pgdat)
{
	unsigned long pfn;
	struct page *page;

	do {
		pfn = pgdat->node_start_pfn +
			node_random_gen_random_max(pgdat->node_spanned_pages);
	} while (!pfn_valid(pfn));
	page = pfn_to_page(pfn);
	prefetch(page);

	return page;
}

static void node_random_gen_page_init(struct pglist_data *pgdat,
				      struct page* pages[])
{
	int i;

	for (i = 1; i < NODE_RANDOM_PREFETCH_NUM; i++)
		pages[i] = node_random_gen_random_page(pgdat);
}

static struct page *node_random_gen_page(struct pglist_data *pgdat,
					 struct page* pages[])
{
	struct page *page;

	memmove(pages, pages + 1,
		sizeof(pages[0]) * (NODE_RANDOM_PREFETCH_NUM - 1));
	pages[NODE_RANDOM_PREFETCH_NUM - 1] =
		node_random_gen_random_page(pgdat);

	page = pages[NODE_RANDOM_PREFETCH_NUM/2];
	if (!PagePoisoned(page) && PageCompound(page)) {
		page = compound_head(page);
		prefetch(page);
		pages[NODE_RANDOM_PREFETCH_NUM/2] = page;
	}

	return pages[0];
}

static int node_random_select_pages(struct pglist_data *pgdat, int target_nr_page,
				    struct xarray *pages, bool put)
{
	int i, nr = 0, nr_page = 0;
	struct page *page, *pages_prefetch[NODE_RANDOM_PREFETCH_NUM];
	XA_STATE(xas, pages, 0);

	node_random_gen_page_init(pgdat, pages_prefetch);
	/* Stop when selected target number of pages or tried enough times */
	for (i = 0;
	     i < target_nr_page * 8 && nr_page < target_nr_page;
	     i++) {
		page = node_random_gen_page(pgdat, pages_prefetch);
		/* TODO: Support hugetlbfs */
		if (PagePoisoned(page) || !PageLRU(page) ||
		    PageHuge(page))
			continue;
		/* Nodes PFN range may overlap */
		if (page_pgdat(page) != pgdat)
			continue;
		if (!get_page_unless_zero(page))
			continue;
		/*
		 * get_page_unless_zero() is fully ordered, just
		 * prevent compiler from reordering.
		 */
		barrier();
		if (!PageLRU(page) || PageHuge(page)) {
			put_page(page);
			continue;
		}

		page_idle_clear_pte_refs(page);
		set_page_idle(page);

		node_random_xas_add(&xas, page);
		nr++;
		nr_page += hpage_nr_pages(page);

		if (put)
			put_page(page);
	}

	return nr;
}

static struct random_migrate_work_item *node_random_find_unused_item(
	struct random_migrate_state *rm_state)
{
	int i;
	struct random_migrate_work_item *item;

	for (i = 0; i < rm_state->nr_item; i++) {
		item = rm_state->work_items + i;
		if (!item->nr)
			return item;
	}

	return NULL;
}

static void node_random_promote_select(struct pglist_data *pgdat,
				       struct random_migrate_state *rp_state)
{
	unsigned long now;
	struct random_migrate_work_item *item;

	item = node_random_find_unused_item(rp_state);
	if (!item)
		return;

	item->nr = node_random_select_pages(
			pgdat,
			rp_state->nr_page * NODE_RANDOM_CANDIDATE_MULTIPLE,
			&item->pages, true);

	node_random_flush_tlb_node(pgdat->node_id);

	now = jiffies;
	item->threshold = rp_state->threshold;
	item->when = now + item->threshold;

	rp_state->when_select = now + rp_state->period;
}

static int node_random_keep_pages(struct xarray *pages, int nr,
				  int target_nr_page, int *nrk_page_out,
				  bool idle)
{
	int i, nrk = 0, nrk_page = 0;
	struct page *page;
	XA_STATE(xas, pages, 0);
	XA_STATE(xask, pages, 0);

	for (i = 0; i < nr && nrk_page < target_nr_page * 2; i++) {
		rcu_read_lock();
		page = xas_load(&xas);
		rcu_read_unlock();
		if (PagePoisoned(page) || !PageLRU(page) ||
		    PageHuge(page) || !get_page_unless_zero(page))
			goto next;
		barrier();
		if (!PageLRU(page) || PageHuge(page)) {
			put_page(page);
			goto next;
		}
		if (page_is_idle(page))
			page_idle_clear_pte_refs(page);
		if (page_is_idle(page) != idle) {
			put_page(page);
			goto next;
		}
		node_random_xas_add(&xask, page);
		nrk++;
		nrk_page += hpage_nr_pages(page);
next:
		rcu_read_lock();
		xas_next(&xas);
		rcu_read_unlock();
	}

	*nrk_page_out = nrk_page;
	return nrk;
}

static void node_random_migrate(struct xarray *pages, int nr, int limit,
				int target_nid)
{
	int i, nr_page = 0;
	struct page *page;
	XA_STATE(xas, pages, 0);

	rcu_read_lock();
	for (i = 0; i < nr && nr_page < limit; i++) {
		page = xas_load(&xas);
		rcu_read_unlock();
		nr_page += hpage_nr_pages(page);
		migrate_misplaced_page(page, NULL, target_nid);
		rcu_read_lock();
		xas_next(&xas);
	}
	for (; i < nr; i++) {
		page = xas_load(&xas);
		put_page(page);
		xas_next(&xas);
	}
	rcu_read_unlock();
}

static int random_migrate_threshold_max(struct random_migrate_state *rm_state)
{
	return max(rm_state->period, rm_state->threshold_max);
}

static void node_random_promote_check(struct pglist_data *pgdat,
				      struct random_migrate_state *rp_state,
				      struct random_migrate_work_item *item)
{
	int nrk, nrk_page, tnid, threshold;
	int target_nr_page = rp_state->nr_page;

	nrk = node_random_keep_pages(&item->pages, item->nr, target_nr_page,
				     &nrk_page, false);

	threshold = item->threshold;
	if (nrk_page > target_nr_page * 11 / 10) {
		threshold = min(threshold * 9 / 10, threshold - 1);
		threshold = max(threshold, 1);
	} else if (nrk_page < target_nr_page * 9 / 10) {
		threshold = max(threshold * 11 / 10, threshold + 1);
		threshold = min(threshold, random_migrate_threshold_max(rp_state));
	}
	rp_state->threshold = threshold;

	tnid = next_promotion_node(pgdat->node_id);
	if (tnid == -1)
		return;
	node_random_migrate(&item->pages, nrk, target_nr_page, tnid);
}

static int random_migrate_init(struct pglist_data *pgdat,
			       struct random_migrate_state *rm_state,
			       int nr_item_force)
{
	int i, nr_item;
	struct random_migrate_work_item *item;

	if (rm_state->nr_item)
		return 0;

	if (nr_item_force)
		nr_item = nr_item_force;
	else
		nr_item = random_migrate_threshold_max(rm_state) /
			rm_state->period;
	rm_state->work_items = kzalloc(sizeof(*rm_state->work_items) * nr_item,
				       GFP_KERNEL);
	if (!rm_state->work_items)
		return -ENOMEM;
	rm_state->nr_item = nr_item;

	for (i = 0; i < nr_item; i++) {
		item = rm_state->work_items + i;
		xa_init_flags(&item->pages, GFP_KERNEL | __GFP_THISNODE);
	}

	return 0;
}

static void random_migrate_destroy(struct random_migrate_state *rm_state)
{
	int i;
	struct random_migrate_work_item *item;

	if (!rm_state->nr_item)
		return;

	for (i = 0; i < rm_state->nr_item; i++) {
		item = rm_state->work_items + i;
		xa_destroy(&item->pages);
	}
	kfree(rm_state->work_items);
	rm_state->nr_item = 0;
}

int node_random_migrate_pages(struct pglist_data *pgdat,
			      struct random_migrate_state *rm_state,
			      int target_nid)
{
	int err, nr;
	struct random_migrate_work_item *item;

	err = random_migrate_init(pgdat, rm_state, 1);
	if (err)
		return err;

	item = rm_state->work_items;
	nr = node_random_select_pages(pgdat, rm_state->nr_page,
				      &item->pages, false);
	node_random_migrate(&item->pages, nr, rm_state->nr_page, target_nid);

	random_migrate_destroy(rm_state);

	return 0;
}

static bool node_random_schedule_work(struct pglist_data *pgdat,
				      struct random_migrate_state *rm_state)
{
	bool unused = false;
	int i;
	unsigned long now = jiffies;
	long delay, min_delay = rm_state->period;
	struct random_migrate_work_item *item;

	for (i = 0; i < rm_state->nr_item; i++) {
		item = rm_state->work_items + i;
		if (!item->nr) {
			unused = true;
			continue;
		}
		delay = item->when - now;
		min_delay = min(min_delay, delay);
	}
	if (min_delay < 0)
		return true;

	delay = rm_state->when_select - now;
	if (delay > 0)
		min_delay = min(min_delay, delay);
	else if (unused)
		return true;

	if (!rm_state->period)
		return false;

	node_random_mod_delayed_work(pgdat->node_id, &rm_state->work, min_delay);

	return false;
}

void node_random_promote_work(struct work_struct *work)
{
	int i;
	unsigned long now;
	struct pglist_data *pgdat = container_of(to_delayed_work(work),
						 struct pglist_data,
						 random_promote_state.work);
	struct random_migrate_state *rp_state = &pgdat->random_promote_state;
	struct random_migrate_work_item *item;

	random_migrate_init(pgdat, rp_state, 0);

restart:
	now = jiffies;
	for (i = 0; i < rp_state->nr_item; i++) {
		item = rp_state->work_items + i;
		if (!item->nr || (long)(now - item->when) < 0)
			continue;
		node_random_promote_check(pgdat, rp_state, item);
		item->nr = 0;
	}

	if (now >= rp_state->when_select)
		node_random_promote_select(pgdat, rp_state);

	if (node_random_schedule_work(pgdat, rp_state)) {
		cond_resched();
		goto restart;
	}
}

void node_random_migrate_start(struct pglist_data *pgdat,
			       struct random_migrate_state *rm_state)
{
	rm_state->when_select = jiffies + rm_state->period;
	node_random_mod_delayed_work(pgdat->node_id, &rm_state->work,
				     rm_state->period);
}

void node_random_migrate_stop(struct random_migrate_state *rm_state)
{
	cancel_delayed_work_sync(&rm_state->work);
	random_migrate_destroy(rm_state);
}

static bool pgdat_free_space_enough(struct pglist_data *pgdat)
{
	int z;

	for (z = pgdat->nr_zones - 1; z >= 0; z--) {
		struct zone *zone = pgdat->node_zones + z;

		if (!populated_zone(zone))
			continue;

		if (!zone_watermark_ok(zone, 0,
				       2 * high_wmark_pages(zone),
				       ZONE_MOVABLE, 0))
			return false;
	}
	return true;
}

static void node_random_demote_select(struct pglist_data *pgdat,
				      struct random_migrate_state *rd_state)
{
	unsigned long now;
	struct random_migrate_work_item *item;

	item = node_random_find_unused_item(rd_state);
	if (!item)
		return;

	if (pgdat_free_space_enough(pgdat)) {
		rd_state->when_select = jiffies + rd_state->period;
		return;
	}

	item->nr = node_random_select_pages(
			pgdat,
			rd_state->nr_page * NODE_RANDOM_CANDIDATE_MULTIPLE,
			&item->pages, true);

	node_random_flush_tlb_node(pgdat->node_id);

	now = jiffies;
	item->threshold = rd_state->threshold;
	item->when = now + item->threshold;

	rd_state->when_select = now + rd_state->period;
}

static bool node_random_enabled(struct random_migrate_state *rm_state)
{
	return rm_state->nr_page && rm_state->period;
}

static void node_random_demote_check(struct pglist_data *pgdat,
				     struct random_migrate_state *rd_state,
				     struct random_migrate_work_item *item)
{
	int nrk, nrk_page, tnid, threshold, tpthreshold;
	int target_nr_page = rd_state->nr_page;
	bool tpenabled;
	struct pglist_data *tpgdat;

	if (!item->nr)
		return;

	nrk = node_random_keep_pages(&item->pages, item->nr, target_nr_page,
				     &nrk_page, true);

	tnid = next_migration_node(pgdat->node_id);
	if (tnid == -1)
		return;
	threshold = item->threshold;
	if (nrk_page > target_nr_page * 11 / 10) {
		threshold = max(threshold * 11 / 10, threshold + 1);
		threshold = min(threshold, random_migrate_threshold_max(rd_state));
	} else if (nrk_page < target_nr_page * 9 / 10) {
		tpgdat = NODE_DATA(tnid);
		tpenabled = node_random_enabled(&tpgdat->random_promote_state);
		tpthreshold = tpgdat->random_promote_state.threshold;
		threshold = min(threshold * 9 / 10, threshold - 1);
		threshold = max(threshold, 1);
		if (tpenabled) {
			threshold = max(threshold, tpthreshold * 3 / 2);
			threshold = max(threshold, tpthreshold + 1);
		}
	}
	rd_state->threshold = threshold;

	node_random_migrate(&item->pages, nrk, target_nr_page, tnid);
}

void node_random_demote_work(struct work_struct *work)
{
	int i;
	unsigned long now;
	struct pglist_data *pgdat = container_of(to_delayed_work(work),
						 struct pglist_data,
						 random_demote_state.work);
	struct random_migrate_state *rd_state = &pgdat->random_demote_state;
	struct random_migrate_work_item *item;

	random_migrate_init(pgdat, rd_state, 0);

restart:
	now = jiffies;
	for (i = 0; i < rd_state->nr_item; i++) {
		item = rd_state->work_items + i;
		if (!item->nr || (long)(now - item->when) < 0)
			continue;
		node_random_demote_check(pgdat, rd_state, item);
		item->nr = 0;
	}

	if (now >= rd_state->when_select)
		node_random_demote_select(pgdat, rd_state);

	if (node_random_schedule_work(pgdat, rd_state)) {
		cond_resched();
		goto restart;
	}
}
