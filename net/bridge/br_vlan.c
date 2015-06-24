#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/slab.h>

#include "br_private.h"

static void __vlan_add_pvid(struct net_port_vlans *v, u16 vid)
{
	if (v->pvid == vid)
		return;

	smp_wmb();
	v->pvid = vid;
}

static void __vlan_delete_pvid(struct net_port_vlans *v, u16 vid)
{
	if (v->pvid != vid)
		return;

	smp_wmb();
	v->pvid = 0;
}

static void __vlan_add_flags(struct net_port_vlans *v, u16 vid, u16 flags)
{
	if (flags & BRIDGE_VLAN_INFO_PVID)
		__vlan_add_pvid(v, vid);

	if (flags & BRIDGE_VLAN_INFO_UNTAGGED)
		set_bit(vid, v->untagged_bitmap);
}

static int __vlan_add(struct net_port_vlans *v, u16 vid, u16 flags)
{
	struct net_bridge_port *p = NULL;
	struct net_bridge *br;
	struct net_device *dev;
	int err;

	if (test_bit(vid, v->vlan_bitmap)) {
		__vlan_add_flags(v, vid, flags);
		return 0;
	}

	if (v->port_idx) {
		p = v->parent.port;
		br = p->br;
		dev = p->dev;
	} else {
		br = v->parent.br;
		dev = br->dev;
	}

	/* Toggle HW filters when filtering is enabled */
	if (p && p->br->vlan_enabled) {
		/* Add VLAN to the device filter if it is supported.
		 * Stricly speaking, this is not necessary now, since
		 * devices are made promiscuous by the bridge, but if
		 * that ever changes this code will allow tagged
		 * traffic to enter the bridge.
		 */
		err = vlan_vid_add(dev, br->vlan_proto, vid);
		if (err)
			return err;
	}

	err = br_fdb_insert(br, p, dev->dev_addr, vid);
	if (err) {
		br_err(br, "failed insert local address into bridge "
		       "forwarding table\n");
		goto out_filt;
	}

	set_bit(vid, v->vlan_bitmap);
	v->num_vlans++;
	__vlan_add_flags(v, vid, flags);

	return 0;

out_filt:
	if (p && p->br->vlan_enabled)
		vlan_vid_del(dev, br->vlan_proto, vid);
	return err;
}

static int __vlan_del(struct net_port_vlans *v, u16 vid)
{
	if (!test_bit(vid, v->vlan_bitmap))
		return -EINVAL;

	__vlan_delete_pvid(v, vid);
	clear_bit(vid, v->untagged_bitmap);

	if (v->port_idx) {
		struct net_bridge_port *p = v->parent.port;

		/* Toggle HW filters when filtering is enabled */
		if (p->br->vlan_enabled)
			vlan_vid_del(p->dev, p->br->vlan_proto, vid);
	}

	clear_bit(vid, v->vlan_bitmap);
	v->num_vlans--;
	if (bitmap_empty(v->vlan_bitmap, VLAN_N_VID)) {
		if (v->port_idx)
			RCU_INIT_POINTER(v->parent.port->vlan_info, NULL);
		else
			RCU_INIT_POINTER(v->parent.br->vlan_info, NULL);
		kfree_rcu(v, rcu);
	}
	return 0;
}

static void __vlan_flush(struct net_port_vlans *v)
{
	smp_wmb();
	v->pvid = 0;
	bitmap_zero(v->vlan_bitmap, VLAN_N_VID);
	if (v->port_idx)
		RCU_INIT_POINTER(v->parent.port->vlan_info, NULL);
	else
		RCU_INIT_POINTER(v->parent.br->vlan_info, NULL);
	kfree_rcu(v, rcu);
}

struct sk_buff *br_handle_vlan(struct net_bridge *br,
			       const struct net_port_vlans *pv,
			       struct sk_buff *skb)
{
	u16 vid;

	if (!br->vlan_enabled)
		goto out;

