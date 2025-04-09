#!/usr/bin/env ruby
# s3tools/submit_analysis_jobs.rb

require 'optparse'
require 'fileutils'
require 'time'

# Default options.
options = {
  analysis: "DIS",   # Allowed: DIS, SIDIS, DISIDIS
  no_slurm: false,
  use_batches: false, # new flag: use per-batch YAML files if true
  pid: nil,          # for SIDIS
  pid1: nil,         # for DISIDIS
  pid2: nil          # for DISIDIS
}

OptionParser.new do |opts|
  opts.banner = "Usage: submit_analysis_jobs.rb -n <project name> [options]"

  opts.on("-n NAME", "--name=NAME", "Project name (required)") do |name|
    options[:name] = name
  end

  opts.on("-a TYPE", "--analysis=TYPE", "Analysis type (DIS, SIDIS, DISIDIS). Default: DIS") do |type|
    options[:analysis] = type.upcase
  end

  opts.on("--pid=PID", Integer, "Final state hadron pid (required for SIDIS)") do |pid|
    options[:pid] = pid
  end

  opts.on("--pid1=PID1", Integer, "First hadron pid (required for DISIDIS)") do |pid1|
    options[:pid1] = pid1
  end

  opts.on("--pid2=PID2", Integer, "Second hadron pid (required for DISIDIS)") do |pid2|
    options[:pid2] = pid2
  end

  opts.on("--no-slurm", "Disable SLURM submission; run jobs locally") do
    options[:no_slurm] = true
  end

  opts.on("--use-batches", "Use per-batch YAML configuration files instead of global config.yaml") do
    options[:use_batches] = true
  end

  opts.on("-h", "--help", "Display Help") do
    puts opts
    exit
  end
end.parse!

# Validate required parameters.
if options[:name].nil? || options[:name].strip.empty?
  puts "Error: You must specify a project name with -n."
  exit 1
end

if options[:analysis] == "SIDIS" && options[:pid].nil?
  puts "Error: For SIDIS analysis, you must specify --pid."
  exit 1
end

if options[:analysis] == "DISIDIS" && (options[:pid1].nil? || options[:pid2].nil?)
  puts "Error: For DISIDIS analysis, you must specify --pid1 and --pid2."
  exit 1
end

project_name = options[:name].strip
analysis_type = options[:analysis]

# Assume the project directory for jobs is in out/<project_name>
base_dir = File.join("out", project_name)
unless Dir.exist?(base_dir)
  puts "Error: Project directory #{base_dir} does not exist in out/."
  exit 1
end

# Check if sbatch is available (unless disabled).
slurm_available = system("which sbatch > /dev/null 2>&1")
use_slurm = slurm_available && !options[:no_slurm]
if use_slurm
  puts "SLURM found. Jobs will be submitted to SLURM."
else
  puts "SLURM not found or disabled. Jobs will be run locally."
end

# Create a SLURM log directory if using SLURM.
if use_slurm
  timestamp = Time.now.strftime("%Y-%m-%d___%H-%M-%S")
  slurm_dir = File.join("hpc", "slurm", timestamp)
  slurm_logdir = File.join(slurm_dir, "log")
  FileUtils.mkdir_p(slurm_dir)
  FileUtils.mkdir_p(slurm_logdir)
  puts "Created SLURM log directory: #{slurm_dir}"
end

# Set analysis executable and macro names.
# Here we assume the analysis macros are run via ROOT.
# For simplicity, we will use the following macro files:
#   DIS:       macros/analysis_DIS.C
#   SIDIS:     macros/analysis_SIDIS.C
#   DISIDIS:   macros/analysis_DISIDIS.C
# We build the appropriate ROOT command accordingly.
macro = case analysis_type
        when "DIS" then "macros/analysis_DIS.C"
        when "SIDIS" then "macros/analysis_SIDIS.C"
        when "DISIDIS" then "macros/analysis_DISIDIS.C"
        else
          puts "Error: Unknown analysis type #{analysis_type}."
          exit 1
        end

