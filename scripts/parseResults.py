import sys
import re

def print_results_line(algorithm_name, cardinality, runtime):
    """ Prints a line in the results file, corresponding to the data gathered by an algorithm on a given problem instance (SD ^ OBS) """
    print "%s,%s,%s" % (algorithm_name, cardinality, runtime) 


def parse_runtime_for_first_diagnosis(results_file_name):
    """ 
    Parses the results obtained by runDiag.py and outputs:
    1) The name of the algorithms used for the diagnosis.
    2) The size of the cardinality of the first diagnosis returned.
    3) The runtime in seconds of finding the first diagnosis
    """
    first_diagnosis_pattern = \
        re.compile("^(.*): found diagnosis\(# 1\) of cardinality (\d*) @ (\d*) s (\d*\.\d*) ms")    
    timeout_pattern = \
        re.compile("^(.*): Terminated by timeout @ (\d*) s (\d*\.\d*) ms")
    diag_done_pattern = \
        re.compile("@ ok <diag>")
        
    results_file = file(results_file_name,"r")
    
    diagnosis_found = False
    for line in results_file:
        # Check if diagnosis was found
        match = first_diagnosis_pattern.search(line)        
        if match is not None:
            values = match.groups()
            runtime = 1000*int(values[2])+float(values[3])
            print_results_line(values[0], values[1],runtime)
            diagnosis_found=True
        else:
            # Check if finished with this observations
            match = diag_done_pattern.search(line)
            if match is not None:
                diagnosis_found=False
            elif diagnosis_found==False:          
                # If diagnosis algorithm did not find a diagnosis, check if timeout has occurred       
                match = timeout_pattern.search(line)
                if match is not None:
                    values = match.groups()
                    runtime = 1000*int(values[1])+float(values[2])
                    print_results_line(values[0], "Not found",runtime)                
  
    results_file.close()
    
    
def main():
    parse_runtime_for_first_diagnosis(sys.argv[1])
    
    
if __name__ == '__main__':
    main()






