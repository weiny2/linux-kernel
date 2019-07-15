#!/usr/bin/env python3
import json
import subprocess
import copy
import pprint
import sys
import getopt
import glob
import datetime
import re
import argparse


pp = pprint.PrettyPrinter(indent=4)

manifest_in_path="manifest_in.json"
manifest_out_path="manifest.json"
script_name="Intel Next Merge script"
log_file_name="intel-next-merge-" + format(datetime.date.today()) + ".log"
patch_manifest="patch-manifest"
config_path="arch/x86/configs"
config_fragments = "intel_next_config_options.config"
rule = "------------------------------------------------------------------------"

config_files_to_val = ["intel_next_clear_generic_defconfig", 
               "dcg_x86_64_defconfig", 
               "intel_next_generic_defconfig", 
               "intel_next_rpm_defconfig"]


#config files to add to git
config_files = [config_fragments] + config_files_to_val
#if verbose mode is set output all cmds to stdout            
verbose_mode=False
log = open(log_file_name,"a")
patch = open(patch_manifest,"a")

def run_shell_cmd(cmd):
   
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
    if verbose_mode:
       print("cmd:" +cmd)
       print("STDOUT:"+stdout)
       print("STDERR:"+stderr)
    if retcode != 0:
        raise Exception(stderr)
    return stdout

def run_shell_cmd_output(cmd):
    print(run_shell_cmd(cmd))
    #this is a W/A so buildbot does not buffer stdout
    sys.stdout.flush()

def git_add_remote(name,repo_url,current_remotes):
    if name in current_remotes:
        run_shell_cmd("git remote set-url {} {}".format(name,repo_url))
    else:
        run_shell_cmd("git remote add {} {}".format(name,repo_url))
        current_remotes.append(name)
    return current_remotes

def git_fetch_remote(name):
    print("Fetching {}".format(name))
    run_shell_cmd("git fetch --prune --force {}".format(name))
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
    current_remotes = run_shell_cmd("git remote")
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
    #fetch remots from enabled branches from manifest_in
    #replaces legacy gen-manifest.sh script
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
            rev = run_shell_cmd("git rev-list {}/{} -1".format(name,branch["branch"]))
            rev = rev.split("\n")[0]
        else:
            rev = branch["stuck_at_ref"]
        branch[u"rev"] = rev
    return manifest

def setup_linus_branch(manifest):
    #sets up the master branch what we are merging on top of
    #manifest #copy.deepcopy(manifest_in)
    main_branch = manifest["master_branch"]
    #if use latest_tag is set, get ref of latest tag
    name = sanitize_repo_name(main_branch["repourl"])
    branch_name= main_branch["branch"]
    main_branch[u"tag"] = None
    rev = ""

    if main_branch["use_latest_tag"] == True:
            git_fetch_remote(name)
            tag = run_shell_cmd("git describe {}/{} --abbrev=0".format(name,main_branch["branch"])).split("\n")[0]
            print("using tag",tag)
            main_branch[u"tag"] =tag
            rev = run_shell_cmd("git rev-list {} -1".format(tag))

    elif main_branch["stuck_at_ref"] != "" :
            rev = main_branch["stuck_at_ref"]
    else:
            git_fetch_remote(main_branch["repourl"])
            fetch_rev = run_shell_cmd("git rev-list {}/{} -1".format(name,main_branch["branch"]))
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
    
    print("Checking out master branch")
    run_shell_cmd("git checkout -f master")
    run_shell_cmd("git reset --hard origin/master")

    print("Resetting master to {}".format(msg))
    log.write("Resetting master to {}\n".format(msg))
    run_shell_cmd("git reset --hard {}".format(format(main_branch["rev"])))


