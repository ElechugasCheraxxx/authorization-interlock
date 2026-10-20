# mission_control.rb - The Brain of Interlock Assessment IDE
# Orchestrates high-level operations logic and delegates raw data parsing to Perl.

require 'json'
require 'open3'
require 'optparse'

module Interlock
  class MissionControl
    attr_reader :active_mission, :tactical_context

    def initialize
      @perl_scalpel = File.expand_path("../surgery-pl/surgery_core.pl", __dir__)
      @tactical_context = :reconnaissance
      @intelligence_log = []
    end

    # DSL: Define a operations engagement
    def self.engage(mission_name, &block)
      control = new
      control.instance_eval(&block)
      control
    end

    def mission(name)
      puts "[Ruby-Brain] Initiating Mission: #{name}"
      @active_mission = name
    end

    def set_context(context)
      @tactical_context = context
      puts "[Ruby-Brain] Operations Context shifted to: #{context}"
    end

    # The Orchestration Loop
    def process_agent_output(agent_id, log_path)
      puts "[Ruby-Brain] Intercepting telemetry from #{agent_id}..."

      # Delegate to Perl (The Slave/Scalpel)
      xml_findings = run_perl_surgery(log_path)

      # Intelligence Analysis (The Brain part)
      analyze_intelligence(xml_findings, agent_id)
      xml_findings
    end

    # PLAYBOOK: Golden Ticket Automation
    def playbook_golden_ticket(agent_id, log_path)
      puts "[Ruby-Brain] [PLAYBOOK] Starting Golden Ticket Hunt on #{agent_id}"
      set_context :domain_compromise

      xml_findings = run_perl_surgery(log_path)

      if xml_findings.include?("DOMAIN_CRITICAL") && xml_findings.include?("krbtgt")
        hash_match = xml_findings.match(/<artifact>(.*)<\/artifact>/)
        hash = hash_match ? hash_match[1] : "unknown"
        puts "[Ruby-Brain] [!!!] GOLDEN TICKET MATERIAL ACQUIRED: KRBTGT Hash #{hash}"

        # 1. Store in Operations Engine (C#) via Bridge
        dispatch_to_tactical_engine(xml_findings)

        # 2. Command Atlas-Go to prepare a forged ticket
        trigger_escalation(agent_id, "GOLDEN_TICKET_FORGING_READY")

        # 3. Update UI (Blackboard)
        update_topology(agent_id)

        return "SUCCESS: KRBTGT Captured"
      else
        puts "[Ruby-Brain] [PLAYBOOK] No KRBTGT material found in current log."
        return "FAILURE: No KRBTGT material"
      end
    end

    # PLAYBOOK: Artifact Harvesting
    def playbook_harvest_artifacts(agent_id, target_dir)
      puts "[Ruby-Brain] [PLAYBOOK] Starting Artifact Harvest on #{agent_id} from #{target_dir}"
      set_context :reconnaissance

      # Simulate a command to the agent to find and pack new artifacts
      # In a real scenario, this would trigger a script on the agent side
      timestamp = Time.now.strftime("%Y%m%d.%H%M%S")
      collection_package = "artifacts_#{agent_id}_#{timestamp}.tar.gz"

      puts "[Ruby-Brain] Sending collection command for pattern: *.log, *.db, *.config"

      # Logic inspired by artifactCollector.rb: find latest modified files
      # trigger_agent_action(agent_id, "find_latest_and_pack", target_dir, collection_package)

      # Update EvidenceVault
      puts "[Ruby-Brain] >>> REGISTERING COLLECTION IN EVIDENCEVAULT: #{collection_package}"
      update_evidence_vault(agent_id, collection_package)

      "SUCCESS: Harvested artifacts to #{collection_package}"
    end

    private

    def update_evidence_vault(agent, package)
      puts "[Ruby-Brain] >>> SYNCING #{package} TO EVIDENCEVAULT (ISO8601 Microseconds)"
    end

    def dispatch_to_tactical_engine(xml)
      # Simulación de inyección en C# (Bridge a TacticalXmlContext)
      puts "[Ruby-Brain] >>> INJECTING KRBTGT DATA INTO REPORTFORGE (C#)"
    end

    def run_perl_surgery(file_path)
      puts "[Ruby-Brain] Commanding Perl-Scalpel to dissect: #{File.basename(file_path)}"

      # Use different flags based on the Ruby context
      mode_flag = case @tactical_context
                  when :domain_compromise then "--mode identity"
                  when :network_recon then "--mode network"
                  else "--mode general"
                  end

      command = "perl \"#{@perl_scalpel}\" --file \"#{file_path}\" #{mode_flag} --mach"
      # Con Mach activado, Perl inyecta directamente. Ya no necesitamos capturar stdout masivamente.
      stdout, stderr, status = Open3.capture3(command)

      if status.success?
        stdout
      else
        puts "[Ruby-Error] Scalpel failure: #{stderr}"
        nil
      end
    end

    def analyze_intelligence(xml_data, agent_id)
      return if xml_data.nil? || xml_data.empty?

      # Here Ruby uses its power to correlate data
      if xml_data.include?("IDENTITY_CRITICAL")
        puts "[Ruby-Brain] CRITICAL: Credential detected by #{agent_id}. Escalating priority!"
        trigger_escalation(agent_id, "HIGH_VALUE_IDENTITY")
      elsif xml_data.include?("LATERAL_MOVEMENT")
        puts "[Ruby-Brain] SUCCESS: Pivot point established by #{agent_id}."
        update_topology(agent_id)
      end

      @intelligence_log << { agent: agent_id, data: xml_data, timestamp: Time.now }
    end

    def trigger_escalation(agent, reason)
      puts "[Ruby-Brain] >>> DISPATCHING ESCALATION: #{reason} for #{agent}"
    end

    def update_topology(agent)
      puts "[Ruby-Brain] >>> UPDATING BLACKBOARD TOPOLOGY via #{agent}"
    end
  end
end

# CLI Entry Point
if __FILE__ == $0
  options = {}
  OptionParser.new do |opts|
    opts.banner = "Usage: mission_control.rb [options]"
    opts.on("-a", "--agent AGENT", "Agent ID") { |v| options[:agent] = v }
    opts.on("-l", "--log LOG", "Log file path") { |v| options[:log] = v }
    opts.on("-p", "--playbook PLAYBOOK", "Playbook name") { |v| options[:playbook] = v }
    opts.on("-c", "--context CONTEXT", "Operations context") { |v| options[:context] = v.to_sym }
  end.parse!

  brain = Interlock::MissionControl.new
  brain.set_context(options[:context]) if options[:context]

  if options[:playbook] == "golden_ticket"
    result = brain.playbook_golden_ticket(options[:agent], options[:log])
    puts result
  elsif options[:agent] && options[:log]
    result = brain.process_agent_output(options[:agent], options[:log])
    puts result
  else
    puts "[Ruby-Brain] No action specified. Running diagnostic."
  end
end
