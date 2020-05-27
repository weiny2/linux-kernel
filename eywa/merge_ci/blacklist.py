#!/usr/bin/env python
#simple script to blacklist branches 
import re
#file location for blacklist 
blacklist_loc ="blacklist"
manifest_loc = "eywa/manifest.in"
def read_blacklist():
    print("Reading blacklist file")
    blacklist = []
    with open(blacklist_loc) as f:
        data = f.read()
        for line in data.split("\n"):
            if len(line) > 0 and line[0] != "#": #skip comments in blacklist file
                line_to_bl,reason = line.split("#")
                blacklist.append((line_to_bl.strip(),reason))
    return blacklist

def edit_manifest(blacklist):
    print("Running blacklist against manifest.in")
    fobj = open(manifest_loc)
    lines_to_write =[]
    lines = fobj.read()
    fobj.close()
    dis_count = 0
    for line in lines.split("\n"):
    	outline = line
        for bentry,reason in blacklist:
            if bentry in line and reason not in line:
                print("Disabled {}".format(outline))
		#comment out line with reason
                outline = "#" +reason + " "+ outline
                dis_count+=1   
        lines_to_write.append(outline)
    #write out results
    fobj = open(manifest_loc,"w")
    fobj.write("\n".join(lines_to_write))  
    fobj.close()
    print("Disabled {} branches".format(dis_count))

def main():
    blacklist = read_blacklist()
    edit_manifest(blacklist)
   
if __name__== "__main__":
    main()
