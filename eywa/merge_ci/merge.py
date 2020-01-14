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
import os

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
#Global log file objs
log = None
patch = None

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
   

def fetch_remotes(manifest_in,skip_fetch,fetch_single,fetch_all):
    #fetch remots from enabled branches from manifest_in
    #replaces legacy gen-manifest.sh script
    done_fetch = {}
    name_single = None
    skip=skip_fetch
    if fetch_single != None:
        name_single = sanitize_repo_name(fetch_single)
        skip=True

    manifest = manifest_in
    for branch in manifest["topic_branches"]:
        if branch["enabled"] == False and fetch_all == False:
            continue
        name = sanitize_repo_name(branch["repourl"])
        rev=""
        if not branch["stuck_at_ref"]:
            if (skip == False and name not in done_fetch) or (name_single == name and name not in done_fetch):
                git_fetch_remote(name)
            done_fetch[name] = True
            rev = run_shell_cmd("git rev-list {}/{} -1".format(name,branch["branch"]))
            rev = rev.split("\n")[0]
        else:
            rev = branch["stuck_at_ref"]
        branch[u"rev"] = rev
    return manifest

def setup_linus_branch(manifest,skip_fetch):
    #sets up the master branch what we are merging on top of
    main_branch = manifest["master_branch"]
    #if use latest_tag is set, get ref of latest tag
    name = sanitize_repo_name(main_branch["repourl"])
    branch_name= main_branch["branch"]
    main_branch[u"tag"] = None
    rev = ""

    if main_branch["use_latest_tag"] == True:
            if skip_fetch == False:
                git_fetch_remote(name)
            #this code is taken out of gen_manifest.sh
            #TODO:find better way than this
            tag_cmd = "git ls-remote --tags {}".format(name)
            tag_cmd+= "| awk '{ print $2 }' | awk -F/ ' { print $3 } ' | sort -Vr | head -1 | sed 's/\^{}//'"
            tag = run_shell_cmd(tag_cmd).split("\n")[0]
            print("using tag",tag)
            main_branch[u"tag"] =tag
            rev = run_shell_cmd("git rev-list {} -1".format(tag)).split("\n")[0]

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
                print_and_log("Skipping {} since patch-manifest says it is merged".format(branch["name"]))
                continue

            print_and_log("Merging {} {} {} {}".format(branch["name"],
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
                    print_and_log("Git rerere handled merge")
                else:
                    print_and_log("Git has has exited with non-zero. check log")
                    raise Exception("Merge has failed. Check git status/merge log to see conflicts/issues. Fix them and then run './merge.py -c (-r)' to continue")
            patch.write("Merging {} {} {} {}\n".format(branch["name"],
                                      branch["repourl"],
                                      branch["branch"],
                                      branch["rev"]))
  
    print("Merge succeeded")

def print_and_log(str):
    print(str)
    log.write(str+"\n")


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
    artifacts=["manifest","manifest.json"]
    date = datetime.date.today()
    #commit everything important
    for artifact in artifacts:
        run_shell_cmd("mv {} eywa".format(artifact))
        run_shell_cmd("git add eywa/{}".format(artifact))
    
    #Flush log files and add them as well
    #Use cp for log file because it will be stil be written even after we add to git which is a little weird
    log.flush()
    patch.flush()
    os.fsync(log.fileno())
    os.fsync(patch.fileno())
    run_shell_cmd("git add README.intel")
    run_shell_cmd("cp {} eywa".format(log_file_name))
    run_shell_cmd("git add eywa/{}".format(log_file_name))
    run_shell_cmd("cp {} eywa/{}".format(patch_manifest,patch_manifest))
    run_shell_cmd("git add eywa/{}".format(patch_manifest))
    commit_msg = "Intel Next: Add release files for {} {}\n\n".format(manifest["master_branch"]["tag"],date)
    commit_msg += "\nAdded: {} \n\n ".format(", ".join(artifacts+[log_file_name,patch_manifest,"README.intel"]))

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
                #If we want to print out DISABLED branches them change for loop above
                f.write("#DISABLED {} {}\n\n".format(branch["repourl"],branch["branch"]))


def gen_manifest(skip_fetch,fetch_single,fetch_all,blacklist,whitelist,enable_list):
 
    manifest=read_manifest(manifest_in_path)
    #Process whitelist
    #Don't check to make sure each argument passed it was valid. The user can figure that out
    if whitelist != None:
        topic_branches =[]
        #Do it in this order so we can re-order the manifest based on cmdline
        for name in whitelist:
            for branch in manifest["topic_branches"]:
               if name == branch["name"]:
                   print_and_log("Exclusively enabled {} due to -w option".format(branch["name"]))
                   branch["enabled"] = True
                   topic_branches.append(branch)
                   break
        #overwrite the topic branches with our new list
        manifest["topic_branches"] = topic_branches

    #Proccess blacklist
    #Don't check to make sure each argument passed it was valid. The user can figure that out
    for branch in manifest["topic_branches"]:
        if branch["name"] in blacklist:
           branch["enabled"] = False
           print_and_log("Disabling {} due to -b option".format(branch["name"]))

    #Process enable_list
    for branch in manifest["topic_branches"]:
        if branch["name"] in enable_list:
           branch["enabled"] = True
           print_and_log("Enabling {} due to -e option".format(branch["name"]))


    #clear out manifest.json to show only enabled branches
    final_topic_branches=[]
    for branch in manifest["topic_branches"]:
        if branch["enabled"] == True or fetch_all == True:
            final_topic_branches.append(branch)
    manifest["topic_branches"] = final_topic_branches
    
    #Add repos as remotes
    add_remotes(manifest)
    #fetch repos
    manifest=fetch_remotes(manifest,skip_fetch,fetch_single,fetch_all)
    return manifest


def config_validation(all_options,branch_tracker):
    #This function checks to make sure all cfg options were set from the manifest
    #Prints a report that is to be read by a human
    
    for cfg_file in config_files_to_val:
        print_and_log("\nValidation for {}".format(cfg_file))
        print_and_log(rule)
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
                    print_and_log("Expected {} defined in {}".format(opt_str,",".join(branch_tracker[name])))
                    if name not in gen_cfg:
                        print_and_log("  Not found".format(name))
                    else:
                        for line in gen_cfg_split:
                            if name in line:
                                print_and_log("  Got {} ".format(line))
                                break
    

def run_regen_script(all_options,branch_tracker):
    run_shell_cmd("mv {} {}".format(config_fragments, config_path))
    run_shell_cmd_output("./regen_configs.sh")
    output_array = []
    #Do validation to make sure everything was set in Kconfig
    config_validation(all_options,branch_tracker)
    #After validation is complete add files to git
    for config_file in config_files:
        output = run_shell_cmd("git diff {}/{}".format(config_path, config_file))
        lsoutput = run_shell_cmd("git ls-files {}/{}".format(config_path, config_file))
        if (lsoutput == "") or (lsoutput != "" and output != ""):
            output_array.append("{}/{}".format(config_path, config_file))
    return output_array



def pre_kconfig_validation(branches):
    #check for conflicting options and raise exception if found
    configs_dict = {}
    for branch in branches:
        if branch["enabled"] == True and branch['config_options'] != []:
            config_options = branch["config_options"]
            for config in config_options:
                name = config["name"]
                value = config["value"]
                if name in configs_dict and configs_dict[name][0] != value :
                    print_and_log("{} is set to {} and {} by '{}' and '{}'".format(name,value,configs_dict[name][0],branch["name"],configs_dict[name][1]))
                    #TODO: Make a list of conflicts print all of them out and then raise exception
                    raise Exception("Option set by two different branches, resolve the conflict and rerun")
                configs_dict[name] = (value,branch["name"])
 
def gen_config():
    manifest=read_manifest(manifest_out_path)
    branches = manifest["topic_branches"]
    configs_li =[]
    branch_tracker={}
    all_options=[]
    #Create a list of cfg options, and also a dict matching config to branch names
    for branch in branches:
        if branch["enabled"] == True and branch['config_options'] != []:
            config_options = branch["config_options"]
            configs_li.append((" ".join([branch["name"],branch["repourl"],branch["branch"]]),config_options))
            all_options+= branch["config_options"]
            for opt in branch["config_options"]:
                name = opt["name"]
                if name in branch_tracker:
                    branch_tracker[name].append(branch["name"])
                else:
                    branch_tracker[name] = [branch["name"]]

    #Write CFG options to fragment
    with open(config_fragments,"w") as f:
        for cfg in configs_li:
            print_and_log("#Branch: {}".format(cfg[0]))
            f.write("#Branch: {}\n".format(cfg[0]))
            for config in cfg[1]:
                print_and_log("{}={}".format(config["name"],config["value"]))
                f.write("{}={}\n".format(config["name"],config["value"]))
    
    print_and_log("Fragment has been written, running ./regen_configs.sh")
    return run_regen_script(all_options,branch_tracker)

def run_git_describe(manifest,fetch_all):
    for branch in manifest["topic_branches"]:
        if branch["enabled"] == True or fetch_all == True:
            remote_name = sanitize_repo_name(branch["repourl"])
            tag = run_shell_cmd("git describe {}/{}".format(remote_name,branch["branch"])).split("\n")[0]
            print("{} {}".format(branch["name"],tag))

def write_manifest_files(manifest):
    s=json.dumps(manifest,indent=4)
    open(manifest_out_path,"w").write(s)
    print_manifest_log(manifest)

def open_logs(mode):
    if mode not in ["a","w"]:
        raise Exception("mode must be 'a' or 'w'")
    global log
    global patch
    log = open(log_file_name,mode)
    patch = open(patch_manifest,mode)

def main():
    parser = argparse.ArgumentParser(description=script_name)
    parser.add_argument('-s','--skip_fetch', help='skip git fetch',action='store_true')
    parser.add_argument('-g','--gen_manifest', help='just generate manifest and don\'t merge',action='store_true')
    parser.add_argument('-c','--continue_merge', help='continue merge using manifest.json/patch_manifest',action='store_true')
    parser.add_argument('-v','--verbose_mode', help='output all git output to terminal',action='store_true')
    parser.add_argument('-u','--update_branch', help='fetch a single branch given repo url',type=str)
    parser.add_argument('-r','--regen_config', help='regen config options after merge is completed or if -n is passed', action='store_true')
    parser.add_argument('-d','--run_describe', help='run git describe on all enabled branches (unless -a is passed )', action='store_true')
    parser.add_argument('-b','--blacklist', help='comma seperated list of branches not to merge even if enabled',type=str)
    parser.add_argument('-e','--enable_list', help='comma seperated list of branches to enable if disabled in manifest_in.json',type=str)
    parser.add_argument('-w','--whitelist', help='comma seperated list of branches to exclusively merge (eywa branch is required; configs is required for -r option)',type=str)
    parser.add_argument('-a','--all', help='merge or run git discribe even if branch is disabled', action='store_true')
    args = parser.parse_args()

    skip_fetch = args.skip_fetch
    continue_merge = args.continue_merge
    fetch_single = args.update_branch
    run_gen_manifest = args.gen_manifest
    regen_config = args.regen_config
    run_describe = args.run_describe
    fetch_all = args.all

    blacklist = []
    if args.blacklist != None:
        blacklist = args.blacklist.split(",")
    whitelist = None
    if args.whitelist != None:
        whitelist = args.whitelist.split(",")
    enable_list = []
    if args.enable_list != None:
        enable_list = args.enable_list.split(",")


    global verbose_mode
    verbose_mode = args.verbose_mode
   
    if continue_merge == False:
        open_logs("w")
        manifest = gen_manifest(skip_fetch,fetch_single,fetch_all,blacklist,whitelist,enable_list)
        if run_describe:
           run_git_describe(manifest,fetch_all)
           return
        manifest=setup_linus_branch(manifest,skip_fetch)
        #write manifest files
        write_manifest_files(manifest)
        if run_gen_manifest:
            print("Manifest generation has completed")
            return
        if regen_config:
           pre_kconfig_validation(manifest["topic_branches"])
        reset_repo(manifest)
    else:
        #Continue merge
        open_logs("a")
        print_and_log("continuing merge with -c option")
        manifest=read_manifest("manifest.json")
        print_manifest_log(manifest)
    
    #Do actual merge now that everything is setup
    do_merge(manifest, continue_merge)
    create_readme_file()
    
    config_change = None 
    if regen_config:
        config_change = gen_config()

    #Last step is to do the merge commit which checks in log files/cfg files etc 
    merge_commit(manifest, config_change)
    #If we get here without Exception, merge is done
    print("Merge script has completed without error")

if __name__ == "__main__":
    main()

