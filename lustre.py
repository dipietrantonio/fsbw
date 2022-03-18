import subprocess
import multiprocessing
from itertools import product
from statistics import mean, stdev
import os



def exec_dd(infile, outfile, bs, count, skip):
    """
    Executes the dd linux command with the given arguments and returns the achieved bandwidth.
    """
    result = subprocess.run(["dd", f"if={infile}", f"of={outfile}", f"bs={bs}", f"count={count}", f"skip={skip}"], stderr=subprocess.PIPE)
    out = result.stderr.decode("utf-8")
    lines = out.splitlines()
    bandwidth_line = lines[2]
    bandwidth =  bandwidth_line.split(',')[-1].strip()
    number, unit = bandwidth.split()
    unit_dict = {
        'TB/s' : 1024,
        'GB/s' : 1,
        'MB/s' : 1.0/1024,
        'KB/s' : 1.0/(1024**2)
    }
    number = float(number) * unit_dict[unit]
    return number



def run_multiple_dd_in_parallel(argument_sets):
    """
    Runs multiple dd commands in parallel, one for each argument set.

    Parameters
    ----------
    - argument_sets: a list of tuples, where each tuple is the argument list to a `exec_dd` function call.

    Returns
    -------
    - List of floats, the ith value is the return value of the ith `exec_dd` call.
    """
    with multiprocessing.Pool(len(argument_sets)) as p:
        bandwidths = p.map(lambda x: exec_dd(*x), argument_sets)
    return bandwidths



class LustreSetStripe:
    """
    Wraps the lfs setstripe command in a nice Python interface.
    """
    def __init__(self):
        self._pfl = [('256M', '1', '4M'), ('4G', '4', '4M'), ('-1', '-1', '32M')]
    
    def update_pfl(self, pfl):
        self._pfl = pfl
    
    def setstripe(self, path):
        args = []
        for extent in self._pfl:
            for x in zip(['-E', '-c', '-S'], extent):
                args.extend(x)
        args.append(path)
        subprocess.run(["lfs", "setstripe"] + args)


    def __call__(self, paths):
        if isinstance(paths, str):
            self.setstripe(paths)
        else:
            for path in paths:
                self.setstripe(path)




def process_bandwidths(bws):
    return mean(bws), stdev(bws)



def add_output_file(argument_sets):
    """
    Adds the output file argument to each argument list. It is done in this way to be able to
    specify a different output file for each other parameters combination.
    """
    new_exps = []
    for i, (inp, bs, counts, skip) in enumerate(argument_sets):
        new_exps.append((inp, f"test_out_{i}", bs, counts, skip))
    return new_exps
        


if __name__ == "__main__":
    # =====================================================================================
    # STEP 1: define Progressive File Layouts to test
    # =====================================================================================
    lyts = [
        [('-1', '-1', '4M')],
        [('256M', '1', '4M'), ('4G', '4', '4M'), ('-1', '-1', '32M')],
        [('32M', '1', '4M'), ('256G', '8', '4M'), ('-1', '-1', '32M')],

    ]
    # =====================================================================================
    # STEP 2: define a set of I/O ops to perform
    # =====================================================================================
    
    # experiment set 1 - bandwidth as a function of file size.
    inputs = ['/dev/zero']
    bs = ['4M']
    counts = list(range(1, 1024, 20))
    skip = [0]
    # experiment set 2 - bandwidth as a function of read/write size.
    inputs = ['/dev/zero']
    bs = ['4K', '4M', '32M', '256M']
    counts = ['20']

    # experiment set 3 - bandwidth as a function of file concurrency
    inputs = ['1_gb_file']
    bs = ['32M']
    counts = [1]
    skip = list(f"{x}M" for x in range(0, 1024, 64))

    # continue here

    lss = LustreSetStripe()
    for l, lyt in enumerate(lyts):
        print("======== Testing PFL", lyt)
        lss.update_pfl(lyt)
            
        # cleanup previous output filesp
        experiments = add_output_file(product(inputs, bs, counts, skip))
        for _, out, _, _ , _ in experiments:
            if os.path.isfile(out): os.unlink(out)
            lss(out)
            
        bandwidths = run_multiple_dd_in_parallel(experiments)

        with open(f"results_{l}.txt", "w") as fp:
            for exp, bw in zip(experiments, bandwidths):
                fp.write(f"Experiment: {exp} -> bw {bw}\n")
        print("Mean and Stdev of bandwidth: {}, {}".format(*process_bandwidths(bandwidths)))
