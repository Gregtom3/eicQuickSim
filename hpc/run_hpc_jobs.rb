#!/usr/bin/env ruby
# run_project_analysis.rb
#
# This combined script first splits your CSV files into batch files
# (and writes per-batch YAML config files) and then creates job submission
# commands (either locally or for SLURM) for each batch.
#
# Usage:
#   run_project_analysis.rb -n <project name> -f <nrows per batch> [options]
#
# Options:
#   -n, --name       Project name (required)
#   -f, --num-files  Number of rows per batch file (required)
#   -a, --analysis   Analysis type: DIS, SIDIS, or DISIDIS (default: DIS)
#   --pid            Final state hadron pid (required for SIDIS)
#   --pid1           First hadron pid (required for DISIDIS)
#   --pid2           Second hadron pid (required for DISIDIS)
#   --no-slurm       Disable SLURM submission and run jobs locally
#   -h, --help       Display this help message

require 'optparse'
require 'fileutils'
require 'csv'
require 'yaml'
require 'time'

# -------------------------
# Option Parsing
# -------------------------
options = {
  num_files: nil,    # required for splitting: number of rows per batch file
  name: nil,         # required: project name
  analysis: "DIS",   # analysis type: DIS, SIDIS, DISIDIS (default DIS)
  no_slurm: false,   # run locally if true (otherwise use SLURM if available)
  pid: nil,          # for SIDIS
  pid1: nil,         # for DISIDIS
  pid2: nil          # for DISIDIS
}

opts = OptionParser.new do |opts|
  opts.banner = "Usage: run_project_analysis.rb -n <project name> -f <nrows per batch> [options]"

  opts.on("-n NAME", "--name=NAME", "Project name (required)") do |name|
    options[:name] = name
  end

  opts.on("-f NUM", "--num-files=NUM", Integer, "Number of rows per batch file (required)") do |num|
    options[:num_files] = num
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

  opts.on("-h", "--help", "Display Help") do
    puts opts
    exit
  end
end

begin
  opts.parse!
rescue OptionParser::ParseError => e
  puts e.message
  puts opts
  exit 1
end

# -------------------------
# Validate Required Options
# -------------------------
if options[:name].nil? || options[:name].strip.empty?
  puts "Error: You must specify a project name with -n."
  puts opts
  exit 1
end

if options[:num_files].nil? || options[:num_files] <= 0
  puts "Error: You must specify a positive number of rows with -f."
  puts opts
  exit 1
end

if options[:analysis] == "SIDIS" && options[:pid].nil?
  puts "Error: For SIDIS analysis, you must specify --pid."
  puts opts
  exit 1
end

if options[:analysis] == "DISIDIS" && (options[:pid1].nil? || options[:pid2].nil?)
  puts "Error: For DISIDIS analysis, you must specify both --pid1 and --pid2."
  puts opts
  exit 1
end

project_name = options[:name].strip
n_per_job    = options[:num_files]
analysis_type = options[:analysis]

# SLURM job parameters.
ACCOUNT = "clas12"
PARTITION = "production"
MEM_PER_CPU = 2000    # in MB
CPUS_PER_TASK = 1
TIME_LIMIT = "24:00:00"

# -------------------------
# Verify Project Directory
# -------------------------
base_dir = File.join("out", project_name)
unless Dir.exist?(base_dir)
  puts "Error: Project directory #{base_dir} does not exist. Please create your project first."
  exit 1
end

# -------------------------
# CSV Splitting Section
# -------------------------
overwrite_batches_choice = nil

