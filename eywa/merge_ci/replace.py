#!/usr/bin/env python
#simple script to blacklist branches 
import re
#file location for blacklist 
blacklist_loc ="replace"
manifest_loc = "manifest"
def read_blacklist():
    print("Reading replace file")
    blacklist = []
    with open(blacklist_loc) as f:
        data = f.read()
        for line in data.split("\n"):
            if len(line) > 0 and line[0] != "#": #skip comments in blacklist file
                line_to_bl = ""
                if "#" in line :
                    line_to_bl,reason = line.split("#")
                blacklist.append((line_to_bl.strip(),reason))
    return blacklist

def edit_manifest(blacklist):
    print("Running replace against manifest")
    fobj = open(manifest_loc)
    lines_to_write =[]
    lines = fobj.read()
    fobj.close()
    dis_count = 0
    replace_count =0
    for line in lines.split("\n"):
        outline = line
        for bentry,reason in blacklist:
            if bentry in line and reason not in line:
                print("Replacing {}".format(outline))
                #comment out line with reason
                outline = reason 
                dis_count+=1
        lines_to_write.append(outline)
    #write out results
    fobj = open(manifest_loc,"w")
    fobj.write("\n".join(lines_to_write))  
    fobj.close()
    print("Replacing {} branches".format(dis_count))

def main():
    blacklist = read_blacklist()
    edit_manifest(blacklist)
   
if __name__== "__main__":
    main()
