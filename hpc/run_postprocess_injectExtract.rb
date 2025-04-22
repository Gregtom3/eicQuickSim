#!/usr/bin/env ruby
#
# run_postprocess_injectExtract.rb
#
# Drives the ROOT macro macros/postprocess_injectExtract.C for each
# (collision,energy) combination, either locally or via SLURM.

require 'optparse'
require 'fileutils'
require 'time'

options = {
  project:           nil,
  asym_config_path:  nil,
  a_ut:              0.3,
  no_slurm:          false
}

OptionParser.new do |opts|
  opts.banner = "Usage: run_postprocess_injectExtract.rb -n <project> --asym-config-path <path> [options]"

  opts.on("-n NAME", "--name NAME", "Project name (required)") do |n|
    options[:project] = n.strip
  end

  opts.on("--asym-config-path PATH", "Directory of scheme_*.csv files (required)") do |p|
    options[:asym_config_path] = p.strip
  end

  opts.on("--A_UT VALUE", Float, "Asymmetry A_UT (default: 0.3)") do |v|
    options[:a_ut] = v
  end

  opts.on("--no-slurm", "Run all jobs locally instead of SLURM") do
    options[:no_slurm] = true
  end

  opts.on("-h", "--help", "Show this help message") do
    puts opts
    exit
  end
end.parse!

if options[:project].to_s.empty? || options[:asym_config_path].to_s.empty?
  abort "Error: --name and --asym-config-path are both required."
end

COLLISIONS = %w[ep en]
ENERGIES   = %w[5x41 10x100 18x275]

TREE_NAME   = "AnalysisTree"
USE_DEPOL   = true
TARGET_POL  = 0.7

def make_and_run(project, collision, energy, cfg_dir, a_ut, tree_name, use_depol, target_pol, no_slurm)
  cfg_file = File.join(cfg_dir, "scheme_#{energy}.csv")
  unless File.exist?(cfg_file)
    warn "Skipping #{collision} #{energy}: missing #{cfg_file}"
    return
  end

  root_dir = File.join("out", project, "config_#{collision}_#{energy}", "root")
  base     = File.basename(cfg_file, ".csv")
  inj_dir  = File.join("out", project, "config_#{collision}_#{energy}", "injectionResults")
  FileUtils.mkdir_p(inj_dir)
  base     = File.basename(cfg_file, ".csv")
  out_csv  = File.join(inj_dir, "#{base}.csv")
  # Build ROOT command without %Q
  args = "\"#{root_dir}\",\"#{tree_name}\",\"#{cfg_file}\",\"#{out_csv}\",#{a_ut},#{use_depol},#{target_pol},\"#{collision}\""
  root_cmd = "root -l -b -q 'macros/postprocess_injectExtract.C(#{args})'"

  if no_slurm
    puts "[LOCAL] #{collision.upcase} @ #{energy}"
    system(root_cmd) or warn "  Command failed: #{root_cmd}"
  else
    yield collision, energy, root_cmd, out_csv
  end
end

if options[:no_slurm]
  puts "=== Running all jobs locally ==="
  COLLISIONS.each do |c|
    ENERGIES.each do |e|
      make_and_run(
        options[:project],
        c, e,
        options[:asym_config_path],
        options[:a_ut],
        TREE_NAME,
        USE_DEPOL,
        TARGET_POL,
        true
      ) {}
    end
  end
else
  # SLURM settings
  ACCOUNT       = "clas12"
  PARTITION     = "production"
  MEM_PER_CPU   = 4000    # MB
  CPUS_PER_TASK = 16
  TIME_LIMIT    = "24:00:00"

  timestamp = Time.now.strftime("%Y-%m-%d___%H-%M-%S")
  slurm_dir = File.join("hpc","slurm",timestamp)
  log_dir   = File.join(slurm_dir,"log")
  FileUtils.mkdir_p(log_dir)

  run_script = "run_postprocess_jobs.sh"
  File.open(run_script,"w") { |f| f.puts("#!/bin/bash") }

  COLLISIONS.each do |c|
    ENERGIES.each do |e|
      make_and_run(
        options[:project],
        c, e,
        options[:asym_config_path],
        options[:a_ut],
        TREE_NAME,
        USE_DEPOL,
        TARGET_POL,
        false
      ) do |collision, energy, cmd, out_csv|
        job    = "postproc_#{options[:project]}_#{collision}_#{energy}"
        slurm  = File.join(slurm_dir, "#{job}.slurm")
        stdout = File.join(log_dir, "#{job}.out")
        stderr = File.join(log_dir, "#{job}.err")

        File.open(slurm,"w") do |f|
          f.puts("#!/bin/bash")
          f.puts("#SBATCH --account=#{ACCOUNT}")
          f.puts("#SBATCH --partition=#{PARTITION}")
          f.puts("#SBATCH --job-name=#{job}")
          f.puts("#SBATCH --cpus-per-task=#{CPUS_PER_TASK}")
          f.puts("#SBATCH --mem-per-cpu=#{MEM_PER_CPU}")
          f.puts("#SBATCH --time=#{TIME_LIMIT}")
          f.puts("#SBATCH --output=#{stdout}")
          f.puts("#SBATCH --error=#{stderr}")
          f.puts
          f.puts("cd #{Dir.pwd}")
          f.puts(cmd)
        end

        FileUtils.chmod("+x", slurm)
        File.open(run_script,"a") { |f| f.puts("sbatch #{slurm}") }
        puts "Prepared SLURM job #{job}; output CSV: #{out_csv}"
      end
    end
  end

  puts "\nAll SLURM scripts written to #{slurm_dir}"
  puts "Submit them via:\n    bash #{run_script}"
end
