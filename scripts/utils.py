import pickle
import subprocess
import multiprocessing
import json
from itertools import product
import os



def exec_dd(infile, outfile, bs, count, skip):
    """
    Executes the `dd` linux command with the given arguments and returns the achieved bandwidth.
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