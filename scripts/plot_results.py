from collections import defaultdict
import pickle
import matplotlib.pyplot as plt


def process_single_node_result(filename):
    with open(filename, "rb") as fp:
        res = pickle.load(fp)

    all_pfls = []

    def find_pfl(pfl):
        for i, p in enumerate(all_pfls):
            if p == pfl:
                return i
        all_pfls.append(pfl)
        return len(all_pfls) - 1
        
    pfl_to_res = defaultdict(list)

    # group outputs by PFL
    for result in res:
        exp, pfl, output = result
        idx = find_pfl(pfl)
        pfl_to_res[idx].append((exp, output))

    legend = []
    file_size = ['512M', '4G', '20G', '200G']
    for i, (pfl, vals) in enumerate(pfl_to_res.items()):
        labels = []
        mean = []
        e = []
        legend.append(repr(all_pfls[pfl]))
        for j, (exp, output) in enumerate(vals):
            labels.append(f"File size: {file_size[j//4]} I/O size: {exp[1]}")
            mean.append(output['reads']['mean'])
            e.append(output['reads']['stddev'])
        
        x = list(a + 0.05 * i for a in range(len(mean)))
        plt.errorbar(x, mean, e, linestyle='None', marker='^')        
    plt.ylabel("MB/s")
    plt.xlabel("Experiment configuration")
    plt.title("Average bandwidth (and stddev) reached by reading a file sequentially.")
    plt.xticks(list(a - 0.05 * len(pfl_to_res) / 2 for a in x), labels, rotation=45)
    plt.legend(legend)
    plt.subplots_adjust(bottom=0.3)
    plt.show()


if __name__ == "__main__":
    import sys
    if len(sys.argv) < 2:
        print("No input file specified as first argument.")
        exit(1)
    filename = sys.argv[1]
    process_single_node_result(filename)