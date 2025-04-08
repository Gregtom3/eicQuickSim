#!/usr/bin/env ruby
require 'fileutils'
require 'time'

# --- Load MAX_EVENTS from file ---
max_events_file = "hpc/max_events.txt"
unless File.exist?(max_events_file)
  puts "Error: Could not find #{max_events_file}. Exiting."
  exit 1
end
MAX_EVENTS = File.read(max_events_file).to_i
puts "Loaded MAX_EVENTS from file: #{MAX_EVENTS}"

# Define a mapping from energy configuration to bin scheme YAML file.
BIN_SCHEMES = {
   "5x41"   => "hpc/bins/x_5x41.yaml",
   "10x100" => "hpc/bins/x_10x100.yaml",
   "18x275" => "hpc/bins/x_18x275.yaml"
}

EXECUTABLE = "./build/analysis_DISIDIS"
ACCOUNT = "clas12"
PARTITION = "production"
MEM_PER_CPU = 4000    # in MB
CPUS_PER_TASK = 1
TIME_LIMIT = "24:00:00"

# Get current timestamp for unique log directory.
timestamp = Time.now.strftime("%Y-%m-%d___%H-%M-%S")
slurm_dir = "hpc/slurm/#{timestamp}"
slurm_logdir = "hpc/slurm/#{timestamp}/log"
FileUtils.mkdir_p(slurm_dir)
FileUtils.mkdir_p(slurm_logdir)
puts "Created SLURM log directory: #{slurm_dir}"

# Save the bin schemes and the executable name to the slurm dir.
File.open("#{slurm_dir}/config_info.txt", "w") do |f|
  f.puts "Executable: #{EXECUTABLE}"
  f.puts "Bin Schemes:"
  BIN_SCHEMES.each do |energy, scheme|
    f.puts "  #{energy}: #{scheme}"
  end
end

# Find all hpc subdirectories that match the pattern "energy_collision" (e.g., "5x41_en")
Dir.glob("hpc/*").each do |subdir|
  next unless File.directory?(subdir)
  # Match directories with names like "5x41_en" or "10x100_ep" or "18x275_en"
  if subdir =~ /hpc\/(\d+x\d+)_(en|ep)$/
    energy = $1
    collision = $2
    puts "Processing subdirectory: #{subdir} (energy: #{energy}, collision: #{collision})"
    
    # Select bin scheme based on energy.
    bin_scheme = BIN_SCHEMES[energy]
    if bin_scheme.nil?
      puts "Warning: no bin scheme defined for energy '#{energy}'. Skipping this subdirectory."
      next
    end

    # Get all batch CSV files in this subdirectory.
    Dir.glob("#{subdir}/batch*.csv").each do |csv_file|
      next if File.basename(csv_file).include?("_weights")
      # Extract batch identifier from file name, e.g., "batch0001" from "batch0001.csv"
      batch_id = File.basename(csv_file, ".csv")
      job_name = "analysis_#{energy}_#{collision}_#{batch_id}"

      # Define output and error file paths.
      output_file = "#{slurm_logdir}/#{job_name}.out"
      error_file  = "#{slurm_logdir}/#{job_name}.err"

      out_dir = "#{slurm_dir}/out/#{energy}_#{collision}"
      FileUtils.mkdir_p(out_dir)
      out_file = "#{out_dir}/#{batch_id}.csv"
        
      # Build the command to be run in the SLURM job.
      # Notice that we now use the appropriate bin scheme for the current energy.
      command = "#{EXECUTABLE} \"#{energy}\" \"#{csv_file}\" #{MAX_EVENTS} \"#{collision}\" \"#{bin_scheme}\" \"#{out_file}\""

      # Create a temporary SLURM script file.
      slurm_script = "#{slurm_dir}/#{job_name}.slurm"
      shell_script = "#{slurm_dir}/#{job_name}.sh"
      File.open(slurm_script, "w") do |f|
        f.puts "#!/bin/bash"
        f.puts "#SBATCH --account=#{ACCOUNT}"
        f.puts "#SBATCH --partition=#{PARTITION}"
        f.puts "#SBATCH --mem-per-cpu=#{MEM_PER_CPU}"
        f.puts "#SBATCH --job-name=#{job_name}"
        f.puts "#SBATCH --cpus-per-task=#{CPUS_PER_TASK}"
        f.puts "#SBATCH --time=#{TIME_LIMIT}"
        f.puts "#SBATCH --output=#{output_file}"
        f.puts "#SBATCH --error=#{error_file}"
        f.puts ""
        f.puts "cd /w/hallb-scshelf2102/clas12/users/gmat/eic/epic-analysis/macro/eicQuickSim"
        f.puts "../../../eic-shell3/eic-shell -- #{shell_script}"
      end

      File.open(shell_script, "w") do |f|
        f.puts "#!/bin/bash"
        f.puts "cd /w/hallb-scshelf2102/clas12/users/gmat/eic/epic-analysis/macro/eicQuickSim"
        f.puts "#{command}"
      end
        
      # Make the SLURM script executable.
      FileUtils.chmod("+x", slurm_script)
      FileUtils.chmod("+x", shell_script)
      puts "Submitting job #{job_name} for #{csv_file}..."

      # Submit the job via sbatch.
      system("sbatch #{slurm_script}")
    end
  end
end

puts "All jobs submitted. SLURM logs and scripts are stored in #{slurm_dir}."