	/* Vlan filter table must be configured at this point.  The
	 * only exception is the bridge is set in promisc mode and the
	 * packet is destined for the bridge device.  In this case
	 * pass the packet as is.
	 */
	if (!pv) {
		if ((br->dev->flags & IFF_PROMISC) && skb->dev == br->dev) {
			goto out;
		} else {
			kfree_skb(skb);
			return NULL;
		}
	}

	/* At this point, we know that the frame was filtered and contains
	 * a valid vlan id.  If the vlan id is set in the untagged bitmap,
	 * send untagged; otherwise, send taged.
	 */
	br_vlan_get_tag(skb, &vid);
	if (test_bit(vid, pv->untagged_bitmap))
		skb->vlan_tci = 0;

out:
	return skb;
}

/* Called under RCU */
bool br_allowed_ingress(struct net_bridge *br, struct net_port_vlans *v,
			struct sk_buff *skb, u16 *vid)
{
	bool tagged;
	__be16 proto;

	/* If VLAN filtering is disabled on the bridge, all packets are
	 * permitted.
	 */
	if (!br->vlan_enabled)
		return true;

	/* If there are no vlan in the permitted list, all packets are
	 * rejected.
	 */
	if (!v)
		goto drop;

	proto = br->vlan_proto;

	/* If vlan tx offload is disabled on bridge device and frame was
	 * sent from vlan device on the bridge device, it does not have
	 * HW accelerated vlan tag.
	 */
	if (unlikely(!vlan_tx_tag_present(skb) &&
		     skb->protocol == proto)) {
		skb = vlan_untag(skb);
		if (unlikely(!skb))
			return false;
	}

	if (!br_vlan_get_tag(skb, vid)) {
		/* Tagged frame */
		if (skb->vlan_proto != proto) {
			/* Protocol-mismatch, empty out vlan_tci for new tag */
			skb_push(skb, ETH_HLEN);
			skb = __vlan_put_tag(skb, skb->vlan_proto,
					     vlan_tx_tag_get(skb));
			if (unlikely(!skb))
				return false;

			skb_pull(skb, ETH_HLEN);
			skb_reset_mac_len(skb);
			*vid = 0;
			tagged = false;
		} else {
			tagged = true;
		}
	} else {
		/* Untagged frame */
		tagged = false;
	}

	if (!*vid) {
		u16 pvid = br_get_pvid(v);

		/* Frame had a tag with VID 0 or did not have a tag.
		 * See if pvid is set on this port.  That tells us which
		 * vlan untagged or priority-tagged traffic belongs to.
		 */
		if (!pvid)
			goto drop;

		/* PVID is set on this port.  Any untagged or priority-tagged
		 * ingress frame is considered to belong to this vlan.
		 */
		*vid = pvid;
		if (likely(!tagged))
			/* Untagged Frame. */
			__vlan_hwaccel_put_tag(skb, proto, pvid);
		else
			/* Priority-tagged Frame.
			 * At this point, We know that skb->vlan_tci had
			 * VLAN_TAG_PRESENT bit and its VID field was 0x000.
			 * We update only VID field and preserve PCP field.
			 */
			skb->vlan_tci |= pvid;

		return true;
	}

	/* Frame had a valid vlan tag.  See if vlan is allowed */
	if (test_bit(*vid, v->vlan_bitmap))
		return true;
drop:
	kfree_skb(skb);
	return false;
}

/* Called under RCU. */
bool br_allowed_egress(struct net_bridge *br,
		       const struct net_port_vlans *v,
		       const struct sk_buff *skb)
{
	u16 vid;

	if (!br->vlan_enabled)
		return true;

	if (!v)
		return false;

	br_vlan_get_tag(skb, &vid);
	if (test_bit(vid, v->vlan_bitmap))
		return true;

	return false;
}