def do_merge(manifest, continue_merge):
    #main merge function. use git rerere for cached conflicts
    if continue_merge:
        print(patch_manifest)
        with open(patch_manifest) as f:
            patch_branches = f.read()

    for branch in manifest["topic_branches"]:
        if branch["enabled"] == True:
            if continue_merge and branch["rev"] in patch_branches:
                continue

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
                run_shell_cmd("git merge -m " + merge_msg + " --no-ff --log {} --rerere-autoupdate" .format(branch["rev"]))
            except Exception as e:
                rerere_output= run_shell_cmd("git rerere status")
                if rerere_output == "":
                    run_shell_cmd("git diff --stat --stat-width=200 --cached")
                    run_shell_cmd("git commit --no-edit")
                    print("Git rerere handled merge")
                else:
                    print("Git has has exited with non-zero. check log")
                    raise Exception("Merge has failed. Check git status/merge log to see conflicts/issues. Fix them and then run './merge.py -c (-r)' to continue")
            patch.write("Merging {} {} {} {}\n".format(branch["name"],
                                      branch["repourl"],
                                      branch["branch"],
                                      branch["rev"]))
  
    print("Merge succeeded")


def create_readme_file():
    #Implements the README.intel from the legacy merge.sh
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

def merge_commit(manifest, config_options):
    #Final Intel next commit. We need to make it look good.
    artifacts=["manifest","manifest.json",log_file_name,"README.intel",patch_manifest]
    date = datetime.date.today()
    #commit everything important
    for artifact in artifacts:
        run_shell_cmd("mv {} eywa".format(artifact))
        run_shell_cmd("git add eywa/".format(artifact))

    commit_msg = "Intel Next: Add release files for {} {}\n\n".format(manifest["master_branch"]["tag"],date)
    commit_msg += "\nAdded: {} \n\n ".format(", ".join(artifacts))

    commit_msg +="\nManifest:\n"
    
    for branch in manifest["topic_branches"]:
        if branch["enabled"]:
            commit_msg +="{} {} {}\n".format(branch["repourl"],branch["branch"],branch["rev"])
    commit_msg +="\n\n"
    
    if config_options:
        commit_msg += "\nConfig Files: \n{} \n\n ".format("\n".join(config_options))
        for config in config_options:
            run_shell_cmd("git add {}".format(config))
    commit_msg +="\n\n"

    run_shell_cmd("git commit -s -m '{}'".format(commit_msg))

def print_manifest_log(manifest):
    #Creates TXT version of the manifest which can be sent out
    master= manifest["master_branch"]
    branches = manifest["topic_branches"]
    with open("manifest","w") as f:
        f.write("#Linux upstream\n")
        if master["use_latest_tag"]:
            f.write("{} {} {} {}\n\n".format(master["repourl"],master["tag"],master["branch"],master["rev"]))
        else:
            f.write("{} {} {}\n\n".format(master["repourl"],master["branch"],master["rev"]))
       
        for branch in filter(lambda x: x['enabled'] == True,branches):
            
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

def gen_manifest(skip_fetch,fetch_single):
 
    manifest=read_manifest(manifest_in_path)
    #Add repos as remotes
    add_remotes(manifest)
    #fetch repos
    manifest=fetch_remotes(manifest,skip_fetch,fetch_single)
    manifest=setup_linus_branch(manifest)
    s=json.dumps(manifest,indent=4)
    open(manifest_out_path,"w").write(s)
    print_manifest_log(manifest)
    return manifest


def config_validation(branches):
    #This function checks to make sure all cfg options were set from the manifest
    #Prints a report that is to be read by a human
    all_options = []
    for branch in branches:
        if branch["enabled"] == True:
            all_options+= branch["config_options"]
    
    
    for cfg_file in config_files_to_val:
        print("\nValidation for {}".format(cfg_file)) 
        print(rule)
        with open(config_path + "/" + cfg_file) as cfg_obj:
            gen_cfg =cfg_obj.read()
            gen_cfg_split = gen_cfg.split("\n")
            for option in all_options:
                opt_str = ""
                name = option["name"]
                value = option["value"]
                if value == "n":
                    opt_str = "# {} is not set".format(name)
                else:
                    opt_str = "{}={}".format(name,value)
                if opt_str not in gen_cfg:
                    print("Expected {}".format(opt_str))
                    if name not in gen_cfg:
                        print("  Not found".format(name))
                    else:
                        for line in gen_cfg_split:
                            if name in line:
                                print("  Got {}".format(line))
                                break
    