# Iterate over each configuration subdirectory (assumed names "config_*")
Dir.glob(File.join(base_dir, "config_*")).each do |config_dir|
  next unless File.directory?(config_dir)

  # Determine the YAML file to use.
  # If --use-batches is specified, then for each batch we'll use the corresponding batch YAML file.
  # Otherwise, we default to the global YAML in the config directory.
  global_yaml = File.join(config_dir, "config.yaml")

  batches_dir = File.join(config_dir, "batches")
  unless Dir.exist?(batches_dir)
    puts "Warning: No batches directory found in #{config_dir}. Skipping."
    next
  end

  # Read max_events from the config directory.
  max_events_file = File.join(config_dir, "max_events.txt")
  unless File.exist?(max_events_file)
    puts "Warning: max_events.txt not found in #{config_dir}. Skipping."
    next
  end
  config_max_events = File.read(max_events_file).to_i

  # For each batch CSV (ignoring *_weights.csv files)
  Dir.glob(File.join(batches_dir, "batch*.csv")).each do |batch_csv|
    next if File.basename(batch_csv) =~ /_weights/
    batch_id = File.basename(batch_csv, ".csv")  # e.g. "batch0001"
    # Determine which YAML file to use.
    if options[:use_batches]
      yaml_config = File.join(batches_dir, "#{batch_id}_config.yaml")
      unless File.exist?(yaml_config)
        puts "Warning: Batch YAML file #{yaml_config} not found. Skipping batch #{batch_id}."
        next
      end
    else
      yaml_config = global_yaml
      unless File.exist?(yaml_config)
        puts "Warning: Global YAML file #{yaml_config} not found. Skipping #{config_dir}."
        next
      end
    end

    # Build job name from config and batch.
    config_basename = File.basename(config_dir)
    job_name = "analysis_#{config_basename}_#{batch_id}"

    # Determine output file for analysis result.
    if use_slurm
      out_dir = File.join(slurm_dir, "out", config_basename)
    else
      out_dir = File.join(config_dir, "analysis_out")
    end
    FileUtils.mkdir_p(out_dir)
    out_file = File.join(out_dir, "#{job_name}.csv")

    # Build the ROOT command based on analysis type.
    case analysis_type
    when "DIS"
      # analysis_DIS.C expects (yaml, batch_csv, max_events)
      cmd_params = %Q{\"#{File.expand_path(yaml_config)}\"}
    when "SIDIS"
      # analysis_SIDIS.C expects (yaml, pid, batch_csv, max_events)
      cmd_params = %Q{\"#{File.expand_path(yaml_config)}\", #{options[:pid]}}
    when "DISIDIS"
      # analysis_DISIDIS.C expects (yaml, pid1, pid2, batch_csv, max_events)
      cmd_params = %Q{\"#{File.expand_path(yaml_config)}\", #{options[:pid1]}, #{options[:pid2]}}
    end

    root_cmd = %Q{root -l -b -q '#{macro}\(#{cmd_params}\)'}
    
    if use_slurm
      # Set SLURM parameters.
      ACCOUNT = "clas12"
      PARTITION = "production"
      MEM_PER_CPU = 4000    # in MB
      CPUS_PER_TASK = 1
      TIME_LIMIT = "24:00:00"
      
      slurm_job_name = job_name
      slurm_output = File.join(slurm_logdir, "#{slurm_job_name}.out")
      slurm_error  = File.join(slurm_logdir, "#{slurm_job_name}.err")
      
      slurm_script = File.join(slurm_dir, "#{job_name}.slurm")
      shell_script = File.join(slurm_dir, "#{job_name}.sh")
      
      File.open(slurm_script, "w") do |f|
        f.puts "#!/bin/bash"
        f.puts "#SBATCH --account=#{ACCOUNT}"
        f.puts "#SBATCH --partition=#{PARTITION}"
        f.puts "#SBATCH --mem-per-cpu=#{MEM_PER_CPU}"
        f.puts "#SBATCH --job-name=#{slurm_job_name}"
        f.puts "#SBATCH --cpus-per-task=#{CPUS_PER_TASK}"
        f.puts "#SBATCH --time=#{TIME_LIMIT}"
        f.puts "#SBATCH --output=#{slurm_output}"
        f.puts "#SBATCH --error=#{slurm_error}"
        f.puts ""
        f.puts "cd #{Dir.pwd}"
        f.puts "#{shell_script}"
      end

      File.open(shell_script, "w") do |f|
        f.puts "#!/bin/bash"
        f.puts "cd #{Dir.pwd}"
        f.puts "#{root_cmd}"
      end

      FileUtils.chmod("+x", slurm_script)
      FileUtils.chmod("+x", shell_script)
      
      puts "Submitting SLURM job #{job_name} for batch #{batch_id} using YAML #{yaml_config}..."
      system("sbatch #{slurm_script}")
    else
      puts "Running job #{job_name} for batch #{batch_id} locally using YAML #{yaml_config}..."
      system(root_cmd)
    end
  end
end

puts "All analysis jobs submitted (or executed) for project '#{project_name}'."
if use_slurm
  puts "SLURM logs and scripts are stored in #{slurm_dir}."
end
