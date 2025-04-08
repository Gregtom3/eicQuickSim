#!/usr/bin/env ruby

require 'csv'
require 'fileutils'

# Check if the correct number of arguments is provided
if ARGV.length < 3
  puts "Usage: ./hpc/write_hpc_csv.rb NFILES_PER_JOB NFILES MAX_EVENTS"
  exit 1
end

# Get parameters from the command-line arguments
N_PER_JOB = ARGV[0].to_i
if N_PER_JOB <= 0
  puts "Error: N_PER_JOB must be a positive integer."
  exit 1
end

# Optional parameters: use -1 if not provided
NFILES    = ARGV[1] ? ARGV[1].to_i : -1
MAX_EVENTS = ARGV[2] ? ARGV[2].to_i : -1

# Define energy configurations and collision types
energies = ["5x41", "10x100", "18x275"]
collisions = ["en", "ep"]

# Define the base command for preprocess_HPC
base_command = "./build/preprocess_HPC"
output_dir = "hpc/"

# Create the output directory if it does not exist
Dir.mkdir(output_dir) unless Dir.exist?(output_dir)

# --- STEP 1: Run preprocess_HPC and generate CSVs ---
energies.each do |energy|
  collisions.each do |collision|
    # Define the output CSV file name
    csv_file = "#{output_dir}all_#{energy}_#{collision}.csv"

    # Build the preprocess_HPC command using NFILES and MAX_EVENTS
    command = "#{base_command} \"#{energy}\" #{NFILES} #{MAX_EVENTS} \"#{collision}\" #{csv_file}"

    # Print and execute the command
    puts "Running: #{command}"
    system(command)

    # Check if the command was successful
    if $?.exitstatus != 0
      puts "Error: preprocess_HPC failed for #{energy} and #{collision}. Check logs."
      exit 1
    else
      puts "Successfully created #{csv_file}"
    end
  end
end

# --- STEP 2: Split CSVs into batch files and copy weights files ---
energies.each do |energy|
  collisions.each do |collision|
    # Define the input CSV and output batch directory
    input_csv = "#{output_dir}all_#{energy}_#{collision}.csv"
    batch_dir = "#{output_dir}#{energy}_#{collision}"

    # Clear the batch directory if it exists, then recreate it
    if Dir.exist?(batch_dir)
      puts "Clearing directory: #{batch_dir}"
      FileUtils.rm_rf(batch_dir)
    end
    FileUtils.mkdir_p(batch_dir)

    # Check if the generated CSV exists
    unless File.exist?(input_csv)
      puts "Warning: File not found: #{input_csv}. Skipping..."
      next
    end

    # Read the input CSV
    puts "Processing and splitting #{input_csv}..."
    csv_data = CSV.read(input_csv, headers: true)

    # Split the CSV into batches of N_PER_JOB rows
    batch_number = 1
    csv_data.each_slice(N_PER_JOB) do |batch|
      current_batch = format('%04d', batch_number)
      batch_csv_path = "#{batch_dir}/batch#{current_batch}.csv"
      puts "Writing #{batch_csv_path} with #{batch.size} rows..."

      # Write each batch to a new CSV file
      CSV.open(batch_csv_path, "w") do |csv|
        csv << csv_data.headers # Write the header
        batch.each { |row| csv << row }
      end

      # Copy the weights file for this energy and collision, if it exists,
      # renaming it to batch####_weights.csv
      weights_file_src = "#{output_dir}all_#{energy}_#{collision}_weights.csv"
      if File.exist?(weights_file_src)
        weights_file_dest = "#{batch_dir}/batch#{current_batch}_weights.csv"
        FileUtils.cp(weights_file_src, weights_file_dest)
        puts "Copied weights file to #{weights_file_dest}"
      else
        puts "Warning: Weights file not found: #{weights_file_src}"
      end

      batch_number += 1
    end

    puts "Finished processing #{input_csv}. Created #{batch_number - 1} batches."
  end
end

# --- Save MAX_EVENTS in a small text file in the output directory ---
max_events_file = "#{output_dir}max_events.txt"
File.open(max_events_file, "w") do |file|
  file.puts MAX_EVENTS
end
puts "Saved MAX_EVENTS (#{MAX_EVENTS}) in #{max_events_file}"

puts "All CSVs have been processed and split successfully!"