/* Called under RCU */
bool br_should_learn(struct net_bridge_port *p, struct sk_buff *skb, u16 *vid)
{
	struct net_bridge *br = p->br;
	struct net_port_vlans *v;

	if (!br->vlan_enabled)
		return true;

	v = rcu_dereference(p->vlan_info);
	if (!v)
		return false;

	if (!br_vlan_get_tag(skb, vid) && skb->vlan_proto != br->vlan_proto)
		*vid = 0;

	if (!*vid) {
		*vid = br_get_pvid(v);
		if (!*vid)
			return false;

		return true;
	}

	if (test_bit(*vid, v->vlan_bitmap))
		return true;

	return false;
}

/* Must be protected by RTNL.
 * Must be called with vid in range from 1 to 4094 inclusive.
 */
int br_vlan_add(struct net_bridge *br, u16 vid, u16 flags)
{
	struct net_port_vlans *pv = NULL;
	int err;

	ASSERT_RTNL();

	pv = rtnl_dereference(br->vlan_info);
	if (pv)
		return __vlan_add(pv, vid, flags);

	/* Create port vlan infomration
	 */
	pv = kzalloc(sizeof(*pv), GFP_KERNEL);
	if (!pv)
		return -ENOMEM;

	pv->parent.br = br;
	err = __vlan_add(pv, vid, flags);
	if (err)
		goto out;

	rcu_assign_pointer(br->vlan_info, pv);
	return 0;
out:
	kfree(pv);
	return err;
}

/* Must be protected by RTNL.
 * Must be called with vid in range from 1 to 4094 inclusive.
 */
int br_vlan_delete(struct net_bridge *br, u16 vid)
{
	struct net_port_vlans *pv;

	ASSERT_RTNL();

	pv = rtnl_dereference(br->vlan_info);
	if (!pv)
		return -EINVAL;

	spin_lock_bh(&br->hash_lock);
	fdb_delete_by_addr(br, br->dev->dev_addr, vid);
	spin_unlock_bh(&br->hash_lock);

	__vlan_del(pv, vid);
	return 0;
}

void br_vlan_flush(struct net_bridge *br)
{
	struct net_port_vlans *pv;

	ASSERT_RTNL();
	pv = rtnl_dereference(br->vlan_info);
	if (!pv)
		return;

	__vlan_flush(pv);
}

bool br_vlan_find(struct net_bridge *br, u16 vid)
{
	struct net_port_vlans *pv;
	bool found = false;

	rcu_read_lock();
	pv = rcu_dereference(br->vlan_info);

	if (!pv)
		goto out;

	if (test_bit(vid, pv->vlan_bitmap))
		found = true;

out:
	rcu_read_unlock();
	return found;
}

static void br_set_hw_filters(struct net_bridge *br)
{
	struct net_bridge_port *p;
	struct net_port_vlans *pv;
	u16 vid, errvid;
	int err;

	/* For each port, walk the vlan bitmap and write the vlan
	 * info to port driver.
	 */
	list_for_each_entry(p, &br->port_list, list) {
		pv = rtnl_dereference(p->vlan_info);
		if (!pv)
			continue;

		for_each_set_bit(vid, pv->vlan_bitmap, VLAN_N_VID) {
			err = vlan_vid_add(p->dev, br->vlan_proto, vid);
			if (err)
				goto err_flt;
		}
	}

	return;

err_flt:
	errvid = vid;
	for_each_set_bit(vid, pv->vlan_bitmap, errvid)
		vlan_vid_del(p->dev, br->vlan_proto, vid);

	list_for_each_entry_continue_reverse(p, &br->port_list, list) {
		pv = rtnl_dereference(p->vlan_info);
		if (!pv)
			continue;

		for_each_set_bit(vid, pv->vlan_bitmap, VLAN_N_VID)
			vlan_vid_del(p->dev, br->vlan_proto, vid);
	}
}