Dir.glob(File.join(base_dir, "config_*")).each do |config_dir|
  next unless File.directory?(config_dir)

  root_dir = File.join(config_dir, "root")
  csv_dir = File.join(config_dir, "csv")
  FileUtils.mkdir_p(root_dir)
  FileUtils.mkdir_p(csv_dir)
  puts "Processing config directory: #{config_dir}"

  # Batches subdirectory in each configuration directory
  batch_dir = File.join(config_dir, "batches")
  if Dir.exist?(batch_dir)
    if overwrite_batches_choice.nil?
      print "Batches already exist in #{config_dir}. Remove old batches and create new ones? (y/n): "
      answer = STDIN.gets.strip.downcase
      if answer.start_with?("y")
        overwrite_batches_choice = true
        FileUtils.rm_rf(batch_dir)
        puts "Removed old batches in #{config_dir}."
      else
        overwrite_batches_choice = false
        puts "Skipping #{config_dir}..."
        next
      end
    else
      if overwrite_batches_choice
        FileUtils.rm_rf(batch_dir)
        puts "Removed old batches in #{config_dir} (global choice)."
      else
        puts "Skipping #{config_dir} (global choice: do not overwrite batches)."
        next
      end
    end
  end
  FileUtils.mkdir_p(batch_dir)

  # Find the CSV file (generated by preprocess_HPC) in this config directory.
  input_csv = File.join(config_dir, "files.csv")
  unless File.exist?(input_csv)
    puts "Warning: #{input_csv} not found. Skipping this config directory."
    next
  end

  # Read the input CSV (with headers)
  begin
    csv_data = CSV.read(input_csv, headers: true)
  rescue => e
    puts "Error reading #{input_csv}: #{e}"
    next
  end

  # Retrieve max_events if available (from "n_events" column)
  max_event = if csv_data.headers.include?("n_events")
                csv_data.map { |row| row["n_events"].to_i rescue 0 }.max || 0
              else
                0
              end

  # Extract collision type and energy configuration from the config directory name.
  # Expected format: "config_<collision>_<energy>" e.g. "config_en_10x100"
  m = File.basename(config_dir).match(/config_([a-z]+)_(\d+x\d+)/)
  if m
    collision_type = m[1]
    energy_config  = m[2]
  else
    collision_type = "unknown"
    energy_config  = "unknown"
  end

  batch_number = 1
  csv_data.each_slice(n_per_job) do |rows|
    batch_index = format('%04d', batch_number)
    batch_csv_path = File.join(batch_dir, "batch#{batch_index}.csv")
    puts "Writing #{batch_csv_path} with #{rows.size} rows..."

    # Write the batch CSV (include header)
    CSV.open(batch_csv_path, "w") do |csv|
      csv << csv_data.headers
      rows.each { |row| csv << row }
    end

    # Copy weights file if available.
    weights_file_src = File.join(config_dir, "files_weights.csv")
    if File.exist?(weights_file_src)
      weights_file_dest = File.join(batch_dir, "batch#{batch_index}_weights.csv")
      FileUtils.cp(weights_file_src, weights_file_dest)
      puts "Copied weights file to #{weights_file_dest}"
    else
      puts "Warning: Weights file not found: #{weights_file_src}"
    end

    # Create per-batch YAML configuration file.
    yaml_config = {
      "analysis_type"  => "DIS",  # Adjust if a different analysis type is needed.
      "energy_config"  => energy_config,
      "csv_source"     => File.expand_path(input_csv),
      "max_events"     => max_event,
      "collision_type" => collision_type,
      "binning_scheme" => "src/bins/example.yaml",
      "output_csv"     => File.join(csv_dir, "analysis_#{analysis_type}_#{collision_type}_#{energy_config}_batch#{batch_index}.csv"),
      "output_tree"    => File.join(root_dir, "analysis_#{analysis_type}_#{collision_type}_#{energy_config}_batch#{batch_index}.root")
    }
    batch_yaml_file = File.join(batch_dir, "batch#{batch_index}_config.yaml")
    File.open(batch_yaml_file, "w") { |file| file.write(YAML.dump(yaml_config)) }
    puts "Created batch config YAML: #{batch_yaml_file}"

    batch_number += 1
  end

  puts "Finished processing #{input_csv}. Created #{batch_number - 1} batch files in #{batch_dir}."

  # Save MAX_EVENTS value for this configuration if available.
  if csv_data.headers.include?("n_events")
    max_events_file = File.join(config_dir, "max_events.txt")
    begin
      File.open(max_events_file, "w") { |file| file.puts max_event }
      puts "Saved MAX_EVENTS value (#{max_event}) in #{max_events_file}"
    rescue => e
      puts "Error processing max events for #{config_dir}: #{e}"
    end
  else
    puts "Warning: 'n_events' column not found in #{input_csv}. MAX_EVENTS file not created for #{config_dir}."
  end

  puts "-" * 60
end

puts "CSV splitting completed successfully for project '#{project_name}'!"

# -------------------------
# Analysis Job Submission
# -------------------------

