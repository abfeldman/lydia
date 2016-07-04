import sys
import re
from sysAndObsBenchmark import sys_to_obs
from sysAndObsBenchmark import sysfile_to_sys
import pipes


   
def run_single_observation(system_name, observation_name):
    """
    Creates the input for running a single observation over a given system
    """
    print("diag %s %s" % (system_name, observation_name))
    print("fm") 

def usage():
    print("Usage: runDiag <system_file_name> [obs <obs_name>] [<timeout>]")
    print("Note that the system_file_name argument requires the name of the relevant .sys file, without the .sys suffix")
    print("Also, note that the timeout parameter is in seconds, and is optional with a default of 30.")

 
def main():
    """ 
    Runs all the observations of a given system.
    Basically, this outputs the commands that lydia should run.
    Accepts the following arguments:
    - system name (the name of the .sys file) [this argument is mandatory]
    - timeout [this argument is optional, default is 30]
    """
#    output_file_name = "sysAndObsISCAS.py"
#    output_file_name = "sysAndObs74x.py"
#    output_file_name = "sysAndObsBenchmark.py"
#    out_file = file(output_file_name,"w")
#    out_file.close();
#    append_ISCAS_85(output_file_name)
#    append_74X(output_file_name)
#    return
    
    cut_diagnoses=1
    cut_time = 30

    if len(sys.argv)<1:
        usage()
        return
    system_name = sysfile_to_sys[sys.argv[1]]
    observations = sys_to_obs[system_name]
    next_param = 2

    # If running for a single observation
    if len(sys.argv)>3:
        if sys.argv[2]!="obs":
            usage()
            return

        observation_name=sys.argv[3]
        observations=[observation_name]
        next_param = 4

    if len(sys.argv)>next_param:
        cut_time=int(sys.argv[next_param])

    print "cut time %d" % cut_time
    print "cut diagnoses %d" % cut_diagnoses
    for obs in observations:
        run_single_observation(system_name, obs)

    
if __name__ == '__main__':
    main()
