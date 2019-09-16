#!/usr/bin/env python3
import json
import subprocess
import copy
import pprint
import sys
import getopt
import os
import glob
import datetime
import re
import argparse


pp = pprint.PrettyPrinter(indent=4)

manifest_in_path="manifest_in.json"
manifest_out_path="manifest.json"
script_name="Intel Next Merge script"
log_file_name="intel-next-merge-" + format(datetime.date.today()) + ".log"
log=open(log_file_name,"w")


def run_git_cmd(cmd):
   
    log.write("cmd:" +cmd+"\n")

    pobj = subprocess.Popen(cmd,
                     shell = True,
                     stdout = subprocess.PIPE,
                     stderr = subprocess.PIPE)
    #One issue here is that if it is a long git command we won't have output until it is done
    stdout,stderr = pobj.communicate()
    retcode = pobj.returncode
    stdout = stdout.decode('ascii')
    stderr = stderr.decode('ascii')
    log.write("STDOUT:\n"+stdout)
    log.write("STDERR:\n"+stderr)
    log.write("retcode:"+str(retcode) + "\n\n")
    if retcode != 0:
        raise Exception(stderr)
    return stdout


def run_git_cmd_output(cmd):
    print(run_git_cmd(cmd))


def git_add_remote(name,repo_url,current_remotes):
    if name in current_remotes:
        run_git_cmd("git remote set-url {} {}".format(name,repo_url))
    else:
        run_git_cmd("git remote add {} {}".format(name,repo_url))
        current_remotes.append(name)
    return current_remotes

def git_fetch_remote(name):
    print("Fetching {}".format(name))
    run_git_cmd("git fetch --prune --force {}".format(name))
    sys.stdout.flush()

def sanitize_repo_name(repo_url):
    #This is a special case for Len's branch which has a ~ in it
    repo_url = repo_url.replace("~","")
    clean_repo_url = re.sub('[^A-Za-z0-9]+','_',repo_url)
    return clean_repo_url

def read_manifest(path):
    mainfest = None
    with open(path) as f:
        manifest = json.loads(f.read())
    return manifest

def add_remotes(manifest):
    current_remotes = run_git_cmd("git remote")
    current_remotes = current_remotes.split("\n")
   
    main_branch = manifest["master_branch"]
    branches = [main_branch]+ manifest["topic_branches"]
    done_fetch ={}
    #Add remotes
   
    for branch in branches:
        if branch["enabled"] == True:
            url = branch["repourl"]
            name = sanitize_repo_name(url)
            current_remotes = git_add_remote(name,url,current_remotes)
   

def fetch_remotes(manifest_in,skip_fetch,fetch_single):
    done_fetch = {}
    name_single = None
    if fetch_single != None:
        name_single = sanitize_repo_name(fetch_single)
   
    manifest = manifest_in
    for branch in manifest["topic_branches"]:
        if branch["enabled"] == False:
            continue
        name = sanitize_repo_name(branch["repourl"])
        rev=""
        if not branch["stuck_at_ref"]:
            if (skip_fetch == False and name not in done_fetch) or (name_single == name and name not in done_fetch):
                git_fetch_remote(name)
            done_fetch[name] = True
            rev = run_git_cmd("git rev-list {}/{} -1".format(name,branch["branch"]))
            rev = rev.split("\n")[0]
        else:
            rev = branch["stuck_at_ref"]
        branch[u"rev"] = rev
    return manifest

def setup_linus_branch(manifest):
    #manifest #copy.deepcopy(manifest_in)
    main_branch = manifest["master_branch"]
    #if use latest_tag is set, get ref of latest tag
    name = sanitize_repo_name(main_branch["repourl"])
    branch_name= main_branch["branch"]
    main_branch[u"tag"] = None
    rev = ""

    if main_branch["use_latest_tag"] == True:
            latest_tag = run_git_cmd("git ls-remote --tags {} | awk '{{ print $2 }}' | awk -F\/ ' {{ print $3 }} ' | sort -Vr | head -1 ".format(name))
            tag = latest_tag.split("^")[0]
            main_branch[u"tag"] = tag
            rev = run_git_cmd("git rev-parse {}".format(tag)).rstrip()

    elif main_branch["stuck_at_ref"] != "" :
            rev = main_branch["stuck_at_ref"]
    else:
            git_fetch_remote(main_branch["repourl"])
            fetch_rev = run_git_cmd("git rev-list {}/{} -1".format(name,main_branch["branch"]))
            rev = fetch_rev.split("\n")[0]
           
    main_branch[u"rev"] = rev
    return manifest

def reset_repo(manifest):
    main_branch = manifest["master_branch"]
    url = main_branch["repourl"]
    name = sanitize_repo_name(url)
    msg = ""
    if main_branch["use_latest_tag"]:
        msg = main_branch["tag"]
    else:
        msg = main_branch["rev"]

    print("Resetting master to {}".format(msg))
    log.write("Resetting master to {}\n".format(msg))
    run_git_cmd("git reset --hard {}".format(format(main_branch["rev"])))


def do_merge(manifest):
    for branch in manifest["topic_branches"]:
        if branch["enabled"] == True:
            print ("Merging {} {} {} {}".format(branch["name"],
                                      branch["repourl"],
                                      branch["branch"],
                                      branch["rev"]))
            log.write("Merging {} {} {} {}\n".format(branch["name"],
                                      branch["repourl"],
                                      branch["branch"],
                                      branch["rev"]))
            try:
                merge_msg= '"Intel Next: Merge commit {} from {} {}"'.format(branch["rev"],branch["repourl"],branch["branch"])
                run_git_cmd("git merge -m " + merge_msg + " --no-ff --log {} --rerere-autoupdate" .format(branch["rev"]))
            except Exception as e:
                rerere_output= run_git_cmd("git rerere status")
                if rerere_output == "":
                    print("Git rerere handled merge")
                    run_git_cmd("git diff --stat --stat-width=200 --cached")
                    run_git_cmd_output("git commit --no-edit")
                else:
                    raise(e)
    print("Merge succeeded")


