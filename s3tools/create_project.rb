#!/usr/bin/env ruby
# s3tools/create_project.rb
require 'optparse'
require 'fileutils'
require 'yaml'

# Default values for numFiles and maxEvents
options = {
  num_files: 3,
  max_events: 1000
}

opts = OptionParser.new do |opts|
  opts.banner = "Usage: create_project.rb -n <project name> [options]"
  
  opts.on("-n NAME", "--name=NAME", "Project name (required)") do |n|
    options[:name] = n
  end

  opts.on("-f NUM", "--num-files=NUM", Integer, "Number of files to process (default: 3)") do |num|
    options[:num_files] = num
  end

  opts.on("-m NUM", "--max-events=NUM", Integer, "Maximum events to process (default: 1000)") do |max|
    options[:max_events] = max
  end

  opts.on("-h", "--help", "Displays Help") do
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

if options[:name].nil? || options[:name].strip.empty?
  puts "Error: You must specify a project name with the -n option."
  puts opts
  exit 1
end

project_name = options[:name].strip
num_files    = options[:num_files]
max_events   = options[:max_events]

# Create the project output directory (out/<project_name>)
base_output_dir = File.join("out", project_name)

# If the project directory already exists, prompt user whether to remove it.
if File.directory?(base_output_dir)
  print "Project '#{project_name}' already exists. Do you want to remove it? (y/n): "
  answer = STDIN.gets.strip.downcase
  if answer.start_with?("y")
    FileUtils.rm_rf(base_output_dir)
    puts "Removed existing project directory."
  else
    puts "Exiting without making changes."
    exit 0
  end
end

# Create the base project directory.
FileUtils.mkdir_p(base_output_dir)

# Define the energy configurations and collision types.
energy_configs = ["5x41", "10x100", "18x275"]
collision_types = ["ep", "en"]

energy_configs.each do |energy_config|
  collision_types.each do |collision|
    # Create subdirectory: config_<collision>_<energy_config>
    config_dir = File.join(base_output_dir, "config_#{collision}_#{energy_config}")
    FileUtils.mkdir_p(config_dir)

    # Define an output CSV file for preprocess_HPC.
    output_csv = File.join(config_dir, "files.csv")

    # Construct the ROOT macro command for preprocess_HPC:
    # It takes arguments: energy_config (string), num_files (int), max_events (int),
    # collision (string), output_csv (string)
    command = %Q{root -l -b -q 'macros/preprocess_HPC.C\(\"#{energy_config}\", #{num_files}, #{max_events}, \"#{collision}\", \"#{output_csv}\"\)'}
    
    puts "Processing project '#{project_name}' (#{collision} - #{energy_config}):"
    puts "  CSV output -> #{output_csv}"
    puts "  Running: #{command}"
    
    unless system(command)
      puts "Error: ROOT command failed for configuration #{collision} #{energy_config}!"
      exit 1
    end
    puts "  Finished processing for configuration #{collision} #{energy_config}."

    # Now create a YAML configuration file for the Analysis macros.
    # For example, for analysis_DIS we use:
    #  - analysis_type: "DIS"
    #  - energy_config: (the current value)
    #  - csv_source: a CSV file (here we use the CSV output from preprocess_HPC;
    #                you might change this if needed, e.g. "artifacts/test.csv")
    #  - max_events: (choose an appropriate value; here we'll use the same as options)
    #  - collision_type: current collision type
    #  - binning_scheme: a fixed path, e.g. "src/bins/example.yaml"
    #  - output_csv: desired path for analysis result (we format a filename using energy & collision)
    yaml_config = {
      "analysis_type"  => "DIS",  # Adjust for each analysis type if needed
      "energy_config"  => energy_config,
      "csv_source"     => output_csv,
      "max_events"     => max_events,
      "collision_type" => collision,
      "binning_scheme" => "src/bins/example.yaml",
      "output_csv"     => File.join(config_dir, "analysis_DIS_#{collision}_#{energy_config}.csv")
    }

    yaml_path = File.join(config_dir, "config.yaml")
    File.open(yaml_path, "w") do |file|
      file.write(yaml_config.to_yaml)
    end
    puts "  Created YAML config: #{yaml_path}"
    
    puts "-" * 60
  end
end

puts "Project '#{project_name}' created successfully with all configurations!"
