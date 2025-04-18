#!/usr/bin/env ruby
# run_postprocess_injectExtract.rb
#
# This script creates postprocessing jobs for the Python script 
# macros/postprocess_injectExtract.py for all combinations of collision type and energy configuration.
#
# It expects an asymmetry configuration directory that contains:
#   scheme_5x41.csv, scheme_10x100.csv, scheme_18x275.csv
#
# Usage:
#   run_postprocess_injectExtract.rb -n <project name> --asym-config-path <path> [options]
#
# Options:
#   -n, --name               Project name (required)
#   --asym-config-path PATH  Path to the asymmetry configuration directory (required)
#   --A_UT VALUE             Asymmetry amplitude A_UT value (default: 0.3)
#   --no-slurm               Disable SLURM submission; run jobs locally (runs the python script 6 times)
#   -h, --help               Display this help message

require 'optparse'
require 'fileutils'
require 'time'

# Default options
options = {
  project: nil,
  A_UT: 0.3,
  asym_config_path: nil,
  no_slurm: false
}

# Option parsing
OptionParser.new do |opts|
  opts.banner = "Usage: run_postprocess_injectExtract.rb -n <project name> --asym-config-path <path> [options]"

  opts.on("-n NAME", "--name NAME", "Project name (required)") do |name|
    options[:project] = name.strip
  end

  opts.on("--asym-config-path PATH", "Path to asymmetry configuration directory (required)") do |path|
    options[:asym_config_path] = path.strip
  end

  opts.on("--A_UT VALUE", Float, "A_UT value (default: 0.3)") do |value|
    options[:A_UT] = value
  end

  opts.on("--no-slurm", "Disable SLURM submission; run jobs locally") do
    options[:no_slurm] = true
  end

  opts.on("-h", "--help", "Display Help") do
    puts opts
    exit
  end
end.parse!

if options[:project].nil? || options[:asym_config_path].nil? || options[:project].empty? || options[:asym_config_path].empty?
  puts "Error: You must provide both a project name and an asymmetry configuration path."
  exit 1
end

# Define the collision and energy configuration sets.
collisions = ["ep", "en"]
energies   = ["5x41", "10x100", "18x275"]

if options[:no_slurm]
  puts "Running jobs locally (no SLURM)..."
  collisions.each do |collision|
    energies.each do |energy|
      # Construct the config file (expected name: scheme_<energy>.csv)
      config_file = File.join(options[:asym_config_path], "scheme_#{energy}.csv")
      unless File.exist?(config_file)
        puts "Warning: Asymmetry config file not found: #{config_file}. Skipping #{collision} #{energy}."
        next
      end

     
      # Build the command for the Python script.
      cmd = "python macros/postprocess_injectExtract.py --project #{options[:project]} " +
            "--collision #{collision} --energy #{energy} --config #{config_file} --A_UT #{options[:A_UT]}"
      puts "Running job for #{collision.upcase} at #{energy} with config #{config_file}..."
      system(cmd)
      if $?.exitstatus != 0
        puts "Error: job for #{collision} #{energy} failed."
      else
        puts "Job for #{collision} #{energy} completed successfully."
      end
    end
  end
else
  # SLURM job parameters.
  ACCOUNT         = "clas12"
  PARTITION       = "production"
  MEM_PER_CPU     = 4000       # in MB
  CPUS_PER_TASK   = 16
  TIME_LIMIT      = "24:00:00"

  # Create a unique SLURM directory to hold job files and logs.
  timestamp    = Time.now.strftime("%Y-%m-%d___%H-%M-%S")
  slurm_dir    = File.join("hpc", "slurm", timestamp)
  slurm_logdir = File.join(slurm_dir, "log")
  FileUtils.mkdir_p(slurm_dir)
  FileUtils.mkdir_p(slurm_logdir)

  # Create a run_jobs.sh file that will accumulate the sbatch commands.
  run_jobs_file = File.join("run_postprocess_jobs.sh")
  File.open(run_jobs_file, "w") do |file|
    file.puts "#!/bin/bash"
  end

  collisions.each do |collision|
    energies.each do |energy|
      # Construct the asymmetry config file path.
      config_file = File.join(options[:asym_config_path], "scheme_#{energy}.csv")
      unless File.exist?(config_file)
        puts "Warning: Asymmetry config file not found: #{config_file}. Skipping #{collision} #{energy}."
        next
      end

      job_name = "postproc_#{options[:project]}_#{collision}_#{energy}"
      slurm_script = File.join(slurm_dir, "#{job_name}.slurm")
      slurm_output = File.join(slurm_logdir, "#{job_name}.out")
      slurm_error  = File.join(slurm_logdir, "#{job_name}.err")
      shell_script = File.join(slurm_dir, "#{job_name}.sh")

      # Command for the Python postprocessing script.
      cmd = "python macros/postprocess_injectExtract.py --project #{options[:project]} " +
            "--collision #{collision} --energy #{energy} --config #{config_file} --A_UT #{options[:A_UT]}"

      # Write the SLURM job script.
      File.open(slurm_script, "w") do |f|
        f.puts "#!/bin/bash"
        f.puts "#SBATCH --account=#{ACCOUNT}"
        f.puts "#SBATCH --partition=#{PARTITION}"
        f.puts "#SBATCH --mem-per-cpu=#{MEM_PER_CPU}"
        f.puts "#SBATCH --job-name=#{job_name}"
        f.puts "#SBATCH --cpus-per-task=#{CPUS_PER_TASK}"
        f.puts "#SBATCH --time=#{TIME_LIMIT}"
        f.puts "#SBATCH --output=#{slurm_output}"
        f.puts "#SBATCH --error=#{slurm_error}"
        f.puts ""
        f.puts "cd #{Dir.pwd}"
        f.puts "source eicQuickSim/bin/activate"
        f.puts "#{shell_script}"
      end

      # Write the corresponding shell script to call the Python script.
      File.open(shell_script, "w") do |f|
        f.puts "#!/bin/bash"
        f.puts "cd #{Dir.pwd}"
        f.puts cmd
      end

      FileUtils.chmod("+x", slurm_script)
      FileUtils.chmod("+x", shell_script)

      # Append the sbatch command to the run_jobs.sh file.
      File.open(run_jobs_file, "a") do |f|
        f.puts "sbatch #{slurm_script}"
      end

      puts "Added job #{job_name} (collision: #{collision}, energy: #{energy}, config: #{config_file})."
    end
  end

  puts "All SLURM postprocessing jobs created successfully for project '#{options[:project]}'."
  puts "SLURM submission commands have been saved in #{run_jobs_file}."
  puts "To submit all jobs, run: \n\n  bash #{run_jobs_file}\n\n"
end
