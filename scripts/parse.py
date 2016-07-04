import sys
import re

MAX_RUNTIME = 30000
NOT_SOLVED = "N/A"

def print_results_line(algorithm_name, observation_counter, diagnosis_counter, cardinality, runtime):
    """ Prints a line in the results file, corresponding to the data gathered by an algorithm on a given problem instance (SD ^ OBS) """
    
    solved="Found"
    if diagnosis_counter==NOT_SOLVED:
        solved="Not Solved"
    elif runtime>MAX_RUNTIME:
        solved="Not Solved"
    else:
        solved="Solved"
    print "%s,%s,%s,%s,%s,%s" % (algorithm_name, observation_counter, solved, diagnosis_counter, cardinality, runtime) 


def parse_runtime_for_first_diagnosis(results_file_name,algorithm_name):
    """ 
    Parses the results obtained by runDiag.py and outputs:    
    - The name of the algorithms used for the diagnosis.
    - The observation index
    - The diagnosis index
    - The size of the cardinality of the first diagnosis returned.
    - The runtime in seconds of finding the first diagnosis
    """
    first_diagnosis_pattern = \
        re.compile("^(.*): found diagnosis\(# 1\) of cardinality (\d*) @ (\d*) s (\d*\.\d*) ms")    
    found_diagnosis_pattern = \
        re.compile("^(.*): found diagnosis\(# (\d*)\) of cardinality (\d*) @ (\d*) s (\d*\.\d*) ms")    

    done_pattern = \
        re.compile("@Done")
    diag_done_pattern = \
        re.compile("@ ok <diag>")
        
        
    results_file = file(results_file_name,"r")
    
    diagnosis_found = False
    observation_index=1
    successful_obs=0
    average_runtime=0

    for line in results_file:
        # Check if diagnosis was found
        match = found_diagnosis_pattern.search(line)                

        if match is not None:
            values = match.groups()
            runtime = 1000*int(values[3])+float(values[4])
            print_results_line(algorithm_name, observation_index, values[1],values[2],runtime)
            if int(values[1])==1 and runtime<MAX_RUNTIME:
                successful_obs=successful_obs+1
                average_runtime=average_runtime+runtime
            diagnosis_found=True
        else:
            # Check if finished with this observations (finished diag or timed out)
            match = done_pattern.search(line)
            if match is None:
                continue
                
            if diagnosis_found==False:
                print_results_line(algorithm_name, observation_index, NOT_SOLVED,NOT_SOLVED,NOT_SOLVED)
            diagnosis_found=False
            observation_index=observation_index+1  

    results_file.close()
    print("Solved %d out of %d obsrevations under %d seconds." % (successful_obs, observation_index-1, MAX_RUNTIME))
    if successful_obs>0:
        print("Average runtime for solved instances:%.2f " % (average_runtime/successful_obs))  
    

    
def main():
    parse_runtime_for_first_diagnosis(sys.argv[1],sys.argv[2])
    
    
if __name__ == '__main__':
    main()






