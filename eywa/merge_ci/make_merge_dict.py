#!/usr/bin/env python
#simple script to generate json file with all branches and repos out of manifest.in
#skip internal intel-next repo 
import json

#file location for blacklist 
manifest_loc = "eywa/manifest"

def add_branch(branches,repo,branch,project):
    for entry in branches:
        if repo == entry["repourl"]:
            entry["branch"].append(branch)
            return
    branches.append({"repourl":repo,"branch":[branch],"project_name":project})

def get_branches(manifest_str):
    branches = [] 
    project_base_name="topic_intel_next"
    #skip linux next kernel repo because that is tracked elsewhere
    eywa_branch = r"ssh://git@gitlab.devtools.intel.com:29418/intel-next/intel-next-kernel.git"
    upstream_repo=r"git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git"
    blacklist =[eywa_branch,upstream_repo]
    for line in manifest_str.split("\n"):
        if line.split(" ")[0] not in blacklist and len(line) > 0 and line[0] !="#":
            parsed = line.split()
            repo = parsed[0]
            branch = parsed[1]
            branch_name = branch.replace("/","_")
            branch_name = branch_name.replace(".","_")
            project = repo.split("://")[1]
            project = project.replace("/","_")
            project = project.replace(":","_")
            project = project.replace(".","_")
            project = project_base_name  + "_" +project 
            add_branch(branches,repo,branch,project)
    return branches

def main():
    manifest = open(manifest_loc).read()
    print(json.dumps(get_branches(manifest),sort_keys=True,indent=4))

if __name__== "__main__":
    main()
