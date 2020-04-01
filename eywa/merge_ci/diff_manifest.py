#!/usr/bin/env python3 
import json
import sys

def read_manifest(path):
    """
    Opens a path and returns a JSON object from the contents of the path.
    """
    mainfest = None
    with open(path) as f:
        manifest = json.loads(f.read())
    return manifest


def print_branches(branches, rev_length=None):
    """
    Takes an iterable of branch dictionaries and prints some contents.
    The optional integer parameter rev_length will shorten the 'rev' value if
    provided.
    If the iterable is empty, print "None".
    """
    if not branches:
        print("None")
    else:
        for branch in branches:
            if rev_length is not None:
                branch["short_rev"] = branch["rev"][:rev_length]
            else:
                branch["short_rev"] = branch["rev"]
            print("{name}: {repourl} {branch} {short_rev}".format(**branch))


def branches_added(m_new,m_old):
    """
    Returns a generator of branches that are in m_new but not in m_old.
    """
    old_name_set = set(branch["name"] for branch in m_old["topic_branches"])
    return [ branch for branch in m_new["topic_branches"] 
                if branch["name"] not in old_name_set ]


def branches_deleted(m_new,m_old):
    """
    Returns a generator of branches that are in m_old but not in m_new.
    """
    new_name_set = set(branch["name"] for branch in m_new["topic_branches"])
    return [ branch for branch in m_old["topic_branches"]
                if branch["name"] not in new_name_set ]


def create_branch_dict(manifest_dict):
    """
    Creates a dictionary from a manifest's topic branches that associates each
    branch's name with the branch.
    This makes lookups O(n) instead of O(n^2) when comparing between two
    lists of branches. (I know, over-engineering, grumble grumble grumble)
    """
    return { branch["name"] : branch for branch in manifest_dict["topic_branches"] }


def merge_branch_dicts(old_branches, new_branches, rev_length=7):
    """
    Returns the intersection of old_branches and new_branches, with a twist:
    the rev values are concatenated together, separated by an arrow string.
    Additionally, values are only added to the result dictionary if the revision
    changed.  This is for presenting updates to branches.
    """
    result_dict = {}
    for name, old_branch in old_branches.items():
        if name in new_branches and old_branch["rev"] != new_branches[name]["rev"]:
            new_dict = old_branch.copy()
            old_branch_rev = old_branch["rev"][:rev_length]
            new_branch_rev = new_branches[name]["rev"][:rev_length]
            new_dict["rev"] = f"{old_branch_rev} -> {new_branch_rev}"
            result_dict[name] = new_dict
    return result_dict.values()
    

def branches_changed(m_new,m_old):
    """
    Wrapper method for calling merge_branch_dicts on two manifests of branches.
    """
    branch_dict_new = create_branch_dict(m_new)
    branch_dict_old = create_branch_dict(m_old)
    return merge_branch_dicts(branch_dict_new, branch_dict_old)

def main():
    if len(sys.argv) != 3:
       print("{} <new-manifes.json> <old-manifest.json>".format(sys.argv[0]))
       sys.exit(1)

    REV_LENGTH = 7
    
    m_new = read_manifest(sys.argv[1])
    m_old = read_manifest(sys.argv[2])
    added = branches_added(m_new,m_old)
    deleted = branches_deleted(m_new,m_old)
    changed = branches_changed(m_new,m_old)
    print ("Total branches merged: {}".format(len(m_new["topic_branches"])))
    print("Branches added/enabled since previous release:")
    print_branches(added, REV_LENGTH)
    print("Branches deleted/disabled since previous release:")
    print_branches(deleted, REV_LENGTH)
    print("Branches updated since previous release:")
    print_branches(changed)

if __name__ == "__main__":
    main()
