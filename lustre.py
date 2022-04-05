import pickle
import subprocess
import multiprocessing
import json
from itertools import product
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



def exec_fsbw(filename, file_size = '512M', block_size = '16M', random = False, block_count = None, read_prob = 0.5):
    args = ["fsbw", f"-f{filename}", f"-S{file_size}", f"-B{block_size}", f"-p{read_prob}", "-j", "-m"]
    if random:
        args.append("-r")
    if block_count is not None:
        args.append(f"-c{block_count}")
        
    result = subprocess.run(args, stdout=subprocess.PIPE)
    text_output = result.stdout.decode("utf-8")
    json_output = json.loads(text_output)
    return json_output



def exec_parallel_fsbw(ntasks, filename, file_size = '512M', block_size = '16M', random = False, block_count = None, read_prob = 0.5):
    args = f'sbatch --wait -N {ntasks} --tasks-per-node=1 -p standard --account project_462000053 --mem=40G --time=00:30:00 --wrap'.split()
    fsbw_str = f"srun fsbw -f{filename} -S{file_size} -B{block_size} -p{read_prob} -j -m"
    if random:
        fsbw_str += " -r"
    if block_count is not None:
        fsbw_str += f" -c{block_count}"
    
    args.append(fsbw_str)
    result = subprocess.run(args, stdout=subprocess.PIPE)
    print(result.stdout)
    text_output = result.stdout.decode("utf-8").splitlines()
    json_output = []
    for line in text_output[1:]:
        json_output.append(json.loads(line))
    return json_output


def run_experiments_in_parallel(argument_sets):
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


def parse_size(spec):
    unit = spec[-1]
    if unit.isdigit():
        return int(spec)
    else:
        base = int(spec[:-1])
        if unit == 'K': return base * 1024
        elif unit == 'M': return base * 1024**2
        elif unit == 'G': return base * 1024**3
        else: raise ValueError()


def _objective_help(avg_bw, stddev_bw):
    return avg_bw / stddev_bw



def objective(result):
    obj = 0
    if 'writes' in result:
        obj += _objective_help(result['writes']['mean'], result['writes']['stddev'])
    if 'reads' in result:
        obj += _objective_help(result['reads']['mean'], result['reads']['stddev'])
    return obj / 2




if __name__ == "__main__":
    # =====================================================================================
    # STEP 1: define Progressive File Layouts to test
    # =====================================================================================
    lyts = [
        [],
        (('-1', '-1', '4M'),),
        (('256M', '1', '4M'), ('4G', '4', '4M'), ('-1', '-1', '4M')),
        (('32M', '1', '4M'), ('256G', '8', '4M'), ('-1', '-1', '4M')),
        (('32M', '1', '4M'), ('4G', '8', '4M'), ('-1', '-1', '4M')),
        (('32M', '1', '4M'), ('4G', '8', '4M'), ('-1', '-1', '1M')),
        (('1M', '1', '1M'), ('64M', '4', '1M'), ('-1', '-1', '1M'))
    ]
    # =====================================================================================
    # STEP 2: define a set of I/O ops to perform
    # =====================================================================================
    
    # experiment set 1 - bandwidth as a function of file size.
    inputs = ['test_1.bin',] #'test_2.bin', 'test_3.bin',] #'test_4.bin']
    file_size = ['20G'] # ['512M', '4G', '20G', '200G']
    file_to_filesize = dict(zip(inputs, file_size))
    block_size = ['4M'] #['10K', '1M', '4M', '16M']
    block_count = [5000]
    read_prob = [1]
    experiments = list(product(inputs, block_size, block_count, read_prob))
    # Run the set od experiments for each progressive file layout
    all_results = []
    lss = LustreSetStripe()
    for exp in experiments:
        for l, lyt in enumerate(lyts):
            print("======== Testing PFL", lyt)
            if len(lyt) > 0:
                lss.update_pfl(lyt)
            for nnodes in [1, 2, 4, 8, 16]:
                # cleanup previous output file if exists, and set the PFL to the new one
                filename = exp[0]
                if os.path.isfile(filename): os.unlink(filename)
                if len(lyt) > 0: lss(filename)
                # create the file with a specific length
                with open(filename, "w") as fp:
                    fp.truncate(parse_size(file_to_filesize[filename]))
                res = exec_parallel_fsbw(nnodes, filename=exp[0], block_size=exp[1], block_count=exp[2], read_prob=exp[3])
                all_results.append((exp, lyt, nnodes, res))
    with open("results_multiple_nodes.bin", "wb") as fp:
        pickle.dump(all_results, fp)
