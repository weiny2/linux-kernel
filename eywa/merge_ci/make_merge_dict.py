#!/usr/bin/env python
#simple script to generate json file with all branches and repos out of manifest.in
#skip internal intel-next repo 
import json

#file location for blacklist 
blacklist_loc ="blacklist"
manifest_loc = "eywa/manifest.in"
def read_blacklist():
    #print("Reading blacklist file")
    blacklist = []
    with open(blacklist_loc) as f:
        data = f.read()
        for line in data.split("\n"):
            if len(line) > 0 and line[0] != "#": #skip comments in blacklist file
                line_to_bl,reason = line.split("#")
                blacklist.append((line_to_bl.strip(),reason))
    return blacklist

def gen_new_manifest(blacklist):
    #print("Running blacklist against manifest.in")
    fobj = open(manifest_loc)
    lines_to_write =[]
    lines = fobj.read()
    fobj.close()
    dis_count = 0
    for line in lines.split("\n"):
        outline = line
        for bentry,reason in blacklist:
            if bentry in line and reason not in line:
                #print("Disabled {}".format(outline))
		#comment out line with reason
                outline = "#" +reason + " "+ outline
                dis_count+=1   
        lines_to_write.append(outline)
    return "\n".join(lines_to_write)

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
    eywa_branch = r"ssh://git-amr-1.devtools.intel.com:29418/otc_intel_next-linux.git"
    for line in manifest_str.split("\n"):
        if eywa_branch not in line and len(line) > 0 and line[0] !="#":
            repo,branch = line.split()
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
    blacklist = read_blacklist()
    manifest = gen_new_manifest(blacklist)
    print(json.dumps(get_branches(manifest),sort_keys=True,indent=4))

if __name__== "__main__":
    main()