static void br_clear_hw_filters(struct net_bridge *br)
{
	struct net_bridge_port *p;
	struct net_port_vlans *pv;
	u16 vid;

	/* For each port, walk the vlan bitmap and clear
	 * the vlan info from the port driver.
	 */
	list_for_each_entry(p, &br->port_list, list) {
		pv = rtnl_dereference(p->vlan_info);
		if (!pv)
			continue;

		for_each_set_bit(vid, pv->vlan_bitmap, VLAN_N_VID)
			vlan_vid_del(p->dev, br->vlan_proto, vid);
	}
}

static void br_manage_vlans(struct net_bridge *br)
{
	if (br->vlan_enabled)
		br_set_hw_filters(br);
	else
		br_clear_hw_filters(br);
}

int br_vlan_filter_toggle(struct net_bridge *br, unsigned long val)
{
	if (!rtnl_trylock())
		return restart_syscall();

	if (br->vlan_enabled == val)
		goto unlock;

	br->vlan_enabled = val;
	br_manage_vlans(br);
	br_manage_promisc(br);

unlock:
	rtnl_unlock();
	return 0;
}

static bool vlan_default_pvid(struct net_port_vlans *pv, u16 vid)
{
	return pv && vid == pv->pvid && test_bit(vid, pv->untagged_bitmap);
}

static void br_vlan_disable_default_pvid(struct net_bridge *br)
{
	struct net_bridge_port *p;
	u16 pvid = br->default_pvid;

	/* Disable default_pvid on all ports where it is still
	 * configured.
	 */
	if (vlan_default_pvid(br_get_vlan_info(br), pvid))
		br_vlan_delete(br, pvid);

	list_for_each_entry(p, &br->port_list, list) {
		if (vlan_default_pvid(nbp_get_vlan_info(p), pvid))
			nbp_vlan_delete(p, pvid);
	}

	br->default_pvid = 0;
}

static int __br_vlan_set_default_pvid(struct net_bridge *br, u16 pvid)
{
	struct net_bridge_port *p;
	u16 old_pvid;
	int err = 0;
	unsigned long *changed;

	changed = kcalloc(BITS_TO_LONGS(BR_MAX_PORTS), sizeof(unsigned long),
			  GFP_KERNEL);
	if (!changed)
		return -ENOMEM;

	old_pvid = br->default_pvid;

	/* Update default_pvid config only if we do not conflict with
	 * user configuration.
	 */
	if ((!old_pvid || vlan_default_pvid(br_get_vlan_info(br), old_pvid)) &&
	    !br_vlan_find(br, pvid)) {
		err = br_vlan_add(br, pvid,
				  BRIDGE_VLAN_INFO_PVID |
				  BRIDGE_VLAN_INFO_UNTAGGED);
		if (err)
			goto out;
		br_vlan_delete(br, old_pvid);
		set_bit(0, changed);
	}

	list_for_each_entry(p, &br->port_list, list) {
		/* Update default_pvid config only if we do not conflict with
		 * user configuration.
		 */
		if ((old_pvid &&
		     !vlan_default_pvid(nbp_get_vlan_info(p), old_pvid)) ||
		    nbp_vlan_find(p, pvid))
			continue;

		err = nbp_vlan_add(p, pvid,
				   BRIDGE_VLAN_INFO_PVID |
				   BRIDGE_VLAN_INFO_UNTAGGED);
		if (err)
			goto err_port;
		nbp_vlan_delete(p, old_pvid);
		set_bit(p->port_no, changed);
	}

	br->default_pvid = pvid;

out:
	kfree(changed);
	return err;

err_port:
	list_for_each_entry_continue_reverse(p, &br->port_list, list) {
		if (!test_bit(p->port_no, changed))
			continue;

		if (old_pvid)
			nbp_vlan_add(p, old_pvid,
				     BRIDGE_VLAN_INFO_PVID |
				     BRIDGE_VLAN_INFO_UNTAGGED);
		nbp_vlan_delete(p, pvid);
	}

	if (test_bit(0, changed)) {
		if (old_pvid)
			br_vlan_add(br, old_pvid,
				    BRIDGE_VLAN_INFO_PVID |
				    BRIDGE_VLAN_INFO_UNTAGGED);
		br_vlan_delete(br, pvid);
	}
	goto out;
}

