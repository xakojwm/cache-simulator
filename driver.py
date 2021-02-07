#!/usr//bin/python
#
# driver.py - The driver tests the correctness of the student's cache
#     simulator. It uses ./test-csim to check the correctness of the
#     simulator
#
import subprocess;
import re;
import os;
import sys;
import optparse;


#
# main - Main function
#
def main():

    # Configure maxscores here
    maxscore= {};
    maxscore['csim'] = 27

    # Parse the command line arguments 
    p = optparse.OptionParser()
    p.add_option("-A", action="store_true", dest="autograde", 
                 help="emit autoresult string for Autolab");
    opts, args = p.parse_args()
    autograde = opts.autograde

    # Check the correctness of the cache simulator
    print "Part A: Testing cache simulator"
    print "Running ./test-csim"
    p = subprocess.Popen("./test-csim", 
                         shell=True, stdout=subprocess.PIPE)
    stdout_data = p.communicate()[0]

    # Emit the output from test-csim
    stdout_data = re.split('\n', stdout_data)
    for line in stdout_data:
        if re.match("TEST_CSIM_RESULTS", line):
            resultsim = re.findall(r'(\d+)', line)
        else:
            print "%s" % (line)


    
    
# execute main only if called as a script
if __name__ == "__main__":
    main()

__main__":
    main()

