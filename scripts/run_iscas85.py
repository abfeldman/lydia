import sys
import re
import os
from sysAndObsBenchmark import sys_to_obs
from sysAndObsBenchmark import sysfile_to_sys
from sysAndObsBenchmark import suite_to_systems
import parseResults
import subprocess
import threading
import time
from twisted.test.test_hook import subPost

# Common commands:
# Check on results: python ./parse.py dual_gdde_c432.txt g

# --------------------- Constants ----------------------------

DUAL_DFS_GDDE_DETAILS = {"name":"dual_dfs_gdde", "process_name":"gdde", "path":"gdde/gdde", "params":'"-d -m -f"', "sys_suffix":"cnf", "obs_suffix":"dnf"}
GDDE_DFS_DETAILS = {"name":"dfs_gdde", "process_name":"gdde", "path":"gdde/gdde", "params":'"-m -f"', "sys_suffix":"cnf", "obs_suffix":"dnf"}
DUAL_GDDE_DETAILS = {"name":"dual_gdde", "process_name":"gdde", "path":"gdde/gdde", "params":'"-d -m"', "sys_suffix":"cnf", "obs_suffix":"dnf"} 
# CAUTION: DO NOT RUN DUAL_GDDE AND GDDE IN PARALLEL, AS THEY WILL KILL EACH OTHERS' PROCESS
GDDE_DETAILS = {"name":"gdde", "process_name":"gdde", "path":"gdde/gdde", "params":"-m", "sys_suffix":"cnf", "obs_suffix":"dnf"}
CDAS_DETAILS = {"name":"cdas", "process_name":"cdas", "path":"cdas/cdas", "params":'"-c -m"', "sys_suffix":"cnf", "obs_suffix":"dnf"}
GOTCHA_DETAILS = {"name":"gotcha", "process_name":"gotcha", "path":"gotcha/gotcha", "params":" ", "sys_suffix":"hdnf", "obs_suffix":"dnf"}
SAFARI_DETAILS = {"name":"safari", "process_name":"safari", "path":"safari/safari", "params":'"--runs 1"', "sys_suffix":"cnf", "obs_suffix":"dnf"}
ALL_ALGORITHMS = {"safari":SAFARI_DETAILS, "cdas":CDAS_DETAILS, "gotcha":GOTCHA_DETAILS,"gdde":GDDE_DETAILS,\
                   "dual_gdde":DUAL_GDDE_DETAILS, "dual_dfs_gdde":DUAL_DFS_GDDE_DETAILS, "dfs_gdde":GDDE_DFS_DETAILS}
MAIL_COMMAND = 'echo "Lydia: %s %s log attached" | mutt -a %s -s "%s %s logs (ISCAS1)" -- roni.stern@gmail.com'
ZIP_COMMAND = 'tar -zcf %s %s' 
PARSE_COMMAND = 'python parse.py %s %s > %s' 
TIMEOUT = 30
WAIT_FOR_KILL_TIME = 1



MC_PATTERN = re.compile("// MC = (\d*)") # Pattern for getting MC comment in Lydia observations

# --------------------- Methods ----------------------------