def run_regen_script(branches):
    run_shell_cmd("mv {} {}".format(config_fragments, config_path))
    run_shell_cmd_output("./regen_configs.sh")
    output_array = []
    #Do validation to make sure everything was set in Kconfig
    config_validation(branches)

    #After validation is complete add files to git
    for config_file in config_files:
        output = run_shell_cmd("git diff {}/{}".format(config_path, config_file))
        lsoutput = run_shell_cmd("git ls-files {}/{}".format(config_path, config_file))
        if (lsoutput == "") or (lsoutput != "" and output != ""):
            output_array.append("{}/{}".format(config_path, config_file))
    return output_array

def gen_config():
    manifest=read_manifest(manifest_out_path)
    branches = manifest["topic_branches"]

    configs_dict = {}
    configs_li =[]
    
    #Create a list of cfg options
    #Track them to make sure there are not conflicting values in two branches
    for branch in branches:
        if branch["enabled"] == True and branch['config_options'] != []:
            config_options = branch["config_options"]
            configs_li.append((" ".join([branch["name"],branch["repourl"],branch["branch"]]),config_options))
            for config in config_options:
                name = config["name"]
                value = config["value"]
                if name in configs_dict and configs_dict[name][0] != value :
                    print("{} is set to {} and {} by '{}' and '{}'".format(name,value,configs_dict[name][0],branch["name"],configs_dict[name][1]))
                    raise Exception("Option set by two different branches, resolve the conflict and rerun")
                configs_dict[name] = (value,branch["name"])
    print("Intel Next CFG options")

    #Once we know there are not any conflicts, run code below that writes the txt fragment from the manifest
    with open(config_fragments,"w") as f:
        for cfg in configs_li:
            print("#Branch: {}".format(cfg[0]))
            f.write("#Branch: {}\n".format(cfg[0]))
            for config in cfg[1]:
                print("{}={}".format(config["name"],config["value"]))
                f.write("{}={}\n".format(config["name"],config["value"]))

    print("Fragment has been written, running ./regen_configs.sh")
    return run_regen_script(branches)

def main():
    parser = argparse.ArgumentParser(description=script_name)
    parser.add_argument('-s','--skip_fetch', help='skip git fetch',action='store_true')
    parser.add_argument('-g','--gen_manifest', help='just generate manifest and don\'t merge',action='store_true')
    parser.add_argument('-c','--continue_merge', help='continue merge using manifest.json/patch_manifest',action='store_true')
    parser.add_argument('-n','--skip_merge', help='skip merge',action='store_true')
    parser.add_argument('-v','--verbose_mode', help='output all git output to terminal',action='store_true')
    parser.add_argument('-u','--update_branch', help='fetch a single branch given repo url',type=str)
    parser.add_argument('-r','--regen_config', help='regen config options after merge is completed or if -n is passed', action='store_true')
    args = parser.parse_args()

    skip_fetch = args.skip_fetch
    continue_merge = args.continue_merge
    fetch_single = args.update_branch
    run_gen_manifest = args.gen_manifest
    regen_config = args.regen_config
    skip_merge = args.skip_merge
   
    global verbose_mode
    verbose_mode = args.verbose_mode
    
   
    if  skip_merge == False and continue_merge == False:
        manifest = gen_manifest(skip_fetch,fetch_single)
        if run_gen_manifest:
            print("Manifest generation has completed")
            return
        #We are starting a new merge so reset the repo
        #manifest=read_manifest(manifest_out_path)
        reset_repo(manifest)
    else:
        manifest=read_manifest("manifest.json")
        print_manifest_log(manifest)
    if skip_merge == False:
        do_merge(manifest, continue_merge)
        create_readme_file()
    
    config_change = None 
    if regen_config:
        config_change = gen_config()

    #Last step is to do the merge commit which checks in log files/cfg files etc 
    merge_commit(manifest, config_change)

if __name__ == "__main__":
    main()

