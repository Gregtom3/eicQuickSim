import os
import sys
import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import math
import yaml
module_path = 'src/eicQuickSim'
if module_path not in sys.path:
    sys.path.insert(0, module_path)
from SpinAsymmetry import SpinAsymmetry

def main():
    parser = argparse.ArgumentParser(description="Post-process injection and asymmetry extraction")
    parser.add_argument("--project", required=True, help="Project name (out/<PROJECT_NAME>)")
    parser.add_argument("--collision", required=True, choices=["ep", "en"], help="Collision type ('ep' or 'en')")
    parser.add_argument("--energy", required=True, help="Energy configuration (e.g. '5x41')")
    parser.add_argument("--config", required=True, help="Path to injection config CSV file (bin definitions)")
    parser.add_argument("--A_UT", type=float, required=True, help="Asymmetry amplitude A_UT value (e.g. 0.3)")
    args = parser.parse_args()

    # Construct directories.
    base_dir = os.path.join("out", args.project, f"config_{args.collision}_{args.energy}")
    root_dir = os.path.join(base_dir, "root")
    summary_dir = os.path.join(base_dir, "injectionSummary")
    os.makedirs(summary_dir, exist_ok=True)

    # Load the injection configuration bin definition (CSV).
    try:
        bin_df = pd.read_csv(args.config, sep=None, engine='python')
    except Exception as e:
        sys.exit(f"Error reading bin configuration file: {e}")

    # Instantiate the refactored SpinAsymmetry.
    # For simplicity, we use a default target polarization of 0.7.
    targetPol = 0.7
    sa = SpinAsymmetry(root_dir, tree_name="AnalysisTree", collision=args.collision,
                       targetPol=targetPol, use_depol=True)

    # Create a 2 x 3 panel of histograms.
    # We will plot three variables: "x", "Q2", and "y".
    # For each variable, we produce one histogram using weight="weight" and one using weight="1".
    fig, axs = plt.subplots(2, 3, figsize=(18, 10))
    
    # Define binning parameters.
    n_bins = 100
    # For "x": logarithmic bins.
    x_min = 1e-3
    x_max = 1e0
    log_bins_x = np.array([10**(math.log10(x_min) + i*(math.log10(x_max)-math.log10(x_min))/n_bins) for i in range(n_bins+1)])
    # For "Q2": logarithmic bins.
    Q2_min = 1e0
    Q2_max = 1e4
    log_bins_Q2 = np.array([10**(math.log10(Q2_min) + i*(math.log10(Q2_max)-math.log10(Q2_min))/n_bins) for i in range(n_bins+1)])
    # For "y": linear bins.
    lin_bins_y = np.linspace(0, 1, 100)
    
    # List of variables and corresponding bins.
    variables = [("x", log_bins_x, True, True), 
                 ("Q2", log_bins_Q2, True, True), 
                 ("y", lin_bins_y, False, False)]
    # Two weight options.
    weight_list = ["weight", "1"]
    
    for row, wt in enumerate(weight_list):
        for col, (var, bins, logx, logy) in enumerate(variables):
            ax = axs[row, col]
            # Plot the histogram.
            sa.plot_1d(var=var, bins=bins, weight=wt, logx=logx, logy=logy, ax=ax)
            ax.set_title(f"{var} (weight={wt})")
    
    # Get total number of entries for the selected collision type.
    nentries = sa.get_nentries()
    
    # Add a suptitle with the collision type, energy configuration, and total entries.
    fig.suptitle(f"Injection Summary for {args.collision.upper()} at {args.energy} | Total entries: {nentries}", fontsize=16)
    
    # Save the 1D histogram figure.
    histo_fig_path = os.path.join(summary_dir, "1d_histograms.png")
    fig.savefig(histo_fig_path, bbox_inches="tight")
    plt.close(fig)
    print(f"Saved histogram figure to: {histo_fig_path}")
    
    # Perform injection of the asymmetry.
    sa.inject(A_UT=args.A_UT, show_plot=False)
    
    # Extract the asymmetry using the provided bin configuration.
    asym_result = sa.extractAsymmetry(bin_df=bin_df)
    asym_csv_path = os.path.join(summary_dir, "asymmetry_extraction.csv")
    asym_result.to_csv(asym_csv_path, index=False)
    print(f"Saved asymmetry extraction results to: {asym_csv_path}")
    
    # Save user input parameters and total entries to a YAML file.
    params = {
        "collision": args.collision,
        "energy_configuration": args.energy,
        "targetPol": targetPol,
        "A_UT": args.A_UT,
        "nEntries": nentries
    }
    yaml_path = os.path.join(summary_dir, "injection_parameters.yaml")
    with open(yaml_path, "w") as f:
        yaml.dump(params, f, default_flow_style=False)
    print(f"Saved injection parameters to: {yaml_path}")
    
if __name__ == "__main__":
    main()