def append_ISCAS_85(output_file_name, obs_per_m=-1):
    append_sys_and_obs_file("../examples/diagnosis/","c432", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","c499", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","c880", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","c1355", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","c1908", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","c2670", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","c3540", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","c5315", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","c6288", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","c7552", output_file_name)

def append_74X(output_file_name, obs_per_m=-1):
    append_sys_and_obs_file("../examples/diagnosis/","74181", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","74182", output_file_name)
    append_sys_and_obs_file("../examples/diagnosis/","74283", output_file_name)    

def create_template(output_file_name):
    in_file = file("sysAndObsBenchmarkTemplate.py","r")
    out_file = file(output_file_name,"a")
    for line in in_file:
        out_file.write(line)
    out_file.write("\n")
    in_file.close()
    out_file.close()

def append_suites(output_file_name):
    """ Appends the standard suites """
    in_file = file("benchmark_suites.py","r")
    out_file = file(output_file_name,"a")
    for line in in_file:
        out_file.write(line)
    out_file.write("\n")
    in_file.close()
    out_file.close()
        
    
    
    

def append_sys_and_obs_file(system_file_path, system_file_prefix, sys_and_obs_file):
    """ 
    Appends a systemsAndObservsations file with a mapping of system file name 
    to system name to observations name 
    
    Example inputs:
        append_sys_and_obs_file("../examples/diagnosis/","2subtractor", "test.txt")
        append_sys_and_obs_file("../examples/diagnosis/","c17", "test.txt")            
    """
    OBS_PER_MC = 10
    
    full_path = system_file_path+system_file_prefix
    
    # Get system name 
    sys_file = file(full_path + ".sys","r")
    sys_name_pattern = re.compile("system (.*)\(\)")
    sys_name = None
    
    line = sys_file.readline()
    while(len(line)>0):
        match = sys_name_pattern.search(line)
        if match is not None:
            sys_name = match.group(1)
            break
        line = sys_file.readline()
    
    if sys_name is None:
        print("Did not find system name in %s.sys file" % full_path + ".sys")
    else:
        print("Found system name %s in file %s.sys" % (sys_name,full_path + ".sys"))
    sys_file.close()
    
    # Get observations name
    obs_file_name = full_path + ".obs"
    obs_file = file(obs_file_name,"r")
    observation_name_pattern = re.compile("observation (.*)")
    observations = []
    mc_counter = 0
    min_cardinality = -1
    line = obs_file.readline()
    while(len(line)>0):
        match = MC_PATTERN.search(line)
        if(match is not None):
            min_cardinality = int(match.groups()[0])
            print "Processing observations of cardinality %d" % min_cardinality
            mc_counter = 0
        else:            
            match = observation_name_pattern.search(line)
            if match is not None:
                obs = match.group(1)
                
                print "Found observation %s for system %s" % (obs, sys_name)
                if(mc_counter>OBS_PER_MC):
                    print "Too much (%d) obs for MC (%d). Ignoring observation %s" % (mc_counter, min_cardinality, obs)
                else:
                    observations.append(obs)
                mc_counter = mc_counter+1
                
        line = obs_file.readline()
    obs_file.close()    
    
    out_file = file(sys_and_obs_file,"a")
    out_file.write('sysfile_to_sys["%s"]="%s"\n' % \
                   (system_file_prefix, sys_name))
    out_file.write('sys_to_obs["%s"]=[\\\n' % sys_name)
    for obs in observations:
        out_file.write('           "%s",\\\n' % obs)
    out_file.write('           ]\n')


def prepare(prepare_script):
    """
    Compile the sys definitions and observations for a solver
    """    
    DIAGNOSIS_EXAMPLES_DIR = "../examples/diagnosis"
    SCRIPTS_DIR = "../../scripts"
    COMPILED_DIR = "compiled"
    original_path = os.path.abspath(".")
    
    os.chdir(DIAGNOSIS_EXAMPLES_DIR)
    
    for system_file_name in sysfile_to_sys.keys():
        command = "%s/%s %s %s" % ( SCRIPTS_DIR,prepare_script,system_file_name, COMPILED_DIR)
        print command   
        subprocess.call(command, shell=True)

    os.chdir(original_path)


def send_results(out_file_name, alg_details, system_file_name):
    """
    Zip the results and send them via e-mail
    """
    out_file_name = os.path.abspath(out_file_name)
    zip_out_file_name = "%s.tar.gz" % out_file_name
    command = ZIP_COMMAND % (zip_out_file_name,out_file_name)
    print command
    subprocess.call(command, shell=True)

    command = MAIL_COMMAND % (alg_details["name"], system_file_name, zip_out_file_name, alg_details["name"], system_file_name)
    print command
    subprocess.call(command, shell=True)



def run_alg_on_system(alg_details, system_file_name, send_mail):
    """ execute the given algorithm on the given systems """
    
    print "Running %s on %s..." % (alg_details["name"],system_file_name)
    
    system_name = sysfile_to_sys[system_file_name]

    out_file_name = "%s_%s.txt" % (alg_details["name"], system_name)
    command = "./run.sh %s %s %s %s %s > %s" % (system_file_name, alg_details["path"], alg_details["sys_suffix"],\
                                                       alg_details["obs_suffix"], alg_details["params"],out_file_name)
    print command
    subprocess.call(command, shell=True)   

    print "Parse results..."
    parsed_out_file_name = "%s.parsed" % (out_file_name)
    command = PARSE_COMMAND % (out_file_name, parsed_out_file_name)
    print command
    subprocess.call(command, shell=True)   

    if send_mail:
        print "Sending the results via e-mail..."        
        send_results(parsed_out_file_name, alg_details, system_file_name)

    return out_file_name


def kill_process(proc_name):
    """ Kills the process with the given name """
    is_alive = is_process_alive(proc_name)
    kill_command = "pkill %s" % proc_name
    
    while is_alive:        
        print "Killing %s" % proc_name
        proc = subprocess.Popen(kill_command, shell=True)
        proc.wait()
        time.sleep(1)
        is_alive = is_process_alive(proc_name)
    
    
def is_process_alive(proc_name):
    """ Check if a given process is alive """
    temp_file_name = "is_process_alive.tmp"
    proc = subprocess.Popen("pgrep %s > %s" % (proc_name,temp_file_name), shell=True)
    proc.wait()
    temp_file = file(temp_file_name, "r")
    lines = temp_file.readlines()
    if len(lines)>0 and len(lines[0])>0:
        return True
    else:
        return False
    

def run_alg_on_system_per_obs(alg_details, system_file_name, send_mail):
    """ execute the given algorithm on the given systems. Create a process per observation """
    
    print "Running %s on %s..." % (alg_details["name"],system_file_name)
    
    system_name = sysfile_to_sys[system_file_name]
    out_file_name = "%s_%s.txt" % (alg_details["name"], system_name)
    out_file = file(out_file_name,"w")
    out_file.close()
    
    temp_out_file_name = "%s.tmp" % out_file_name
    for observation_name in sys_to_obs[system_name]:
        #execute diagnosis algorithm on an observation

        command = "./run_single.sh %s %s %s %s %s %s > %s" % (system_file_name, observation_name, alg_details["path"], \
                                                                  alg_details["sys_suffix"], alg_details["obs_suffix"],\
                                                                  alg_details["params"],temp_out_file_name)
        command = "%s %d %s\n" % ("python ./timeoutExec.py ", TIMEOUT+30, command)
        print command
        subprocess.call(command, shell=True)
        
        kill_process(alg_details["process_name"])
        
        # Write @Done at end of every observation for parse.py
        temp_file = file(temp_out_file_name,"a")
        temp_file.write("@Done\n")    
        temp_file.close()    
        
        # Append results to a big results file
        command = "./append.sh %s %s" % (out_file_name, temp_out_file_name)
        print command
        subprocess.call(command, shell=True)           

    print "Parse results..."
    parsed_out_file_name = "%s.parsed" % (out_file_name)
    command = PARSE_COMMAND % (out_file_name, alg_details["name"], parsed_out_file_name)
    print command
    subprocess.call(command, shell=True)   

    if send_mail:
        print "Sending the results via e-mail..."        
        send_results(parsed_out_file_name, alg_details, system_file_name)

    return out_file_name

def main():
    """ 
    Runs all the observations of a given system.
    Basically, this outputs the commands that lydia should run.
    """

    # execute prepare script on all ISCAS if this is required
    if len(sys.argv)>2 and sys.argv[1]=="prepare":
        print "Preparing system and observation files..."
        prepare(sys.argv[2])
        return
    if  len(sys.argv)>2 and sys.argv[1]=="loadSysAndObs":
        obs_per_mc = int(sys.argv[2])
        create_template("sysAndObsBenchmark_%d.py" % obs_per_mc)
        append_ISCAS_85("sysAndObsBenchmark_%d.py" % obs_per_mc,obs_per_mc)
        append_74X("sysAndObsBenchmark_%d.py" % obs_per_mc,obs_per_mc)
        append_suites("sysAndObsBenchmark_%d.py" % obs_per_mc)
        return
            
    # Check if output should be sent via email
    send_mail = True
    param=1        
    if len(sys.argv)>1 and sys.argv[param]=="no-mail":
        send_mail=False
        param=param+1
        print "Output will not be sent via email"
    else:
        print "Output will be sent via email"

    systems = sysfile_to_sys.keys()
    algorithms = ["safari","gotcha","cdas"]
    
    if len(sys.argv)>param:
        if sys.argv[param]=="sys":
            systems = [sys.argv[param+1]]
        elif sys.argv[param]=="alg":
            algorithms = [sys.argv[param+1]]
        elif sys.argv[param]=="suite":        
            systems = suite_to_systems[sys.argv[param+1]]
            algorithms = sys.argv[param+2:]
        elif len(sys.argv)>param+1:
            algorithms = [sys.argv[param]]
            systems = [sys.argv[param+1]]
        else:
            print " --------- Usage --------"
            print " loadSysAndObs <obs_per_mc>: creates a file with all the sys and obs of 74XXX and ISCAS85 " 
            print " prepare <script>: run script on all sys and obs files. This is intended for compilation scripts (e.g. sys to cnf, obs to dnf)"
            print " sys <sys_name> : run all algorithms on a specific system "
            print " alg <alg_name> : run a given algorithm on all benchmark systems "
            print " suite <suite_name> <alg_name1> <alg_name2> .... : run a set of algorithms on a given suite of systems (defined in sysAndObsBenchmark) "
            print " <alg_name> <sys_name> : run a given algorithm on a given system "
            return

    
    for algorithm in algorithms:
        print "----------- Running %s ---------- " % algorithm
        alg_details = ALL_ALGORITHMS[algorithm]
        for system_file_name in systems:
            out_file_name = run_alg_on_system_per_obs(alg_details, system_file_name,send_mail)
#            build_commands_for_alg_on_system_per_obs(alg_details, system_file_name,send_mail)
  
 
         
if __name__ == '__main__':
    main()
