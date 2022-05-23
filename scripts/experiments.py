from utils import *


def single_node_experiment():
    # =====================================================================================
    # STEP 1: define Progressive File Layouts to test
    # =====================================================================================
    lyts = [
        [],
        (('-1', '-1', '4M'),),
        (('256M', '1', '4M'), ('4G', '4', '4M'), ('-1', '-1', '4M')),
        (('32M', '1', '4M'), ('256M', '8', '4M'), ('-1', '-1', '4M')),
        (('32M', '1', '4M'), ('4G', '8', '4M'), ('-1', '-1', '4M')),
        (('32M', '1', '4M'), ('4G', '8', '4M'), ('-1', '-1', '1M')),
        (('1M', '1', '1M'), ('64M', '4', '1M'), ('-1', '-1', '1M'))
    ]
    # =====================================================================================
    # STEP 2: define a set of I/O ops to perform
    # =====================================================================================
    
    # experiment set 1 - bandwidth as a function of file size.
    inputs = ['test_1.bin', 'test_2.bin', 'test_3.bin', 'test_4.bin']
    file_size = ['512M', '4G', '20G', '200G']
    file_to_filesize = dict(zip(inputs, file_size))
    block_size = ['10K', '1M', '4M', '16M']
    block_count = [5000]
    read_prob = [1]
    experiments = list(product(inputs, block_size, block_count, read_prob))


    # =====================================================================================
    # STEP 3: Run the set od experiments for each progressive file layout
    # =====================================================================================
    all_results = []
    lss = LustreSetStripe()
    for exp in experiments:
        for l, lyt in enumerate(lyts):
            # cleanup previous output file if exists, and set the PFL to the new one
            filename = exp[0]
            filesize = file_to_filesize[filename]
            if os.path.isfile(filename): os.unlink(filename)
            if len(lyt) > 0:
                lss.update_pfl(lyt)
                lss(filename)
            # at this point the file exists but it is of 0 length. Use "truncate" to make
            # it of the desired size. If we didn't do this, because the file exists in the 
            # metadata server, `fsbw` would just grab its size, that is, 0.
            with open(filename, "w") as fp:
                fp.truncate(parse_size(filesize))
            res = exec_fsbw(filename=exp[0], block_size=exp[1], block_count=exp[2], read_prob=exp[3])
            all_results.append((exp, lyt, res))
    with open("single_node_experiment.bin", "wb") as fp:
        pickle.dump(all_results, fp)


def multi_node_experiment():
    # TODO implement it
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
                res = exec__fsbw(nnodes, filename=exp[0], block_size=exp[1], block_count=exp[2], read_prob=exp[3])
                all_results.append((exp, lyt, nnodes, res))
    with open("results_multiple_nodes.bin", "wb") as fp:
        pickle.dump(all_results, fp)


if __name__ == "__main__":
    single_node_experiment()