timestamp   = Time.now.strftime("%Y-%m-%d___%H-%M-%S")
slurm_dir   = File.join("hpc", "slurm", timestamp)
slurm_logdir = File.join(slurm_dir, "log")
FileUtils.mkdir_p(slurm_dir)
FileUtils.mkdir_p(slurm_logdir)
puts "Created SLURM log directory: #{slurm_dir}"

# Create the run_jobs.sh file in the base project directory.
run_jobs_file = File.join(base_dir, "run_jobs.sh")
File.open(run_jobs_file, "w") do |file|
file.puts "#!/bin/bash"
end


# Set the analysis macro file based on the analysis type.
macro = case analysis_type
        when "DIS" then "macros/analysis_DIS.C"
        when "SIDIS" then "macros/analysis_SIDIS.C"
        when "DISIDIS" then "macros/analysis_DISIDIS.C"
        else
          puts "Error: Unknown analysis type #{analysis_type}."
          exit 1
        end

current_slurm_script = ""

Dir.glob(File.join(base_dir, "config_*")).each do |config_dir|
  next unless File.directory?(config_dir)

  batches_dir = File.join(config_dir, "batches")
  unless Dir.exist?(batches_dir)
    puts "Warning: No batches directory found in #{config_dir}. Skipping."
    next
  end

  max_events_file = File.join(config_dir, "max_events.txt")
  unless File.exist?(max_events_file)
    puts "Warning: max_events.txt not found in #{config_dir}. Skipping."
    next
  end
  config_max_events = File.read(max_events_file).to_i

  # Process each batch CSV file (skip files ending in _weights.csv)
  Dir.glob(File.join(batches_dir, "batch*.csv")).each do |batch_csv|
    next if File.basename(batch_csv) =~ /_weights/
    batch_id = File.basename(batch_csv, ".csv")
    # Always use per-batch YAML file.
    yaml_config = File.join(batches_dir, "#{batch_id}_config.yaml")
    unless File.exist?(yaml_config)
      puts "Warning: Batch YAML file #{yaml_config} not found. Skipping batch #{batch_id}."
      next
    end

    config_basename = File.basename(config_dir)
    job_name = "analysis_#{config_basename}_#{batch_id}"

    # Determine output directory for analysis results.
    out_dir = File.join(slurm_dir, "out", config_basename)
    FileUtils.mkdir_p(out_dir)
    out_file = File.join(out_dir, "#{job_name}.csv")

    # Build ROOT command parameters based on analysis type.
    cmd_params = case analysis_type
                 when "DIS"
                   %Q{"#{File.expand_path(yaml_config)}"}
                 when "SIDIS"
                   %Q{"#{File.expand_path(yaml_config)}", #{options[:pid]}}
                 when "DISIDIS"
                   %Q{"#{File.expand_path(yaml_config)}", #{options[:pid1]}, #{options[:pid2]}}
                 end

    root_cmd = %Q{root -l -b -q '#{macro}\(#{cmd_params}\)'}




    slurm_job_name = job_name
    slurm_output = File.join(slurm_logdir, "#{slurm_job_name}.out")
    slurm_error  = File.join(slurm_logdir, "#{slurm_job_name}.err")
    slurm_script = File.join(slurm_dir, "#{job_name}.slurm")
    shell_script = File.join(slurm_dir, "#{job_name}.sh")
    current_slurm_script = slurm_script

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
    f.puts "../eic-shell3/eic-shell -- #{shell_script}"
    end

    File.open(shell_script, "w") do |f|
    f.puts "#!/bin/bash"
    f.puts "cd #{Dir.pwd}"
    f.puts "#{root_cmd}"
    end

    FileUtils.chmod("+x", slurm_script)
    FileUtils.chmod("+x", shell_script)

    puts "Adding SLURM job command for #{job_name} for batch #{batch_id} using YAML #{yaml_config}..."
    # Instead of immediately submitting the job, append the sbatch command to run_jobs.sh.
    File.open(run_jobs_file, "a") do |f|
    f.puts "sbatch #{slurm_script}"
    end
  end
end

puts "All analysis jobs processed for project '#{project_name}'."
puts "SLURM job submission commands have been saved to #{run_jobs_file}."
puts "To submit all jobs, run: bash #{run_jobs_file} OUTSIDE OF EIC-SHELL"

last_line = File.readlines(current_slurm_script).last.chomp
if match = last_line.match(/(hpc\/.*)/)
    puts "\nFor testing --- Run... \n\n bash #{match[1]}\n\n"
else
    exit 
end