int br_vlan_set_default_pvid(struct net_bridge *br, unsigned long val)
{
	u16 pvid = val;
	int err = 0;

	if (val >= VLAN_VID_MASK)
		return -EINVAL;

	if (!rtnl_trylock())
		return restart_syscall();

	if (pvid == br->default_pvid)
		goto unlock;

	/* Only allow default pvid change when filtering is disabled */
	if (br->vlan_enabled) {
		pr_info_once("Please disable vlan filtering to change default_pvid\n");
		err = -EPERM;
		goto unlock;
	}

	if (!pvid)
		br_vlan_disable_default_pvid(br);
	else
		err = __br_vlan_set_default_pvid(br, pvid);

unlock:
	rtnl_unlock();
	return err;
}

int br_vlan_init(struct net_bridge *br)
{
	br->vlan_proto = htons(ETH_P_8021Q);
	br->default_pvid = 1;
	return br_vlan_add(br, 1,
			   BRIDGE_VLAN_INFO_PVID | BRIDGE_VLAN_INFO_UNTAGGED);
}

/* Must be protected by RTNL.
 * Must be called with vid in range from 1 to 4094 inclusive.
 */
int nbp_vlan_add(struct net_bridge_port *port, u16 vid, u16 flags)
{
	struct net_port_vlans *pv = NULL;
	int err;

	ASSERT_RTNL();

	pv = rtnl_dereference(port->vlan_info);
	if (pv)
		return __vlan_add(pv, vid, flags);

	/* Create port vlan infomration
	 */
	pv = kzalloc(sizeof(*pv), GFP_KERNEL);
	if (!pv) {
		err = -ENOMEM;
		goto clean_up;
	}

	pv->port_idx = port->port_no;
	pv->parent.port = port;
	err = __vlan_add(pv, vid, flags);
	if (err)
		goto clean_up;

	rcu_assign_pointer(port->vlan_info, pv);
	return 0;

clean_up:
	kfree(pv);
	return err;
}

/* Must be protected by RTNL.
 * Must be called with vid in range from 1 to 4094 inclusive.
 */
int nbp_vlan_delete(struct net_bridge_port *port, u16 vid)
{
	struct net_port_vlans *pv;

	ASSERT_RTNL();

	pv = rtnl_dereference(port->vlan_info);
	if (!pv)
		return -EINVAL;

	spin_lock_bh(&port->br->hash_lock);
	fdb_delete_by_addr(port->br, port->dev->dev_addr, vid);
	spin_unlock_bh(&port->br->hash_lock);

	return __vlan_del(pv, vid);
}

void nbp_vlan_flush(struct net_bridge_port *port)
{
	struct net_port_vlans *pv;
	u16 vid;

	ASSERT_RTNL();

	pv = rtnl_dereference(port->vlan_info);
	if (!pv)
		return;

	if (port->br->vlan_enabled) {
		for_each_set_bit(vid, pv->vlan_bitmap, VLAN_N_VID)
			vlan_vid_del(port->dev, port->br->vlan_proto, vid);
	}

	__vlan_flush(pv);
}

bool nbp_vlan_find(struct net_bridge_port *port, u16 vid)
{
	struct net_port_vlans *pv;
	bool found = false;

	rcu_read_lock();
	pv = rcu_dereference(port->vlan_info);

	if (!pv)
		goto out;

	if (test_bit(vid, pv->vlan_bitmap))
		found = true;

out:
	rcu_read_unlock();
	return found;
}

int nbp_vlan_init(struct net_bridge_port *p)
{
	return p->br->default_pvid ?
			nbp_vlan_add(p, p->br->default_pvid,
				     BRIDGE_VLAN_INFO_PVID |
				     BRIDGE_VLAN_INFO_UNTAGGED) :
			0;
}