def create_readme_file():
    rule = "------------------------------------------------------------------------"
    readme_out="INTEL CONFIDENTIAL. FOR INTERNAL USE ONLY.\n"
    readme_out+=rule+"\n"
    for f in glob.glob("README.*"):
        with open(f) as fobj:
            readme_out +=f + ":\n\n"
            readme_out +=fobj.read()+"\n"

    readme_out +="\n" +rule +"\n"
    readme_out +="Intel Next Maintainers <intel-next-maintainers@eclists.intel.com>"
    with open("README.intel","w") as f:
        f.write(readme_out)

def merge_commit(manifest):
    artifacts=["manifest","manifest.json",log_file_name,"README.intel"]
    date = datetime.date.today()
    #commit everything important
    for artifact in artifacts:
        os.system("cp {} eywa".format(artifact))
        run_git_cmd("git add eywa/".format(artifact))

    commit_msg = "Intel Next: Add release files for {} {}\n\n".format(manifest["master_branch"]["tag"],date)
    commit_msg += "Added: {} \n\n ".format(", ".join(artifacts))
    commit_msg +="manfiest:\n"

    for branch in manifest["topic_branches"]:
        if branch["enabled"]:
            commit_msg +="{} {} {}\n".format(branch["repourl"],branch["branch"],branch["rev"])
    commit_msg +="\n"
    run_git_cmd("git commit -s -m '{}'".format(commit_msg))

def print_manifest_log(manifest):
    master= manifest["master_branch"]
    branches = manifest["topic_branches"]
    with open("manifest","w") as f:
        f.write("#Linux upstream\n")
        if master["use_latest_tag"]:
            f.write("{} {} {} {}\n\n".format(master["repourl"],master["tag"],master["branch"],master["rev"]))
        else:
            f.write("{} {} {}\n\n".format(master["repourl"],master["branch"],master["rev"]))
       
        for branch in branches:
            contributors = ["{} <{}>".format(contrib["name"],contrib["email"]) for contrib in branch["contributor"]]
            ip_owner = ["{} <{}>".format(contrib["name"],contrib["email"]) for contrib in branch["ip_owner"]]
            sdl_contact = ["{} <{}>".format(contrib["name"],contrib["email"]) for contrib in branch["sdl_contact"]]
            cfg_options = ["#\t{}={}".format(cfg["name"],cfg["value"]) for cfg in branch["config_options"]]

            f.write("#Topic Branch: {}\n".format(branch["name"]))
            f.write("#Classification: {}\n".format(branch["status"]))
            f.write("#Description: {}\n".format(branch["description"]))
            f.write("#Jira: {}\n".format(branch["jira"]))
            f.write("#Contributor: {}\n".format(", ".join(contributors)))
            f.write("#Branch Type: {}\n".format(branch["branch_type"]))
            f.write("#Ip Owner: {}\n".format(", ".join(ip_owner)))
            f.write("#SDL Contact: {}\n".format(", ".join(sdl_contact)))
            if cfg_options != []:
                f.write("#Config Options:\n{}\n".format("\n".join(cfg_options)))
            else:
                f.write("#Config Options: N/A\n")
           
            if branch["enabled"]:
                f.write("{} {} {}\n\n".format(branch["repourl"],branch["branch"],branch["rev"]))
            else:
                f.write("#DISABLED {} {}\n\n".format(branch["repourl"],branch["branch"]))

def gen_manifest(use_rest_api,skip_fetch,fetch_single):
    if use_rest_api:
        os.system("curl http://10.54.75.56:9000/intelnext/api/manifest > manifest_in.json")
 
    manifest=read_manifest(manifest_in_path)
    #Add repos as remotes
    add_remotes(manifest)
    #fetch repos
    manifest=fetch_remotes(manifest,skip_fetch,fetch_single)
    manifest=setup_linus_branch(manifest)
    s=json.dumps(manifest,indent=4)
    open(manifest_out_path,"w").write(s)
    print_manifest_log(manifest)

def main():
    parser = argparse.ArgumentParser(description=script_name)
    parser.add_argument('-s','--skip_fetch', help='Skip git fetch',action='store_true')
    parser.add_argument('-g','--gen_manifest', help='Just generate manifest and dont merge',action='store_true')
    parser.add_argument('-c','--continue_merge', help='Continue merge using manifest.json',action='store_true')
    parser.add_argument('-o','--online', help='Use REST API to download manifest_in.json',action='store_true')
    parser.add_argument('-u','--update_branch', help='fetch a single branch given repo url',type=str)
    args = parser.parse_args()

    skip_fetch = args.skip_fetch
    continue_merge = args.continue_merge
    fetch_single = args.update_branch
    online = args.online
    run_gen_manifest = args.gen_manifest
    
    if  continue_merge == False:
        gen_manifest(online,skip_fetch,fetch_single)
        if run_gen_manifest:
            print("Manifest generation has completed")
            return
        #We are starting a new merge so reset the repo
        reset_repo(manifest)
    else:
        #TODO:Handle log as append
        manifest=read_manifest("manifest.json")
        print_manifest_log(manifest)
    do_merge(manifest)
    create_readme_file()
    merge_commit(manifest)

if __name__ == "__main__":
    main()

