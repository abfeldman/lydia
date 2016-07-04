import sys
import re
import os
import subprocess
from sysAndObsBenchmark import sysfile_to_sys
import os

UNZIP_COMMAND = "tar -zxf %s"
EXTRACT_MINC = 'cut -d "," -f 2,4 %s > %s'
EXTRACT_AVG_TIME = re.compile("^Average runtime for solved instances:(\d*\.\d*)") 
EXTRACT_SUCCESS_RATE = re.compile("^Solved (\d*) out of (\d*)")

def aggregate(minc_path, path_in_zip, algorithm_name, system_name):
    lydia_system_name = sysfile_to_sys[system_name]
    results_file_name = "%s_%s.txt.parsed" % (algorithm_name, lydia_system_name)
    results_zip_file_name = "%s.tar.gz" % results_file_name

    if os.path.exists(results_zip_file_name)==False:
        print "%s not found " % results_zip_file_name
        return (system_name, algorithm_name, -1.0,-1.0,-1.0)
    minc_file_name = "%s/%s.csv" % (minc_path, system_name)

    parsed_minc_file_name = "./minc.temp.txt"
    command = UNZIP_COMMAND % results_zip_file_name
    subprocess.call(command, shell=True)

    command = EXTRACT_MINC % (minc_file_name, parsed_minc_file_name)
    subprocess.call(command, shell=True)

    parsed_minc_file = file(parsed_minc_file_name, "r")


    file_name = "%s/%s" % (path_in_zip, results_file_name)
    parsed_results_file = file(file_name,"r")
    accurate_lines = 0
    parsed_minc_file.readline() # First line is not important

    while True:
        results_line = parsed_results_file.readline()

        # If reached last line - break
        if EXTRACT_SUCCESS_RATE.match(results_line) is not None: 
            break

        results_parts = results_line.split(",")
        obs_results = results_parts[1]
        solved = results_parts[2]
        diagnosis_num = results_parts[3]
        minc_results = results_parts[4]

            
        if  solved=="Solved" and int(diagnosis_num)==1:
            while True:
                minc_line = parsed_minc_file.readline()
                minc_parts = minc_line.split(",")        
                obs = minc_parts[0]
                if obs==obs_results:
                    minc = int(minc_parts[1])
                    minc_results = int(minc_results)
                    if minc_results==minc:
                        accurate_lines=accurate_lines+1
                    else:
                        print "Incorrect min. card. at obs %s: expected %d, found %d" % (obs_results,minc, minc_results)
                        if minc_results<minc:
                            raise ValueError("obs %s: minc %d, but found %d" % (obs_results,minc,minc_results))
                    break

    success_parts = EXTRACT_SUCCESS_RATE.match(results_line).groups()   
    solved = int(success_parts[0])   
    total = int(success_parts[1])
    success_rate = float(solved)*100.0/total

    if solved>0:
       accuracy = float(accurate_lines)*100.0/solved
       results_line = parsed_results_file.readline()
       average_runtime = float(EXTRACT_AVG_TIME.match(results_line).groups()[0])/1000
    else:
       average_runtime=-1.0
       accuracy = 0                      
    parsed_minc_file.close()
    parsed_results_file.close()
    # system_name, algorithm_name, success rate, accuracy
    return (system_name, algorithm_name, success_rate, accuracy,average_runtime)

    

def usage():
    print "Usage: <path_to_minc> <path_in_zip> algs <algorithm names> sys <system_names> "    
    print "Parameter 1: path to files with min cardinality numbers"
    print "Parameter 2: path in zip file"
    print "Parameter 3: algorithm names"
    print "Parameter 4: name of systems (optional)"

def main():
    if(len(sys.argv)<3):
        usage()
        return 
    
    minc_path = sys.argv[1]
    path_in_zip = sys.argv[2]
    if(sys.argv[3]!= "algs"):
        usage()
        return
        
    algorithms = []
    param = 4
    while param<len(sys.argv) and sys.argv[param]!="sys":
        algorithms.append(sys.argv[param])
        param = param+1
    
    if sys.argv[param]=="sys":
        system_names = []
        param = param + 1
        while param<len(sys.argv):
            system_names.append(sys.argv[param])
            param = param+1
    else:
        system_names = sysfile_to_sys.keys()
    
    for algorithm_name in algorithms:    
        for system_name in system_names:
            print "%s,%s,%.2f,%.2f,%.2f" % aggregate(minc_path, path_in_zip, algorithm_name, system_name)

    
""" Aggregate results for all system files and a given algorithm """         
if __name__ == '__main__':
    main()
        
    
    
